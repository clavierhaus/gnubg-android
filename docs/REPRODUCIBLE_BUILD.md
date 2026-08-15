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
