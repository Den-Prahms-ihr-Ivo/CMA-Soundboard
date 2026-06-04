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

// –– Helper Functions ––––––––––––––––––––––––––––––––––––––––––––

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

static void press_button_once(hal_button_t btn)
{
    hal_mock_button_set(btn, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(btn, false);
    soundboard_mixer_tick(&mixer); /* let prev-state update so next press is a clean rising edge */
}

static void run_fast_crossfade(void)
{
    int ticks = (int)(1.0f / PLAYLIST_CROSSFADE_STEP_FAST) + 2;
    for (int i = 0; i < ticks; i++)
        soundboard_mixer_tick(&mixer);
}

static void run_slow_crossfade(void)
{
    int ticks = (int)(1.0f / PLAYLIST_CROSSFADE_STEP_SLOW) + 2;
    for (int i = 0; i < ticks; i++)
        soundboard_mixer_tick(&mixer);
}

// –– REQ-PLAY-001 — Crossfade between source and active override ––

void test_crossfade_rates_are_named_constants(void)
{
    // Both constants must be positive; FAST must be greater than SLOW
    // (larger step per tick = faster transition).
    TEST_ASSERT_TRUE(PLAYLIST_CROSSFADE_STEP_FAST > 0.0f);
    TEST_ASSERT_TRUE(PLAYLIST_CROSSFADE_STEP_SLOW > 0.0f);
    TEST_ASSERT_TRUE(PLAYLIST_CROSSFADE_STEP_FAST > PLAYLIST_CROSSFADE_STEP_SLOW);
}

void test_crossfade_source_to_music_uses_fast(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard

    run_fast_crossfade();

    soundboard_mixer_vol_state_t s = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.laptop_vol);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.playlist_vol);
}

void test_crossfade_source_to_chill_uses_slow(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer)); // guard

    // After FAST ticks the slow crossfade is not yet complete.
    run_fast_crossfade();

    float laptop = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(laptop > 0.01f); // not complete
    TEST_ASSERT_TRUE(laptop < 0.9f);  // but has moved meaningfully
}

void test_crossfade_music_to_source_uses_fast(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    run_fast_crossfade();

    // Exit music mode.
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer)); // guard
    run_fast_crossfade();

    float laptop = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, laptop);
}

void test_crossfade_chill_to_source_uses_fast(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer)); // guard
    run_slow_crossfade();

    // Exit chill — returns at FAST rate.
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer)); // guard
    run_fast_crossfade();

    float laptop = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, laptop);
}

void test_crossfade_music_to_chill_uses_slow(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    run_fast_crossfade();

    // Direct swap to chill — should use slow rate.
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer));

    // After FAST ticks, chill transition should not be complete.
    run_fast_crossfade();
    float playlist = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_TRUE(playlist > 0.0f); // still active, still transitioning
    TEST_ASSERT_TRUE(playlist < 1.0f); // not finished yet
}

void test_crossfade_chill_to_music_uses_fast(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer)); // guard
    run_slow_crossfade();

    // Swap to music — should complete in FAST ticks.
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer));
    run_fast_crossfade();

    float playlist = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, playlist);
}

void test_crossfade_does_not_interfere_with_soundbyte_ducking(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    run_fast_crossfade();

    float playlist_before = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, playlist_before); // guard: crossfade done

    drive_to_soundbyte_playing();
    for (int i = 0; i < 10; i++)
        soundboard_mixer_tick(&mixer);

    soundboard_mixer_vol_state_t s = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_TRUE(s.playlist_vol < playlist_before); // playlist ducked
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.laptop_vol);        // source stays at 0
}

void test_soundbyte_during_crossfade_ducks_playlist(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX); // fast ducking
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);           // duck to floor
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer));

    // Run half the slow crossfade so playlist is mid-rise.
    int half_ticks = (int)(0.5f / PLAYLIST_CROSSFADE_STEP_SLOW);
    for (int i = 0; i < half_ticks; i++)
        soundboard_mixer_tick(&mixer);

    float playlist_mid_crossfade = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_TRUE(playlist_mid_crossfade > 0.1f); // crossfade has moved
    TEST_ASSERT_TRUE(playlist_mid_crossfade < 0.9f); // but not finished

    // Trigger soundbyte and run enough ticks for ducking to dominate.
    // Few ticks at max ducking aggressiveness drops the playlist quickly.
    drive_to_soundbyte_playing();
    for (int i = 0; i < 10; i++)
        soundboard_mixer_tick(&mixer);

    float playlist_after_duck = soundboard_mixer_get_vol_states(&mixer).playlist_vol;
    TEST_ASSERT_TRUE(playlist_after_duck < playlist_mid_crossfade);
    // No assertion about laptop_vol — crossfade is still in progress, not under test.
}

