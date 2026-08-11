# Time controls for GNU Backgammon — design and upstream RFC

Status: design ruling of 2026-08-11 (maintainer). This document is written
double-duty: the port's binding design, and the RFC that accompanies the
eventual patch to bug-gnubg. Public under the documentation law.

## 1. The case

GNU Backgammon documented its own intention here two decades ago: the
V0.16 manual carries a "Time controls" node whose entire content is *"The
time control feature is not fully implemented with the user interface
yet. Hopefully this will be improved in the future."* The feature was
never finished; the modern tree contains no trace of it. Meanwhile the
game moved: delay clocks are standard at live tournaments (USBGF/WBGF
regulation: 12-second Simple Delay per move, reserve of 2 minutes ×
match length, reserve expiration loses the match), and timed play is the
online norm. An analyst that cannot represent the ordinary competitive
conditions of 2026 is incomplete in a way the project itself already
acknowledged in 2006.

This work completes the abandoned feature. It is contributed by a
downstream port (CBG, the Android port) that needs it and has
field-tested it; the design keeps the engine's philosophy intact — see
§3 on what is deliberately NOT touched.

## 2. Scope, phase 1

A self-contained module `timecontrol.c` / `timecontrol.h`:

- **Pure state machine, no I/O, no threads, no globals.** An opaque
  `timecontrol` struct; every transition is an explicit function taking
  the struct and a caller-supplied monotonic timestamp in milliseconds.
  The module never reads the system clock itself — that single decision
  makes it deterministic, host-testable to the millisecond, and immune
  to the timer-source quirks of any platform (mobile Doze, VM
  suspension, NTP steps).
- **Model:** per-player reserve bank + per-move simple delay (Bronstein
  US-delay semantics: the delay runs first on every turn, unused delay
  is never banked, the reserve depletes only after the delay is
  exhausted). Parameters: delay ms, reserve ms per player. The
  federation regulation (12 s, 2 min/pt × length) is the documented
  default; the module itself is parameter-agnostic.
- **API shape (final names with the code):**
  `tc_init(tc, delay_ms, reserve_p0_ms, reserve_p1_ms)`;
  `tc_start_turn(tc, player, now_ms)`;
  `tc_tick(tc, now_ms)` → returns event (NONE / FLAG_P0 / FLAG_P1);
  `tc_pause(tc, now_ms)` / `tc_resume(tc, now_ms)`;
  `tc_state(tc, out)` for display (active side, delay remaining,
  both reserves).
- **Timeout is an event, not a game result.** The module reports the
  flag; what a flag *means* (match forfeit under tournament rules) is
  the caller's rulebook. The engine's match state is never touched.

## 3. What is deliberately NOT in phase 1

- **No `moverecord` or SGF schema changes.** Per-move time storage is
  the controversial surface; the clock is useful without it, and a
  schema debate must not sink the module. A later phase may propose
  optional properties.
- **No evaluation coupling.** gnubg's play never reads the clock; the
  engine remains a judge. (Playing-strength-under-time is explicitly
  out of scope.)
- **No GTK work in the first patch.** Command layer only
  (`set tc delay/reserve/on/off`, `show tc`), keeping the reviewable
  surface minimal; the GTK clock display can follow as its own patch.

## 4. The harness (the teeth-gnawing part)

`tools/clock_harness/` builds the module on the host and asserts, with
simulated timestamps (never wall time):

- **The maintainer's scenario, permanently:** a 10-point match, every
  turn acted within 11 s of a 12 s delay, hundreds of turns — both
  reserves BYTE-EQUAL to their initial values at the end.
- Boundary rows: action at exactly delay expiry; 1 ms past; delay
  spanning multiple ticks; reserve exhaustion mid-tick (flag fires once,
  at the correct millisecond, for the correct player).
- Pause/resume across turn boundaries; pause during delay vs during
  bank; monotonic-time jumps (the caller clamps, but the module must be
  well-defined under any non-decreasing input).
- Turn-change storms (rapid side flips must each grant a full fresh
  delay and charge nothing).
- Serialization round-trip (save/load mid-turn reproduces state
  exactly) — needed for gnubg's match save and CBG's process death.

Exit-coded, run on every change, per the repository's standing law.

## 5. Integration — organic, and deliberately blind to the GUI

The design premise makes the deepest integration question dissolve: a
timestamp-driven module needs **no ticker inside gnubg at all**. The
command-line program blocks at its prompt while the player thinks —
which is exactly what a physical clock at the table does: it runs while
you think and settles when you act. Time is therefore charged at action
boundaries, and the whole integration is a handful of one-line calls at
verified anchors in the existing source:

- **New files** `timecontrol.c` / `timecontrol.h`, added to
  `Makefile.am`. The module is pure C99 — `stdint.h`, an opaque struct,
  const-correct, no globals, and **zero dependencies, not even glib** —
  so it compiles and unit-tests anywhere a C compiler exists. The one
  static instance and its glue live on the gnubg side, not in the
  module.
- **Command surface**, in the house style of `set.c` (the
  `CommandSetClockwise` naming family, a `command acSetClock[]`
  subtable declared in `backgammon.h` and registered in the main
  command tree): `set clock on|off`, `set clock delay <seconds>`,
  `set clock reserve <minutes-per-point>`, and `show clock` printing
  both reserves, the running delay, and any fallen flag. Off by
  default: a clock you can decline, always.
- **Turn-lifecycle hooks**, four one-liners at anchors verified in the
  current source: `NewGame()` (play.c:844) initialises the match's
  clock from length and settings; the turn hand-over in
  `TurnDone()` / `NextTurn()` (play.c:1663 / 1767) settles the
  outgoing player's elapsed time (delay first, then reserve) and
  starts the incoming player's delay. Timestamps come from the
  program's existing time source (`get_time()`, already exported in
  backgammon.h), converted to non-decreasing milliseconds in the glue —
  the module is well-defined under any monotone input by contract.
- **A fallen flag is reported, never enforced**: one line through
  gnubg's normal output channel and a fact in `show clock`. What a
  flag means — a forfeit under tournament regulations — is the
  operator's act, exactly as at a real table where the clock does not
  reach over and concede the match for you. No match-state surgery,
  phase 1 or ever, without the maintainers asking for it.
- **The GUI is untouched — and that is the invitation, not an
  omission.** `tc_state()` returns everything a display needs: active
  side, running delay, both reserves, flags. It is a ready-made
  display contract. A GTK clock beside the board is an afternoon's
  work for someone who knows `gtkboard.c` — and that afternoon is
  deliberately left on the table for the list. The patch brings the
  engine of the clock; the face of it belongs to whoever has waited
  twenty years to draw one.

**CBG side:** the facade exports the same tc calls; the Kotlin clock
state machine is DELETED and replaced by a thin poller + display.
Display ruling (maintainer, 2026-08-11): TWO clocks, both players always
visible, Galaxy-style — the active side carries the loud delay
countdown, the inactive side shows its bank dimmed and static; sizes
readable across a table. The Off/Competition switch and the ledger's
clock provenance are unchanged, already-shipped app-layer policy.

## 6. Sequencing

1. Module + harness in this tree (engine-core/timecontrol.c is
   port-owned NEW code, upstream-styled; no vendored file is modified).
2. CBG facade + display replacement; device verification of the
   maintainer's three field findings (size, two clocks, untouched banks
   under sub-delay play).
3. Patch preparation against gnubg git head; RFC (this document,
   §§1-4) + patch to bug-gnubg; FSF assignment as required.
