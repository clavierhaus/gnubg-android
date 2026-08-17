/*
 * Copyright (C) 2000-2003 Joern Thyssen <joern@thyssen.nu>
 * Copyright (C) 2001-2021 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * $Id: analysis.h,v 1.51 2019/11/05 20:49:22 plm Exp $
 */

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "list.h"
#include "gnubg-types.h"

typedef enum {
    LUCK_VERYBAD, LUCK_BAD, LUCK_NONE, LUCK_GOOD, LUCK_VERYGOOD
} lucktype;

typedef enum {
    SKILL_VERYBAD,
    SKILL_BAD,
    SKILL_DOUBTFUL,
    SKILL_NONE
} skilltype;

#define badSkill(st)  ((st) < SKILL_NONE)

#define N_SKILLS ((int)SKILL_NONE + 1)
#define N_LUCKS ((int)LUCK_VERYGOOD + 1)

typedef struct {
    int fMoves, fCube, fDice;   /* which statistics have been computed? */

    int anUnforcedMoves[2];
    int anTotalMoves[2];

    int anTotalCube[2];
    int anCloseCube[2];
    int anDouble[2];
    int anTake[2];
    int anPass[2];

    int anMoves[2][N_SKILLS];

    int anLuck[2][N_LUCKS];

    int anCubeMissedDoubleDP[2];
    int anCubeMissedDoubleTG[2];
    int anCubeWrongDoubleDP[2];
    int anCubeWrongDoubleTG[2];
    int anCubeWrongTake[2];
    int anCubeWrongPass[2];

    /* all accumulated errors have dimension 2x2 
     *  - first dimension is player
     *  - second dimension is error rate in:
     *  - EMG and MWC for match play
     *  - Normalized and unnormalized equity for money games
     */

    float arErrorCheckerplay[2][2];
    float arErrorMissedDoubleDP[2][2];
    float arErrorMissedDoubleTG[2][2];
    float arErrorWrongDoubleDP[2][2];
    float arErrorWrongDoubleTG[2][2];
    float arErrorWrongTake[2][2];
    float arErrorWrongPass[2][2];
    float arLuck[2][2];

    /* luck adjusted result */

    float arActualResult[2];
    float arLuckAdj[2];
    float arVarianceActual[2];
    float arVarianceLuckAdj[2];
    int nGames;

} statcontext;

typedef enum {
    RAT_AWFUL,
    RAT_BEGINNER, RAT_CASUAL_PLAYER, RAT_INTERMEDIATE, RAT_ADVANCED,
    RAT_EXPERT, RAT_WORLD_CLASS, RAT_SUPERNATURAL, RAT_UNDEFINED
} ratingtype;
#define N_RATINGS ((int)RAT_UNDEFINED + 1)

extern const char *aszRating[N_RATINGS];
extern const char *aszLuckRating[N_LUCKS];

extern int afAnalysePlayers[2];

extern ratingtype GetRating(const float rError);
extern void IniStatcontext(statcontext * psc);
extern void AddStatcontext(const statcontext * pscA, statcontext * pscB);

#define STATCONTEXT_MAXSIZE 8192

extern void DumpStatcontext(char *szOutput, const statcontext * psc, const char *player, const char *op, int nMatchTo);

extern void updateStatisticsGame(const listOLD * plGame);

extern void updateStatisticsMatch(listOLD * plMatch);

extern lucktype getLuckRating(float rLuck);

extern float relativeFibsRating(float r, int n);

extern float absoluteFibsRating(const float rChequer, const float rCube, const int n, const float rOffset);

extern float absoluteFibsRatingChequer(const float rChequer, const int n);

extern float absoluteFibsRatingCube(const float rCube, const int n);


#define CHEQUERPLAY  0
#define CUBEDECISION 1
#define COMBINED     2

#define TOTAL        0
#define PERMOVE      1

#define PLAYER_0     0
#define PLAYER_1     1

#define NORMALISED   0
#define UNNORMALISED 1

extern void getMWCFromError(const statcontext * psc, float aaaar[3][2][2][2]);

extern skilltype Skill(float r);

extern int MatchAnalysed(void);
extern float LuckAnalysis(const TanBoard anBoard, int n0, int n1, matchstate * pms);
extern lucktype Luck(float r);

#endif
