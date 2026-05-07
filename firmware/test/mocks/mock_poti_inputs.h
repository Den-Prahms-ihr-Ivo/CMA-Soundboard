#ifndef MOCK_POTI_INPUT_H
#define MOCK_POTI_INPUT_H

#include "../../../firmware/main/interfaces/poti_inputs.h"

extern const poti_input_t MOCK_POTI_INPUT;

void mock_poti_input_set_raw(poti_channel_t ch, uint16_t value);
void mock_poti_input_reset(void);

#endif
