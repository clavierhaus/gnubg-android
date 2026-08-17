/*
 * Copyright (C) 2000-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2002-2016 the AUTHORS
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
 * $Id: gtkboard.h,v 1.110 2020/01/19 19:26:05 Superfly_Jon Exp $
 */

#ifndef GTKBOARD_H
#define GTKBOARD_H

#include "backgammon.h"

#include <gtk/gtk.h>

#include "eval.h"
#include "gtkpanels.h"
#include "common.h"
#include "render.h"
#include "gtklocdefs.h"

#define TYPE_BOARD			(board_get_type ())
#define BOARD(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_BOARD, Board))
#define BOARD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_BOARD, BoardClass))
#define IS_BOARD(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_BOARD))
#define IS_BOARD_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_BOARD))
#define BOARD_GET_CLASS(obj)  		(G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_BOARD, BoardClass))

typedef struct {
    GtkVBoxClass parent_class;
} BoardClass;

typedef enum {
    DICE_NOT_SHOWN = 0, DICE_BELOW_BOARD, DICE_ON_BOARD, DICE_ROLLING
} DiceShown;

/* private data */
typedef struct {
    GtkWidget *drawing_area, *dice_area, *table, *wmove,
        *reset, *edit, *name0, *name1, *score0, *score1,
        *crawford, *jacoby, *widget, *key0, *key1, *stop, *stopparent,
        *doub, *lname0, *lname1, *lscore0, *lscore1, *mname0, *mname1, *mscore0, *mscore1, *play;
    GtkWidget *mmatch, *lmatch, *match;
    GtkAdjustment *amatch, *ascore0, *ascore1;
    GtkWidget *roll;
    GtkWidget *take, *drop, *redouble;
    GtkWidget *pipcount0, *pipcount1;
    GtkWidget *pipcountlabel0, *pipcountlabel1;
    GtkWidget *pwvboxcnt;

    gtk_locdef_surface *appmKey[2];

    gboolean playing, computer_turn;
    gint drag_point, drag_colour, x_drag, y_drag, x_dice[2], y_dice[2], drag_button, click_time, cube_use;      /* roll showing on the off-board dice */
    DiceShown diceShown;
    TanBoard old_board;

    gint cube_owner;            /* -1 = bottom, 0 = centred, 1 = top */
    gint qedit_point;           /* used to remember last point in quick edit mode */
    gint resigned;
    gint nchequers;
    move *all_moves, *valid_move;
    movelist move_list;

    renderimages ri;

    /* remainder is from FIBS board: data */
    char name[MAX_NAME_LEN], name_opponent[MAX_NAME_LEN];
    gint match_to, score, score_opponent;
    gint points[28];            /* 0 and 25 are the bars */
    gint turn;                  /* -1 is X, 1 is O, 0 if game over */
    unsigned int diceRoll[2];   /* 0, 0 if not rolled */
    guint cube;
    gint can_double, opponent_can_double;       /* allowed to double */
    gint doubled;               /* -1 if X is doubling, 1 if O is doubling */
    gint colour;                /* -1 for player X, 1 for player O */
    gint direction;             /* -1 playing from 24 to 1, 1 playing from 1 to 24 */
    gint home, bar;             /* 0 or 25 depending on fDirection */
    gint off, off_opponent;     /* number of men borne off */
    gint on_bar, on_bar_opponent;       /* number of men on bar */
    gint to_move;               /* 0 to 4 -- number of pieces to move */
    gint forced, crawford_game; /* unused, Crawford game flag */
    gint jacoby_flag;           /* jacoby enabled flag */
    gint redoubles;             /* number of instant redoubles allowed */
    int DragTargetHelp;         /* Currently showing draw targets? */
    int iTargetHelpPoints[4];   /* Drag target position */
    int grayBoard;              /* Show board grayed when editing */

#if defined(USE_BOARD3D)
    BoardData3d *bd3d;          /* extra members for 3d board */
#endif
    renderdata *rd;             /* The board colour settings */
} BoardData;

typedef struct {
    GtkVBox vboxxx;
    /* private data */
    BoardData *board_data;
} Board;

typedef enum {
    ANIMATE_NONE, ANIMATE_BLINK, ANIMATE_SLIDE
} animation;

typedef enum {
    GUI_SHOW_PIPS_NONE, GUI_SHOW_PIPS_PIPS, GUI_SHOW_PIPS_EPC, GUI_SHOW_PIPS_WASTAGE, N_GUI_SHOW_PIPS
} GuiShowPips;

extern animation animGUI;
extern int fGUIBeep;
extern int fGUIHighDieFirst;
extern int fGUIIllegal;
extern int fShowIDs;
extern GuiShowPips gui_show_pips;
extern int fGUISetWindowPos;
extern int fGUIDragTargetHelp;
extern int fGUIUseStatsPanel;
extern int fGUIGrayEdit;
extern unsigned int nGUIAnimSpeed;

extern GType board_get_type(void);
extern GtkWidget *board_new(renderdata * prd, int inPreview);
extern GtkWidget *board_cube_widget(Board * board);
extern void DestroySetCube(GObject * po, GtkWidget * pw);
extern void Copy3dDiceColour(renderdata * prd);
typedef enum { MT_STANDARD, MT_FIRSTMOVE, MT_EDIT } manualDiceType;
extern GtkWidget *board_dice_widget(Board * board, manualDiceType mdt);
extern gint game_set(Board * board, TanBoard points, int roll,
                     const gchar * name, const gchar * opp_name, gint match,
                     gint score, gint opp_score, gint die0, gint die1, gint computer_turn, gint nchequers);
extern void board_set_playing(Board * board, gboolean f);
extern void board_animate(Board * board, int move[8], int player);
extern unsigned int convert_point(int i, int player);


extern void board_create_pixmaps(GtkWidget * board, BoardData * bd);
extern void board_free_pixmaps(BoardData * bd);
extern void board_edit(BoardData * bd);

extern void InitBoardPreview(BoardData * bd);

extern int animate_player, *animate_move_list, animation_finished;

enum TheoryTypes { TT_PIPCOUNT = 1, TT_EPC = 2, TT_RETURNHITS = 4, TT_KLEINCOUNT = 8 };
void UpdateTheoryData(BoardData * bd, int UpdateTypes, const TanBoard points);

extern void read_board(BoardData * bd, TanBoard points);
extern void update_pipcount(BoardData * bd, const TanBoard points);
extern void InitBoardData(BoardData * bd);
extern gboolean board_button_press(GtkWidget * board, GdkEventButton * event, BoardData * bd);
extern gboolean board_motion_notify(GtkWidget * widget, GdkEventMotion * event, BoardData * bd);
extern gboolean board_button_release(GtkWidget * board, GdkEventButton * event, BoardData * bd);
extern void RollDice2d(BoardData * bd);
extern void DestroyPanel(gnubgwindow window);
extern void
DrawDie(cairo_t * pd,
        unsigned char *achDice[], unsigned char *achPip[],
        const int s, int x, int y, int fColour, int n, int alpha);

extern int UpdateMove(BoardData * bd, TanBoard anBoard);
extern void stop_board_expose(BoardData * bd);

#endif
