# F-Droid submission — checklist and reviewer notes

Target: com.clavierhaus.gnubg on the F-Droid main repository, via a metadata
merge request to fdroiddata (the maintainer-driven "advanced path", faster
than the RFP queue). This document is the maintainer's checklist and the
honest answers to the questions a thorough reviewer will ask.

## Compliance checklist (verified 2026-07-13)

- [x] Public source repo, real up-to-date code (github.com/clavierhaus/gnubg-android)
- [x] FOSS license with a license file: GPL-3.0-or-later, COPYING present
- [x] Releases are tagged (v0.21.2 etc.), tags are SIGNED
- [x] Fastlane metadata in-repo: title, short/full description, 6
      screenshots, icon, per-versionCode changelogs (11, 12)
- [x] Issue tracker public; author contact in-repo (gnubg@clavierhaus.at)
- [x] NO INTERNET permission — provably offline, no network anti-feature,
      no trackers, no ads (verify: no INTERNET/ACCESS_NETWORK in the manifest)
- [x] No proprietary dependencies: no Google Play Services, Firebase,
      Crashlytics, or play-services of any kind
- [x] No prebuilt binaries tracked (the .so is BUILT from source by
      F-Droid; jniLibs/ is gitignored; only the standard gradle-wrapper.jar
      is present, which F-Droid rebuilds)
- [x] Signing inverted for F-Droid: absent keystore.properties ->
      assembleRelease produces an UNSIGNED APK; F-Droid applies its own key
      (RELEASING.md documents this)
- [x] From-source native build: build_glib_android.sh + build_native_android.sh
      compile glib and the gnubg engine with a PINNED NDK (27.0.11718014),
      arm64-v8a, android-28 — no prebuilt native code enters the APK

## The three things a reviewer will question — and the honest answers

### 1. The insight corpus was drafted with an LLM

A reviewer reading NOTICE, CORPUS_HARVEST_PLAN.md, or the shipped
insights_v0.json will see that the coaching phrases were first-drafted with
large language models (claude-sonnet-4-6). This is unusual but fully FOSS-
compliant, and the paper trail is deliberate, not hidden:

- What SHIPS is gnubg-app/app/src/main/assets/insights_v0.json — a GPLv3
  data file, checked in, built by the checked-in tools/harvest/bake.py from
  checked-in curated markdown. It is Corresponding Source under GPLv3 §1.
- No LLM output ships verbatim. Every one of the 18 phrases was human-
  verified and edited into original expression before inclusion; the asset
  records this (draft_assistance_note) and the curated files record the
  per-entry editing and rejection reasoning. The maintainer authored the
  final wording ("tier: authored").
- The offline harvest tooling (tools/harvest/) is not invoked by the app,
  ships nothing into the APK, and needs no API key to build the app. If the
  LLM providers vanished, the APK would be byte-identical.
- Nothing here is an anti-feature under F-Droid's definitions (no non-free
  network service at runtime, no non-free dependency, no tracking).

Short version for the MR: "Coaching phrases were LLM-assisted first drafts,
each edited into original GPLv3 expression by the maintainer; the shipped
asset is checked-in GPL data built by a checked-in script; no model is
invoked at build or run time."

### 2. engine-core is a vendored copy of gnubg, not a submodule

F-Droid prefers external code as git submodules over copied trees.
engine-core/ is a curated subset of GNU Backgammon (GPLv3+, same license as
this app), copied rather than submoduled because: (a) it is a SUBSET —
only the engine translation units the Android port compiles, not the full
gnubg tree with its GTK/desktop code; (b) it carries small, tracked port
adaptations; (c) gnubg ships as release tarballs on ftp.gnu.org, not as a
git repo suitable for submoduling at a pinned commit. The provenance is
documented and the license is identical to the app's, so there is no
license-mixing question — only a packaging-style preference. Prepared to
discuss if the reviewer wants it restructured.

### 3. gnubg.weights is a ~1 MB neural-net blob

The bundled asset gnubg.weights is GNU Backgammon's trained neural-network
weights, shipped as data under gnubg's GPLv3+. It is not a prebuilt
executable and not opaque proprietary content — it is the engine's free
trained parameters, the same file gnubg distributes. Free data, GPL,
redistributable. (If a reviewer treats bundled trained weights as needing a
build-from-training story, note that gnubg itself ships these as data, not
as something rebuilt per release — upstream precedent.)

## The build recipe (DONE — fdroid/com.clavierhaus.gnubg.yml)

Reference recipe committed at fdroid/com.clavierhaus.gnubg.yml (the
authoritative copy lives in the maintainer's fdroiddata fork). Key point,
the one thing that would otherwise fail F-Droid's isolated build:

- build_glib_android.sh fetched GLib over the network (download.gnome.org).
  F-Droid's build phase is network-isolated, so this is now offline-aware:
  the script prefers a PRE-PROVIDED archive at the repo root and only curls
  as a local-dev fallback (verified: given the archive, the selection loop
  never reaches curl). The recipe provisions + sha256-verifies the archive
  in the non-isolated `sudo` prebuild phase, then copies it to the repo root
  where the script finds it.

Verified in the assistant sandbox: script syntax, and the offline
archive-selection logic. NOT verifiable here (no NDK/SDK/meson): the full
native build and the gradle assemble — those must be proven in F-Droid's
own buildserver Docker image (`fdroid build com.clavierhaus.gnubg`) before
opening the MR. That local `fdroid build` is the real go/no-go gate.

## Submission steps (maintainer, on GitLab)

1. Register/fork fdroiddata on GitLab.
2. Add metadata/com.clavierhaus.gnubg.yml (drafted next, in this repo for
   reference; the authoritative copy lives in the fdroiddata fork).
3. Prove it in F-Droid's own buildserver Docker image:
   fdroid readmeta / rewritemeta / lint / build com.clavierhaus.gnubg
4. Commit with the "New App" label; open the merge request; reference any
   RFP issue; respond promptly to reviewer questions.
5. Reproducible builds are best-practice-not-required, but encouraged for
   new apps (can't be added later without breaking the signing key). Decide
   whether to enable now.

Note (0.22.0): the coaching-insight corpus (insights_v0.json) and its
matcher are no longer part of this application as of 0.22.0. The coach
shows gnubg's own evaluations directly. Statements above about the corpus
describe releases up to 0.21.7 only.
