# HANDOVER — session of 2026-07-12 (coach state rebuild → 0.20.0 release)

## BOOTSTRAP — paste this as the FIRST message of a new chat
A fresh session knows nothing: no repo, no paths, no state. This block is the
only thing that must be pasted by hand; everything else follows from it.

```
Project gnubg-android. Run exactly this, then continue from the handover:

git clone https://github.com/clavierhaus/gnubg-android /home/claude/repo
cd /home/claude/repo

Then read, in this order, before saying anything else:
1. CLAUDE.md          (the contract -- every order binds)
2. docs/STATUS.md     (authoritative current state)
3. docs/HANDOVER.md   (what is in flight, environment, next steps)

My tree is /home/erweitert/gnubg-android. Give me only full pastable
command blocks. Never run ./release.sh yourself.
```

Purpose: a fresh assistant session continues seamlessly from here. Tracked in
git so `git pull` keeps the maintainer's tree and any assistant clone in sync.

## Read in this order, before anything else
1. **CLAUDE.md** — the contract. Every order binds; THE SPIRIT BINDS governs
   them all. Two orders were added this session:
   - *ONE STATE, ONE WRITER, PROJECTED FROM GNUBG* — gnubg is sole authority
     for match STATE as for LOGIC; the UI state is a projection with exactly
     one writer; transient app-side state is a thin named overlay.
   - *KOTLIN IS COMPILE-VERIFIED BEFORE EVERY PUSH* — `syntax_check.sh` is
     C-only; any Kotlin commit must pass `:app:compileDebugKotlin` in the
     assistant sandbox first.
2. **docs/STATUS.md** — authoritative current state (Coach COMPLETE, 0.20.0).
3. This file — what is in flight and what comes next.

## Environment
- Maintainer tree: `/home/erweitert/gnubg-android` (Fedora 44; he builds with
  `./build_and_deploy.sh`, `--apk-only` for Kotlin-only). Test device:
  Pixel 8 Pro, landscape. Logcat, always:
  `adb logcat -c` then `adb logcat -v time -s gnubg-vm:I gnubg-coach:I gnubg-cube:I`
- Assistant clone: `/home/claude/repo` (fresh session: clone
  `https://github.com/clavierhaus/gnubg-android`, branch main).
- Assistant compile gate (rebuild in a fresh sandbox; ~5 min, one time):
  cmdline-tools → `/opt/android-sdk/cmdline-tools/latest`, accept licenses,
  `apt-get install -y openjdk-21-jdk-headless`,
  `echo "sdk.dir=/opt/android-sdk" > gnubg-app/local.properties` (**keep
  untracked; delete before commits**), then
  `cd gnubg-app && ANDROID_HOME=/opt/android-sdk ./gradlew --no-daemon :app:compileDebugKotlin`.
  No NDK needed: the .so is built outside Gradle by build_and_deploy.sh.
- The assistant NEVER runs `./release.sh` (needs the maintainer's gh auth and
  signing key). Commits: `git commit -F tmp/m.txt`, push immediately.

## IN FLIGHT: release v0.20.0 — one command from done
Staged and verified: CHANGELOG `[0.20.0] — 2026-07-12`, RELEASE_NOTES.md,
versionName 0.20.0 / versionCode 8 (staging commit `cbda0d6`). All release.sh
guards pass through "buildable-clone check"; the gh guard was the last blocker
and was rewritten (`ce251a7`) to validate ONLY the active account via
`gh api user` (a stale inactive account, invalid default `OE1FEU-DF5JT`, was
failing `gh auth status` beside the valid active `clavierhaus`).
**Next step (maintainer):** `git pull origin main && ./release.sh --dry-run && ./release.sh`
**Verify:** v0.20.0 tag + release at github.com/clavierhaus/gnubg-android/releases.
Optional hygiene, his call: `gh auth logout -h github.com -u OE1FEU-DF5JT`.

### Release-tooling defects fixed this session (the release itself was never broken)
- Tag-fetch guard died silently on a stale local tag diverging from origin
  (leftover of the old history reset). Fix `6b384db`; user-side remedy
  `git fetch origin --tags --force` (already applied on his machine).
- Buildable-clone check resolved includes against the LOCAL DISK, so untracked
  vendoring leftovers (engine-core/gtk/*, inc3d.h, render.h — deliberately
  .gitignored, never compiled on Android) produced machine-dependent false
  positives. Fix `bc36529`: resolve against the tracked set (`git ls-files`),
  i.e. exactly what a fresh clone contains. GTK is NOT build-required.
- Recurring defect class across all three guards: output swallowed by
  `>/dev/null`. All now fail with named reasons. Preserve this principle.

## This session's architecture delta (already merged and field-verified)
GameViewModel was rebuilt under the ONE-STATE order (core commits `994dfe0`,
`11fe94d`):
- `settleFromEngine(result?)` is THE settle: derives the phase FROM gnubg
  after every engine op (game over / resignation / cube / rolled human turn /
  waiting). `result` = score-delta game-over context (survives gnubg's
  NextTurn auto-advance, play.c:291). No caller picks a phase.
- `beginEngineWork(BusyKind)` is the ONE entry into engine work: clears both
  coach glances + pendingCubeAction, sets the busy overlay, arms the dice
  watcher. `holdCoachReview()` is the ONE coach-hold entry.
- `watchEngineDice` (Dispatchers.Default) writes ONLY its leaf flow
  `_liveEngineDice`; public `gameState` = combine(_gameState, leaf), gated on
  ENGINE_THINKING. The cross-thread clobber race is dead by construction.
- `BusyKind { NONE, JUDGING, REPLYING }` (BoardState.kt) drives the panel's
  thinking text. Deleted inventions: `_coachReplying`, `performingHeldCube`,
  phase-faking in `continueCoachCube` (now calls `doRollNow`/`offerDoubleNow`/
  `takeNow`/`dropNow` directly), scattered glance clears, per-path settles.
- M4 completed: GNU's doubles are answerable (CUBE_OFFERED branch in
  CoachScreen: "GNU doubles to N." + Take/Drop → coach diversion → judged,
  held, carried out on GNU's turn).

**Invariant greps — run after ANY GameViewModel edit:**
- `readMatchState(phase` appears ONLY inside `settleFromEngine`.
- `phase = GamePhase.ENGINE_THINKING` written ONLY in `beginEngineWork`.
- `GamePhase.COACH_REVIEW)` written ONLY in `holdCoachReview`.
- `watchEngineDice` never touches `_gameState`.

## Queued next (in the maintainer's stated order of interest)
1. Confirm v0.20.0 published; then open `[Unreleased]` work.
2. **Insight layer** ("verbal reasoning on top" — his stated next goal):
   docs/COMPANION.md + docs/CORPUS_PLAN.md are the plan. Phase A = a gnubg
   features verb; Tier-A original GPLv3+ phrases; backgammon-teacher MIT
   feature schema with attribution. No-double/missed-double coaching returns
   here, NON-blocking (docs/COACH_CUBE_PLAN.md).
3. Try-again sandbox (M1 remainder, docs/STATUS.md gaps section).
4. Decide whether the `gnubg-cube` diagnostic logs stay post-release.

## Conventions that keep this working
- Full pastable command blocks, always (cd + every command + flags).
- Read every file/symbol before asserting anything about it, every session.
- Never speculate: reproduce, instrument, read the failing output.
- Fix the function the evidence implicates, and nothing else (Q-3).
- Docs for the maintainer: serif fonts only (docx already committed:
  docs/QUICKSTART.md + docs/QUICKSTART.docx).
- All maintainer-facing sysadmin commands: `sudo` where root paths are touched.
