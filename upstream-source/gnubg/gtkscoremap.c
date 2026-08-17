/*
 * Copyright (C) 2020 Aaron Tikuisis <Aaron.Tikuisis@uottawa.ca>
 * Copyright (C) 2020-2023 Isaac Keslassy <keslassy@gmail.com>
 * Copyright (C) 2022-2023 the AUTHORS
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
 * $Id: gtkscoremap.c,v 1.30 2023/06/11 21:26:03 plm Exp $
 */


/***************************************************      Explanations:       *************************************
      The main function is GTKShowScoreMap.
            1. It builds the window, including the score map, gauge and radio buttons, by calling BuildTableContainer()
                    [which calls InitQuadrant() for each score (i.e. in each scoremap square=quadrant) to initialize
                    the drawing tools].
                    It also initializes variables.
            2. New: It calls CalcEquities(), which: 
                - initializes the match state in each quadrant, by filling
                    psm->aaQuadrantData[i][j].ci for each i,j
                - for each score i,j, also calls CalcQuadrantEquities() to find
                    the equity of each decision and therefore the best decision as well,
                    and uses it to set the text for the corresponding quadrant (the best decision is stored in
                    psm->aaQuadrantData[i][j].decisionString). This text is displayed in step 5 below.
                    In the move scoremap, FindMostFrequentMoves() finds the top-k most frequent distinct best moves
                    and assigns them distinct colors, as well as English descriptions (in the "alpha version" where
                    English description is allowed).
            4. It calls  UpdateScoreMapVisual(), which for each quadrant runs ColourQuadrant(), updating:
                    (a) The color of each quadrant (for cube scoremap: using FindColour1() or FindColour2(),
                    depending on the coloring scheme; and for move scoremap, using FindMoveColour()). For instance,
                    in the move scoremap, FindMoveColour() checks whether the current best move is equal to one of
                    the TOP_K most common moves, and updates the quadrant and border colors according to the associated
                    color (e.g., if it's red, it sets the border in strong red, and the quadrant in weak red, based
                    on the equity difference with the 2nd best move).
                    (b) The equity text
                    (c) The black border for the square with a similar score to the current one
                    (d) The hover text of each quadrant (in SetHoverText())
                    Next, UpdateScoreMapVisual() sets:
                    (e) the row/col score labels
                    Then, for the cube scoremap, it runs DrawGauge() which updates
                    (f) the gauge,
                    and also it adds labels to the gauge.
            5. Careful! Finally, at the end, ColourQuadrant() asks to redraw, which implicitly calls DrawQuadrant().
                    This function updates the decision text in each square.
                    It also adds the equity text if the corresponding option is set.
    Radio buttons:
     - Changing the eval ply launches the ScoreMapPlyToggled function that also calls the functions in steps 2-5 above.
     - Changing the colour-by (ND/D or T/P) launches the ColourByToggled function that only calls UpdateScoreMapVisual
         (no need for new equity values); 
     - and so on for the different radio buttons.
*/

/* 
01/2023: Isaac Keslassy:
- new Settings>Options>ScoreMap panel where all default options can be configured
    -> set as a frame
    -> later moved to Settings>Analysis>ScoreMap
- added Analyse > "Scoremap (move decision)" to the existing cube-ScoreMap entry
- added ply display in hover of cube ScoreMap, and aligned displays
- addressed the issues that occur when stopping computation in the middle, and 
    changed the order in which the ScoreMap displays, by using expanding squares
- also changed the order in which operations are launched: instead of computing the 
    cube info at all scores in one batch at first, which means a longer time before the 
    progress-bar evaluations even start, thus troubling the user, now compute the info 
    for each score just before evaluating it
- also recomputing the cube info only on needed squares upon table scaling
- also not recomputing the money-play equity unless needed
- (added then canceled a smooth table scaling where we see the previous computation results
    and colors while increasing the table size; the additional time to redisplay the previous 
    computations is not worth it) 
- removed label-by and layout radio buttons; they will only be configured in the 
    default settings panel
- corrected "similar score" inaccuracy when scaling the table up
- solved several bugs, including wrongly freeing memory allocations
- checked that it works in both Windows 10 and Ubuntu 22

12/2022: Isaac Keslassy: a few changes including:
- new help button to obtain explanations
- new "scoreless" (unlimited) option of Money without Jacoby
- new cube equity display options: absolute, or relative ND-D, or relative D/P-D/T
- new tooltip hover text to explain each option
- added effective ply mention in hover text (because gnubg sometimes returns a 2-ply eval in a 3-ply mode if 
    the eval difference between the best and 2nd-best move is large)
- default smaller quadrant size to fit a smaller screen
- default analysis: 2-ply
- new way to decide the table size, i.e. the maximum match size to analyze, based on the current match size:
        1) for a small match of size <= 3 -> use a table of size 3
        2) for a big match of size >=7 -> use a table of size 7
        3) for a medium-sized match or for money play -> use a table of size 5
- slightly modified options layout order
- removed bugs (cube value radio button upon match size downscale, match play equity upon table redraw)

Many thanks to Pierre Zakia and Philippe Michel for their patience and helpful comments!
*/

/* TBD - known issue:

The scoremap button of the 1st move of any game in an analyzed match doesn't work at first. The error message is:
"No game in progress (type `new game' to start one)."
However, if we click on another move (even w/o doing anything else) then come back, the scoremap of the 1st move then works.
Yet, if we change the game and come back: again, it doesn't work.

It looks like this is not a scoremap issue. For instance, the "show" button has the same issue: if we pick the 1st move
(or one of its alternatives) of any game and click "show", we get an "illegal move" error message; if we click elsewhere
and come back, it works again. It may be a movelist construction issue.
*/

#include "config.h"
#include "gtklocdefs.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "backgammon.h"
#include "eval.h"
#include "gtkscoremap.h"
#include "drawboard.h"
#include "format.h"
#include "gtkwindows.h"
//#include "gtkoptions.h"  


/*         GLOBAL *EXTERN) VARIABLES           */
scoreMapPly scoreMapPlyDefault = TWO_PLY; //default -> TWO_PLY at the end 
const char* aszScoreMapPly[NUM_PLY] = {N_("0-ply"), N_("1-ply"), N_("2-ply"), N_("3-ply"), N_("4-ply") };
const char* aszScoreMapPlyCommands[NUM_PLY] = { "0", "1", "2", "3", "4" };

scoreMapMatchLength scoreMapMatchLengthDefIdx = VAR_LENGTH;
/*the following list needs to correspond to the fixed lengths in the (typedef enum) scoreMapMatchLength */
const int MATCH_LENGTH_OPTIONS[NUM_MATCH_LENGTH]= {3,5,7,9,11,15,21,-1};   //list of allowed match sizes
const char* aszScoreMapMatchLength[NUM_MATCH_LENGTH]            = { N_("3"), N_("5"), N_("7"), N_("9"), N_("11"), N_("15"), N_("21"), N_("Based on current match length") };
const char* aszScoreMapMatchLengthCommands[NUM_MATCH_LENGTH]    = { "3", "5", "7", "9", "11", "15", "21", "-1" };

scoreMapLabel scoreMapLabelDef = LABEL_AWAY;
const char* aszScoreMapLabel[NUM_LABEL] = {N_("By away score"), N_("By true score")};
const char* aszScoreMapLabelCommands[NUM_LABEL] = { "away", "score"}; 

/*
 * In the next for blocks, the asz*Short[] arrays are used in this file only
 * while the longer ones are used in gtkgame.c as well for the settings pane.
 */
scoreMapJacoby scoreMapJacobyDef = MONEY_NO_JACOBY;
const char* aszScoreMapJacoby[NUM_JACOBY] = { N_("Unlimited game"), N_("Money game with Jacoby")};
static const char* aszScoreMapJacobyShort[2] = { N_("Unlimited (money w/o J)"), N_("Money w/ J") };
const char* aszScoreMapJacobyCommands[NUM_JACOBY] = { "nojacoby", "jacoby"}; 

scoreMapCubeEquityDisplay scoreMapCubeEquityDisplayDef = CUBE_NO_EVAL;
const char* aszScoreMapCubeEquityDisplay[NUM_CUBEDISP] = {N_("None"), N_("Equity"), N_("Double vs No Double"), N_("Double/Pass vs Double/Take")};
static const char* aszScoreMapCubeEquityDisplayShort[NUM_CUBEDISP] = {N_("None"), N_("Equity"), N_("D-ND"), N_("D/P-D/T")};
const char* aszScoreMapCubeEquityDisplayCommands[NUM_CUBEDISP] = { "no", "equity", "dnd", "dpdt"}; 

scoreMapMoveEquityDisplay scoreMapMoveEquityDisplayDef = MOVE_NO_EVAL;
const char* aszScoreMapMoveEquityDisplay[NUM_MOVEDISP] = {N_("None"), N_("Equity"), N_("Relative to second best")};
static const char* aszScoreMapMoveEquityDisplayShort[NUM_MOVEDISP] = {N_("None"), N_("Equity"), N_("vs 2nd best")};
const char* aszScoreMapMoveEquityDisplayCommands[NUM_MOVEDISP] = { "no", "absolute", "relative"}; 

scoreMapColour scoreMapColourDef = ALL;
const char* aszScoreMapColour[NUM_COLOUR] = {N_("All"), N_("Double vs No Double"), N_("Double/Pass vs Double/Take")};
static const char* aszScoreMapColourShort[NUM_COLOUR] = {N_("All"), N_("D vs ND"), N_("D/P vs D/T")};
const char* aszScoreMapColourCommands[NUM_COLOUR] = { "all", "dnd", "dpdt"}; 

scoreMapLayout scoreMapLayoutDef = VERTICAL;
const char* aszScoreMapLayout[NUM_LAYOUT] = { N_("Bottom"), N_("Right") };
const char* aszScoreMapLayoutCommands[NUM_LAYOUT] = { "bottom", "right"}; 



/*         DEFINITIONS        */

// these sizes help define the requested minimum quadrant size; but then GTK can set a higher value
// e.g. we may ask for height/width of 60, but final height will be 60 & final width 202
#define MAX_SIZE_QUADRANT 60 //(was 80)
#define DEFAULT_WINDOW_WIDTH 400        //in hint: 400; in rollout: 560; in tempmap: 400; in theory: 660
#define DEFAULT_WINDOW_HEIGHT 500       //in hint: 300; in rollout: 400; in tempmap: 500; in theory: 300
static int MAX_TABLE_WIDTH = (int)(0.85 * DEFAULT_WINDOW_WIDTH);
static int MAX_TABLE_HEIGHT = (int)(0.7 * DEFAULT_WINDOW_HEIGHT);

#define MAX_FONT_SIZE 12
#define MIN_FONT_SIZE 7


#define MAX_TABLE_SIZE 22 // CAREFUL: should be max(MATCH_LENGTH_OPTIONS)+1  (max-1 for cube scoremap, max+1 for move)
// #define MIN_TABLE_SIZE 6
#define LARGE_TABLE_SIZE 14 //beyond, make row/col score labels smaller

#define TABLE_SPACING 0 //should we show an "exploded" setting with separated cells, or a merged one (TABLE_SPACING=0)?
#define BORDER_SIZE 2 //size of border that has the full color corresponding to the decision
#define OUTER_BORDER_SIZE 1 //size of border that has the full color corresponding to the decision
#define BORDER_PADDING 0 //should we pad the borders and show their inner workings?


#define GAUGE_SIXTH_SIZE 7
#define GAUGE_SIZE (GAUGE_SIXTH_SIZE * 6+1)
#define GAUGE_BLUNDER (arSkillLevel[SKILL_VERYBAD]) // defines gauge scale (palette of colors in gauge)

// used in DecisionVal
typedef enum {ND, DT, DP, TG} decisionval;
//corresponding cube decision text in same order
static const char * CUBE_DECISION_TEXT[4] = {"ND","D/T","D/P","TG"};

#define IMPOSSIBLE_CMOVES (MAX_INCOMPLETE_MOVES+1)

// **** color definitions ****

// Used in colouring array (e.g., in ApplyColour())
#define RED 0
#define GREEN 1
#define BLUE 2

// we code the 4 cube colors for ND, DT, DP, TGTD as red, green, blue and gold, respectively;
// and use them in the 4 cube scoremap coloring options
static const float cubeColors[4][3] = {{1.0f,0.0f,0.0f}, {0.0f,0.95f,0.45f}, {0.0f,0.5f,1.0f}, {1.0f,0.75f,0.0f}};

#define TOP_K 4 // in move scoremap, number of most frequent best-move decisions we want to keep, corresponding to number
                //      of distinct (non-grey/white) scoremap colors
// careful: need to code below at least the TOP_K corresponding RGB colors, as used in FindMoveColour()
// converging on the same colors and same intensity as for the cube scoremap for now
static const float topKMoveColors[4][3] = {{1.0f,0.0f,0.0f}, {0.0f,0.95f,0.45f}, {0.0f,0.5f,1.0f}, {1.0f,0.75f,0.0f}};

static const float white[3] = {1.0, 1.0, 1.0};

// if a move is not in TOP_K, we color it in grey:
static const float grey[3] = {0.5, 0.5, 0.5};

//draw in black the border of current-score quadrant
static const float black[3] = {0.0, 0.0, 0.0};

// Used like a colour, to indicate not to colour at all
static const float NOBORDER[3]={-1.0,-1.0,-1.0};

// **** end color definitions ****

// score type definitions, for displaying special quadrants
typedef enum {NOT_TRUE_SCORE, TRUE_SCORE, LIKE_TRUE_SCORE} scorelike;
typedef enum {REGULAR, SCORELESS, MONEY_J, DMP, GG, GS} specialscore; 
typedef enum {ALLOWED, NO_CRAWFORD_DOUBLING, MISMATCHED_SCORES, UNREASONABLE_CUBE, UNALLOWED_DOUBLE, YET_UNDEFINED} allowedscore;


/* Note that quadrantdata and gtkquadrant are decoupled, because the correspondence between gtk quadrants
   and the quadrant data changes when toggling between label by away-scores and true score. */

typedef struct {
/* Represents the data that is displayed in a single quadrant. */
    // 1. shared b/w cube and move scoremaps
    cubeinfo ci; // Used when computing the equity for this quadrant.
    char decisionString[FORMATEDMOVESIZE]; // String for correct decision: 2-letter for cube, more for move
    char equityText[50];  //if displaying equity text in quadrant
    scorelike isTrueScore;  // indicates whether equal to current score, same as current score, or neither
    specialscore isSpecialScore; // indicates whether SCORELESS, MONEY_J, DMP, GG, GS, or regular
    allowedscore isAllowedScore; // indicates whether the score is not allowed and the quadrant should be greyed
    char unallowedExplanation[100]; // if unallowed, explains why

    // 2. specific to cube scoremap:
    float ndEquity; // Equity for no double
    float dtEquity; // Equity for double-take
    decisionval dec; // The correct decision

    // 3. specific to move scoremap:
    movelist ml; //scores ordered list of best moves  TODO (still relevant?): (1) replace with int anMove[8] (2) record index of this move in topKDecisions[]
} quadrantdata;

typedef struct {
/* The GTK widgets that make up a single quadrant. */
    GtkWidget * pDrawingAreaWidget;
    GtkWidget * pContainerWidget;
} gtkquadrant;

typedef struct {
//in move scoremap, struct to hold decisions and their frequencies
    char moveStr[FORMATEDMOVESIZE];
    int numMatches;
    int anMove[8];
} weightedDecision;

typedef struct { 
// Main structure, holding nearly everything
    // 1. shared b/w cube and move scoremaps:
    int cubeScoreMap; // whether we want a ScoreMap for cube or move (=1 for cube, =0 for move)
    const matchstate *pms; // state of the *true* match 
    matchstate msTemp; // temp match state at some score (i,j)
    evalcontext ec; // The eval context (takes into account the selected ply)
    int tableSize; // This actually represents the max away score minus 1 (since we never show score=1)
    int truenMatchTo;   //match length of the real game (before scoremap was launched)
    // int tempScaleUp; //temporary indicator for when the table size is increased
    // int oldTableSize; //keeping the old size in the above case
    
    /* current selected options*/
    scoreMapLabel labelBasedOn; 
    scoreMapColour colourBasedOn;
    scoreMapCubeEquityDisplay displayCubeEval;
    scoreMapMoveEquityDisplay displayMoveEval;
    scoreMapLayout layout;
    scoreMapJacoby labelTopleft;
    int moneyJacoby; // goes w/ previous line
    int matchLengthIndex; // index of simulated match size
    int matchLength;      //simulated match size
    int signednCube;    //simulated cube value (in move ScoreMap: with sign that indicates what player owns the cube)

    quadrantdata aaQuadrantData[MAX_TABLE_SIZE][MAX_TABLE_SIZE];
    quadrantdata moneyQuadrantData;
    gtkquadrant aagQuadrant[MAX_TABLE_SIZE][MAX_TABLE_SIZE];
    gtkquadrant moneygQuadrant;
    GtkWidget *apwRowLabel[MAX_TABLE_SIZE]; // Row and column score labels
    GtkWidget *apwColLabel[MAX_TABLE_SIZE];
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pwTableContainer;     // container for the above
    GtkWidget *pwCubeBox;
    GtkWidget *pwCubeFrame;
    GtkWidget* pwMegaVContainer;    // container for everything in scoremap layout
    GtkWidget *pwHContainer;    // container for options in horizontal layout
    GtkWidget *pwVContainer;    // container for options in vertical layout
    GtkWidget *pwLastContainer; // points to last used container (either vertical or horizontal)
    GtkWidget *pwOptionsBox;          // options in horizontal/vertical layout 

    // 2. specific to cube scoremap:
    GtkWidget *apwFullGauge[GAUGE_SIZE];    // for cube scoremap
    GtkWidget *apwGauge[4];                 // for cube scoremap

    // 3. specific to move scoremap:
    char topKDecisions[TOP_K][FORMATEDMOVESIZE];            //top-k most frequent best-move decisions
    // char * topKClassifiedDecisions[TOP_K];  //top-k most frequent best-move decisions with "English" description
    int topKDecisionsLength;                //b/w 0 and K
} scoremap;

// *******************************************************************

static GtkWidget *pwDialog = NULL;

//define desired number of output digits, i.e. precision, throughout the file (in equity text, hover text, etc)
#define DIGITS MAX(MIN(fOutputDigits, MAX_OUTPUT_DIGITS),0)

#define MATCH_SIZE(psm) (psm->matchLength) //(psm->cubeScoreMap ? psm->cubematchLength : psm->movematchLength)

//coding reminder: (1) static = valid in this file only; (2) we need to define a function before calling it, so if a function A calls a function B and A is written before B, we define B at the start
static void 
CubeValToggled(GtkWidget * pw, scoremap * psm);

void
BuildOptions(scoremap * psm);

