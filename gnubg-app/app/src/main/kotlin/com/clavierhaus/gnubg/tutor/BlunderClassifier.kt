package com.clavierhaus.gnubg.tutor

import com.clavierhaus.gnubg.Engine

/**
 * Classifies a move equity loss into a BlunderLevel using gnubg OWN
 * classifier (Engine.skill -> analysis.c Skill), NOT a reimplemented band table.
 * gnubg is the sole authority for skill thresholds (arSkillLevel, gnubg.c
 * canonical 0.16/0.08/0.04). The previous version invented 0.25/0.5/1.0/3.0x
 * multiplier bands and a fifth level; both are removed.
 */
object BlunderClassifier {

    /**
     * @param equityLoss best_equity - played_equity (>= 0; larger = worse).
     * gnubg Skill() takes the signed delta (played - best <= 0), so we negate.
     */
    fun classify(equityLoss: Float): BlunderLevel =
        BlunderLevel.fromGnubgOrdinal(Engine.skill(-equityLoss))
}
