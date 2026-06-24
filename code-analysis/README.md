# code-analysis

Persistent audit tooling for the GNUbg Android repository.

This directory is tracked by git and must not depend on `tmp/`.

Generated audit output goes to:

    tmp/codebase_audit_<timestamp>/

The `tmp/` directory is disposable. Every audit run therefore creates a complete
standalone bundle.

## Primary script

Run from anywhere inside the repository:

    bash code-analysis/collect_codebase_audit.sh

or:

    bash code-analysis/collect_codebase_audit.sh --open

`--open` generates the bundle and opens `00_index.md` in `$PAGER`.

## Audit scope

This is a whole-codebase audit.

It is not merely a JNI-integration audit and not merely a de-Androidification
audit.

The central source-of-truth question is:

    Does any layer rewrite, approximate, duplicate, bypass, or reinterpret GNUbg?

GNUbg is the gameplay authority. Any gameplay-domain concept outside GNUbg must
either be a faithful, documented GNUbg-derived facade or be flagged for review.

The audit also checks:

- repository structure;
- build coherence;
- Android/Kotlin cleanliness;
- Java cleanliness, if present;
- resources and binary hygiene;
- JNI/native bridge cleanliness;
- embedded GNUbg/upstream adaptation;
- architecture and state ownership;
- duplicated concepts;
- documentation/code drift;
- test coverage and handover readiness.

## Output bundle

A run creates:

    tmp/codebase_audit_<timestamp>/
      00_index.md
      01_repo_structure.txt
      02_git_log.txt
      03_build_system.md
      04_android_manifest.xml
      05_kotlin_sources.md
      06_java_sources.md
      07_android_resources.md
      08_jni_bridge.md
      09_cmake_and_headers.md
      10_engine_core_sources.md
      11_gnubg_source_of_truth_audit.md
      12_reinvention_duplication_scan.md
      13_architecture_state_ownership.md
      14_code_quality_hotspots.md
      15_documentation.md
      16_documentation_code_discrepancies.md
      17_test_coverage.md
      18_binary_resource_inventory.md
      19_all_text_sources.md
      20_integrity_hashes.sha256
      21_archive.sha256
      resource_binaries/
      codebase_audit_bundle.tar.gz

## Exclusions

The collector excludes:

- `.git/`
- `.gradle/` and nested Gradle caches
- `.kotlin/` and nested Kotlin caches
- `tmp/` and nested scratch directories
- build directories
- generated native/CMake build output
- nested `.deps/` compiler dependency directories
- `upstream-source/gnubg/autom4te.cache/`
- `jni-bridge/external/glib/` vendored dependency tree
- APK/AAB/class/object/shared-library build outputs
- archive files
- `external/backgammon-teacher/`

`external/backgammon-teacher/` is excluded entirely because it is not part of
the current concept.
