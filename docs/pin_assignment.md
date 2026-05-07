# GPIO Pin Assignment — Soundboard (Main Device)

**Status: DRAFT — verify against your specific ESP32-S3 module and PCB layout before committing**

## Constraints that shaped this assignment

1. **ADC1 only for potentiometers.** ADC2 is shared with the Wi-Fi RF path. Although this
   project does not use Wi-Fi, the Bluetooth radio can interfere with ADC2 readings.
   All six potentiometers are assigned to ADC1 channels (GPIO 1–10).
   The internal ADC is never used for audio — see ADR-SB-012.

2. **USB OTG requires GPIO 19 (D−) and GPIO 20 (D+).** These are fixed by the silicon.
   USB OTG host mode is used for the USB stick (ADR-SB-002). Single port, single purpose —
   the laptop connects via 3.5mm jack, not USB (ADR-SB-012).

3. **I2S shared bus: PCM1808 (ADC) + PCM5102A (DAC).** Five GPIOs total: BCLK, WS
   (LRCK), DOUT, DIN, MCLK. The ESP32-S3 is I2S master; both ICs are slaves clocked by
   it. MCLK is required by both ICs and is shared on GPIO 18. See ADR-SB-012.

4. **Strapping pins avoided.** GPIO 0 (boot mode), GPIO 3 (JTAG), GPIO 45, GPIO 46 are
   not used for peripherals.

5. **JTAG debugging pins left accessible.** GPIO 39–42 are reserved for JTAG in
   bring-up. Reassign if pin count becomes tight in final hardware.

6. **Physical source switch.** Hardware switch — firmware reads it as a GPIO input.
   One GPIO, pull-up, active low.

---

## Assignment table

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 1 | POT_1_LAPTOP_VOL | ADC1 IN | Laptop volume pot |
| 2 | POT_2_BT_VOL | ADC1 IN | Bluetooth volume pot |
| 3 | POT_3_SB_VOL | ADC1 IN | Soundboard volume pot |
| 4 | POT_4_DUCK_AGGR | ADC1 IN | Ducking aggressiveness pot |
| 5 | POT_5_DUCK_LEVEL | ADC1 IN | Ducking level pot |
| 6 | POT_6_PAUSE_VOL | ADC1 IN | Pause volume pot |
| 7 | BTN_SB_1 | IN | Soundboard button 1, pull-up, active low |
| 8 | BTN_SB_2 | IN | Soundboard button 2, pull-up, active low |
| 9 | BTN_SB_3 | IN | Soundboard button 3, pull-up, active low |
| 10 | BTN_SB_4 | IN | Soundboard button 4, pull-up, active low |
| 11 | BTN_SB_5 | IN | Soundboard button 5, pull-up, active low |
| 12 | BTN_SB_6 | IN | Soundboard button 6, pull-up, active low |
| 13 | BTN_SET_PAUSE | IN | Set Pause button, pull-up, active low |
| 14 | SW_SOURCE | IN | Physical source switch (LAPTOP=low, BT=high) |
| 15 | I2S_BCLK | OUT | I2S bit clock — shared bus master to PCM1808 + PCM5102A |
| 16 | I2S_WS | OUT | I2S word select / LRCK — shared bus master |
| 17 | I2S_DOUT | OUT | I2S data out → PCM5102A DAC → audio output |
| 18 | I2S_MCLK | OUT | Master clock — shared to PCM1808 and PCM5102A ✓ CONFIRMED |
| 19 | USB_D_MINUS | USB | Fixed — USB OTG D− |
| 20 | USB_D_PLUS | USB | Fixed — USB OTG D+ |
| 21 | I2S_DIN | IN | I2S data in ← PCM1808 ADC ← 3.5mm laptop jack ✓ CONFIRMED |
| 35 | LED_STATUS | OUT | System status LED (boot OK / fault) |
| 36 | LED_BT_CONN | OUT | Bluetooth connected indicator |
| 37 | LED_USB_MOUNT | OUT | USB stick mounted indicator |
| 39 | JTAG_TCK | — | Reserved for bring-up debugging |
| 40 | JTAG_TDI | — | Reserved for bring-up debugging |
| 41 | JTAG_TDO | — | Reserved for bring-up debugging |
| 42 | JTAG_TMS | — | Reserved for bring-up debugging |

