package com.clavierhaus.gnubg.tutor

/**
 * Mirror of gnubg's cubedecision enum in engine-core/eval.h lines 192-214.
 *
 * The Kotlin ordinal of every value MUST match gnubg's int value -- the
 * facade (Engine.cubeDecision, future Engine.tutorCubeAnalyze) returns the
 * decision as a raw int, and this enum decodes it. If gnubg's enum ever
 * changes upstream, regenerate from eval.h.
 *
 * Per gnubg cube theory:
 *   *_TAKE         -- if the cube is offered, opponent should accept.
 *   *_PASS         -- if the cube is offered, opponent should drop.
 *   *_BEAVER       -- opponent should beaver (re-double, keeping the cube).
 *   TOOGOOD_*      -- doubler is far enough ahead that they should KEEP the
 *                     cube rather than offer; the suffix says what opponent
 *                     would do if doubled anyway.
 *   NODOUBLE_*     -- doubler should not double yet (not in market window);
 *                     suffix says what opponent would do if doubled anyway.
 *   REDOUBLE_/NO_REDOUBLE_/TOOGOODRE_  -- same as the above for owned-cube
 *                                          redouble decisions.
 *   *_DEADCUBE     -- match-play case where the cube cannot be turned again.
 *   OPTIONAL_*     -- borderline case; gnubg treats it as a 0-ply hint to
 *                     double in unclear positions.
 *   NOT_AVAILABLE  -- cube cannot be offered (Crawford, dead cube, etc.).
 */
enum class CubeDecision {
    DOUBLE_TAKE,            // 0
    DOUBLE_PASS,            // 1
    NODOUBLE_TAKE,          // 2
    TOOGOOD_TAKE,           // 3
    TOOGOOD_PASS,           // 4
    DOUBLE_BEAVER,          // 5
    NODOUBLE_BEAVER,        // 6
    REDOUBLE_TAKE,          // 7
    REDOUBLE_PASS,          // 8
    NO_REDOUBLE_TAKE,       // 9
    TOOGOODRE_TAKE,         // 10
    TOOGOODRE_PASS,         // 11
    NO_REDOUBLE_BEAVER,     // 12
    NODOUBLE_DEADCUBE,      // 13
    NO_REDOUBLE_DEADCUBE,   // 14
    NOT_AVAILABLE,          // 15
    OPTIONAL_DOUBLE_TAKE,   // 16
    OPTIONAL_REDOUBLE_TAKE, // 17
    OPTIONAL_DOUBLE_BEAVER, // 18
    OPTIONAL_DOUBLE_PASS,   // 19
    OPTIONAL_REDOUBLE_PASS; // 20

    companion object {
        /** Map gnubg's int return value to the enum; null on out-of-range. */
        fun fromOrdinal(ordinal: Int): CubeDecision? = values().getOrNull(ordinal)
    }
}

/** What the engine should do in response to a human double offer. */
enum class CubeAction { TAKE, DROP, NONE }

/**
 * The set of cube decisions that involve a beaver. With no beaver UI yet,
 * each of these collapses to TAKE; this set is exposed so callers can log
 * the collapse explicitly. When beaver UI is implemented, these move to a
 * new CubeAction.BEAVER and cubeDecisionAction() branches accordingly.
 */
val BEAVER_DECISIONS: Set<CubeDecision> = setOf(
    CubeDecision.DOUBLE_BEAVER,
    CubeDecision.NODOUBLE_BEAVER,
    CubeDecision.NO_REDOUBLE_BEAVER,
    CubeDecision.OPTIONAL_DOUBLE_BEAVER,
)

/**
 * Map a cube decision (from a human-offered double) to the engine's action.
 *
 *   *_TAKE family, *_DEADCUBE family, *_BEAVER family -> TAKE
 *     (beavers currently collapse to take; deadcube cases must accept since
 *     the cube cannot turn further in match play.)
 *   *_PASS family                                      -> DROP
 *   NOT_AVAILABLE                                      -> NONE
 *
 * Drives the when-block in GameViewModel.offerDouble.
 */
fun cubeDecisionAction(cd: CubeDecision): CubeAction = when (cd) {
    CubeDecision.DOUBLE_TAKE,
    CubeDecision.NODOUBLE_TAKE,
    CubeDecision.TOOGOOD_TAKE,
    CubeDecision.REDOUBLE_TAKE,
    CubeDecision.NO_REDOUBLE_TAKE,
    CubeDecision.TOOGOODRE_TAKE,
    CubeDecision.NODOUBLE_DEADCUBE,
    CubeDecision.NO_REDOUBLE_DEADCUBE,
    CubeDecision.OPTIONAL_DOUBLE_TAKE,
    CubeDecision.OPTIONAL_REDOUBLE_TAKE,
    CubeDecision.DOUBLE_BEAVER,
    CubeDecision.NODOUBLE_BEAVER,
    CubeDecision.NO_REDOUBLE_BEAVER,
    CubeDecision.OPTIONAL_DOUBLE_BEAVER -> CubeAction.TAKE

    CubeDecision.DOUBLE_PASS,
    CubeDecision.TOOGOOD_PASS,
    CubeDecision.REDOUBLE_PASS,
    CubeDecision.TOOGOODRE_PASS,
    CubeDecision.OPTIONAL_DOUBLE_PASS,
    CubeDecision.OPTIONAL_REDOUBLE_PASS -> CubeAction.DROP

    CubeDecision.NOT_AVAILABLE -> CubeAction.NONE
}
