# GNU Backgammon for Android — Status V0.8.10

Internal development status after the Home Hub restructuring baseline.

## Version

Current internal status: **0.8.10**

This is not a public release declaration. It marks the first documentation checkpoint after the 0.8.9 private consolidation, the generated-cache cleanup, and the mode-based Home Hub scaffold.

## Repository baseline

The current baseline separates two concerns that were previously mixed in the working tree:

1. Generated Android and Gradle build output is no longer tracked.
2. The application shell now has a mode-based Home Hub scaffold.

Relevant recent commits:

- `dcfb503` — Introduce mode-based Home Hub scaffold
- `906af76` — Untrack generated Android build cache
- `fe021ee` — Ignore private evaluation APK builds
- `dcbeed9` — Snapshot before Home Hub restructure

## Architectural status

The app now starts from a sparse Home Hub rather than directly entering Play.

Current top-level areas:

- Play
- Learn
- Analyse
- Options
- Profile

Play remains the existing live game. The working Play implementation was moved into its own package area, not redesigned. This preserves the GNUbg bridge, match play, dice handling, cube-handling work, undo behaviour, destination-stack convenience, and board rendering while making room for the broader application structure.

Learn, Analyse, and Profile are placeholders at this checkpoint. Options currently wraps the existing settings surface.

## Package layout

Current source layout:

- `hub/` — Home Hub screen
- `play/` — existing live game UI, moved from the old `ui/` package
- `learn/` — placeholder mode
- `analyse/` — placeholder mode
- `options/` — settings/options area
- `profile/` — simplified profile/defaults placeholder
- `shared/` — shared app-mode model
- `engine/` — existing GNUbg-facing engine/view-model layer
- `ui/theme/` — retained theme package

## User-visible behaviour

Expected behaviour at this checkpoint:

1. The app launches into the Home Hub.
2. Tapping Play enters the existing live board/game.
3. Tapping Learn, Analyse, or Profile opens a simple placeholder screen.
4. Tapping Options opens the existing settings surface through the new options wrapper.

## Home Hub design status

The Home Hub is intentionally sparse at this stage. It is not yet a finished visual design, and it should not be read as a final interface specification.

The structural idea is simple: the app opens into a quiet central hub with four primary mode entries — Play, Learn, Analyse, and Options — while Profile remains separate as a universal player/defaults area rather than a gameplay mode. The central visual field is deliberately reserved for later design work.

At this checkpoint, the purpose of the Home Hub is architectural: to prove that the application can start from a mode-based shell while preserving the existing Play implementation unchanged.

## Important constraints

GNU Backgammon remains authoritative for game logic. Kotlin/Android handles app structure, presentation, rendering, touch interaction, and convenience behaviours. The Play mode must not drift into a separate reimplementation of backgammon rules.

The existing Play mode is deliberately preserved as the working core. Future work should avoid destabilising it while building Learn, Analyse, Options, and Profile around it.

## Known limitations at this checkpoint

- Learn is a placeholder.
- Analyse is a placeholder.
- Profile is a placeholder.
- Options currently wraps the existing settings screen.
- The Home Hub has no final visual design.
- No library/archive surface has been introduced as a top-level mode.
- Set Up Board is not yet implemented under Analyse.
- The Play mode still carries the live 0.8.9 game behaviour and known limitations.

## Next intended steps

Likely next steps for 0.8.10:

1. Refine the Home Hub structurally without visual overdesign.
2. Add a minimal Profile/defaults surface:
   - player
   - new games
   - help during play
   - look and feel
3. Add Analyse placeholder subdivisions, including future Set Up Board.
4. Keep Play stable and regression-test after each shell change.
5. Update README and technical notes once the mode shell has settled.
