#include "hal_mock_buttons_and_switches.h"

static bool mock_button_states[HAL_BUTTON_COUNT];

bool hal_button_is_pressed(hal_button_t button)
{
  if (button >= HAL_BUTTON_COUNT)
  {
    return false;
  }

  return mock_button_states[button];
}

void hal_mock_button_set(hal_button_t button, bool pressed)
{
  if (button >= HAL_BUTTON_COUNT)
  {
    return;
  }

  mock_button_states[button] = pressed;
}

void hal_mock_buttons_reset(void)
{
  for (int i = 0; i < HAL_BUTTON_COUNT; ++i)
  {
    mock_button_states[i] = false;
  }
}