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
}

void soundboard_mixer_tick(soundboard_mixer_t *mixer)
{
    mixer->active_source = hal_source_switch_read();

    mixer->laptop_ema = ema_update(mixer->laptop_ema,
                                   mixer->potis->read_normalized(POTI_LAPTOP_VOL));
    mixer->bluetooth_ema = ema_update(mixer->bluetooth_ema,
                                      mixer->potis->read_normalized(POTI_BLUETOOTH_VOL));
    mixer->duck_speed_ema = ema_update(mixer->duck_speed_ema,
                                       mixer->potis->read_normalized(POTI_DUCKING_SPEED));
    mixer->duck_level_ema = ema_update(mixer->duck_level_ema,
                                       mixer->potis->read_normalized(POTI_DUCKING_LEVEL));

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

    switch (mixer->active_source)
    {
    case HAL_AUDIO_SOURCE_LAPTOP:
        mixer->state.laptop_vol = mixer->laptop_ema * mixer->duck_gain;
        mixer->state.bluetooth_vol = 0.0f;
        break;
    case HAL_AUDIO_SOURCE_BLUETOOTH:
        mixer->state.laptop_vol = 0.0f;
        mixer->state.bluetooth_vol = mixer->bluetooth_ema * mixer->duck_gain;
        break;
    default:
        mixer->state.laptop_vol = 0.0f;
        mixer->state.bluetooth_vol = 0.0f;
        break;
    }
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
