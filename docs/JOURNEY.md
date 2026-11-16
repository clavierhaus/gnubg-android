# The Journey: Porting GNU Backgammon to Android with an AI Agent

*June 12 – November 9, 2026 · 894 commits · 29 release tags · from a five-function
JNI stub to CBG Pro 1.0.4 on F-Droid*

---

## What this document is

`CLAUDE.md` is the law of this repository: the rules an AI coding agent must
follow when it works here. It grew from 146 lines on 2026-06-29 to 1,162 lines
by 2026-10-26, in 38 commits. Read cold, it looks like a wall of prohibitions —
"never speculate", "never assert an unread symbol", "a loop over the board is
almost certainly reinvention". Without context it is easy to call that
overcorrection.

This document is the context. It tells, from the git history, how the app was
built, what went wrong along the way, and why each rule exists. Every incident
here is cited to the commit that recorded it, so any claim can be checked with
`git show <hash>`.

It has two threads that cannot be separated:

1. **Building a real port.** Not an app that happens to embed an engine, but
   GNU Backgammon itself on a phone — same rules, same analysis, same numbers —
   with only the presentation being new.
2. **Learning to work with an AI agent.** Which failure modes showed up, in
   what order, how they were caught, which countermeasures worked, and which
   ones overshot and had to be corrected themselves.

`CLAUDE.md` remains the binding document. This one explains it; it does not
replace it.

---

## Part I — The premise: a port, not an invention

### The first commit (2026-06-12, `ef569da`)

The repository begins with `libgnubg-engine.so` building and running on
Android API 28, arm64: gnubg 1.08.003, GLib 2.88.1 cross-compiled, and a JNI
layer of exactly five entry points — initialise, evaluatePosition,
findBestMove, classifyPosition, applyMove.

That could have been the architecture: an evaluator in a box, and an app
written around it that knows backgammon on its own. It was rejected the next
day.

### "True port, not stripped evaluator" (2026-06-13, `32960fc`)

Phase 8 added gnubg's game layer to the build — `play.c`, `analysis.c`,
`sgf.c`, `set.c`, `matchid.c` and the rest — and replaced only the desktop
shell. `android-app.c` took the portable functions out of `gnubg.c`
verbatim; the GTK calls became Android callbacks (`ShowBoard` →
`gnubg_on_board_changed`, `playSound` → `gnubg_on_sound_event`, and so on).

That decision is the foundation of everything after it: gnubg runs the
match. The app shows it.

### The pull starts immediately (2026-06-14, `e463bcf`)

Two days into the UI, a commit is already titled *"remove all
reimplementations — use gnubg exclusively for moves, pip count, board
swap"*. The first touch-input work had computed moves, pips and the
board swap in Kotlin, because that is the obvious way to write a
board game. Throughout the project, writing your own logic was always
the easy path. Routing through the engine was the deliberate one.

### The facade (2026-06-18 → 06-25)

A platform-neutral C facade (`gnubg_mobile.{c,h}`) was introduced, and in
one day every match command — new game, roll, move, double, take, drop,
resign, accept, reject, agree — was routed through it, one commit per
verb. On 06-25 the remaining direct engine access was moved behind it in
tiers, and the cube stopped being "hand-poked" into the match state
(`2382004`). The layering that `CLAUDE.md` still describes dates from
this week:

```
Compose UI → ViewModel → Engine.kt → JNI (marshalling only)
          → android-app.c + gnubg_mobile facade → engine-core (gnubg, the authority)
```

---

## Part II — From principle to checkpoint (late June – early July)

### The cube incident (2026-06-29, `776bc4a`)

`CLAUDE.md` was first committed that evening (`8333466`), replacing an
auto-generated template. It already said that gnubg is the sole
authority. In practice the agent treated that as background, not as a
test, and a violation got through: the facade's cube decision passed a
**hand-rolled evaluation context** (cubeless, 1-ply) to gnubg's cube
machinery instead of gnubg's own named instances.

The first attempted fix added **a second hand-rolled context on top of
the first**. The commit message calls it "a more sophisticated violation
of the same rule."

The answer was the **PORT CHECKPOINT**: questions to answer in writing
before any code. Q1: which gnubg routine does this? Q2: am I building a
value gnubg already has a named instance of? Q3: does the verb I'm
extending obey Q2 itself? The audit that followed (`41f09b3`) found seven
violations, V1–V7, with a shared shape: **call sites named correctly,
parameters hand-rolled**. Over the next two days each was fixed in its own
commit, routing through `GetEvalCube()`, `GetEvalChequer()`,
`GetMatchStateCubeInfo()` and all six evalsetups.

