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

static void drive_to_soundbyte_playing(void)
{
    hal_mock_button_set(HAL_BUTTON_SB_1, true);
    soundboard_mixer_tick(&mixer);
    TEST_ASSERT_TRUE_MESSAGE(soundboard_mixer_is_soundbyte_playing(&mixer), "Faild to play soundbyte.");
    hal_mock_button_set(HAL_BUTTON_SB_1, false);
}

static void warm_up_to_ema_convergence(void)
{
    // warm up so EMA has converged
    for (int i = 0; i < 50; i++)
        soundboard_mixer_tick(&mixer);
}

// –– REQ-AUDIO-001 –––––––––––––––––––––––––––––––––––––––––––––––

void test_source_selection_laptop(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_LAPTOP);
    mock_poti_input_set_raw(POTI_LAPTOP_GAIN, 1000);
    mock_poti_input_set_raw(POTI_BLUETOOTH_GAIN, 1000);

    warm_up_to_ema_convergence();

    soundboard_mixer_gain_state_t g = soundboard_mixer_get_gain_states(&mixer);

    TEST_ASSERT_TRUE(g.laptop_gain > 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.bluetooth_gain);
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_LAPTOP, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_selection_bluetooth(void)
{
    hal_mock_set_source_switch(HAL_AUDIO_SOURCE_BLUETOOTH);
    mock_poti_input_set_raw(POTI_LAPTOP_GAIN, 1000);
    mock_poti_input_set_raw(POTI_BLUETOOTH_GAIN, 1000);

    warm_up_to_ema_convergence();

    soundboard_mixer_gain_state_t g = soundboard_mixer_get_gain_states(&mixer);

    TEST_ASSERT_TRUE(g.bluetooth_gain > 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g.laptop_gain);
    TEST_ASSERT_EQUAL(HAL_AUDIO_SOURCE_BLUETOOTH, soundboard_mixer_get_active_audio_source(&mixer));
}

void test_source_switch_change_applies_next_tick(void)
{
    // No warm-up needed — this test checks state transition only, not gain values.
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

    return UNITY_END();
}
