# CBG Plus -- overlay discipline

This private repo exists for exactly one thing: the features that define the
Plus edition. Everything else is developed in the PUBLIC repo
(github.com/clavierhaus/gnubg-android) and merges DOWN into here. Never the
reverse. The public repo is the sole home of open development and the
continuously updated source of F-Droid releases; for non-customers it is the
whole product.

## The Plus boundary (authoritative)

Plus-only, developed here on the `plus` overlay:
  a) Insight -- the explanatory verbosity in training (matcher + corpus + Why
     area), and future growth of the corpus.
  b) The board editor in Analyse (free-position setup by hand) and the future
     undo-and-play-the-suggestion flow in coaching mode.

Free, always: everything else -- including the Analyse mode itself with
pasted GNU BG ID / XGID import and analysis, and ID display/copy-out.

## Mechanics

`main` mirrors public main. `plus` = main + one overlay commit re-enabling
the fenced features (CoachScreen+matcher+corpus restored; AnalyseScreen with
the editor entry). Routine: fetch public, merge into main, merge main into
plus; conflicts can only arise in the four overlay files.

## Before any Plus release (Play Store)

- applicationId: at.clavierhaus.backgammon (side-by-side with free)
- own release keystore (never the free key), Play App Signing enrollment
- customer source access per TRADEMARKS.md + GPL section 6(d): full
  corresponding source incl. build scripts to binary recipients
- CONTACT YAIR WAINBERGER (yairwein/backgammon-teacher) before walking any
  Play Store path: agreed covenant (correspondence, 2026-07) -- his MIT
  material is used with his blessing against upstream give-back of whatever
  benefits his project; the publication conversation is owed before launch.
  License notice already carried verbatim in tools/harvest/prompt_template.py
  and the corpus asset acknowledgements.

## Synchronisation law (no exceptions)

Every change, revert, commit and push to the public FOSS repo is mirrored
here IMMEDIATELY: run ./tools/plus/sync_from_foss.sh after every public
mutation. plus-overlay.paths defines the ONLY paths permitted to differ
between public main and the plus branch; tools/plus/check_foss_parity.sh
audits all three invariants (main mirrors public exactly; plus contains
every FOSS commit; zero content drift outside the manifest) and raises an
alarm on the tiniest discrepancy. A red audit outranks all other work.

