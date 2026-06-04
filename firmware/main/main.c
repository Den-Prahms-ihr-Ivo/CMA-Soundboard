/**
 * main.c — Soundboard firmware entry point.
 *
 * Intentionally minimal at this stage. FreeRTOS tasks are created here
 * as each module reaches GREEN in testing. Do not add task creation until
 * the corresponding module tests pass on host.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    hal_t *hal = hal_esp32_impl();
    // initialise GPIO 35 as output — do this somewhere appropriate
    gpio_set_direction(HAL_PIN_LED_STATUS, GPIO_MODE_OUTPUT);

    bool state = false;
    for (;;)
    {
        hal->led_set(HAL_PIN_LED_STATUS, state);
        state = !state;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
