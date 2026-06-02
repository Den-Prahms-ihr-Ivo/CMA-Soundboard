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
    // warm up so EMA has converged
    for (int i = 0; i < 50; i++)
        soundboard_mixer_tick(&mixer);
} // VR 203162

static void drive_to_soundbyte_playing(void)
{
    hal_mock_button_set(HAL_BUTTON_SB_1, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_SB_1, false);
}

static void reset_mixer(void)
{
    hal_mock_reset();
    mock_poti_input_reset();
    hal_mock_buttons_reset();
    soundboard_mixer_init(&mixer, &MOCK_POTI_INPUT);
}

// –– REQ-AUDIO-001 –––––––––––––––––––––––––––––––––––––––––––––––

void test_source_selection_laptop(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, 1000);
    mock_poti_input_set_raw(POTI_BLUETOOTH_VOL, 1000);

    warm_up_to_ema_convergence();

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);

    TEST_ASSERT_TRUE(g.laptop_vol > 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.bluetooth_vol);
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_LAPTOP, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_selection_bluetooth(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, 1000);
    mock_poti_input_set_raw(POTI_BLUETOOTH_VOL, 1000);

    warm_up_to_ema_convergence();

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);

    TEST_ASSERT_TRUE(g.bluetooth_vol > 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.laptop_vol);
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_BLUETOOTH, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_switch_change_applies_next_tick(void)
{
    // No warm-up needed — this test checks state transition only, not vol values.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);

    soundboard_mixer_tick(&mixer);

    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);

    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_BLUETOOTH, soundboard_mixer_get_active_audio_source(&mixer));

    soundboard_mixer_tick(&mixer);

    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_LAPTOP, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_switch_does_not_affect_playback_positive(void)
{
    // Set Source Laptop
    // Set soundbyte playing true
    drive_to_soundbyte_playing();
    TEST_ASSERT_TRUE(soundboard_mixer_is_soundbyte_playing(&mixer));

    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);

    soundboard_mixer_tick(&mixer);

    // check soundbyte playing true and source laptop
    TEST_ASSERT_TRUE(soundboard_mixer_is_soundbyte_playing(&mixer));
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_LAPTOP, soundboard_mixer_get_active_audio_source(&mixer));

    // set source bluetooth
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);

    soundboard_mixer_tick(&mixer);

    // check soundbyte playing true and source bluetooth
    TEST_ASSERT_TRUE(soundboard_mixer_is_soundbyte_playing(&mixer));
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_BLUETOOTH, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_switch_does_not_affect_playback_negative(void)
{
    // Set Source Laptop
    // Set soundbyte playing is false by default
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);

    soundboard_mixer_tick(&mixer);

    // check soundbyte playing true and source laptop
    TEST_ASSERT_FALSE(soundboard_mixer_is_soundbyte_playing(&mixer));
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_LAPTOP, soundboard_mixer_get_active_audio_source(&mixer));

    // set sourc bluetooth
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);

    soundboard_mixer_tick(&mixer);

    // check soundbyte not playing and source bluetooth
    TEST_ASSERT_FALSE(soundboard_mixer_is_soundbyte_playing(&mixer));
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_BLUETOOTH, soundboard_mixer_get_active_audio_source(&mixer));
}

// –– REQ-AUDIO-002 –––––––––––––––––––––––––––––––––––––––––––––––

void test_poti_vol_min(void)
{
    // ADC = 0 → vol converges to 0.0 after EMA warm-up.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, 0);

    warm_up_to_ema_convergence();

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.laptop_vol);
}

void test_poti_vol_max(void)
{
    // ADC = MAX → vol converges to 1.0 after EMA warm-up.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);

    warm_up_to_ema_convergence();

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, g.laptop_vol);
}

void test_poti_smoothing_rejects_spike(void)
{
    // Steady-state at ~0.5, then one spike to MAX.
    // EMA alpha=0.1: vol moves ~10% toward the spike, not to 1.0.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX / 2);
    warm_up_to_ema_convergence();

    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    soundboard_mixer_tick(&mixer);

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_TRUE(g.laptop_vol > 0.4f); // moved slightly upward
    TEST_ASSERT_TRUE(g.laptop_vol < 0.7f); // did not pop to 1.0
}

void test_poti_channels_independent(void)
{
    // Two channels set to different values; each tracks its own EMA independently.
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX / 2); // ~0.5
    mock_poti_input_set_raw(POTI_BLUETOOTH_VOL, HAL_ADC_MAX);  // ~1.0

    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    warm_up_to_ema_convergence();

    soundboard_mixer_vol_state_t g = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, g.laptop_vol);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.bluetooth_vol); // inactive source gated to 0

    // Switch source — no extra warm-up needed because EMA was already tracking.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);
    soundboard_mixer_tick(&mixer); // one tick to apply the new source

    g = soundboard_mixer_get_vol_states(&mixer);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, g.bluetooth_vol); // converged, not converging
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.laptop_vol);
}

