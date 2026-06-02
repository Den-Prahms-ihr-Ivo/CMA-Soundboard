# Requirements — Playback Modes (Phase A.5 → Phase B)

Module: `soundboard_mixer` (host-testable extensions) and `playback_manager` (Phase B, target-only)
Language: C
Test runner: `./run_tests.sh` (Unity tests on host)
Phase gate: Phase A GREEN before any work in this file begins

This file bridges Phase A and Phase B. The first two requirements (REQ-PLAY-001 and 002)
extend the host-testable mixer with crossfade and playlist navigation logic. REQ-PLAY-003
covers folder enumeration. REQ-PLAY-004 and 005 are the higher-level music mode and Tabata
mode behaviours that depend on USB mass storage and the ADF pipeline — these are target-only
and verified on hardware, not by Unity.

The split exists because crossfading and playlist navigation are pure mixer logic that
benefits from being designed and tested in isolation before being tangled up with file I/O
and ADF integration. Same reasoning that put the original mixer in Phase A.

---

## REQ-PLAY-001 — Crossfade between source and playlist on override activation

**Status:** GREEN
**Host-testable:** Yes — extends `soundboard_mixer.c` with new state and outputs

**Description**
When a source override (MusicMode or Tabata) activates, the active background source
(Laptop or Bluetooth) fades down to 0 while the playlist channel fades up to `playlist_vol`.
When the override deactivates, the playlist channel fades down to 0 while the source
fades back up. The crossfade is smooth — no hard cuts at any point.

Both directions of the crossfade use the same rate, controlled by a named constant
`PLAYLIST_CROSSFADE_STEP` (initial value to be tuned during bring-up). The rate is not
tied to the ducking aggressiveness potentiometer — playlist crossfade is a separate
concern from soundbyte ducking.

The crossfade state is independent of the ducking state machine. A soundbyte triggering
during playlist mode causes ducking of the playlist channel (REQ-AUDIO-003 applies),
not the source channel — the source is already at 0 because override is active.

**Acceptance criteria**

- Override activated (MusicMode or Tabata) → source channel begins fade-down, playlist channel begins fade-up
- After crossfade completes → source channel vol = 0, playlist channel vol = playlist_vol
- Override cleared → playlist channel begins fade-down, source channel begins fade-up
- After crossfade completes → playlist channel vol = 0, source channel vol = source_vol
- Crossfade is symmetric — same rate up and down
- `PLAYLIST_CROSSFADE_STEP` is a named constant
- Crossfade does not interfere with soundbyte ducking on the active channel

**Tests**

```
test_soundboard_mixer.c
  ::test_crossfade_source_down_on_override_activation
  ::test_crossfade_playlist_up_on_override_activation
  ::test_crossfade_source_up_on_override_clear
  ::test_crossfade_playlist_down_on_override_clear
  ::test_crossfade_rate_is_named_constant
  ::test_crossfade_does_not_interfere_with_soundbyte_ducking
```

---

## REQ-PLAY-002 — Playlist track navigation through mixer state

**Status:** GREEN
**Host-testable:** Yes — extends `soundboard_mixer.c` with playlist position state

**Description**
The mixer maintains a `playlist_position` (current track index) and a `playlist_length`
(total tracks in the active playlist). Length is set externally by the C file loader
when a playlist is loaded. Position is advanced by `soundboard_mixer_next_track()` and
decremented by `soundboard_mixer_previous_track()`.

When a track change is requested or a track ends, the mixer emits a
`TriggerPlaylistTrack(new_position)` output that the C file loader consumes to start
playback of the new file. End-of-track is signalled to the mixer via
`hal_mock_set_playlist_track_complete(true)` (analogous to soundbyte completion).

End-of-playlist behaviour wraps: position N-1 → position 0. This matches the music mode
"reshuffle and repeat" intent from the original design — the wrap point is where the
file loader could reshuffle.

