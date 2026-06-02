cmake_minimum_required(VERSION 3.20)
project(soundboard_host_tests C)

# ---------------------------------------------------------------------------
# Unity test framework (git submodule at firmware/test/host/Unity)
# ---------------------------------------------------------------------------
add_subdirectory(Unity)

# ---------------------------------------------------------------------------
# Source paths
#
# Tests compile the module under test directly alongside the mock HAL.
# No ESP-IDF, no FreeRTOS, no ADF — host-only code only.
#
# REPO_ROOT is passed in by run_tests.sh:
#   cmake -DREPO_ROOT=<path> ..
# ---------------------------------------------------------------------------
if(NOT DEFINED REPO_ROOT)
    # Fallback for running cmake manually from this directory
    set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
endif()

set(FIRMWARE_MAIN "${REPO_ROOT}/firmware/main")
set(TEST_MAIN "${REPO_ROOT}/firmware/test")
set(HAL_SOURCES
    "${TEST_MAIN}/mocks/hal_mock.c"
    "${TEST_MAIN}/mocks/hal_mock_buttons_and_switches.c"
    "${TEST_MAIN}/mocks/mock_poti_inputs.c"
)

# ---------------------------------------------------------------------------
# Helper macro — one call per test module.
# Usage: add_module_test(test_name source1.c source2.c ...)
#
# Creates an executable that links Unity and the mock HAL, then registers
# it with CTest so `ctest` picks it up automatically.
# ---------------------------------------------------------------------------
macro(add_module_test TEST_NAME)
    add_executable(${TEST_NAME}
        ${TEST_NAME}.c
        ${HAL_SOURCES}
        ${ARGN}
    )
    target_include_directories(${TEST_NAME} PRIVATE
        "${FIRMWARE_MAIN}/hal"
        "${FIRMWARE_MAIN}/soundboard_mixer"
        "${FIRMWARE_MAIN}/interfaces"
        "${Unity_SOURCE_DIR}/src"
    )
    target_link_libraries(${TEST_NAME} PRIVATE unity)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

enable_testing()

# ---------------------------------------------------------------------------
# Test executables — add one line per module as C modules are written.
#
# Example (uncomment when audio_task.c exists and has a test file):
#   add_module_test(test_audio_task
#       "${FIRMWARE_MAIN}/audio_task/audio_task.c"
#   )
#
# ---------------------------------------------------------------------------
add_module_test(test_soundboard_mixer
    "${FIRMWARE_MAIN}/soundboard_mixer/soundboard_mixer.c"
)
target_include_directories(test_soundboard_mixer PRIVATE "${FIRMWARE_MAIN}/soundboard_mixer")

add_module_test(test_soundboard_mixer_play
    "${FIRMWARE_MAIN}/soundboard_mixer/soundboard_mixer.c"
)
target_include_directories(test_soundboard_mixer_play PRIVATE "${FIRMWARE_MAIN}/soundboard_mixer")

add_module_test(test_tabata
    "${FIRMWARE_MAIN}/tabata/tabata.c"
)
target_include_directories(test_tabata PRIVATE "${FIRMWARE_MAIN}/tabata")

# file_loader has no HAL dependency — uses fs_interface_t injection only.
add_executable(test_file_loader
    test_file_loader.c
    "${FIRMWARE_MAIN}/file_loader/file_loader.c"
    "${TEST_MAIN}/mocks/mock_fs.c"
)
target_include_directories(test_file_loader PRIVATE
    "${FIRMWARE_MAIN}/file_loader"
    "${TEST_MAIN}/mocks"
    "${Unity_SOURCE_DIR}/src"
)
target_link_libraries(test_file_loader PRIVATE unity)
add_test(NAME test_file_loader COMMAND test_file_loader)
