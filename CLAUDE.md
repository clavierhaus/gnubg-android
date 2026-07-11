# CLAUDE.md -- gnubg-android

READ THIS FULLY BEFORE ANY ACTION. These rules override your defaults. When a
rule here conflicts with what seems easier or more idiomatic, the rule wins.

## THE ONE RULE THAT MATTERS MOST

**gnubg is the SOLE authority for all game logic AND all analysis. PORT it,
NEVER reinvent it.**

This is a true port of GNU Backgammon's engine to Android. The vendored C
engine in engine-core/ already implements every rule, move, cube decision,
score, and analysis. Your job is to expose and wire that existing logic --
never to re-implement backgammon semantics in Kotlin or in new C.

"Game logic" is NOT the only forbidden territory. gnubg also computes, inside
its own evaluation, everything about what a position or move MEANS: shot
counts, primes, blots, anchors, board strength, position class, race vs
contact, blunder severity, which features matter. ALL of that is gnubg's, not
yours. The crime is not limited to deciding a rule -- it includes describing,
classifying, scoring, ranking, characterizing, or assigning quality/meaning to
any position or move. If gnubg computes it internally and you recompute it in
Kotlin, that is reinvention, FULL STOP, even when no rule of backgammon is
being decided.

Before writing ANY logic that decides legality, generates moves, evaluates a
position, scores a move, makes a cube decision, computes pips, progresses
match state, OR describes/classifies/ranks/characterizes a position or move:
STOP. Find the gnubg function that already does it and call it.
If you are about to write a loop that computes something about the board,
you are almost certainly reinventing -- search engine-core/ first.

Concretely, you MUST route through gnubg's own routines, e.g.:
- legal moves / sub-moves: GenerateMoves (via gnubg_mobile_get_legal_moves)
- move scoring / best move: FindnSaveBestMoves
- applying a move: ApplyMove / ApplySubMove
- position identity: PositionKey / PositionID
- matchstate replay: FixMatchState + ApplyMoveRecord (as AnalyzeGame does)
- cube decisions: the cube path in the engine, never a hand-rolled rule
- eval contexts / strength: aecSettings presets, esEvalChequer

If you cannot find the gnubg function, ASK -- do not invent one.

Kotlin may ONLY: draw the board, compute hitboxes in board-relative
coordinates, hold transient UI state (displayed dice order, uncommitted
submove snapshots for undo), and call JNI. Kotlin must NOT invent game
legality, cube legality, scoring, pip counts, match progression, OR any
analysis, description, classification, or quality judgement of a position or
move.

## THE BRIGHT LINE (no interpretation required)

If you write a function that reads the board array (board[...]) and returns
anything other than a pixel/hitbox coordinate or a value gnubg literally handed
you, STOP -- that is reinvention. Reading board[n] to compute a fact ABOUT the
position (a count, a shot, a prime, a blot, an anchor, a strength, a class, a
label, a "notable" difference) is gnubg's job, even if no rule is being
decided. The board is gnubg's to interpret. Kotlin loops over board[] exist for
exactly one purpose: drawing checkers and hit-testing taps. Any other loop over
board[] is a bug.

"Engine-free", "pure", "deterministic", "no engine calls", "unit-testable in
isolation" are NOT virtues for analysis code -- they are the SIGNATURE OF A
VIOLATION. Analysis is gnubg's job; "engine-free analysis" is a contradiction.
If you catch yourself writing or justifying a file that describes a position
without calling the engine, you are reinventing. The correct move is to call
gnubg, or to not compute the value at all.

## THREADING QUESTIONS GO THROUGH docs/THREADING.md

Any request to "thread the engine", "use all the cores", or "make the move
faster with threads" is answered by re-verifying docs/THREADING.md against the
source, not from first principles. Short version, to be re-checked not recalled:
the app is single-threaded (USE_MULTITHREAD undefined), a single move is one
serial pruned NEON-vectorised search that gnubg does NOT decompose into tasks, so
threading a live move buys nothing and adds concurrency risk. Threading is the
right tool ONLY for analysis and rollouts, which gnubg already decomposes, and
only with the safeguards the doc lists. Re-litigate only from a new measurement.