static int isAllowed (int i, int j, scoremap * psm) {
/*"isAllowed()" helps set the unallowed quadrants in move scoremap. Reasons to forbid
 a quadrant:
REASON 1. we don't allow i=0 i.e. 1-away Crawford with j=1 i.e. 1-away post-Crawford,
         and more generally incompatible scores in i and j
REASON 2. we don't allow doubling in a Crawford game
REASON 3. more tricky: we don't allow an absurd cubing situation, eg someone leading in Crawford
         with a >1 cube, or someone at matchLength-2 who would double to 4
*/
    int iAway=MAX(i,1); //if i==0 i.e. 1-away post-Crawford it should be 1
    int jAway=MAX(j,1);

    if (!psm->cubeScoreMap) {
        // REASON 1
        //there are 3 squares we don't allow: (0,1), (1,0) and (1,1)
        if ( (i==0 && j==1) || (i==1 && j==0) || (i==1 && j==1) ) {
            psm->aaQuadrantData[i][j].isAllowedScore = MISMATCHED_SCORES;
            if (i==1 && j==1)
//                strcpy(psm->aaQuadrantData[i][j].unallowedExplanation, _("11This score is not allowed because one player is in Crawford while the other is in post-Crawford"));          
                strcpy(psm->aaQuadrantData[i][j].unallowedExplanation, _("This score is not allowed because both players cannot simultaneously reach one-away Crawford scores"));
            else
                strcpy(psm->aaQuadrantData[i][j].unallowedExplanation, _("This score is not allowed because one player is in Crawford while the other is in post-Crawford"));
            return 0;
        } else if (abs(psm->signednCube)>1 && (i==1 || j==1)) {
            // REASON 2: no doubling in Crawford
            psm->aaQuadrantData[i][j].isAllowedScore = NO_CRAWFORD_DOUBLING;
            strcpy(psm->aaQuadrantData[i][j].unallowedExplanation, _("Doubling is not allowed in a Crawford game"));
            return 0;
        } else { 
            // REASON 3
            //if I have doubled and got signednCube=-4, ie my opponent owns a 4-cube => the previous cube was +2
            //      => it cannot be that I am 1-away or 2-away from victory (or I wouldn't have had any reason to double);
            //      and likewise my opponent cannot be 1-away from victory since he doubled to +2 cube before
            //      (& same when mirroring for my opponent => 2*2 sub-cases)
            // Note: signednCube > 1 when player with fMove holds double; but the table is organized by player 0/player 1
            //      => need to correct for that as well
            int p0Cube = ((psm->signednCube>1 && psm->pms->fMove == 1) || (psm->signednCube<-1 && psm->pms->fMove != 1));
            int p1Cube = (psm->signednCube<-1 && psm->pms->fMove == 1) || (psm->signednCube>1 && psm->pms->fMove != 1);

            if (p0Cube && (abs(psm->signednCube) >= 2*iAway || abs(psm->signednCube) >= 4*jAway))   {
                // g_print("\n signednCube:%d, iAway:%d, jAway:%d",signednCube,iAway,jAway);
                psm->aaQuadrantData[i][j].isAllowedScore = UNREASONABLE_CUBE;
                if (abs(psm->signednCube) >= 2*iAway) {
                    sprintf(psm->aaQuadrantData[i][j].unallowedExplanation,
                                _("It is unreasonable that %s will double to %d while being %d-away"),
                                ap[0].szName, abs(psm->signednCube), iAway);
                } else if (abs(psm->signednCube) >= 4*jAway) {
                    sprintf(psm->aaQuadrantData[i][j].unallowedExplanation,
                                _("It is unreasonable that %s will have previously doubled to %d while being %d-away"),
                                ap[1].szName, abs(psm->signednCube)/2, jAway);
                }
                // g_print("\n1:%s",str);
                // psm->aaQuadrantData[i][j].unallowedExplanation = g_strdup(str);
                // g_print("\n2:%s",psm->aaQuadrantData[i][j].unallowedExplanation);
                return 0;
            } else if (p1Cube && (abs(psm->signednCube) >= 2*jAway || abs(psm->signednCube) >= 4*iAway)) {
                psm->aaQuadrantData[i][j].isAllowedScore = UNREASONABLE_CUBE;
                if (abs(psm->signednCube) >= 2*jAway) {
                    sprintf(psm->aaQuadrantData[i][j].unallowedExplanation,
                                _("It is unreasonable that %s will double to %d while being %d-away"),
                                ap[1].szName, abs(psm->signednCube), jAway);
                } else if (abs(psm->signednCube) >= 4*iAway) {
                    sprintf(psm->aaQuadrantData[i][j].unallowedExplanation,
                                _("It is unreasonable that %s will have previously doubled to %d while being %d-away"),
                                ap[0].szName, abs(psm->signednCube)/2, iAway);
                }
                return 0;
            }
        }

    }
    psm->aaQuadrantData[i][j].isAllowedScore = ALLOWED;
    return 1;
}

static decisionval
DecisionVal(float ndEq, float dtEq) {
/* Determines what the correct cube action is. */
    float dEq = MIN(dtEq, 1.0f); // Equity after double

    if ((ndEq < dEq) && (dtEq < 1.0f))
        return DT; //D/T
    else if (ndEq < dEq)
        return DP; //D/P
    else if (dtEq < 1.0f) // ND
        return ND;
    else  // TGTD
        return TG;
}

static float
mmwc2eq(const float rMwc, const cubeinfo *pci) {
/* Converts mwc to equity, in case of a match. If a money game, no conversion takes place. */
    if (pci->nMatchTo==0)
        return rMwc;
    else
        return mwc2eq(rMwc,pci);
}

static int
CalcQuadrantEquities(quadrantdata * pq, const scoremap * psm, int recomputeFully) {
/* In Cube ScoreMap: Calculates the DT and ND equities for the given quadrant. Updates data in pq accordingly.
In Move ScoreMap: Calculates the ordered best moves and their equities.
*/
    if (psm->cubeScoreMap) {
        if (!GetDPEq(NULL, NULL, & pq->ci)) { // Cube not available
                strcpy(pq->decisionString,"");
                pq->isAllowedScore = UNALLOWED_DOUBLE;
                strcpy(pq->unallowedExplanation, _("Unallowed double: cube not available"));
        } else {
            if (recomputeFully) {
                float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
                if (GeneralCubeDecisionE(aarOutput, psm->pms->anBoard, & pq->ci, & psm->ec, NULL)) { 
                        //GeneralCubeDecisionE is from eval.c
                        // extern int GeneralCubeDecisionE(float aarOutput[2][NUM_ROLLOUT_OUTPUTS], const TanBoard anBoard,
                        // cubeinfo * const pci, const evalcontext * pec, const evalsetup * UNUSED(pes))
                    /*If we are within this condition, it means there was a problem, e.g. 
                    the user stopped the computation in the middle*/
                    pq->ndEquity=-1000;
                    pq->dtEquity=-1000;
                    strcpy(pq->decisionString,"");
                    return -1;
                }
                // Convert MWC to equity, and store in the scoremap
                pq->ndEquity= mmwc2eq(aarOutput[0][OUTPUT_CUBEFUL_EQUITY], & pq->ci);
                pq->dtEquity= mmwc2eq(aarOutput[1][OUTPUT_CUBEFUL_EQUITY], & pq->ci);
            }
    //g_print("Score: %d-%d ",psm->aaQuadrantData[i][j].ci.anScore[0], psm->aaQuadrantData[i][j].ci.anScore[1]);
            // Produce 2-letter string corresponding to the correct cube action (eg DT for double-take)
            //pq->decisionString=DecisionShortString(pq->ndEquity,pq->dtEquity);
            pq->dec=DecisionVal(pq->ndEquity,pq->dtEquity);
            strcpy(pq->decisionString,CUBE_DECISION_TEXT[pq->dec]);
    // g_print("ND %f; DT %f; %s\n",ndEq,dtEq,pq->decisionString);
        }
        return 0;
    } else {  //move scoremap
        if (FindnSaveBestMoves(&(pq->ml),psm->pms->anDice[0],psm->pms->anDice[1], (ConstTanBoard) psm->pms->anBoard, NULL, //or pkey
                                        arSkillLevel[SKILL_DOUBTFUL], &(pq->ci), &psm->ec, aamfAnalysis) <0) { 
            strcpy(pq->decisionString,"");
            pq->ml.cMoves=0; //also used for DestroyDialog
            pq->ml.amMoves = NULL;
            //g_free(pq->ml.amMoves);
            return -1;
        }
        //g_assert(pq->ml.cMoves > 0);
        if (pq->ml.cMoves > 0) {
            FormatMove(pq->decisionString, (ConstTanBoard) psm->pms->anBoard, pq->ml.amMoves[0].anMove);
            //g_free(pq->ml.amMoves);        
            return 0;
        } else {    
            g_message("Problem, no moves found. FindnSaveBestMoves returned %d, %d, %1.3f; decision: %s\n",pq->ml.cMoves,pq->ml.cMaxMoves, pq->ml.rBestScore,pq->decisionString);
            return -1;
        }
    }
}

static int
CompareDecisionFrequencies (const void *a, const void *b)
{
/* qsort compare function */
    return ((const weightedDecision *)b)->numMatches-((const weightedDecision *)a)->numMatches;
}

static void
AddFrequentMoveList(weightedDecision *list, int *pcurSize, const char * moveStr, const int anMove[8]) {
/*
Sub-function that adds a new move to the list of frequent moves, i.e. either creates a new frequent move
if the new move is not in the list, or increments the count for the corresponding frequent move.
*/
    if (strcmp(moveStr,"")==0) return; // Don't save empty strings
    int i;
    for (i=0; i<*pcurSize && strcmp(moveStr, list[i].moveStr); i++); // Find first match
    // it looks like the algorithm works as follows: if we find a new move, i.e. didn't get any match and therefore i==*pcurSize,
    // then add the new move to the list and increment the list size; else, if it's the same as move i, increment the count for i
    if (i==*pcurSize) {
        (*pcurSize)++;
        strcpy(list[i].moveStr,moveStr);
        memcpy(list[i].anMove,anMove,8*sizeof(int));
        list[i].numMatches=1;
    } else
        list[i].numMatches++;
}

static void
FindMostFrequentMoves(scoremap *psm) {
/* Find the top-k most frequent moves (in a move scoremap only!), and assign them colors.

*/
    int i,j;

    //1. copy scoremap decisions into an array structure called that contains decisions and their frequencies
    //      (using a new struct called weightedDecision)
    weightedDecision scoreMapDecisions[psm->tableSize*psm->tableSize+1];  // = (weightedDecision *) g_malloc(sizeof(...));
    psm->topKDecisionsLength=0;
    // for all moves, add them to the list of frequent moves, i.e. either create a new frequent move or update the count of
    // the corresponding frequent move (this is implemented in AddFrequentMoveList())
    for (i=0; i<psm->tableSize; i++)
        for (j=0; j<psm->tableSize; j++)
            if (psm->aaQuadrantData[i][j].isAllowedScore == ALLOWED)
                AddFrequentMoveList(scoreMapDecisions,&psm->topKDecisionsLength,psm->aaQuadrantData[i][j].decisionString, psm->aaQuadrantData[i][j].ml.amMoves[0].anMove);
    AddFrequentMoveList(scoreMapDecisions,&psm->topKDecisionsLength,psm->moneyQuadrantData.decisionString, psm->moneyQuadrantData.ml.amMoves[0].anMove);

    g_assert(psm->topKDecisionsLength > 0);
    if (psm->topKDecisionsLength <= 0)
        return;

    //3. find most frequent moves
    qsort (scoreMapDecisions, psm->topKDecisionsLength, sizeof (weightedDecision), CompareDecisionFrequencies);  /* sort words by frequency */

    //4. record the first k (no need for counter anymore)
    psm->topKDecisionsLength=MIN(TOP_K,psm->topKDecisionsLength); //Cut off to TOP_K if there are more.
    for (i=0; i<psm->topKDecisionsLength; i++)
        strcpy(psm->topKDecisions[i],scoreMapDecisions[i].moveStr);
}

// rgbcolor
// FloatToRgbcolor (float rgbvals[3]) {
//     /* this function translates the float array notation into an rgbcolor struct */
//     rgbcolor rgbAux;
//     rgbAux.rval=rgbvals[RED];
//     rgbAux.gval=rgbvals[GREEN];
//     rgbAux.bval=rgbvals[BLUE];
//     return rgbAux;
// }

static void
ApplyBorderColour(GtkWidget * pw, const float borderrgbvals[3])
/* Colours the border of (pw) according to rgbvals.
*/
{
    float * borderrval = g_malloc(sizeof(float));
    float * bordergval = g_malloc(sizeof(float));
    float * borderbval = g_malloc(sizeof(float));

    *borderrval = borderrgbvals[RED];
    *bordergval = borderrgbvals[GREEN];
    *borderbval = borderrgbvals[BLUE];

    /* Next store the colours in pw's association table. These values are read and "painted" on the
       quadrant to apply the colour. */
    g_object_set_data_full(G_OBJECT(pw), "borderrval", borderrval, g_free); //table of associations from strings to pointers.
    g_object_set_data_full(G_OBJECT(pw), "bordergval", bordergval, g_free);
    g_object_set_data_full(G_OBJECT(pw), "borderbval", borderbval, g_free);
}


static void
ApplyColour(GtkWidget * pw, const float rgbvals[3], const float borderrgbvals[3])
/* Colours the drawing area (pw) according to rgbvals.
*/
{
    float * rval = g_malloc(sizeof(float));
    float * gval = g_malloc(sizeof(float));
    float * bval = g_malloc(sizeof(float));
    float * borderrval = g_malloc(sizeof(float));
    float * bordergval = g_malloc(sizeof(float));
    float * borderbval = g_malloc(sizeof(float));

    *rval = rgbvals[RED];
    *gval = rgbvals[GREEN];
    *bval = rgbvals[BLUE];
    *borderrval = borderrgbvals[RED];
    *bordergval = borderrgbvals[GREEN];
    *borderbval = borderrgbvals[BLUE];

    /* Next store the colours in pw's association table. These values are read and "painted" on the
       quadrant to apply the colour. */
    g_object_set_data_full(G_OBJECT(pw), "rval", rval, g_free); //table of associations from strings to pointers.
    g_object_set_data_full(G_OBJECT(pw), "gval", gval, g_free);
    g_object_set_data_full(G_OBJECT(pw), "bval", bval, g_free);
    g_object_set_data_full(G_OBJECT(pw), "borderrval", borderrval, g_free); //table of associations from strings to pointers.
    g_object_set_data_full(G_OBJECT(pw), "bordergval", bordergval, g_free);
    g_object_set_data_full(G_OBJECT(pw), "borderbval", borderbval, g_free);
}

// //we decided to not use this function at the end and keep all fonts in black
// void
// UpdateFontColor(quadrantdata * pq, rgbcolor rgbBackground)
// {
//     //we want to use a black font color on dark background colors, and white on dark
//     // the following formula computes the background luminnace for that
//     // it is taken from https://stackoverflow.com/questions/1855884/determine-font-color-based-on-background-color
//     float luminance = 0.299 * rgbBackground.rval + 0.587 * rgbBackground.gval + 0.114 * rgbBackground.bval; //luminance
//     // float luminance = 0.299 * rgbvals[0] + 0.587 * rgbvals[1] + 0.114 * rgbvals[2]; //luminance

//     strcpy(pq->rgbFont, (luminance>0.5) ? ("black") : ("white")); //black vs white //[test: ("red") : ("yellow"));]

//     // pgq->rgbFont.rval = pgq->rgbFont.gval = pgq->rgbFont.bval = (luminance>0.5) ? (0.0) : (1.0); //black vs white
//     // rgbFont[0]=pgq->rgbFont[1]=pgq->rgbFont[2]= (luminance>0.5) ? (0.0) : (1.0); //black vs white
//     // if (luminance > 0.5)
//     //      = {0.0,0.0,0.0}; //black
//     // else
//     //     = {1.0,1.0,1.0}; //white
// }

static void
UpdateStyleGrey(gtkquadrant * pgq)
{
    float rgbvals[3]={0.75,0.75,0.75};
    ApplyColour(pgq->pDrawingAreaWidget,rgbvals,NOBORDER);
    //return FloatToRgbcolor(rgbvals);
}

static float
CalcIntensity(float r)  {
/* Transforms the value r (an equity difference) to an intensity to be used
   to determine the colour. This can be tweaked.
   Currently, anything considered "Very bad" is given full intensity, and linear in between.
*/
    float maxIntensity = arSkillLevel[SKILL_VERYBAD];
    return MAX(MIN(r/maxIntensity,1.0f),-1.0f);
}

static void
ColorInterpolate(float rgbResult[3], float x, const float rgbvals0[3], const float rgbvals1[3])
{
/*  fills rgbResult with a linearly-interpolated color at x \in [0,1] between color rgbvals0 and color rgbval1, such that
 x=0 yields rgbvals0 and x=1 yields rgbvals1,
*/
    x=MAX(0.0f,MIN(1.0f,x));
    for (int i=0; i<3; i++) {
        rgbResult[i] = (1.0f-x)*rgbvals0[i]+x*rgbvals1[i];
    }
}

static void
FindColour1(GtkWidget * pw, float r, scoreMapColour toggledColor, int useBorder)
/* Changes the colour according to the value r for the two binary gauges.
In addition, writes in rgbvals the desired border color for the quadrant.
*/
{
    r = CalcIntensity(r); // returns r in [-1,1]
    int bestDecisionColor;

    // GtkStyle *ps = gtk_style_copy(gtk_widget_get_style(pw)); //used?   <-----------------------------
    float rgbvals[3];

    if (toggledColor==DND) { //no double / double
        if (r<0) { //i.e. use red
            ColorInterpolate(rgbvals,-r,white,cubeColors[0]);
            bestDecisionColor=0;
            // rgbvals[RED]=1.0f;
            // rgbvals[BLUE]=rgbvals[GREEN]=MAX(1.0f+r,0.0f);
        } else { // green/cyan?
            ColorInterpolate(rgbvals,r,white,cubeColors[1]);
            bestDecisionColor=1;
            // rgbvals[GREEN]=1.0;
            // rgbvals[RED]=rgbvals[BLUE]=MAX(1.0f-r,0.0f);
        }
    } else { //Take/Pass
        if (r<0) { //green
            ColorInterpolate(rgbvals,-r,white,cubeColors[1]);
            bestDecisionColor=1;
            // rgbvals[GREEN]=1.0;
            // rgbvals[RED]=rgbvals[BLUE]=MAX(1.0f+r,0.0f);
        } else { // blue
            ColorInterpolate(rgbvals,r,white,cubeColors[2]);
            bestDecisionColor=2;
            // rgbvals[BLUE]=1.0;
            // rgbvals[GREEN]=MAX(1.0f-r,0.0f);
            // rgbvals[RED]=MAX((1.0f-r)/2.0f,0.0f); //avoiding purple tint
        }
    }
    //colors the quadrant in the desired (light) color
    if (!useBorder)
       ApplyColour(pw,rgbvals, NOBORDER);
    else if (bestDecisionColor>=0)
       ApplyColour(pw,rgbvals, cubeColors[bestDecisionColor]);
    else {
       g_assert_not_reached();
       ApplyColour(pw,rgbvals, NOBORDER);
    }
}



static void
FindColour2(GtkWidget * pw, float nd, float dt, int useBorder) {
/* In cube colormap, changes the colour according to the nd and dt values, when the colouring is based on both decisions
Note: since the cube colormap also has a gauge that uses this function, we cannot set gtkquadrant * pgq as an input (since
a gauge doesn't have it), and therefore cannot color the border directly from here, since we only have a pointer to the
main quadrant widget, not the borders around it.
As a result, the function writes in rgbvals the desired border color for the quadrant, and we use it after calling the function.
*/
    // initial idea: let color=color(opt) = red for ND, green for DT, blue for DP (corresponding to RGB...)
    //          and let final_r=sqrt(mistake)=sqrt(equity(opt)-equity(2nd best))=strength of color, using sqrt(x)
    //          rather than x to amplify small differences
    //          For example, full ND = red = (1,0,0) in RGB and DT = green = (0,1,0); so
    //          borderline ND/DT = (1,1,0) [or (0.5,0.5,0)] = yellow
    //          See https://www.freestylersupport.com/wiki/tutorial:sequences_ideas:rainbow_tutorial for an illustration.
    //
    // Version 2.0: a mix of colors looks bad/complicated (see alpha in commented example below: even a weak yellow doesn't work),
    //          so let's have borderline ND/D = (1,1,1) = white, not yellow
    //          With full white: the color is independent of the direction (symmetric) => 3 cases, not 6
    //          Note that ND=TGTD in such a case, but we can use a different color if it helps
    //
    // V 3.0: strong green and weak green look the same, trying different colors. Also the blue has a purple tint, correcting it.
    //
    // V4.0: the sqrt was flattening differences. Replacing the sqrt by something like (error/maxError), this looks better
    //          (see CalcIntensity()).
    //
    // V5.0: following feedback by Ian Shaw, switching to a different (4th) color for TGTD

    float r, dp = 1.0f;
    int bestDecisionColor;
    float rgbvals[3] = {0.9f, 0.9f, 0.9f}; //arbitrary initialization to grey

    //float alpha=0.05;
    // it is also possible to use the DecisionVal() function, but here we also want to determine the equity difference with 2nd best
    if (nd >= MIN(dt,dp) && dt <= dp) { //opt=nd
        r=CalcIntensity(nd-dt); //was: CalcIntensity(nd-MIN(dt,dp)); //note that CalcIntensity(x>=0) is guaranteed to be in [0,1]
        ColorInterpolate(rgbvals,r,white,cubeColors[0]);
        bestDecisionColor=0;
        // rgbvals[RED]=1.0f;
        // rgbvals[GREEN]=1.0f-r;
        // rgbvals[BLUE]=1.0f-r;
    } else if (nd >= MIN(dt,dp)) { //opt=tgtd
        r=CalcIntensity(nd-dp); //note that CalcIntensity(x>=0) is guaranteed to be in [0,1]
        ColorInterpolate(rgbvals,r,white,cubeColors[3]);
        bestDecisionColor=3;
    } else if (nd <= MIN(dt,dp) && dt <= dp) { //opt=dt
        r=CalcIntensity(MIN(dt-nd,dp-dt)); //r=1->green=(0,1,0), r=0->white=(1,1,1)
        ColorInterpolate(rgbvals,r,white,cubeColors[1]);
        bestDecisionColor=1;
    } else if (nd <= MIN(dt,dp) && dp <= dt) { //opt=dp
        r=CalcIntensity(MIN(dp-nd,dt-dp));
        ColorInterpolate(rgbvals,r,white,cubeColors[2]);
        bestDecisionColor=2;
        // rgbvals[RED]=(1.0f-r)/2.0f;  //<---- here the /2.0 to avoid a purple-ish color
        // rgbvals[GREEN]=1.0f-r;
        // rgbvals[BLUE]=1.0f;
    } else { 
        // at this point all cases should be covered but some old compilers don't see it
	// and complain that bestDecisionColor may later be used uninitialized
        g_assert_not_reached();
	bestDecisionColor = -1;
    }

    //colors the quadrant in the desired (light) color
    if (!useBorder)
       ApplyColour(pw, rgbvals, NOBORDER);
    else if (bestDecisionColor >= 0)
       ApplyColour(pw, rgbvals, cubeColors[bestDecisionColor]);
    else {
       g_assert_not_reached();
       ApplyColour(pw, rgbvals, NOBORDER);
    }
}

