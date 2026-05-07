#include "hal_potis.h"

#include <stdint.h>

#define HAL_ADC_MAX_VALUE 4095u

/*
 * Private ADC channel mapping.
 * No other module should know these ADC channels.
 */
static const uint8_t poti_adc_channels[POTI_COUNT] = {
    [POTI_LAPTOP_VOL] = 0,
    [POTI_BLUETOOTH_VOL] = 1,
    [POTI_SOUNDBOARD_VOL] = 2,
    [POTI_DUCKING_SPEED] = 3,
    [POTI_DUCKING_LEVEL] = 4,
    [POTI_SET_PAUSE_VOL] = 5,
};

/*
 * Replace this with your real platform ADC read.
 */
static uint16_t read_adc_channel(uint8_t channel)
{
    (void)channel;
    return 0;
}

uint16_t hal_poti_read_raw(poti_channel_t ch)
{
    if (ch < 0 || ch >= POTI_COUNT)
        return 0;
    return read_adc_channel(poti_adc_channels[ch]);
}

float hal_poti_read_normalized(poti_channel_t ch)
{
    return (float)hal_poti_read_raw(ch) / (float)HAL_ADC_MAX_VALUE;
}

const poti_input_t HAL_POTI_INPUT = {
    .read_raw = hal_poti_read_raw,
    .read_normalized = hal_poti_read_normalized,
};
