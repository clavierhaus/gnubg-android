# Reproducible builds — CBG and CBG Pro

**The claim:** building this repository's release commit produces a
byte-identical unsigned release APK, on any machine with the same
toolchain. We do not ask to be believed; this document is the recipe
for checking.

Both editions make the claim the same way: CBG (this tree, `main`) and
CBG Pro (the `plus` branch of the product repository) share one build
pipeline, one recipe, and one signing key. A signature is attached to
identical bytes; it is never part of them — which is why the comparable
artifact is the **unsigned** release APK, exactly as F-Droid's
verification defines it.

## Check it yourself

    ./tools/verify_reproducible.sh

The script builds the checked-out commit **twice, independently** — two
fresh `git worktree` checkouts, each running the repository's own build
scripts end to end (the native build including glib, then
`assembleRelease` with no signing key present) — and compares the two
APKs by sha256. Exit code 0 is the claim holding; on failure both
worktrees are kept so the artifacts can be diffed (`diffoscope` is the
right tool).

The script contains no build logic of its own. Whatever a release runs,
it runs — twice. If the release process changes, the verification
changes with it, because they are the same scripts.

## What makes the builds deterministic

Nothing exotic — the boring, auditable measures, all visible in the
build scripts themselves:

- Source paths are erased from binaries (`-ffile-prefix-map` for the
  repository, the glib source tree, and the NDK).
- Compile-time timestamps are disabled (`-D__DATE__= -D__TIME__=
  -D__TIMESTAMP__=`).
- The build stamp shown in the app is the **commit time**, not the
  build time.
- The dependency build (glib, pcre2) uses the same flags through its
  cross file.
- Gradle inputs are pinned by the lockfiles and wrapper in the tree.

## Toolchain

The authoritative toolchain pins for published builds live where they
are enforced, not restated here: the F-Droid metadata recipe for each
app id pins the exact commit and build environment F-Droid's CI uses to
verify the published APK against the maintainer's. If you rebuild with
a materially different NDK or SDK revision, expect the comparison to
tell you so — that is the comparison doing its job.

## The one key

Both editions are signed with the same release key
(`367c17e5…`). Verification never needs it: the claim is about the
bytes before the signature, and those you can produce yourself.

## F-Droid's environment, locally (added 2026-09-11)

The check above proves this tree builds the same way twice **on one
machine**. It does not prove it builds the same way in F-Droid's
container, and for three releases that difference was met for the first
time inside F-Droid's CI, forty minutes a round. The fourth release
found three new ways to fail there before a single line of Gradle ran:
a stale edit in the fdroiddata clone, a recipe rewrite that duplicated
every published build block, and a hard-coded NDK path in the dev build.
None of them was about the code.

So the release now builds **in F-Droid's container, on the maintainer's
machine, first**:

    ./tools/fdroid_build_local.sh 102

That script is fdroiddata's own `fdroid build` CI job, mirrored line by
line (image `registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie`,
fdroidserver at master, `fdroid build --test --on-server --no-tarball`),
with only the GitLab plumbing removed. The APK it leaves in
`$FDROIDDATA/tmp/` is byte-for-byte what F-Droid's verifier will build
for the same recipe commit -- it is the same program, in the same image,
reading the same recipe.

The cycle then runs in the only order that makes the verifier a
confirmation rather than a step: build here in the container, sign that
APK, attach it to the GitHub release, push the recipe, and F-Droid's
comparison passes on its first run. A failure shows up here, in minutes,
with the log in front of the maintainer -- and it is fixed in the recipe
or the scripts *before* anything is tagged.

The named volumes `fdroid-build-cache-ndk` and `fdroid-build-cache-gradle`
keep the NDK and Gradle downloads between runs; `podman volume rm` them
to start cold.
