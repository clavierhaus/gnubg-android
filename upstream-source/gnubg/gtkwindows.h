/*
 * Copyright (C) 2006-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2006-2018 the AUTHORS
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
 * $Id: gtkwindows.h,v 1.26 2019/11/14 20:39:10 plm Exp $
 */

#ifndef GTKWINDOWS_H
#define GTKWINDOWS_H

#include <gtk/gtk.h>

#define GTK_STOCK_DIALOG_GNU "gtk-dialog-gnu"   /* stock gnu head icon */
#define GTK_STOCK_DIALOG_GNU_QUESTION "gtk-dialog-gnuquestion"  /* stock gnu head icon with question mark */
#define GTK_STOCK_DIALOG_GNU_BIG "gtk-dialog-gnubig"    /* large gnu icon */

enum {                          /* Dialog flags */
    DIALOG_FLAG_NONE = 0,
    DIALOG_FLAG_MODAL = 1,
    DIALOG_FLAG_NOOK = 2,
    DIALOG_FLAG_CLOSEBUTTON = 4,
    DIALOG_FLAG_NOTIDY = 8,
    DIALOG_FLAG_MINMAXBUTTONS = 16,
    DIALOG_FLAG_NORESPONSE = 32
};

typedef enum {
    DA_MAIN,
    DA_BUTTONS,
    DA_OK
} dialogarea;

typedef enum {
    DT_INFO,
    DT_QUESTION,
    DT_AREYOUSURE,
    DT_WARNING,
    DT_ERROR,
    DT_CUSTOM,
    NUM_DIALOG_TYPES
} dialogtype;

extern GtkWidget *GTKCreateDialog(const char *szTitle, const dialogtype dt,
                                  GtkWidget * parent, int flags, GCallback okFun, void *p);
extern GtkWidget *DialogArea(GtkWidget * pw, dialogarea da);
extern void GTKRunDialog(GtkWidget * dialog);

extern int GTKGetInputYN(char *szPrompt);
extern char *GTKGetInput(char *title, char *prompt, GtkWidget * parent);

extern int GTKMessage(const char *sz, dialogtype dt);
extern void GTKSetCurrentParent(GtkWidget * parent);
extern GtkWidget *GTKGetCurrentParent(void);

typedef enum {
    WARN_FULLSCREEN_EXIT = 0,
    WARN_SET_SHADOWS,
    WARN_UNACCELERATED,
    WARN_STOP,
    WARN_ENDGAME,
    WARN_RESIGN,
    WARN_ROLLOUT,
    WARN_NUM_WARNINGS
} warningType;

extern int GTKShowWarning(warningType warning, GtkWidget * pwParent);
extern warningType ParseWarning(char *str);
extern void SetWarningEnabled(warningType warning, int value);
extern int GetWarningEnabled(warningType warning);
extern void PrintWarning(warningType warning);
extern void WriteWarnings(FILE * pf);

#endif
