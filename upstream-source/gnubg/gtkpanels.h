/*
 * Copyright (C) 2006 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2008 the AUTHORS
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
 * $Id: gtkpanels.h,v 1.8 2020/02/23 21:00:47 plm Exp $
 */

/* position of windows: main window, game list, and annotation */

#ifndef GTKPANELS_H
#define GTKPANELS_H

typedef enum {
    WINDOW_MAIN = 0,
    WINDOW_GAME,
    WINDOW_ANALYSIS,
    WINDOW_ANNOTATION,
    WINDOW_HINT,
    WINDOW_MESSAGE,
    WINDOW_COMMAND,
    WINDOW_THEORY,
    NUM_WINDOWS
} gnubgwindow;

typedef struct {
    int nWidth, nHeight;
    int nPosX, nPosY, max;
} windowgeometry;

extern void SaveWindowSettings(FILE * pf);
extern void HidePanel(gnubgwindow window);
extern void getWindowGeometry(gnubgwindow window);
extern int PanelShowing(gnubgwindow window);
extern void ClosePanels(void);

extern int GetPanelSize(void);
extern void SetPanelWidth(int size);
extern void GTKGameSelectDestroy(void);

#endif
