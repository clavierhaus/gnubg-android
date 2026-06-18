# Tutor Mode Implementation Plan

## 1. Purpose

Tutor Mode is the central mobile-native teaching layer for GNU
Backgammon Android.

The aim is not to reproduce the desktop GNU Backgammon tutor screen.
The aim is to make GNUbg's judgement visible, tactile, and useful on a
phone or tablet.

The desktop program is already excellent at calculation. The mobile app
must become excellent at communication.

Tutor Mode should answer three questions for the player:

1. What did GNUbg think of my decision?
2. What should I have seen on the board?
3. How can I recognise this pattern next time?

The board remains the primary explanation surface. Numbers and prose are
secondary layers.

## 2. Core Design Rule

Every tutor explanation must be grounded.

A tutor hint may only claim a reason when that reason can be traced to:

- GNUbg evaluation output;
- deterministic board features;
- a clear delta between the user's move and GNUbg's preferred move;
- cube/match-score facts exposed by GNUbg.

If no reliable reason can be identified, the app must show only the
objective comparison:

    GNUbg prefers another move by 0.062.
    Show best move?

It is better to say less than to teach a false lesson.

## 3. Non-Negotiable Architecture Rule

All new Tutor Mode and gameplay-support logic must be de-Androidified.

Android code may own:

- rendering;
- Compose UI state collection;
- user interaction dispatch;
- Android lifecycle;
- local preference persistence;
- temporary ViewModel orchestration.

Android code must not own:

- backgammon rules;
- tutor reasoning;
- GNUbg command semantics;
- move-quality interpretation;
- shot-count analysis;
- point-making analysis;
- cube-decision interpretation;
- Try Again game-state semantics.

Those responsibilities belong either in:

- the platform-neutral Kotlin tutor/game layer; or
- the GNUbg mobile facade/native layer when the logic belongs beside GNUbg.

The ViewModel may capture and expose state, but it must not become the
Tutor Mode brain.


## 3. Architecture Overview

Tutor Mode is split into four layers.

### 3.1 Engine / Facade Layer

Location:

    jni-bridge/include/gnubg_mobile.h
    jni-bridge/src/gnubg_mobile.c
    jni-bridge/src/native-lib.c

Responsibility:

- ask GNUbg for move evaluation;
- ask GNUbg for cube evaluation;
- expose stable mobile-facing tutor APIs;
- never expose JNI details through the platform-neutral facade;
- never generate human prose.

The engine layer returns facts.

### 3.2 Kotlin Tutor Facts Layer

Location target:

    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/tutor/

Responsibility:

- preserve pre-move board state;
- preserve user move;
- compare user move with best move;
- compute board feature deltas;
- classify severity;
- expose immutable tutor data to the UI.

This layer translates engine facts into structured app facts.

### 3.3 Tutor Decision Layer

Responsibility:

- select the main tutor theme;
- decide whether to interrupt;
- choose conservative message templates;
- enforce the grounding rule;
- avoid over-explaining.

This layer creates `TutorHint` objects, not free-form analysis.

### 3.4 Compose UI Layer

Responsibility:

- show coach cards;
- draw arrows and highlights;
- show shot badges;
- provide Try Again / Show Best Move / More Detail;
- preserve flow and board visibility.

The UI renders tutor state. It does not decide backgammon meaning.

## 4. Settings Subsection

Tutor Mode needs a dedicated settings subsection, most likely under
Options / Settings.

### 4.1 Top-Level Tutor Setting

Setting:

    Tutor Mode

Values:

    Off
    Gentle
    Serious
    Classic

Implementation note:

These should not become separate code paths. They are presets for the
same tutor system.

### 4.2 Interruption Threshold

Setting:

    Show tutor feedback

Values:

    Every decision
    Inaccuracies and worse
    Mistakes and worse
    Blunders only
    End of game only

Suggested equity thresholds:

    Inaccuracy: >= 0.030
    Mistake:    >= 0.060
    Blunder:    >= 0.120

These should be adjustable later, but presets are enough for the first
implementation.

### 4.3 Try Again

Setting:

    Offer Try Again

Values:

    On
    Off

Default:

    On

Purpose:

This controls whether Tutor Mode offers to restore the pre-move position
after a significant mistake.

### 4.4 Visual Hints

Setting:

    Board annotations

Values:

    Off
    Best move only
    User vs best
    Full visual explanation

Default:

    User vs best

### 4.5 Numbers

Setting:

    Equity detail

Values:

    Hidden by default
    Show loss only
    Show move equities
    Classic detail