## NEVER SPECULATE -- EXHAUST THE RESOURCES (added 2026-07-11, maintainer order)

Speculation is never an option. When behavior is not understood, the resources
are, in order, and ALL of them before any hypothesis is voiced:

  1. The COMPLETE vendored engine source. Read the entire call chain end to
     end -- CommandX through every function it reaches -- not sampled lines
     around a grep hit. A chain is not read until every early return, every
     global it touches, and every loop exit condition has been seen.
  2. The port's own code, same standard, including MY OWN recent verbs: a
     verb written this session is the least trusted code in the tree, and its
     lock/return balance is verified by reading, not by recalling that I
     meant it to be balanced.
  3. The repository history: git log/diff between the last working commit and
     the first broken one bounds the search space to the actual change.
  4. Prior-session transcripts (the journal) for decisions and known traps.
  5. Instrumentation + the device, via the logcat discipline above: when the
     static resources cannot distinguish remaining candidates, add targeted
     logs and obtain the measurement. Requesting a measurement is not
     speculation; narrating guesses is.
  6. The maintainer, for device-side facts I cannot obtain.

A hypothesis may be stated only as an ordered list where every item is paired
with the concrete check that confirms or kills it, and the checks are then
EXECUTED. Serial guess-narration -- theory, shrug, next theory -- is the
failure mode this order exists to end.

## LOGCAT: PRECISE TAG FILTERING, ALWAYS (added 2026-07-11)

When requesting or reading device logs, filter by the app's own tags with -s --
never a bare `logcat | grep`. The canonical command:

    adb logcat -d -v time -s gnubg-vm:I gnubg-tutor:I gnubg-cube:I gnubg-coach:I

Tags in use: `gnubg-vm` (game flow: confirm/roll/cube), `gnubg-tutor` (tutor
analysis), `gnubg-cube` (cube evaluations), `gnubg-coach` (coach verdict, WITH
timing). Every new subsystem gets its own tag and a duration log for anything
that can take >200ms, so "slow" vs "stuck" is a number in the log, not a
diagnosis argued from a screenshot.

## NEVER ASSERT A FILE, PATH, SYMBOL, OR FACT WITHOUT READING IT FIRST

This rule exists because it was broken. On 2026-07-11, mid-release, I told the
maintainer to `git add engine-core/lib/SFMT-neon.h` and build around it. That file
does not exist and never did. I had read `#elif defined(HAVE_NEON) #include
"SFMT-neon.h"` in SFMT.c, concluded the file must be present and tracked, and
asserted it as fact -- without once running `ls` on it. The maintainer lost two
build-and-deploy cycles to `pathspec did not match` and `No such file` errors
chasing a file I invented. An `#include` in a conditional branch is a CLAIM the
file is reachable, not proof it exists on disk; the NDK can satisfy NEON without
that header.

The failure mode: reading a reference TO a thing and reporting the thing as
verified. It is the same mistake as asserting gnubg behaviour without reading the
function, only about the filesystem instead of the engine. Both substitute a
plausible inference for a checked fact.

The rule, absolute:

  - Before naming a file as existing -- in an instruction, a command, a commit,
    a doc -- I have run `ls` / `view` / `git ls-files` on that exact path in this
    session. An `#include`, a build reference, or a memory of it is NOT
    confirmation.
  - Before telling the maintainer to run a command against a path, that path is
    confirmed to exist (or the command is explicitly the thing that creates it).
  - Before asserting a symbol, function, struct, line number, or config value, I
    have read it in the source this session -- not recalled it, not inferred it
    from a caller.
  - If I have not checked, I do not assert. I say I need to check, then check.
    "I believe" and "should be" are not permitted to stand in for a fact the
    maintainer will act on; either I verify it or I flag it explicitly as
    unverified and do not build instructions on it.
  - When I discover I asserted something unchecked, I say so plainly, name what
    was wrong, and correct it -- in the commit and to the maintainer. No quiet
    walk-back.

### The rule extends to every symbol NEW CODE touches (added 2026-07-11)

