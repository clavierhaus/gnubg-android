# Cover letter for bug-gnubg — send with 0001-time-controls.diff

Subject: [PATCH] Time controls: completing the manual's oldest promise

---

Hello,

The GNU Backgammon manual for version 0.16 contains a section titled
"Time controls" whose entire text reads: "The time control feature is
not fully implemented with the user interface yet. Hopefully this will
be improved in the future."

That was some twenty years ago. The attached patch is an attempt to
make good on it.

WHAT IT ADDS

    set clock on|off            (off by default -- a clock you can decline)
    set clock delay <seconds>   (default 12)
    set clock reserve <minutes> (per point of match length; default 2)
    show clock

The model is the current USBGF/WBGF tournament regulation: a per-move
simple delay (US delay: the delay runs first on every turn, unused
delay is never banked) in front of a per-player reserve of 2 minutes
times match length. The clock is armed at the first game of a match
and persists across its games; each change of the side to move grants
a fresh delay -- one press of a physical clock per turn, cube actions
included.

A fallen flag is REPORTED, never enforced: gnubg prints that the
player's reserve has expired and that the match may be claimed under
tournament rules, and play continues. What a flag means belongs to the
tournament's rulebook, not the engine's; the clock at a real table does
not reach over and concede the match for you, and neither does this
one. Nothing in matchstate, moverecord, or any file format is touched.

HOW IT IS BUILT

timecontrol.c/h is a self-contained pure-C99 state machine with zero
dependencies -- not even glib. It never reads the system clock: every
entry point takes a caller-supplied millisecond timestamp, which turned
out to fit gnubg unusually well, because get_time() in timer.c already
returns exactly that -- monotonic milliseconds. The perfect timestamp
source has been sitting in the tree for years; this module just drinks
from it.

Because it is timestamp-driven, there is NO ticker anywhere: the CLI
charges time at action boundaries, which is precisely how a physical
clock behaves -- it runs while you think and settles when you act. The
whole integration outside the new module is small: two calls in
play.c's turn machinery plus a short glue block, four command handlers
in set.c, one in show.c, the command-table entries, and Makefile.am.
491 insertions in total, of which 308 are the new module and its
header; no existing behaviour changes while the clock is off.

HOW IT IS TESTED

The module carries a standalone harness (I will gladly post it, or a
follow-up patch adding it under a suitable path) that compiles it with
-std=c99 -pedantic -Wall -Wextra -Werror and drives it with simulated
timestamps only. Eight test families: the founding invariant is a
10-point match under the regulation in which every move is completed
within 11 seconds -- four hundred turns later both reserves must be
BIT-IDENTICAL to their starting values. Further rows cover exact
delay-boundary arithmetic (the millisecond at and after expiry),
insensitivity to settle frequency (eighty small settles equal one large
one), flag latching and single delivery including a flag falling
exactly at a hand-over, pause semantics, tolerance of a misbehaving
(decreasing) timestamp source, and a serialization round-trip that
continues bit-identically after restore.

WHAT IS DELIBERATELY NOT IN IT

No per-move time storage (no moverecord or SGF changes -- the clock is
useful without opening a format discussion), no evaluation coupling
(gnubg's play never reads the clock; the engine remains a judge), and
no GTK work. That last one is an invitation rather than an omission:
tc_state() returns everything a display needs -- active side, running
delay, both reserves, fallen flags -- so a clock face beside the board
is an afternoon's work for someone who knows gtkboard.c far better
than I do. I would be delighted if this patch caused that afternoon.

The diff is against today's git head (0143cad). I am happy to rework
any of it to the project's taste, to split it differently, and to
complete FSF copyright assignment if that is the current practice --
tell me the procedure and I will follow it.

I owe this engine a great deal; my Android port of it exists entirely
on this project's shoulders. This is a first attempt to pay something
back in the only currency that counts here.

Best regards,
Peter

[attach: 0001-time-controls.diff]
