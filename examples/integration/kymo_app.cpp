/* SPDX-License-Identifier: MIT */
/** @file kymo_app.cpp
 * @brief Fixed example application: bindings, RX and explicit module scheduling.
 * @details No files, simulated time, host deadlines or physical TX offsets here.
 */
#include "kymo_app.h"
#include "kymo_bindings.h"
#include "kymo_port.h"
#include "kymo_tx.h"
#include "kymo_trace_records.h"

/** @brief Fixed round-robin sources; Count defines the wrap point. */
enum class SendSource : uint8_t {
    Telemetry, ParameterReadback, ParameterDescription, WatchReply,
    WatchSample, MemoryInfo, MemorySample, Trace, Count
};
/** @brief Startup records accepted by the mux so far. */
enum class MetadataStage : uint8_t { Session, Dictionary, Complete };

static const size_t RX_BUDGET_BYTES = 16u;
static const uint32_t TRACE_MARKER_PERIOD_CYCLES = 200u;
static const uint32_t TRACE_RECORD_GENERATION = 1u;

static KymoContext sTelemetry;
static KymoParamReceiver sParameters;
static KymoWatch sWatch;
static KymoMemory sMemory;
static KymoTraceRecorder sRecorder;
static KymoMux sMux;
static SendSource sSource = SendSource::Telemetry;
static MetadataStage sMetadataStage = MetadataStage::Session;
static uint8_t sInitAttempted = 0u;
static uint8_t sReady = 0u;
static uint8_t sDraining = 0u;
static uint8_t sEndQueued = 0u;
static uint8_t sDrained = 0u;
static uint32_t sCycle = 0u;
static KymoStatus sError = KYMO_OK;

static_assert(EXAMPLE_TX_CAPACITY >= KYMO_TRACE_FRAME_MAX, "trace frame capacity");
static_assert(KYMO_WATCH_FRAME_MAX_SIZE >= KYMO_PARAM_FRAME_MAX_SIZE, "reply capacity");
static_assert(KYMO_WATCH_FRAME_MAX_SIZE >= KYMO_MEMORY_FRAME_MAX_SIZE, "reply capacity");

/** @brief Remember terminal errors; BUSY/IDLE/SKIPPED are normal flow control. */
/*******************************************************************************
 * checkStatus
 ******************************************************************************/
static void checkStatus(KymoStatus status)
{
    if ((status != KYMO_OK) && (status != KYMO_IDLE) &&
        (status != KYMO_BUSY) && (status != KYMO_SKIPPED)) {
        sError = status;
    }
}

/** @brief Copy one entire module frame; the module keeps no borrowed pointer. */
/*******************************************************************************
 * copyFrame
 ******************************************************************************/
static size_t copyFrame(void *user, const uint8_t *bytes, size_t length)
{
    size_t accepted = 0u;
    KymoStatus status = exampleTxSubmit(bytes, length);
    (void)user;
    checkStatus(status);
    if (status == KYMO_OK) {
        accepted = length;
    }
    return accepted;
}

/** @brief Match the runtime's narrower callback without changing the Core API. */
/*******************************************************************************
 * copyTelemetryFrame
 ******************************************************************************/
static uint8_t copyTelemetryFrame(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t accepted = (uint8_t)copyFrame(user, bytes, length);
    return accepted;
}

/** @brief Encode into temporary storage; submit copies it before return. */
/*******************************************************************************
 * encodeSample
 ******************************************************************************/
static void encodeSample(KymoStatus status, const KymoDataPoint *point)
{
    uint8_t frame[KYMO_FRAME_MAX_SIZE] = {};
    size_t length = 0u;
    checkStatus(status);
    if (status == KYMO_OK) {
        length = KymoProtocol_Encode(point, frame, sizeof(frame));
        if (length == 0u) {
            sError = KYMO_BAD_SAMPLE;
        } else {
            checkStatus(exampleTxSubmit(frame, length));
        }
    }
}

/** @brief Queue each admin record once, retrying the existing mux on BUSY. */
/*******************************************************************************
 * queueTraceMetadata
 ******************************************************************************/
static void queueTraceMetadata(void)
{
    KymoStatus status = KYMO_BUSY;
    if (sMetadataStage == MetadataStage::Session) {
        status = KymoMux_QueueAdmin(&sMux, KYMO_TRACE_PAYLOAD_SESSION_META,
            TRACE_RECORD_GENERATION, exampleTraceMetadata, sizeof(exampleTraceMetadata) - 1u);
    } else if (sMetadataStage == MetadataStage::Dictionary) {
        status = KymoMux_QueueAdmin(&sMux, KYMO_TRACE_PAYLOAD_DICTIONARY,
            TRACE_RECORD_GENERATION, exampleTraceDictionary, sizeof(exampleTraceDictionary) - 1u);
    }
    if (status == KYMO_OK) {
        sMetadataStage = (sMetadataStage == MetadataStage::Session) ?
            MetadataStage::Dictionary : MetadataStage::Complete;
    }
    checkStatus(status);
}

/** @brief Give one source a turn; never consume a reply into occupied storage. */
/*******************************************************************************
 * produceOneFrame
 ******************************************************************************/