The mixer does not know about file paths or playlist contents — only the position
within a list whose length was set externally.

**Acceptance criteria**

- `soundboard_mixer_next_track()` → playlist_position increments, TriggerPlaylistTrack(N+1) emitted
- `soundboard_mixer_previous_track()` → playlist_position decrements
- Position N-1 + next → wraps to position 0
- Position 0 + previous → wraps to N-1
- Track complete signal → next track triggered automatically (sequential playback)
- `playlist_length` of 0 → next/previous have no effect, no crash
- `playlist_length` of 1 → next/previous always stay at position 0

**Tests**

```
test_soundboard_mixer.c
  ::test_playlist_next_increments_position
  ::test_playlist_previous_decrements_position
  ::test_playlist_next_wraps_at_end
  ::test_playlist_previous_wraps_at_start
  ::test_playlist_track_complete_triggers_next
  ::test_playlist_length_zero_safe
  ::test_playlist_length_one_safe
```

---

## REQ-PLAY-003 — File loader enumerates folder and selects file

**Status:** GREEN
**Host-testable:** Partial — folder-mapping logic can be tested with a mock filesystem,
but actual FAT/USB reads require target hardware

**Description**
The C file loader module is responsible for mapping a button index or mode to a folder
path, enumerating the files in that folder, and selecting one for playback. Behaviour
depends on context:

- Soundboard buttons → SB1/ through SB6/ folders, **random** file selection
- Music mode → MUSIC/ folder, **shuffled sequential** playback
- Tabata cues → TABATA/work._, TABATA/rest._, TABATA/complete.\* — **fixed file names**

Random seed is derived from `esp_random()` on boot and not reset during operation.
Only WAV and MP3 files are considered — other file types in the folder are ignored.

The file loader exposes `playlist_length` to the mixer when a playlist folder is loaded.
This is how REQ-PLAY-002's externally-set length arrives.

**Acceptance criteria**

- Button index 0 → enumerates SB1/ folder, selects random WAV or MP3
- Button index 5 → enumerates SB6/ folder, selects random WAV or MP3
- Empty folder → returns no file, no crash
- Folder missing → returns no file, no crash
- Non-audio files in folder → ignored, not selected
- Random selection uses esp_random() seed, not a fixed seed
- Music mode load → playlist_length reported to mixer matches file count in MUSIC/
- Named constants: SB_FOLDER_PREFIX ("SB"), MUSIC_FOLDER ("MUSIC"), TABATA_FOLDER ("TABATA")

**Tests**

```
test_file_loader.c
  ::test_loader_maps_button_index_to_folder
  ::test_loader_returns_null_for_empty_folder
  ::test_loader_returns_null_for_missing_folder
  ::test_loader_ignores_non_audio_files
  ::test_loader_reports_playlist_length
  ::test_loader_folder_constants_are_named

Target-only verification:
  ::test_loader_reads_real_fat_filesystem  (hardware bring-up)
```

Random selection uniformity is not tested — only that a valid file is returned when
the folder contains files. True randomness testing is out of scope.

---

## REQ-PLAY-004 — Music mode integrates mixer, file loader, and ADF pipeline

**Status:** TODO
**Host-testable:** No — depends on USB mass storage, FAT filesystem, and ADF pipeline

**Description**
Music mode is activated by the music mode button. When active:

1. Override is set to MusicMode (REQ-AUDIO-001 applies — Laptop and Bluetooth gains zeroed)
2. Source channel crossfades down, playlist channel crossfades up (REQ-PLAY-001)
3. MUSIC/ folder is enumerated, shuffled, and `playlist_length` set on the mixer (REQ-PLAY-003)
4. First track plays; on end-of-file, next track in shuffle plays (REQ-PLAY-002)
5. After all tracks play, the shuffle reshuffles and continues

Soundboard buttons remain functional during music mode — soundbytes play over the music
with ducking applied to the playlist channel (REQ-AUDIO-003).

Music mode deactivates on:

