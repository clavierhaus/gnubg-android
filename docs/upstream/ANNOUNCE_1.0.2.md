To: bug-gnubg@gnu.org
Subject: CBG Pro 1.0.2 (Android port): upstream master b1b2772c merged

Hello,

CBG Pro 1.0.2, the Android port of GNU Backgammon, is on F-Droid and
GitHub (https://github.com/clavierhaus/gnubg-android/releases/tag/v1.0.2).
This is a courtesy note so upstream knows which of its work has reached
users through the port.

The port now vendors gnubg master at b1b2772c (2026-09-10). The 31 commits
since the previous vendoring (June, 284efab7 / 7b2e857d) are all carried.
Two of them change what the app does:

  0faafabb  Fix move comparison functions to return equality -- the coach's
            ranked alternatives; matters on Android, whose qsort differs
            from glibc's for a non-transitive comparator.
  9838b31c  SetCubeInfoMatch(): out-of-bounds gammon price lookup -- cube
            128 and above.

Three reach code the app runs without changing its output (8b28f715,
a8a2df74, b1b2772c). The remainder are carried for fidelity and are inert
in this build -- GTK, USE_MULTITHREAD, bearoff databases (the port ships
none), BBS/dice files, cache resize -- each with its reason recorded in the
port's CHANGELOG.

One finding from the port's side, for the record rather than as a bug in
gnubg: the port's rollout worker pool ran gnubg's NoLocking evaluation
family from several threads against the shared evaluation cache. That is
the port's mistake, not gnubg's -- gnubg only runs that family at one
thread. 1.0.2 runs rollouts serially; the port will move to the WithLocking
family (USE_MULTITHREAD) next, taking whatever thread-count default lands
from the "Change to the default number of evaluation threads" thread
rather than choosing its own.

Best regards,
Peter
