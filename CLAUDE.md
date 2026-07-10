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
- Home Hub reads: Play Tournament Match -> Analyse Position -> Options, with
  Profile in the corner. "Live Game Analysis" was removed as a hub entry; the
  tutor is a match-setup option that reads the persisted settings.tutorMode.
- Analyse Position is BUILT (analyse/AnalyseScreen.kt): paste a GNU BG ID or an
  XGID, gnubg installs and evaluates it. Review Match is NOT built and has no
  hub slot -- a slot is not reserved for a feature that does not exist.
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