// float CalcMoveIntensity(float r)  {
// /* Transforms the value r (an equity difference) to an intensity to be used
//    to determine the colour. This can be tweaked.
//    Currently, anything considered "Very bad" is given full intensity, and linear in between.
// */
//     float maxIntensity = 0.050f; //arSkillLevel[SKILL_VERYBAD];
//     return MAX(MIN(r/maxIntensity,1.0f),0.0f); //fmaxf, fabsf
// }

static void
FindMoveColour(gtkquadrant * pgq, quadrantdata * pq, const scoremap * psm, float r) { //)GtkWidget * pw, char * bestMove, const scoremap * psm, float r) {
/*  This function colors the quadrants in the move scoremap.
It looks at the best move of each quadrant, and compares it with the top-k most frequent moves of the scoremap
(or less if there are less than k); it then colors the quadrant according to TOP_K_COLORS.
For example, if the best move is the 3rd most frequent move, it looks at TOP_K_COLORS[3].
In addition, the function applies directly a desired strong border color TOP_K_COLORS[3] for the quadrant.
For instance, if TOP_K_COLORS[3]==(deep) blue, the border will be colored in deep blue, while the quadrant will be colored in light
blue, where the blue intensity is proportional to the equity difference with the 2nd best move, i.e. the magnitude of the mistake.
Finally, the function also adds the decision classification that corresponds to the best move (and therefore to the
corresponding color).
*/
    float rgbvals[] = {0.9f, 0.9f, 0.9f}; //greyish if no match
    // rgbcolor rgbBackground; //rgbvals={0.9f, 0.9f, 0.9f};
    int i;
    // rgbvals[RED]=rgbvals[BLUE]=rgbvals[GREEN]=0.9f; //greyish if no match

    // for each decision, we look whether it's equal to a top color, and color it accordingly
    // Note: it's better to pick bright colors (for the black font), and maybe colors
    //      that are different from the cube scoremap colors
  //g_print("\n %f->%f",r,CalcIntensity(r));
    r = CalcIntensity(r);
// g_print("\n-> r:%f....best move:%s",r, bestMove);
    for (i = 0; i < psm->topKDecisionsLength; i++) {
        if (strcmp(pq->decisionString,psm->topKDecisions[i]) == 0) { //there is a match
            ColorInterpolate(rgbvals,r,white,topKMoveColors[i]);
            ApplyColour(pgq->pDrawingAreaWidget, rgbvals, topKMoveColors[i]); // apply intensity-modulated color on main quadrant
            return; //break;
        }
    }
// if no match at all (no return): we pick grey for all remaining moves that are not in top-k
    ColorInterpolate(rgbvals,r,white,grey); //333
    ApplyColour(pgq->pDrawingAreaWidget, rgbvals, grey); // apply intensity-modulated color on main quadrant
}

#define CalcRVal(toggle,nd,dt) (toggle==PT ? dt-1.0f : MIN(dt,1.0f)-nd)
                    //if yes, it's T vs P -> confusing, because the 2nd player wants the *lower* score =>r = -(dp-dt)
                    // if no, it's ND vs D -> r = dt-nd is natural, but we need to take TGTD into account:
                    //     so 1st player doubles if the best option for player 2, namely min(dt,dp), is >nd


static float
Interpolate(float x, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
    // float t;

    if (x <= x0) {
        return y0;
    } else if (x <= x1) {
        return y0 + (x-x0)/(x1-x0)*(y1-y0);
    } else if (x <= x2) {
        return y1 + (x-x1)/(x2-x1)*(y2-y1);
    } else if (x <= x3) {
        return y2 + (x-x2)/(x3-x2)*(y3-y2);
    } else
        return y3;
}

static void
DrawGauge(GtkWidget * pw, int i, scoreMapColour toggledColor) {

    float maxDiff=GAUGE_BLUNDER; //e.g. SKILL_VERYBAD;

    //If we consider the "multi-color" choice of ND/DT/DP/TGTD, we call function FindColour2 that depends on a float array.
    // Else, if we want a simpler binary choice, we call UpdateStyle that only depends on a single float.
    if (toggledColor == ALL) {
        // VERSION 1.0: getting a difference between the opt and the 2nd best of 1 at the peak
        // if (i<=GAUGE_THIRD_SIZE){
        //     nd=2.0-3.0*(float)i/(float)GAUGE_THIRD_SIZE;
        //     dt=1.0-(float)i/(float)GAUGE_THIRD_SIZE;
        // } else if (i<=2*GAUGE_THIRD_SIZE){
        //     nd=(float)i/(float)GAUGE_THIRD_SIZE-2.0;
        //     dt=2.0*(float)i/(float)GAUGE_THIRD_SIZE-2.0;
        // }  else if (i<=3*GAUGE_THIRD_SIZE){
        //     nd=2.0*(float)i/(float)GAUGE_THIRD_SIZE-4.0;
        //     dt=4.0-(float)i/(float)GAUGE_THIRD_SIZE;
        // }
        // V 2.0: flatten the gauge differences: getting a difference between the opt and the 2nd best of maxDiff at the peak
        //      This enables linking it to SKILL_VERYBAD for instance.
        float nd = Interpolate(0.5f*(float)i/(float)GAUGE_SIXTH_SIZE, 0.0f, 1.0f+maxDiff, 1.0f, 1.0f-2.0f*maxDiff, 2.0f, 1.0f-maxDiff, 3.0f, 1.0f+maxDiff);
        float dt = Interpolate(0.5f*(float)i/(float)GAUGE_SIXTH_SIZE, 0.0f, 1.0f, 1.0f, 1.0f-maxDiff, 2.0f, 1.0f+maxDiff, 3.0f, 1.0001f); //epsilon added at the end to distinguish TGTD from ND
        FindColour2(pw, nd, dt, FALSE);
    } else {
        float x = GAUGE_BLUNDER*((float)i /(float)(GAUGE_SIXTH_SIZE*3)-1.0f); // we want to scale the gauge by the GAUGE_BLUNDER value
        FindColour1(pw, x, toggledColor, FALSE);
    }
    gtk_widget_queue_draw(pw); // here we are calling DrawQuadrant() in a hidden way through the "draw" call
}

static void
SetHoverText(char buf [], quadrantdata * pq, const scoremap * psm) {
/*
This functions sets the hover text.
We first put a header text for special squares (e.g. same score as current match, or money),
    then we put the equities for the 3 alternatives (ND, D/T, D/P).
Note: we add one more space for "ND" b/c it has one less character than D/T, D/P so they are later aligned
*/

    char ssz[200];
    char space2[50];
    
    strcpy(buf, "");

    switch (pq->isTrueScore) {
        case TRUE_SCORE:
            strcpy(buf, "<b>");
            strcat(buf, _("Current match score:"));
            strcat(buf, " </b>\n\n");
            break;
        case LIKE_TRUE_SCORE:
            strcpy(buf, "<b>");
            strcat(buf, _("Like current match score:"));
            strcat(buf, " </b>\n\n");
            break;
        case NOT_TRUE_SCORE:
            /* "" in buf is fine */
            break;
    }
    switch (pq->isSpecialScore) {  //the square is special: MONEY_J, DMP etc. => write in hover text 
        case SCORELESS:
            strcpy(buf, "<b>Unlimited (money, no Jacoby):</b>\n\n");
            break;
        case MONEY_J:
            strcpy(buf, "<b>Money (with Jacoby):</b>\n\n");
            break;
        case DMP:
            strcpy(buf, "<b>");
            strcat(buf, _("DMP (double match point):"));
            strcat(buf, " </b>\n\n");
            break;
        case GG:
            strcpy(buf, "<b>");
            strcat(buf, _("Typical GG (gammon-go):"));
            strcat(buf, " </b>\n\n");
            break;
        case GS:
            strcpy(buf, "<b>");
            strcat(buf, _("Typical GS (gammon-save):"));
            strcat(buf, " </b>\n\n");
            break;
        case REGULAR:
            /* "" in buf is fine */
            break;
    }

    if (psm->cubeScoreMap) {
        float nd=pq->ndEquity;
        float dt=pq->dtEquity;
        float dp=1.0f;
        if (nd<-900.0) //reflects issue, typically user stops computation in the middle
            sprintf(ssz,_("Score not analysed\nComputation stopped by user"));
        else {
            sprintf(space2, "%*c", DIGITS + 7, ' ');   //define spacing before putting ply of 1st line
            if (pq->dec == ND) { //ND is best //format: "+" forces +/-; .*f displays f with precision DIGITS
                sprintf(ssz, _("<tt>1. ND \t%+.*f%s(%u-ply)\n2. D/P\t%+.*f  %+.*f  (%u-ply)\n3. D/T\t%+.*f  %+.*f  (%u-ply)</tt>"), DIGITS,nd,space2,psm->ec.nPlies,DIGITS,dp,DIGITS,dp-nd,psm->ec.nPlies,DIGITS,dt,DIGITS,dt-nd,psm->ec.nPlies);//pq->ml.amMoves[0].esMove.ec.nPlies 
            } else if (pq->dec == DT) {//DT is best
                sprintf(ssz, _("<tt>1. D/T\t%+.*f%s(%u-ply)\n2. D/P\t%+.*f  %+.*f  (%u-ply)\n3. ND \t%+.*f  %+.*f  (%u-ply)</tt>"), DIGITS,dt,space2,psm->ec.nPlies,DIGITS,dp,DIGITS,dp-dt,psm->ec.nPlies,DIGITS,nd,DIGITS,nd-dt,psm->ec.nPlies);
            } else if (pq->dec == DP) { //DP is best
                sprintf(ssz, _("<tt>1. D/P\t%+.*f%s(%u-ply)\n2. D/T\t%+.*f  %+.*f  (%u-ply)\n3. ND \t%+.*f  %+.*f  (%u-ply)</tt>"), DIGITS,dp, space2,psm->ec.nPlies,DIGITS,dt,DIGITS,dt-dp,psm->ec.nPlies,DIGITS,nd,DIGITS,nd-dp,psm->ec.nPlies);
            } else { //TGTD is best
                sprintf(ssz, _("<tt>1. ND \t%+.*f%s(%u-ply)\n2. D/T\t%+.*f  %+.*f  (%u-ply)\n3. D/P\t%+.*f  %+.*f  (%u-ply)</tt>"), DIGITS,nd, space2,psm->ec.nPlies,DIGITS,dt,DIGITS,dt-nd,psm->ec.nPlies,DIGITS,dp,DIGITS,dp-nd,psm->ec.nPlies);
            }                
        }
        strcat(buf,ssz);
    } else { // move scoremap
        // in the mouse hover window, we want to display the best move, then the 2nd and 3rd best moves and
        //      their equity difference with the best move. We also introduce spacing to align the "2nd column"
        //      on the right for the equity differences of the 2nd and 3rd best moves.
        int len0, len1, len2, len;
        char space[50];
        char szMove0[FORMATEDMOVESIZE], szMove1[FORMATEDMOVESIZE], szMove2[FORMATEDMOVESIZE];

        if (pq->ml.cMoves>0) { // best move
            FormatMove(szMove0, (ConstTanBoard) psm->pms->anBoard, pq->ml.amMoves[0].anMove);
        }
        if (pq->ml.cMoves>1) { // if exists, 2nd best move
            FormatMove(szMove1, (ConstTanBoard) psm->pms->anBoard, pq->ml.amMoves[1].anMove);
        }
        if (pq->ml.cMoves>2) { // if exists, 3rd best move
            FormatMove(szMove2, (ConstTanBoard) psm->pms->anBoard, pq->ml.amMoves[2].anMove);
            // as in the bottom-right window, we want to display 3 columns: "move...moveEquity...equityDiffWithBestMove"
            // unfortunately, it is not easy to align the 3 columns: pango does not have a multi-alignment option,
            //      and cannot run a <table><tr><td> either
            // therefore, as we first want to align the column of the 3 equities (middle column), we find the length
            //      of the longest string in the first column and add a variable number of spaces to make the
            //      first column of the same length in all rows.
            // in addition, we use <tt> to make sure that all characters take the same amount of space ("sans" font was
            //      not enough")
            // finally, if some lines have a positive equity (".012") and others a negative ("-.012"), then the "-"
            //      breaks the alignment by 1 character, so we want to take it into account as well
            // ADDED: it turns out that gnubg sometimes returns a 2-ply eval even in a 3-ply mode if the eval difference 
            //      between the best and 2nd-best move is large. We now mention the n-ply in the hover text.
            len0 = (int)strlen(szMove0) + (pq->ml.amMoves[0].rScore<-0.0005); //length, + 1 potentially b/c of the "-"
            len1 = (int)strlen(szMove1) + (pq->ml.amMoves[1].rScore<-0.0005);
            len2 = (int)strlen(szMove2) + (pq->ml.amMoves[2].rScore<-0.0005);
            len = MAX(len0,MAX(len1,len2)) +2 ; //we leave an additional spacing of 2 characters, where 2 is arbitrary,
                                                //  b/w the longest move string and its equity
            sprintf(space, "%*c", len - len0, ' ');   //define spacing for best move
            sprintf(space2, "%*c", DIGITS + 6, ' ');   //define spacing before putting ply of 1st line
            sprintf(ssz, _("<tt>1. %s%s%1.*f%s(%u-ply)"), szMove0, space, DIGITS, pq->ml.amMoves[0].rScore, space2, pq->ml.amMoves[0].esMove.ec.nPlies);
            strcat(buf,ssz);
            sprintf(space,"%*c", len-len1, ' ');   //define spacing for 2nd best move
            sprintf(ssz, _("\n2. %s%s%1.*f  %+.*f (%u-ply)"), szMove1,space,DIGITS,pq->ml.amMoves[1].rScore,DIGITS,pq->ml.amMoves[1].rScore-pq->ml.amMoves[0].rScore, pq->ml.amMoves[1].esMove.ec.nPlies);
            strcat(buf,ssz);
            sprintf(space,"%*c", len-len2, ' ');   //define spacing for 3rd best move
            sprintf(ssz, _("\n3. %s%s%1.*f  %+.*f (%u-ply)</tt>"), szMove2,space,DIGITS,pq->ml.amMoves[2].rScore,DIGITS,pq->ml.amMoves[2].rScore-pq->ml.amMoves[0].rScore, pq->ml.amMoves[2].esMove.ec.nPlies);
            strcat(buf,ssz);
        }  else if (pq->ml.cMoves==2) { // there are only 2 moves
            len0 = (int)strlen(szMove0)+(pq->ml.amMoves[0].rScore<-0.0005);
            len1 = (int)strlen(szMove1)+(pq->ml.amMoves[1].rScore<-0.0005);
            len = MAX(len0,len1)+2;
            sprintf(space,"%*c", len-len0, ' ');   //define spacing for best move
            sprintf(space2, "%*c", DIGITS + 6, ' ');   //define spacing before putting ply of 1st line
            sprintf(ssz, _("<tt>1. %s%s%1.*f%s(%u-ply)"), szMove0, space, DIGITS, pq->ml.amMoves[0].rScore, space2, pq->ml.amMoves[0].esMove.ec.nPlies);
            strcat(buf,ssz);
            sprintf(space,"%*c", len-len1, ' ');   //define spacing for 2nd best move
            sprintf(ssz, _("\n2. %s%s%1.*f  %+.*f  (%u-ply)</tt>"), szMove1,space,DIGITS,pq->ml.amMoves[1].rScore,DIGITS,pq->ml.amMoves[1].rScore-pq->ml.amMoves[0].rScore, pq->ml.amMoves[1].esMove.ec.nPlies);
            strcat(buf,ssz);
        } else if (pq->ml.cMoves==1) { //single move
            sprintf(ssz, _("<tt>1. %s  %1.*f         (%u-ply)</tt>"), szMove0,DIGITS,pq->ml.amMoves[0].rScore, pq->ml.amMoves[0].esMove.ec.nPlies);
            strcat(buf,ssz);
        } else {
            /* This cannot happen. If there is no legal move there is no
               move analysis and no Score Map button to bring us here. 
               ... Actually this may happen if the user stops the computation.
               The new hover text reflects it.*/
            //g_assert_not_reached();
            // sprintf(ssz,"no legal moves");
            sprintf(ssz,_("Score not analysed\nComputation stopped by user"));
            strcat(buf,ssz);
        }
    }
}


static void
ColourQuadrant(gtkquadrant * pgq, quadrantdata * pq, const scoremap * psm) {
/* Colours the given quadrant and sets the hover text.
*/
    float nd=pq->ndEquity;
    float dt=pq->dtEquity;
    float dp=1.0f;
    float r;
    char buf [300];

    // color the squares
    if (pq->isAllowedScore!=ALLOWED) { // Cube not available or unallowed square, e.g. i says Crawford and j post-C.
        UpdateStyleGrey(pgq);
        // UpdateFontColor(pq, rgbBackground);
        gtk_widget_set_tooltip_text(pgq->pContainerWidget, pq->unallowedExplanation);
    } else {
        // First compute and fill the color, as well as update the equity text
        if (psm->cubeScoreMap) {
            // if no eval to display, nothing to do for the eval text; if absolute or relative eval, update now
            // also, if the user stopped the computation in the middle and the eval is worthless, don't display it
            if (nd<-900 && psm->displayCubeEval != CUBE_NO_EVAL) {
                sprintf(pq->equityText,"\n -- ");
            } else if (psm->displayCubeEval == CUBE_ABSOLUTE_EVAL) {
                sprintf(pq->equityText,"\n%+.*f",DIGITS, MAX(nd, MIN(dt, dp)));
            } else if (psm->displayCubeEval == CUBE_RELATIVE_EVAL_ND_D) {
                sprintf(pq->equityText,"\n%+.*f",DIGITS, MIN(dt, dp) - nd);
            } else if (psm->displayCubeEval == CUBE_RELATIVE_EVAL_DT_DP) {
                sprintf(pq->equityText, "\n%+.*f", DIGITS, dp - dt);
            }
            if (psm->colourBasedOn != ALL) {
                r = CalcRVal(psm->colourBasedOn, nd, dt);
                FindColour1(pgq->pDrawingAreaWidget, r,psm->colourBasedOn, TRUE);
            } else {
                FindColour2(pgq->pDrawingAreaWidget,nd,dt,TRUE);
            }
        } else { //move scoremap
            //r is the equity difference b/w the top 2 moves, assuming they exist
            // we also set the equity text
            if (pq->ml.cMoves==0) { //no moves
                r=0.0f;
//                if (psm->displayMoveEval != MOVE_NO_EVAL)
                    sprintf(pq->equityText,"\n -- ");
            }   else {//at least 1 move
                r = (pq->ml.cMoves>1) ? (pq->ml.amMoves[0].rScore - pq->ml.amMoves[1].rScore) : (1.0f);
                if (psm->displayMoveEval == MOVE_ABSOLUTE_EVAL)
                    sprintf(pq->equityText,"\n%+.*f",DIGITS, (pq->ml.amMoves[0].rScore));
                else if (psm->displayMoveEval == MOVE_RELATIVE_EVAL) {
                    if (pq->ml.cMoves==1)
                        sprintf(pq->equityText,"\n -- ");
                    else
                        sprintf(pq->equityText,"\n%+.*f",DIGITS, r);
                } // we don't do anything if NO_EVAL, i.e. no equity text
            }

            FindMoveColour(pgq,pq,psm,r);//pgq
            // use black font on bright background, and white on dark
            // UpdateFontColor(pq, rgbBackground);
        }

        // Draw in black the border of the (like-)true-score quadrant (if it exists)
        if (pq->isTrueScore!=NOT_TRUE_SCORE) {
            ApplyBorderColour(pgq->pDrawingAreaWidget, black);
        }

        // Next set the hover text
        SetHoverText (buf,pq,psm);

        gtk_widget_set_tooltip_markup(pgq->pContainerWidget, buf);
    }
    gtk_widget_queue_draw(pgq->pDrawingAreaWidget); // here we are calling DrawQuadrant() in a hidden way through the "draw" call,
                                                    //      as initially defined in InitQuadrant()
}

