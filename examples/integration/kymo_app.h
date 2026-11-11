/* SPDX-License-Identifier: MIT */
/** @file kymo_app.h
 * @brief Feature selection for the demonstration application.
 * @details Each feature owns its configuration in its named file.
 * @par Usage Initialize once, then call KymoApp_Main from kymo_init.h.
 */
#ifndef EXAMPLE_KYMO_APP_H
#define EXAMPLE_KYMO_APP_H
#include <kymo_types.h>
/** @brief Initialize the base and the five selected demo features.
 * @return Initialization status; no loop starts on error.
 * @par Usage This short composition is the feature selection point. */
KymoStatus exampleAppInit(void);
#endif
