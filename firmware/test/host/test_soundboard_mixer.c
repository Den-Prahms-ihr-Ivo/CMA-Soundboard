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

static void warm_up_to_ema_convergence(void)
{
    // warm up so EMA has converged
    for (int i = 0; i < 50; i++)
        soundboard_mixer_tick(&mixer);
}

static void drive_to_soundbyte_playing(void)
{
    hal_mock_button_set(HAL_BUTTON_SB_1, true);
    soundboard_mixer_tick(&mixer);
    hal_mock_button_set(HAL_BUTTON_SB_1, false);
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

    return UNITY_END();
}