static void
UpdateScoreMapVisual(scoremap * psm)
/* Update all the visual components of the score map (called at the outset, or after one of the options changes):
Specifically: (1) The color of each square (2) The hover text of each square (3) the row/col score labels (4) the gauge.
*/
{
    int i, j, i2, j2;

    //start by the top-left money square 
    ColourQuadrant(& psm->moneygQuadrant, & psm->moneyQuadrantData, psm);

    // start by updating the score labels for each row/col and the color and hover text
    // i2, j2 = indices in the visual table (i.e., in aagQuadrant)
    // i,j = away-scores -2 (i.e., indices in aaQuadrantData)
    for (i2 = 0; i2 < psm->tableSize; ++i2) {
        // if (psm->cubeScoreMap)
        i = (psm->labelBasedOn == LABEL_AWAY) ? i2 : psm->tableSize-1-i2;
        // else //move scoremap
        //     i = (labelBasedOn == LABEL_AWAY) ? i2 : psm->tableSize-1-i2;

        /* Colour and apply hover text to all the quadrants */
        for (j2 = 0; j2 < psm->tableSize; ++j2) {
            j = (psm->labelBasedOn == LABEL_AWAY) ? j2 : psm->tableSize-1-j2;
            //if(i<=oldSize && j<=oldSize)
                ColourQuadrant(& psm->aagQuadrant[i2][j2], & psm->aaQuadrantData[i][j], psm);
        } // end of: for j2

        /* update score labels */

        // // PROBLEM: how to label when there are many columns and the labels become unreadable?
        // // SOL 1: put odd top labels on a new row
        // if (psm->tableSize < LARGE_TABLE_SIZE || i%2) {
        //     if (labelBasedOn == LABEL_AWAY)
        //             sprintf(sz, _("%d-away"), i+2);
        //     else
        //             sprintf(sz, _("%d/%d"), i2,psm->tableSize+1);
        //     gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
        //     gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz);
        // } else {
        //     if (labelBasedOn == LABEL_AWAY) {
        //                 sprintf(sz, _("%d-away"), i+2);
        //                 sprintf(sz2, _("\n%d-away"), i+2);
        //     }    else  {
        //                 sprintf(sz, _("%d/%d"), i2,psm->tableSize+1);
        //                                     sprintf(sz2, _("\n%d/%d"), i2,psm->tableSize+1);
        //     }
        //     gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
        //     gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz2);
        // }
        // // SOL 2: smaller top labels
        // if (psm->tableSize < LARGE_TABLE_SIZE) {
        //     if (labelBasedOn == LABEL_AWAY)
        //             sprintf(sz, _("%d-away"), i+2);
        //     else
        //             sprintf(sz, _("%d/%d"), i2,psm->tableSize+1);
        //     gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
        //     gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz);
        // } else {
        //     // Pango.AttrList attrs = new Pango.AttrList ();
        //     // attrs.insert (Pango.attr_scale_new (Pango.Scale.SMALL));
        //     // label.attributes = attrs;
        //     if (labelBasedOn == LABEL_AWAY) {
        //                 sprintf(sz, _("%d-away"), i+2);
        //                 sprintf(sz2, _("\n%d-away"), i+2);
        //     }    else  {
        //                 sprintf(sz, _("%d/%d"), i2,psm->tableSize+1);
        //                                     sprintf(sz2, _("\n%d/%d"), i2,psm->tableSize+1);
        //     }
        //     gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
        //     gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz2);
        //     gtk_widget_modify_font((psm->apwColLabel[i2]), pango_font_description_from_string("sans 6"));
        // }
        // // SOL 3: spacing and shorten text to just the number
        //if (psm->tableSize < LARGE_TABLE_SIZE) {

        // first we build short, long and hover text; then we assign them to the labels and label hover text
        // based on the table length

        // PROBLEM 2: display the real score so as to reflect the true played match length
        // Assume that we have a true played match length of 21 and the table shows up to 7-away
        // When we display the true match score, say for 2-away, we want to show 19/21, not 5/7.
        // Problem: MATCH_SIZE(psm) gives the "fake" match size: in the example, it shows 7. This is used for evaluations.
        // Instead, we store the true match length in psm->truenMatchTo.
        // For the display, we define delta as how much to add to the computed score (shared shift):
        // - in the example above, we want to add delta=21-7=14
        // - but if e.g. the true match size is 3 and we simulate with up to 7-away, then we keep delta=0
        // i.e. we use the max of the two match sizes (true and fake) as the one to display

        char sz[100]; 
        char longsz[100];
        char hoversz[100]={""};
        char empty[100]={""};
        int delta = MAX(0, psm->truenMatchTo - MATCH_SIZE(psm)); 

        if (psm->cubeScoreMap) {
            if (psm->labelBasedOn == LABEL_AWAY)  {
                sprintf(sz, "%d", i+2);
                sprintf(longsz, _(" %d-away "), i+2);
            } else {
                sprintf(sz, "%d", i2 + delta);
                sprintf(longsz, " %d/%d ", i2 + delta, psm->tableSize + 1 + delta); 
            }
        } else {//move scoremap
            if (psm->labelBasedOn == LABEL_AWAY) {
                if (i2==0) {
                    sprintf(sz, _("1pC"));
                    sprintf(longsz, _(" post-Cr "));
                    sprintf(hoversz, _(" 1-away post-Crawford "));
                } else if (i2==1) {
                    sprintf(sz, _("1C"));
                    sprintf(longsz, _(" 1-away Cr "));
                    sprintf(hoversz, _(" 1-away Crawford "));
                } else {
                    sprintf(sz, "%d", i);
                    sprintf(longsz, _(" %d-away "), i);
                }
            } else {
                if (i2<psm->tableSize-2) {
                    sprintf(sz, "%d", i2 + delta);
                    sprintf(longsz, " %d/%d ", i2 + delta, MATCH_SIZE(psm) + delta);
                } else if (i2==psm->tableSize-2) {
                    sprintf(sz, _("%dC"), MATCH_SIZE(psm)-1 + delta);
                    sprintf(longsz, _(" %d/%d Cr "), MATCH_SIZE(psm)-1 + delta,MATCH_SIZE(psm) + delta);
                    sprintf(hoversz, _(" %d/%d Crawford "), MATCH_SIZE(psm)-1 + delta,MATCH_SIZE(psm) + delta);
                } else {
                    sprintf(sz, _("%dpC"), MATCH_SIZE(psm)-1 + delta);
                    sprintf(longsz, _(" %d/%d post-Cr "), MATCH_SIZE(psm)-1 + delta,MATCH_SIZE(psm) + delta);
                    sprintf(hoversz, _(" %d/%d post-Crawford "), MATCH_SIZE(psm)-1 + delta,MATCH_SIZE(psm) + delta);
                }
            }
        }
        if (strcmp(hoversz,empty) == 0) //if we haven't already assigned something specific to the hover text, use the long text
            strcpy(hoversz,longsz);
        // g_print("\n %s %s %s",sz,longsz,hoversz);

        // now that we build the labels and label hover text, we can assign them
        if (psm->tableSize >= LARGE_TABLE_SIZE) {
            gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
            gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz);
        } else {
            gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), longsz);
            gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), longsz);
        }
        gtk_widget_set_tooltip_markup(psm->apwRowLabel[i2], hoversz);
        gtk_widget_set_tooltip_markup(psm->apwColLabel[i2], hoversz); 

        // } else {
        //     // Pango.AttrList attrs = new Pango.AttrList ();
        //     // attrs.insert (Pango.attr_scale_new (Pango.Scale.SMALL));
        //     // label.attributes = attrs;
        //     if (labelBasedOn == LABEL_AWAY) {
        //                 sprintf(sz, _("%d-away"), i+2);
        //                 sprintf(sz2, _("\n%d-away"), i+2);
        //     }    else  {
        //                 sprintf(sz, _("%d/%d"), i2,psm->tableSize+1);
        //                                     sprintf(sz2, _("\n%d/%d"), i2,psm->tableSize+1);
        //     }
        //     gtk_label_set_text(GTK_LABEL(psm->apwRowLabel[i2]), sz);
        //     gtk_label_set_text(GTK_LABEL(psm->apwColLabel[i2]), sz2);
        //     gtk_widget_modify_font((psm->apwColLabel[i2]), pango_font_description_from_string("sans 6"));
        // }

    //         description = pango_font_description_from_string("sans");
    //         pango_font_description_set_size(description, MIN(MAX_FONT_SIZE*PANGO_SCALE,allocation.height * PANGO_SCALE / 25));
    //         layout = gtk_widget_create_pango_layout(psm->apwRowLabel[i2], NULL);
    //         pango_layout_set_font_description(layout, description);
    //             // x,y from top left of square from which to start writing
    // int x = 0.2f * allocation.width;//in my test: width=46, height=20
    // // y = 2 + (allocation.height - 4) / 10.0f;
    // int y = 0.2f * allocation.height;
    //         cairo_t * cr;
    //         pango_layout_set_markup(layout, sz,-1);
    //             gtk_locdef_paint_layout(gtk_widget_get_style(psm->apwRowLabel[i2]), gtk_widget_get_window(psm->apwRowLabel[i2]), cr, GTK_STATE_NORMAL, TRUE, NULL, psm->apwRowLabel[i2], NULL, (int) x, (int) y, layout);

    // g_object_unref(layout);
    } // end of: for i2


    if  (psm->cubeScoreMap){
        /* update gauge */
        for (i = 0; i < GAUGE_SIZE; i++) {
            DrawGauge(psm->apwFullGauge[i],i,psm->colourBasedOn);
        }
        /* update labels on gauge */
        switch (psm->colourBasedOn) {
            case PT:
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[0]), _("T"));
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[1]), "");
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[2]), "");
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[3]), _("P"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[0], _("Take"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[1], "");
                gtk_widget_set_tooltip_markup(psm->apwGauge[2], "");
                gtk_widget_set_tooltip_markup(psm->apwGauge[3], _("Pass"));
                break;
            case DND:
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[0]), _("ND"));
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[1]), "");
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[2]), "");
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[3]), _("D"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[0], _("No double"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[1], "");
                gtk_widget_set_tooltip_markup(psm->apwGauge[2], "");
                gtk_widget_set_tooltip_markup(psm->apwGauge[3], _("Double"));
                break;
            case ALL:
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[0]), _("ND"));
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[1]), _("D/T"));
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[2]), _("D/P"));
                gtk_label_set_text(GTK_LABEL(psm->apwGauge[3]), _("TG"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[0], _("No double/take"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[1], _("Double/take"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[2], _("Double/pass"));
                gtk_widget_set_tooltip_markup(psm->apwGauge[3], _("Too good to double"));
                break;
            default:
                g_assert_not_reached();
        }
    }
}


static specialscore
UpdateIsSpecialScore(const scoremap * psm, int i, int j)
/*
- In move ScoreMap, determines whether a quadrant corresponds to a special score like SCORELESS, MONEY_J, DMP, 
GG, GS; or is just REGULAR.
Note: we only apply a strict definition for cube==1. We could widen the definition if it's too strict: e.g.,
if we are 3-away 4-away and cube=4, then it's practically DMP; etc.
 - In cube ScoreMap, only determines MONEY.
i,j indicate indices in aaQuadrantData. We use i=j=-1 for the money square.
*/
{
    if (i<0 || j<0) {//both in cube and move scoremaps 
        if (psm->moneyJacoby)
            return MONEY_J;
        else
            return SCORELESS;
    }
    else if (!psm->cubeScoreMap  && psm->signednCube == 1) {//i.e. allowed square in move scoremap and the cube is centered
                    // [we do not check if the square is allowed as there are corner cases
                    // where it's unallowed then allowed again because of a change in settings
        if (i==0 && j==0)
            return DMP;
        else if ( (psm->pms->fMove && i==1 && j==2 ) || (!psm->pms->fMove && i==2 && j==1 )) //2-away 1-away Crawford, trailing
            return GG;
        else if ( (!psm->pms->fMove && i==1 && j==2 ) || (psm->pms->fMove && i==2 && j==1 )) //2-away 1-away Crawford, leading
            return GS;
    }
    return REGULAR;
}


static scorelike
UpdateIsTrueScore(const scoremap * psm, int i, int j)
/* Determines whether a quadrant corresponds to the true score.
- Does not consider whether the cube value is the same as the true cube value.
- For the money square, we use i=j=-1
*/
{
    int scoreLike = 0;
    // we now want to check whether the i-away and j-away values of a quadrant correspond to the real away values of the game.
    // We distinguish the two cases: cube scoremap and move scoremap, since they display away scores in different quadrants

    if (i<0 || j<0) //money
        return (psm->pms->nMatchTo==0) ? TRUE_SCORE : NOT_TRUE_SCORE;
    else {
        // in cube scoremap, i,j indicate indices in aaQuadrantData, i.e., they are away-scores-2.
        if (psm->cubeScoreMap)
            scoreLike = ((i+2 == psm->pms->nMatchTo-psm->pms->anScore[0]) && (j+2 == psm->pms->nMatchTo-psm->pms->anScore[1])) ? 1 : 0;
        //in move scoremap, we first check that a quadrant is allowed. If so, we distinguish the cases: 2-away and more,
        // or Crawford, or post-Crawford. It is possible to compress the formulas, but in this way we see all cases
        else if (!psm->cubeScoreMap && (psm->aaQuadrantData[i][j].isAllowedScore == ALLOWED)) {
            if (psm->pms->nMatchTo - psm->pms->anScore[0] > 1 && psm->pms->nMatchTo - psm->pms->anScore[1] > 1) //no Crawford issue
                scoreLike=((i == psm->pms->nMatchTo-psm->pms->anScore[0]) && (j == psm->pms->nMatchTo-psm->pms->anScore[1])) ? 1 : 0;
            else if (psm->pms->nMatchTo-psm->pms->anScore[0] == 1 && psm->pms->fPostCrawford == 0)
                scoreLike=((i == 1) && (j == psm->pms->nMatchTo-psm->pms->anScore[1])) ? 1 : 0; //i=1 Crawford
            else if (psm->pms->nMatchTo-psm->pms->anScore[1] == 1 && psm->pms->fPostCrawford == 0)
                scoreLike=((j == 1) && (i == psm->pms->nMatchTo-psm->pms->anScore[0])) ? 1 : 0; // j=1 Crawford
            else if (psm->pms->nMatchTo-psm->pms->anScore[0] == 1 && psm->pms->fPostCrawford == 1)
                scoreLike=((i == 0) && (j == psm->pms->nMatchTo-psm->pms->anScore[1])) ? 1 : 0; // i=0 post-C
            else if (psm->pms->nMatchTo-psm->pms->anScore[1] == 1 && psm->pms->fPostCrawford == 1)
                scoreLike=((j == 0) && (i == psm->pms->nMatchTo-psm->pms->anScore[0])) ? 1 : 0; // j=0 post-C
        } else //square is not allowed
            scoreLike = 0;
    
        //in addition, we want to take the cube value into account when deciding whether a quadrant is indeed "score-like"
        // we will require the cube situation (as shown at the bottom in the cube radio buttons) to be the same as in the true game
        if (scoreLike) {//we want to see if we need to cancel b/c the cube is not the same
            //if we are in a cube scoremap, we only check that the cube value is the same
            //if we are in a move scoremap, we also need to check that it's the same player who holds the cube
            //      In such a case, we use the formula ams.fCubeOwner= (signednCube>0) ? (ams.fMove) : (1-ams.fMove);
            // (an alternative approach would be to record the signednCube when we start the scoremap, and check that
            //      we still get the same, i.e. psm->initialSignednCube == psm->signednCube)
            if ((psm->cubeScoreMap && abs(psm->signednCube) == psm->pms->nCube) ||
                (!(psm->cubeScoreMap) && (abs(psm->signednCube) == psm->pms->nCube) && ((psm->pms->nCube == 1) ||
                                                    (psm->signednCube > 0 && psm->pms->fCubeOwner==psm->pms->fMove) ||
                                                    (psm->signednCube < 0 && psm->pms->fCubeOwner!=psm->pms->fMove)))) {
                //then it's OK, same cube value, keep scoreLike
            } else { //not the same cube value
                scoreLike = 0;
            }
        }
        if (scoreLike) {
            // g_message("nMatchTo:%d, match size:%d", psm->pms->nMatchTo,MATCH_SIZE(psm));
            return (psm->pms->nMatchTo == MATCH_SIZE(psm) || psm->labelBasedOn==LABEL_AWAY) ? TRUE_SCORE : LIKE_TRUE_SCORE;
        }
        else
            return NOT_TRUE_SCORE;    
    }
}


static void 
InitQuadrantCubeInfo(scoremap * psm, int i, int j) 
{
    /* cubeinfo */
    /* NOTE: it is important to change the score and cube value in the match, and not in the
    cubeinfo, because the gammon values in cubeinfo need to be set correctly.
    This is taken care of in GetMatchStateCubeInfo().
    */
    if (psm->cubeScoreMap) {
        // Alter the match score: we want i,j to be away scores - 2 (note match length is tableSize+1)
        // Examples:    i=0 <-> score=matchLength-2 <-> 2-away
        //              i=tableSize-1=matchLength-2 <-> score=m-1-(m-2)-1=0  <->matchLength-away
        psm->msTemp.anScore[0] = psm->tableSize - i - 1;
        psm->msTemp.anScore[1] = psm->tableSize - j - 1;
        
        /* Create cube info using this data */
        //     psm->aaQuadrantData[i][j] = (scoremap *) g_malloc(sizeof(scoremap));
        GetMatchStateCubeInfo(&(psm->aaQuadrantData[i][j].ci), & psm->msTemp);

        //initialize square properties
        psm->aaQuadrantData[i][j].isTrueScore = UpdateIsTrueScore(psm, i, j);
        psm->aaQuadrantData[i][j].isSpecialScore = UpdateIsSpecialScore(psm, i, j);
        psm->aaQuadrantData[i][j].isAllowedScore = ALLOWED;
    }
    else { //move ScoreMap
        if (isAllowed(i, j, psm)) {
            //"isAllowed()" helps set the unallowed quadrants in a *move* scoremap. Three reasons to forbid
            //  a quadrant:
            // 1. we don't allow i=0 i.e. 1-away Crawford with j=1 i.e. 1-away post-Crawford,
            //          and more generally incompatible scores in i and j
            // 2 we don't allow doubling in a Crawford game
            // 3 we don't allow an absurd cubing situation, eg someone leading in Crawford
            //          with a >1 cube, or someone at matchLength-2 who would double to 4

            // Here we found that there may be a bug with the confusion b/w fCrawford and fPostCrawford
            // Using fCrawford while we should use the other...
            // We set ams.fCrawford=1 for the Crawford games and 0 for the post-Crawford games
            psm->msTemp.fCrawford = (i == 1 || j == 1); //Crawford by default, unless i==0 or j==0
            psm->msTemp.fPostCrawford = (i == 0 || j == 0); //Crawford by default, unless i==0 or j==0
            if (i == 0) { //1-away post Crawford
                psm->msTemp.anScore[0] = MATCH_SIZE(psm) - 1;
            }
            else  //i-away Crawford
                psm->msTemp.anScore[0] = MATCH_SIZE(psm) - i;
            if (j == 0) { //1-away post Crawford
                psm->msTemp.anScore[1] = MATCH_SIZE(psm) - 1;
            }
            else  //j-away
                psm->msTemp.anScore[1] = MATCH_SIZE(psm) - j;
        
            // // Create cube info using this data
            //     psm->aaQuadrantData[i][j] = (scoremap *) g_malloc(sizeof(scoremap));
            GetMatchStateCubeInfo(&(psm->aaQuadrantData[i][j].ci), & psm->msTemp);

            //we also want to update the "special" quadrants, i.e. those w/ the same score as currently,
            // or DMP etc.
            // note that if the cube changes, e.g. 1-away 1-away is not DMP => this depends on the cube value
            psm->aaQuadrantData[i][j].isTrueScore = UpdateIsTrueScore(psm, i, j);
            psm->aaQuadrantData[i][j].isSpecialScore = UpdateIsSpecialScore(psm, i, j);
        } else {
            // we decide to arbitrarily mark all unallowed squares as not special in any way
            psm->aaQuadrantData[i][j].isTrueScore = NOT_TRUE_SCORE;
            psm->aaQuadrantData[i][j].isSpecialScore = REGULAR; //if the cube makes the position unallowed
            // (eg I cannot double to 4 when 1-away...) we don't want to mark any squares as special
            
        }   
    }
}

// void 
// InitQuadrantCubeInfo(scoremap * psm, int i, int j) 
// {

//     if (isAllowed(i, j, psm)) {
//         //"isAllowed()" helps set the unallowed quadrants in *move* scoremap. Three reasons to forbid
//         //  a quadrant:
//         // 1. we don't allow i=0 i.e. 1-away Crawford with j=1 i.e. 1-away post-Crawford,
//         //          and more generally incompatible scores in i and j
//         // 2 we don't allow doubling in a Crawford game
//         // 3 we don't allow an absurd cubing situation, eg someone leading in Crawford
//         //          with a >1 cube, or someone at matchLength-2 who would double to 4

