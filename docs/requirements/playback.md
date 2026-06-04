# Requirements — Playback Modes (Phase A.5 → Phase B)

Module: `soundboard_mixer` (host-testable extensions) and `playback_manager` (Phase B, target-only)
Language: C
Test runner: `./run_tests.sh` (Unity tests on host)
Phase gate: Phase A GREEN before any work in this file begins

This file bridges Phase A and Phase B. REQ-PLAY-001 and 002 extend the host-testable mixer
with override transitions and playlist navigation. REQ-PLAY-003 covers folder enumeration.
REQ-PLAY-004 through 006 are higher-level mode behaviours that depend on USB mass storage
and the ADF pipeline — these are target-only and verified on hardware.

The split exists because override transitions and playlist logic are pure mixer concerns
that benefit from being designed and tested in isolation before being tangled up with
file I/O and ADF integration.

---

## Override modes overview

There are three source-override modes: **MusicMode**, **TabataMode**, and **ChillMode**.
Each has its own dedicated button (`HAL_BUTTON_MUSIC_MODE`, `HAL_BUTTON_TABATA`,
`HAL_BUTTON_CHILL`) and its own folder on the USB stick (`MUSIC/`, `TABATA/`, `CHILL/`).

Button semantics are uniform across all three modes:

- Press mode button while no override active → enter that mode
- Press the _same_ mode button while in that mode → exit override, return to source
- Press a _different_ mode button while in some other override → swap directly to the new mode
- Toggle the physical source switch → exit any active override, return to new source

Crossfade rates differ by destination:

- `PLAYLIST_CROSSFADE_STEP_FAST` (~1s): used when transitioning _into_ MusicMode or TabataMode, and used for all _exits_ back to source
- `PLAYLIST_CROSSFADE_STEP_SLOW` (~3s): used when transitioning _into_ ChillMode

The crossfade rate is a property of the destination, not the source channel. Transitioning
from Music to Chill uses the slow rate (Chill is the destination). Transitioning from
Chill to Music uses the fast rate (Music is the destination). Exiting any mode back to
source uses the fast rate.

Each mode plays from its folder:

- **MusicMode** — continuous shuffled playback from `MUSIC/`, reshuffles and repeats
- **TabataMode** — plays one random file from `TABATA/`, then automatically returns to previous source
- **ChillMode** — continuous shuffled playback from `CHILL/`, reshuffles and repeats (same playlist behaviour as MusicMode, only the volume pot and crossfade rate differ)

ChillMode has its own dedicated volume potentiometer (`POTI_CHILL_VOL`). MusicMode and
TabataMode share the playlist channel with no per-mode volume pot — they use a default
playlist volume constant or borrow the soundboard volume pot (TBD during implementation).

---

## REQ-PLAY-001 — Crossfade between source and active override

**Status:** GREEN
**Host-testable:** Yes — extends `soundboard_mixer.c` with override state and crossfade logic

**Description**
When the active override changes (source to override, override to source, or override to override),
the mixer crossfades between the relevant channels at the rate determined by the destination
mode.

The mixer maintains:

- `current_override` — the currently active override (NONE, MusicMode, TabataMode, ChillMode)
- `crossfade_state` — internal ramping state for source and playlist channels

On any change to `current_override`, the mixer:

1. Determines the destination's crossfade rate (FAST for None / Music / Tabata; SLOW for Chill)
2. Begins fading the outgoing channel down toward 0 at that rate
3. Begins fading the incoming channel up toward its target volume at that rate
4. Updates the active source / playlist routing once the crossfade completes