Broken again the same day it was written, in a subtler form. Writing
CoachScreen.kt -- a new file -- I dereferenced `BoardPalettes.forName`,
`settings.paletteName`, and `pal.uiBackground`. None of the three exists. I had
verified that the TYPES existed (`BoardPalettes`, `GameSettings`,
`BoardPalette`) and then wrote their members from expectation. Two were caught
before commit; `uiBackground` reached the maintainer's build and failed it.
Asserting a fact to the compiler is still asserting a fact: an unresolved
reference costs the maintainer a build-and-deploy cycle exactly like a
nonexistent file does.

The extension, absolute:

  - Verifying a type exists does NOT verify its members. Before new code
    dereferences a field, calls a method, or names an enum constant, I have
    read that member's declaration in the source THIS session. Plausible names
    (`uiBackground`, `forName`) are the signature of this failure mode --
    convention is not confirmation.
  - Before committing a NEW file, I re-read it symbol by symbol and, for every
    external member it touches, point to the declaration (file:line) I read.
    The pre-commit check for new Kotlin is exactly the C discipline: grep the
    declaration, not the vibe.
  - The strongest form of compliance is to copy the usage from a working call
    site (as ReviewScreen was the template for the screen structure) -- and
    where I copy, I copy exactly, not approximately from memory of it.
  - Brace/paren balance checks and syntax_check.sh do not cover Kotlin
    references; until a Kotlin compile exists in my environment, member-level
    read-verification is the ONLY defence, so it is mandatory, not best-effort.

A fabricated fact costs more than a slow answer: it sends the maintainer to
execute against a reality that is not there. Checking is cheap. Guessing is not.

## THE PORT CHECKPOINT (per-commit, no exceptions)

The rule above is the rule. This is the operational test that catches
violations. Apply it before writing ANY C or Kotlin that touches game
logic, cube logic, evaluation, or analysis. Apply it before suggesting
ANY paste-once that does the same.

For every change, answer these IN WRITING, in chat, before code:

  Q1. What gnubg function, struct, enum, or named instance does the
      equivalent thing? Name it -- file path and approximate line
      number. If I cannot name one, I STOP and search engine-core/
      until I find one or confirm there is none.

  Q2. Am I about to construct a value (evalcontext, evalsetup,
      cubeinfo, movefilter, or any parameter passed to a gnubg
      routine) that gnubg already has a named instance of --
      ap[].esCube, ap[].esChequer, esEvalCube, esEvalChequer,
      esAnalysisCube, esAnalysisChequer, aecSettings[],
      EVALSETUP_2PLY, MOVEFILTER_NORMAL, GetEvalCube(),
      GetEvalChequer(), GetMatchStateCubeInfo(), and so on?

      If yes -- USE the named instance. A "private evalcontext /
      cubeinfo / movefilter / evalsetup" in the facade is, BY
      DEFAULT, a BUG. The facade exposes gnubg; it does not
      reparameterise gnubg with values invented by the porter.

  Q3. If I am extending or calling an existing facade verb, does
      THAT verb itself obey Q2, or did a prior hand inject a private
      struct? If the verb itself is a reinvention, FIX THE VERB.
      Do not layer further code on broken ground.

  Q4. Does this value get SHOWN to the user, or logged, as a fact
      about the game -- a count, a quality, a classification, a
      label, a severity, a "notable" difference, a position type,
      a best move, a shot count, anything the user reads as truth
      about the position? If yes, name the gnubg routine that
      PRODUCES it. "I computed it in Kotlin" / "I derived it from
      the board" is a FAILING answer. If gnubg does not expose it,
      the answer is to NOT SHOW IT -- not to compute it myself.

## UI GEOMETRY: ONE SOURCE, EVERY DEVICE

This is a cornerstone, not a style note. The app targets Android 12+, which is a
long tail of resolutions, densities and aspect ratios. It must look the same on
all of them, and a tap must land where the eye says it will, on every one.

**The layout law.**

- The board fills the screen horizontally. Always. Nothing is cropped sideways.
- Aspect ratio is absorbed VERTICALLY, by stretching. A non-uniform scale
  (`sx != sy`) is therefore normal and expected, not an edge case.
- Board features that stretch with the board -- points, bar, frame, trays -- are
  expressed in board units and scaled by `sx` and `sy` respectively.