//     /* cubeinfo */
//     /* NOTE: it is important to change the score and cube value in the match, and not in the
//     cubeinfo, because the gammon values in cubeinfo need to be set correctly.
//     This is taken care of in GetMatchStateCubeInfo().
//     */
//         if (psm->cubeScoreMap) {
//             // Alter the match score: we want i,j to be away scores - 2 (note match length is tableSize+1)
//             // Examples:    i=0 <-> score=matchLength-2 <-> 2-away
//             //              i=tableSize-1=matchLength-2 <-> score=m-1-(m-2)-1=0  <->matchLength-away
//             psm->msTemp.anScore[0] = psm->tableSize - i - 1;
//             psm->msTemp.anScore[1] = psm->tableSize - j - 1;
//         }
//         else { //move ScoreMap
//             // Here we found that there may be a bug with the confusion b/w fCrawford and fPostCrawford
//             // Using fCrawford while we should use the other...
//             // We set ams.fCrawford=1 for the Crawford games and 0 for the post-Crawford games
//             psm->msTemp.fCrawford = (i == 1 || j == 1); //Crawford by default, unless i==0 or j==0
//             psm->msTemp.fPostCrawford = (i == 0 || j == 0); //Crawford by default, unless i==0 or j==0
//             if (i == 0) { //1-away post Crawford
//                 psm->msTemp.anScore[0] = MATCH_SIZE(psm) - 1;
//             }
//             else  //i-away Crawford
//                 psm->msTemp.anScore[0] = MATCH_SIZE(psm) - i;
//             if (j == 0) { //1-away post Crawford
//                 psm->msTemp.anScore[1] = MATCH_SIZE(psm) - 1;
//             }
//             else  //j-away
//                 psm->msTemp.anScore[1] = MATCH_SIZE(psm) - j;
//         }
//         // // Create cube info using this data
//         //     psm->aaQuadrantData[i][j] = (scoremap *) g_malloc(sizeof(scoremap));
//         GetMatchStateCubeInfo(&(psm->aaQuadrantData[i][j].ci), & psm->msTemp);

//         //we also want to update the "special" quadrants, i.e. those w/ the same score as currently,
//         // or DMP etc.
//         // note that if the cube changes, e.g. 1-away 1-away is not DMP => this depends on the cube value
//         psm->aaQuadrantData[i][j].isTrueScore = UpdateIsTrueScore(psm, i, j, FALSE);
//         psm->aaQuadrantData[i][j].isSpecialScore = UpdateIsSpecialScore(psm, i, j, FALSE);
//     }
//     else {
//         // we decide to arbitrarily mark all unallowed squares as not special in any way
//         psm->aaQuadrantData[i][j].isTrueScore = NOT_TRUE_SCORE;
//         psm->aaQuadrantData[i][j].isSpecialScore = REGULAR; //if the cube makes the position unallowed
//         // (eg I cannot double to 4 when 1-away...) we don't want to mark any squares as special
        
//     }   
// }


static int
CalcEquities(scoremap * psm, int oldSize, int updateMoneyOnly, int calcOnly)

/* Iterates through scores:
    1) Creates the cubeinfo array, based on the given cube value -- i.e., updates psm->aaQuadrantData[i][j].ci
        - We use updateMoneyOnly when a user toggles/untoggles the Jacoby option and we want to recompute the top-left (money)
            quadrant only, not the whole scoremap
        - Also, in the move scoremap, both players can double, so non-1 cube values could be negative to indicate
            the player who doubles (e.g. 2,-2,4,-4 etc.)
    2) Finds equities at each score, and use these to set the text for the corresponding box.
        - Does not update the gui.
        - Only does entries in the table >= oldSize. (scomputing old values when resizing the table.)
        - In the move scoremap, it also computes the most frequent best moves across the scoremap
    When calcOnly is set: only do the 2nd step (e.g. if we only changed the ply, no need to recompute the cubeinfo array)
*/
{
    /* first free the allocation in money quadrant if it's recomputed (in move scoremap, because 
    findnSaveBestMoves does a malloc) 
    */    
        
    if (!psm->cubeScoreMap && (oldSize==0 || updateMoneyOnly) && psm->moneyQuadrantData.ml.cMoves < IMPOSSIBLE_CMOVES) {
        //g_message("money freed: cMoves=%d",psm->moneyQuadrantData.ml.cMoves);
        g_free(psm->moneyQuadrantData.ml.amMoves);
        psm->moneyQuadrantData.ml.cMoves = IMPOSSIBLE_CMOVES;
    }

    if(!calcOnly) {
        // matchstate ams = (*psm->pms); // Make a copy of the "master" matchstate  
        //         //[note: backgammon.h defines an extern ms => using a different name]
        // psm->pmsTemp = &ams; 
        //psm->pmsTemp->nMatchTo = MATCH_SIZE(psm); // Set the match length
        psm->msTemp.nCube = abs(psm->signednCube); // Set the cube value
        if (psm->signednCube == 1) // Cube value 1: centre the cube
            psm->msTemp.fCubeOwner = -1;
        else // Cube value > 1: Uncentre the cube. Ensure that the player on roll owns the cube (and thus can double)
            psm->msTemp.fCubeOwner = (psm->signednCube > 0) ? (psm->msTemp.fMove) : (1 - psm->msTemp.fMove);

        /*top-left quadrant: we always recompute the money-play value; now set whether it's money play or an unlimited match 
        in mps->pmsTemp we only change nMatchTo and fJacoby; below for non-money-play we change back nMatchTo, but don't 
        touch fJacoby, as we assume that it has no impact there*/
        if (oldSize==0 || updateMoneyOnly) { //if the money square equity wasn't already computed, or we ask for recomputation
            if (psm->labelTopleft == MONEY_JACOBY) {
                psm->msTemp.nMatchTo = 0;
                psm->msTemp.fJacoby = 1;
            }
            else if (psm->labelTopleft == MONEY_NO_JACOBY) {
                psm->msTemp.nMatchTo = 0;
                psm->msTemp.fJacoby = 0; //disabling the impact of Jacoby
            }
            GetMatchStateCubeInfo(&(psm->moneyQuadrantData.ci), & psm->msTemp); 
            psm->moneyQuadrantData.isTrueScore=UpdateIsTrueScore(psm, -1, -1);
            psm->moneyQuadrantData.isSpecialScore=UpdateIsSpecialScore(psm, -1, -1);
            psm->moneyQuadrantData.isAllowedScore=ALLOWED;
        }
    }
    if (updateMoneyOnly) {
        //we only recompute the top-left money square, and don't bother with the progress bar
        CalcQuadrantEquities(&(psm->moneyQuadrantData), psm, TRUE); // Recalculate money equity.
        // g_message("new money cMoves=%d",psm->moneyQuadrantData.ml.cMoves);
    } else {
        //recompute fully (beyond oldSize); we only recompute the money equity when oldSize==0
        ProgressStartValue(_("Finding correct decisions"), (oldSize==0)? psm->tableSize*psm->tableSize + 1 : MAX(psm->tableSize*psm->tableSize-oldSize*oldSize, 1) );
        //g_print("Finding %d-ply cube equities:\n",pec->nPlies);

        /* We start by computing the money-play value, since if the user stops the process
        in the middle, it's often the most useful to display and therefore to compute first*/
        //if(oldSize == 0 || oldSize == psm->tableSize)  //causes bug: it colors the cell in dark grey, and doesn't show a move
                            //maybe the moneyQuadrantData becomes empty?
        if (oldSize==0) {  //if the money square equity wasn't already computed [we are in the !updateMoneyOnly case]
            CalcQuadrantEquities(&(psm->moneyQuadrantData), psm, TRUE);
                    // g_message("new money cMoves=%d",psm->moneyQuadrantData.ml.cMoves);
            /*careful: upon table scaling, ProgressValueAdd causes a redraw => a potential problem  in the pango markup text because it may not be ready yet!*/
            ProgressValueAdd(1);//not sure if it should be included above, but probably minor
        }

        /*
        Next, we fill the table. i,j correspond to the locations in the table. 
            - In cube ScoreMap, the away-scores are 2+i, 2+j (because the (0,0)-entry of 
                the table corresponds to 2-away 2-away).
            - In move scoremap: i=0->1-away post Crawford; i=1->1-away Crawford; 
                i=2+->i-away

        In a first version, we incremented i from 1 to tableSize, then j similarly, 
        i.e. row by row in (i,j) (or col by col as seen by the user with "away" labels).
        
        But if the user interrupts the process in the middle, maybe it's better to progress 
        using growing squares or by computing the diagonal first. Here we implement the 
        growing squares. (Thanks to Philippe Michel for the feedback!)

        In a third version, instead of first initializing all of the cubeinfo values before starting a 
        progress bar, which may upset the user since the bar doesn't seem to start, we now
        compute both at the same : (1) the cubeinfo and (2) the resulting equity values; so we need 
        to combine the (aux,aux2) indexation values (which are used for expanding squares) with the 
        (i,j) ones. 
        */
        if(!calcOnly) {
            psm->msTemp.nMatchTo = MATCH_SIZE(psm); // Set the match length
            // psm->pmsTemp->nCube = abs(psm->signednCube); // Set the cube value
            // if (psm->signednCube == 1) // Cube value 1: centre the cube
            //     psm->pmsTemp->fCubeOwner = -1;
            // else // Cube value > 1: Uncentre the cube. Ensure that the player on roll owns the cube (and thus can double)
            //     psm->pmsTemp->fCubeOwner = (psm->signednCube > 0) ? (psm->pmsTemp->fMove) : (1 - psm->pmsTemp->fMove);

        }
        for (int aux=oldSize; aux<psm->tableSize; aux++) {
            for (int aux2=aux; aux2>=0; aux2--) {
                /* first we free the malloc with the equity that may have been provided previously
                */
                if(!psm->cubeScoreMap && psm->aaQuadrantData[aux2][aux].ml.cMoves < IMPOSSIBLE_CMOVES) {
                    // g_message("DELETED+FREED: aux2=%d, aux=%d, tableSize=%d, moves=%d......",aux2,aux,psm->tableSize,psm->aaQuadrantData[aux2][aux].ml.cMoves);
                    g_free(psm->aaQuadrantData[aux2][aux].ml.amMoves);
                    psm->aaQuadrantData[aux2][aux].ml.cMoves = IMPOSSIBLE_CMOVES;
                }
                /*Note: in move scoremaps, we check a square validity in InitQuadrantCubeInfo() -> isAllowed();
                    [and maybe do so again implicitly in  CalcQuadrantEquities()->FindnSaveBestMoves()]
                However in cube scoremaps, we only do it later in CalcQuadrantEquities()->GetDPEq()
                */               
                if(!calcOnly) // skip with ScoreMapPlyToggled
                    InitQuadrantCubeInfo(psm, aux2, aux);
                if (psm->aaQuadrantData[aux2][aux].isAllowedScore == ALLOWED || psm->cubeScoreMap) {
                    //Only running the line below when (i >= oldSize || j >= oldSize) 
                    // [now aux>=oldSize] yields a bug with grey squares on resize
                    // if(aux>=oldSize) {
                        CalcQuadrantEquities(&psm->aaQuadrantData[aux2][aux], psm, (aux >= oldSize));
                    // if (myDebug)
                    //     g_print("i=%d,j=%d,FindnSaveBestMoves returned %d, %1.3f; decision: %s\n",i,j,psm->aaQuadrantData[i][j].ml.cMoves,psm->aaQuadrantData[i][j].ml.rBestScore,psm->aaQuadrantData[i][j].decisionString);
                    // if (aux >= oldSize) // Only count the ones where equities are recomputed (other ones occur near-instantly)
                        ProgressValueAdd(1);
                    // }
                }
                else {
                    strcpy(psm->aaQuadrantData[aux2][aux].decisionString, "");
                    // psm->aaQuadrantData[aux2][aux].ml.cMoves=0; //also used for DestroyDialog
                }
                if (aux2<aux) {     //same as above but now doing the other side of the square, without the 
                                    //(aux,aux) vertex on the diagonal
                    if(!psm->cubeScoreMap && psm->aaQuadrantData[aux][aux2].ml.cMoves < IMPOSSIBLE_CMOVES) {
                        // g_message("DELETED+FREED: aux=%d, aux2=%d, tableSize=%d, moves=%d......",aux,aux2,psm->tableSize,psm->aaQuadrantData[aux][aux2].ml.cMoves);
                        g_free(psm->aaQuadrantData[aux][aux2].ml.amMoves);
                        psm->aaQuadrantData[aux][aux2].ml.cMoves = IMPOSSIBLE_CMOVES;
                    }
                    if(!calcOnly) 
                        InitQuadrantCubeInfo(psm, aux, aux2);
                    if (psm->aaQuadrantData[aux][aux2].isAllowedScore == ALLOWED || psm->cubeScoreMap) {
                        // if(aux>=oldSize) {
                            CalcQuadrantEquities(&psm->aaQuadrantData[aux][aux2], psm, (aux >= oldSize));
                        // if (aux >= oldSize)
                            ProgressValueAdd(1);
                        // }
                    }
                    else {
                        strcpy(psm->aaQuadrantData[aux][aux2].decisionString, "");
                        //psm->aaQuadrantData[aux][aux2].ml.cMoves=0; //also used for DestroyDialog                        
                    }
                }
            }
        }
        /* if we show the true score in the axes and not the away score: when we scale up a table, a "current" score in a 5-point match becomes a "similar" score
                in a 7-pt match; but we don't currently check that, as DMP, GG, GS etc don't change => check this case only */
        for (int i = 0; i < psm->tableSize; i++) {
            for (int j = 0; j < psm->tableSize; j++) {
                psm->aaQuadrantData[i][j].isTrueScore = UpdateIsTrueScore(psm, i, j);
            }
        }
        
        ProgressEnd();

    }

    // if(!calcOnly) {
    //     for (int i = 0; i < psm->tableSize; i++) {
    //         for (int j = 0; j < psm->tableSize; j++) {
 
    //     }
    // }
    /* **************************** */


    if (!psm->cubeScoreMap)
        FindMostFrequentMoves(psm);
    
    return 0;
}

static void
CutTextTo(char *output, const char * input, const size_t cutoff) {
/* This function cuts the input string so as to display it in two rows whenever it is too long. (longer than cutoff)
Specifically: when the decision string length is beyond some cutoff value, we look for the first space in
    the 2nd half of its length to cut it.
The resulting string is written to "output".
*/
    size_t len = strlen(input);
    strcpy(output, input); // First copy the whole thing over
    if (len <= cutoff) // Already short enough
        return;

    char *pch = strchr(output+(len+1)/2, ' '); //finds the 1st space in the 2nd half of the string

    if (pch == NULL) { //nothing in 2nd half
        size_t halfLen = (len+1)/2;
	char *prefix = g_strndup(input, halfLen);

        pch = strrchr(prefix,' '); // find last space in 1st half
        if (pch == NULL) // No space => no cut.
            return;
        else {
            pch = output + (pch-prefix);       // now point to the corresponding character of output string
        }
    g_free(prefix);
    }
    *pch = '\n'; // Replace the space with a newline
}

static gboolean
DrawQuadrant(GtkWidget * pw, cairo_t * cr, scoremap * psm) {
/* This is called by gtk when the object is drawn (for certain GTK versions)
Alternatively, it is called from ExposeQuadrant, which is called by gtk.
(This is triggered at the end of UpdateScoreMapVisual.)
The function updates the decision text in each square.
*/
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    const quadrantdata * pq;
    PangoLayout *layout;
    PangoFontDescription *description;
    int i=0,j=0, x,y;
    GtkAllocation allocation;
    gtk_widget_get_allocation(pw, &allocation);
    char buf[200];
    char aux[100];
    //gchar * tmp;
    float fontsize;

    int width, height;
    float rescaleFactor;

#if GTK_CHECK_VERSION(3,0,0)
        width = gtk_widget_get_allocated_width(pw);
        height = gtk_widget_get_allocated_height(pw);
#else
        width = allocation.width;
        height = allocation.height;
#endif

    // Paint the drawing area with the appropriate colour
    float *rval = g_object_get_data(G_OBJECT(pw), "rval");
    float *gval = g_object_get_data(G_OBJECT(pw), "bval");
    float *bval = g_object_get_data(G_OBJECT(pw), "gval");

    if (rval!=NULL) {
        cairo_rectangle(cr, 0, 0, width, height);

        cairo_set_line_width(cr, 0);
        cairo_set_source_rgb(cr, *rval, *gval, *bval);
        cairo_fill_preserve(cr);

        rval = g_object_get_data(G_OBJECT(pw), "borderrval");
        if (rval!=NULL && *rval>=0) {
            gval = g_object_get_data(G_OBJECT(pw), "borderbval");
            bval = g_object_get_data(G_OBJECT(pw), "bordergval");
            cairo_set_source_rgb(cr, *rval, *gval, *bval);
            cairo_set_line_width(cr, 2*BORDER_SIZE);
            cairo_stroke_preserve(cr);
            cairo_set_source_rgb(cr, 0,0,0); // Black border
            cairo_set_line_width(cr, 1);
            cairo_stroke_preserve(cr);
        }
#if GTK_CHECK_VERSION(3,0,0)
        gtk_render_frame(gtk_widget_get_style_context(pw), cr, 0, 0, width, height);
#endif
    }


    if (pi == NULL)
        return TRUE;

    if (*pi >= 0) {
        i = (*pi) / MAX_TABLE_SIZE;
        j = (*pi) % MAX_TABLE_SIZE;
        // Replace by actual away score
        i = (psm->labelBasedOn == LABEL_AWAY ? i : psm->tableSize-i-1);
        j = (psm->labelBasedOn == LABEL_AWAY ? j : psm->tableSize-j-1);
        pq=&psm->aaQuadrantData[i][j];
    } else {
        pq=&psm->moneyQuadrantData;
    }

    description = pango_font_description_from_string("sans");
    fontsize=MAX(MIN_FONT_SIZE, MIN(MAX_FONT_SIZE, (float)allocation.height/7.0f)); // Set text height as function of the quadrant height
    pango_font_description_set_size(description, (gint)(fontsize*PANGO_SCALE));

    layout = gtk_widget_create_pango_layout(pw, NULL);
    pango_layout_set_font_description(layout, description);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    //pango_font_description_free (description);

    //g_print("... %s:%s",pq->decisionString,pq->equityText);
    /* build the text that will be displayed in the quadrant

    Version 1: we don't want to display text if (1) we are in the middle of scaling up the table size (psm->tempScaleUp == 1) 
    and (2) we display a square for which we haven't computed the decision yet, i.e. beyond psm->oldTableSize; 
    also within this function, i,j are not well defined for the money square, so we make sure to allow text
    for the money square by using the identifying (*pi < 0) from above

    Version 2: we disregard scaling up the table and remove this condition.
    */
            // if ((! (psm->tempScaleUp && (i>=psm->oldTableSize || j>=psm->oldTableSize))) || (*pi < 0)) {
    if (pq->isAllowedScore==ALLOWED) { // non-greyed quadrant
        // we start by cutting long decision strings into two rows; this is only relevant to the move scoremap
        CutTextTo(aux, pq->decisionString, 12);

        if ((psm->cubeScoreMap && psm->displayCubeEval != CUBE_NO_EVAL) || (!psm->cubeScoreMap && psm->displayMoveEval != MOVE_NO_EVAL))  {
            strcat(aux, pq->equityText); //was strcat
        }

        if (pq->isTrueScore) { //emphasize the true-score square
            strcpy(buf, "<b><span size=\"x-small\">");
            if (pq->ci.nMatchTo == 0) { //money like the current real game, but maybe with a different Jacoby option
                //if (pq->ci.fJacoby == moneyJacoby) //money play has same Jacoby settings as now
                //    strcat(buf, _("Current: "));

                if (psm->moneyJacoby)  // we analyze with Jacoby
                    strcat(buf, _("Money")); 
                else
                    strcat(buf, _("Unlimited"));
            }
            else if (pq->isTrueScore == TRUE_SCORE)
                strcat(buf, _("Current score")); // or "(Same score)"
            else if (pq->isTrueScore == LIKE_TRUE_SCORE)
                strcat(buf, _("Similar score")); // "like current score" or "like game score" are a bit long
            strcat(buf, "</span>\n");
            strcat(buf, aux);	//pq->decisionString);
            strcat(buf, "</b>");
            // pango_layout_set_markup(layout, buf, -1);
        } else if (pq->isSpecialScore != REGULAR) { //the square is special: MONEY, DMP etc. => write in text             
            if (pq->isSpecialScore == MONEY_J) 
                strcpy(buf, _("Money"));
            else if (pq->isSpecialScore == SCORELESS)
                strcpy(buf, _("Unlimited"));
            else if (pq->isSpecialScore == DMP)
                strcpy(buf, _("DMP"));
            else if (pq->isSpecialScore == GG)
                strcpy(buf, _("GG"));
            else if (pq->isSpecialScore == GS)
                strcpy(buf, _("GS"));
            strcat(buf, "\n");
            strcat(buf, aux);	//pq->decisionString);
        } else { //regular (not MONEY, DMP, GG, GS), not true score
            strcpy(buf, aux);	//pq->decisionString);
        }
    } else {
        strcpy(buf, "");//e.g. grey squares that do not make sense
    }
    // pango_layout_set_markup(layout, buf, -1);
    // } else {
    //     strcpy(buf, "");
    //     // pango_layout_set_text(layout, buf, -1);
    // }
    //strcat(buf,NULL);
    //tmp = g_markup_escape_text(buf, -1); // to avoid non-NULL terminated messages -> didn't work, displays the "html"
    //if (buf != (const char *) NULL) // this one also didn't work, everything goes to the "else" 
    pango_layout_set_markup(layout, buf, -1);
    // else
    //     pango_layout_set_text(layout, tmp, -1);   
    //g_free(tmp);
    //g_message("buf: %d->%s\n",*pi , buf);

    pango_layout_get_size(layout, &width, &height); /* Find the size of the text */
    /* Note these sizes are PANGO_SCALE * number of pixels, whereas allocation.width/height are in pixels. */

    if (width/PANGO_SCALE > allocation.width-4 || height/PANGO_SCALE > allocation.height-4) { /* Check if the text is too wide (we'd like at least a 2-pixel border) */
        /* If so, rescale it to achieve a 2-pixel border. */
        /* question: why not do a while instead of the if?
            answer: The code immediately after sets the font size so that it does exactly fit.
                    No need to keep looping to check again.*/
        if (width>0 && height>0)
            rescaleFactor=fminf(((float)allocation.width-4.0f)*(float)PANGO_SCALE/((float)width),((float)allocation.height-4.0f)*(float)PANGO_SCALE/((float)height));
        else {
            g_message("error zero width/height\n");
            rescaleFactor=0.5f;
            }
        pango_font_description_set_size(description, (gint)(fontsize*rescaleFactor*PANGO_SCALE));
        //g_print("font size: %d, rescale factor: %f\n",(int)(fontsize*rescaleFactor*PANGO_SCALE),rescaleFactor);
        pango_layout_set_font_description(layout,description);

        /* Measure size again, so that centering works right. */
        pango_layout_get_size(layout, &width, &height);
    }

    /* Compute where the top-right corner should go, so that the text is centred. */
    x=(allocation.width - width/PANGO_SCALE)/2;
    y=(allocation.height - height/PANGO_SCALE)/2;

#if GTK_CHECK_VERSION(3,0,0)
    gtk_render_layout(gtk_widget_get_style_context(pw), cr, (double)x, (double)y, layout);
#else
    gtk_paint_layout(gtk_widget_get_style(pw), gtk_widget_get_window(pw), GTK_STATE_NORMAL, TRUE, NULL, pw, NULL, (int) x, (int) y, layout);
#endif
    pango_font_description_free (description);
    g_object_unref(layout);

    return TRUE;
}


