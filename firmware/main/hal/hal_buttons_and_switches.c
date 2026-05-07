#include "hal_buttons_and_switches.h"

#include <stdint.h>

/*
 * Private physical pin mapping.
 * No other module should know these pin numbers.
 */
#define HAL_PIN_BTN_SB_1 7
#define HAL_PIN_BTN_SB_2 8
#define HAL_PIN_BTN_SB_3 9
#define HAL_PIN_BTN_SB_4 10
#define HAL_PIN_BTN_SB_5 11
#define HAL_PIN_BTN_SB_6 12
#define HAL_PIN_BTN_SET_PAUSE 13
#define HAL_PIN_SOURCE_SWITCH 14

static const uint8_t button_pins[HAL_BUTTON_COUNT] = {
    [HAL_BUTTON_SB_1] = HAL_PIN_BTN_SB_1,
    [HAL_BUTTON_SB_2] = HAL_PIN_BTN_SB_2,
    [HAL_BUTTON_SB_3] = HAL_PIN_BTN_SB_3,
    [HAL_BUTTON_SB_4] = HAL_PIN_BTN_SB_4,
    [HAL_BUTTON_SB_5] = HAL_PIN_BTN_SB_5,
    [HAL_BUTTON_SB_6] = HAL_PIN_BTN_SB_6,
    [HAL_BUTTON_SET_PAUSE] = HAL_PIN_BTN_SET_PAUSE,
    // TODO: this needs to be extended
};

/*
 * Replace this with your real platform GPIO read.
 * Example:
 *   return gpio_read(pin);
 */
static bool read_gpio_pin(uint8_t pin)
{
  (void)pin;
  return false;
}

bool hal_button_is_pressed(hal_button_t button)
{
  if (button < 0 || button >= HAL_BUTTON_COUNT)
  {
    return false;
  }

  return read_gpio_pin(button_pins[button]);
}

hal_audio_source_t hal_source_switch_read(void)
{
  return HAL_AUDIO_SOURCE_UNDEFINED;
}