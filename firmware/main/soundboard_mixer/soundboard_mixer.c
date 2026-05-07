#include "soundboard_mixer.h"

#define EMA_ALPHA 0.1f

static float ema_update(float prev, float sample)
{
    return EMA_ALPHA * sample + (1.0f - EMA_ALPHA) * prev;
}

void soundboard_mixer_init(soundboard_mixer_t *mixer, const poti_input_t *potis)
{
    mixer->potis           = potis;
    mixer->active_source   = HAL_AUDIO_SOURCE_UNDEFINED;
    mixer->soundbyte_playing = false;
    mixer->laptop_ema      = 0.0f;
    mixer->bluetooth_ema   = 0.0f;
    mixer->state.laptop_gain    = 0.0f;
    mixer->state.bluetooth_gain = 0.0f;
}

void soundboard_mixer_tick(soundboard_mixer_t *mixer)
{
    mixer->active_source = hal_source_switch_read();

    mixer->laptop_ema    = ema_update(mixer->laptop_ema,
                                      mixer->potis->read_normalized(POTI_LAPTOP_GAIN));
    mixer->bluetooth_ema = ema_update(mixer->bluetooth_ema,
                                      mixer->potis->read_normalized(POTI_BLUETOOTH_GAIN));

    switch (mixer->active_source)
    {
        case HAL_AUDIO_SOURCE_LAPTOP:
            mixer->state.laptop_gain    = mixer->laptop_ema;
            mixer->state.bluetooth_gain = 0.0f;
            break;
        case HAL_AUDIO_SOURCE_BLUETOOTH:
            mixer->state.laptop_gain    = 0.0f;
            mixer->state.bluetooth_gain = mixer->bluetooth_ema;
            break;
        default:
            mixer->state.laptop_gain    = 0.0f;
            mixer->state.bluetooth_gain = 0.0f;
            break;
    }

    for (hal_button_t btn = HAL_BUTTON_SB_1; btn <= HAL_BUTTON_SB_6; btn++)
    {
        if (hal_button_is_pressed(btn))
        {
            mixer->soundbyte_playing = true;
            break;
        }
    }
}

soundboard_mixer_gain_state_t soundboard_mixer_get_gain_states(soundboard_mixer_t *mixer)
{
    return mixer->state;
}

hal_audio_source_t soundboard_mixer_get_active_audio_source(soundboard_mixer_t *mixer)
{
    return mixer->active_source;
}

bool soundboard_mixer_is_soundbyte_playing(soundboard_mixer_t *mixer)
{
    return mixer->soundbyte_playing;
}