#if ! GTK_CHECK_VERSION(3,0,0)
static void
ExposeQuadrant(GtkWidget * pw, GdkEventExpose * UNUSED(pev), scoremap * psm)
/* Called by gtk. */
{
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(pw));
    DrawQuadrant(pw, cr, psm);
    cairo_destroy(cr);
}
#endif

// #if ! GTK_CHECK_VERSION(3,0,0)
// static void
// ExposeQuadrant2(GtkWidget * pw, GdkEventExpose * UNUSED(pev), scoremap * psm)
// /* Called by gtk. */
// {
//     cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(pw));
//     DrawQuadrant2(pw, cr, psm);
//     cairo_destroy(cr);
// }
// #endif

#if 0
//this is only used to obtain a widget size after drawing for debugging purposes
// call it using something like:
//      g_signal_connect(pq->pDrawingAreaWidget, "size-allocate", G_CALLBACK(GetWidgetSize), NULL);

static void
GetWidgetSize(GtkWidget *UNUSED(widget), GtkAllocation *allocation, void *UNUSED(data)) {
    g_print("width = %d, height = %d\n", allocation->width, allocation->height);
}
#endif

static void
InitQuadrant(gtkquadrant * pq, GtkWidget * ptable, scoremap * psm, const int quadrantWidth, const int quadrantHeight, const int i, const int j, const int money)
/* Initialize the given gtk quadrant: create the gtk components, place them in the table, tell the quadrant where it is.
*/
{

    GtkWidget * pwv;
    GtkWidget * pwh;
    GtkWidget * pwv2;
    GtkWidget * pwh2;
//   int index;

    // start by creating container in scoremap quadrant (square)
    pq->pContainerWidget = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pq->pContainerWidget), FALSE);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(ptable), pq->pContainerWidget, money ? 1 : i + 2, money ? 1 : j + 2, 1, 1);
    gtk_widget_set_hexpand(pq->pContainerWidget, TRUE);
#else
    gtk_table_attach_defaults(GTK_TABLE(ptable), pq->pContainerWidget, money ? 1 : i + 2, money ? 2 : i + 3, money ? 1 : j + 2, money ? 2 : j + 3);
#endif

    // gtk_container_set_border_width(GTK_CONTAINER(pq->pContainerWidget),2);

        // GdkColor red = {0, 0xffff, 0x0000, 0x0000};
        // gtk_widget_modify_bg(pq->pContainerWidget, GTK_STATE_NORMAL, &red);

    /* next, put the borders and the main quadrant in the container:
        |-------  border[0] -----------|
        |border[1]  main     border [2]|
        |-------  border[3] -----------|

    ! Since we want to create 2 sets of borders (black outside, colored inside), we create and put as Russian dolls: pwv > pwh > pwv2 > pwh2

    */
#if GTK_CHECK_VERSION(3,0,0)
        pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
        pwv = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pq->pContainerWidget), pwv);
    // gtk_container_set_border_width(GTK_CONTAINER(pwh), 2);


//     pq->pDrawingAreaWidgetBorder[0] = gtk_drawing_area_new();
//     gtk_box_pack_start(GTK_BOX(pwv), pq->pDrawingAreaWidgetBorder[0], FALSE, FALSE, BORDER_PADDING);//0
//     // gtk_container_add(GTK_CONTAINER(pq->pContainerWidget), pq->pDrawingAreaWidget);
//     gtk_widget_set_size_request(pq->pDrawingAreaWidgetBorder[0], BORDER_SIZE, BORDER_SIZE);

#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwh = gtk_hbox_new(FALSE, 0);//parameters: homogeneous; spacing
#endif
    gtk_box_pack_start(GTK_BOX(pwv), pwh, TRUE, TRUE, 0);//parameters: expand, fill, padding



#if GTK_CHECK_VERSION(3,0,0)
        pwv2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
        pwv2 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwh), pwv2, TRUE, TRUE, 0);//parameters: expand, fill, padding


#if GTK_CHECK_VERSION(3,0,0)
    pwh2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwh2 = gtk_hbox_new(FALSE, 0);//parameters: homogeneous; spacing
#endif
    gtk_box_pack_start(GTK_BOX(pwv2), pwh2, TRUE, TRUE, 0);//parameters: expand, fill, padding


    // pq->pDrawingAreaWidgetBorder[1] = gtk_drawing_area_new();
    // gtk_box_pack_start(GTK_BOX(pwh), pq->pDrawingAreaWidgetBorder[1], FALSE, FALSE, BORDER_PADDING);
    // gtk_widget_set_size_request(pq->pDrawingAreaWidgetBorder[1], BORDER_SIZE, BORDER_SIZE);

    //main
    pq->pDrawingAreaWidget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(pwh2), pq->pDrawingAreaWidget, TRUE, TRUE, 0);//0
    // gtk_container_add(GTK_CONTAINER(pq->pContainerWidget), pq->pDrawingAreaWidget);
    gtk_widget_set_size_request(pq->pDrawingAreaWidget, quadrantWidth, quadrantHeight);

    // this helps print the final allocated value if needed:
    // g_signal_connect(pq->pDrawingAreaWidget, "size-allocate", G_CALLBACK(GetWidgetSize), NULL);

    // gtk_window_set_default_size(pq->pDrawingAreaWidget, quadrantWidth, quadrantHeight);


    // pq->pDrawingAreaWidgetBorder[2] = gtk_drawing_area_new();
    // gtk_box_pack_start(GTK_BOX(pwh), pq->pDrawingAreaWidgetBorder[2], FALSE, FALSE, BORDER_PADDING);
    // gtk_widget_set_size_request(pq->pDrawingAreaWidgetBorder[2], BORDER_SIZE, BORDER_SIZE);

    // pq->pDrawingAreaWidgetBorder[3] = gtk_drawing_area_new();
    // gtk_box_pack_start(GTK_BOX(pwv), pq->pDrawingAreaWidgetBorder[3], FALSE, FALSE, BORDER_PADDING);
    // gtk_widget_set_size_request(pq->pDrawingAreaWidgetBorder[3], BORDER_SIZE, BORDER_SIZE);

    // Set user_data to an encoding of which grid entry this is
    int * pi = (int *) g_malloc(sizeof(int));
    *pi = money ? -1 : i * MAX_TABLE_SIZE + j;

    g_object_set_data_full(G_OBJECT(pq->pDrawingAreaWidget), "user_data", pi, g_free);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(pq->pDrawingAreaWidget), "gnubg-score-map-quadrant");
        g_signal_connect(G_OBJECT(pq->pDrawingAreaWidget), "draw", G_CALLBACK(DrawQuadrant), psm);
#else
        g_signal_connect(G_OBJECT(pq->pDrawingAreaWidget), "expose_event", G_CALLBACK(ExposeQuadrant), psm);
#endif

//     // int * piArr[4];
//     for (index=0; index<4; index++) {
//     // // Set user_data to an encoding of which grid entry this is
//     //     piArr[index] = (int *) g_malloc(sizeof(int));
//     //     *piArr[index] = money ? -1 : i * MAX_TABLE_SIZE + j; 

//         g_object_set_data(G_OBJECT(pq->pDrawingAreaWidgetBorder[index]), "user_data", NULL);
// #if GTK_CHECK_VERSION(3,0,0)
//             gtk_style_context_add_class(gtk_widget_get_style_context(pq->pDrawingAreaWidgetBorder[index]), "gnubg-score-map-quadrant");
//             g_signal_connect(G_OBJECT(pq->pDrawingAreaWidgetBorder[index]), "draw", G_CALLBACK(DrawQuadrant), psm);
// #else
//             g_signal_connect(G_OBJECT(pq->pDrawingAreaWidgetBorder[index]), "expose_event", G_CALLBACK(ExposeQuadrant), psm);
// #endif
//     }
#if 0
    int * piArr[4];
    int * piOuterArr[4];

    for (index = 0; index < 4; index++) {
    // Set user_data to an encoding of which grid entry this is
        piArr[index] = (int *) g_malloc(sizeof(int));
        piArr[index] = NULL; // this is used in DrawQuadrant to draw only and not try to write text in the border
        // *piArr[index] = money ? -1 : i * MAX_TABLE_SIZE + j;

        piOuterArr[index] = (int *) g_malloc(sizeof(int));
        piOuterArr[index] = NULL; // this is used in DrawQuadrant to draw only and not try to write text in the outer border

        // *piOuterArr[index] = money ? -1 : i * MAX_TABLE_SIZE + j;
    }
#endif

    // gtk_box_set_homogeneous (GTK_BOX(pwh),FALSE);
}

static void
BuildTableContainer(scoremap * psm, int oldSize)
/* Creates the GTK table (or grid starting wth GTK3).
   A pointer to the table is put into psm->pwTable / psm->pwGrid, and it is placed in psm->pwTableContainer (to be displayed).
   Only does entries in the table >= oldSize. (Avoid computing old values when resizing the table.)
*/
{
    int tableSize = psm->tableSize;

    int quadrantHeight = MIN(MAX_SIZE_QUADRANT, MAX_TABLE_HEIGHT/tableSize);
    int quadrantWidth = MIN(MAX_SIZE_QUADRANT, MAX_TABLE_WIDTH/tableSize);
    int i,j;
    GtkWidget * pw;

#if GTK_CHECK_VERSION(3,0,0)
    psm->pwGrid = gtk_grid_new();

    gtk_grid_set_column_homogeneous(GTK_GRID(psm->pwGrid), ((psm->cubeScoreMap) ? FALSE : TRUE));
    gtk_grid_set_row_homogeneous(GTK_GRID(psm->pwGrid), ((psm->cubeScoreMap) ? FALSE : TRUE));

    gtk_grid_set_row_spacing(GTK_GRID(psm->pwGrid), TABLE_SPACING);
    gtk_grid_set_column_spacing(GTK_GRID(psm->pwGrid), TABLE_SPACING);
#else
    psm->pwTable = gtk_table_new(tableSize+2, tableSize+2, ((psm->cubeScoreMap) ? FALSE : TRUE));

    gtk_table_set_row_spacings (GTK_TABLE(psm->pwTable), TABLE_SPACING);
    gtk_table_set_col_spacings (GTK_TABLE(psm->pwTable), TABLE_SPACING);
#endif

    //    gtk_container_add(GTK_CONTAINER(psm->pwvcnt), psm->pwTable);

    //    gtk_container_set_border_width (GTK_CONTAINER(psm->pwvcnt),10);
        // gtk_box_pack_start(GTK_BOX(psm->pwv), psm->pwTable, FALSE, FALSE, 0);
        // gtk_widget_show_all(psm->pwv);

    /* drawing areas: starting with money square, then moving to (i,j) */

    
#if GTK_CHECK_VERSION(3,0,0)
    InitQuadrant( & psm->moneygQuadrant, psm->pwGrid, psm, quadrantWidth, quadrantHeight, 0, 0, TRUE);
#else
    InitQuadrant( & psm->moneygQuadrant, psm->pwTable, psm, quadrantWidth, quadrantHeight, 0, 0, TRUE);
#endif
    if(oldSize==0) {
        strcpy(psm->moneyQuadrantData.decisionString,"");
        strcpy(psm->moneyQuadrantData.equityText,"");
        strcpy(psm->moneyQuadrantData.unallowedExplanation,"");
    }

    for (i = 0; i < tableSize; ++i) {
        for (j = 0; j < tableSize; ++j) {
#if GTK_CHECK_VERSION(3,0,0)
            InitQuadrant(&psm->aagQuadrant[i][j], psm->pwGrid, psm, quadrantWidth, quadrantHeight, i, j, FALSE);
#else
            InitQuadrant(&psm->aagQuadrant[i][j], psm->pwTable, psm, quadrantWidth, quadrantHeight, i, j, FALSE);
#endif
            if  (i>=oldSize || j>=oldSize) {
                strcpy(psm->aaQuadrantData[i][j].decisionString,"");
                strcpy(psm->aaQuadrantData[i][j].equityText,"");
                strcpy(psm->aaQuadrantData[i][j].unallowedExplanation,"");
            }
        }

        /* score labels */
        pw = gtk_label_new("");
        psm->apwRowLabel[i]=pw; // remember it to update later

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(psm->pwGrid), pw, 1, i + 2, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#else
        gtk_table_attach(GTK_TABLE(psm->pwTable), pw, 1, 2, i + 2, i + 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0); //at the end: xpadding, ypadding; was 3,0
#endif

        pw = gtk_label_new("");
        psm->apwColLabel[i]=pw;

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(psm->pwGrid), pw, i + 2, 1, 1, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(psm->pwTable), pw, i + 2, i + 3, 1, 2);
#endif
    }


    //g_signal_connect(G_OBJECT(psm->pwTable), "draw", G_CALLBACK(DrawQuadrant), psm);

        // gtk_box_pack_start(GTK_BOX(psm->pwv), psm->pwTable, FALSE, FALSE, 0);
        // gtk_widget_show_all(psm->pwv);

    // Player labels: first top, then left
    gchar * sz;
    if (!psm->cubeScoreMap || psm->pms->fMove)
        sz=g_strdup_printf("%s", ap[0].szName);
    else
        sz=g_strdup_printf(_("%s can double"), ap[0].szName);

    pw = gtk_label_new(sz);
    g_free(sz);
//        psm->apwPlayerTop=pw; // remember it to update later

    gtk_label_set_justify(GTK_LABEL(pw), GTK_JUSTIFY_CENTER);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(psm->pwGrid), pw, 2, 0, tableSize, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(psm->pwTable), pw, 2, tableSize + 2, 0, 1);
#endif

    if (!psm->cubeScoreMap || !psm->pms->fMove)
        sz=g_strdup_printf("%s", ap[1].szName);
    else
        sz=g_strdup_printf(_("%s can double"), ap[1].szName);
    pw = gtk_label_new(sz);
    gtk_label_set_angle(GTK_LABEL(pw), 90);
    gtk_label_set_justify(GTK_LABEL(pw), GTK_JUSTIFY_CENTER);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(psm->pwGrid), pw, 0, 2, 1, tableSize);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 3);
    gtk_widget_set_margin_end(pw, 3);
#else
    gtk_widget_set_margin_left(pw, 3);
    gtk_widget_set_margin_right(pw, 3);
#endif
    gtk_box_pack_start(GTK_BOX(psm->pwTableContainer), psm->pwGrid, TRUE, TRUE, 0);
#else
    gtk_table_attach(GTK_TABLE(psm->pwTable), pw, 0, 1, 2, tableSize + 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 3, 0);
    gtk_box_pack_start(GTK_BOX(psm->pwTableContainer), psm->pwTable, TRUE, TRUE, 0);
#endif

    gtk_widget_show_all(psm->pwTableContainer);

    g_free(sz);
}

static void
BuildCubeFrame(scoremap * psm)
/*
   Creates and/or updates the frame with all the cube toggle buttons.
   A pointer to the containing box is kept in psm->pwCubeBox.
*/
{
    int *pi;
    int i, j, m;
/*
    Max cube value. Overestimate, actual value is 2^(floor(ln2(MAX_CUBE_VAL))).
    E.g.,   match size 3 -> MAX_CUBE_VAL=5 (so actual max value 4),
            match size 4 -> MAX_CUBE_VAL=7 (so 4), 
            match size 5 -> MAX_CUBE_VAL=9 (so 8)
*/
    const int MAX_CUBE_VAL=2*MATCH_SIZE(psm)-1;
    char sz[100];
    GtkWidget *pw;
/*
   initialization is not really needed but silences a
   "may be used uninitialized" warning from gcc4 (not later versions)
*/
    GtkWidget *pwx = NULL; 
    GtkWidget *pwLabel;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif

    /* Cube value */
    // pwFrame=gtk_frame_new(_("Cube value"));
    // gtk_box_pack_start(GTK_BOX(psm->pwCubeBox), pwFrame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    psm->pwCubeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    psm->pwCubeBox = gtk_hbox_new(FALSE, 8);
#endif

    // in cube scoremap, we attach everything to the same row
    // while in move scoremap, we attach the "1" to an initial row,
    //      then the first set of cubes >=2 to the top right row
    //      and the 2nd set of cubes >=2 to the bottom right row
    //      To distinguish, we provide positive and negative numbers to the (internal) cube values
    if (psm->cubeScoreMap) {    //========== cube scoremap
        for (i=1; (2*i<= MAX_CUBE_VAL || i <= ABS(psm->signednCube)); i=2*i) {
            /* Iterate i = allowed cube values. (Only powers of two, up to and including the max score in the table)
            * Note 1: in a cube scoremap, we don't ask if we can cube where there is no cube possible, so we want 
            * 2*i and not i to be below the max allowed
            * Note 2: problem: assume we select a cube of 8 in a match of 21, then want to check a match of 3; 
            * the cube becomes irrelevant, but we don't see its radio button anymore; so we also add this case 
            * with a greyed radio button
            */
            if (i == psm->pms->nCube)
                 sprintf(sz,"%d (%s) ", i, _("current"));
             else
                 sprintf(sz,"%d", i);
            if (i==1)
                pw = pwx = gtk_radio_button_new_with_label(NULL, _(sz)); // (centered)"));
            else {
                pw = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwx), _(sz));
            }
            gtk_box_pack_start(GTK_BOX(psm->pwCubeBox), pw, FALSE, FALSE, 0);
            gtk_widget_set_tooltip_text(pw, _("Select the cube value (before the current decision)")); 
            pi = (int *)g_malloc(sizeof(int));
            *pi=i;
            g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), i == ABS(psm->signednCube));
            if (2 * i > MAX_CUBE_VAL)               
                gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(pw), TRUE);
            g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(CubeValToggled), psm);
        }     

    } else {                    //========== move scoremap
        pw = pwx = gtk_radio_button_new_with_label(NULL, "1");
        gtk_box_pack_start(GTK_BOX(psm->pwCubeBox), pw, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text(pw, _("Select the cube value and who doubled (before the current decision)")); 
        pi = (int *)g_malloc(sizeof(int));
        *pi=1;
        g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), (1==psm->signednCube));
        g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(CubeValToggled), psm);

        /* 
        - i represents the column in pwTable, which shows the cube options
        - Intuitively, we want to find i=log2(MAX_CUBE_VAL) (rounded down)
        - The formula is a bit more complex because of table scaling down, see down below 
        for explanations
        */
        for (i = 0, m = 2; (m <= MAX_CUBE_VAL || m <= ABS(psm->signednCube)); i++, m = 2 * m);  

