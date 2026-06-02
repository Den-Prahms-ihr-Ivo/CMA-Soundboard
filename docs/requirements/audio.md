# Phase A — Audio pipeline core

Audio mixer, ducking, playback

Phase A covers the audio pipeline foundation: source selection, gain control, ducking logic, and soundbyte playback. These requirements must be green before Bluetooth or remote integration begins. All logic tests run on host via `hal_mock`. Audio HAL calls are verified on target separately.

---

## REQ-AUDIO-001

**Status:** GREEN

**Title:** Audio source selection reflects physical switch state

**Description:** The active audio source (laptop or Bluetooth) is determined by reading the physical switch via the HAL on every tick. The firmware applies gain only to the active source. The inactive source produces no output.

**Acceptance criteria:**

- `switch = LAPTOP` → laptop gain applied, BT gain zeroed
- `switch = BLUETOOTH` → BT gain applied, laptop gain zeroed
- switch changes mid-tick → takes effect on next tick
- source switch change does not affect soundboard playback

**Tests:**

- `test_audio_mixer.c::test_source_selection_laptop()`
- `test_audio_mixer.c::test_source_selection_bluetooth()`
- `test_audio_mixer.c::test_source_switch_change_applies_next_tick()`
- `test_audio_mixer.c::test_source_switch_does_not_affect_playback_positive()`
- `test_audio_mixer.c::test_source_switch_does_not_affect_playback_negative()`

---

## REQ-AUDIO-002

**Status:** GREEN

**Title:** Potentiometer values map to gain levels with smoothing

**Description:** Each potentiometer ADC reading is passed through an exponential moving average filter before being applied as a gain value. Raw ADC counts map linearly to gain in the range [0.0, 1.0]. The smoothing alpha is a named constant.

**Acceptance criteria:**

- ADC = 0 (full counter-clockwise) → gain = 0.0
- ADC = MAX → gain = 1.0
- gain changes smoothly — single-sample spike does not cause audible pop
- `AUDIO_SMOOTH_ALPHA` is a named constant, default 0.1
- each potentiometer has its own independent smoothing state

**Tests:**

- `test_audio_mixer.c::test_poti_gain_min()`
- `test_audio_mixer.c::test_poti_gain_max()`
- `test_audio_mixer.c::test_poti_smoothing_rejects_spike()`
- `test_audio_mixer.c::test_poti_channels_independent()`

---

### REQ-AUDIO-003

**Status:** GREEN

**Title:** Ducking gate fades background on soundbyte trigger

**Description:** When a soundbyte begins playing, the active background source fades from its current gain to the ducking level at the rate set by the ducking aggressiveness potentiometer. When the soundbyte ends, background fades back to full. Both fade-down and fade-up are smooth ramps, never hard cuts.

**Acceptance criteria:**

- soundbyte start → background begins fade-down at aggressiveness rate
- soundbyte end → background begins fade-up at aggressiveness rate
- ducking level poti = 0 → background fades to silence (not mute)
- ducking level poti = max → no ducking (background stays at full)
- aggressiveness poti = min → slowest fade
- aggressiveness poti = max → fastest fade
- two soundbytes back to back → ducking stays engaged, no double-fade

**Tests:**

- `test_audio_mixer.c::test_ducking_fades_on_trigger()`
- `test_audio_mixer.c::test_ducking_recovers_on_complete()`
- `test_audio_mixer.c::test_ducking_level_controls_depth()`
- `test_audio_mixer.c::test_ducking_aggressiveness_controls_rate()`
- `test_audio_mixer.c::test_ducking_back_to_back_soundbytes()`

---

### REQ-AUDIO-004

**Status:** TODO

**Title:** Soundbyte playback triggered by button press

**Description:** Each of the six soundboard buttons maps to a sound file on the USB stick. Pressing a button starts playback of the mapped file at soundboard volume. If a soundbyte is already playing when a button is pressed, the current soundbyte stops and the new one starts immediately.

**Acceptance criteria:**

- button press → mapped soundbyte begins playback
- soundbyte plays at `soundboard_volume` gain level
- second button press during playback → first stops, second starts
- soundbyte completes → `playback_active = false`, ducking recovers
- unmapped button → no playback, no error

**Tests:**

- `test_audio_mixer.c::test_soundbyte_triggers_on_button()`
- `test_audio_mixer.c::test_soundbyte_interrupts_current()`
- `test_audio_mixer.c::test_soundbyte_complete_clears_active()`
- `test_audio_mixer.c::test_unmapped_button_safe()`

---

### REQ-AUDIO-005

**Status:** TODO

**Title:** Set Pause reduces background to pause volume

**Description:** When the Set Pause button is pressed (locally or via remote), the background audio fades to the level set by the pause volume potentiometer. A second press of Set Pause restores background to full. Set Pause is a toggle, not a momentary.

**Acceptance criteria:**

- first Set Pause press → background fades to `pause_volume`
- second Set Pause press → background fades back to full
- Set Pause active during soundbyte → soundbyte plays at `soundboard_volume`, background stays at `pause_volume` (not `ducking_level`)
- `pause_volume` poti = max → Set Pause has no audible effect
- Set Pause state persists across source switch changes

**Tests:**

- `test_audio_mixer.c::test_set_pause_toggles_volume()`
- `test_audio_mixer.c::test_set_pause_during_soundbyte()`
- `test_audio_mixer.c::test_set_pause_persists_across_source_switch()`

---

## Phase B — Bluetooth management (upcoming)

Phase B requirements cover Bluetooth device discovery, known device persistence in NVS, connection management, automatic reconnect on boot, and device toggle. These are written after Phase A audio tests are green — Bluetooth introduces significant real-time complexity and should not be debugged simultaneously with the audio pipeline.

---

## Phase C — Remote and BMS (upcoming)

Phase C covers BLE remote communication, button mapping from remote to soundboard actions, and the remote's BMS logic (80% charge ceiling, three-level LED warnings). The BMS logic is pure and fully testable on host — it is a good candidate for the first Rust module.