// –– REQ-AUDIO-003 –––––––––––––––––––––––––––––––––––––––––––––––

void test_ducking_fades_on_trigger(void)
{
    // Background at full vol; max aggressiveness gives an observable drop in few ticks.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    float vol_before = soundboard_mixer_get_vol_states(&mixer).laptop_vol;

    drive_to_soundbyte_playing();
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float vol_after = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_after < vol_before);
}

void test_ducking_recovers_on_complete(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    drive_to_soundbyte_playing();
    for (int i = 0; i < 20; i++)
        soundboard_mixer_tick(&mixer);

    float vol_ducked = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_ducked < 0.5f); // confirm ducking is active before testing recovery

    hal_mock_set_soundbyte_playing(false);
    for (int i = 0; i < 5; i++)
        soundboard_mixer_tick(&mixer);

    float vol_recovering = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_recovering > vol_ducked);
}

void test_ducking_level_controls_depth(void)
{
    // Level = 0: fades near silence but must not reach 0 (floor).
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();
    drive_to_soundbyte_playing();
    for (int i = 0; i < 30; i++)
        soundboard_mixer_tick(&mixer);

    float vol_near_silence = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_near_silence > 0.0f); // not mute
    TEST_ASSERT_TRUE(vol_near_silence < 0.1f); // near silence

    // Level = MAX: ducking target = 1.0, so background is unaffected.
    reset_mixer();
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, HAL_ADC_MAX);
    warm_up_to_ema_convergence();
    drive_to_soundbyte_playing();
    for (int i = 0; i < 30; i++)
        soundboard_mixer_tick(&mixer);

    float vol_full = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, vol_full);
}

void test_ducking_aggressiveness_controls_rate(void)
{
    const int TICKS = 5;

    // Max aggressiveness: fast fade.
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();
    drive_to_soundbyte_playing();
    for (int i = 0; i < TICKS; i++)
        soundboard_mixer_tick(&mixer);
    float vol_fast = soundboard_mixer_get_vol_states(&mixer).laptop_vol;

    // Min aggressiveness: slow fade — same tick count, less drop.
    reset_mixer();
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, 0);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();
    drive_to_soundbyte_playing();
    for (int i = 0; i < TICKS; i++)
        soundboard_mixer_tick(&mixer);
    float vol_slow = soundboard_mixer_get_vol_states(&mixer).laptop_vol;

    TEST_ASSERT_TRUE(vol_fast < vol_slow);
}

void test_ducking_back_to_back_soundbytes(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_VOL, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_SPEED, HAL_ADC_MAX);
    mock_poti_input_set_raw(POTI_DUCKING_LEVEL, 0);
    warm_up_to_ema_convergence();

    // Soundbyte 1: let ducking settle.
    drive_to_soundbyte_playing();
    for (int i = 0; i < 20; i++)
        soundboard_mixer_tick(&mixer);
    float vol_ducked = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_TRUE(vol_ducked < 0.5f); // confirm ducking is active

    // Soundbyte 2 starts and soundbyte 1 completes in the same tick.
    // Ducking must stay engaged — no fade-up between the two.
    hal_mock_button_set(HAL_BUTTON_SB_2, true);
    hal_mock_set_soundbyte_playing(false);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_SB_2, false);

    float vol_between = soundboard_mixer_get_vol_states(&mixer).laptop_vol;
    TEST_ASSERT_FLOAT_WITHIN(0.05f, vol_ducked, vol_between);
}

// –– REQ-AUDIO-004 –––––––––––––––––––––––––––––––––––––––––––––––

// TODO: implement
// ...

int main(void)
{
    UNITY_BEGIN();

    // –– REQ-AUDIO-001 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_source_selection_laptop);
    RUN_TEST(test_source_selection_bluetooth);
    RUN_TEST(test_source_switch_change_applies_next_tick);
    RUN_TEST(test_source_switch_does_not_affect_playback_positive);
    RUN_TEST(test_source_switch_does_not_affect_playback_negative);

    // –– REQ-AUDIO-002 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_poti_vol_min);
    RUN_TEST(test_poti_vol_max);
    RUN_TEST(test_poti_smoothing_rejects_spike);
    RUN_TEST(test_poti_channels_independent);

    // –– REQ-AUDIO-003 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_ducking_fades_on_trigger);
    RUN_TEST(test_ducking_recovers_on_complete);
    RUN_TEST(test_ducking_level_controls_depth);
    RUN_TEST(test_ducking_aggressiveness_controls_rate);
    RUN_TEST(test_ducking_back_to_back_soundbytes);

    // –– REQ-AUDIO-004 ––––––––––––––––––––––––––––––––––––––––––
    return UNITY_END();
}
