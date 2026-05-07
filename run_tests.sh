#!/usr/bin/env bash
# run_tests.sh — Run all host-side tests for the Soundboard project.
#
# Usage: ./run_tests.sh [--rust-only | --c-only]
#
# Rust tests: cargo test for all crates in the workspace
# C tests:    CMake + ctest for firmware/test/host/ (Unity)
#
# Both must be green before any firmware is built.
# This script exits non-zero if any test fails.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST_TARGET="x86_64-unknown-linux-gnu"  # Change for macOS: x86_64-apple-darwin or aarch64-apple-darwin
C_TEST_BUILD_DIR="${REPO_ROOT}/build/host_tests"


# ---------------------------------------------------------------------------
# C host tests (Unity via CMake)
# ---------------------------------------------------------------------------

echo ""
echo "---- C host tests (CMake + ctest) ----"
mkdir -p "${C_TEST_BUILD_DIR}"

if cmake \
    -S "${REPO_ROOT}/firmware/test/host" \
    -B "${C_TEST_BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DREPO_ROOT="${REPO_ROOT}" \
    2>&1 \
&& cmake --build "${C_TEST_BUILD_DIR}" 2>&1 \
&& ctest --test-dir "${C_TEST_BUILD_DIR}" --output-on-failure 2>&1; then
    echo "[PASS] C host tests"
else
    echo "[FAIL] C host tests"
    FAIL=1
fi


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "============================================================"
if [ "${FAIL}" -eq 0 ]; then
    echo " ALL TESTS PASSED"
else
    echo " TESTS FAILED — see output above"
fi
echo "============================================================"
exit "${FAIL}"
