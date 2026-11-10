/* SPDX-License-Identifier: MIT */
/** @file main_native.cpp
 * @brief File startup, simulated runtime and bounded shutdown of the host demo.
 */
#include "kymo_app.h"
#include "kymo_port.h"
#include "kymo_port_native.h"

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
    int result = 1;
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((argc == 3) && (exampleNativeOpen(argv[1], argv[2]) != 0u)) {
        status = exampleAppInit();
        for (uint32_t cycle = 0u; (cycle < 5000u) &&
             (status == KYMO_OK) && (examplePortHealthy() != 0u); ++cycle) {
            examplePortService();
            status = exampleAppCycle();
        }
        if ((status == KYMO_OK) && (examplePortHealthy() != 0u)) {
            status = KYMO_BUSY;
            /* A host-test deadline is never permission to reuse DMA storage. */
            for (uint32_t cycle = 0u; (cycle < 10000u) &&
                 (status == KYMO_BUSY) && (examplePortHealthy() != 0u); ++cycle) {
                examplePortService();
                status = exampleAppDrain();
            }
        }
        result = (status == KYMO_IO_ERROR) ? 2 : ((status == KYMO_IDLE) ? 0 : 1);
        if (exampleNativeClose() == 0u) {
            result = 1;
        }
    }
    return result;
}
