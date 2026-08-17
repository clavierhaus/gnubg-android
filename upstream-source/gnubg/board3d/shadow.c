/*
 * Copyright (C) 2003-2020 Jon Kinsey <jon_kinsey@hotmail.com>
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
 * $Id: shadow.c,v 1.33 2021/10/30 13:49:26 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "render.h"

#if !GTK_CHECK_VERSION(3,0,0)
static int midStencilVal;
#endif

extern int
ShadowsInitilised(const BoardData3d * bd3d)
{
    return (bd3d != NULL && bd3d->shadowsInitialised);
}

void
shadowInit(BoardData3d * bd3d, renderdata * prd)
{
    if (bd3d->shadowsInitialised)
        return;

#if !GTK_CHECK_VERSION(3,0,0)
    int stencilBits;
	
    /* Darkness as percentage of ambient light */
    prd->dimness = (float)(prd->lightLevels[1] * (100 - prd->shadowDarkness)) / (100.0f * 100.0f);

    initOccluders(bd3d);

    /* Check the stencil buffer is present */
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (!stencilBits) {
        g_warning(_("No stencil buffer, no shadows\n"));
        return;
    }
    midStencilVal = (int) (1u << (stencilBits - 1));
    glClearStencil(midStencilVal);

    bd3d->shadowsInitialised = TRUE;
#else
    (void)prd;	/* suppress unused parameter compiler warning */
#endif
}

#ifdef TEST_HARNESS
void GenerateShadowEdges(const Occluder * pOcc);
extern void
draw_shadow_volume_edges(Occluder * pOcc)
{
    if (pOcc->show) {
#if !GTK_CHECK_VERSION(3,0,0)
        glPushMatrix();
        moveToOcc(pOcc);
        GenerateShadowEdges(pOcc);
        glPopMatrix();
#endif
    }
}
#endif

extern void
draw_shadow_volume_extruded_edges( /*lint -e{818} */ Occluder * pOcc, const float light_position[4], unsigned int prim)
{
    if (pOcc->show) {
        float olight[4];
        mult_matrix_vec((ConstMatrix) pOcc->invMat, light_position, olight);

#if !GTK_CHECK_VERSION(3,0,0)
        glNewList(pOcc->shadow_list, GL_COMPILE);
        glPushMatrix();
        moveToOcc(pOcc);
        glBegin(prim);
        GenerateShadowVolume(pOcc, olight);
        glEnd();
        glPopMatrix();
        glEndList();
#else
        (void)prim;	/* suppress unused parameter compiler warning */
#endif
    }
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
draw_shadow_volume_to_stencil(const BoardData3d * bd3d)
{
    /* First clear the stencil buffer */
    glClear(GL_STENCIL_BUFFER_BIT);

    /* Disable drawing to colour buffer, lighting and don't update depth buffer */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);

    /* Z-pass approach */
    glStencilFunc(GL_ALWAYS, midStencilVal, (unsigned int) ~ 0u);

    glCullFace(GL_FRONT);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

    DrawShadows(bd3d);

    glCullFace(GL_BACK);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    DrawShadows(bd3d);

    /* Enable colour buffer, lighting and depth buffer */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
}
#endif

void
shadowDisplay(const BoardData * bd, const BoardData3d * bd3d, const renderdata * prd)
{
#if !GTK_CHECK_VERSION(3,0,0)
    float zero[4] = { 0, 0, 0, 0 };
    float d1[4];
    float specular[4];
    float diffuse[4];

	/* Create shadow volume in stencil buffer */
    glEnable(GL_STENCIL_TEST);
    draw_shadow_volume_to_stencil(bd3d);

    /* Pass 2: Redraw model, dim light in shadowed areas */
    glStencilFunc(GL_NOTEQUAL, midStencilVal, (unsigned int) ~ 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    d1[0] = d1[1] = d1[2] = d1[3] = prd->dimness;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, d1);
    /* No specular light */
    glGetLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_SPECULAR, zero);

	drawBoard(bd, bd3d, prd);

    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glDisable(GL_STENCIL_TEST);
#else
    (void)bd;	/* suppress unused parameter compiler warning */
    (void)bd3d;	/* suppress unused parameter compiler warning */
    (void)prd;	/* suppress unused parameter compiler warning */
#endif
}
