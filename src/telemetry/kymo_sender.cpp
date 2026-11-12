/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_ENABLED && KYMO_ENABLE_CPP
#include "kymo_sender.h"

/*******************************************************************************
 * KymoSender::KymoSender
 ******************************************************************************/
KymoSender::KymoSender()
    : stream_(nullptr), clock_(nullptr), clockUser_(nullptr), startTimeMs_(0u),
      useTimestamp_(KYMO_ENABLE_TIMESTAMP != 0), fault_(false), txLength_(0u), txOffset_(0u),
      buffer_()
{
}

/*******************************************************************************
 * KymoSender::KymoSender
 ******************************************************************************/
KymoSender::KymoSender(KymoStream &stream, bool enableTimestamp)
    : stream_(&stream), clock_(nullptr), clockUser_(nullptr), startTimeMs_(0u),
      useTimestamp_((KYMO_ENABLE_TIMESTAMP != 0) && enableTimestamp), fault_(false), txLength_(0u),
      txOffset_(0u), buffer_()
{
}

/*******************************************************************************
 * KymoSender::begin
 ******************************************************************************/
void KymoSender::begin(KymoStream &stream, bool enableTimestamp)
{
    stream_ = &stream;
    useTimestamp_ = (KYMO_ENABLE_TIMESTAMP != 0) && enableTimestamp;
    startTimeMs_ = 0u;
    txLength_ = 0u;
    txOffset_ = 0u;
    fault_ = false;
}

/*******************************************************************************
 * KymoSender::setClock
 ******************************************************************************/
void KymoSender::setClock(KymoClockFn clock, void *user)
{
    clock_ = clock;
    clockUser_ = user;
}

/*******************************************************************************
 * KymoSender::setStartTime
 ******************************************************************************/
void KymoSender::setStartTime(uint32_t startTimeMs)
{
    startTimeMs_ = startTimeMs;
}

/*******************************************************************************
 * KymoSender::setTimestampEnabled
 ******************************************************************************/
void KymoSender::setTimestampEnabled(bool enable)
{
    useTimestamp_ = (KYMO_ENABLE_TIMESTAMP != 0) && enable;
}

/*******************************************************************************
 * KymoSender::isTimestampEnabled
 ******************************************************************************/
bool KymoSender::isTimestampEnabled() const
{
    bool enabled = useTimestamp_;
    return enabled;
}

/*******************************************************************************
 * KymoSender::sendFrame
 ******************************************************************************/
KymoStatus KymoSender::sendFrame(uint8_t channelId, uint8_t flags, float x, float y, float z,
                                 bool includeTimestamp)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (stream_ == nullptr)
    {
        status = KYMO_BAD_CONFIG;
    }
    else if (fault_)
    {
        status = KYMO_IO_ERROR;
    }
    else if ((txOffset_ != txLength_) || stream_->busy())
    {
        status = KYMO_BUSY;
    }
    else if (!emb_u8_only_bits(flags, KYMO_SUPPORTED_FLAGS) ||
             ((KYMO_ENABLE_TIMESTAMP == 0) && includeTimestamp))
    {
        status = KYMO_BAD_SAMPLE;
    }
    else
    {
        KymoDataPoint point = {};
        point.id = channelId;
        point.flags = flags;
#if KYMO_ENABLE_X
        point.x = x;
#else
        (void)x;
#endif
        point.value = y;
#if KYMO_ENABLE_Z
        point.z = z;
#else
        (void)z;
#endif
#if KYMO_ENABLE_TIMESTAMP
        if (includeTimestamp && useTimestamp_ && (clock_ != nullptr))
        {
            point.flags = EMB_U8_OR(point.flags, KYMO_FLAG_TIMESTAMP);
            point.timestamp_ms = clock_(clockUser_) - startTimeMs_;
        }
#endif
        const size_t length = KymoProtocol_Encode(&point, buffer_, sizeof(buffer_));
        if (length == 0u)
        {
            status = KYMO_BAD_SAMPLE;
        }
        else
        {
            txLength_ = static_cast<uint8_t>(length);
            txOffset_ = 0u;
            status = (flush() == KYMO_IO_ERROR) ? KYMO_IO_ERROR : KYMO_OK;
        }
    }
    return status;
}

/*******************************************************************************
 * KymoSender::send
 ******************************************************************************/
KymoStatus KymoSender::send(uint8_t channelId, float value, bool includeTimestamp)
{
    KymoStatus status = sendFrame(channelId, 0u, 0.0f, value, 0.0f, includeTimestamp);
    return status;
}

/*******************************************************************************
 * KymoSender::send2D
 ******************************************************************************/
KymoStatus KymoSender::send2D(uint8_t channelId, float x, float y, bool includeTimestamp)
{
    KymoStatus status = sendFrame(channelId, KYMO_FLAG_X, x, y, 0.0f, includeTimestamp);
    return status;
}

/*******************************************************************************
 * KymoSender::send3D
 ******************************************************************************/
KymoStatus KymoSender::send3D(uint8_t channelId, float x, float y, float z, bool includeTimestamp)
{
    KymoStatus status =
        sendFrame(channelId, EMB_U8_OR(KYMO_FLAG_X, KYMO_FLAG_Z), x, y, z, includeTimestamp);
    return status;
}

/*******************************************************************************
 * KymoSender::flush
 ******************************************************************************/
KymoStatus KymoSender::flush()
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (stream_ == nullptr)
    {
        status = KYMO_BAD_CONFIG;
    }
    else if (fault_)
    {
        status = KYMO_IO_ERROR;
    }
    else if (txOffset_ == txLength_)
    {
        status = KYMO_OK;
    }
    else if (stream_->busy())
    {
        status = KYMO_BUSY;
    }
    else
    {
        const size_t remaining = static_cast<size_t>(txLength_ - txOffset_);
        const size_t written = stream_->write(buffer_ + txOffset_, remaining);
        if (written > remaining)
        {
            fault_ = true;
            status = KYMO_IO_ERROR;
        }
        else
        {
            txOffset_ = static_cast<uint8_t>(txOffset_ + written);
            status = (txOffset_ == txLength_) ? KYMO_OK : KYMO_BUSY;
        }
    }
    return status;
}

#endif /* KYMO_ENABLED && KYMO_ENABLE_CPP */