> **Lesson.** A principle the agent agrees with is not the same as a
> principle it applies. A rule becomes enforceable when it turns into a
> question that must be answered before the work.

### Analysis is gnubg's too (2026-07-03, `d07187d`, `e353ec0`)

The tutor had grown five Kotlin files — `FeatureExtractor`,
`FeatureVector`, `FeatureDelta`, `PositionType`, `TutorAnalyzer` — that
read the board array and described positions on their own: blots,
primes, position class, what was "notable". None of it decided a rule of
backgammon, so none of it looked like a violation of a rule about game
*logic*.

All five were deleted. The rule was widened from logic to *meaning*: if
gnubg computes it internally, recomputing it in Kotlin is reinvention,
even when nothing is being decided. Three tools came with it:

- the **bright-line board-read rule**: Kotlin loops over `board[]` exist
  to draw checkers and hit-test taps;
- **Q4**: if a value is shown to the user as a fact about the game, name
  the gnubg routine that produces it — *if gnubg doesn't expose it, don't
  show it*;
- the **new-file tripwire**: before creating any non-UI file, name the
  gnubg routine it wraps. That one sentence would have stopped all five
  files before their first line.

The July code audit (`4590ada`) applied this to the existing code. Its
first finding, `gnubg_mobile_get_candidates`, was both dead and a
reinvention: it scored moves by hand at a weaker ply than gnubg's own
`FindnSaveBestMoves`. 202 lines were deleted (`b1d6ee0`).

---

## Part III — When the rule is turned on correct code (July 9–10)

This is the most instructive stretch in the history, because the failure
in it is the *opposite* of reinvention.

### Documentation that had gone wrong (`fad3aeb`)

After a burst of code, `STATUS.md` — which called itself "current truth"
— described a hub that no longer existed, a tutor at a ply it no longer
used, a symbol (`fac_ec_default`) that was gone from the tree, and a
missing UI panel that existed. A wrong document is worse than a missing
one: it sends a contributor to fix what isn't broken. Each correction
was recorded rather than silently deleted.

The same commit found two engine divergences labelled "copied verbatim"
that weren't. `EVALSETUP_2PLY` had been transcribed positionally and
slid eight fields, so `nTrials` came out **zero** in all four evalsetups.
The compiler had warned about it 24 times per build for months, and the
warnings had been read as noise. That became the rule that **a gnubg
global defined in the port is engine code**, and a "verbatim" comment is
a claim to verify.

### The duplicate verb (`422d6e0`)

A new resignation verb, `gnubg_mobile_command_agree`, was written next
to an identical one that had existed for months, along with duplicate
declarations, JNI wrappers and Kotlin externals. The device build caught
it. The agent had said it couldn't compile C without the NDK and had
never tested that claim: stock gcc, the glib headers and a one-line
`<android/log.h>` stub were enough. Two things followed:
`tools/syntax_check.sh`, which reproduces the error in about a second,
and **Q0 — does it already exist?**, with the greps to run and paste.
The checkpoint had always asked *which* gnubg routine was being wrapped,
never whether someone had already wrapped it.

### The bear-off episode (`bba46e2` → `39783b0`, `f9efed3`)

The audit had flagged `unplayableDiceFor` — which greys out dice that
can't be played — for review as possible reinvention. On 07-09, commit
`bba46e2` deleted its bear-off branch with a confident argument: gnubg
stores every sub-move as `dest = src − die`, "nothing is clamped", so
the old code's probing was guesswork. The commit message called the
deleted code "Kotlin reimplementing gnubg move semantics: the thing
CLAUDE.md exists to prevent."

The premise was false. `GenerateMovesSub` does write `src − die`, but
`SaveMoves` then clamps every negative destination to −1. A bear-off
records no die at all. The deleted code had been correct, and its
comment said so. Without it, a legal six could not bear off.

Things then got worse before they got better. To defend the false
premise, three sub-move paths were rewritten around a "played prefix"
state machine with sub-multiset matching — a scheme gnubg doesn't have.
The real defect was a single function, `landingPointsForSource`, which
pooled sub-moves from unrelated legal moves into one graph and walked
paths through it that gnubg never offered.

`39783b0` reverted all of it, restored the deleted code with its reason
on record, and fixed only the one function. `f9efed3` added three rules
that are about not over-applying the main one:

- **Q-1 — why does the existing code do that?** Deleting working code is
  a change and needs the same evidence. "This looks like invention" is a
  hypothesis, not a finding.
- **Q-2 — find every writer before concluding an encoding.**
  `grep -rn "anMove\[" engine-core/` takes one second and shows both.
