# HANDOVER -- current state for a fresh session (written 2026-09-10, evening)

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
A fresh sandbox has NO credential: no token in env, no ~/.git-credentials,
no ssh key, no gh. The maintainer pastes a PAT into the chat; install it:

    git config --global credential.helper store
    printf 'https://clavierhaus:%s@github.com\n' '<PAT>' > ~/.git-credentials
    chmod 600 ~/.git-credentials
    git -C /home/claude/repo remote set-url origin https://github.com/clavierhaus/gnubg-android.git

The token stays in the chat, never in a commit. Ask for it once, early,
in the first message that needs a push. Do not propose patch files,
base64 blocks or any other workaround -- that cost a session in September.

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

## State at handover
main = see `git log --oneline -20`; everything below is merged and pushed.
Version in tree is still 1.0.1 (code 101): release_fdroid.sh does the bump.
[Unreleased] in CHANGELOG.md is the 1.0.2 content. The maintainer runs:

    ./tools/syntax_check.sh
    ./tools/rollout_harness/run_tests.sh      # T1-T4 green; M1 "DIFFER at 12" is expected
    ./build_and_deploy.sh --reconfigure
    ./release_fdroid.sh --dry-run --version 1.0.2 --summary "..."
    ./release_fdroid.sh --version 1.0.2 --summary "..."

Done today (2026-09-10), each its own commit, all compile/harness-gated:
- THE SCREEN IS ONE PICTURE (CLAUDE.md, shared/ScreenGrid.kt): OnePicture at
  MainActivity's mode switch, every board call site Unscaled, the law in
  the contract, tools/geometry_sweep.sh. Replaces four same-day band-aids.
  Issue #7 (OnePlus 15, 2772x1272) is fixed by it; reply drafted, post
  after the maintainer's device shows the rail whole at reset.
- Game-over rail fit at reference: no Spacer CHILDREN in a pitched column.
- Analyse "Start pos" preset: engine half in the engine's frame (was under
  white; proven by the 208-pip count in the mailed screenshots). Closes #1.
- Upstream sync to b1b2772c: 31 commits carried, 5 seams 3-way merged,
  per-commit verdicts in CHANGELOG. Same-seed rollouts byte-identical.
- The 6 September orders entered in CLAUDE.md; PROVENANCE names Savannah.
- build_and_deploy.sh derives the NDK from the pinned version.
- ROLLOUT POOL IS SERIAL (stubs.c gnubg_init_rollout). The NoLocking
  evaluation family shares cEval without locks; at 12 workers same-seed
  rollouts differed on every run, on the pre-sync tree too. Proven on the
  maintainer's machine: 12/12 identical at 1 worker, 12 distinct at 12.
  MULTICORE_ANALYSIS.md section 3. Gate B's 1-vs-4 claim withdrawn.
  GNUBG_ROLLOUT_THREADS overrides for measurement; run_tests.sh M1 shows it.

## Open issues (tracker)
- #7: fixed on main, awaiting device confirmation and the reply.
- #8 hypergammon: upstream question; the maintainer's thread is on
  bug-gnubg (2026-09-10). Tracker reply drafted; issue stays open, tag
  `upstream`. Requires hyper*.bd (port ships no .bd) and a MET decision.
- #1, #3, #4, #5: maintainer said he closes these (#5 delivered in 1.0.0;
  #1 delivered + preset fix; #4 is a feature: replay to prior decision).

## Queued next, in order
1. Post: #7 reply; the bug-gnubg sync announcement (docs/upstream, below).
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
