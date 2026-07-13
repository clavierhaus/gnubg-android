## GNU Backgammon for Android 0.21.0

The coach learns to speak. When gnubg flags a move in Train with the Coach,
the Why area now explains it: up to two short teaching phrases, each tagged
Race / Board / Threat, matched against gnubg's own position-feature deltas
between the played move and the best. Fourteen patterns ship -- primes,
anchors, blots, hits, blitzes, timing, races -- every signature measured
against the engine, every phrase maintainer-authored. When no pattern fits,
the coach stays silent rather than reaching.

### Added
- **The insight layer** (the headline above). Silence is a feature: phrases
  appear only when the measured signature clears its gates.
- **XG-grade cube analysis in Analyse Position**: winning-chance table for
  both sides, cubeless and cubeful equity, the three action equities with
  deltas from optimal, a Rollout button (144 games, cubeful,
  variance-reduced, with confidence figures), and the match equity table
  named. Changing the MET recomputes a shown result on the spot.
- **The Analyse result stands alone**: after Analyse the entry controls
  give way to the result; Back returns to entry.

### Fixed
- "Cube not available" is honoured -- no decision rows when gnubg says
  there is no cube decision for the roller.
- Match-equity-table changes re-derive a shown result instead of leaving
  stale numbers beside a live label.

### Security
- Every release now ships a SHA256 checksum sidecar (`sha256sum -c
  app-debug.apk.sha256` to verify), and release tags are signed
  (`git tag -v v0.21.0`). This is the first signed release.

**Verifying this download:** each release attaches `app-debug.apk.sha256`.
After downloading both, run `sha256sum -c app-debug.apk.sha256` in the
download directory (macOS: `shasum -a 256 -c`); it prints `OK` when the APK
is intact.
