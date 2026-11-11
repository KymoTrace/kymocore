/* SPDX-License-Identifier: MIT */
/** @file main_native.cpp
 * @brief File startup, simulated runtime and bounded shutdown of the host demo.
 */
#include "kymo_app.h"
#include "kymo_init.h"
#include "features/kymo_trace.h"
#include "kymo_port.h"
#include "kymo_port_native.h"

static const int EXIT_SUCCESSFUL = 0;
static const int EXIT_RUN_FAILURE = 1;
static const int EXIT_TX_CONTRACT_FAILURE = 2;
static const int REQUIRED_ARGUMENT_COUNT = 3;
static const int REQUEST_FILE_ARGUMENT = 1;
static const int CAPTURE_FILE_ARGUMENT = 2;
static const uint32_t SIMULATION_CYCLES = 5000u;
static const uint32_t DRAIN_DEADLINE_CYCLES = 10000u;

/** @brief Run the same application cycle that firmware would schedule.
 * @param[in] argc Must be three.
 * @param[in] argv Program, request file, capture file.
 * @return 0 success, 1 startup/I/O/drain failure, 2 latched TX contract fault.
 */
/*******************************************************************************
 * main
 ******************************************************************************/
int main(int argc, char **argv)
{
    int result = EXIT_RUN_FAILURE;
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((argc == REQUIRED_ARGUMENT_COUNT) &&
        (exampleNativeOpen(argv[REQUEST_FILE_ARGUMENT], argv[CAPTURE_FILE_ARGUMENT]) != 0u)) {
        status = exampleAppInit();
        for (uint32_t cycle = 0u; (cycle < SIMULATION_CYCLES) &&
             ((status == KYMO_OK) || (status == KYMO_BUSY) || (status == KYMO_IDLE)) &&
             (examplePortHealthy() != 0u); ++cycle) {
            exampleTraceCycle(cycle);
            status = KymoApp_Main();
        }
        if (((status == KYMO_OK) || (status == KYMO_BUSY) || (status == KYMO_IDLE)) &&
            (examplePortHealthy() != 0u)) {
            status = KymoApp_RequestStop();
            status = (status == KYMO_OK) ? KYMO_BUSY : status;
            /* A host-test deadline is never permission to reuse DMA storage. */
            for (uint32_t cycle = 0u; (cycle < DRAIN_DEADLINE_CYCLES) &&
                 (status == KYMO_BUSY) && (examplePortHealthy() != 0u); ++cycle) {
                status = KymoApp_Main();
            }
        }
        result = (status == KYMO_IO_ERROR) ? EXIT_TX_CONTRACT_FAILURE :
            ((status == KYMO_IDLE) ? EXIT_SUCCESSFUL : EXIT_RUN_FAILURE);
        if (exampleNativeClose() == 0u) {
            result = EXIT_RUN_FAILURE;
        }
    }
    return result;
}
