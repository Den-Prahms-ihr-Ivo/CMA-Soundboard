#include "soundboard_mixer.h"

void soundboard_mixer_init(soundboard_mixer_t *mixer, const poti_input_t *potis)
{
    // TODO: implement
}

void soundboard_mixer_tick(soundboard_mixer_t *mixer)
{
    /* TODO: implement */
}

soundboard_mixer_gain_state_t soundboard_mixer_get_gain_states(soundboard_mixer_t *mixer)
{
    soundboard_mixer_gain_state_t state;

    state.laptop_gain = 0.0f;
    state.bluetooth_gain = 0.0f;

    return state;
}

hal_audio_source_t soundboard_mixer_get_active_audio_source(soundboard_mixer_t *mixer)
{
    return HAL_AUDIO_SOURCE_UNDEFINED;
}

bool soundboard_mixer_is_soundbyte_playing(soundboard_mixer_t *mixer)
{
    return false;
}