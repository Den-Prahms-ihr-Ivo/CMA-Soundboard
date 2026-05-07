/**
 * main.c — Soundboard firmware entry point.
 *
 * Intentionally minimal at this stage. FreeRTOS tasks are created here
 * as each module reaches GREEN in testing. Do not add task creation until
 * the corresponding module tests pass on host.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    /* Soundboard — firmware not yet implemented.
     * See docs/requirements/ for phase status.
     * See docs/journal.md for current progress. */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