- Pieces that must stay square or round -- checkers, dice, cube, buttons -- are
  drawn from a single dimension, so their pixel size does NOT follow `sy`.

**The invariant.**

    Every interactive element's hit rectangle IS the rectangle it was drawn from.
    Not a copy of it. Not a formula that agrees with it. The same value.

Compute each rectangle once, from the canvas size, in one place. Draw from it.
Hit-test against it. A hit rect derived independently is a bug the moment the
aspect ratio changes, and it will change.

Measured, for the board pane (about 82% of the width; the rest is the side
column):

    screen 16:11   sx/sy 0.959     hit rect too TALL  by 0.11 u per edge
    screen 16:10   sx/sy 1.055     hit rect too SHORT by 0.14 u per edge
    screen 16:9    sx/sy 1.172     hit rect too SHORT by 0.45 u per edge
    screen 20:9    sx/sy 1.468     hit rect too SHORT by 1.23 u per edge
                                   (the Pixel 8 Pro test device)

`sx/sy` crosses 1.000 at about 16:10.4, inside the ordinary phone band. So the
error changes SIGN across devices we ship to: on one the cube's top and bottom
are dead, on another the tap rect sticks out past the drawn cube. There is no
correction factor that fixes both. The only fix is that there is one rectangle.

**Corollaries.**

- Never write the same geometry expression twice. If a comment says "must match
  the hit-test above exactly", the design is already wrong -- a comment is not a
  mechanism.
- Never mix scales. `uy(a) + ux(b)` is meaningless: it adds an x-scaled length to
  a y coordinate, and the element slides as the pane widens.
- Never hit-test a pixel-square in unit space, or a unit-rect in pixel space. A
  unit square is not a pixel square when `sx != sy`.
- The settings gear sits TOP-LEFT on every screen. No exceptions. If it would
  cover something tappable, move the something, not the gear.
- Nothing scrolls, anywhere. The game view law applies to every pane: what does
  not fit is made to fit (smaller controls, tighter spacing, fewer words).
- Tap target equals drawn button. Do not use `Modifier.offset` to move a control
  out of the space its parent gave it: offset is a layout modifier, so drawing and
  pointer input move together, but anything pushed beyond the parent's bounds stops
  receiving events -- visible, untappable. Fix the layout that produced the dead
  space instead. Within the parent's bounds offset is harmless; `padding` is
  clearer, because it moves the slot rather than the content inside it.
- Distributive space (gaps between groups) is weighted. Intrinsic space (a gap
  between two adjacent chips) is dp. Neither is a hit rectangle.

**What this rule already cost.** Board.kt drew the cube as a pixel square and
hit-tested it as a unit square, so 16% of it was dead at the top and 16% at the
bottom. `undoLeft` was computed twice, with different formulas and different
widths, leaving the left HALF of the Undo button inert. The Commit tap band ran
past the drawn button and over the bear-off tray. The Undo/Commit tap band was
2.5x the drawn height and started above the buttons, stealing taps from the
checkers beneath. `btnY` added an x-scaled gap to a y coordinate. Every one of
these is invisible at one aspect ratio and wrong at another.

## Q-1 -- WHY DOES THE EXISTING CODE DO THAT?

Before deleting, replacing or "simplifying" code that is already here, answer why
it is the way it is. If it carries a comment explaining itself, that comment is a
claim made by someone who probably read the engine. Disprove it, in the source,
before acting -- or leave it alone.

    unplayableDiceFor said: "bear-off (dest clamped to -1)" and probed
    Engine.applySubMove to recover the die. It was right. It was deleted as
    "invention based on a false premise". The false premise belonged to the
    deleter (bba46e2, retracted in 39783b0).

Deleting working code is a change. It gets the same checkpoint as writing new
code, and the same burden of evidence. "This looks like invention" is a
hypothesis, not a finding.

## Q-2 -- FIND EVERY WRITER BEFORE CONCLUDING AN ENCODING

Never infer how a field is encoded from the first place you see it written.

    GenerateMovesSub writes  anMoves[k*2+1] = i - anRoll[k];
    SaveMoves then writes    pm->anMove[i] = anMoves[i] > -1 ? anMoves[i] : -1;