static void produceOneFrame(void)
{
    uint8_t reply[KYMO_WATCH_FRAME_MAX_SIZE] = {};
    size_t length = 0u;
    KymoDataPoint point = {};
    KymoStatus status = KYMO_IDLE;
    uint32_t nowMs = examplePortClockMs(nullptr);
    switch (sSource) {
    case SendSource::Telemetry:
        checkStatus(Kymo_Main(&sTelemetry));
        break;
    case SendSource::ParameterReadback:
        status = KymoParam_NextReadback(&sParameters, &point);
        encodeSample(status, &point);
        break;
    case SendSource::ParameterDescription:
        length = KymoParam_NextDescription(&sParameters, reply, sizeof(reply));
        break;
    case SendSource::WatchReply:
        length = KymoWatch_NextReply(&sWatch, reply, sizeof(reply));
        break;
    case SendSource::WatchSample:
        status = KymoWatch_NextSample(&sWatch, nowMs, &point);
        encodeSample(status, &point);
        break;
    case SendSource::MemoryInfo:
        length = KymoMemory_NextInfo(&sMemory, nowMs, reply, sizeof(reply));
        break;
    case SendSource::MemorySample:
        status = KymoMemory_NextSample(&sMemory, nowMs, &point);
        encodeSample(status, &point);
        break;
    case SendSource::Trace:
        queueTraceMetadata();
        if (sError == KYMO_OK) {
            checkStatus(KymoMux_Service(&sMux));
        }
        break;
    default:
        sError = KYMO_BAD_CONFIG;
        break;
    }
    if (length != 0u) {
        checkStatus(exampleTxSubmit(reply, length));
    }
    sSource = static_cast<SendSource>((static_cast<uint8_t>(sSource) + 1u) %
        static_cast<uint8_t>(SendSource::Count));
}

/** @brief Wire each existing module explicitly; initialization failures stop main. */
/*******************************************************************************
 * initializeModules
 ******************************************************************************/
static KymoStatus initializeModules(void)
{
    KymoConfig telemetry = exampleTelemetryBindings();
    KymoWatchConfig watch = exampleWatchBindings();
    KymoMemoryConfig memory = exampleMemoryBindings();
    KymoTraceConfig trace = exampleTraceBindings();
    KymoMuxConfig mux = {};
    KymoStatus status = KYMO_BAD_CONFIG;
    telemetry.write = copyTelemetryFrame;
    /* Both callbacks COPY, so module busy callbacks remain NULL. Only the
     * application buffer can be borrowed by the physical port. */
    mux.recorder = &sRecorder;
    mux.write = copyFrame;
    status = Kymo_Init(&sTelemetry, &telemetry);
    if (status == KYMO_OK) {
        status = KymoParam_Init(&sParameters, exampleParameters, KYMO_COUNT_OF(exampleParameters));
    }
    if (status == KYMO_OK) {
        status = KymoWatch_Init(&sWatch, &watch);
    }
    if (status == KYMO_OK) {
        status = KymoMemory_Init(&sMemory, &memory);
    }
    if (status == KYMO_OK) {
        status = KymoTrace_Init(&sRecorder, &trace);
    }
    if (status == KYMO_OK) {
        status = KymoMux_Init(&sMux, &mux);
    }
    return status;
}

/*******************************************************************************
 * exampleAppInit
 ******************************************************************************/
KymoStatus exampleAppInit(void)
{
    KymoStatus status = KYMO_REJECTED;
    if (sInitAttempted == 0u) {
        sInitAttempted = 1u;
        status = initializeModules();
        sReady = (uint8_t)(status == KYMO_OK);
    }
    return status;
}

/*******************************************************************************
 * exampleAppCycle
 ******************************************************************************/
KymoStatus exampleAppCycle(void)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    uint8_t input[RX_BUDGET_BYTES] = {};
    size_t received = 0u;
    if (sReady != 0u) {
        status = KYMO_REJECTED;
        if (sDraining == 0u) {
            if (sError == KYMO_OK) {
                received = examplePortRead(input, sizeof(input));
                (void)KymoParam_Feed(&sParameters, input, received);
                (void)KymoWatch_Feed(&sWatch, input, received);
                if ((sMetadataStage == MetadataStage::Complete) &&
                    ((sCycle % TRACE_MARKER_PERIOD_CYCLES) == 0u)) {
                    KymoTrace_Mark(&sRecorder, EXAMPLE_TRACE_CONTEXT_ID, EXAMPLE_TRACE_MARKER_ID, sCycle);
                }
                status = exampleTxService();
                checkStatus(status);
                if ((status == KYMO_IDLE) && (sError == KYMO_OK)) {
                    produceOneFrame();
                }
                ++sCycle;
            }
            status = sError;
        }
    }
    return status;
}

/*******************************************************************************
 * exampleAppDrain
 ******************************************************************************/
KymoStatus exampleAppDrain(void)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (sReady != 0u) {
        sDraining = 1u;
        if ((sError == KYMO_OK) && (sDrained == 0u)) {
            status = exampleTxService();
            checkStatus(status);
            if ((status == KYMO_IDLE) && (sError == KYMO_OK)) {
                queueTraceMetadata();
                if ((sMetadataStage == MetadataStage::Complete) && (sEndQueued == 0u)) {
                    status = KymoMux_QueueSessionEnd(&sMux, TRACE_RECORD_GENERATION);
                    checkStatus(status);
                    sEndQueued = (uint8_t)(status == KYMO_OK);
                }
                if (sError == KYMO_OK) {
                    status = KymoMux_Service(&sMux);
                    checkStatus(status);
                    /* IDLE means the mux submitted no frame in this turn. */
                    sDrained = (uint8_t)((sEndQueued != 0u) && (status == KYMO_IDLE));
                }
            }
        }
        status = (sError != KYMO_OK) ? sError : ((sDrained != 0u) ? KYMO_IDLE : KYMO_BUSY);
    }
    return status;
}