- **Q-3 — fix the function the evidence implicates, and nothing else.**

> **Lesson.** The anti-reinvention rule can be used as a weapon against
> correct code. An agent that has learned "reinvention is the crime"
> will find reinvention everywhere, including where there is none.
> The project noticed this itself and added the counterweights. Rules
> that look absolute in isolation (Part I) sit next to rules that limit
> how they are applied (Q-1 to Q-3).

### Two more corrections of the same kind

- **"It was right, I was wrong"** (`f883960`). The agent had dismissed the
  tutor's single-game format as scaffolding with "no engine reason". In
  fact a single game is the only coherent cubeless format: a cubeless
  7-point match is just seven games with a scoreboard, and gnubg's own
  `gnubg_can_double` already makes the cube dead at one point. The
  mistake was asking "can the engine do it?" instead of "does it mean
  anything?" *More general is not more correct.*
- **A rule in `CLAUDE.md` that was itself an assertion** (`514662f`).
  `CLAUDE.md` said `Modifier.offset` moves the drawing but not the tap
  target. An earlier commit had called that "wrong to assert". Neither
  had been tested. The truth was narrower: offset is harmless inside the
  parent's bounds, and content pushed beyond them stops getting touches.
  The rule was rewritten to match what was measured, and a claim in
  `STATUS.md` made "from the rule rather than from the code" was
  retracted.

### UI geometry becomes a cornerstone (`3ac5a71`, 07-10)

The board stretches to fill the screen, so its x and y scales differ.
The measured ratio sx/sy runs from 0.959 at 16:11 to 1.468 on the 20:9
test device and **crosses 1.0 inside the normal phone range**, so a
drawing-versus-tap error changes sign from one shipping device to the
next. `Board.kt` had five such errors, each invisible at one aspect
ratio and wrong at another: the cube drawn as a pixel square but
hit-tested as a unit square; Undo computed twice with different
formulas, leaving its left half dead; the Commit tap band extending over
the bear-off tray. The invariant that came out of this: **an element's
tap rectangle is the rectangle it was drawn from — the same value,
computed once.**

---

## Part IV — The day of contracts and the first real release (July 11)

On 07-11 seven versions were tagged (0.10.0 through 0.11.4), 0.11.0
shipped as "the first real release", and `CLAUDE.md` received eight
commits. Each rule traces to something that happened that day.

| Incident | Rule |
|---|---|
| Mid-release, the agent told the maintainer to `git add engine-core/lib/SFMT-neon.h`. The file never existed; it had been inferred from an `#include` in an `#elif` branch. Two build cycles were lost. (`b943b55`) | **Never assert a file, path, symbol or fact without reading it first.** |
| Hours later, a new `CoachScreen.kt` used three members (`BoardPalettes.forName`, `settings.paletteName`, `pal.uiBackground`) on types whose *existence* had been checked. None of the members existed. One reached the maintainer's build. (`03ea63a`, `1c73dd6`) | **Verifying a type doesn't verify its members.** Copy from working call sites, exactly. |
| A stuck-turn bug was investigated by narrating one theory after another (cursor corruption, filters, deadlock, `fInterrupt`), each killed by a read that should have come first. (`97bab46`) | **Never speculate — exhaust the resources.** |
| "Slow or stuck?" was argued from screenshots. (`87adf4d`, `a6a8e2e`) | Every subsystem logs a duration; a precise logcat procedure. |
| Instructions arrived as fragments the maintainer had to assemble. (`03209e8`) | **Full commands only.** |
| Earlier orders had been satisfied by their wording while their purpose was defeated: a diagnostic "fingerprint" that summed to a constant and measured nothing; a parallel dice path built beside gnubg's. (`8286d60`) | **The spirit binds, not the letter.** |

About "never speculate": ChatGPT's review says engineering needs
hypotheses, and it's right. The rule in `CLAUDE.md` doesn't actually
forbid them. It forbids *voicing* them before the cheap checks are done,
and it requires every hypothesis to come with the check that would
confirm or kill it. The wording is absolute because the failure was
absolute: theory, shrug, next theory, while the maintainer waited
between rounds. Under the current rule the agent can still form
hypotheses; it just has to check them before reporting them.

---

## Part V — One state, one writer (July 12)

0.20.0, "Train with the Coach", was staged and then **withdrawn before
tagging** (`a0ee586`). Field testing had shown a whole class of
defects: a stale verdict left on screen, a "GNU is replying" frame that
never cleared, a pass path that behaved differently from the continue
path, and a race where a background dice watcher overwrote a phase the
engine thread had just set, freezing the board.