Reading only the first produced the rule "dest == src - die, always". The second
clamps every negative to -1, so -1 is a sentinel for "off" and encodes no die at
all. That single unchecked generalisation broke bear-offs, deleted correct code,
and grew a state machine to defend itself.

    grep -rn "anMove\[" engine-core/

runs in one second and shows both. Do it, every time, before stating what a field
means. An encoding claim is a search result, not a recollection.

## Q-3 -- FIX THE FUNCTION THE EVIDENCE IMPLICATES, AND NOTHING ELSE

The demonstrated defect was one function: landingPointsForSource pooled the
sub-moves of unrelated legal moves into one graph and searched it. What followed
was a rewrite of tapSource, dragMove and tryDestinationStackMove, a `played`
prefix on BoardState, a nextSubMoves() decoder, and sub-multiset matching --
none of which gnubg has, none of which any evidence asked for. All reverted.

Scope is not a matter of taste here. Every function touched beyond the implicated
one is a function whose behaviour cannot be verified by the report that prompted
the change. If a second function looks wrong, get evidence for it first, and fix
it in its own commit.

## Q0 -- DOES IT ALREADY EXIST?

Before writing any new function, verb, external or ViewModel method, search for
it. Not "check carefully": run the search and paste the result.

  - facade verb  -> grep -rn "CommandFoo" jni-bridge/    (the gnubg routine it wraps)
  - JNI symbol   -> grep -rn "Engine_fooBar" jni-bridge/src/native-lib.c
  - Kotlin       -> grep -rn "fun fooBar" gnubg-app/

This question is Q0 because it precedes the rest. The checkpoint used to begin by
asking WHICH gnubg function is being wrapped, and never asked whether someone had
already wrapped it. That omission shipped a duplicate definition of
gnubg_mobile_command_agree next to the one that had been there for months --
along with duplicate header declarations, duplicate JNI wrappers and duplicate
externals. The whole answering chain for resignations already existed and was
dead; only the reader for ms.fResigned was missing.

The same omission nearly rebuilt SetGNUbgID, and nearly rebuilt match saving.
Neither was caught by a compiler. Both were caught by a person, late.

## RUN ./tools/syntax_check.sh BEFORE EVERY COMMIT THAT TOUCHES C

There is no excuse for shipping a C file that does not compile. gcc, the glib
headers and a JDK are enough -- the only NDK-specific header the facade needs is
<android/log.h>, stubbed in tools/shim. The full NDK is needed to LINK for the
device, not to find a redefinition, a bad type, or a missing declaration.

The duplicate above was found by the user's device build after a full native
compile. It would have been found in one second by:

    ./tools/syntax_check.sh

## A GNUBG GLOBAL DEFINED BY THE PORT IS ENGINE CODE

The de-GTK'd build omits gnubg.c, so android-app.c re-provides globals that
backgammon.h declares extern: esAnalysisChequer, esEvalChequer, aamfAnalysis,
arSkillLevel, and others. Using a gnubg *named instance* is not sufficient if
the port hand-initialises it.

Two bugs found this way, both labelled "copied verbatim" and neither verbatim:

- EVALSETUP_2PLY transcribed the rolloutcontext tail positionally and supplied
  three of its eleven leading bit-fields. Everything slid eight positions;
  nTrials came out ZERO in all four evalsetups. Latent only because et was
  EVAL_EVAL. Fixed by naming every field (5bf9cd1).
- arSkillLevel[] was set to 0.75x gnubg's canonical thresholds, making the
  tutor harsher than gnubg. Realigned (see PROVENANCE.md).

So: **a definition of a gnubg global inside this port is engine code.** It must
be checked against upstream gnubg.c, not trusted because a comment says it was
copied. A "copied verbatim" comment is a claim to verify, not evidence. The
compiler was reporting the EVALSETUP_2PLY bug 24 times per build, and it was
read as noise for months. Warnings in android-app.c are load-bearing.

## THE NEW-FILE TRIPWIRE (this is where the invention accumulated)

