#ifndef POTI_INPUT_H
#define POTI_INPUT_H

#include <stdint.h>

typedef enum
{
    POTI_LAPTOP_VOL = 0,
    POTI_BLUETOOTH_VOL,
    POTI_SOUNDBOARD_VOL,
    POTI_DUCKING_SPEED,
    POTI_DUCKING_LEVEL,
    POTI_SET_PAUSE_VOL,
    POTI_COUNT
} poti_channel_t;

typedef struct
{
    uint16_t (*read_raw)(poti_channel_t ch);
    float (*read_normalized)(poti_channel_t ch);
} poti_input_t;

#endif
