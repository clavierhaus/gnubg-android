# Coach cube coaching (M4) -- the prerequisite for match lengths > 1

Status: PLAN (2026-07-12). Ordered BEFORE the Coach setup screen (maintainer):
a setup screen offering 3/5/7-point matches would promise coaching the coach
cannot yet give, because at length 1 the cube is dead by the rules and the
whole verdict path only judges chequer play. Match length > 1 makes the cube
live; the coach must teach cube decisions before it may offer those lengths.

## What already exists (verified, no new analysis needed)

gnubg_mobile_cube_decision(board, out[18], &cd) ALREADY runs gnubg's own
GeneralCubeDecisionENoLocking + FindCubeDecision and returns:
  out[0..6]   no-double eval outputs (win/gammon/bg probabilities...)
  out[7..13]  double eval outputs
  out[14..17] arDouble = OPTIMAL, NODOUBLE, TAKE, DROP  (cubeful equities)
  cd          the classified cubedecision enum (DOUBLE_TAKE, DOUBLE_PASS,
              NODOUBLE_TAKE, TOOGOOD_TAKE/PASS, OPTIONAL_*, *_DEADCUBE, ...)
gnubg_mobile_cube_recommendation(cd, text) gives gnubg's OWN wording for cd.
So the analysis is done; M4 is a VERDICT wrapper + presentation, exactly as the
chequer coach was a wrapper over FindnSaveBestMoves.

## The two cube decision points the coach must judge

A cube turn has two distinct human decisions, and the coach owns both:

  1. DOUBLE-OR-NOT (start of the human's turn, before rolling). The player may
     double or roll. gnubg's cd says what is right: any DOUBLE_* / OPTIONAL_* =
     doubling is correct (or at least not wrong); NODOUBLE_* = rolling is right;
     TOOGOOD_* = too good to double (play on). The coach compares the player's
     action (did they double? did they roll?) to cd and to the equity margin in
     arDouble, and issues a verdict in the same three tiers as chequer play:
     right / minor / blunder, with the equity cost.

  2. TAKE-OR-DROP (when GNU doubles the human). cd's take/pass component says
     which is right; arDouble[TAKE] vs arDouble[DROP] gives the margin. Same
     verdict tiers.

## Engine verbs to add (facade, gnubg does the judging)

  gnubg_mobile_coach_cube_verdict(board, cube, cube_owner, ..., int action,
                                  int out[N])
    action encodes what the human DID (0 rolled/no-double, 1 doubled, 2 took,
    3 dropped). Internally calls the SAME cube_decision path, then scores the
    human action against cd + arDouble. Output layout parallels the chequer
    verdict: [rank-equivalent]=is-best flag, equities for chosen vs best,
    skill band via a cube-equity threshold, cd, and the recommendation text
    handle. NO new gnubg reasoning -- FindCubeDecision already decided.

## Flow / UI (mirrors the chequer coach exactly)

  - When the human faces a cube decision (their own turn: double-or-not; or
    GNU has doubled: take-or-drop), the board enters the same COACH_REVIEW hold
    AFTER the human acts: judge, show the verdict, GNU waits, then continue.
  - The panel's three tiers carry over verbatim, worded for the cube:
    "Correct double." / "Doubling was premature (-0.0xx)." / "Pass! Taking
    here loses -0.1xx." The "Why" stub is where the cube reason line will go.
  - No toggle explorer for cube (there is no board move to show); instead the
    panel shows the equities: your action vs gnubg's, take-point context.

## Cube skill thresholds

Reuse gnubg's own skill bands the way the chequer path reuses arSkillLevel;
cube errors are measured in the same equity currency (arDouble margins), so
the doubtful/bad/very-bad cutoffs apply directly. No invented thresholds.

## Order within M4

  1. coach_cube_verdict verb (C) + JNI + Engine.kt external, syntax-checked.
  2. VM: detect the two cube decision points in the coach flow; hold; verdict.
  3. Panel: cube verdict tiers (no explorer); reuse the review chrome.
  4. THEN the setup screen (length + strength) -- lengths > 1 now honest.
  5. Field-test cube verdicts at 3-point before enabling longer matches.

## Deferred (not M4)

MWC display (needs eq2mwc exposure); beavers/raccoons (money play, not match);
the cube REASON phrase (insight layer, docs/COMPANION.md, after the chequer
reason line is proven).

## Regression + correction (2026-07-12)

The first cube flow intercepted the ordinary ROLL to coach the no-double
decision (action 0): rollDice diverted to a cube verdict every turn, because
canDouble() is true at the start of most turns. This held a CUBE verdict before
the player could move, and its glance masked the chequer verdict (panel shows
cube-if-present) -- the reported "checker evaluation disabled, only cube flags"
regression. It also risked a double hold (cube then chequer) per turn.

Correction: the no-double roll interception is REMOVED. Cube coaching now
triggers ONLY on cube actions the player explicitly initiates -- offerDouble
(action 1), acceptDouble (2), dropDouble (3) -- which are genuine, occasional,
and non-intrusive. confirm()'s chequer branch clears any stale cube glance so a
chequer move always owns the panel. Chequer evaluation flows normally again.

DEFERRED (not lost): coaching the no-double / missed-double decision. The right
design is NON-blocking -- judge it ALONGSIDE the chequer verdict (or as a
passive note), never as a hold that pre-empts the move. It belongs with the
insight layer, where a "you could have doubled here" line sits beside the
chequer reason, not as a separate forced review. Tracked here so it returns by
design rather than by re-adding the interception.
