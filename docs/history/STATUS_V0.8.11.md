# GNU Backgammon by clavierhaus.at — Status V0.8.11

## Milestone

V0.8.11 is a structural and strategic milestone.

The Android front-end now exposes a broader GNU Backgammon lifecycle and command surface through the JNI bridge, Kotlin `Engine.kt`, and `GameViewModel` wrappers. This does not introduce new visible UI controls yet. Instead, it lays the groundwork for future Play, Analyse, Learn/Tutor, and session-control implementations to act through GNUbg itself rather than through Android-side approximations.

## Strategic significance

Until V0.8.10, the Android application could only use the GNUbg functions that had already been bound by the initial bridge implementation. That exposed surface was useful but incomplete.

V0.8.11 corrects the architectural direction:

- GNUbg remains authoritative for game, match, session, cube, file, and lifecycle state.
- Android presents UI, navigation, configuration, and user interaction.
- Missing GNUbg controls should be exposed deliberately through JNI/ViewModel wrappers.
- Kotlin must not invent parallel game semantics when GNUbg already provides them.

This is important for long-term functional equivalence with desktop GNU Backgammon.

## Commands now represented at Android bridge level

The groundwork exposes or prepares ViewModel access to GNUbg lifecycle and command functions including:

- new game
- new match
- new session
- end game
- resign
- next
- accept
- reject
- decline
- agree
- redouble
- load game
- save game
- load match
- save match
- load position
- save position

The earlier SGF-oriented load/save calls remain available.

`quit` was intentionally not bound at this stage, because it is not currently linked into the Android native bridge and has unclear Android app-shell semantics.

## UI implications

No new UI controls were added in V0.8.11.

The design rule remains:

- Settings screens contain configuration only.
- Live board interactions remain on the board surface.
- Roll, double, move, undo, confirm, dice handling, and destination-stack convenience must not be duplicated in the reserved left-side control area.
- The lower-left reserved area is now understood as the future location for genuine GNUbg-backed lifecycle controls such as resign, end game, new match, and possibly save/load workflows.

Any future button in that area should call a GNUbg-backed ViewModel method, not fake state transitions in Kotlin.

## Current mode architecture

The app still starts at the Home Hub.

Top-level modes remain:

- Play
- Learn
- Analyse
- Options
- Profile

The existing Play implementation remains preserved. Learn, Analyse, and Profile remain structural placeholders. Options continues to wrap the configuration/settings surface.

## Build status

At the time of this milestone:

- Android debug build succeeds.
- Native `gnubg-engine` bridge build succeeds.
- Commit `e30e234` records the GNUbg lifecycle bridge groundwork.

## Version summary

V0.8.11 should be understood as:

> The point where the project stopped treating the initial Android bridge as the boundary of GNUbg functionality, and began exposing GNUbg’s own command architecture as the foundation for future Android UI.
