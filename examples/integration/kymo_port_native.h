/* SPDX-License-Identifier: MIT */
/** @file kymo_port_native.h
 * @brief File lifecycle of the native simulation only.
 * @par Usage Only main_native.cpp opens/closes files; application code uses kymo_port.h.
 */
#ifndef EXAMPLE_KYMO_PORT_NATIVE_H
#define EXAMPLE_KYMO_PORT_NATIVE_H
#include <stdint.h>

/** @brief Open binary request and capture files once per native process.
 * @param[in] inputPath Existing request file; may be empty.
 * @param[in] outputPath Capture file to overwrite; distinct from input.
 * @return 1 on success, 0 on invalid paths or file error.
 * @par Usage Before exampleAppInit; no file paths enter the application.
 */
uint8_t exampleNativeOpen(const char *inputPath, const char *outputPath);
/** @brief Close files only after the port releases all borrowed storage.
 * @return 1 on success, 0 for file errors or outstanding ownership.
 * @par Usage After exampleAppDrain returns KYMO_IDLE, or after a safe failure.
 */
uint8_t exampleNativeClose(void);
#endif
