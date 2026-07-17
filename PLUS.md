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

`main` mirrors public main. `plus` = main + ONE overlay commit re-enabling
the fenced features (insight files restored, editor entry button restored).
Routine: fetch public, merge into main, merge main into plus.

## Before any Plus release (Play Store)

- applicationId: at.clavierhaus.backgammon (side-by-side with free)
- own release keystore (never the free key), Play App Signing enrollment
- customer source access per TRADEMARKS.md + GPL section 6(d): full
  corresponding source incl. build scripts to binary recipients