#if GTK_CHECK_VERSION(3,0,0)
        pwGrid = gtk_grid_new();
        gtk_box_pack_start(GTK_BOX(psm->pwCubeBox), pwGrid, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text(pwGrid, _("Select the cube value and who doubled (before the current decision)"));
#else
        pwTable = gtk_table_new(2, 1 + i, FALSE); //GtkWidget* gtk_table_new (guint rows, guint columns, gboolean homogeneous)
        gtk_box_pack_start(GTK_BOX(psm->pwCubeBox), pwTable, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text(pwTable, _("Select the cube value and who doubled (before the current decision)"));
#endif
        /* j is the row we select*/
        for (j = 0; j < 2; j++) {
            sprintf(sz, _("%s doubled to: "), ((psm->pms->fMove) ? (ap[j].szName) : (ap[1-j].szName)));
            pwLabel = gtk_label_new(_(sz));
#if GTK_CHECK_VERSION(3,0,0)
            gtk_widget_set_halign(pwLabel, GTK_ALIGN_END); //GTK_ALIGN_START);
            gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER); //GTK_ALIGN_START);
            gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, j, 1, 1);//void gtk_grid_attach (GtkGrid* grid,GtkWidget* child,gint left,gint top,gint width,gint height)
#else
            gtk_misc_set_alignment(GTK_MISC(pwLabel), 1, 0.5);//void gtk_misc_set_alignment (GtkMisc* misc,gfloat xalign [from 0 (left) to 1 (right)],gfloat yalign [from 0 (top) to 1 (bottom)])
            gtk_table_attach_defaults(GTK_TABLE(pwTable), pwLabel, 0, 1, j, j + 1); //void gtk_table_attach_defaults (GtkTable* table,GtkWidget* widget,guint left_attach,guint right_attach,guint top_attach,guint bottom_attach)
#endif
        }

            /* Iterate through allowed cube values. (Only powers of two) 
            As above, also allow unallowed cubes if we scaled down the match length, but then make the 
            radio button unavailable. Specifically, if the match length is 3, we'll allow a cube of 4,
            but wouldn't normally allow a cube of 8 (>MAX_CUBE_VAL). However, if the previously 
            simulated match size was 5, then maybe we chose a cube of 8  (ABS(psm->signednCube)), since
            it was allowed. Now we'll show it, but disable it.
            */

        for (i = 1, m = 2; (m <= MAX_CUBE_VAL || m <= ABS(psm->signednCube)); m = 2*m, i++) { 
            for (j = 0; j < 2; j++) {
                if (m == psm->pms->nCube && (m == 1 || (j == 0 && psm->pms->fMove == psm->pms->fCubeOwner) ||
                            (j == 1 && psm->pms->fMove != psm->pms->fCubeOwner))) {
                    sprintf(sz,"%d (%s) ", m, _("current"));
                } else {
                    sprintf(sz,"%d", m);
                }
                pw = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwx),sz);
#if GTK_CHECK_VERSION(3,0,0)
                gtk_grid_attach(GTK_GRID(pwGrid), pw, i, j, 1, 1);
#else
                gtk_table_attach_defaults(GTK_TABLE(pwTable), pw, i, i + 1, j, j + 1);
#endif
                pi = (int *)g_malloc(sizeof(int));
                *pi = (j == 0) ? m : -m; // Signed cube value
                g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), (*pi == psm->signednCube));
                if (m > MAX_CUBE_VAL)
                    gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(pw), TRUE);
                g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(CubeValToggled), psm);
            }
        }
    }
    gtk_container_add(GTK_CONTAINER(psm->pwCubeFrame), psm->pwCubeBox);
    gtk_widget_show_all(psm->pwCubeFrame);
}

static void
ScoreMapPlyToggled(GtkWidget * pw, scoremap * psm) 
/* This is called by gtk when the user clicks on one of the ply radio buttons.
*/
{
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        /* recalculate equities */
        psm->ec.nPlies = *pi;
        CalcEquities(psm,0,FALSE, TRUE);
        UpdateScoreMapVisual(psm);
    }

}

static void
ColourByToggled(GtkWidget * pw, scoremap * psm)
/* This is called by gtk when the user clicks on one of the "colour based on" radio buttons.
*/
{
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        psm->colourBasedOn=(scoreMapColour)(*pi);
        UpdateScoreMapVisual(psm); // also updates gauge and gauge labels
    }
}

/* We decided to not use the LabelBy radio buttons, as they are defined in the Analysis>Settings menu */
// static void
// LabelByToggled(GtkWidget * pw, scoremap * psm)
// /* This is called by gtk when the user clicks on one of the "label by" radio buttons.
// */
// {
//     int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");// "label"
//     if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
//         psm->labelBasedOn=(scoreMapLabel)(*pi);
//         UpdateScoreMapVisual(psm);
//     }
// }

static void
TopleftToggled(GtkWidget* pw, scoremap* psm) 
/* This is called by gtk when the user clicks on one of the "Top-left" radio buttons.
*/
{
    int* pi = (int*)g_object_get_data(G_OBJECT(pw), "user_data");// "topleft option"
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        psm->labelTopleft = (scoreMapJacoby)(*pi);  //in the set: { MONEY_NO_JACOBY, MONEY_JACOBY} 
        if (psm->labelTopleft== MONEY_JACOBY)
            psm->moneyJacoby = TRUE;
        else
            psm->moneyJacoby = FALSE;
        CalcEquities(psm,psm->tableSize,TRUE,FALSE);
        UpdateScoreMapVisual(psm); // Update square colours.
    }
}

/* We decided to not use the layout radio buttons, as they are defined in the Analysis>Settings menu */
// static void
// LayoutToggled(GtkWidget * pw, scoremap * psm) 
// /* This is called by gtk when the user clicks on one of the "layout" radio buttons.
// */
// {
//     int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
//     if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
//         psm->layout=(scoreMapLayout)(*pi);
//         gtk_container_remove(GTK_CONTAINER(psm->pwLastContainer), psm->pwOptionsBox);
//         psm->pwLastContainer = (psm->layout == VERTICAL) ? (psm->pwVContainer) : (psm->pwHContainer);
//         BuildOptions(psm);
//         UpdateScoreMapVisual(psm);
//     }
// }

static void
DisplayEvalToggled(GtkWidget * pw, scoremap * psm)
/* This is called by gtk when the user clicks on one of the "display eval" radio buttons.
*/
{
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        if (psm->cubeScoreMap)
                psm->displayCubeEval=(scoreMapCubeEquityDisplay)(*pi);
        else
                psm->displayMoveEval=(scoreMapMoveEquityDisplay)(*pi);
        UpdateScoreMapVisual(psm); // also updates gauge and gauge labels
    }
}

static void
CubeValToggled(GtkWidget * pw, scoremap * psm)
/* This is called by gtk when the user clicks on one of the "cube value" radio buttons.
*/
{
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    //g_print("\n CubeValToggled: cube:%d", *pi);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        psm->signednCube = (*pi);
        /* Change all the cubeinfos to use the selected signednCube value & recalculate all equities*/
        if(CalcEquities(psm,0,FALSE,FALSE)) //if it's OK, it returns 0
            return;
        UpdateScoreMapVisual(psm); // Update square colours. (Also updates gauge and gauge labels)
    }
}

static void
MatchLengthToggled(GtkWidget * pw, scoremap * psm)
/* This is called by gtk when the user picks a new match length.
*/
{
   /*   int index = gtk_combo_box_get_active(GTK_COMBO_BOX(pw));
    int newmatchLength=MATCH_LENGTH_OPTIONS[index];
    if (MATCH_SIZE(psm)!=newmatchLength) {*/
    int* pi = (int*)g_object_get_data(G_OBJECT(pw), "user_data");

    //int newmatchLength = MATCH_LENGTH_OPTIONS[*pi]; 
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {
        psm->matchLengthIndex = (*pi);
        psm->matchLength = MATCH_LENGTH_OPTIONS[psm->matchLengthIndex]; 
       //g_print("\n matchLengthToggled: size:%d", psm->matchLength);
       /* recalculate equities */
            //if (psm->cubeScoreMap)
            //    psm->cubematchLength= psm->matchLength;
            //else
            //    psm->movematchLength= psm->matchLength;
        int oldTableSize=psm->tableSize;
        psm->tableSize = (psm->cubeScoreMap)? psm->matchLength-1 : psm->matchLength+1; //psm->cubematchLength-1 : psm->movematchLength+1;
#if GTK_CHECK_VERSION(3,0,0)
        gtk_container_remove(GTK_CONTAINER(psm->pwTableContainer), psm->pwGrid);
#else
        gtk_container_remove(GTK_CONTAINER(psm->pwTableContainer), psm->pwTable);
#endif
        gtk_container_remove(GTK_CONTAINER(psm->pwCubeFrame), psm->pwCubeBox);
        BuildTableContainer(psm,oldTableSize);
        BuildCubeFrame(psm);
        // if (psm->tableSize > oldTableSize) {
        //     psm->tempScaleUp=1; //indicator used in drawQuadrant to avoid putting the text that may mess up pango
        //     psm->oldTableSize=oldTableSize;
        //     UpdateScoreMapVisual(psm,oldTableSize);
        if (psm->tableSize > oldTableSize) {
            /* first initialize the new elements like we did for the new psm */
            for (int i=0;i<psm->tableSize; i++) {
                for (int j=0;j<psm->tableSize; j++) {
                    if(i>=oldTableSize || j>=oldTableSize) {
                        psm->aaQuadrantData[i][j].isAllowedScore=YET_UNDEFINED; 
                        psm->aaQuadrantData[i][j].isTrueScore = NOT_TRUE_SCORE;
                        psm->aaQuadrantData[i][j].isSpecialScore = REGULAR; 
                        // psm->aaQuadrantData[i][j].ml.cMoves = IMPOSSIBLE_CMOVES; // what if they were previously allocated!?!? => free now in CalcEquities
                        // psm->aaQuadrantData[i][j].ml.amMoves = NULL;
                    }
                }
            }
            /* next compute their equities */
            CalcEquities(psm,oldTableSize,FALSE,FALSE);
        } 
        //     psm->tempScaleUp=0;
        //     psm->oldTableSize=0;
        // }
        UpdateScoreMapVisual(psm);
    }
}


//  char* values[3];
//     values[0] = "Hello";
//     values[1] = "Mew meww";
//     values[2] = "Miau miau =3";

//     for(int i=0; i<sizeof(values)/sizeof(*values); i++)
//         printf("%s\n", values[i]);
static void
BuildLabelFrame(scoremap *psm, GtkWidget * pwv, const char * frameTitle, const char * frameToolTip, const char * labelStrings[],
    int labelStringsLen, int toggleOption, void (*functionWhenToggled)(GtkWidget *, scoremap *), 
    int sensitive, int vAlignExpand) { 
/* Sub-function to build a new frame with a new set of labels, with a whole bunch of needed parameters

- pwFrame ----------
| -- pwh2 -------- |
| | pw (options) | |
| ---------------- |
--------------------
*/
    int *pi;
    int i;
    GtkWidget * pwFrame;
    GtkWidget * pwh2;
    GtkWidget * pw;
    GtkWidget * pwx = NULL;
//    GtkWidget * pwDefault = NULL;

    pwFrame=gtk_frame_new(_(frameTitle));
    gtk_box_pack_start(GTK_BOX(pwv), pwFrame, vAlignExpand, FALSE, 0);
    gtk_widget_set_tooltip_text(pwFrame, _(frameToolTip)); 
    gtk_widget_set_sensitive(pwFrame, sensitive);

#if GTK_CHECK_VERSION(3,0,0)
    pwh2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh2 = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh2);

    for(i=0; i<labelStringsLen; i++) {
        if (i==0) {
            pw = pwx = gtk_radio_button_new_with_label(NULL, _(labelStrings[0])); // First radio button
        } else {
            pw =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwx), _(labelStrings[i])); // Associate this to the other radio buttons
        }
        gtk_box_pack_start(GTK_BOX(pwh2), pw, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text(pw, _(frameToolTip));
        pi = (int *)g_malloc(sizeof(int));
        *pi=(int)i; // here use "=(int)labelEnum[i];" and put it in the input of the function if needed, while
                    //  defining sth like " int labelEnum[] = { NUMBERS, ENGLISH, BOTH };" before calling the function
        g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);
        if (toggleOption==i) // again use "if (DefaultLabel==labelEnum[i])" if needed
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), 1); //we set this to toggle it on in case it's the default option
        g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK((*functionWhenToggled)), psm);
    }
//    return pwDefault;
}

static void
DestroyDialog(gpointer p, GObject * UNUSED(obj))
/* Called by gtk when the score map window is closed.
Allows garbage collection.
*/
{
    scoremap *psm = (scoremap *) p;

    if (pwDialog) {//should always be the case
        pwDialog = NULL;
    }

    /* garbage collect */
    // for (int i=0; i<TOP_K; i++) {
    //     // g_free(psm->topKDecisions[i]);
    //     g_free(psm->topKClassifiedDecisions[i]);
    // }

    if(!psm->cubeScoreMap){
        // for (int i=0;i<psm->tableSize; i++) {
        //     for (int j=0;j<psm->tableSize; j++) {    
        for (int i=0;i<MAX_TABLE_SIZE; i++) {
            for (int j=0;j<MAX_TABLE_SIZE; j++) {

                /* g_free for quadrant data:
                - if cMoves is ==IMPOSSIBLE_CMOVES, it means we haven't touched it since initialization
                - (if it's ==0, it means there was an issue in the computation; previously we already g_free'd it, now we do it here)
                We now g_free everything in between
                */

                if(psm->aaQuadrantData[i][j].ml.cMoves < IMPOSSIBLE_CMOVES){ //} && psm->aaQuadrantData[i][j].ml.cMoves > 0){
                    // g_message("FREED: i=%d, j=%d, tableSize=%d, moves=%d......",i,j,psm->tableSize,psm->aaQuadrantData[i][j].ml.cMoves);
                    g_free(psm->aaQuadrantData[i][j].ml.amMoves);
                } 
                // else {
                //     g_message("NOT FREED: i=%d, j=%d, tableSize=%d, moves=%d......",i,j,psm->tableSize,psm->aaQuadrantData[i][j].ml.cMoves);                    
                // }
            }
        }
        if(psm->moneyQuadrantData.ml.cMoves < IMPOSSIBLE_CMOVES) {
                    // g_message("FINAL FREED money cMoves=%d",psm->moneyQuadrantData.ml.cMoves);
            g_free(psm->moneyQuadrantData.ml.amMoves); 
        }
    } 
    g_free(psm);
}

//Module to add text, based on AddTitle from gtkgame.c
static void
AddText(GtkWidget* pwBox, char* Text)
{
    /* TBD: gtk_rc_style_new is deprecated in GTK3.0:
    "warning: 'gtk_rc_style_new' is deprecated: Use 'GtkStyleContext' instead"
    same for gtk_widget_modify_style below
    */
    GtkRcStyle * ps = gtk_rc_style_new();
    GtkWidget * pwText = gtk_label_new(Text);
    GtkWidget * pwHBox;

#if GTK_CHECK_VERSION(3,0,0)
    pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, FALSE, FALSE, 4);

    ps->font_desc = pango_font_description_new();
    //pango_font_description_set_family_static(ps->font_desc, "serif");
    pango_font_description_set_size(ps->font_desc, 8 * PANGO_SCALE);
    gtk_widget_modify_style(pwText, ps);
    g_object_unref(ps);

    gtk_box_pack_start(GTK_BOX(pwHBox), pwText, TRUE, FALSE, 0);
}

////Module to add text, based on AddTitle from gtkgame.c
//static void
//AddVText(GtkWidget* pwBox, char* Text)
//{
//    GtkRcStyle* ps = gtk_rc_style_new();
//    GtkWidget* pwText = gtk_label_new(Text), * pwVBox;
//
//#if GTK_CHECK_VERSION(3,0,0)
//    pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
//#else
//    pwVBox = gtk_vbox_new(FALSE, 0);
//#endif
//    gtk_box_pack_start(GTK_BOX(pwBox), pwVBox, FALSE, FALSE, 4);
//
//    ps->font_desc = pango_font_description_new();
//    //pango_font_description_set_family_static(ps->font_desc, "serif");
//    pango_font_description_set_size(ps->font_desc, 8 * PANGO_SCALE);
//    gtk_widget_modify_style(pwText, ps);
//    g_object_unref(ps);
//
//    gtk_box_pack_start(GTK_BOX(pwVBox), pwText, TRUE, FALSE, 0);
//}

// display info on move scoremap
extern void
GTKShowMoveScoreMapInfo(GtkWidget* UNUSED(pw), GtkWidget* pwParent) 
{
    GtkWidget* pwInfoDialog, * pwBox;
    // const char* pch;

    pwInfoDialog = GTKCreateDialog(_("Move ScoreMap Info"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwInfoDialog, DA_MAIN)), pwBox);

    // gtk_box_pack_start(GTK_BOX(pwBox), SelectableLabel(pwDialog, VERSION_STRING), FALSE, FALSE, 4);

     //#if GTK_CHECK_VERSION(3,0,0)
     //    gtk_box_pack_start(GTK_BOX(pwBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
     //#else
     //    gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);
     //#endif

        // Add explanation text for move ScoreMap
    AddText(pwBox, _(" - Cr = Crawford, J = Jacoby, DMP = Double Match Point, GG = Gammon Go, GS = Gammon Save\
        \n\n- At each score, the corresponding grid square indicates the best move\
        \n\n- Different colors correspond to different best moves (red for the most frequent one, blue for the 2nd most, etc.)\
        \n\n- A darker color intensity reflects a larger equity difference with the 2nd best move\
        \n\n- As usual, if at 2-ply no move equity is close to that of the best move, the 3-ply or 4-ply analysis can stop at 2-ply\
        \n\n- Feel free to hover with the mouse on the different elements for more details\
        \n\n- Default ScoreMap settings can be selected in the Settings > Analysis menu"));

    GTKRunDialog(pwInfoDialog);
}

// display info on cube scoremap
extern void
GTKShowCubeScoreMapInfo(GtkWidget* UNUSED(pw), GtkWidget* pwParent) 
{
    GtkWidget* pwInfoDialog, * pwBox;
    // const char* pch;

    pwInfoDialog = GTKCreateDialog(_("Cube ScoreMap Info"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwInfoDialog, DA_MAIN)), pwBox);

    AddText(pwBox, _("- D = Double, ND = No Double, D/T = Double/Take, D/P = Double/Pass, TG = Too Good\
        \n\n     To Double, J = Jacoby\
        \n\n- At each score, the corresponding grid square indicates the best cube decision\
        \n\n- Different colors correspond to different cube decisions, as indicated by the gauge\
        \n\n- A darker color intensity reflects a larger equity difference with the 2nd best decision\
        \n\n- Feel free to hover with the mouse on the different elements for more details\
        \n\n- Default ScoreMap settings can be selected in the Settings > Analysis menu"));

    GTKRunDialog(pwInfoDialog);
}

