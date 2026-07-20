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

To enable your git push, I will paste a GitHub PAT (repo scope) on request;
install it per the Credentials section of docs/HANDOVER.md. Build the Kotlin
compile gate per that same file before your first Kotlin push.
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
- Assistant clone: `/home/claude/repo`.

### Credentials (NEVER written to any tracked file — supplied at runtime)
This repo is public and git-tracked; a token committed here is a public leak.
So no secret lives in this document. Instead:

**GitHub read/write (assistant push access).** A fresh sandbox has no
credential. To enable `git push`, the maintainer pastes a PAT into the new
chat and the assistant installs it into the sandbox credential store (the
token value stays in the chat, never in a commit):
```
git config --global credential.helper store
printf 'https://clavierhaus:%s@github.com\n' 'PAT_GOES_HERE' > ~/.git-credentials
chmod 600 ~/.git-credentials
git -C /home/claude/repo remote set-url origin https://github.com/clavierhaus/gnubg-android.git
```
PAT scope needed: `repo` (contents read/write). Classic or fine-grained both
work; fine-grained should grant Contents: Read and write on this repo. If push
returns 403, the token lacks `repo`/Contents-write or has expired — the
maintainer issues a new one at github.com/settings/tokens. Cloning a public
repo needs no credential; only pushing does.

**Release signing + publish (maintainer only, never the assistant).**
`./release.sh` needs the maintainer's `gh` auth (active account `clavierhaus`)
and the APK signing keystore. `keystore.properties` is gitignored and lives
only on the maintainer's machine; the assistant never has it and never runs
release.sh. Nothing about signing belongs in the sandbox.

### Assistant Kotlin compile gate (contract: verify before every Kotlin push)
`syntax_check.sh` is C-only; brace-counting is inventory, not verification.
A fresh sandbox has no Android toolchain — build it once (~5 min). No NDK is
needed: the gate compiles Kotlin only, against the prebuilt `.so` already in
`gnubg-app/app/src/main/jniLibs/arm64-v8a/` (the maintainer's
build_and_deploy.sh builds the `.so` with the NDK; the assistant does not).

One-time setup in a new sandbox:
```
# 1. Java compiler (javac, not just the JRE)
apt-get install -y openjdk-21-jdk-headless

# 2. Android SDK command-line tools
mkdir -p /opt/android-sdk/cmdline-tools && cd /tmp
curl -sS -o clt.zip https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip -q clt.zip -d /opt/android-sdk/cmdline-tools
mv /opt/android-sdk/cmdline-tools/cmdline-tools /opt/android-sdk/cmdline-tools/latest
yes | /opt/android-sdk/cmdline-tools/latest/bin/sdkmanager --licenses >/dev/null 2>&1 || true

# 3. Point Gradle at the SDK (local.properties MUST stay untracked)
echo "sdk.dir=/opt/android-sdk" > /home/claude/repo/gnubg-app/local.properties
```
Run the gate before every push that touches Kotlin:
```
cd /home/claude/repo/gnubg-app && \
  ANDROID_HOME=/opt/android-sdk ./gradlew --no-daemon :app:compileDebugKotlin
```
Expect `BUILD SUCCESSFUL`. Missing SDK packages (compileSdk 35, build-tools)
download automatically on first run. **Before any commit:
`rm -f /home/claude/repo/gnubg-app/local.properties`** — it is machine-specific
and must never be committed. Standard commit loop: run the gate → confirm green
→ `rm local.properties` → `git commit -F tmp/m.txt` → `git push`.

## IN FLIGHT: release v0.20.1 — one command from done
0.20.0 shipped 2026-07-12. 0.20.1 is a cosmetic follow-up with two
user-visible changes and no engine changes:

  1. Coach move-list chip labels ("P" for the player's move, "1"/"2"/"3" for
     gnubg's better alternatives) drew as coloured pills without letters since
     the coach shipped, because a 30dp containing slot squeezed the compact
     button's inner Text constraint to zero width (30 − 19 − 19 dp of padding).
     Replaced with a 26dp circular IdentChip in CoachScreen.kt.
  2. Publisher mark "clavierhaus.at" added to the hub's top-right corner,
     symmetric with the settings gear, DejaVu Serif with "vie" in GNU orange
     (Vienna pun), the rest off-white (HomeHubScreen.kt).

Both changes verified with :app:compileDebugKotlin green in the assistant
sandbox before push.

Staged: CHANGELOG `[0.20.1] — 2026-07-12` (Added + Fixed), RELEASE_NOTES.md
rewritten, versionName 0.20.1 / versionCode 9, STATUS.md header + build line
updated, this file updated. All release.sh guards known-good from the 0.20.0 run.
**Next step (maintainer):** `git pull origin main && ./release.sh --dry-run && ./release.sh`
**Verify:** v0.20.1 tag + release at github.com/clavierhaus/gnubg-android/releases.

### Release-tooling defects fixed prior to 0.20.0 (kept for reference, no re-work needed)
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
