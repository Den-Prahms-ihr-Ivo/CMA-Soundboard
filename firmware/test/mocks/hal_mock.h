#ifndef HAL_MOCK_H
#define HAL_MOCK_H

/**
 * hal_mock.h — Mock HAL setter/getter declarations.
 *
 * Include this file ONLY in host test files, never in production code.
 * Production code includes hal.h only.
 *
 * Pattern from Project 1 (Temptation Box). Setters inject hardware state
 * before calling the module under test. Getters observe what the module
 * told the HAL to do.
 *
 * Call hal_mock_reset() at the start of every test function.
 */

#include "hal.h"

/* Reset all mock state to defaults. Call at the start of every test. */
void hal_mock_reset(void);

#endif /* HAL_MOCK_H */
