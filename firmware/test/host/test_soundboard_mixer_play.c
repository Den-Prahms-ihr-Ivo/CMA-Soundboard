#include "unity.h"
#include "../soundboard_mixer/soundboard_mixer.h"
#include "../mocks/hal_mock.h"
#include "../mocks/hal_mock_buttons_and_switches.h"
#include "../mocks/mock_poti_inputs.h"

static soundboard_mixer_t mixer;

void setUp(void)
{
    hal_mock_reset();
    mock_poti_input_reset();
    hal_mock_buttons_reset();
    soundboard_mixer_init(&mixer, &MOCK_POTI_INPUT);
}

void tearDown(void) {}

// –– Helper Functions –––––––––––––––––––––––––––––––––––––––––––––––

static void warm_up_to_ema_convergence(void)
{
    for (int i = 0; i < 50; i++)
        soundboard_mixer_tick(&mixer);
}

static void drive_to_soundbyte_playing(void)
{
    hal_mock_button_set(HAL_BUTTON_SB_1, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_SB_1, false);
}

static void drive_to_override_active(void)
{
    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, false);
}

static void drive_to_crossfade_complete(void)
{
    drive_to_override_active();
    int ticks = (int)(1.0f / PLAYLIST_CROSSFADE_STEP) + 5;
    for (int i = 0; i < ticks; i++)
        soundboard_mixer_tick(&mixer);
}

// –– REQ-PLAY-001 –––––––––––––––––––––––––––––––––––––––––––––––

void test_crossfade_source_down_on_override_activation(void)
{
    // Override activation → source channel begins fading down.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    float vol_before = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    drive_to_override_active();
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float vol_after = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_after < vol_before);
}

void test_crossfade_playlist_up_on_override_activation(void)
{
    // Override activation → playlist channel begins fading up.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    drive_to_override_active();
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float playlist = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_TRUE(playlist > 0.0f);
}

void test_crossfade_source_up_on_override_clear(void)
{
    // After crossfade completes, clearing override fades source back up.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    drive_to_crossfade_complete();

    float vol_at_zero = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, vol_at_zero); // guard: source fully faded

    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, false);
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float vol_recovering = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_recovering > vol_at_zero);
}

void test_crossfade_playlist_down_on_override_clear(void)
{
    // After crossfade completes, clearing override fades playlist back down.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    drive_to_crossfade_complete();

    float playlist_full = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, playlist_full); // guard: playlist at full

    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_MUSIC_MODE, false);
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float playlist_fading = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_TRUE(playlist_fading < playlist_full);
}

void test_crossfade_rate_is_named_constant(void)
{
    // After exactly ceil(1.0 / PLAYLIST_CROSSFADE_STEP) ticks the crossfade must
    // be complete — verifying that PLAYLIST_CROSSFADE_STEP controls the rate.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    drive_to_override_active();
    int ticks = (int)(1.0f / PLAYLIST_CROSSFADE_STEP) + 2;
    for (int i = 0; i < ticks; i++)
        soundboard_mixer_tick(&mixer);

    soundboard_mixer_vol_state_t s = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.laptop_vol);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.playlist_vol);
}

void test_crossfade_does_not_interfere_with_soundbyte_ducking(void)
{
    // During override, ducking applies to the playlist channel, not the source.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    drive_to_crossfade_complete();

    float playlist_before_duck = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, playlist_before_duck); // guard: crossfade done

    drive_to_soundbyte_playing();
    for (int i = 0; i < 10; i++)
        soundboard_mixer_tick(&mixer);

    soundboard_mixer_vol_state_t s = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_TRUE(s.playlist_vol < playlist_before_duck); // playlist ducked
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.laptop_vol);             // source stays at 0
}

void test_crossfade_vol_poti_controls_playlist_target(void)
{
    // playlist_vol output converges to POTI_PLAYLIST_VOL level after crossfade.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX / 2); // 50%
    warm_up_to_ema_convergence();

    drive_to_crossfade_complete();

    float playlist = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, playlist);
}

