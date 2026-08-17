/*
 * Copyright (C) 2003-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2003-2018 the AUTHORS
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
 * $Id: widget3d.c,v 1.65 2022/04/22 21:07:49 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "tr.h"
#include "gtklocdefs.h"
#include "gtkboard.h"

static void
configure_3dCB(GtkWidget * widget, void* data)
{
	BoardData* bd = (BoardData*)data;
    if (display_is_3d(bd->rd)) {
        static int curHeight = -1, curWidth = -1;
        GtkAllocation allocation;
        int width, height;
        gtk_widget_get_allocation(widget, &allocation);
        width = allocation.width;
        height = allocation.height;
        if (width != curWidth || height != curHeight)
		{
			int viewport[4] = { 0, 0, width, height };
			glSetViewport(viewport);
            SetupViewingVolume3d(bd, bd->bd3d, bd->rd, viewport);

            curWidth = width;
            curHeight = height;
        }
    }
}

static void
realize_3dCB(void* data)
{
    BoardData* bd = (BoardData*)data;
    InitGL(bd);
    GetTextures(bd->bd3d, bd->rd);
    preDraw3d(bd, bd->bd3d, bd->rd);

#ifdef WIN32
    if (fResetSync) {
        fResetSync = FALSE;
        (void) setVSync(fSync);
    }
#endif
}

void
MakeCurrent3d(const BoardData3d * bd3d)
{
    GLWidgetMakeCurrent(bd3d->drawing_area3d);
}

void
UpdateShadows(BoardData3d * bd3d)
{
    bd3d->shadowsOutofDate = TRUE;
}

static gboolean
expose_3dCB(GtkWidget* UNUSED(widget), GdkEventExpose* UNUSED(exposeEvent), void* data)
{
    const BoardData* bd = (const BoardData*)data;

    /*
     * Calling RecalcViewingVolume() here is a work around to avoid
     * artifacts in 3D display after the stats histogram has been shown.
     * Doing this at each expose is probably not the best fix.
     */

    RecalcViewingVolume(bd);

    if (bd->bd3d->shadowsOutofDate) {   /* Update shadow positions */
        bd->bd3d->shadowsOutofDate = FALSE;
        updateOccPos(bd);
    }
#ifdef TEST_HARNESS
    // Test harness for 3d board development
    TestHarnessDraw(bd);
#else
    // Normal drawing
    Draw3d(bd);
#endif
    return TRUE;
}

extern int
CreateGLWidget(BoardData * bd, int useMouseEvents)
{
    GtkWidget *p3dWidget;
    int signals;

    /*
     * Initialise this else valgrind reports
     * use of uninitialised values in GL related code
     */
    bd->bd3d = (BoardData3d *) g_malloc0(sizeof(BoardData3d));

    InitBoard3d(bd, bd->bd3d);

    /* Drawing area for OpenGL */
    p3dWidget = bd->bd3d->drawing_area3d = GLWidgetCreate(realize_3dCB, configure_3dCB, expose_3dCB, bd);
    if (p3dWidget == NULL)
        return FALSE;

    /* set up events and signals for OpenGL widget */
    signals = GDK_EXPOSURE_MASK;
    if (useMouseEvents)
        signals |= GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK;
    gtk_widget_set_events(p3dWidget, signals);

    if (useMouseEvents)
    {
        g_signal_connect(G_OBJECT(p3dWidget), "button_press_event", G_CALLBACK(board_button_press), bd);
        g_signal_connect(G_OBJECT(p3dWidget), "button_release_event", G_CALLBACK(board_button_release), bd);
        g_signal_connect(G_OBJECT(p3dWidget), "motion_notify_event", G_CALLBACK(board_motion_notify), bd);
    }

    return TRUE;
}

gboolean
InitGTK3d(int *argc, char ***argv)
{
	gboolean initOkay = GLInit(argc, argv);
	if (initOkay)
	{
		/* Call LoadTextureInfo to get texture details from textures.txt */
		LoadTextureInfo();
		SetupFlag();
	}

	return initOkay;
}

gboolean
RenderToBuffer3d(GtkWidget* widget, GdkEventExpose* UNUSED(eventData), void* data)
{
	const RenderToBufferData* renderToBufferData = (const RenderToBufferData*)data;
    TRcontext *tr;
    GtkAllocation allocation;
	BoardData3d *bd3d = renderToBufferData->bd->bd3d;
	int fSaveBufs = bd3d->fBuffers;
	int viewport[4] = { 0, 0, renderToBufferData->width, renderToBufferData->height };

    /* Sort out tile rendering stuff */
    tr = trNew();
#define BORDER 10
    gtk_widget_get_allocation(widget, &allocation);
    trTileSize(tr, allocation.width, allocation.height, BORDER);
    trImageSize(tr, renderToBufferData->width, renderToBufferData->height);
    trImageBuffer(tr, GL_RGB, GL_UNSIGNED_BYTE, renderToBufferData->puch);

    /* Sort out viewing perspective */
	glSetViewport(viewport);
	SetupViewingVolume3d(renderToBufferData->bd, bd3d, renderToBufferData->bd->rd, viewport);

    if (renderToBufferData->bd->rd->planView)
        trOrtho(tr, -bd3d->horFrustrum, bd3d->horFrustrum, -bd3d->vertFrustrum, bd3d->vertFrustrum, 0.0, 5.0);
    else
        trFrustum(tr, -bd3d->horFrustrum, bd3d->horFrustrum, -bd3d->vertFrustrum, bd3d->vertFrustrum, zNearVAL, zFarVAL);

    bd3d->fBuffers = FALSE;     /* Turn this off whilst drawing */

    /* Draw tiles */
    do {
        trBeginTile(tr);
        Draw3d(renderToBufferData->bd);
    } while (trEndTile(tr));

    bd3d->fBuffers = fSaveBufs;

    trDelete(tr);

    /* Reset viewing volume for main screen */
	glGetIntegerv(GL_VIEWPORT, viewport);
	SetupViewingVolume3d(renderToBufferData->bd, bd3d, renderToBufferData->bd->rd, viewport);

	return FALSE;	// Don't swap buffers
}
