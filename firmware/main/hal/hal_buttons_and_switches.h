#ifndef HAL_BUTTONS_H
#define HAL_BUTTONS_H

#include <stdbool.h>
#include <stdint.h>
#include "hal_types.h"

bool hal_button_is_pressed(hal_button_t button);

hal_audio_source_t hal_source_switch_read(void);

void hal_soundbyte_start(hal_button_t btn);
bool hal_soundbyte_is_playing(void);

#endif