The crossfade is smooth — no hard cuts at any point. Both directions of the crossfade
use the same rate (the destination's rate).

**Acceptance criteria**

- No override to MusicMode: source fades down, playlist fades up, both at FAST rate
- No override to ChillMode: source fades down, playlist fades up, both at SLOW rate
- MusicMode to no override: playlist fades down, source fades up, both at FAST rate
- ChillMode to no override: playlist fades down, source fades up, both at FAST rate
- MusicMode to ChillMode: crossfade uses SLOW rate (Chill is destination)
- ChillMode to MusicMode: crossfade uses FAST rate (Music is destination)
- `PLAYLIST_CROSSFADE_STEP_FAST` and `PLAYLIST_CROSSFADE_STEP_SLOW` are named constants
- Crossfade does not interfere with soundbyte ducking on the active channel

**Tests**

```
test_soundboard_mixer.c
  ::test_crossfade_source_to_music_uses_fast
  ::test_crossfade_source_to_chill_uses_slow
  ::test_crossfade_music_to_source_uses_fast
  ::test_crossfade_chill_to_source_uses_fast
  ::test_crossfade_music_to_chill_uses_slow
  ::test_crossfade_chill_to_music_uses_fast
  ::test_crossfade_rates_are_named_constants
  ::test_crossfade_does_not_interfere_with_soundbyte_ducking
```

---

## REQ-PLAY-002 — Override button semantics

**Status:** GREEN
**Host-testable:** Yes — extends `soundboard_mixer.c` with button-to-override mapping

**Description**
The mixer interprets override button presses according to uniform semantics:

- Press a mode button while no override is active → set override to that mode
- Press the same mode button while that mode is active → clear override (return to source)
- Press a different mode button while some override is active → swap directly to the new override
- Toggle the physical source switch while any override is active → clear override (return to new source)

The override change in turn triggers a crossfade per REQ-PLAY-001. The mixer does not
need to know about button debouncing — it receives clean button events from the HAL.

**Acceptance criteria**

- override = None, press Music button → override = MusicMode
- override = MusicMode, press Music button → override = None
- override = MusicMode, press Chill button → override = ChillMode (no intermediate None)
- override = ChillMode, press Music button → override = MusicMode (no intermediate None)
- override = MusicMode, press Tabata button → override = TabataMode
- override = MusicMode, toggle source switch → override = None
- override change always triggers a crossfade (REQ-PLAY-001 applies)
- override button events are edge-triggered, not level-triggered.

**Tests**

```
test_soundboard_mixer.c
  ::test_music_button_enters_music_mode
  ::test_music_button_exits_music_mode
  ::test_chill_button_enters_chill_mode
  ::test_chill_button_exits_chill_mode
  ::test_tabata_button_enters_tabata_mode
  ::test_tabata_button_exits_tabata_mode
  ::test_music_to_chill_via_chill_button
  ::test_chill_to_music_via_music_button
  ::test_source_switch_toggle_clears_override
```

---

## REQ-PLAY-003 — File loader enumerates folder and selects file

**Status:** TODO
**Host-testable:** Partial — folder-mapping logic can be tested with a mock filesystem,
but actual FAT/USB reads require target hardware

**Description**
The C file loader module is responsible for mapping a button index or mode to a folder
path, enumerating the files in that folder, and selecting one for playback. Behaviour
depends on context:

- Soundboard buttons → `SB1/` through `SB6/` folders, **random** file selection per press
- MusicMode → `MUSIC/` folder, **shuffled sequential** playback
- ChillMode → `CHILL/` folder, **shuffled sequential** playback
- TabataMode → `TABATA/` folder, **random** file selection (single file per session)

Random seed is derived from `esp_random()` on boot and not reset during operation.
Only WAV and MP3 files are considered — other file types in the folder are ignored.

The file loader exposes `playlist_length` to the mixer when a playlist folder is loaded.

**Acceptance criteria**

- Button index 0 → enumerates `SB1/` folder, selects random WAV or MP3
- Button index 5 → enumerates `SB6/` folder, selects random WAV or MP3
- MusicMode load → reads `MUSIC/`, reports playlist_length matching file count
- ChillMode load → reads `CHILL/`, reports playlist_length matching file count
- TabataMode load → reads `TABATA/`, selects random file (single file, not a playlist)
- Empty folder → returns no file, no crash
- Folder missing → returns no file, no crash
- Non-audio files in folder → ignored, not selected
- Random selection uses esp_random() seed, not a fixed seed
- Named constants: `SB_FOLDER_PREFIX` ("SB"), `MUSIC_FOLDER` ("MUSIC"),
  `CHILL_FOLDER` ("CHILL"), `TABATA_FOLDER` ("TABATA")

**Tests**

```
test_file_loader.c
  ::test_loader_maps_button_index_to_folder
  ::test_loader_loads_music_playlist
  ::test_loader_loads_chill_playlist
  ::test_loader_loads_tabata_random_file
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
Music mode is activated by the Music button. When active:

1. Override is set to MusicMode (REQ-PLAY-002)
2. Source channel crossfades down, playlist channel crossfades up at FAST rate (REQ-PLAY-001)
3. `MUSIC/` folder is enumerated, shuffled, and `playlist_length` set on the mixer (REQ-PLAY-003)
4. First track plays; on end-of-file, next track in shuffle plays
5. After all tracks play, the shuffle reshuffles and continues

Soundboard buttons remain functional during music mode — soundbytes play over the music
with ducking applied to the playlist channel (REQ-AUDIO-003).

Music mode deactivates per REQ-PLAY-002:

- Second press of Music button → exit to source
- Press of Chill or Tabata button → swap to that mode
- Source switch toggle → exit to source

On exit: current track stops, crossfade to source channel at FAST rate.

**Acceptance criteria** (verified on hardware)

- Music button press → audible crossfade from source to `MUSIC/` files (~1s)
- Tracks play continuously without audible gap at transitions
- After all tracks → playlist reshuffles and continues without user action
- Soundboard button during music mode → soundbyte audible over ducked music
- Music button press again → audible crossfade back to source (~1s)
- Chill button while in music mode → audible crossfade to chill (~3s)
- Source switch toggle → audible crossfade back to new source
- `MUSIC/` folder empty or missing → music button has no effect, no crash

**Tests**

```
Target-only — no host test. Verified by ear during hardware bring-up.
LED ring shows red while music mode active.
```

---

## REQ-PLAY-005 — Tabata mode plays one random track and returns to previous source

**Status:** GREEN
**Host-testable:** Partial — override entry/exit on track completion is host-testable;
actual file playback requires hardware

**Description**
Tabata mode is activated by the Tabata button. When active:

1. The previous source (or override) is captured before transition
2. Override is set to TabataMode (REQ-PLAY-002)
3. Crossfade from previous channel to playlist channel at FAST rate (REQ-PLAY-001)
4. `TABATA/` folder is enumerated and one file is selected at random (REQ-PLAY-003)
5. The selected file plays to completion
6. On end-of-file, override is automatically cleared and the previous state is restored
   with a FAST crossfade

Soundboard buttons remain functional during Tabata — soundbytes play over the Tabata
track with ducking applied to the playlist channel.

Tabata mode also exits early per REQ-PLAY-002:

- Press Tabata button again → exit immediately, return to previous source
- Press Music or Chill button → swap to that mode (does not return to previous source)
- Source switch toggle → exit to new source

This is the deliberate simplification of the original Tabata design: no interval timing,
no work/rest counting, no audio cue orchestration. The mixed track contains all of that
baked in. The firmware's job is to play it and return.

**Acceptance criteria**

- Tabata button press while in source mode → override = TabataMode, previous source stored as Laptop or Bluetooth
- Tabata button press while in Music or Chill → override = TabataMode, previous override stored
- Track end-of-file signal → override clears, previous state restored
- Tabata button press during playback → override clears immediately (no wait for end)
- Music or Chill button during Tabata → swap to that mode, do not restore previous
- `TABATA/` folder empty → Tabata button has no effect, no crash
- Previous source/override state correctly restored after track ends

**Tests**

```
test_soundboard_mixer.c (host-testable parts only)
  ::test_tabata_button_captures_previous_source
  ::test_tabata_button_captures_previous_override
  ::test_tabata_track_complete_restores_previous_source
  ::test_tabata_track_complete_restores_previous_override
  ::test_tabata_button_during_playback_exits_immediately
  ::test_music_button_during_tabata_swaps_without_restore

Target-only verification:
  ::test_tabata_plays_real_file_and_returns  (hardware bring-up)
```

---

## REQ-PLAY-006 — Chill mode plays continuously with slow crossfade

**Status:** TODO
**Host-testable:** No — depends on USB mass storage, FAT filesystem, and ADF pipeline

**Description**
Chill mode is activated by the Chill button. Behaviour is identical to MusicMode except:

- Crossfade uses the SLOW rate (~3s) on entry
- Folder is `CHILL/` instead of `MUSIC/`
- Playlist channel volume is controlled by `POTI_CHILL_VOL` rather than the default
  playlist volume

Chill mode deactivates per REQ-PLAY-002 (same as MusicMode). The 3-second crossfade
also applies to swap-from-other-mode entries (Music to Chill, Tabata to Chill). All exits
from Chill use the FAST rate, because the destination of an exit is the source channel
(or another override using FAST rate).

The intent: Chill is the "stretching music" mode. The longer crossfade and dedicated
volume pot let the trainer set a calmer, quieter feel for cool-down without disturbing
the louder workout volume on the soundboard or laptop channels.

**Acceptance criteria** (verified on hardware)

- Chill button press → audible 3-second crossfade from source to `CHILL/` files
- Tracks play continuously without audible gap at transitions
- After all tracks → playlist reshuffles and continues without user action
- `POTI_CHILL_VOL` controls the playlist channel volume during Chill (not other modes)
- Soundboard button during Chill → soundbyte audible over ducked chill music
- Chill button press again → audible 1-second crossfade back to source
- Music or Tabata button while in Chill → audible 1-second crossfade to that mode
- Source switch toggle → audible 1-second crossfade to new source
- `CHILL/` folder empty or missing → Chill button has no effect, no crash

**Tests**

```
Target-only — no host test. Verified by ear during hardware bring-up.
```

---

## Phase A.5 completion gate (REQ-PLAY-001, REQ-PLAY-002, REQ-PLAY-005 host-testable parts)

These can complete entirely on host before any hardware work begins.

- REQ-PLAY-001 and REQ-PLAY-002 Unity tests GREEN on host
- REQ-PLAY-005 host-testable tests GREEN (override entry/exit, state restoration)
- All Phase A tests still GREEN — no regressions
- `run_tests.sh` exits 0
- Git clean, journal updated

## Phase B playback completion gate (REQ-PLAY-003, 004, 006, and REQ-PLAY-005 target parts)

These require hardware and complete during Phase B alongside Bluetooth.

- REQ-PLAY-003 host-testable tests GREEN
- Manual target test: music mode plays continuously from USB stick (REQ-PLAY-004)
- Manual target test: Tabata plays one track and returns (REQ-PLAY-005)
- Manual target test: chill plays continuously with slow crossfade (REQ-PLAY-006)
- All crossfades smooth and audible at correct rates
- `run_tests.sh` exits 0
- Git clean, journal updated
