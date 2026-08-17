/*
 * Copyright (C) 2016-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2019-2020 the AUTHORS
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
 * $Id: model.h,v 1.11 2020/05/26 19:55:54 plm Exp $
 */

#ifndef MODEL_H
#define MODEL_H

#include <glib.h>

/* Occlusion model */
typedef struct {
    GArray *planes;
    GArray *edges;
    GArray *points;
} OccModel;

typedef struct {
    float invMat[4][4];
    float trans[3];
    float rot[3];
    int rotator;

    unsigned int shadow_list;
    OccModel *handle;
    int show;
} Occluder;

extern void GenerateShadowVolume(const Occluder * pOcc, const float olight[4]);

extern void initOccluder(Occluder * pOcc);
extern void freeOccluder(Occluder * pOcc);
extern void copyOccluder(const Occluder * fromOcc, Occluder * toOcc);
extern void moveToOcc(const Occluder * pOcc);

extern void addClosedSquare(Occluder * pOcc, float x, float y, float z, float w, float h, float d);
extern void addSquare(Occluder * pOcc, float x, float y, float z, float w, float h, float d);
extern void addSquareCentered(Occluder * pOcc, float x, float y, float z, float w, float h, float d);
extern void addCube(Occluder * pOcc, float x, float y, float z, float w, float h, float d);
extern void addWonkyCube(Occluder * pOcc, float x, float y, float z, float w, float h, float d, float s, int full);
extern void addCylinder(Occluder * pOcc, float x, float y, float z, float r, float d, unsigned int numSteps);
extern void addHalfTube(Occluder * pOcc, float r, float h, unsigned int numSteps);
extern void addDice(Occluder * pOcc, float size);

extern void draw_shadow_volume_extruded_edges(Occluder * pOcc, const float light_position[4], unsigned int prim);

#endif
