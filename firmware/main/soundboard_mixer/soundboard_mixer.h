#ifndef SOUNDBOARD_MIXER_H
#define SOUNDBOARD_MIXER_H

#include <stdint.h>
#include <stdbool.h>
#include "../interfaces/poti_inputs.h"
#include "hal_buttons_and_switches.h"

#define PLAYLIST_CROSSFADE_STEP 0.05f

typedef struct
{
    float laptop_vol;
    float bluetooth_vol;
    float soundboard_vol;
    float playlist_vol;
} soundboard_mixer_vol_state_t;

typedef enum
{
    DUCK_IDLE,
    DUCK_FADING_DOWN,
    DUCK_HELD,
    DUCK_FADING_UP,
} duck_state_t;

typedef struct
{
    const poti_input_t *potis;
    soundboard_mixer_vol_state_t state;
    hal_audio_source_t active_source;
    bool soundbyte_playing;
    float laptop_ema;
    float bluetooth_ema;
    duck_state_t duck_state;
    float duck_gain;
    float duck_speed_ema;
    float duck_level_ema;
    float soundboard_ema;
    bool  set_pause_active;
    bool  set_pause_prev;
    float pause_gain;
    float pause_level_ema;
    bool  override_active;
    bool  override_prev_music;
    bool  override_prev_tabata;
    float crossfade_gain;
    float playlist_level_ema;
    int   playlist_position;
    int   playlist_length;
    int   playlist_trigger;
} soundboard_mixer_t;

void soundboard_mixer_init(soundboard_mixer_t *mixer, const poti_input_t *potis);
void soundboard_mixer_tick(soundboard_mixer_t *mixer);

// REQ-AUDIO-001
soundboard_mixer_vol_state_t soundboard_mixer_get_vol_states(soundboard_mixer_t *mixer);
hal_audio_source_t soundboard_mixer_get_active_audio_source(soundboard_mixer_t *mixer);
bool soundboard_mixer_is_soundbyte_playing(soundboard_mixer_t *mixer);

// REQ-PLAY-002
void soundboard_mixer_set_playlist_length(soundboard_mixer_t *mixer, int length);
void soundboard_mixer_next_track(soundboard_mixer_t *mixer);
void soundboard_mixer_previous_track(soundboard_mixer_t *mixer);
int  soundboard_mixer_get_playlist_position(soundboard_mixer_t *mixer);
int  soundboard_mixer_get_playlist_trigger(soundboard_mixer_t *mixer);

#endif /* SOUNDBOARD_MIXER_H */