Default:

    Show loss only

### 4.6 Cube Tutor

Setting:

    Cube tutor

Values:

    Off
    Major cube errors only
    All cube decisions

Default:

    Major cube errors only

### 4.7 Rollouts

Setting:

    Rollout access

Values:

    Disabled
    Advanced only
    Classic mode

Default:

    Advanced only

Rollouts must be bounded, cancellable, and clearly labelled as deeper
analysis.

## 5. Kotlin Data Model

Initial package:

    com.clavierhaus.gnubg.tutor

### 5.1 TutorSeverity

    enum class TutorSeverity {
        GOOD,
        INACCURACY,
        MISTAKE,
        BLUNDER
    }

### 5.2 TutorTheme

Start with only reliable themes:

    enum class TutorTheme {
        SAFETY,
        SHOT_COUNT,
        POINT_MAKING,
        EXPOSURE,
        RACE,
        CUBE_TAKE_PASS,
        GAMMON_DANGER
    }

Do not add fuzzy themes until they can be computed reliably.

### 5.3 TutorMove

Fields:

    move: String
    equity: Float
    rank: Int

### 5.4 TutorEvaluation

Fields:

    userMove: String
    bestMove: String
    userEquity: Float
    bestEquity: Float
    equityLoss: Float
    severity: TutorSeverity
    topMoves: List<TutorMove>

Source:

GNUbg evaluation.

### 5.5 TutorFeatureDelta

Fields:

    userShotsLeft: Int?
    bestShotsLeft: Int?
    userBlots: Int
    bestBlots: Int
    userMadePoints: List<Int>
    bestMadePoints: List<Int>
    userPipCount: Int
    bestPipCount: Int
    keyPointMadeByBest: Int?

Source:

Deterministic Kotlin board analysis.

### 5.6 TutorHint

Fields:

    severity: TutorSeverity
    mainTheme: TutorTheme?
    headline: String
    shortExplanation: String?
    measurableFacts: List<String>
    userMove: String
    bestMove: String
    equityLoss: Float
    allowTryAgain: Boolean
    allowShowBestMove: Boolean
    allowMoreDetail: Boolean

Source:

Tutor decision layer.

## 6. First Native / Facade Requirements

Before UI work, inspect existing evaluation exposure.

Questions to answer:

- Can Kotlin already request the best move?
- Can Kotlin already get equity for the user's move?
- Can Kotlin already get top N moves?
- Can Kotlin evaluate a move without committing it?
- Can Kotlin restore the exact pre-move state?
- Can Kotlin obtain cube/take/drop evaluation facts?

Likely first facade API:

    gnubg_mobile_get_best_move(...)

or:

    gnubg_mobile_evaluate_move(...)

The first API should be narrow. It should support the MVP only.

Avoid building a full analysis API before the UI loop is proven.

## 7. MVP Flow

### 7.1 Before Move Commit

When the player has selected a legal move and taps Commit:

- preserve pre-move board;
- preserve dice;
- preserve selected move string;
- commit move normally through the existing engine path.

### 7.2 After Commit

Tutor layer asks GNUbg:

- what was the best move?
- what was the user's move equity?
- what was the best move equity?
- what is the equity loss?

If loss is below threshold:

- no interruption;
- optionally show a small positive acknowledgement.

If loss exceeds threshold:

- create `TutorEvaluation`;
- compute `TutorFeatureDelta`;
- create `TutorHint`;
- show Coach Card.

### 7.3 Coach Card MVP

Collapsed state:

    Mistake: -0.084
    Better: 13/8 6/5

Buttons:

    Show Best Move
    Try Again
    More

### 7.4 Try Again

When tapped:

- restore pre-move board;
- restore dice;
- clear current selection;
- allow the user to move again;
- optionally keep a subtle hint active;
- do not reveal the answer unless requested.

This is the key learning loop.

### 7.5 Show Best Move

When tapped:

- draw best move arrows on the board;
- optionally toggle user move vs best move;
- keep board visible.

## 8. Board Annotation MVP

Initial annotations:

- user move arrows;
- best move arrows;
- highlighted destination points;
- optional exposed blot markers.

Second stage:

- shot-count badges;
- hit-source highlights;
- point-making highlights.

Annotations must be temporary or dismissible.

The board must never become permanently cluttered.

## 9. Shot Count Feature

This is the first high-impact visual teaching feature.

For both user move and best move:

- apply the move to a copy of the board;
- compute opponent hitting numbers;
- count total direct shots;
- identify target blots;
- identify dice values that hit.

