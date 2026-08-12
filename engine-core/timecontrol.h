/*
 * timecontrol.h -- tournament match clock for GNU Backgammon.
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
 *
 * Design (see also the accompanying RFC):
 *
 * A pure state machine, C99, no dependencies -- not even glib.  The
 * module never reads the system clock: every entry point takes a
 * caller-supplied timestamp in milliseconds from any non-decreasing
 * source.  That single decision makes the module deterministic,
 * unit-testable to the millisecond on any host, and immune to the
 * timer pathologies of any platform.
 *
 * The model is the tournament standard (USBGF/WBGF): a per-move
 * "simple delay" (US delay / Bronstein style: the delay runs first on
 * every turn, unused delay is never banked) in front of a per-player
 * reserve.  A reserve reaching zero raises a flag EVENT; the module
 * never enforces a result.  What a fallen flag means -- a forfeit
 * under tournament regulations -- belongs to the operator, exactly as
 * at a physical table, where the clock does not reach over and concede
 * the match for you.
 *
 * Because it is timestamp-driven, no ticker is required anywhere: a
 * command-line program simply settles elapsed time at action
 * boundaries, which is precisely how a physical clock behaves -- it
 * runs while you think and settles when you act.  tc_state() exposes
 * everything a display could want; a graphical clock face is left as
 * an exercise for whoever has waited twenty years to draw one.
 */

#ifndef TIMECONTROL_H
#define TIMECONTROL_H

#include <stdint.h>
#include <stddef.h>

#define TC_PLAYERS 2

/* Events reported by settling calls.  A flag is raised exactly once
 * per player; afterwards that player's reserve stays at zero and the
 * clock keeps serving the other player normally. */
typedef enum {
    TC_EVENT_NONE = 0,
    TC_EVENT_FLAG_PLAYER0,
    TC_EVENT_FLAG_PLAYER1
} tcevent;

/* The complete clock state.  The struct is transparent in the house
 * style; treat it as read-only outside timecontrol.c and mutate it
 * only through the functions below. */
typedef struct {
    int fActive;                    /* clock in use for this match */
    int fPaused;
    int nActiveSide;                /* 0 or 1; -1 = no side running */
    uint64_t msDelay;               /* configured per-move delay */
    uint64_t msDelayLeft;           /* delay remaining this turn */
    uint64_t amsReserve[TC_PLAYERS];
    int afFlagged[TC_PLAYERS];      /* latched flag per player */
    uint64_t msLast;                /* timestamp of the last settle */
} timecontrol;

/* Read-only snapshot for displays: everything a clock face needs. */
typedef struct {
    int fActive;
    int fPaused;
    int nActiveSide;
    uint64_t msDelayLeft;
    uint64_t amsReserve[TC_PLAYERS];
    int afFlagged[TC_PLAYERS];
} tcstate;

/* Arm the clock: per-move delay and each player's reserve, in
 * milliseconds.  Resets every flag and stops both clocks; the first
 * tc_start_turn() begins play. */
extern void tc_init(timecontrol * ptc, uint64_t msDelay,
                    uint64_t msReserve0, uint64_t msReserve1);

/* Hand the move to nSide (0 or 1) at time msNow: the incumbent side
 * (if any) is settled up to msNow first, then nSide receives a fresh
 * per-move delay.  Passing nSide == -1 settles and stops both clocks
 * (between games, at match end).  Returns any flag event raised while
 * settling the incumbent. */
extern tcevent tc_start_turn(timecontrol * ptc, int nSide, uint64_t msNow);

/* Charge elapsed time to the running side: delay first, then reserve.
 * Call at any action boundary, and as often as convenient in between
 * (a display poll may settle every frame; frequency never changes the
 * arithmetic).  Returns a flag event at most once per player. */
extern tcevent tc_settle(timecontrol * ptc, uint64_t msNow);

/* Suspend/resume: time between tc_pause() and tc_resume() is charged
 * to nobody. */
extern tcevent tc_pause(timecontrol * ptc, uint64_t msNow);
extern void tc_resume(timecontrol * ptc, uint64_t msNow);

/* Fill a display snapshot. */
extern void tc_state(const timecontrol * ptc, tcstate * pstate);

/* One-line text serialization, for match save/restore.  tc_serialize
 * writes at most cch bytes (NUL-terminated) and returns the number of
 * characters that were (or would have been) written, or -1 on error;
 * tc_deserialize parses a line produced by tc_serialize and returns 0
 * on success, -1 on malformed input (leaving *ptc untouched). */
extern int tc_serialize(const timecontrol * ptc, char *sz, size_t cch);
extern int tc_deserialize(timecontrol * ptc, const char *sz);

#endif                          /* TIMECONTROL_H */
