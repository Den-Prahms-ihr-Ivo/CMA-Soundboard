#ifndef SOUNDBOARD_MIXER_H
#define SOUNDBOARD_MIXER_H

#include <stdint.h>
#include <stdbool.h>
#include "../interfaces/poti_inputs.h"
#include "../hal/hal.h"

typedef struct
{
    uint16_t laptop_gain;
    uint16_t bluetooth_gain;
} soundboard_mixer_gain_state_t;

typedef struct
{
    const poti_input_t *potis;
    soundboard_mixer_gain_state_t state;
} soundboard_mixer_t;

void soundboard_mixer_tick(soundboard_mixer_t *mixer);

// REQ-AUDIO-001
void soundboard_mixer_init(soundboard_mixer_t *mixer, const poti_input_t *potis);

// REQ-AUDIO-001
soundboard_mixer_gain_state_t soundboard_mixer_get_gain_states(soundboard_mixer_t *mixer);

// REQ-AUDIO-001
hal_audio_source_t soundboard_mixer_get_active_audio_source(soundboard_mixer_t *mixer);

// REQ-AUDIO-001
bool soundboard_mixer_is_soundbyte_playing(soundboard_mixer_t *mixer);

#endif /* SOUNDBOARD_MIXER_H */
