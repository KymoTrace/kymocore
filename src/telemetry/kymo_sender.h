/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_sender.h
 * @brief Optional C++ push sender: submit individual points when the application wants.
 *
 * @details Enable KYMO_ENABLE_CPP=1 to compile this API. Use kymo_runtime.h for
 * automatic cyclic sampling; use KymoSender when the application decides itself
 * when a value is sent. The sender keeps one frame and resumes short writes via
 * flush(). It uses the same status codes and clock callback type as the runtime.
 * No hardware ownership, allocation or target-specific overloads are provided;
 * the application derives a KymoStream adapter for its transport.
 * @defgroup kymo_sender C++ sender
 * @{
 * @par Usage
 * @code{.cpp}
 * class UartStream : public KymoStream { ... };
 * UartStream uart;
 * KymoSender sender(uart);
 * if (sender.send(7, 1.0f) == KYMO_OK) { while (sender.flush() == KYMO_BUSY) {} }
 * @endcode
 */
#ifndef KYMO_SENDER_H
#define KYMO_SENDER_H

#include "../kymo_types.h"

#if KYMO_EFFECTIVE_CPP || defined(DOXYGEN)
#include "kymo_protocol.h"

/**
 * @brief Abstract byte output implemented by the application.
 *
 * @details write() reports the accepted prefix; zero applies backpressure.
 * busy() defaults to false, which requires write() to copy the data. Override
 * it for drivers that keep the pointer (DMA, USB) until completion.
 * @par Usage
 * Derive a small adapter for UART, USB, TCP or a test capture buffer.
 */
class KymoStream
{
  public:
    /**
     * @brief Accept a prefix of the offered bytes.
     * @param[in] data Readable bytes owned by the sender.
     * @param[in] length Offered byte count.
     * @return Accepted count in 0..length; larger values latch KYMO_IO_ERROR.
     */
    virtual size_t write(const uint8_t *data, size_t length) = 0;

    /**
     * @brief Report whether the last buffer is still owned by the driver.
     * @return True while in flight or unavailable.
     */
    virtual bool busy() const
    {
        return false;
    }

    /** @brief Virtual destructor for polymorphic adapters. */
    virtual ~KymoStream()
    {
    }
};

/**
 * @brief Single-buffer sender with application-controlled sampling.
 *
 * @details Methods are single-task and non-reentrant. The stream must outlive this
 * object. A pending frame or busy stream rejects new submissions with KYMO_BUSY.
 * Requests for fields disabled by build switches return KYMO_BAD_SAMPLE.
 * @par Usage
 * KymoSender sender(stream); sender.send(0, value); then sender.flush() each cycle.
 */
class KymoSender
{
  public:
    /**
     * @brief Construct a sender without stream; call begin() before sending.
     * @par Usage
     * KymoSender sender; sender.begin(stream);
     */
    KymoSender();

    /**
     * @brief Construct a sender bound to a persistent stream.
     * @param[in,out] stream Application stream; not owned.
     * @param[in] enableTimestamp Master permission for requested timestamps.
     */
    explicit KymoSender(KymoStream &stream, bool enableTimestamp = true);

    /** @brief Non-copyable: the frame buffer may be borrowed by the stream. */
    KymoSender(const KymoSender &other) = delete;

    /**
     * @brief Non-assignable: rebind deliberately with begin().
     * @return Never; deleted.
     */
    KymoSender &operator=(const KymoSender &other) = delete;

    /**
     * @brief Attach or rebind a stream and reset pending state and fault.
     * @param[in,out] stream Persistent stream to use.
     * @param[in] enableTimestamp New master timestamp permission.
     * @pre No driver may still hold the old buffer pointer.
     */
    void begin(KymoStream &stream, bool enableTimestamp = true);

    /**
     * @brief Install or clear the millisecond clock used for timestamps.
     * @param[in] clock Clock callback, or NULL to suppress timestamps.
     * @param[in,out] user Context forwarded to the clock; may be NULL.
     * @par Usage
     * sender.setClock(read_clock_ms, nullptr);
     */
    void setClock(KymoClockFn clock, void *user = nullptr);

    /**
     * @brief Set the clock tick that represents timestamp zero.
     * @param[in] startTimeMs Epoch subtracted from clock readings.
     */
    void setStartTime(uint32_t startTimeMs);

    /**
     * @brief Enable or disable requested timestamps (stays false without KYMO_ENABLE_TIMESTAMP).
     * @param[in] enable New master permission.
     */
    void setTimestampEnabled(bool enable);

    /**
     * @brief Read the master timestamp permission.
     * @return Current permission flag.
     */
    bool isTimestampEnabled() const;

    /**
     * @brief Submit a scalar/Y value.
     * @param[in] channelId Channel identifier 0..255.
     * @param[in] value Finite measurement.
     * @param[in] includeTimestamp Request a timestamp (needs permission and clock).
     * @return KYMO_OK if accepted (call flush() until KYMO_OK), KYMO_BUSY if a frame
     *         is pending or the stream is busy, KYMO_BAD_SAMPLE for invalid data,
     *         KYMO_BAD_CONFIG without stream, KYMO_IO_ERROR after a driver fault.
     */
    KymoStatus send(uint8_t channelId, float value, bool includeTimestamp = false);

    /**
     * @brief Submit an X/Y pair; requires KYMO_ENABLE_X=1.
     * @param[in] channelId Channel identifier 0..255.
     * @param[in] x Finite X coordinate.
     * @param[in] y Finite Y coordinate.
     * @param[in] includeTimestamp Request a timestamp.
     * @return Same codes as send().
     */
    KymoStatus send2D(uint8_t channelId, float x, float y, bool includeTimestamp = false);

    /**
     * @brief Submit an X/Y/Z triple; requires KYMO_ENABLE_X=1 and KYMO_ENABLE_Z=1.
     * @param[in] channelId Channel identifier 0..255.
     * @param[in] x Finite X coordinate.
     * @param[in] y Finite Y coordinate.
     * @param[in] z Finite Z coordinate.
     * @param[in] includeTimestamp Request a timestamp.
     * @return Same codes as send().
     */
    KymoStatus send3D(uint8_t channelId, float x, float y, float z, bool includeTimestamp = false);

    /**
     * @brief Advance a pending short write once.
     * @return KYMO_OK when nothing is pending, KYMO_BUSY while bytes remain or the
     *         stream is busy, KYMO_BAD_CONFIG without stream, KYMO_IO_ERROR after a fault.
     * @par Usage
     * Call once per application cycle after a successful send().
     */
    KymoStatus flush();

  private:
    /**
     * @brief Shared validation, encoding and first write attempt.
     * @param[in] channelId Channel identifier.
     * @param[in] flags Coordinate flags.
     * @param[in] x X value when flagged.
     * @param[in] y Y value.
     * @param[in] z Z value when flagged.
     * @param[in] includeTimestamp Timestamp request.
     * @return Status as documented for send().
     */
    KymoStatus sendFrame(uint8_t channelId, uint8_t flags, float x, float y, float z,
                         bool includeTimestamp);

    /** @brief Borrowed stream, NULL before begin(). */
    KymoStream *stream_;
    /** @brief Optional clock callback for timestamps. */
    KymoClockFn clock_;
    /** @brief Context forwarded to clock_. */
    void *clockUser_;
    /** @brief Epoch subtracted from clock readings. */
    uint32_t startTimeMs_;
    /** @brief Master timestamp permission. */
    bool useTimestamp_;
    /** @brief Latched driver-contract fault. */
    bool fault_;
    /** @brief Current frame length, zero when empty. */
    uint8_t txLength_;
    /** @brief Bytes of the frame already accepted. */
    uint8_t txOffset_;
    /** @brief Persistent storage for the single encoded frame. */
    uint8_t buffer_[KYMO_FRAME_MAX_SIZE];
};

/** @} */
#endif /* KYMO_EFFECTIVE_CPP */
#endif /* KYMO_SENDER_H */