## PCM1808 hardware strapping

The PCM1808 requires no I2C or SPI configuration — format and mode are selected by
three logic pins strapped at board level. Set these during PCB layout; they are not
connected to ESP32-S3 GPIOs.

| PCM1808 Pin | Strap | Meaning |
|-------------|-------|---------|
| FMT (pin 12) | GND | I2S standard format (matches PCM5102A default) |
| MD0 (pin 14) | GND | Single-speed mode (fs up to 50 kHz) |
| MD1 (pin 15) | GND | Single-speed mode |

Confirm fs = 48 kHz during bring-up (48 kHz is preferred over 44.1 kHz for ESP-ADF
pipelines — the resampler overhead is lower and Bluetooth A2DP typically delivers 44.1
kHz which ADF resamples before mixing).

## HAL constant names

These names are used in `hal.h` and the Rust FFI structs. Do not use raw GPIO numbers in
application code.

```c
// Potentiometers — ADC1 channel indices
#define HAL_ADC_CH_LAPTOP_VOL   0   // GPIO 1
#define HAL_ADC_CH_BT_VOL       1   // GPIO 2
#define HAL_ADC_CH_SB_VOL       2   // GPIO 3
#define HAL_ADC_CH_DUCK_AGGR    3   // GPIO 4
#define HAL_ADC_CH_DUCK_LEVEL   4   // GPIO 5
#define HAL_ADC_CH_PAUSE_VOL    5   // GPIO 6

// Soundboard buttons
#define HAL_BTN_SB_1            7
#define HAL_BTN_SB_2            8
#define HAL_BTN_SB_3            9
#define HAL_BTN_SB_4            10
#define HAL_BTN_SB_5            11
#define HAL_BTN_SB_6            12
#define HAL_BTN_SET_PAUSE       13

// Source switch
#define HAL_PIN_SOURCE_SWITCH   14

// I2S — shared bus, ESP32-S3 as master
#define HAL_PIN_I2S_BCLK        15  // Bit clock (master out)
#define HAL_PIN_I2S_WS          16  // Word select / LRCK (master out)
#define HAL_PIN_I2S_DOUT        17  // Data out → PCM5102A DAC
#define HAL_PIN_I2S_MCLK        18  // Master clock → PCM1808 + PCM5102A
#define HAL_PIN_I2S_DIN         21  // Data in ← PCM1808 ADC

// LEDs
#define HAL_PIN_LED_STATUS      35
#define HAL_PIN_LED_BT_CONN     36
#define HAL_PIN_LED_USB_MOUNT   37
```

## Open questions (resolve before PCB layout)

- [x] ~~Does the chosen DAC IC require MCLK?~~ **Resolved:** PCM5102A requires MCLK.
      GPIO 18 allocated. PCM1808 also uses MCLK — shared on same line. (ADR-SB-012)
- [x] ~~Is a line-in ADC required?~~ **Resolved:** Yes. Laptop connects via 3.5mm jack.
      PCM1808 ADC selected. GPIO 21 (I2S_DIN) confirmed active. (ADR-SB-012)
- [x] ~~Confirm the ESP32-S3 module variant.~~ **Resolved:** ESP32-S3-DevKitC-1.
      USB port is OTG-capable by default on this board. GPIO availability confirmed
      against the DevKitC-1 pinout — all assigned pins are broken out.
- [ ] **BLE coexistence test** — confirm simultaneous A2DP + BLE central works without
      significant audio glitching. This is a known complexity point from ADR-SB-005.
      **Deliberately deferred.** PCB layout is far ahead of current project stage.
      This test must be completed before committing to a PCB but not before Phase A–C
      firmware is working on devkits. Revisit when Phase B (Bluetooth) is GREEN.
