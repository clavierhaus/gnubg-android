# HANDOVER -- current state for a fresh session (updated 2026-09-21)

## BOOTSTRAP -- paste this as the FIRST message of a new chat

```
Project gnubg-android (CBG Pro). Run exactly this, then continue from the handover:

git clone https://github.com/clavierhaus/gnubg-android /home/claude/repo
cd /home/claude/repo

Then read, in this order, before saying anything else:
1. CLAUDE.md          (the contract -- every order binds)
2. docs/HANDOVER.md   (this file: state, environment, next steps)
3. CHANGELOG.md       ([Unreleased] is what the next release ships)

My tree is /home/erweitert/gnubg-android. Give me only full pastable
command blocks. Never run release_fdroid.sh yourself.
To enable your git push I will paste a GitHub PAT on request.
```

## Credentials -- read this before asking for anything else
A fresh sandbox has NO credential: no token in env, no
/root/.git-credentials, no ssh key, no gh. (These are SANDBOX paths --
the assistant's container runs as root -- never paths on the X1.) The
maintainer pastes a PAT into the chat; install it:

    git config --global credential.helper store
    printf 'https://clavierhaus:%s@github.com\n' '<PAT>' > /root/.git-credentials
    chmod 600 /root/.git-credentials
    git -C /home/claude/repo remote set-url origin https://github.com/clavierhaus/gnubg-android.git

If a push answers "access denied by the git proxy ... will not inject a
credential" (the sandbox's egress proxy owns GitHub auth and ignores the
credential store), pass the PAT as a header on that one command instead:

    TOKEN=$(sed -n 's#https://clavierhaus:\([^@]*\)@github.com#\1#p' /root/.git-credentials)
    git -C /home/claude/repo -c http.extraHeader="Authorization: Basic $(printf 'clavierhaus:%s' "$TOKEN" | base64 -w0)" push origin main

The token stays in the chat, never in a commit. Ask for it once, early,
in the first message that needs a push. Do not propose patch files,
base64 blocks or any other workaround -- that cost a session in September.

## THE X1 -- where every path lives (read before writing any command for the maintainer)

The maintainer's shell user is `peter`; the tree's owner is `erweitert`.
`~` and `$HOME` therefore resolve to the WRONG home on his machine, and
every script default that leans on `$HOME` (release.sh and
release_fdroid.sh look for apksigner under `${ANDROID_HOME:-$HOME/Android/Sdk}`)
resolves to nothing. RULE: every path in every command block is absolute
and starts with `/home/erweitert/`. Never `~`, never `$HOME`, never a
relative path that assumes a cwd the block did not set.

| What                    | Absolute path on the X1                                   | Who needs it |
|-------------------------|-----------------------------------------------------------|--------------|
| repository              | /home/erweitert/gnubg-android                             | everything |
| fdroiddata fork clone   | /home/erweitert/fdroiddata (SIBLING of the repo; release_fdroid.sh defaults to `$(dirname "$ROOT")/fdroiddata`, or `FDROIDDATA=...`) | release_fdroid.sh step 4 |
| Android SDK             | /home/erweitert/android-sdk (must ALSO be exported as `ANDROID_HOME`, see below) | build_native_android.sh, build_glib_android.sh, release.sh (apksigner), release_fdroid.sh (apksigner) |
| NDK (pinned)            | /home/erweitert/android-sdk/ndk/28.2.13676358 (`NDK_VERSION` in build_native_android.sh == recipe `ndk:`) | native build |
| platform (compileSdk)   | /home/erweitert/android-sdk/platforms/android-36 (`compileSdk = 36` in gnubg-app/app/build.gradle.kts) | gradle |
| build-tools / apksigner | /home/erweitert/android-sdk/build-tools/<newest>/apksigner (found by `find "$ANDROID_HOME/build-tools" -name apksigner`) | release.sh, release_fdroid.sh |
| signing config          | /home/erweitert/gnubg-android/gnubg-app/keystore.properties (gitignored; `storeFile=` inside it names the .jks) | release.sh |
| release keystore        | whatever `storeFile=` in keystore.properties says -- verify it, do not assume `~/gnubg-release.jks` from RELEASING.md | release.sh |
| scratch                 | /home/erweitert/gnubg-android/tmp (never /tmp) | scripts |

`ANDROID_HOME` is the one thing the scripts do NOT default to
/home/erweitert: build_native_android.sh and build_glib_android.sh try
`/home/erweitert/android-sdk` as a last resort, but release.sh and
release_fdroid.sh find apksigner ONLY through `$ANDROID_HOME`
(or `$ANDROID_SDK_ROOT`). If it is unset in the shell that runs the
release, preflight dies with "apksigner not found under build-tools".
Every release block therefore begins by exporting it.

### The path check -- paste BEFORE any release, read every line

Expected values are printed from the pins in the tree, then the machine
is asked for what it has. Every `MISSING` or mismatch is a stop.

