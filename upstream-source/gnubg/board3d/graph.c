/*
 * Copyright (C) 2003-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2006-2021 the AUTHORS
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
 * $Id: graph.c,v 1.46 2022/03/06 22:45:43 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "renderprefs.h"
#include "gtklocdefs.h"
#include <glib/gi18n.h>
#include "common.h"
#include "analysis.h"

#define COL_WIDTH 5.f
#define MID_GAP 1
#define INTER_GAP 4
#define TOTAL_GAP 5
#define RES_WIDTH (2 * COL_WIDTH + MID_GAP + INTER_GAP)
#define NUM_WIDTH (modelWidth * NUM_WIDTH_PER)
#define NUM_WIDTH_PER .1f
#define NUM_HEIGHT (modelHeight * NUM_HEIGHT_PER)
#define NUM_HEIGHT_PER .15f

static float modelWidth, modelHeight;
static OGLFont numberFont, totalText;

static gboolean
graph_button_press_event(void)
{
    return FALSE;
}

static void
configureCB(GtkWidget * widget, void *data)
{
    const GraphData *gd = (const GraphData *) data;
    int width, height;
    float maxY, maxX;
    GtkAllocation allocation;

    gtk_widget_get_allocation(widget, &allocation);
    width = allocation.width;
    height = allocation.height;
    maxX = (float) gd->numGames * RES_WIDTH + RES_WIDTH + TOTAL_GAP;
    modelWidth = maxX * (NUM_WIDTH_PER + 1 + NUM_WIDTH_PER);

    maxY = gd->maxY * 1.05f + 1;
    modelHeight = maxY * (1 + NUM_HEIGHT_PER);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) modelWidth, 0.0, (double) modelHeight, -1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void
realizeCB(void *UNUSED(data))
{
    /* Deep blue background colour */
    glClearColor(.2f, .2f, .4f, 1.f);

    if (!CreateNumberFont(&numberFont, FONT_VERA, FONT_PITCH, FONT_SIZE, FONT_HEIGHT_RATIO))
        g_print(_("Error creating font\n"));

    /* Translate using only the characters contained in Vera font */
    if (!CreateFontText(&totalText, _("Totals"), FONT_VERA, FONT_PITCH, FONT_SIZE, FONT_HEIGHT_RATIO))
        g_print(_("Error creating font\n"));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
DrawBar(const float col[4], float x, float y, float first, float second)
{
    glPushMatrix();
    glTranslatef(x, y, 0.f);

    glBegin(GL_QUADS);
    glColor3f(0.f, 1.f, 0.f);
    glVertex2f(0.f, 0.f);
    glVertex2f(COL_WIDTH, 0.f);
    glVertex2f(COL_WIDTH, first);
    glVertex2f(0.f, first);

    glColor3f(0.f, 0.f, 1.f);
    glVertex2f(0.f, first);
    glVertex2f(COL_WIDTH, first);
    glVertex2f(COL_WIDTH, first + second);
    glVertex2f(0.f, first + second);
    glEnd();

    glLineWidth(3.f);
    glColor3f(col[0], col[1], col[2]);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0.f, 0.f);
    glVertex2f(0.f, first + second);
    glVertex2f(COL_WIDTH, first + second);
    glVertex2f(COL_WIDTH, 0.f);
    glEnd();
    glLineWidth(1.f);
    glBegin(GL_POINTS);
    glVertex2f(0.f, first + second);
    glVertex2f(COL_WIDTH, first + second);
    glEnd();

    glPopMatrix();
}

static void
DrawColourBar(int playerCol, float x, float y, float first, float second)
{
    float col[4];
    int i;
    renderdata *prd = GetMainAppearance();
    for (i = 0; i < 4; i++) {
        if (display_is_2d(prd))
            col[i] = (float) prd->aarColour[playerCol][i];
        else
            col[i] = prd->ChequerMat[playerCol].ambientColour[i];
    }
    DrawBar(col, x, y, first, second);
}

static void
DrawBars(unsigned int num, float *const *values, int totalBar)
{
    float x = NUM_WIDTH + RES_WIDTH * (float) num;
    if (totalBar)
        x += TOTAL_GAP;

    DrawColourBar(0, x + INTER_GAP / 2.0f, NUM_HEIGHT, values[0][0], values[0][1]);
    DrawColourBar(1, x + INTER_GAP / 2.0f + COL_WIDTH + MID_GAP, NUM_HEIGHT, values[1][0], values[1][1]);
}

static void
PrintBottomNumber(unsigned int num, float width, float height, float x, float y)
{
    char numStr[11];
    sprintf(numStr, "%u", num);

    glPushMatrix();
    glTranslatef(x, y, 0.f);

    glColor3f(1.f, 1.f, 1.f);
    glScalef(width, height, 1.f);
    glLineWidth(.5f);
    glPrintCube(&numberFont, numStr, 0);        //TODO No AA at the moment
    glPopMatrix();
}

static void
PrintSideNumber(int num, float width, float height, float x, float y)
{
    char numStr[10];
    sprintf(numStr, "%d", num);

    glPushMatrix();
    glTranslatef(x, y, 0.f);

    glScalef(width, height, 1.f);
    glLineWidth(.5f);
    glPrintNumbersRA(&numberFont, numStr);

    glPopMatrix();
}

