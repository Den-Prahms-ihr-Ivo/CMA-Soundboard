#include "mock_poti_inputs.h"

#define MOCK_ADC_MAX_VALUE 4095u

static uint16_t mock_values[POTI_COUNT];

static uint16_t mock_read_raw(poti_channel_t ch)
{
    if (ch < 0 || ch >= POTI_COUNT)
        return 0;
    return mock_values[ch];
}

static float mock_read_normalized(poti_channel_t ch)
{
    return (float)mock_read_raw(ch) / (float)MOCK_ADC_MAX_VALUE;
}

const poti_input_t MOCK_POTI_INPUT = {
    .read_raw = mock_read_raw,
    .read_normalized = mock_read_normalized,
};

void mock_poti_input_set_raw(poti_channel_t ch, uint16_t value)
{
    if (ch < 0 || ch >= POTI_COUNT)
        return;
    if (value > MOCK_ADC_MAX_VALUE)
        value = MOCK_ADC_MAX_VALUE;
    mock_values[ch] = value;
}

void mock_poti_input_reset(void)
{
    for (int i = 0; i < POTI_COUNT; ++i)
        mock_values[i] = 0;
}
