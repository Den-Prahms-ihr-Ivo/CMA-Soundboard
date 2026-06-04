#include "soundboard_mixer.h"

#define EMA_ALPHA        0.1f
#define DUCK_MAX_STEP    0.1f
#define DUCK_FLOOR_MIN   0.05f

static float ema_update(float prev, float sample)
{
    return EMA_ALPHA * sample + (1.0f - EMA_ALPHA) * prev;
}

void soundboard_mixer_init(soundboard_mixer_t *mixer, const poti_input_t *potis)
{
    mixer->potis = potis;
    mixer->active_source = HAL_AUDIO_SOURCE_UNDEFINED;
    mixer->soundbyte_playing = false;
    mixer->laptop_ema = 0.0f;
    mixer->bluetooth_ema = 0.0f;
    mixer->state.laptop_vol = 0.0f;
    mixer->state.bluetooth_vol = 0.0f;
    mixer->duck_state = DUCK_IDLE;
    mixer->duck_gain = 1.0f;
    mixer->duck_speed_ema = 0.0f;
    mixer->duck_level_ema = 0.0f;
    mixer->soundboard_ema = 0.0f;
    mixer->state.soundboard_vol = 0.0f;
    mixer->set_pause_active = false;
    mixer->set_pause_prev   = false;
    mixer->pause_gain       = 1.0f;
    mixer->pause_level_ema  = 0.0f;
    mixer->override_active      = false;
    mixer->override_prev_music  = false;
    mixer->override_prev_tabata = false;
    mixer->crossfade_gain       = 0.0f;
    mixer->playlist_level_ema   = 0.0f;
    mixer->state.playlist_vol   = 0.0f;
    mixer->current_override     = OVERRIDE_NONE;
    mixer->previous_override    = OVERRIDE_NONE;
    mixer->override_prev_chill  = false;
    mixer->chill_level_ema      = 0.0f;
    mixer->prev_source          = HAL_AUDIO_SOURCE_UNDEFINED;
    mixer->playlist_position   = 0;
    mixer->playlist_length     = 0;
    mixer->playlist_trigger    = -1;
}

