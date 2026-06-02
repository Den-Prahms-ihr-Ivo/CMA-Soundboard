#include "tabata.h"
#include "hal_buttons_and_switches.h"
#include <string.h>

void tabata_init(tabata_t *tab)
{
    memset(tab, 0, sizeof(*tab));
    tab->phase       = TABATA_PHASE_IDLE;
    tab->pending_cue = TABATA_CUE_NONE;
}

void tabata_tick(tabata_t *tab)
{
    bool btn    = hal_button_is_pressed(HAL_BUTTON_TABATA);
    bool rising = btn && !tab->prev_tabata_btn;
    tab->prev_tabata_btn = btn;

    if (rising)
    {
        if (!tab->active)
        {
            tab->active      = true;
            tab->phase       = TABATA_PHASE_WORK;
            tab->round       = 1;
            tab->ticks_left  = TABATA_WORK_DURATION_S;
            tab->pending_cue = TABATA_CUE_WORK;
        }
        else
        {
            tab->active = false;
            tab->phase  = TABATA_PHASE_IDLE;
        }
        return;
    }

    if (!tab->active)
        return;

    tab->ticks_left--;
    if (tab->ticks_left > 0)
        return;

    if (tab->phase == TABATA_PHASE_WORK)
    {
        tab->phase       = TABATA_PHASE_REST;
        tab->ticks_left  = TABATA_REST_DURATION_S;
        tab->pending_cue = TABATA_CUE_REST;
    }
    else
    {
        if (tab->round >= TABATA_ROUNDS)
        {
            tab->active      = false;
            tab->phase       = TABATA_PHASE_IDLE;
            tab->pending_cue = TABATA_CUE_COMPLETE;
        }
        else
        {
            tab->round++;
            tab->phase       = TABATA_PHASE_WORK;
            tab->ticks_left  = TABATA_WORK_DURATION_S;
            tab->pending_cue = TABATA_CUE_WORK;
        }
    }
}

bool           tabata_is_active(tabata_t *tab) { return tab->active; }
tabata_phase_t tabata_get_phase(tabata_t *tab) { return tab->phase; }
int            tabata_get_round(tabata_t *tab) { return tab->round; }

tabata_cue_t tabata_consume_cue(tabata_t *tab)
{
    tabata_cue_t cue = tab->pending_cue;
    tab->pending_cue = TABATA_CUE_NONE;
    return cue;
}