// –– REQ-PLAY-002 –––––––––––––––––––––––––––––––––––––––––––––––

void test_playlist_next_increments_position(void)
{
    soundboard_mixer_set_playlist_length(&mixer, 5);
    soundboard_mixer_next_track(&mixer);

    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_previous_decrements_position(void)
{
    soundboard_mixer_set_playlist_length(&mixer, 5);
    soundboard_mixer_next_track(&mixer); // → 1
    soundboard_mixer_next_track(&mixer); // → 2
    soundboard_mixer_previous_track(&mixer); // → 1

    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_next_wraps_at_end(void)
{
    soundboard_mixer_set_playlist_length(&mixer, 3);
    soundboard_mixer_next_track(&mixer); // → 1
    soundboard_mixer_next_track(&mixer); // → 2

    TEST_ASSERT_EQUAL_INT(2, soundboard_mixer_get_playlist_position(&mixer)); // guard

    soundboard_mixer_next_track(&mixer); // → wraps to 0

    TEST_ASSERT_EQUAL_INT(0, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(0, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_previous_wraps_at_start(void)
{
    soundboard_mixer_set_playlist_length(&mixer, 3);
    soundboard_mixer_previous_track(&mixer); // 0 → wraps to 2

    TEST_ASSERT_EQUAL_INT(2, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(2, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_track_complete_triggers_next(void)
{
    soundboard_mixer_set_playlist_length(&mixer, 5);
    hal_mock_set_playlist_track_complete(true);
    soundboard_mixer_tick(&mixer);

    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_length_zero_safe(void)
{
    // Zero-length playlist: next/previous must not crash and must not move.
    soundboard_mixer_set_playlist_length(&mixer, 0);
    soundboard_mixer_next_track(&mixer);
    soundboard_mixer_previous_track(&mixer);

    TEST_ASSERT_EQUAL_INT(0, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(-1, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_length_one_safe(void)
{
    // Single-track playlist: position stays 0 but trigger is still emitted.
    soundboard_mixer_set_playlist_length(&mixer, 1);
    soundboard_mixer_next_track(&mixer);

    TEST_ASSERT_EQUAL_INT(0, soundboard_mixer_get_playlist_position(&mixer));
    TEST_ASSERT_EQUAL_INT(0, soundboard_mixer_get_playlist_trigger(&mixer));
}

void test_playlist_trigger_cleared_on_tick(void)
{
    // Trigger is a one-shot: set by navigation, cleared at the start of the next tick.
    soundboard_mixer_set_playlist_length(&mixer, 5);
    soundboard_mixer_next_track(&mixer);

    TEST_ASSERT_EQUAL_INT(1, soundboard_mixer_get_playlist_trigger(&mixer)); // set

    soundboard_mixer_tick(&mixer);

    TEST_ASSERT_EQUAL_INT(-1, soundboard_mixer_get_playlist_trigger(&mixer)); // cleared
}

int main(void)
{
    UNITY_BEGIN();

    // –– REQ-PLAY-001 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_crossfade_source_down_on_override_activation);
    RUN_TEST(test_crossfade_playlist_up_on_override_activation);
    RUN_TEST(test_crossfade_source_up_on_override_clear);
    RUN_TEST(test_crossfade_playlist_down_on_override_clear);
    RUN_TEST(test_crossfade_rate_is_named_constant);
    RUN_TEST(test_crossfade_does_not_interfere_with_soundbyte_ducking);
    RUN_TEST(test_crossfade_vol_poti_controls_playlist_target);

    // –– REQ-PLAY-002 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_playlist_next_increments_position);
    RUN_TEST(test_playlist_previous_decrements_position);
    RUN_TEST(test_playlist_next_wraps_at_end);
    RUN_TEST(test_playlist_previous_wraps_at_start);
    RUN_TEST(test_playlist_track_complete_triggers_next);
    RUN_TEST(test_playlist_length_zero_safe);
    RUN_TEST(test_playlist_length_one_safe);
    RUN_TEST(test_playlist_trigger_cleared_on_tick);

    return UNITY_END();
}