Tutor message example:

    Main reason: safety.
    Your move leaves 17 shots.
    GNUbg's move leaves 6.

This is deterministic and safe.

## 10. Point-Making Feature

Detect whether GNUbg's best move makes a key point and the user's move
does not.

Initial key points:

- own 5-point;
- own 4-point;
- bar point;
- opponent 5-point anchor;
- advanced anchor;
- prime extension.

Tutor message example:

    Main reason: point-making.
    GNUbg wants to make your 5-point.

Only show this when the board delta clearly supports it.

## 11. Cube Tutor MVP

Cube Tutor should be a separate track.

Initial facts:

- correct action;
- user action;
- equity loss;
- take/drop result;
- win chance if available;
- gammon danger if available;
- match score context.

Initial UI:

- compact cube card;
- take/pass or double/no-double verdict;
- optional gauge later.

No complex prose until the cube facts are reliable.

## 12. Compose UI States

Suggested state model:

    sealed interface TutorUiState {
        data object Hidden : TutorUiState
        data class CoachCard(val hint: TutorHint) : TutorUiState
        data class CompareMoves(val hint: TutorHint) : TutorUiState
        data class TryAgain(val hint: TutorHint) : TutorUiState
        data class Detail(val evaluation: TutorEvaluation) : TutorUiState
    }

The board should receive a separate annotation state:

    data class BoardTutorAnnotations(
        val userMoveArrows: List<MoveArrow>,
        val bestMoveArrows: List<MoveArrow>,
        val highlightedPoints: List<Int>,
        val shotBadges: List<ShotBadge>
    )

## 13. Commit-Sized Implementation Phases

### Phase 1: Documentation and Settings Skeleton

- add Tutor Mode settings subsection;
- store settings in preferences;
- no engine behavior change.

### Phase 2: Tutor State Skeleton

- add tutor package;
- add data classes;
- add empty TutorUiState to GameViewModel;
- no UI interruption yet.

### Phase 3: Pre-Move Snapshot

- introduce a platform-neutral `TutorPreMoveSnapshot` model;
- introduce a platform-neutral `TutorMoveContext` model;
- capture board, dice, move string, match facts, and settings before
  committing a human move;
- expose the captured context through inert tutor state;
- do not implement Try Again restoration yet;
- do not place restoration semantics in Android code;
- test no behavior change.

### Phase 4: Static Coach Card Prototype

- show a manually generated tutor card after move commit behind a debug flag;
- prove layout and dismissal;
- no GNUbg evaluation yet.

### Phase 5: Best Move Evaluation

- expose minimal native/facade evaluation function;
- compare user move vs best move;
- show severity and best move.

### Phase 6: Try Again

- add a neutral restore contract outside Android UI code;
- route restoration through the platform-neutral game/tutor layer or
  mobile facade;
- let Android dispatch only the user's Try Again action;
- ensure move selection resets correctly;
- test normal move, bar entry, doubles, no-legal-move cases.

### Phase 7: Board Arrows

- draw user and best move arrows;
- add toggle in Coach Card.

### Phase 8: Shot Count

- compute shot count deltas;
- show shot-count message;
- draw shot badges.

### Phase 9: Point-Making

- compute key-point deltas;
- add conservative point-making template.

### Phase 10: Cube Tutor MVP

- evaluate cube action;
- show cube tutor card;
- no advanced rollout UI yet.

## 14. Test Matrix

Minimum manual tests for each phase:

- new 3-point match;
- normal roll and move;
- doubles roll;
- bar re-entry;
- no legal move;
- resignation and next game;
- match over state;
- cube offer;
- take/drop if reachable;
- return to hub and resume.

For Tutor Mode specifically:

- Tutor Off produces no interruptions.
- Gentle mode interrupts only above threshold.
- Try Again restores board and dice.
- Show Best Move does not commit anything.
- Dismiss tutor continues normal play.
- Tutor state clears on new match.
- Tutor state clears on return to hub.

## 15. Non-Goals for MVP

Do not start with:

- full desktop move tables as primary UI;
- rollout workflow;
- broad concept taxonomy;
- LLM-generated explanations;
- long prose;
- post-game statistics;
- training curriculum;
- cloud sync;
- achievements.

These belong later.

## 16. Success Definition

The first successful Tutor Mode release should make this interaction feel
excellent:

    I moved.
    GNUbg judged the move.
    The app showed me what I missed directly on the board.
    I could try again.

If that loop feels good, the product has crossed the line from a mobile
port into a mobile-native tutor.
