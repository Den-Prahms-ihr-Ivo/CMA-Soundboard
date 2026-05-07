#ifndef HAL_H
#define HAL_H

/**
 * hal.h — Hardware Abstraction Layer for the Soundboard main device.
 *
 * Pattern established in Project 1 (Temptation Box) and carried forward.
 * All hardware access goes through this struct. Application code never calls
 * GPIO, ADC, or I2S APIs directly — it calls through the HAL.
 *
 * hal_esp32.c  — real implementation, compiled for target only
 * hal_mock.c   — mock implementation for host tests
 * hal_mock.h   — mock setter/getter declarations (include in tests only,
 *                never in production code)
 *
 * The Rust audio_mixer crate does NOT call into the HAL. The C audio task
 * reads the HAL, packages the values into MixerInputs, calls the Rust
 * mixer tick, then applies MixerOutputs back through the HAL (ADF pipeline).
 */

#include <stdbool.h>
#include <stdint.h>
#include "hal_buttons_and_switches.h"
#include "hal_potis.h"
#include "hal_types.h"

/* --------------------------------------------------------------------------
 * HAL struct
 * -------------------------------------------------------------------------- */

/**
 * All hardware operations are accessed through this struct.
 * Assign hal_esp32_impl() on device, hal_mock_impl() in host tests.
 */
typedef struct
{
    /**
     * Read a potentiometer ADC channel.
     *
     * @param channel  ADC channel index (HAL_ADC_CH_*)
     * @return         Raw ADC count in [0, HAL_ADC_MAX]
     */
    uint16_t (*adc_read)(uint8_t channel);

    /**
     * Read the physical source switch.
     *
     * @return  HAL_AUDIO_SOURCE_LAPTOP or HAL_AUDIO_SOURCE_BLUETOOTH
     */
    hal_audio_source_t (*source_switch_read)(void);

} hal_t;

void hal_init(void);

/**
 * Returns a pointer to the ESP32-S3 HAL implementation.
 * Only available when compiled for the target.
 * Defined in hal/hal_esp32.c.
 */
hal_t *hal_esp32_impl(void);

/**
 * Returns a pointer to the mock HAL implementation.
 * Available on host and target. Used in all unit tests.
 * Defined in hal/hal_mock.c.
 * Mock setter/getter declarations are in hal_mock.h — include that in tests.
 */
hal_t *hal_mock_impl(void);

#endif /* HAL_H */