- Second press of music mode button
- Physical source switch toggle

On deactivation: current track stops, playlist channel crossfades down, source channel
crossfades up, override clears.

**Acceptance criteria** (verified on hardware)

- Music mode button press → audible crossfade from source to MUSIC/ files
- Tracks play continuously without audible gap at transitions
- After all tracks → playlist reshuffles and continues without user action
- Soundboard button during music mode → soundbyte audible over ducked music
- Music mode button press again → audible crossfade back to source
- Source switch toggle → audible crossfade back to new source
- MUSIC/ folder empty or missing → music mode button has no effect, no crash

**Tests**

```
Target-only — no host test. Verified by ear and visual inspection
of LED ring (red for music mode) during hardware bring-up.
```

---

## REQ-PLAY-005 — Tabata mode runs interval protocol with audio cues

**Status:** TODO
**Host-testable:** Partial — interval timing and round counting can be host-tested;
audio cue playback requires hardware

**Description**
Tabata mode is activated by the Tabata mode button. When active, the device runs the
standard Tabata protocol: 8 rounds of 20 seconds work followed by 10 seconds rest.
Audio cue files from the TABATA/ folder play at each interval transition:

- TABATA/work.\* — plays at start of each work interval
- TABATA/rest.\* — plays at start of each rest interval
- TABATA/complete.\* — plays when all 8 rounds are done

Interval timing begins after the cue file finishes playing. If a cue file is missing,
the interval starts immediately without a cue — no crash.

The override is set to Tabata while active (Laptop and Bluetooth gains zeroed via
REQ-AUDIO-001 and REQ-PLAY-001 crossfade).

On completion of all 8 rounds, the override clears automatically and the source reverts
to the physical switch state. The Tabata button also exits the mode immediately if
pressed during a session — no graceful "finish current round" behaviour.

Constants:

- `TABATA_WORK_DURATION_S = 20`
- `TABATA_REST_DURATION_S = 10`
- `TABATA_ROUNDS = 8`

**Acceptance criteria**

- Tabata button press → override = Tabata, work cue triggered, 20s work interval begins
- After 20s → rest cue triggered, 10s rest interval begins
- After 10s → next work cue (repeat 8 rounds total)
- After round 8 rest → complete cue, override clears, source restored via crossfade
- Missing cue file → interval timer proceeds without cue, no crash
- Tabata button press during session → session aborted, override clears immediately
- Soundboard button during Tabata → soundbyte plays, interval timer continues
- All three duration/round constants are named

**Tests**

```
test_tabata.c (host-testable parts only)
  ::test_tabata_runs_correct_round_count
  ::test_tabata_work_duration_is_named_constant
  ::test_tabata_rest_duration_is_named_constant
  ::test_tabata_rounds_is_named_constant
  ::test_tabata_completes_clears_override
  ::test_tabata_button_aborts_session
  ::test_tabata_soundbyte_does_not_abort_session

Target-only verification:
  ::test_tabata_audio_cues_at_transitions  (hardware bring-up)
  ::test_tabata_missing_cue_does_not_crash (hardware bring-up)
```

---

## Phase A.5 completion gate (REQ-PLAY-001, REQ-PLAY-002)

These two requirements close the mixer extensions and can complete entirely on host.

- REQ-PLAY-001 and REQ-PLAY-002 Unity tests GREEN on host
- All Phase A tests still GREEN — no regressions
- `run_tests.sh` exits 0
- Git clean, journal updated

## Phase B playback completion gate (REQ-PLAY-003, 004, 005)

These require hardware and complete during Phase B alongside Bluetooth.

- REQ-PLAY-003 host-testable tests GREEN
- Manual target test: music mode plays continuously from USB stick
- Manual target test: Tabata completes 8 rounds with correct timing
- Audio cues play at correct interval transitions
- Crossfade from source to playlist is smooth and audible
- `run_tests.sh` exits 0
- Git clean, journal updated