```
export ANDROID_HOME=/home/erweitert/android-sdk
export ANDROID_SDK_ROOT=/home/erweitert/android-sdk
cd /home/erweitert/gnubg-android
echo "== tree =="; pwd; git status --porcelain; git rev-parse --abbrev-ref HEAD; git log --oneline -1
echo "== pins (expected) =="
grep -n '^NDK_VERSION=' build_native_android.sh
grep -n 'compileSdk' gnubg-app/app/build.gradle.kts
grep -n 'GLIB_VERSION=\|PCRE2_VERSION=' build_glib_android.sh
grep distributionUrl gnubg-app/gradle/wrapper/gradle-wrapper.properties
grep -n 'ndk:' fdroid/com.clavierhaus.gnubg.yml
echo "== SDK on this machine (actual) =="
ls -d /home/erweitert/android-sdk                      || echo "MISSING: SDK"
ls /home/erweitert/android-sdk/ndk/                    || echo "MISSING: ndk/ (expect 28.2.13676358)"
ls /home/erweitert/android-sdk/platforms/              || echo "MISSING: platforms/ (expect android-36)"
find "$ANDROID_HOME/build-tools" -name apksigner | sort -V | tail -n1 || echo "MISSING: apksigner"
echo "== toolchain =="
javac -version; java -version 2>&1 | head -1
cmake --version | head -1; meson --version; ninja --version
nproc
echo "== signing =="
ls -l /home/erweitert/gnubg-android/gnubg-app/keystore.properties || echo "MISSING: keystore.properties"
KS="$(sed -n 's/^storeFile=//p' /home/erweitert/gnubg-android/gnubg-app/keystore.properties)"
echo "storeFile=$KS"; case "$KS" in ~*|\$HOME*) echo "STOP: storeFile uses ~ or \$HOME -- make it /home/erweitert/...";; esac
ls -l "$KS" || echo "MISSING: keystore file named by storeFile"
echo "== fdroiddata =="
ls -d /home/erweitert/fdroiddata/.git                  || echo "MISSING: /home/erweitert/fdroiddata clone"
git -C /home/erweitert/fdroiddata remote -v
git -C /home/erweitert/fdroiddata status --porcelain --untracked-files=no
echo "== auth =="
gh auth status
ssh -T git@gitlab.com 2>&1 | head -1
echo "== helpers =="
ls -l tools/syntax_check.sh tools/rollout_harness/run_tests.sh tools/verify_reproducible.sh tools/fdroid_recipe_append.py release.sh release_fdroid.sh
```

What a correct output looks like, line by line:
- tree: clean (`git status --porcelain` prints nothing), on `main`, at the
  head the assistant named in the pull box.
- ndk/ lists `28.2.13676358` and it equals both `NDK_VERSION=` and the
  recipe's `ndk:` line. platforms/ lists `android-36`.
- apksigner: one path printed under /home/erweitert/android-sdk/build-tools.
- javac and java both 21.x. meson must satisfy the pinned glib's
  `meson_version` (build_glib_android.sh's `meson setup` fails loudly at
  configure if it does not); cmake/ninja any current version (the bytes
  are pinned by us, see CLAUDE.md THE F-DROID BUILD CHECK).
- nproc >= `GNUBG_ROLLOUT_WORKERS` in jni-bridge/src/stubs.c (currently 1)
  or run_tests.sh refuses with exit 3.
- keystore.properties present; `storeFile` an absolute /home/erweitert path
  that `ls` finds. RELEASING.md's `~/gnubg-release.jks` is an example,
  not the truth -- the truth is the line in keystore.properties.
- fdroiddata: `.git` present; remotes `origin` (the gitlab.com fork,
  SSH) and `upstream` (https://gitlab.com/fdroid/fdroiddata.git --
  release_fdroid.sh adds it if absent); no tracked modifications beyond
  metadata/com.clavierhaus.gnubg.yml.
- `gh auth status` logged in as clavierhaus; the gitlab ssh line greets
  by name ("Welcome to GitLab, @...").

Only when every line reads right does the release start
(`./release_fdroid.sh --dry-run ...` first, then without `--dry-run`).
The maintainer runs it; the assistant never does.

## Environment (verified 2026-09-10)
- Maintainer: Fedora, 12 cores, gcc 15, Pixel 8 Pro (1344x2992 @ 480 dpi
  = 997x448 dp landscape, THE reference device), Android SDK at
  /home/erweitert/android-sdk, NDK 28.2.13676358 (pinned in
  build_native_android.sh; build_and_deploy.sh derives it from there).
- Assistant sandbox: ONE core (nproc = 1). Its rollout-harness runs prove
  the serial path only; never cite them as a determinism gate at N > 1.
- Kotlin gate (5 min to set up): openjdk-21-jdk (with javac), cmdline-tools
  at /opt/android-sdk, `yes | sdkmanager --licenses`,
  `echo sdk.dir=/opt/android-sdk > gnubg-app/local.properties` (delete
  before commit), then
  `cd gnubg-app && ANDROID_HOME=/opt/android-sdk ./gradlew --no-daemon :app:compileDebugKotlin`.
