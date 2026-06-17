# Settings UX Blueprint

Version: 0.1
Target milestone: post-0.8.9 planning
Status: private design and implementation blueprint

## 1. Purpose

This document collects the first design ideas for the future settings interface of GNU Backgammon by clavierhaus.at.

The Android application is no longer treated as a proof of concept. It is the first serious mobile surface for GNU Backgammon. The settings interface should therefore not merely expose a few preferences. It should become the structured gateway to the depth of GNUbg, while remaining approachable on a touch device.

The goal is to map the relevant functionality of GNU Backgammon into an attractive, readable, landscape-only mobile interface.

## 2. Design Premise

The app is strictly landscape. Portrait mode is not a target.

This gives enough horizontal screen estate for five or six top-level tabs. The settings interface can therefore use a wide, immediately readable tab strip rather than hiding everything behind nested menus.

The current inspiration is the Blackmagic Pocket Cinema Camera interface: direct, dense, professional, immediately usable, and capable of exposing complex functionality on a small screen without feeling like a desktop application.

GNU Backgammon should not copy the GNUbg desktop interface. The task is not to reproduce menus. The task is to translate GNUbg’s power into a modern touch interface.

## 3. Navigation Model

The main settings screen uses horizontal top-level tabs.

A first working set of tabs:

- Game
- Board
- Engine
- Analysis
- Training
- Expert

Five tabs may be enough initially. Six tabs are acceptable in landscape if spacing and typography remain clear.

Each tab has two possible horizontal pages:

1. Main page
2. Fine-tuning page

The main page contains the most relevant controls in that field.

The second page contains related refinements that are still part of normal usage. It is only one swipe away and should not feel hidden.

Anything beyond this two-page structure belongs in Expert mode or Advanced settings.

## 4. Vertical Scrolling

Unlike the Blackmagic camera interface, the app has the luxury of vertical scrolling.

This means each tab can contain more than one card or section, as long as the first visible screen remains clean.

Scrolling should be used for additional related controls, not for dumping unstructured complexity onto the user.

Recommended structure per tab:

- one prominent section at the top
- one or two secondary sections below
- clear section headings
- compact rows
- short explanatory subtitles where useful

## 5. Mockup Mode vs Implementation Mode

During mockup work, placeholder settings are allowed.

Lorem ipsum, fake switches, provisional labels, and non-functional controls are acceptable if they help design the structure, rhythm, grouping, and density of the interface.

During implementation, every setting must eventually be classified as one of:

- connected to real GNUbg functionality
- UI-only behaviour
- planned but not yet implemented
- placeholder/mockup only

The mockup should make exploration easy. The implementation should make truth explicit.

## 6. Exposure Levels

Settings should be organised by user exposure level.

### Essential

Visible immediately. Safe, obvious, and understandable to most users.

Examples:

- match length
- opponent strength
- board theme
- pip count
- point numbers
- tutor mode
- hints

### Advanced

Still user-facing, but requires some understanding of backgammon or GNUbg.

Examples:

- cube options
- Jacoby rule
- beavers
- equity display
- MWC display
- mistake thresholds
- evaluation depth presets

### Expert

Technical, dense, or potentially confusing. Not part of the normal first experience.

Examples:

- detailed evaluation parameters
- rollout configuration
- match equity table options
- diagnostic information
- engine state inspection
- import/export/debug tools

## 7. Proposed Tab Structure

### Game

Purpose: match rules, scoring, and play structure.

Main page:

- Match length
- Crawford rule
- Jacoby rule
- Beavers
- Automatic doubles

Fine-tuning page:

- Match scoring details
- Cube rule explanations
- Score display options
- Game-start behaviour
- Resume/new match behaviour

Expert candidates:

- tournament presets
- custom rule sets
- match equity table selection
- debug display for score/cube state

### Board

Purpose: appearance and board interaction.

Main page:

- Board theme
- Checker style
- Dice style
- Point numbers
- Pip count
- Move animation on/off

Fine-tuning page:

- checker size
- dice size
- highlight intensity
- legal-move markers
- tap sensitivity
- board contrast
- colour accessibility presets

Expert candidates:

- layout diagnostics
- board-relative coordinate debugging
- device aspect ratio test modes
- custom theme import/export

### Engine

Purpose: opponent behaviour and GNUbg strength.

Main page:

