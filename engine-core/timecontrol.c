/*
 * timecontrol.c -- tournament match clock for GNU Backgammon.
 *
 * Copyright (C) 2026 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * See timecontrol.h for the model.  Invariants maintained here:
 *
 *  I1  Reserves never underflow and never move while the delay has
 *      remainder: a turn completed within the delay leaves the
 *      reserve BIT-IDENTICAL to its previous value.
 *  I2  Settling is insensitive to call frequency: many small settles
 *      and one large settle over the same interval charge the same
 *      amounts.
 *  I3  A flag is raised exactly once per player and is latched; the
 *      other player's clock continues to serve normally.
 *  I4  Any non-decreasing timestamp source is legal; a decreasing
 *      timestamp (a broken caller) charges nothing rather than
 *      corrupting state.
 *  I5  Paused time is charged to nobody.
 */

#include <stdio.h>
#include <string.h>

#include "timecontrol.h"

extern void
tc_init(timecontrol * ptc, uint64_t msDelay, uint64_t msReserve0, uint64_t msReserve1)
{
    memset(ptc, 0, sizeof *ptc);
    ptc->fActive = 1;
    ptc->nActiveSide = -1;
    ptc->msDelay = msDelay;
    ptc->amsReserve[0] = msReserve0;
    ptc->amsReserve[1] = msReserve1;
}

/* Charge [msLast, msNow) to the running side, delay first.  The
 * caller has checked fActive. */
static tcevent
settle(timecontrol * ptc, uint64_t msNow)
{
    uint64_t msElapsed;
    int nSide = ptc->nActiveSide;

    if (msNow <= ptc->msLast) {  /* I4: never charge on a broken source */
        return TC_EVENT_NONE;
    }
    msElapsed = msNow - ptc->msLast;
    ptc->msLast = msNow;

    if (ptc->fPaused || nSide < 0)
        return TC_EVENT_NONE;    /* I5: nobody is charged */

    if (ptc->msDelayLeft >= msElapsed) {
        ptc->msDelayLeft -= msElapsed;
        return TC_EVENT_NONE;    /* I1: reserve untouched */
    }
    msElapsed -= ptc->msDelayLeft;
    ptc->msDelayLeft = 0;

    if (ptc->afFlagged[nSide])
        return TC_EVENT_NONE;    /* I3: latched, reserve stays zero */

    if (ptc->amsReserve[nSide] > msElapsed) {
        ptc->amsReserve[nSide] -= msElapsed;
        return TC_EVENT_NONE;
    }
    ptc->amsReserve[nSide] = 0;
    ptc->afFlagged[nSide] = 1;
    return nSide == 0 ? TC_EVENT_FLAG_PLAYER0 : TC_EVENT_FLAG_PLAYER1;
}

extern tcevent
tc_settle(timecontrol * ptc, uint64_t msNow)
{
    if (!ptc->fActive)
        return TC_EVENT_NONE;
    return settle(ptc, msNow);
}

extern tcevent
tc_start_turn(timecontrol * ptc, int nSide, uint64_t msNow)
{
    tcevent ev;

    if (!ptc->fActive)
        return TC_EVENT_NONE;

    ev = settle(ptc, msNow);     /* the incumbent pays up to the hand-over */

    if (nSide < 0 || nSide >= TC_PLAYERS) {
        ptc->nActiveSide = -1;
        ptc->msDelayLeft = 0;
    } else {
        ptc->nActiveSide = nSide;
        ptc->msDelayLeft = ptc->msDelay;  /* every move earns a fresh delay */
    }
    return ev;
}

extern tcevent
tc_pause(timecontrol * ptc, uint64_t msNow)
{
    tcevent ev;

    if (!ptc->fActive || ptc->fPaused)
        return TC_EVENT_NONE;
    ev = settle(ptc, msNow);
    ptc->fPaused = 1;
    return ev;
}

extern void
tc_resume(timecontrol * ptc, uint64_t msNow)
{
    if (!ptc->fActive || !ptc->fPaused)
        return;
    ptc->fPaused = 0;
    if (msNow > ptc->msLast)
        ptc->msLast = msNow;     /* the gap is charged to nobody (I5) */
}

extern void
tc_state(const timecontrol * ptc, tcstate * pstate)
{
    int i;

    pstate->fActive = ptc->fActive;
    pstate->fPaused = ptc->fPaused;
    pstate->nActiveSide = ptc->nActiveSide;
    pstate->msDelayLeft = ptc->msDelayLeft;
    for (i = 0; i < TC_PLAYERS; ++i) {
        pstate->amsReserve[i] = ptc->amsReserve[i];
        pstate->afFlagged[i] = ptc->afFlagged[i];
    }
}

extern int
tc_serialize(const timecontrol * ptc, char *sz, size_t cch)
{
    return snprintf(sz, cch, "tc1 %d %d %d %llu %llu %llu %llu %d %d %llu",
                    ptc->fActive, ptc->fPaused, ptc->nActiveSide,
                    (unsigned long long) ptc->msDelay,
                    (unsigned long long) ptc->msDelayLeft,
                    (unsigned long long) ptc->amsReserve[0],
                    (unsigned long long) ptc->amsReserve[1],
                    ptc->afFlagged[0], ptc->afFlagged[1],
                    (unsigned long long) ptc->msLast);
}

extern int
tc_deserialize(timecontrol * ptc, const char *sz)
{
    timecontrol tc;
    unsigned long long d, dl, r0, r1, last;

    memset(&tc, 0, sizeof tc);
    if (sscanf(sz, "tc1 %d %d %d %llu %llu %llu %llu %d %d %llu",
               &tc.fActive, &tc.fPaused, &tc.nActiveSide,
               &d, &dl, &r0, &r1,
               &tc.afFlagged[0], &tc.afFlagged[1], &last) != 10)
        return -1;
    if (tc.nActiveSide < -1 || tc.nActiveSide >= TC_PLAYERS)
        return -1;
    tc.msDelay = d;
    tc.msDelayLeft = dl;
    tc.amsReserve[0] = r0;
    tc.amsReserve[1] = r1;
    tc.msLast = last;
    *ptc = tc;
    return 0;
}
