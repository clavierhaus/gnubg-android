/*
 * Copyright (C) 2020 Aaron Tikuisis <Aaron.Tikuisis@uottawa.ca>
 * Copyright (C) 2020-2023 Isaac Keslassy <keslassy@gmail.com>
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
 * $Id: gtkscoremap.h,v 1.4 2023/01/18 21:49:36 plm Exp $
 */

#ifndef GTKSCOREMAP_H
#define GTKSCOREMAP_H

#include "gnubg-types.h"        /* for matchstate */

typedef enum {
    ZERO_PLY, ONE_PLY, TWO_PLY, THREE_PLY, FOUR_PLY,
    NUM_PLY
} scoreMapPly;

extern scoreMapPly scoreMapPlyDefault;
extern const char* aszScoreMapPly[NUM_PLY];
extern const char* aszScoreMapPlyCommands[NUM_PLY];

typedef enum {
    LENGTH_THREE, LENGTH_FIVE, LENGTH_SEVEN, LENGTH_NINE, LENGTH_ELEVEN, LENGTH_FIFTEEN, LENGTH_TWENTY_ONE,
    VAR_LENGTH,
    NUM_MATCH_LENGTH
} scoreMapMatchLength;

extern scoreMapMatchLength scoreMapMatchLengthDefIdx;
extern const int MATCH_LENGTH_OPTIONS[NUM_MATCH_LENGTH];
extern const char* aszScoreMapMatchLength[NUM_MATCH_LENGTH];
extern const char* aszScoreMapMatchLengthCommands[NUM_MATCH_LENGTH];

/* Used in the "Label by" radio buttons - we assume the same order as the labels */
typedef enum { LABEL_AWAY, LABEL_SCORE, NUM_LABEL} scoreMapLabel;
extern scoreMapLabel scoreMapLabelDef;
extern const char* aszScoreMapLabel[NUM_LABEL];
extern const char* aszScoreMapLabelCommands[NUM_LABEL]; 

/* Used in the "Top-left" radio buttons - we assume the same order as the labels */
/* Additional options elsewhere:
- apwScoreMapJacoby (gtkgame.c); 
- "set scoremapjacoby" (gtkgame.c); 
- CommandSetScoreMapJacoby (backgammon.h)*/
typedef enum {MONEY_NO_JACOBY, MONEY_JACOBY, NUM_JACOBY} scoreMapJacoby;
extern scoreMapJacoby scoreMapJacobyDef;
extern const char* aszScoreMapJacoby[NUM_JACOBY];
extern const char* aszScoreMapJacobyCommands[NUM_JACOBY]; 

/* Used in the "Display Eval" radio buttons - we assume the same order as the labels */
typedef enum {CUBE_NO_EVAL, CUBE_ABSOLUTE_EVAL, CUBE_RELATIVE_EVAL_ND_D, CUBE_RELATIVE_EVAL_DT_DP, NUM_CUBEDISP} scoreMapCubeEquityDisplay;
extern scoreMapCubeEquityDisplay scoreMapCubeEquityDisplayDef;
extern const char* aszScoreMapCubeEquityDisplay[NUM_CUBEDISP];
extern const char* aszScoreMapCubeEquityDisplayCommands[NUM_CUBEDISP]; 
// Same for move evaluation
typedef enum {MOVE_NO_EVAL, MOVE_ABSOLUTE_EVAL, MOVE_RELATIVE_EVAL,  NUM_MOVEDISP} scoreMapMoveEquityDisplay;
extern scoreMapMoveEquityDisplay scoreMapMoveEquityDisplayDef;
extern const char* aszScoreMapMoveEquityDisplay[NUM_MOVEDISP];
extern const char* aszScoreMapMoveEquityDisplayCommands[NUM_MOVEDISP]; 

/* Used in the "Colour by" radio buttons - we assume the same order as the labels */
/* Additional options elsewhere:
- apwScoreMapColour (gtkgame.c); 
- CommandSetScoreMapColour (backgammon.h)
- "set scoremapcolour" (gtkgame.c); 
*/
typedef enum {ALL, DND, PT, NUM_COLOUR} scoreMapColour;
extern scoreMapColour scoreMapColourDef;
extern const char* aszScoreMapColour[NUM_COLOUR];
extern const char* aszScoreMapColourCommands[NUM_COLOUR]; 

/* Layout options: either horizontal or vertical. */
/* Additional options elsewhere:
- apwScoreMapLayout (gtkgame.c); 
- CommandSetScoreMapLayout (backgammon.h)
- "set scoremaplayout" (gtkgame.c); 
*/
typedef enum {VERTICAL, HORIZONTAL, NUM_LAYOUT} scoreMapLayout;
extern scoreMapLayout scoreMapLayoutDef;
extern const char* aszScoreMapLayout[NUM_LAYOUT];
extern const char* aszScoreMapLayoutCommands[NUM_LAYOUT]; 

extern void GTKShowScoreMap(const matchstate ams[], int cube);
extern void GTKShowMoveScoreMapInfo(GtkWidget* pw, GtkWidget* pwParent);
extern void GTKShowCubeScoreMapInfo(GtkWidget* pw, GtkWidget* pwParent);

#endif                          /* GTKSCOREMAP_H */
