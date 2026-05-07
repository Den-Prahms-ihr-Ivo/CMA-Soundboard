# Soundboard — Development Journal

---

## Session 1 — Project start

**Status:** Scaffold complete. First tests GREEN.

### What was done

- Wrote ADRs SB-001 through SB-011. Key decisions:
  - ESP-ADF for audio (ADR-SB-001), no Arduino audio
  - USB stick for sound files (ADR-SB-002)
  - Software ducking ramps, no hard mute (ADR-SB-003)
  - Bluedroid for A2DP (ADR-SB-004), BLE for remote (ADR-SB-005)
  - BMS charges to 80% (ADR-SB-006)
  - Rust for all pure-logic modules; C for ESP-ADF, Bluedroid, USB OTG (ADR-SB-009)
  - Cargo workspace at repo root (ADR-SB-010)
  - no_std for all firmware-linked Rust crates (ADR-SB-011)

- Drafted GPIO pin assignment (docs/pin_assignment.md). Open questions noted
  about DAC MCLK requirement and laptop audio input path (analog vs USB audio class).

- Wrote Phase A requirements (docs/requirements/audio.md):
  REQ-AUDIO-001 through REQ-AUDIO-005, all TODO.

- Scaffolded repo structure:
  - Cargo workspace with 5 crates: audio_mixer, button_input, poti_smoothing,
    known_devices, bms
  - firmware/ ESP-IDF project with hal/, audio/, bt/, usb/, ble/ subdirectories
  - components/rust_bindings/ for linking Rust staticlibs into PlatformIO build
  - run_tests.sh for dual test runner (cargo + ctest)

- Implemented audio_mixer crate (crates/audio_mixer/src/lib.rs):
  - MixerInputs / MixerOutputs structs (repr(C), FFI-safe)
  - MixerState with EMA smoothing state, ducking state machine, pause state
  - DuckingState enum: Idle / FadingDown / Held / FadingUp (exhaustive match)
  - audio_mixer_tick() — pure function, no_std, no heap allocation
  - audio_mixer_state_init() — placement-init pattern (caller allocates storage)

- HAL scaffold:
  - hal.h with full struct-of-function-pointers interface
  - hal_mock.c with setters/getters for C host tests
  - hal_target.c with stub implementations (all TODO for bring-up)

- rust_ffi.h — C header matching Rust FFI surface (manually maintained
  until cbindgen is integrated into the build)

- First tests written and passing:
  - crates/audio_mixer/tests/source_selection.rs
  - 4 tests, all GREEN: source_selection_laptop, source_selection_bluetooth,
    source_switch_change_applies_next_tick,
    source_switch_change_does_not_affect_soundbyte
  - REQ-AUDIO-001: TODO → still TODO (needs all 4 tests, plus target verification)
    but the test suite is GREEN

### no_std lessons learned this session

- Panic handlers belong in binary crates, not libraries. A library crate that
  defines `#[panic_handler]` will conflict with std when test binaries link it.
- `staticlib` crate type on host requires `eh_personality`, which no_std doesn't
  provide. Solution: `rlib` for host (test) builds, `staticlib` gated behind a
  feature flag for firmware builds.
- `Box` is not available in no_std without an explicit global allocator.
  The mixer uses a placement-init pattern instead: the C caller allocates
  MixerState and Rust writes into it via `core::ptr::write`.

---

## Session 3 — Module variant confirmed, BLE coexistence deferred

**Status:** Docs updated. No code changes.

### Decisions recorded

- **Board:** ESP32-S3-DevKitC-1 confirmed. USB OTG-capable by default on this board.
  All assigned GPIOs verified as broken out on the DevKitC-1 pinout. Pin assignment
  is now fully resolved for this board variant.

- **BLE coexistence test deliberately deferred.** PCB layout is not on the immediate
  horizon. The test (simultaneous A2DP + BLE central, no audio glitching) must happen
  before PCB commitment but not before Phase A–C firmware is working on devkits.
  Natural trigger: when Phase B (Bluetooth, REQ-BT-xxx) reaches GREEN.

### Open questions remaining

None blocking current work. All pin assignment questions resolved. BLE coexistence
is tracked but explicitly deferred.

### Next session

Write remaining Phase A test files and get them to GREEN:
- poti_smoothing.rs (REQ-AUDIO-002)
- ducking.rs (REQ-AUDIO-003)
- soundbyte.rs (REQ-AUDIO-004)
- set_pause.rs (REQ-AUDIO-005)



**Status:** ADR-SB-012 written. Pin assignment updated. No code changes.

### Decision made

Laptop connects via 3.5mm stereo line-in jack. Audio ADC IC confirmed as PCM1808
(same TI family as PCM5102A DAC, hardware-configurable, no I2C required).

I2S topology: ESP32-S3 as master, shared BCLK/WS/MCLK bus. PCM1808 on DIN (GPIO 21),
PCM5102A on DOUT (GPIO 17). MCLK (GPIO 18) confirmed active and shared.

PCM1808 strapping: FMT=GND (I2S standard), MD0=GND, MD1=GND (single-speed).
No ESP32-S3 GPIOs needed for PCM1808 control — strap at board level.

USB OTG stays single-purpose: stick only. No hub required.

### Open questions closed

- MCLK required → GPIO 18 allocated ✓
- Laptop audio path → 3.5mm + PCM1808 → GPIO 21 active ✓

### Open questions remaining

- ESP32-S3 module variant (devkit vs custom PCB)
- BLE + A2DP coexistence validation (must happen before PCB layout)

### Next session

1. Write remaining Phase A test files:
   - poti_smoothing.rs (REQ-AUDIO-002)
   - ducking.rs (REQ-AUDIO-003)
   - soundbyte.rs (REQ-AUDIO-004)
   - set_pause.rs (REQ-AUDIO-005)
2. Get all Phase A tests to GREEN.
3. Update REQ-AUDIO-001 through 005 status fields.
4. Decide on laptop audio input path (affects pin assignment and ADF topology).
5. Begin audio_task.c stub — the C side that feeds the Rust mixer each tick.

### Open questions

- Does the laptop connect via analog line-in (needs ADC IC, e.g. PCM1808) or
  via USB Audio Class? The USB path would require a USB hub IC (one port for
  stick, one for laptop) or a second USB port. Significantly different topology.
- Does the chosen DAC (PCM5102A candidate) require MCLK? Determines if GPIO 18
  is needed.
- Confirm ESP32-S3 module variant — devkit vs custom PCB.