- Opponent strength
  - Beginner
  - Advanced
  - Master
- Tutor mode
- Hint mode
- Engine move speed
- Cube decision display

Fine-tuning page:

- evaluation depth preset
- checker-play strength preset
- cube strength preset
- resign behaviour
- delay before engine move
- human-readable engine explanation mode

Expert candidates:

- exact evaluation settings
- rollout settings
- neural net/weights diagnostics
- engine command log
- GNUbg state inspection

### Analysis

Purpose: understanding moves, mistakes, equity, and match decisions.

Main page:

- Show equity
- Show match winning chances
- Classify mistakes
- Show best move after error
- Save blunders

Fine-tuning page:

- doubtful/bad/very bad thresholds
- display equity as points or percent
- cube analysis display
- move comparison view
- analysis after match
- analysis after every move

Expert candidates:

- rollout depth
- equities table
- cubeful/cubeless comparison
- detailed move list export
- position ID and match ID tools

### Training

Purpose: deliberate improvement and repeatable exercises.

Main page:

- Replay saved blunders
- Daily position review
- Checker-play training
- Cube training
- Endgame training

Fine-tuning page:

- number of positions per session
- repeat interval for mistakes
- difficulty of generated positions
- include cube decisions
- include bearoff positions
- training statistics

Expert candidates:

- position database management
- import/export training sets
- GNUbg-generated position filters
- custom training queues

### Expert

Purpose: expose the deep GNUbg machinery without compromising the approachable interface.

Main page:

- Engine diagnostics
- Evaluation settings
- Rollout settings
- Import/export
- Logs
- Build/version information

Fine-tuning page:

- detailed native engine state
- command bridge diagnostics
- JNI status
- weight file information
- device performance test
- developer toggles

Expert mode should be powerful but visually distinct. It should not pollute the normal tabs.

## 8. Visual Direction

The settings interface should feel professional, not playful.

Current visual direction:

- landscape-only
- wide tab bar
- large clear headings
- dense but readable cards
- strong colour blocks
- immediate touch targets
- short explanatory subtitles
- no desktop-style nested menus

The visual language can be inspired by professional camera interfaces: compact, confident, and designed for repeated use.

However, the app should eventually receive its own professional design language rather than remain a literal imitation.

## 9. Implementation Strategy

Implementation should happen step by step.

### Phase 1: Mockup population

Populate all tabs with plausible rows, including placeholders.

Goal:

- establish layout
- test density
- test swipe behaviour
- test readability
- test scrolling
- test first impression

### Phase 2: Classification

Mark each setting internally as:

- implemented
- UI-only
- GNUbg mapping needed
- placeholder
- expert-only

### Phase 3: Wiring

Connect implemented controls to real state.

Do not fake engine behaviour in final implementation.

### Phase 4: GNUbg mapping

For each GNUbg feature:

- identify the native engine call or required bridge work
- decide whether the setting belongs in normal UI or Expert
- expose it through Kotlin state
- keep GNUbg authoritative where game logic is concerned

### Phase 5: Design pass

Replace the current improvised visual design with professional artwork and consistent interaction patterns.

The existing Kotlin structure and engine bridge should remain usable; artwork and layout polish should not require rethinking the engine integration.

## 10. iOS Implications

The Android app is the first serious mobile target.

The settings architecture should be designed with iOS in mind from the beginning:

- avoid Android-only conceptual patterns where possible
- keep engine-facing state clean
- separate UI presentation from engine authority
- document which settings map to native GNUbg state
- keep terminology consistent across platforms

An iOS developer should not be asked to build a backgammon engine or a clone. The iOS task is to build a native Apple front-end around the same GNUbg foundation and product concept.

## 11. Guiding Principles

- The app is landscape-only.
- Use the available horizontal space.
- Five or six top-level tabs are acceptable.
- Each tab may have two pages: main and fine-tuning.
- Anything beyond two pages goes to Expert.
- Mockups may contain placeholders.
- Final implementation must distinguish real, UI-only, planned, and placeholder controls.
- GNUbg remains the authority for rules, legality, cube state, evaluation, and analysis.
- Kotlin provides touch interaction, state presentation, and mobile UX.
- The GUI should make GNUbg approachable without making it shallow.
- The goal is not to imitate GNUbg desktop.
- The goal is to make GNU Backgammon live properly on modern mobile platforms.
