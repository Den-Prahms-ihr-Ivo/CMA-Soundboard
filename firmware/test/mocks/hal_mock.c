#include "hal_mock.h"
#include "../interfaces/poti_inputs.h"
#include <stdint.h>
#include <string.h>

static uint16_t s_adc[POTI_COUNT];
static hal_audio_source_t s_source;

static uint16_t mock_adc_read(uint8_t ch) { return s_adc[ch]; }
static hal_audio_source_t mock_source_switch_read(void) { return s_source; }

static hal_t s_mock_hal = {
    .adc_read = mock_adc_read,
    .source_switch_read = mock_source_switch_read,
};

hal_t *hal_mock_impl(void) { return &s_mock_hal; }

void hal_mock_reset(void)
{
    memset(s_adc, 0, sizeof(s_adc));
    s_source = HAL_AUDIO_SOURCE_UNDEFINED;
}

void hal_mock_set_adc(uint8_t channel, uint16_t value) { s_adc[channel] = value; }
void hal_mock_set_source_switch(hal_audio_source_t source) { s_source = source; }

hal_audio_source_t hal_mock_get_source_switch(void) { return s_source; }
