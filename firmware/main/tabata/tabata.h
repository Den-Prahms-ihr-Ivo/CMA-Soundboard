#ifndef TABATA_H
#define TABATA_H

#include <stdbool.h>

#define TABATA_WORK_DURATION_S 20
#define TABATA_REST_DURATION_S 10
#define TABATA_ROUNDS          8

typedef enum
{
    TABATA_PHASE_IDLE,
    TABATA_PHASE_WORK,
    TABATA_PHASE_REST,
} tabata_phase_t;

typedef enum
{
    TABATA_CUE_NONE,
    TABATA_CUE_WORK,
    TABATA_CUE_REST,
    TABATA_CUE_COMPLETE,
} tabata_cue_t;

typedef struct
{
    tabata_phase_t phase;
    int            round;
    int            ticks_left;
    bool           active;
    tabata_cue_t   pending_cue;
    bool           prev_tabata_btn;
} tabata_t;

void           tabata_init(tabata_t *tab);

/* Call once per second. Detects HAL_BUTTON_TABATA (rising edge) to start or
 * abort the session. Advances the interval timer while active. */
void           tabata_tick(tabata_t *tab);

bool           tabata_is_active(tabata_t *tab);
tabata_phase_t tabata_get_phase(tabata_t *tab);
int            tabata_get_round(tabata_t *tab);

/* Returns the pending cue and clears it to TABATA_CUE_NONE (one-shot). */
tabata_cue_t   tabata_consume_cue(tabata_t *tab);

#endif /* TABATA_H */