void test_crossfade_completes_after_soundbyte_during_transition(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_PLAYLIST_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    press_button_once(HAL_BUTTON_CHILL);

    // Trigger soundbyte during crossfade
    int quarter_ticks = (int)(0.25f / PLAYLIST_CROSSFADE_STEP_SLOW);
    for (int i = 0; i < quarter_ticks; i++)
        soundboard_mixer_tick(&mixer);
    drive_to_soundbyte_playing();

    // Let soundbyte play, then end it
    for (int i = 0; i < 10; i++)
        soundboard_mixer_tick(&mixer);
    hal_mock_set_soundbyte_playing(false);

    // Run a generous number of ticks — both ducking recovery and crossfade
    // completion should finish
    for (int i = 0; i < 200; i++)
        soundboard_mixer_tick(&mixer);

    soundboard_mixer_vol_state_t s = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.playlist_vol); // crossfade reached destination
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.laptop_vol);   // source stays at 0
}

// –– REQ-PLAY-002 — Override button semantics ––––––––––––––––––––

void test_music_button_enters_music_mode(void)
{
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer));
}

void test_music_button_exits_music_mode(void)
{
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

void test_chill_button_enters_chill_mode(void)
{
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer));
}

void test_chill_button_exits_chill_mode(void)
{
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

void test_tabata_button_enters_tabata_mode(void)
{
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer));
}

void test_tabata_button_exits_tabata_mode(void)
{
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

void test_music_to_chill_via_chill_button(void)
{
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_CHILL);
    // Direct swap — no intermediate NONE.
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer));
}

void test_chill_to_music_via_music_button(void)
{
    press_button_once(HAL_BUTTON_CHILL);
    TEST_ASSERT_EQUAL(OVERRIDE_CHILL, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer));
}

void test_source_switch_toggle_clears_override(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    soundboard_mixer_tick(&mixer);

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard

    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);
    soundboard_mixer_tick(&mixer);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

// –– REQ-PLAY-005 — Tabata state capture and restoration ––––––––

void test_tabata_button_captures_previous_source(void)
{
    // No override → enter Tabata → previous should be NONE (source mode)
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_previous_override(&mixer));
}

void test_tabata_button_captures_previous_override(void)
{
    // In Music → enter Tabata → previous should be MUSIC
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer)); // guard
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer));
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_previous_override(&mixer));
}

void test_tabata_track_complete_restores_previous_source(void)
{
    // Tabata from source → track ends → back to NONE
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard

    hal_mock_set_playlist_track_complete(true);
    soundboard_mixer_tick(&mixer);

    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

void test_tabata_track_complete_restores_previous_override(void)
{
    // Tabata from Music → track ends → back to MUSIC
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard

    hal_mock_set_playlist_track_complete(true);
    soundboard_mixer_tick(&mixer);

    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer));
}

void test_tabata_button_during_playback_exits_immediately(void)
{
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard

    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_NONE, soundboard_mixer_get_override(&mixer));
}

void test_music_button_during_tabata_swaps_without_restore(void)
{
    // Tabata from Music → pressing Music swaps to Music directly (not restoring via track end)
    press_button_once(HAL_BUTTON_MUSIC_MODE);
    press_button_once(HAL_BUTTON_TABATA);
    TEST_ASSERT_EQUAL(OVERRIDE_TABATA, soundboard_mixer_get_override(&mixer)); // guard

    press_button_once(HAL_BUTTON_MUSIC_MODE);
    TEST_ASSERT_EQUAL(OVERRIDE_MUSIC, soundboard_mixer_get_override(&mixer));
}

int main(void)
{
    UNITY_BEGIN();

    // –– REQ-PLAY-001 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_crossfade_rates_are_named_constants);
    RUN_TEST(test_crossfade_source_to_music_uses_fast);
    RUN_TEST(test_crossfade_source_to_chill_uses_slow);
    RUN_TEST(test_crossfade_music_to_source_uses_fast);
    RUN_TEST(test_crossfade_chill_to_source_uses_fast);
    RUN_TEST(test_crossfade_music_to_chill_uses_slow);
    RUN_TEST(test_crossfade_chill_to_music_uses_fast);
    RUN_TEST(test_crossfade_does_not_interfere_with_soundbyte_ducking);
    RUN_TEST(test_soundbyte_during_crossfade_ducks_playlist);
    RUN_TEST(test_crossfade_completes_after_soundbyte_during_transition);

    // –– REQ-PLAY-002 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_music_button_enters_music_mode);
    RUN_TEST(test_music_button_exits_music_mode);
    RUN_TEST(test_chill_button_enters_chill_mode);
    RUN_TEST(test_chill_button_exits_chill_mode);
    RUN_TEST(test_tabata_button_enters_tabata_mode);
    RUN_TEST(test_tabata_button_exits_tabata_mode);
    RUN_TEST(test_music_to_chill_via_chill_button);
    RUN_TEST(test_chill_to_music_via_music_button);
    RUN_TEST(test_source_switch_toggle_clears_override);

    // –– REQ-PLAY-005 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_tabata_button_captures_previous_source);
    RUN_TEST(test_tabata_button_captures_previous_override);
    RUN_TEST(test_tabata_track_complete_restores_previous_source);
    RUN_TEST(test_tabata_track_complete_restores_previous_override);
    RUN_TEST(test_tabata_button_during_playback_exits_immediately);
    RUN_TEST(test_music_button_during_tabata_swaps_without_restore);

    return UNITY_END();
}
