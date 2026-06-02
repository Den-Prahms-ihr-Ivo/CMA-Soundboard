#include "unity.h"
#include "tabata.h"
#include "../mocks/hal_mock.h"
#include "../mocks/hal_mock_buttons_and_switches.h"

static tabata_t tab;

void setUp(void)
{
    hal_mock_reset();
    hal_mock_buttons_reset();
    tabata_init(&tab);
}

void tearDown(void) {}

// –– Helper Functions ––––––––––––––––––––––––––––––––––––––––––––

static void drive_to_active(void)
{
    hal_mock_button_set(HAL_BUTTON_TABATA, true);
    tabata_tick(&tab);
    hal_mock_button_set(HAL_BUTTON_TABATA, false);
}

static void run_one_round(void)
{
    for (int i = 0; i < TABATA_WORK_DURATION_S + TABATA_REST_DURATION_S; i++)
        tabata_tick(&tab);
}

static void run_full_session(void)
{
    for (int i = 0; i < TABATA_ROUNDS; i++)
        run_one_round();
}

// –– REQ-PLAY-005 ––––––––––––––––––––––––––––––––––––––––––––––––

void test_tabata_rounds_is_named_constant(void)
{
    // All three constants must be defined with the specified values.
    TEST_ASSERT_EQUAL_INT(8,  TABATA_ROUNDS);
    TEST_ASSERT_EQUAL_INT(20, TABATA_WORK_DURATION_S);
    TEST_ASSERT_EQUAL_INT(10, TABATA_REST_DURATION_S);
}

void test_tabata_work_duration_is_named_constant(void)
{
    // Work phase lasts exactly TABATA_WORK_DURATION_S ticks.
    TEST_ASSERT_EQUAL_INT(20, TABATA_WORK_DURATION_S);

    drive_to_active();
    TEST_ASSERT_EQUAL(TABATA_PHASE_WORK, tabata_get_phase(&tab)); // guard

    for (int i = 0; i < TABATA_WORK_DURATION_S - 1; i++)
        tabata_tick(&tab);
    TEST_ASSERT_EQUAL(TABATA_PHASE_WORK, tabata_get_phase(&tab)); // still in work

    tabata_tick(&tab);
    TEST_ASSERT_EQUAL(TABATA_PHASE_REST, tabata_get_phase(&tab)); // transitioned to rest
}

void test_tabata_rest_duration_is_named_constant(void)
{
    // Rest phase lasts exactly TABATA_REST_DURATION_S ticks.
    TEST_ASSERT_EQUAL_INT(10, TABATA_REST_DURATION_S);

    drive_to_active();
    for (int i = 0; i < TABATA_WORK_DURATION_S; i++)
        tabata_tick(&tab);
    TEST_ASSERT_EQUAL(TABATA_PHASE_REST, tabata_get_phase(&tab)); // guard

    for (int i = 0; i < TABATA_REST_DURATION_S - 1; i++)
        tabata_tick(&tab);
    TEST_ASSERT_EQUAL(TABATA_PHASE_REST, tabata_get_phase(&tab)); // still in rest

    tabata_tick(&tab);
    TEST_ASSERT_EQUAL(TABATA_PHASE_WORK, tabata_get_phase(&tab)); // round 2 begins
}

void test_tabata_runs_correct_round_count(void)
{
    // Exactly TABATA_ROUNDS rounds complete, then session ends.
    drive_to_active();
    TEST_ASSERT_TRUE(tabata_is_active(&tab)); // guard

    run_full_session();

    TEST_ASSERT_FALSE(tabata_is_active(&tab));
    TEST_ASSERT_EQUAL_INT(TABATA_ROUNDS, tabata_get_round(&tab));
}

void test_tabata_completes_clears_override(void)
{
    // Session is still active through round 7, then ends after round 8.
    drive_to_active();
    TEST_ASSERT_TRUE(tabata_is_active(&tab)); // guard

    for (int i = 0; i < TABATA_ROUNDS - 1; i++)
        run_one_round();
    TEST_ASSERT_TRUE(tabata_is_active(&tab)); // still active after 7 rounds

    run_one_round();
    TEST_ASSERT_FALSE(tabata_is_active(&tab));
    TEST_ASSERT_EQUAL(TABATA_PHASE_IDLE, tabata_get_phase(&tab));
}

void test_tabata_button_aborts_session(void)
{
    // Second press of the Tabata button mid-session → immediate abort.
    drive_to_active();
    TEST_ASSERT_TRUE(tabata_is_active(&tab)); // guard

    for (int i = 0; i < 5; i++)
        tabata_tick(&tab);

    hal_mock_button_set(HAL_BUTTON_TABATA, true);
    tabata_tick(&tab);
    hal_mock_button_set(HAL_BUTTON_TABATA, false);

    TEST_ASSERT_FALSE(tabata_is_active(&tab));
    TEST_ASSERT_EQUAL(TABATA_PHASE_IDLE, tabata_get_phase(&tab));
}

void test_tabata_soundbyte_does_not_abort_session(void)
{
    // Pressing a soundboard button must not abort the Tabata session.
    drive_to_active();
    TEST_ASSERT_TRUE(tabata_is_active(&tab)); // guard

    hal_mock_button_set(HAL_BUTTON_SB_1, true);
    for (int i = 0; i < 5; i++)
        tabata_tick(&tab);
    hal_mock_button_set(HAL_BUTTON_SB_1, false);

    TEST_ASSERT_TRUE(tabata_is_active(&tab));
    TEST_ASSERT_EQUAL(TABATA_PHASE_WORK, tabata_get_phase(&tab));
}

void test_tabata_cue_emitted_on_phase_transition(void)
{
    // Work-start cue emitted on session start; rest cue emitted at work→rest.
    drive_to_active();

    TEST_ASSERT_EQUAL(TABATA_CUE_WORK, tabata_consume_cue(&tab)); // start cue
    TEST_ASSERT_EQUAL(TABATA_CUE_NONE, tabata_consume_cue(&tab)); // consumed

    for (int i = 0; i < TABATA_WORK_DURATION_S; i++)
        tabata_tick(&tab);

    TEST_ASSERT_EQUAL(TABATA_CUE_REST, tabata_consume_cue(&tab)); // rest cue
}

int main(void)
{
    UNITY_BEGIN();

    // –– REQ-PLAY-005 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_tabata_rounds_is_named_constant);
    RUN_TEST(test_tabata_work_duration_is_named_constant);
    RUN_TEST(test_tabata_rest_duration_is_named_constant);
    RUN_TEST(test_tabata_runs_correct_round_count);
    RUN_TEST(test_tabata_completes_clears_override);
    RUN_TEST(test_tabata_button_aborts_session);
    RUN_TEST(test_tabata_soundbyte_does_not_abort_session);
    RUN_TEST(test_tabata_cue_emitted_on_phase_transition);

    return UNITY_END();
}
