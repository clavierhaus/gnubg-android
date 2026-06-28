# GNU Backgammon Android Settings Mapping — V0.8.13 Draft

Purpose: map every planned settings-row placeholder to a real GNUbg source pendant before exposing it in Android UI.

Rule: Settings are configuration only. Game/session actions remain in the Home Hub or active board/game screen.

## Already Android-backed

| Settings row | Android model | Android setter | GNUbg/native status |
|---|---|---|---|
| Match length | `GameSettings.matchLength` | `setMatchLength` | Used when starting matches; deeper GNUbg setter audit pending |
| Crawford rule | `GameSettings.crawford` | `setCrawford` | Used by cube legality gate; GNUbg pendant audit pending |
| Jacoby rule | `GameSettings.jacoby` | `setJacoby` | Android setting exists; GNUbg pendant audit pending |
| Automatic doubles | `GameSettings.automaticDoubles` | `setAutomaticDoubles` | Android setting exists; GNUbg pendant audit pending |
| Beavers | `GameSettings.beavers` | `setBeavers` | Android setting exists; GNUbg pendant audit pending |
| Board theme | `GameSettings.boardTheme` | `setBoardTheme` | Android-only presentation |
| Point numbers | `GameSettings.showPointNumbers` | `setShowPointNumbers` | Android-only presentation |
| Pip count | `GameSettings.showPipCount` | `setShowPipCount` | Android-only presentation using GNUbg pip count |
| Playing strength | `GameSettings.difficulty` | `setDifficulty` | Android model exists; GNUbg strength pendant audit pending |
| Tutor mode | `GameSettings.tutorMode` | `setTutorMode` | Android model exists; GNUbg tutor pendant audit pending |
| Hint | `GameSettings.hint` | `setHint` | Android model exists; GNUbg hint pendant audit pending |
| Show equity | `GameSettings.showEquity` | `setShowEquity` | Android model exists; GNUbg analysis output pendant audit pending |
| Show MWC | `GameSettings.showMWC` | `setShowMWC` | Android model exists; GNUbg analysis output pendant audit pending |
| Doubtful threshold | `GameSettings.thresholdDoubtful` | `setThresholdDoubtful` | Android model exists; GNUbg threshold pendant audit pending |
| Bad threshold | `GameSettings.thresholdBad` | `setThresholdBad` | Android model exists; GNUbg threshold pendant audit pending |
| Very bad threshold | `GameSettings.thresholdVeryBad` | `setThresholdVeryBad` | Android model exists; GNUbg threshold pendant audit pending |

## Planned placeholders requiring GNUbg pendant audit

### Game

| Planned row | Desired meaning | GNUbg source pendant | Android binding decision |
|---|---|---|---|
| Cube enabled | Enable/disable doubling cube | pending | pending |
| Maximum cube | Cap cube value | pending | pending |
| Starting side | Human/engine/automatic start preference | pending | pending |
| Dice / roll policy | Manual/automatic roll policy or RNG defaults | pending | pending |

### Board

| Planned row | Desired meaning | GNUbg source pendant | Android binding decision |
|---|---|---|---|
| Move landing hints | Long-press destination-number highlight | Android-only | already implemented; may expose switch later |
| Destination-stack helper | Android convenience move helper | Android-only | already implemented; may expose switch later |
| Dice swap gesture | Android interaction behaviour | Android-only | already implemented; may expose switch later |
| Board orientation | Human home-board orientation / side | pending | pending |
| Larger point numbers | Display accessibility | Android-only | pending |
| High contrast checkers | Display accessibility | Android-only | pending |
| Animation speed | Android UI animation | Android-only | pending |

### Engine

| Planned row | Desired meaning | GNUbg source pendant | Android binding decision |
|---|---|---|---|
| Evaluation depth | GNUbg evaluation depth/plies | pending | pending |
| Move filter | GNUbg move-filter settings | pending | pending |
| Cube decision strength | GNUbg cube analysis settings | pending | pending |
| Plies / search depth | GNUbg skill/search depth | pending | pending |
| Rollout trials | GNUbg rollout trial count | pending | pending |
| Variance reduction | GNUbg rollout option | pending | pending |
| Deterministic test mode | RNG seed / repeatable engine behaviour | pending | pending |

### Analysis / Tutor

| Planned row | Desired meaning | GNUbg source pendant | Android binding decision |
|---|---|---|---|
| Warn before bad move | Tutor warning gate | pending | pending |
| Explain move choice | GNUbg hint/explanation output | pending | pending |
| Show cube action | Display cube recommendation | pending | pending |
| Show best move | Display top GNUbg move | pending | pending |
| Show alternatives | Display candidate move list | pending | pending |

## Next pass

For every pending row:
1. identify GNUbg command/function/global option;
2. determine whether current Android native build links that symbol;
3. expose safe JNI wrapper;
4. add `Engine.kt` declaration;
5. add `GameViewModel` configuration setter/getter;
6. only then make the Settings row active.

Rows without real GNUbg pendant should remain Android-only or placeholder, explicitly labelled.
