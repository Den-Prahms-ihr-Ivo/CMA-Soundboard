# Soundboard — Project Description

## Purpose

The Soundboard is an intermediary audio device designed for use at a sports club where the current audio setup was reported as no longer fun or effective during training. It sits between a laptop and an audio mixer, handling Bluetooth connectivity, audio source switching, sound playback, and intelligent ducking so that soundbytes are heard clearly without abruptly muting the background music.

## Primary use case

The device operates in a sports hall with no internet connection. A trainer runs music from a laptop or connects a phone via Bluetooth. When a soundboard button is pressed — live at the device or via wireless remote — the background music ducks (fades, not mutes) while the soundbyte plays, then recovers. A dedicated pause button reduces volume so athletes can talk between sets.

## Context within the four-project roadmap

Project 2 of 4. The Temptation Box (project 1, pure C) established the methodology: TDD with host/target split, HAL abstraction, ADR documentation, requirements-first development. The Soundboard carries all of this forward and introduces Rust incrementally once the core audio pipeline is proven in C. The Smart Timer (project 3) deepens hybrid C/Rust. The Gun Alarm Clock (project 4) targets pure Rust.

## Feature overview

| Category | Feature | Notes |
|---|---|---|
| Bluetooth | Auto-connect to known devices on boot | Maintains known device list in NVS |
| Bluetooth | Toggle between active connected devices | Easy playlist handover between people |
| Audio source | Physical switch: laptop vs Bluetooth | Hard switch, not software-only |
| Soundboard | Six soundboard buttons | Triggerable locally and via remote |
| Soundboard | Sound files from USB stick | No internet required, swap to update |
| Ducking | Ducking gate on soundbyte playback | Fades, does not mute |
| Ducking | Six potentiometers for mix control | See potentiometer table below |
| Remote | Wireless remote — at minimum Set Pause | BLE |
| Remote | MagSafe compatible charging | Rudimentary BMS |
| Remote BMS | Charges to 80% maximum | Protects battery longevity |
| Remote BMS | LED warnings at 30% / 20% / 10% | Amber / orange / red |

## Potentiometer assignment

| # | Name | Controls |
|---|---|---|
| 1 | Laptop volume | Overall level of laptop audio input |
| 2 | Phone / BT volume | Overall level of Bluetooth audio input |
| 3 | Soundboard volume | Overall level of soundbyte playback |
| 4 | Ducking aggressiveness | How quickly background fades on soundbyte trigger |
| 5 | Ducking level | How far background volume drops during a soundbyte |
| 6 | Pause volume | Volume level during Set Pause — lets athletes talk |

## Desired project qualities

The device must work reliably in a sports hall with no internet, must be operable by a non-technical trainer without referring to a manual, and must be physically robust enough to survive regular transport and setup. It should be impressive as a portfolio piece while remaining scoped tightly enough to finish. It must not become a full DJ controller or require a laptop to configure.

## What this project is not

Not a full DJ controller. Not cloud-connected. Not dependent on a companion app. Not a professional audio interface. The USB stick approach deliberately trades flexibility for simplicity and offline resilience.