static void
DrawLeftAxis(const GraphData * pgd)
{
    int scale[] = { 1, 5, 10, 20, 50, 100, 0 };
    int *pScale = scale;
    int i, numPoints, pointInc;

    while (pScale[1] && pgd->maxY > (float) *pScale * 5.f)
        pScale++;

    pointInc = *pScale;
    numPoints = (int) (pgd->maxY / (float) pointInc);
    if (numPoints == 0)
        numPoints = 1;

    for (i = 1; i <= numPoints; i++) {
        float y = NUM_HEIGHT;

        y += (float) (i * pointInc);
        glColor3f(1.f, 1.f, 1.f);
        PrintSideNumber(i * pointInc, NUM_WIDTH * 10, NUM_HEIGHT * 10, NUM_WIDTH - 1, y);

        glColor3f(.5f, .5f, .5f);
        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glBegin(GL_LINES);
        glVertex2f(NUM_WIDTH - .5f, y);
        glVertex2f(modelWidth - 1, y);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
    }
}

static void
DrawGraph(const GraphData * gd)
{
    unsigned int i;
    float lastx = 0;

    glPushMatrix();
    glTranslatef(NUM_WIDTH + RES_WIDTH * (float) gd->numGames + TOTAL_GAP + COL_WIDTH + (INTER_GAP + MID_GAP) / 2.0f,
                 NUM_HEIGHT / 2, 0.f);

    glColor3f(1.f, 1.f, 1.f);
    glLineWidth(.5f);
    glScalef(NUM_WIDTH * 10, NUM_HEIGHT * 10, 1.f);
    glDrawText(&totalText);

    glPopMatrix();

    DrawLeftAxis(gd);

    for (i = 0; i < gd->numGames; i++) {
        float x = NUM_WIDTH + RES_WIDTH * (float) i + COL_WIDTH + (INTER_GAP + MID_GAP) / 2.0f;

        DrawBars(i, gd->data[i], 0);

        if (x > lastx + NUM_WIDTH) {
            PrintBottomNumber(i + 1, NUM_WIDTH * 10, NUM_HEIGHT * 10, x, NUM_HEIGHT / 2.0f);
            lastx = x;
        }
    }

    /* Total bars */
    DrawBars(i, gd->data[i], 1);

    /* Axis */
    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_LINES);
    glVertex2f(NUM_WIDTH, NUM_HEIGHT);
    glVertex2f(modelWidth - 1, NUM_HEIGHT);
    glVertex2f(NUM_WIDTH, NUM_HEIGHT);
    glVertex2f(NUM_WIDTH, modelHeight * .95f);
    glEnd();
    glBegin(GL_POINTS);
    glVertex2f(NUM_WIDTH, NUM_HEIGHT);
    glEnd();
}

static gboolean
exposeCB(GtkWidget * UNUSED(widget), GdkEventExpose * UNUSED(eventData), void *data)
{
    const GraphData *gd = (const GraphData *) data;
    glClear(GL_COLOR_BUFFER_BIT);
    DrawGraph(gd);
    return TRUE;
}

static void
destroy_event(GtkWidget * UNUSED(widget), void *UNUSED(data))
{
    FreeNumberFont(&numberFont);
    FreeTextFont(&totalText);
}

GtkWidget *
StatGraph(GraphData * pgd)
{
    float f1, f2;
    GtkWidget *pw;

    /* Drawing widget for OpenGL */
    pw = GLWidgetCreate(realizeCB, configureCB, exposeCB, pgd);
    if (pw == NULL)
        return NULL;

    f1 = pgd->data[pgd->numGames][0][0] + pgd->data[pgd->numGames][0][1];
    f2 = pgd->data[pgd->numGames][1][0] + pgd->data[pgd->numGames][1][1];
    pgd->maxY = (f1 > f2) ? f1 : f2;
    if (pgd->maxY < .5f)
        pgd->maxY = .5f;

    gtk_widget_set_events(pw, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(pw), "button_press_event", G_CALLBACK(graph_button_press_event), pgd);
    g_signal_connect(G_OBJECT(pw), "destroy", G_CALLBACK(destroy_event), NULL);

    return pw;
}

void
SetNumGames(GraphData * pgd, unsigned int numGames)
{
    pgd->numGames = numGames;
    pgd->data = Alloc3d(numGames + 1, 2, 2);
}

extern GraphData *
CreateGraphData(void)
{
    return (GraphData *) g_malloc(sizeof(GraphData));
}

void
TidyGraphData(GraphData * pgd)
{
    Free3d(pgd->data, pgd->numGames + 1, 2);
    g_free(pgd);
}

void
AddGameData( /*lint -e{818} */ GraphData * pgd, int game, const statcontext * psc)
{
    float aaaar[3][2][2][2];
    float f1, s1, f2, s2;
    getMWCFromError(psc, aaaar);

    f1 = psc->arErrorCheckerplay[0][0] * 10;
    s1 = aaaar[CUBEDECISION][TOTAL][PLAYER_0][0] * 10;
    f2 = psc->arErrorCheckerplay[1][0] * 10;
    s2 = aaaar[CUBEDECISION][TOTAL][PLAYER_1][0] * 10;

    pgd->data[game][0][0] = f1;
    pgd->data[game][0][1] = s1;
    pgd->data[game][1][0] = f2;
    pgd->data[game][1][1] = s2;
}