Before creating ANY new file that is not purely a Composable (i.e. anything
under tutor/, engine/, jni-bridge/, or a new .c/.h/.kt that holds logic rather
than UI layout), STOP and state IN CHAT, before writing a single line:

  - which gnubg routine or struct this file wraps or exposes, by name; and
  - that it contains no board interpretation of its own.

If I cannot name the gnubg routine it wraps, the file must not be created --
ASK instead. New non-UI files are exactly where five invented analysis files
accumulated (FeatureExtractor, FeatureVector, FeatureDelta, PositionType,
TutorAnalyzer). Requiring this statement per new file would have stopped all
five before the first line. No new logic file without naming its gnubg basis
first. No exceptions, no "I'll wire it to the engine later."

A pre-existing private struct in the facade is NOT evidence that
gnubg has nothing equivalent. It is most often evidence that an
earlier port-rule violation was never cleaned up. Treat such structs
as suspect, not authoritative.

When the checkpoint fails -- by inattention, time pressure, or because
pre-existing code seemed to authorize it -- the change is invalid.
REVERT before adding more work on top. Do not paper over a violation
with a more sophisticated violation.

## ARCHITECTURE (the layers, top to bottom)

Compose UI (Kotlin)
  hub/HomeHubScreen  play/GameLayout  analyse/AnalyseScreen  review/ReviewScreen
  options/SettingsScreen
  -> GameViewModel / BoardState (engine/)
  -> Engine.kt  (JNI external fun declarations)
  -> JNI boundary
  -> native-lib.c   (the ONLY file with <jni.h>; marshalling ONLY, no logic)
  -> android-app.c + gnubg_mobile.{c,h}  (platform-neutral C facade)
  -> engine-core/   (vendored gnubg 1.08.003; the authority)

- The facade is platform-neutral: boards cross as flat int[50], self-locking
  via pthread_mutex_lock(&gnubg_lock). It is the seam where Android meets
  gnubg. New engine access goes through a facade verb, not directly in
  native-lib.c.
- native-lib.c does marshalling only. It must NOT contain game logic. It
  currently reaches the engine directly in only two intentional places
  (gnubg_on_board_changed callback, runCommand/HandleCommand) -- do not add
  more.
- Board encoding: gnubg TanBoard as flat IntArray(50): board[0..24] =
  opponent, board[25..49] = player, index 24 = bar. Human is player 0.

## ENVIRONMENT -- ABSOLUTE PATH RULES

- Home directory is /home/erweitert. NEVER /home/peter.
- NEVER use tilde (~) expansion in any path. Tilde expansion causes
  wrong-path errors in this project. Always write absolute paths.
- Repo root: /home/erweitert/gnubg-android
- App package: com.clavierhaus.gnubg
- Test device: Pixel 8 Pro (serial 47121FDJG00040), Android 16, via adb.

## BUILD AND DEPLOY

- The ONLY build entry point is ./build_and_deploy.sh at the repo root. It
  owns the full pipeline: cmake native build -> copy libgnubg-engine.so ->
  wipe Gradle cache -> assembleDebug -> adb install -> launch.
- Flags: --apk-only (Kotlin-only changes), --native-only, --reconfigure
  (wipe CMake dir; use after C structural changes or stale-cache errors),
  --reinstall, --no-build.
- After ANY C change (engine-core/, jni-bridge/), do a full native rebuild.
- PRISTINE CLEAN BUILDS: every APK is recompiled from a clean state. Do not
  ship or commit on top of a dirty/partial build.
- Verify the build SUCCEEDS and the app launches BEFORE committing. A broken
  native build must never reach a commit.

## LOGCAT (read it correctly or you see nothing)

- Use: adb logcat | grep -E "gnubg-tutor|gnubg-vm"
- NEVER use: adb logcat -s tag:I  (ANDROID_LOG_TAGS override shows nothing)
- The build_and_deploy.sh --logcat flag is unreliable (env override). Run
  logcat as a SEPARATE command after launch.

## GIT -- SOURCE OF TRUTH

- git is the source of truth. Read git state first. Commit before large
  changes. Make checkpoint commits.
- Work on a feature branch, never directly on main for new work.
- Verify a clean build before every commit.
- libgnubg-engine.so is a BUILD ARTIFACT. Keep it OUT of commits. If it shows
  as modified, restore it (git checkout -- <path>) before staging. Stage
  source files explicitly; do not git add -A blindly.
