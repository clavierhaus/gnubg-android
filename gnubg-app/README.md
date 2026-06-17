# GNU Backgammon by clavierhaus.at


<!-- GNUBG_ANDROID_STATUS_START -->
## Current milestone: V0.8.11

V0.8.11 is a structural and strategic milestone. The Android front-end now exposes a broader GNU Backgammon lifecycle and command surface through the JNI bridge, Kotlin `Engine.kt`, and `GameViewModel` wrappers.

This version does **not** add new visible game-control UI. It lays the groundwork for future Android controls to call GNUbg-native lifecycle commands directly, preserving GNUbg as the authoritative source for game, match, cube, session, file, and lifecycle state.

Key points:

- Android now has bridge groundwork for GNUbg lifecycle commands such as new game, new match, new session, end game, resign, next, accept/reject/decline/agree, redouble, and load/save game, match, and position.
- The original Play board remains preserved.
- Settings screens remain configuration-only.
- Roll, double, move, undo, confirm, dice handling, and destination-stack convenience remain live board interactions.
- The lower-left Play area is reserved for future genuine GNUbg-backed lifecycle controls, not duplicated board actions.
- `quit` is intentionally not bound at this stage because it is not currently linked into the Android bridge and has unclear Android app-shell semantics.

See [`docs/STATUS_V0.8.11.md`](docs/STATUS_V0.8.11.md) for the detailed milestone note.

<!-- GNUBG_ANDROID_STATUS_END -->