All of these had one root cause. The UI kept its own copy of the
engine's state and re-synchronised it by hand, from several handlers and
two threads. The order `79216f6` stated it directly: **gnubg is the sole
authority for match state, exactly as for match logic.** The visible
state is a *projection* read from gnubg by exactly one writer. Genuinely
app-side state (the coach hold, dice order, the undo snapshot) is a thin
named overlay on top of it.

The refactor (`994dfe0`) is worth reading. The projection already
existed and had been re-implemented badly by each new path. After the
refactor every gameplay action goes through one `settleFromEngine`.
"No caller picks a phase; gnubg IS the phase." The invariants became
**grep-enforceable**: `ENGINE_THINKING` is written only by
`beginEngineWork`, the dice watcher writes only its own leaf flow, and
so on. The frozen-board race became "dead by construction".

The same day, the Kotlin compiler became a gate. `syntax_check.sh` only
covers C, and twice it had passed Kotlin breakage.

> **Lesson.** This is where the project first did what a good
> specification should do: it turned a rule into a structure the code
> can't violate without being visibly wrong. The later milestones —
> one picture, single-sourced worker counts, derived Makefiles, the
> fidelity gate — repeat the same move.

---

## Part VI — Shipping in public: F-Droid and reproducibility (July 13–22)

The F-Droid submission (0.21.1, 07-13) started a separate apprenticeship
in build engineering. The buildserver had its own paths and offline
constraints, and F-Droid's reproducible-build check compares its own
rebuild of the APK with the published one byte for byte. Over nine days
the fixes were: `-ffile-prefix-map` for build paths; a deterministic
build-id; the NDK pinned to F-Droid's version; a CMake build directory
wiped before every configure; and finally (0.22.6) a build stamp taken
from the commit time instead of the wall clock. The changelog records
0.22.6 as the first release verified reproducible on F-Droid's own
hardware.

It didn't stay solved; see Part X.

---

## Part VII — Two editions and back (July 17 – August 15)

On 07-17 the project split into a free FOSS edition and a "Plus" edition.
The first cut withdrew the whole Analyse screen (`58abe64`). It was
reverted the same day (`82f895a`) because the intended boundary covered
only the board editor (`7001235`). Over-cutting is the same failure as
the bear-off episode, applied to product scope.

Running two codebases that were 90% identical created a new category of
failure. An overlay manifest, a parity checker and a sync script were
built, and on some days the history shows dozens of
`Merge remote-tracking branch 'origin/main' into pluswork` commits. Most
of the **process laws** in the standing-orders ledger come from this
period:

- *Pipelines mask exit codes.* `guard.sh | tail` returns `tail`'s
  status; an `&&` chain went straight past a red parity alarm and
  pushed. The ledger's first amendment came "minutes after its birth"
  (`d1b3d75`).
- *Patch-apply is not merge ancestry.*
- *Verification greps must prove absence, not just presence.* A merge
  shipped raw conflict markers while its verification only grepped for
  the new function names.
- *Multi-repo operations run as separate small calls.* A long chain
  resumed in the wrong directory after a `;`.

On 08-11 came **the master rule**: the FOSS edition is the complete,
first-class app, never a staging area. On 08-15, by maintainer ruling,
the editions merged back into one: **CBG Pro 1.0.0**, one application
id, one key, public, GPL3 (`7b6e3dc`). The edition machinery was
retired, and with it the whole category of failure it had produced.

> **Lesson.** Some of the rules in `CLAUDE.md` guard against a structure
> that no longer exists. That's history, not law, and it's the strongest
> argument for moving such material out of the rules file and into a
> document like this one.

---

## Part VIII — Fidelity in the numbers: rollouts (August 1–15)

Rollouts needed code that gnubg's desktop build doesn't have: a worker
pool, scheduling, merging results. On 08-01 the maintainer extended the
main rule to cover that code: **"This is a gnubg port, not some
improvement… we deliver what we promise."** Where the port has to own
the plumbing, every *number-producing* line is gnubg's, ported verbatim
with the source line cited. The pool's aggregation is `rollout.c`'s
Welford recursion line for line, including the trial-index order.

The reason verbatim mattered more than "mathematically equivalent"
showed up two weeks later. **Gate B**, a side-by-side comparison with
desktop gnubg using the same position, seed and trial count, found
three divergences, each fixed verbatim from desktop 1.07.001
(`94445e1`): the Mersenne generator wasn't re-seeded per trial; the
rollout evaluation contexts were **zeroed** (cubeless, no pruning) where
desktop's are cubeful and pruned; and the basis cube was passed as 0
instead of the position's cube. None of the three would have been found
by reading.

