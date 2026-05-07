#ifndef HAL_POTI_H
#define HAL_POTI_H

#include <stdint.h>
#include "../interfaces/poti_inputs.h"

#define HAL_ADC_MAX 4095

uint16_t hal_poti_read_raw(poti_channel_t ch);

float hal_poti_read_normalized(poti_channel_t ch);

extern const poti_input_t HAL_POTI_INPUT;

#endif
