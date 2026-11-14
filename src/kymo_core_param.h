/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_param.h
 * @brief Complete application interface for the default Core parameter feature.
 * @details No receiver or dispatcher state is exposed. Table, names, units and
 * target variables must outlive Core processing. Calls run in the Core task;
 * synchronize access to targets with other tasks/ISRs as required by the device.
 * @par Usage Include with kymo_core.h; configure the table, then call Param Init.
 */
#ifndef KYMO_CORE_PARAM_H
#define KYMO_CORE_PARAM_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_PARAMETERS || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Longest parameter name in bytes, without terminator.
 * @details Parameter Init rejects longer names.
 * @par Usage
 * Keep variable names used with KYMO_PARAM within this length.
 */
#define KYMO_PARAM_NAME_MAX 24u

/**
 * @brief Longest unit text in bytes, without terminator.
 * @details Parameter Init rejects longer units.
 * @par Usage
 * Short units such as "°C" or "rpm".
 */
#define KYMO_PARAM_UNIT_MAX 8u

/**
 * @brief Largest parameter control frame: one description with maximum name and unit.
 * @details Sync (2), descriptor, length, 14 fixed payload bytes, name, unit and CRC.
 * @par Usage
 * Use at least KYMO_PARAM_FRAME_MAX_SIZE bytes for the Core TX buffer.
 */
#define KYMO_PARAM_FRAME_MAX_SIZE (19u + KYMO_PARAM_NAME_MAX + KYMO_PARAM_UNIT_MAX)

/**
 * @brief Table entry for a variable; the variable's name becomes the parameter name.
 *
 * @details Expands to a KymoParameter initializer. Each argument is used once;
 * the name is the variable expression as written, e.g. "kp" or "ctrl.kp".
 * @param id_ Wire ID (0..255), unique in the table.
 * @param variable_ A float lvalue the host may write.
 * @param minimum_ Smallest accepted value.
 * @param maximum_ Largest accepted value.
 * @param unit_ Unit text such as "°C", or "" for none.
 * @par Usage
 * static const KymoParameter params[] = { KYMO_PARAM(100, kp, 0.0f, 10.0f, "") };
 */
#define KYMO_PARAM(id_, variable_, minimum_, maximum_, unit_) \
    {(uint8_t)(id_), &(variable_), (float)(minimum_), (float)(maximum_), #variable_, (unit_)}

    /**
     * @brief One parameter the host may write.
     *
     * @details Owned by the application, typically a const table. The receiver keeps
     * a pointer to the table, so it must outlive the receiver.
     * @par Usage
     * { 100u, &kp, 0.0f, 10.0f } lets the host set kp between 0 and 10 via ID 100.
     */
    typedef struct
    {
        /**
         * @brief Wire ID of writes to this parameter and of its readback.
         * @details Input; unique within the table. Choose IDs that the measurement
         * channels do not use, so readbacks are not mixed with measured values.
         */
        uint8_t id;

        /**
         * @brief Application variable that accepted writes update.
         * @details Input; must not be NULL and must stay valid while the receiver is used.
         */
        float *pTarget;

        /**
         * @brief Smallest accepted value, inclusive.
         * @details Input; finite and not greater than maximum.
         */
        float minimum;

        /**
         * @brief Largest accepted value, inclusive.
         * @details Input; finite and not smaller than minimum.
         */
        float maximum;

        /**
         * @brief Name shown by the host, e.g. "kp".
         * @details Input; NULL or up to KYMO_PARAM_NAME_MAX bytes. NULL sends an empty
         * name and the host shows the ID instead. Must outlive the receiver.
         */
        const char *pName;

        /**
         * @brief Unit shown by the host, e.g. "°C".
         * @details Input; NULL or up to KYMO_PARAM_UNIT_MAX bytes; NULL means none.
         */
        const char *pUnit;
    } KymoParameter;

    /**
     * @brief Parameter-only configuration for the default Core.
     * @details Table entries are referenced, not copied. No hardware callbacks.
     * @par Usage Set pParametersAv and parameterCount before KymoCoreParam_init.
     */
    typedef struct
    {
        /** @brief Persistent parameter definitions. */
        const KymoParameter *pParametersAv;

        /** @brief Entry count, 1..KYMO_MAX_PARAMETERS. */
        size_t parameterCount;
    } KymoCoreParamConfig;

#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

    /**
     * @brief Initialize parameters and activate their processing in Core Main.
     * @details Uses feature-owned static storage; no heap or separate registration.
     * Requires a configured Core with RX and sufficient TX storage. Call once
     * before the first Main. Setup failure latches a Core configuration fault;
     * calls after Main are rejected without changing the running Core.
     * @param[in] pConfig Table configuration; NULL is invalid.
     * @return OK, BAD_CONFIG, REJECTED, or the previously latched Core error.
     * @par Usage Check success before entering the common main loop. Not ISR-safe.
     */
    KymoStatus KymoCoreParam_init(const KymoCoreParamConfig *pConfig);
#endif
#ifdef __cplusplus
}
#endif
#endif
#endif