### The one-core green (September 10, `46bb124`, `aa427c9`)

The rollout harness's determinism test (same seed twice, byte-identical
results) had been reported green from the agent's one-core sandbox for
an upstream sync. On the maintainer's twelve-core machine it **failed on
every run**, including on the tree from before the sync. The cause,
read in the source: the trial core uses gnubg's *NoLocking* evaluation
family, which gnubg only ever runs single-threaded. With N workers, the
shared evaluation cache is a data race by construction — rare per
lookup, certain per run at twelve threads. The sandbox and a four-thread
phone had passed it by luck.

The pool now runs one worker. The shipped worker count has a single
source. The harness **refuses** to run on a host with fewer cores than
that count, and the release preflight prints the host it ran on. That is
**the determinism gate**: a test counts as a gate only where the risk
actually is.

---

## Part IX — The artifact, not the story (August 12–15)

Two incidents in four days showed that verification had to be pointed at
the thing that ships, not at the thing being worked on.

**Verify the commit, not the worktree** (`b4e90d1`). A history rewrite
cut a commit from an index that still held an old file, while the
Kotlin gate ran green on the edited worktree. Two claims that a display
was "structurally impossible" were then made to the maintainer about
code the repository didn't actually contain.

**The single-source law** (`56eb2d7`). The rollout harness's source list
had been typed out by hand through a grep that missed one line
(`android-app.c` is spelled differently from its siblings). That
produced a phantom cascade of "missing" symbols and a linker hack
iterated on top of the phantom. Then a confident story was told about
why the harness Makefile was missing from history, and the evidence
didn't support it. The maintainer's order is quoted verbatim in the
commit: *"This may never ever happen again… make this a law never to be
broken again."* The four clauses: authoritative definitions are read
whole or derived mechanically; verification must be independent of the
process it checks; confidence matches evidence ("I cannot prove how
this happened" is a complete answer); and the maintainer's machine is
the reference.

The third clause was put to work immediately and produced the real
answer: a root `.gitignore` rule meant for the vendored autotools tree
had silently excluded the harness Makefile from every `git add -A` since
08-02. The Makefile now derives its sources from `CMakeLists.txt` at
build time and can't drift again.

**1.0.0 → 1.0.1** (`6d9f2a9`, `5cfcd50`). Review Match showed pip counts
of 167/167 in the middle of a game, because `BoardState` *defaulted*
both counts to 167 and two screens never passed real values. A plausible
number that nothing computed is exactly what the "correct or silent"
law forbids. The default became "not computed, draw nothing". 1.0.0 was
never sent to F-Droid, so the catalog wouldn't carry a known blemish
under a description promising that every number on screen is true. The
F-Droid release was re-cut as 1.0.1.

---

## Part X — Upstream (September onward)

### Where gnubg actually lives (`b73e374`)

`git.savannah.gnu.org/git/gnubg.git` is the only source of truth for
upstream. Not a GitLab mirror (which `PROVENANCE.md` had named), not a
distro tarball, and not the bug-gnubg mailing list. The list is a
courtesy channel, not a review queue: the people with commit access push
directly, and a patch posted there with no reply has usually already
been merged. *Never infer upstream state from list traffic; read the
repository.*

### The monthly check and the first sync (`ccfe40a`, 09-10)

On the first of every month, upstream is compared against the pinned
vendoring base. Every upstream change is carried in. The version only
bumps when a change reaches code that is compiled into the engine *and*
reachable from the port, and that has to be shown per commit, not judged
from the subject line. The first run carried 31 upstream commits and
stated per commit whether it reached the port, with evidence: for
example the move-comparison tie order (via `FindnSaveBestMoves`) and
cube info at cube ≥ 128 (via `GetMatchStateCubeInfo`) did; a mutex
compiled out by `USE_MULTITHREAD` did not. The sync shipped as 1.0.2,
and the announcement to bug-gnubg, listing the hashes and the F-Droid
version, was written into `docs/upstream/` (`5e9fa19`).

### Two directions, two rules

ChatGPT's review read "every upstream change is carried" and "engine
patches wait for upstream acceptance" as contradicting each other. They
govern opposite directions. **Inbound**: everything from Savannah comes
in. **Outbound** (the upstream-first ruling, 10-26, `6202857`): any
change to how vendored gnubg *behaves* goes to the mailing list first
and is built only after upstream accepts it; "rejected means not built."
What the port may do freely is UI over gnubg's own verbs, and
self-contained additions compiled beside an unmodified gnubg.

The **match clock** is the example of both. In August the Kotlin clock
state machine was condemned rather than repaired. Its arithmetic moved
into a self-contained C module, `engine-core/timecontrol.c`, written to
upstream standards and harness-tested, because gnubg's own old manual
documents a "Time controls" feature that was never finished (`2c5acd7`,
`459e9a8`). Under the upstream-first ruling the module counts as an
addition, called only from JNI. The hooks it would need in `play.c` and
`set.c` remain an unapplied submission, `docs/upstream/0001-time-controls.diff`.

### Reproducibility, again (`09ce491`, 09-11)

F-Droid's rebuild of 1.0.2 differed from the published APK in every
native library. The diff was read byte by byte and four causes were
found. `release.sh` had been running *its own* CMake configure (Debug
build type, stale glib); CMake versions disagreed on toolchain flags;
meson left a build-time RUNPATH in the libraries; and the commit stamp
varied in length between a fresh clone and a large one. The fix
established **one build path**: the release runs exactly the recipe's
build, checks the unsigned APK's hash against a two-worktree proof, and
signs only those bytes. The **F-Droid build check** that followed
replaced a proposed local container rebuild with a short table of
version pins to compare.

---

## Part XI — The screen is one picture (September 10)

On one day a new 2772×1272 device report produced four layout fixes in
a row: a spacer nudged here, a width tuned there. Each fix moved the
overflow somewhere else. The fifth commit replaced the patches with an
abstraction (`4ad7b46`, `ca3bc21`). Every screen is designed once, for
the reference device (Pixel 8 Pro, 997×448 dp landscape). On smaller
panes the *whole screen* scales by one factor through the density its
children read, so layout, drawing and tap-testing share it, and the tap
rectangle equals the drawn rectangle by construction. Screens are
partitions of weighted cells that can't overlap. A new device report
becomes a row in `tools/geometry_sweep.sh`, not a debugging session.

This is the July geometry invariant (Part III) turned from a rule into
a mechanism.

---

## Part XII — The fidelity gate: "port" becomes measurable (September 26)

Until 09-26, the claim that this app *is* gnubg rested entirely on
**reading**: call the right function, use the named instance, cite the
line. The determinism gate proved the port agreed with itself, and Gate
B had covered rollouts only. Nothing had ever put a position through
this engine *and* through an independent build of gnubg and compared
the results.

The first such measurement (`tools/book_conformance`, 30 opening
positions against the BackgammonDB opening book) found 2-ply cubeful
equities off by up to 0.003 on one play in five. The same evening the
cause was pinned down (`5937752`). Upstream gnubg was built from source
at the vendoring base, and the difference was reproduced by a single
flag: under `-ffast-math`, gnubg's SIMD sigmoid uses the CPU's
**approximate** reciprocal (`lib/neuralnetsse.c:204-226`). Upstream's
configure adds that flag unconditionally for gcc; this port's build
never had it. With fast-math off, results were identical across scalar,
SSE2, AVX, FMA3, gcc and clang.

**The ruling** (`f615b66`): the source at Savannah *is* gnubg. A
distribution's binary is one compiler's rendering of it, carrying an
approximation upstream chose for speed and never checked across
architectures. The port ships the exact-arithmetic branch and says so,
in `PROVENANCE.md` and on the About screen.

**The gate**: a reference built from upstream's source by *upstream's
own* build system (independent of the port, per the single-source law);
a random sample drawn fresh from a printed seed across all position
classes, plies, and cubeful and cubeless evaluation; a permanent row for
every position that has ever failed; an epsilon set by measurement and
never widened to make a run pass; and no release without agreement.

**Status, honestly.** As of 1.0.4 the gate is specified but not yet
built. `tools/book_conformance`, the opening-book harness that found
the divergence, exists. `tools/fidelity_gate` doesn't. 1.0.4 shipped
without it under an explicit maintainer ruling for that release, and
the HANDOVER queue lists it as a blocker for 2.0. The rule has gotten
ahead of its mechanism, and the record says so instead of pretending
otherwise.

> **Lesson.** This is where the arc that began on 2026-06-29 is heading.
> First the rule was a principle; then a checkpoint (questions in
> writing); then a set of structures (one writer, one picture,
> single-sourced values); finally a measurement. Each stage depended on
> the one before it. You can't measure fidelity to gnubg if the code
> was never routed through gnubg in the first place.

---

## Part XIII — What the journey taught

### The agent's failure modes, as this project met them

| Failure mode | First recorded instance | Countermeasure |
|---|---|---|
| Re-implementing what the engine does | Moves and pips in Kotlin, day two (`e463bcf`) | The one rule; port checkpoint |
| Correct call, invented parameters | Hand-rolled cube evalcontext (`776bc4a`) | Q2: use the named instance |
| Re-describing what the engine means | Five feature-analysis files (`d07187d`) | Q4; new-file tripwire |
| Re-building what already exists | Duplicate `command_agree` (`422d6e0`) | Q0 with pasted greps; host syntax check |
| Over-applying the rule to correct code | Bear-off deletion (`bba46e2`) | Q-1, Q-2, Q-3 |
| Inferring facts from references | `SFMT-neon.h` (`b943b55`) | Read before asserting |
| Inferring members from convention | `uiBackground` (`03ea63a`) | Member-level reads; compile gate |
| Narrating guesses | Stuck-turn theories (`97bab46`) | Hypotheses paired with checks |
| Keeping a shadow copy of engine state | The frozen coach board (`79216f6`) | One state, one writer |
| Complying with the letter, not the intent | The constant fingerprint (`8286d60`) | The spirit binds |
| Verifying the wrong object | Worktree vs. commit (`b4e90d1`) | Read back from the commit |
| Self-confirming checks | Grep diffed against the same grep (`56eb2d7`) | Independent verification |
| A gate run where the risk isn't | One-core determinism run (`aa427c9`) | Gate at the shipped thread count |
| Plausible numbers nothing computed | Pips 167/167 (`6d9f2a9`) | Correct or silent |

Almost every row is the same thing in a different place: **a plausible
inference standing in for a checked fact.** The rules look numerous
because the failure shows up in many places: the engine, the
filesystem, the compiler, the build, the state, the numbers, the
agent's own reports.

### How the rules evolved

1. **Incident → rule.** Nearly every section of `CLAUDE.md` names the
   incident that produced it. That is why it reads like a record of past
   corrections: it is one.
2. **Rule → class.** "The spirit binds" exists because each new rule was
   at first satisfied only for the incident that triggered it.
3. **Rule → counter-rule.** Q-1 to Q-3, the retracted offset rule and the
   tutor correction show that the project limited its own absolutism
   when that absolutism caused damage.
4. **Rule → structure.** The most effective rules are the ones nobody
   needs to remember any more: one writer (enforceable by grep), one
   picture, single-sourced worker counts, derived source lists, one
   build path.
5. **Structure → measurement.** The fidelity gate checks the outcome
   directly, whatever path the code took to get there.

### What the result says

The app is stable and fast, and F-Droid's own hardware rebuilds it byte
for byte. Its engine *is* gnubg's source, and the one measured
divergence from a desktop build was traced to a single compiler flag
upstream adds, not to the port. The issues users file are mostly about
presentation and features (themes, move-list paging, drawing XGIDs the
way XG does, showing each roll in Review), not about wrong backgammon.
The rules didn't produce this by being long. They produced it because
each one held a correction in place long enough for something better —
a structure or a measurement — to replace it. The last of those, the
fidelity gate, is the remaining piece.

---

## Part XIV — Implications for `CLAUDE.md`

Writing this document required reading `CLAUDE.md` against the history,
and that turned up material that is stale or contradictory. Both are
real hazards for an agent, which has to pick one instruction when two
conflict:

- **Logcat.** Lines 203–221 prescribe `adb logcat -v time -s tag:I` and
  forbid a bare `logcat | grep`; lines 586–591 prescribe exactly
  `logcat | grep` and forbid `-s`. The section higher up in the file is
  the newer one (07-11/12, from field lessons). The lower one dates from
  the file's first commit on 06-29 and was never removed.
- **THE SCREEN IS ONE PICTURE** appears twice (lines 907 and 939, from
  `c119328` and `47ed3d6`), the second reflecting the move of OnePicture
  to the app root.
- **CURRENT STATE** still says "Version: 0.9.1 consolidation" and names
  `docs/STATUS.md`, `MASTER_V0.9.md` and `PHASE3_TUTOR_ANALYSIS.md`,
  which now live in `docs/history/`. It says there are four strength
  presets and "NO Expert/Master in gnubg"; the app exposes World class
  and Grandmaster (`GameSettings.kt:26-27`; `NUM_SETTINGS` is 9 in
  `eval.h`).
- **KNOWN-INCOMPLETE** says Save Match and Review Match are not built.
  Twenty lines earlier, CURRENT STATE says Review Match is built, and
  both have shipped since 0.11.0.
- The member-level read rule says "until a Kotlin compile exists in my
  environment"; the Kotlin compile gate has existed since 07-12.
- THE MONTHLY UPSTREAM CHECK says "Next check: 2026-10-01".
- THE FIDELITY GATE says "No agreement, no release", but the tool it
  describes doesn't exist yet, and 1.0.4 shipped under a per-release
  ruling. A rule whose mechanism is missing should say so in its own
  text, so an agent doesn't read it as already in force or quietly
  ignore it.
- The Plus-era process laws describe a two-repository structure that was
  retired on 08-15.

A possible split, consistent with this document:

- **`CLAUDE.md`** keeps the rules that are still in force, short and
  imperative, each with a one-line reason and a pointer to the part of
  this document that explains it. The hard edges stay: they are why the
  app works.
- **`docs/JOURNEY.md`** (this file) holds the incidents, the reasoning,
  and the superseded structures.
- Environment and tooling specifics (paths, logcat procedure, build
  flags) go in one place, once.

---

## Appendix — Origin of every `CLAUDE.md` change

| Date | Commit | Change | Triggered by |
|---|---|---|---|
| 06-29 | `8333466` | File tracked (146 lines) | Replaces auto-generated template |
| 06-29 | `8410995` | ASCII in code, UTF-8 in docs | Typography residue in sources |
| 06-29 | `776bc4a` | PORT CHECKPOINT Q1–Q3 | Hand-rolled cube evalcontext, fixed by a second one |
| 07-03 | `e353ec0` | Analysis is gnubg's; board-read line; Q4; new-file tripwire | Five invented feature-analysis files |
| 07-09 | `fad3aeb` | Ported globals are engine code; state corrected | EVALSETUP_2PLY nTrials = 0; stale STATUS.md |
| 07-09 | `422d6e0` | Q0; syntax check | Duplicate `command_agree` |
| 07-10 | `f9efed3` | Q-1, Q-2, Q-3 | Bear-off deletion and invented prefix machinery |
| 07-10 | `3ac5a71` | UI geometry: one rectangle | Five draw/tap divergences in Board.kt |
| 07-10 | `9162a79` | Hub order; Review Match built | Feature completion |
| 07-10 | `514662f` | Offset rule corrected | Untested assertions in both directions |
| 07-10 | `dc80876` | Gear top-left; nothing scrolls | A scrolling editor pane |
| 07-11 | `b943b55` | Never assert unread files/symbols | `SFMT-neon.h` |
| 07-11 | `9519e9d` | Threading routed via THREADING.md | Repeated "use all cores" questions |
| 07-11 | `03ea63a` | Read every member new code touches | `uiBackground` |
| 07-11 | `87adf4d` | Logcat tags; duration logs | Slow vs. stuck |
| 07-11 | `97bab46` | Never speculate | Serial theories on the stuck turn |
| 07-11 | `a6a8e2e` | Clear-then-stream logcat | Stale buffer dumps |
| 07-11 | `03209e8` | Full commands only | Fragmentary instructions |
| 07-11 | `8286d60` | The spirit binds | Letter-compliance record |
| 07-12 | `79216f6` | One state, one writer | Coach state-sync defects; 0.20.0 withdrawn |
| 07-12 | `994dfe0` | Kotlin compile gate | syntax_check passed Kotlin breakage twice |
| 08-09 | `a9c5f63` | Standing-orders ledger | Rulings that lived only in session memory |
| 08-09 | `ef0824b` | Plus-realm addendum | Edition split |
| 08-09 | `d1b3d75` | Pipelines mask exit codes | Push past a red parity alarm |
| 08-11 | `2c5acd7` | Upstream clock ruling | Kotlin clock state machine |
| 08-12 | `b4e90d1` | Master rule; commit-not-worktree | Plus box for a FOSS feature; worktree-only verification |
| 08-14 | `56eb2d7` | Single-source law | Hand-mirrored source list; unsupported narrative |
| 08-15 | `7b6e3dc` | The consolidation | Editions merged into CBG Pro 1.0.0 |
| 09-10 | `c119328`, `47ed3d6` | The screen is one picture (twice) | Four band-aids on one device |
| 09-10 | `b73e374` | Savannah; monthly check; sync is announced | Upstream policy, 09-06 |
| 09-10 | `aa427c9` | Determinism gate | One-core green vs. twelve-core red |
| 09-11 | `f820dec` | F-Droid build check | Proposed local container rebuild |
| 09-11 | `09ce491` | One build path | F-Droid rebuild of 1.0.2 differed |
| 09-26 | `1b40a1d` | Fidelity gate | First measurement against an independent reference |
| 09-26 | `5937752` | Epsilon by measurement | fast-math SIMD sigmoid identified |
| 09-26 | `f615b66` | The ruling: source is gnubg | Packages vs. source |
| 10-26 | `6202857` | Upstream-first ruling | Email request for a "never resign" option |