void soundboard_mixer_tick(soundboard_mixer_t *mixer)
{
    mixer->playlist_trigger = -1;
    hal_audio_source_t new_source = hal_source_switch_read();

    mixer->laptop_ema = ema_update(mixer->laptop_ema,
                                   mixer->potis->read_normalized(POTI_LAPTOP_VOL));
    mixer->bluetooth_ema = ema_update(mixer->bluetooth_ema,
                                      mixer->potis->read_normalized(POTI_BLUETOOTH_VOL));
    mixer->duck_speed_ema = ema_update(mixer->duck_speed_ema,
                                       mixer->potis->read_normalized(POTI_DUCKING_SPEED));
    mixer->duck_level_ema = ema_update(mixer->duck_level_ema,
                                       mixer->potis->read_normalized(POTI_DUCKING_LEVEL));
    mixer->soundboard_ema = ema_update(mixer->soundboard_ema,
                                       mixer->potis->read_normalized(POTI_SOUNDBOARD_VOL));
    mixer->pause_level_ema = ema_update(mixer->pause_level_ema,
                                        mixer->potis->read_normalized(POTI_SET_PAUSE_VOL));
    mixer->playlist_level_ema = ema_update(mixer->playlist_level_ema,
                                           mixer->potis->read_normalized(POTI_PLAYLIST_VOL));
    mixer->chill_level_ema = ema_update(mixer->chill_level_ema,
                                        mixer->potis->read_normalized(POTI_CHILL_VOL));

    /* Button detection — must run before HAL poll so a simultaneous
       complete+new-trigger keeps is_soundbyte_playing true when polled. */
    for (hal_button_t btn = HAL_BUTTON_SB_1; btn <= HAL_BUTTON_SB_6; btn++)
    {
        if (hal_button_is_pressed(btn))
        {
            hal_soundbyte_start(btn);
            break;
        }
    }
    mixer->soundbyte_playing = hal_soundbyte_is_playing();

    if (hal_playlist_track_consume_complete())
    {
        if (mixer->current_override == OVERRIDE_TABATA)
            mixer->current_override = mixer->previous_override;
        else
            soundboard_mixer_next_track(mixer);
    }

    /* Set Pause toggle — rising-edge detection on the button */
    bool set_pause_now = hal_button_is_pressed(HAL_BUTTON_SET_PAUSE);
    if (set_pause_now && !mixer->set_pause_prev)
        mixer->set_pause_active = !mixer->set_pause_active;
    mixer->set_pause_prev = set_pause_now;

    /* Pause gain — fades to pause_level when active, back to 1.0 when not */
    float pause_target = mixer->set_pause_active ? mixer->pause_level_ema : 1.0f;
    float pause_step   = mixer->duck_speed_ema * DUCK_MAX_STEP;
    if (mixer->pause_gain > pause_target)
    {
        mixer->pause_gain -= pause_step;
        if (mixer->pause_gain < pause_target)
            mixer->pause_gain = pause_target;
    }
    else if (mixer->pause_gain < pause_target)
    {
        mixer->pause_gain += pause_step;
        if (mixer->pause_gain > pause_target)
            mixer->pause_gain = pause_target;
    }

    /* Override buttons — uniform rising-edge semantics:
       same mode = exit to NONE, different mode = swap directly, source change = exit */
#define APPLY_OVERRIDE(btn, prev_field, mode)                               \
    do {                                                                     \
        bool _now = hal_button_is_pressed(btn);                              \
        if (_now && !mixer->prev_field) {                                    \
            override_mode_t _next = (mixer->current_override == (mode))     \
                                    ? OVERRIDE_NONE : (mode);               \
            if (_next == OVERRIDE_TABATA)                                   \
                mixer->previous_override = mixer->current_override;         \
            mixer->current_override = _next;                                \
        }                                                                    \
        mixer->prev_field = _now;                                            \
    } while (0)

    APPLY_OVERRIDE(HAL_BUTTON_MUSIC_MODE, override_prev_music,  OVERRIDE_MUSIC);
    APPLY_OVERRIDE(HAL_BUTTON_TABATA,     override_prev_tabata, OVERRIDE_TABATA);
    APPLY_OVERRIDE(HAL_BUTTON_CHILL,      override_prev_chill,  OVERRIDE_CHILL);

#undef APPLY_OVERRIDE

    /* Source switch toggle while override active → exit override */
    if (new_source != mixer->active_source &&
        mixer->active_source != HAL_AUDIO_SOURCE_UNDEFINED &&
        mixer->current_override != OVERRIDE_NONE)
    {
        mixer->current_override = OVERRIDE_NONE;
    }
    mixer->active_source = new_source;

    /* Crossfade — rate set by destination mode (SLOW for Chill, FAST for all others) */
    float cf_step = (mixer->current_override == OVERRIDE_CHILL)
                    ? PLAYLIST_CROSSFADE_STEP_SLOW : PLAYLIST_CROSSFADE_STEP_FAST;
    if (mixer->current_override != OVERRIDE_NONE)
    {
        mixer->crossfade_gain += cf_step;
        if (mixer->crossfade_gain >= 1.0f)
            mixer->crossfade_gain = 1.0f;
    }
    else
    {
        mixer->crossfade_gain -= cf_step;
        if (mixer->crossfade_gain <= 0.0f)
            mixer->crossfade_gain = 0.0f;
    }

    /* Ducking state machine */
    float floor = DUCK_FLOOR_MIN + (1.0f - DUCK_FLOOR_MIN) * mixer->duck_level_ema;
    float step = mixer->duck_speed_ema * DUCK_MAX_STEP;

    switch (mixer->duck_state)
    {
    case DUCK_IDLE:
        if (mixer->soundbyte_playing)
            mixer->duck_state = DUCK_FADING_DOWN;
        break;

    case DUCK_FADING_DOWN:
        mixer->duck_gain -= step;
        if (mixer->duck_gain <= floor)
        {
            mixer->duck_gain = floor;
            mixer->duck_state = DUCK_HELD;
        }
        break;

    case DUCK_HELD:
        mixer->duck_gain = floor;
        if (!mixer->soundbyte_playing)
            mixer->duck_state = DUCK_FADING_UP;
        break;

    case DUCK_FADING_UP:
        if (mixer->soundbyte_playing)
        {
            mixer->duck_state = DUCK_FADING_DOWN;
        }
        else
        {
            mixer->duck_gain += step;
            if (mixer->duck_gain >= 1.0f)
            {
                mixer->duck_gain = 1.0f;
                mixer->duck_state = DUCK_IDLE;
            }
        }
        break;
    }

    /* When paused, pause_gain governs the background — duck cannot cut below it.
       source_factor fades the background to 0 as the crossfade completes. */
    float bg_gain      = mixer->set_pause_active ? mixer->pause_gain : mixer->duck_gain;
    float source_factor   = 1.0f - mixer->crossfade_gain;
    float playlist_factor = mixer->crossfade_gain;

    switch (mixer->active_source)
    {
    case HAL_AUDIO_SOURCE_LAPTOP:
        mixer->state.laptop_vol    = mixer->laptop_ema    * bg_gain * source_factor;
        mixer->state.bluetooth_vol = 0.0f;
        break;
    case HAL_AUDIO_SOURCE_BLUETOOTH:
        mixer->state.laptop_vol    = 0.0f;
        mixer->state.bluetooth_vol = mixer->bluetooth_ema * bg_gain * source_factor;
        break;
    default:
        mixer->state.laptop_vol    = 0.0f;
        mixer->state.bluetooth_vol = 0.0f;
        break;
    }

    mixer->state.playlist_vol   = mixer->playlist_level_ema * playlist_factor * mixer->duck_gain;
    mixer->state.soundboard_vol = mixer->soundbyte_playing  ? mixer->soundboard_ema : 0.0f;
}

soundboard_mixer_vol_state_t soundboard_mixer_get_vol_states(soundboard_mixer_t *mixer)
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

void soundboard_mixer_set_playlist_length(soundboard_mixer_t *mixer, int length)
{
    mixer->playlist_length = length;
}

void soundboard_mixer_next_track(soundboard_mixer_t *mixer)
{
    if (mixer->playlist_length == 0)
        return;
    mixer->playlist_position = (mixer->playlist_position + 1) % mixer->playlist_length;
    mixer->playlist_trigger  = mixer->playlist_position;
}

void soundboard_mixer_previous_track(soundboard_mixer_t *mixer)
{
    if (mixer->playlist_length == 0)
        return;
    mixer->playlist_position =
        (mixer->playlist_position - 1 + mixer->playlist_length) % mixer->playlist_length;
    mixer->playlist_trigger = mixer->playlist_position;
}

int soundboard_mixer_get_playlist_position(soundboard_mixer_t *mixer)
{
    return mixer->playlist_position;
}

int soundboard_mixer_get_playlist_trigger(soundboard_mixer_t *mixer)
{
    return mixer->playlist_trigger;
}

override_mode_t soundboard_mixer_get_override(soundboard_mixer_t *mixer)
{
    return mixer->current_override;
}

override_mode_t soundboard_mixer_get_previous_override(soundboard_mixer_t *mixer)
{
    return mixer->previous_override;
}
