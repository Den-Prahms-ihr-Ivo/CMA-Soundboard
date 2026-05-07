#ifndef MOCK_HAL_BUTTONS_H
#define MOCK_HAL_BUTTONS_H

#include "hal_buttons_and_switches.h"

void hal_mock_buttons_reset(void);

/* --------------------------------------------------------------------------
 * Setters — inject hardware state before calling the module under test
 * -------------------------------------------------------------------------- */

/* Set state of button position. */
void hal_mock_button_set(hal_button_t button, bool pressed);

/* Set raw ADC reading for a potentiometer channel (HAL_ADC_CH_*). */
void hal_mock_set_adc(uint8_t channel, uint16_t value);

/* Set the physical source switch position. */
void hal_mock_set_source_switch(hal_audio_source_t source);

#endif