void
BuildOptions(scoremap * psm) { 
/* Build the options part of the scoremap window, i.e. the part that goes after the scoremap table (and the gauge if it exists)
*/

    GtkWidget *pwv;
    int vAlignExpand=FALSE; // set to true to expand vertically the group of frames rather than packing them to the top

    // Holder for both columns in vertical layout, for single column in horizontal layout
    if (psm->layout == VERTICAL) {
#if GTK_CHECK_VERSION(3,0,0)
        psm->pwOptionsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
        psm->pwOptionsBox = gtk_hbox_new(FALSE, 6);
#endif

        gtk_box_pack_start(GTK_BOX(psm->pwLastContainer), psm->pwOptionsBox, FALSE, FALSE, 0); // i.e. in pwVContainer for vertical layout option

   // First column (using vertical box)
#if GTK_CHECK_VERSION(3,0,0)
        pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
        pwv = gtk_vbox_new(FALSE, 6); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif
        gtk_box_pack_start(GTK_BOX(psm->pwOptionsBox), pwv, TRUE, FALSE, 0);

    } else { //horizontal layout, single column

#if GTK_CHECK_VERSION(3,0,0)
        psm->pwOptionsBox =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
        psm->pwOptionsBox =  gtk_vbox_new(FALSE, 6); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif
        gtk_box_pack_start(GTK_BOX(psm->pwLastContainer), psm->pwOptionsBox, FALSE, FALSE, 0); // i.e. in pwHContainer for horizontal layout option

//         //inserting a holder cell to align everything
#if GTK_CHECK_VERSION(3,0,0)
        pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
        pwv = gtk_vbox_new(FALSE, 6); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif
        gtk_box_pack_start(GTK_BOX(psm->pwOptionsBox), pwv, TRUE, FALSE, 40);
    }

     /* eval ply */

    //frameToolTip = "Select the ply at which to evaluate the equity at each score";
    BuildLabelFrame(psm, pwv, _("Evaluation"), _("Select the ply at which to evaluate the equity at each score"), aszScoreMapPly, NUM_PLY, psm->ec.nPlies, ScoreMapPlyToggled, TRUE, vAlignExpand);

    /* Match length */ 
    // const char* lengthStrings[NUM_MATCH_LENGTH-1] = { N_("3"), N_("5"), N_("7"), N_("9"), N_("11"), N_("15"), N_("21") };
    BuildLabelFrame(psm, pwv, _("Match length"), _("Select the match length (which determines the grid size)"), aszScoreMapMatchLength, NUM_MATCH_LENGTH-1, psm->matchLengthIndex, MatchLengthToggled, TRUE, vAlignExpand);

    /*cube*/

    psm->pwCubeFrame=gtk_frame_new(_("Cube value"));
    gtk_box_pack_start(GTK_BOX(pwv), psm->pwCubeFrame, vAlignExpand, FALSE, 0); 

    BuildCubeFrame(psm);

  
    // ***************************************** //

   // If vertical layout: second column (using vertical box)
    if (psm->layout == VERTICAL) {

#if GTK_CHECK_VERSION(3,0,0)
    pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
    pwv = gtk_vbox_new(FALSE, 6); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif
    gtk_box_pack_start(GTK_BOX(psm->pwOptionsBox), pwv, TRUE, FALSE, 0);
    }

    // if (!(psm->cubeScoreMap)) { // && layout == VERTICAL)) {
    //     /* topleft toggle frame (  unless in cube scoremap, where it's inserted earlier to equalize )*/
    //     // const char* aszScoreMapJacoby[2] = { N_("Scoreless (Money w/o J)"), N_("Money w/ J") }; //was: { N_("64-away"), N_("Money (J)"), N_("Money (No J)") };
    //     //frameToolTip = "Select whether the top-left square should provide a scoreless evaluation (money play without Jacoby) or an evaluation of money play with Jacoby";
    //     BuildLabelFrame(psm, pwv, _("Top-left square"), _("Select whether the top-left square should provide a scoreless evaluation (money play without Jacoby) or an evaluation of money play with Jacoby"), aszScoreMapJacoby, NUM_JACOBY, psm->labelTopleft, TopleftToggled, TRUE, vAlignExpand);
    // }

    /* topleft toggle frame */
        BuildLabelFrame(psm, pwv, _("Top-left square"), _("Select whether the top-left square should provide an \"unlimited\" evaluation (money play without Jacoby) or an evaluation of money play with Jacoby"), aszScoreMapJacobyShort, NUM_JACOBY, psm->labelTopleft, TopleftToggled, TRUE, vAlignExpand);

    /* Display Eval frame */
    if (psm->cubeScoreMap) {
        //frameToolTip = "Select whether to display the equity, and how to display it: absolute equity, relative equity difference between Double and No-Double, or relative equity difference between Double/Pass and Double/Take";
        BuildLabelFrame(psm, pwv, _("Display equity (hover over grid for details)"), _("Select whether to display the equity, and how to display it: absolute equity, relative equity difference between Double and No-Double, or relative equity difference between Double/Pass and Double/Take"), aszScoreMapCubeEquityDisplayShort, NUM_CUBEDISP, psm->displayCubeEval, DisplayEvalToggled, TRUE, vAlignExpand);
    } else {
        //frameToolTip =  "Select whether to display the equity, and how to display it: absolute equity, or relative equity difference between best move and second best move";
        BuildLabelFrame(psm, pwv, _("Display equity (hover over grid for details)"), _("Select whether to display the equity, and how to display it: absolute equity, or relative equity difference between best move and second best move"), aszScoreMapMoveEquityDisplayShort, NUM_MOVEDISP, psm->displayMoveEval, DisplayEvalToggled, TRUE, vAlignExpand);
    }

    /* colour by frame */
   if (psm->cubeScoreMap) {
        // We now insert the radio buttons of the "colour by" scheme, in (a) cube scoremap, or 
        // (b) move scoremap we disable the buttons since they are only relevant to the cube scoremap

        /* Colour by */

        //frameToolTip = "Select whether to colour the table squares using all options, or emphasize the Double vs. No-Double decision, or emphasize the Double/Take vs. Double/Pass decision";
        BuildLabelFrame(psm, pwv, _("Colour by"), _("Select whether to colour the table squares using all options, or emphasize the Double vs. No-Double decision, or emphasize the Double/Take vs. Double/Pass decision"), aszScoreMapColourShort, NUM_COLOUR, psm->colourBasedOn, ColourByToggled, psm->cubeScoreMap, vAlignExpand);
   }  

    // /* Label by toggle */
    // // This button offers the choice between the display of true scores and away scores.

    // //frameToolTip = "Select whether to orient the table axes by the away score, i.e. the difference between the current score and the match length, or the true score. For example, a player with a true score of 2 out of 7 has an away score of 5";
    // BuildLabelFrame(psm, pwv, _("Label by"), _("Select whether to orient the table axes by the away score, i.e. the difference between the current score and the match length, or the true score. For example, a player with a true score of 2 out of 7 has an away score of 5"), aszScoreMapLabel, 2, psm->labelBasedOn, LabelByToggled, TRUE, vAlignExpand);
     
    // /* Layout frame */

    // //frameToolTip = "Select whether the options pane should be at the bottom or on the right side of the table";
    // BuildLabelFrame(psm, pwv, _("Location of option pane"), _("Select whether the options pane should be at the bottom or on the right side of the table"), aszScoreMapLayout, NUM_LAYOUT, psm->layout, LayoutToggled, TRUE, vAlignExpand);


    gtk_widget_show_all(psm->pwLastContainer);

} //end BuildOptions


extern void
GTKShowScoreMap(const matchstate * pms, int cube)
/*
 * Opens the score map window.
 * Create the GUI; calculate equities and update colours; run the dialog window.
 * cube == 1 means we want a scoremap for the cube eval, == 0 for the move eval.
 */
{
    scoremap *psm;
    int screenWidth, screenHeight;

#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGaugeGrid;
#else
    GtkWidget *pwGaugeTable; /* = NULL; ? */
#endif

    GtkWidget *pw;
    GtkWidget *pwButton;


/* Layout 1: options on right side (LAYOUT_HORIZONTAL==TRUE, i.e. layout==HORIZONTAL)
-- pwh -----------------------------------------------------
| --- pwv ----------------- ---pwv (reused)--------------- |
| | psm->pwTableContainer | | -pwFrame (multiple times)- | |
| |                       | | | --pwv2 (mult. times)-- | | |
| |                       | | | | pw (options)       | | | |
| | pwGaugeTable/Grid     | | | ---------------------- | | |
| |                       | | -------------------------- | |
| ------------------------- ------------------------------ |
------------------------------------------------------------


Layout 2: options on bottom (LAYOUT_HORIZONTAL==FALSE, i.e. layout==VERTICAL)
--- pwv ---------------
| psm->pwv             |
| (contains the table) |
| pwGaugeTable/Grid    |
| - pwh (mult. times)- |
| | pw (options)     | |
| -------------------- |
------------------------

Implemented as: 

-- psm->pwHContainer----------------------------------------------------|
| --- psm->pwVContainer---------  -- (options w/ vertical) -------|     |
| |                             | |                               |     |
| | psm->pwTableContainer       | |                               |     |
| |                             | |                               |     |
| | pwGaugeTable/Grid           | |                               |     |
| |                             | |                               |     |
| | (options w/ horizontal)     | |                               |     |
| ------------------------------  --------------------------------|     |
------------------------------------------------------------------------|

(using "psm->pwLastContainer = (layout == VERTICAL) ? (psm->pwVContainer) : (psm->pwHContainer);")

Finally, we pack everything inside a "pwMegaVContainer", which enables us to introduce a shared element at the bottom
if needed (this was initially planned for some explanation text, which was then removed).


*/

    /* dialog */

    // First, following feedback: making sure there is only one score map window open
    if (pwDialog) { //i.e. we didn't close it using DestroyDialog()
        gtk_widget_destroy(gtk_widget_get_toplevel(pwDialog));
        pwDialog = NULL;
    }

    // Second, making it non-modal, i.e. we can access the window below and move the Score Map window
    if (cube)
        pwDialog = GTKCreateDialog(_("Cube ScoreMap: Best Cube Decision at Different Scores"),
                            DT_INFO, NULL, DIALOG_FLAG_NONE, NULL, NULL);
    else
        pwDialog = GTKCreateDialog(_("Move ScoreMap: Best Move Decision at Different Scores"),
            DT_INFO, NULL, DIALOG_FLAG_NONE, NULL, NULL);
    // pwDialog = GTKCreateDialog(_("Score Map - Eval at Different Scores"),
    //                            DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);
    // pwDialog = GTKCreateDialog(_("Score Map - Eval at Different Scores"),
    //                            DT_INFO, NULL, DIALOG_FLAG_MINMAXBUTTONS, NULL, NULL); //| DIALOG_FLAG_NOTIDY



#if GTK_CHECK_VERSION(3,22,0)
    GdkRectangle workarea = {0};
    gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()), &workarea);
    screenWidth=MAX(workarea.width,100); // in case we get 0...
    screenHeight=MAX(workarea.height,100);
    // printf ("W: %u x H:%u\n", workarea.width, workarea.height);
    //alternative scheme, untried: goes along the following lines:
    // display = gdk_display_get_default();  monitor = gdk_display_get_monitor(display, 0);  gdk_monitor_get_geometry(monitor, &r);
#else
    GdkScreen *screen = gdk_screen_get_default();
    guint monitor;
    GdkRectangle screen_geometry = { 0, 0, 0, 0 };

#if GTK_CHECK_VERSION(2,20,0)
    monitor = gdk_screen_get_primary_monitor(screen);
#else
    monitor = 0;
#endif

    gdk_screen_get_monitor_geometry(screen, monitor, &screen_geometry);
    screenWidth=MAX(screen_geometry.width,100); // in case we get 0...
    screenHeight=MAX(screen_geometry.height,100);
#endif

    MAX_TABLE_WIDTH = MIN(MAX_TABLE_WIDTH, (int)(0.3f * (float)screenWidth));
    MAX_TABLE_HEIGHT = MIN(MAX_TABLE_HEIGHT, (int)(0.6f * (float)screenHeight));



    /* ******************* we define the psm default values ******************** */

    psm = (scoremap *) g_malloc(sizeof(scoremap));
    psm->cubeScoreMap = cube;   // throughout this file: determines whether we want a cube scoremap or a move scoremap
    //colourBasedOn=ALL;     //default gauge; see also the option to set the starting gauge at the bottom
    // psm->describeUsing=DEFAULT_DESCRIPTION; //default description mode: NUMBERS, ENGLISH, BOTH -> moved to static variable
    psm->pms = pms;
    // matchstate ams = (*pms); // Make a copy of the "master" matchstate  
                //[note: backgammon.h defines an extern ms => using a different name]
    psm->msTemp = *pms; 
 
    psm->ec.fCubeful = TRUE;
    psm->ec.nPlies = scoreMapPlyDefault; // evalPlies;
    psm->ec.fUsePrune = TRUE; // FALSE;
    psm->ec.fDeterministic = TRUE;
    psm->ec.rNoise = 0.0f;
    psm->truenMatchTo = pms->nMatchTo;
    // psm->tempScaleUp=0;
    // psm->oldTableSize=0;

    psm->labelBasedOn = scoreMapLabelDef; 
    psm->colourBasedOn = scoreMapColourDef;   
    psm->displayCubeEval = scoreMapCubeEquityDisplayDef;
    psm->displayMoveEval = scoreMapMoveEquityDisplayDef;
    psm->layout = scoreMapLayoutDef;
    if (pms->nCube==1) //need to define here to set the default cube radio button
        psm->signednCube=1;
    else {
        psm->signednCube =  (pms->fCubeOwner==pms->fMove) ? (pms->nCube) : (-pms->nCube);
    }
    psm->labelTopleft = scoreMapJacobyDef;
    psm->moneyJacoby = (scoreMapJacobyDef ==  MONEY_NO_JACOBY)? FALSE : TRUE;
    
    // ******* DEFINITION OF MATCH LENGTH ***
        /* 
        Goal: choose the match length to simulate, which impacts the table size
        (A) The user picks a default match length index (fixed or variable) through scoreMapMatchLengthDefIdx
        in the Settings > Options
        (B) When we launch the ScoreMap, we look at this default, and set: "psm->matchLengthIndex = (int) scoreMapMatchLengthDefIdx;"
        (C) If the choice is "variable length", we choose the match length (=maximum match size to analyze)
        based on the length of the current real match:
        1) small match of size <= 3 -> use 3
        2) big match of size >=7 -> use 7
        3) medium-sized match or money play -> use 5
        (D) Now the user can also change the match length for this ScoreMap only, which will not affect the default
        match length. Since we only offer fixed lengths, if the user chose a variable default, we need to display
        the correct fixed length.
      */
    psm->matchLengthIndex = (int) scoreMapMatchLengthDefIdx;
    //g_print("\n match length index:%d",psm->matchLengthIndex);

    if (psm->matchLengthIndex <NUM_MATCH_LENGTH-1) //user wants a fixed default match length
        psm->matchLength = MATCH_LENGTH_OPTIONS[psm->matchLengthIndex];
    else { //user wants a variable default match size
        if (pms->nMatchTo <= 3 && pms->nMatchTo > 0) //i.e. we are in a money match or a short match
            //psm->matchLength = 3; //default value for small match
            psm->matchLengthIndex = 0;
        else if (pms->nMatchTo >= 7) //i.e. we are in a long match and don't want a huge table
            //psm->matchLength = 7; //default value if big match
            psm->matchLengthIndex = 2;
        else //middle-sized match or money play
            //psm->matchLength = 5; //default value for regular match
            psm->matchLengthIndex = 1;
        psm->matchLength = MATCH_LENGTH_OPTIONS[psm->matchLengthIndex];
    }
    //g_print("\n match length index:%d, match length:%d",psm->matchLengthIndex,psm->matchLength);

    // // Find closest appropriate table size among the options
   // // In the next expression, the ";" at the end means that it gets directly to finding the right index, and doesn't execute
   // //      anything in the loop
   // // Since we restrict to the pre-defined options, there is no reason to define MIN/MAX TABLE SIZE
    //if (psm->cubeScoreMap)// && cubematchLength==0)
    //    cubematchLength=pms->nMatchTo;
    //else if (!psm->cubeScoreMap)// && psm->movematchLength==0)
    //    psm->movematchLength=pms->nMatchTo;

    // if psm->cubeScoreMap, we show away score from 2-away to (psm->matchLength)-away
    // if not, we show away score at 1-away twice (w/ and post crawford), then 2-away through (psm->matchLength)-away
    psm->tableSize = (psm->cubeScoreMap) ? psm->matchLength - 1 : psm->matchLength + 1;//psm->cubematchLength-1 : psm->movematchLength+1;
    
    /* 1) We initialize isAllowedScore to make sure there is no redrawing without a defined value.
    Since we're already here, we also initialize the other properties. 
    2) We also initialize ml.cMoves to -1
    */
    for (int i=0;i<psm->tableSize; i++) {
        for (int j=0;j<psm->tableSize; j++) {
            psm->aaQuadrantData[i][j].isAllowedScore=YET_UNDEFINED; 
            psm->aaQuadrantData[i][j].isTrueScore = NOT_TRUE_SCORE;
            psm->aaQuadrantData[i][j].isSpecialScore = REGULAR; 
            // psm->aaQuadrantData[i][j].ml.cMoves = IMPOSSIBLE_CMOVES;
        }
    }
    for (int i=0;i<MAX_TABLE_SIZE; i++) {
        for (int j=0;j<MAX_TABLE_SIZE; j++) {
            psm->aaQuadrantData[i][j].ml.cMoves = IMPOSSIBLE_CMOVES;
        }
    }
    psm->moneyQuadrantData.isAllowedScore=YET_UNDEFINED; 
    psm->moneyQuadrantData.isTrueScore = NOT_TRUE_SCORE;
    psm->moneyQuadrantData.isSpecialScore = REGULAR; 
    psm->moneyQuadrantData.ml.cMoves = IMPOSSIBLE_CMOVES;



#if GTK_CHECK_VERSION(3,0,0)
    psm->pwTableContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    psm->pwTableContainer = gtk_vbox_new(FALSE, 0); // parameters: homogeneous, spacing
#endif
    // for (int i=0; i<TOP_K; i++) {
    //     // psm->topKDecisions[i]=g_malloc(FORMATEDMOVESIZE * sizeof(char));
    //     psm->topKClassifiedDecisions[i]=g_malloc((FORMATEDMOVESIZE+5)*sizeof(char));
    // }

    BuildTableContainer(psm,0); //0==oldSize

// *************************************************************************************************
#if GTK_CHECK_VERSION(3,0,0)
    psm->pwMegaVContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
    psm->pwMegaVContainer = gtk_vbox_new(FALSE, 2); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), psm->pwMegaVContainer);

#if GTK_CHECK_VERSION(3,0,0)
    psm->pwHContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    psm->pwHContainer = gtk_hbox_new(FALSE, 2); //gtk_hbox_new (gboolean homogeneous, gint spacing);
#endif
    gtk_box_pack_start(GTK_BOX(psm->pwMegaVContainer), psm->pwHContainer, TRUE, TRUE, 0);
    
     /* left vbox: holds table and gauge */
#if GTK_CHECK_VERSION(3,0,0)
    psm->pwVContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
    psm->pwVContainer = gtk_vbox_new(FALSE, 2); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif

    gtk_box_pack_start(GTK_BOX(psm->pwHContainer), psm->pwVContainer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(psm->pwVContainer), psm->pwTableContainer, TRUE, TRUE, 0);   //gtk_box_pack_start (GtkBox *box, GtkWidget *child,
                                        // gboolean expand,  gboolean fill, guint padding);

    /* horizontal separator */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(psm->pwVContainer), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
#else
    gtk_box_pack_start(GTK_BOX(psm->pwVContainer), gtk_hseparator_new(), FALSE, FALSE, 2);
#endif

    /* If cube decision, show gauge */
    if (psm->cubeScoreMap) {

#if GTK_CHECK_VERSION(3,0,0)
        pwGaugeGrid = gtk_grid_new();
#else
        pwGaugeTable = gtk_table_new(2, GAUGE_SIZE, FALSE);
#endif

        for (int i = 0; i < GAUGE_SIZE; i++) {
            pw = gtk_drawing_area_new();
            gtk_widget_set_size_request(pw, 8, 20); // [*widget, width, height]
#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pwGaugeGrid), pw, i, 1, 1, 1);
            gtk_widget_set_hexpand(pw, TRUE);
#else
            gtk_table_attach_defaults(GTK_TABLE(pwGaugeTable), pw, i, i + 1, 1, 2);
#endif
            g_object_set_data(G_OBJECT(pw), "user_data", NULL);

#if GTK_CHECK_VERSION(3,0,0)
            gtk_style_context_add_class(gtk_widget_get_style_context(pw), "gnubg-score-map-quadrant");
            g_signal_connect(G_OBJECT(pw), "draw", G_CALLBACK(DrawQuadrant), NULL);
#else
            g_signal_connect(G_OBJECT(pw), "expose_event", G_CALLBACK(ExposeQuadrant), NULL);
#endif
            DrawGauge(pw,i,ALL);
            psm->apwFullGauge[i]=pw;
        }
        for (int i = 0; i < 4; ++i) {
            psm->apwGauge[i] = gtk_label_new("");
#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pwGaugeGrid), psm->apwGauge[i], GAUGE_SIXTH_SIZE * 2 * i, 0, 1, 1);
#else
            gtk_table_attach_defaults(GTK_TABLE(pwGaugeTable), psm->apwGauge[i], GAUGE_SIXTH_SIZE * 2 * i, GAUGE_SIXTH_SIZE * 2 * i + 1, 0, 1);
#endif
        }

#if GTK_CHECK_VERSION(3,0,0)
        gtk_box_pack_start(GTK_BOX(psm->pwVContainer), pwGaugeGrid, FALSE, FALSE, 2);
#else
        gtk_box_pack_start(GTK_BOX(psm->pwVContainer), pwGaugeTable, FALSE, FALSE, 2);
#endif

        /* horizontal separator */

#if GTK_CHECK_VERSION(3,0,0)
        gtk_box_pack_start(GTK_BOX(psm->pwVContainer), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
#else
        gtk_box_pack_start(GTK_BOX(psm->pwVContainer), gtk_vseparator_new(), FALSE, FALSE, 2);
#endif
    }

     /* vertical separator */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(psm->pwHContainer), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 2);
#else
    gtk_box_pack_start(GTK_BOX(psm->pwHContainer), gtk_vseparator_new(), FALSE, FALSE, 2);
#endif

    /* options box, with either vertical or horizontal layout */
    psm->pwLastContainer = (psm->layout == VERTICAL) ? (psm->pwVContainer) : (psm->pwHContainer);
    BuildOptions(psm); 

    gtk_box_pack_start(GTK_BOX(psm->pwMegaVContainer), pwButton = gtk_button_new_with_label(_("Explanations")), FALSE, FALSE, 8);
    gtk_widget_set_tooltip_text(pwButton, _("Click to obtain more explanations on this ScoreMap window")); 
    //g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GTKShowMoveScoreMapInfo), pwDialog); 
    if (psm->cubeScoreMap)
        g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GTKShowCubeScoreMapInfo), pwDialog);
    else
        g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GTKShowMoveScoreMapInfo), pwDialog);
 

// **************************************************************************************************
    /* calculate values and set colours/text in the table */

    /* For each i,j, fill sm->aaQuadrantData[i][j].ci, find equities, and set the text*/
    CalcEquities(psm,0,FALSE,FALSE);
    UpdateScoreMapVisual(psm);     //Update: (1) The color of each square (2) The hover text of each square
                                    //      (3) the row/col score labels (4) the gauge.

    /* modality */

    gtk_window_set_default_size(GTK_WINDOW(pwDialog), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    /* The DestroyDialog function frees the needed memory! */
    g_object_weak_ref(G_OBJECT(pwDialog), DestroyDialog, psm);

    GTKRunDialog(pwDialog);
}