- C gates: `tools/syntax_check.sh` and `tools/rollout_harness/run_tests.sh`
  (needs libglib2.0-dev; compiles the port's engine subset in one gcc call).
- Upstream: git.savannah.gnu.org/git/gnubg.git ONLY (clone works
  anonymously; the GitLab URL does not). Vendoring base: b1b2772c
  (PROVENANCE.md). Next monthly check: 2026-10-01.

## State at handover (2026-09-21)
main = see `git log --oneline -20`; everything below is merged and pushed.
Version in tree is 1.0.2 (code 102), released and verified reproducible
on F-Droid (recipe commit 0a28f12b6c51915636d0589f9509ca16891d8262).
[Unreleased] in CHANGELOG.md is the 1.0.3 content: issues #11, #12, #13.
release_fdroid.sh does the bump. The maintainer runs, on the X1, after
the path check above reads clean:

    export ANDROID_HOME=/home/erweitert/android-sdk
    export ANDROID_SDK_ROOT=/home/erweitert/android-sdk
    cd /home/erweitert/gnubg-android
    ./tools/syntax_check.sh
    ./tools/rollout_harness/run_tests.sh      # T1 at GNUBG_ROLLOUT_WORKERS; M1 informational
    ./build_and_deploy.sh --reconfigure       # C changed (new facade verb): full native rebuild
    ./release_fdroid.sh --dry-run --version 1.0.3 --summary "GNU's move traced on the board; Analyse: Edit the analysed position; opponent-on-roll boards drawn from the opponent's side"
    ./release_fdroid.sh --version 1.0.3 --summary "GNU's move traced on the board; Analyse: Edit the analysed position; opponent-on-roll boards drawn from the opponent's side"

Done 2026-09-21, three commits on main, C gate + compileDebugKotlin green:
- #12: gnubg_mobile_get_last_move (newest MOVE_NORMAL of plGame, anMove +
  fPlayer) -> Engine.getLastMove -> BoardState.engineLastMove, set at the
  projection only when the record is GNU's. Board.kt traces it in GNU's
  frame (display point 24-i, count board[i], bar = top half) until the
  player's first sub-move. There was never an animation; the trace is the
  answer to "the board jumped".
- #13: Analyse result now passes turn = onRoll (it passed none). Point
  numbers on STUDY boards (viewModel == null) count from the on-roll
  player's bear-off as gnubg's drawboard.c does by fRoll; dice were
  already on the on-roll side. Live board keeps the human's numbering.
- #11: Edit button in the Analyse result view -> beginEdit(), which seeds
  the editor from the result. The position was never locked; Back led to
  the paste view where Set up starts empty.

Done 2026-09-10/11 (1.0.2): ONE PICTURE (ScreenGrid.kt), upstream sync to
b1b2772c, serial rollout pool, reproducible release pipeline (release.sh
runs build_native_android.sh; two-worktree proof; apksigner
--alignment-preserved). Details in CHANGELOG 1.0.2 and git log.

## Open issues (tracker)
- #10: fixed in 1.0.2 -- reply drafted (close).
- #11, #12, #13: fixed on main, ship in 1.0.3. Replies drafted; the #12
  reply must say "GNU's move is traced on the board" -- NOT a speed
  setting (an earlier draft promised one; nothing of the kind was built).
- #8 hypergammon: upstream question (bug-gnubg thread 2026-09-10); tag
  `upstream`, stays open.
- #4 (undo after both dice): feature, queued.

## Queued next, in order
1. Release 1.0.3 (block above); then post the #10/#11/#12/#13 replies.
2. Read tmp/stale-worktree-edits.diff WITH the maintainer: it is what his
   tree carried uncommitted since before the consolidation (TutorAnalyzer,
   GameViewModel, Engine.kt, gnubg_mobile.c ...). Decide keep/drop.
3. 1.1: the WithLocking family -- USE_MULTITHREAD + MT_SetNumThreads per
   MULTICORE_ANALYSIS.md sections 2.1-2.3 (unshelve) and 3; thread count
   from upstream's default (Takahashi's Sept 2026 patch, MIN(cores,4)),
   never chosen by the port; acceptance = run_tests.sh T1 green at 12.
4. Issue #4 (undo after both dice): replay to the previous decision point.
5. Insight layer per docs/COMPANION.md + CORPUS_*; the August bug-gnubg
   licensing thread (nets trained on 2-ply output) is relevant reading.
6. 2026-10-01: monthly upstream check against b1b2772c.

## Conventions that kept this working (see CLAUDE.md for the binding form)
- Read every file/symbol before asserting anything about it.
- Never speculate: reproduce, instrument, read the failing output. When the
  sandbox cannot reproduce (one core), say so and hand the maintainer a
  self-labelling measurement block; ask for its WHOLE output.
- `git status --porcelain` before any stash/checkout dance on his tree.
- A `grep -c` that finds nothing exits 1 and breaks an `&&` chain.