- Commit messages: describe what changed and why; reference the gnubg routine
  used when wiring engine behaviour.

## OUTPUT FORMAT

- PURE ASCII in code, build scripts, and configuration: .c, .h, .kt, .sh,
  .py, Makefiles, .gitignore, meson cross-files, etc. Replace em-dash with
  --, curly apostrophe with ', arrow with ->. The ASCII discipline is for
  files that get grepped, compiled, parsed, or piped through tooling.
- UTF-8 is fine in documentation: docs/, README.md, PROVENANCE.md, and any
  other .md or .tex prose. Em-dashes, section signs, curly quotes, and
  similar typography are welcome where they make prose read better.
- Commit messages: prefer ASCII for grep/tooling compatibility, but it is
  not a hard rule.

## ROOT / SUDO

- Prefix root-touching commands with sudo without being asked: reading
  /boot/efi, /sys/kernel/debug, /proc/acpi/*, and running bootctl, ukify,
  kernel-install, journalctl for older boots, etc. (System-admin context;
  rarely needed for the app build itself.)

## CURRENT STATE (orient here; verify against git, do not trust memory)

- Version: 0.9.1 consolidation. Authoritative status doc: docs/STATUS.md.
  Deep reference: docs/MASTER_V0.9.md. Tutor internals:
  docs/PHASE3_TUTOR_ANALYSIS.md.
- Tutor analysis (Phase 13) is implemented and HAS a UI surface
  (TutorAnalysisPanel). After each human move gnubg scores the move and the
  panel reports blunder level and equity loss. It evaluates at fixed 2-ply via
  the named instance esAnalysisChequer.ec, independent of the opponent-strength
  selector (commit 32a7c91). fac_ec_default no longer exists; any doc still
  claiming 1-ply is stale.
- Engine strength is wired to gnubg's four named presets
  (Beginner/Casual play/Intermediate/Advanced = aecSettings 0..3) via
  gnubg_mobile_set_engine_strength. There is NO Expert/Master in gnubg.
- Home Hub reads: Play Tournament Match -> Analyse Position -> Review Match ->
  Options, with Profile in the corner. "Live Game Analysis" was removed as a hub
  entry; the tutor is a match-setup option that reads the persisted
  settings.tutorMode.
- Analyse Position is BUILT (analyse/AnalyseScreen.kt): paste a GNU BG ID or an
  XGID, gnubg installs and evaluates it.
- Review Match is BUILT in first form (review/ReviewScreen.kt): open a saved .sgf
  and step through it. It keeps no cursor -- gnubg's CommandNext and
  CommandPrevious walk the game record, and the matchstate is read back. The
  per-move verdict is not wired yet, though hint_moves and analyze_played_move
  both exist. It took the third hub slot when it existed, not before.
- AppMode still contains LEARN, which remains unreachable. Learn and Profile
  are scaffolds.
- 5-tab Settings (Tournament/Board/Engine/Analysis/Expert) exists.

## KNOWN-INCOMPLETE (do not assume these work; do not "fix" by reinventing)

- Cube pass/drop after a human double, and beaver handling, are incomplete.
- blockedDiceFor computes s0/s1 but returns an always-empty set (latent bug).
- Cube/resign toast and bar-dance Continue button are deferred, not done.
- Save match [2] and Review Match [3] (the two remaining requested features)
  are NOT built. The engine side of save is already wired -- CommandSaveMatch
  via FACADE_FILE_OP, Engine.saveMatch -- so only Android file plumbing is
  missing. Do not rebuild the engine side.

## HOW TO WORK HERE

1. Read the relevant existing code AND the matching gnubg source before
   proposing a change. Do not pattern-match from memory.
2. Prefer the smallest change that routes through gnubg. If a change needs new
   game logic, you are on the wrong path -- find the engine function.
3. Make surgical, verifiable edits. Show the diff. Build clean. Test on
   device. Then commit (source only, not the .so).
4. When unsure whether gnubg already does something: assume it does, and
   search engine-core/ before writing anything.

If you find yourself reinventing a wheel, STOP and ask.
