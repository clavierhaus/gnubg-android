package com.clavierhaus.gnubg.tutor

/**
 * Move-error severity, mirroring gnubg own skilltype (engine-core/analysis.h).
 * gnubg classifies an equity loss into exactly these four levels via Skill()
 * (analysis.c:287) using the arSkillLevel thresholds (gnubg.c canonical
 * 0.16/0.08/0.04). The ordinals match gnubg enum 1:1 so the engine result
 * maps directly: 0=VERY_BAD, 1=BAD, 2=DOUBTFUL, 3=NONE.
 *
 * Labels follow gnubg aszSkillType (play.c:85): "very bad", "bad", "doubtful".
 */
enum class BlunderLevel(val gnubgLabel: String?) {
    VERY_BAD("very bad"),  // 0  SKILL_VERYBAD
    BAD("bad"),            // 1  SKILL_BAD
    DOUBTFUL("doubtful"),  // 2  SKILL_DOUBTFUL
    NONE(null);            // 3  SKILL_NONE

    companion object {
        /** Map a gnubg skilltype ordinal (from Engine.skill) to a BlunderLevel. */
        fun fromGnubgOrdinal(ordinal: Int): BlunderLevel =
            entries.getOrElse(ordinal) { NONE }
    }
}
