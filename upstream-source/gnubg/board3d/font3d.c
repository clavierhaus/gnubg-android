/*
 * Copyright (C) 2005-2019 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: font3d.c,v 1.48 2022/03/14 21:52:48 plm Exp $
 */

/* Draw 3d numbers using vera font and freetype library */

#include "config.h"

#include "fun3d.h"
#include "types3d.h"
#include "util.h"
#include "output.h"
#include <glib/gi18n.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define BEZIER_STEPS 5
#define BEZIER_STEP_SIZE 0.2
static double controlPoints[4][2];      /* 2D array storing values of de Casteljau algorithm */

int
CreateNumberFont(OGLFont *ppFont, const char *fontFile, int pitch, float size, float heightRatio)
{
    char *filename;
    FT_Library ftLib;
    if (FT_Init_FreeType(&ftLib))
        return 0;

    filename = BuildFilename(fontFile);
    if (!CreateOGLFont(ftLib, ppFont, filename, pitch, size, heightRatio)) {
        outputerrf(_("Failed to create font from (%s)\n"), filename);
        g_free(filename);
        return 0;
    }
    g_free(filename);
    return !FT_Done_FreeType(ftLib);
}

FT_Pos
GetKern(const OGLFont * pFont, char cur, char next)
{
    if (next)
        return pFont->kern[cur - '0'][next - '0'];
    else
        return 0;
}

float
getTextLen3d(const OGLFont * pFont, const char *str)
{
    long int len = 0;
    while (*str) {
        len += pFont->advance + GetKern(pFont, str[0], str[1]);
        str++;
    }
    return ((float) len / 64.0f) * pFont->scale;
}

static void
AddPoint(GArray* points, double x, double y)
{
	if (points->len > 0) {      /* Ignore duplicate contour points */
		Point3d* point = &g_array_index(points, Point3d, points->len - 1);
		/* See if point is duplicated (often happens when tesselating)
		 * so suppress lint error 777 (testing floats for equality) */
		if ( /*lint --e(777) */ point->data[0] == x && point->data[1] == y)
			return;
	}

	{
		Point3d newPoint;
		newPoint.data[0] = x;
		newPoint.data[1] = y;
		newPoint.data[2] = 0;

		g_array_append_val(points, newPoint);
	}
}

static void
evaluateQuadraticCurve(GArray* points)
{
	int i;
	for (i = 0; i <= BEZIER_STEPS; i++) {
		double bezierValues[2][2];

		double t = i * BEZIER_STEP_SIZE;

		bezierValues[0][0] = (1.0 - t) * controlPoints[0][0] + t * controlPoints[1][0];
		bezierValues[0][1] = (1.0 - t) * controlPoints[0][1] + t * controlPoints[1][1];

		bezierValues[1][0] = (1.0 - t) * controlPoints[1][0] + t * controlPoints[2][0];
		bezierValues[1][1] = (1.0 - t) * controlPoints[1][1] + t * controlPoints[2][1];

		bezierValues[0][0] = (1.0 - t) * bezierValues[0][0] + t * bezierValues[1][0];
		bezierValues[0][1] = (1.0 - t) * bezierValues[0][1] + t * bezierValues[1][1];

		AddPoint(points, bezierValues[0][0], bezierValues[0][1]);
	}
}

static void
PopulateContour(GArray* contour, const FT_Vector* points, const char* pointTags, int numberOfPoints)
{
	int pointIndex;

	for (pointIndex = 0; pointIndex < numberOfPoints; pointIndex++) {
		char pointTag = pointTags[pointIndex];

		if (pointTag == FT_Curve_Tag_On || numberOfPoints < 2) {
			AddPoint(contour, (double)points[pointIndex].x, (double)points[pointIndex].y);
		}
		else {
			int previousPointIndex = (pointIndex == 0) ? numberOfPoints - 1 : pointIndex - 1;
			int nextPointIndex = (pointIndex == numberOfPoints - 1) ? 0 : pointIndex + 1;
			Point3d controlPoint, previousPoint, nextPoint;
			controlPoint.data[0] = (double)points[pointIndex].x;
			controlPoint.data[1] = (double)points[pointIndex].y;
			previousPoint.data[0] = (double)points[previousPointIndex].x;
			previousPoint.data[1] = (double)points[previousPointIndex].y;
			nextPoint.data[0] = (double)points[nextPointIndex].x;
			nextPoint.data[1] = (double)points[nextPointIndex].y;

			g_assert(pointTag == FT_Curve_Tag_Conic);   /* Only this main type supported */

			while (pointTags[nextPointIndex] == FT_Curve_Tag_Conic) {
				nextPoint.data[0] = (controlPoint.data[0] + nextPoint.data[0]) * 0.5;
				nextPoint.data[1] = (controlPoint.data[1] + nextPoint.data[1]) * 0.5;

				controlPoints[0][0] = previousPoint.data[0];
				controlPoints[0][1] = previousPoint.data[1];
				controlPoints[1][0] = controlPoint.data[0];
				controlPoints[1][1] = controlPoint.data[1];
				controlPoints[2][0] = nextPoint.data[0];
				controlPoints[2][1] = nextPoint.data[1];

				evaluateQuadraticCurve(contour);

				pointIndex++;
				previousPoint = nextPoint;
				controlPoint.data[0] = (double)points[pointIndex].x;
				controlPoint.data[1] = (double)points[pointIndex].y;
				nextPointIndex = (pointIndex == numberOfPoints - 1) ? 0 : pointIndex + 1;
				nextPoint.data[0] = (double)points[nextPointIndex].x;
				nextPoint.data[1] = (double)points[nextPointIndex].y;
			}

			controlPoints[0][0] = previousPoint.data[0];
			controlPoints[0][1] = previousPoint.data[1];
			controlPoints[1][0] = controlPoint.data[0];
			controlPoints[1][1] = controlPoint.data[1];
			controlPoints[2][0] = nextPoint.data[0];
			controlPoints[2][1] = nextPoint.data[1];

			evaluateQuadraticCurve(contour);
		}
	}
}

void
PopulateVectoriser(Vectoriser * pVect, const FT_Outline * pOutline)
{
    int startIndex = 0;
    int contourIndex;

    pVect->contours = g_array_new(FALSE, FALSE, sizeof(Contour));
    pVect->numPoints = 0;

    for (contourIndex = 0; contourIndex < pOutline->n_contours; contourIndex++) {
        Contour newContour;
        FT_Vector *pointList = &pOutline->points[startIndex];
        char *tagList = &pOutline->tags[startIndex];

        int endIndex = pOutline->contours[contourIndex];
        int contourLength = (endIndex - startIndex) + 1;

        newContour.conPoints = g_array_new(FALSE, FALSE, sizeof(Point3d));
        PopulateContour(newContour.conPoints, pointList, tagList, contourLength);
        pVect->numPoints += newContour.conPoints->len;
        g_array_append_val(pVect->contours, newContour);

        startIndex = endIndex + 1;
    }
}

extern float
GetFontHeight3d(const OGLFont* font)
{
	return font->height;
}

GList* combineList;
static Tesselation curTess;

extern void
TidyMemory(const Vectoriser* pVect, const Mesh* pMesh)
{
	GList* pl;
	unsigned int c, i;

	for (c = 0; c < pVect->contours->len; c++) {
		Contour* contour = &g_array_index(pVect->contours, Contour, c);
		g_array_free(contour->conPoints, TRUE);
	}
	g_array_free(pVect->contours, TRUE);
	for (i = 0; i < pMesh->tesselations->len; i++) {
		Tesselation* subMesh = &g_array_index(pMesh->tesselations, Tesselation, i);
		g_array_free(subMesh->tessPoints, TRUE);
	}
	g_array_free(pMesh->tesselations, TRUE);

	for (pl = g_list_first(combineList); pl; pl = g_list_next(pl)) {
		g_free(pl->data);
	}
	g_list_free(combineList);
}

void TESS_CALLBACK
tcbError(GLenum error)
{                               /* This is very unlikely to happen... */
	g_print(_("Tessellation error! (%d)\n"), error);
}

void TESS_CALLBACK
tcbVertex(void* data, Mesh* UNUSED(pMesh))
{
	double* vertex = (double*)data;
	Point3d newPoint;

	newPoint.data[0] = vertex[0];
	newPoint.data[1] = vertex[1];
	newPoint.data[2] = vertex[2];

	g_array_append_val(curTess.tessPoints, newPoint);
}

void TESS_CALLBACK
tcbCombine(const double coords[3], const double* UNUSED(vertex_data[4]), float UNUSED(weight[4]), void** outData)
{
	/* Just return vertex position (colours etc. not required) */
	Point3d* newEle = (Point3d*)g_malloc(sizeof(Point3d));

	memcpy(newEle->data, coords, sizeof(double[3]));

	combineList = g_list_append(combineList, newEle);
	*outData = newEle;
}

void TESS_CALLBACK
tcbBegin(GLenum type, Mesh* UNUSED(pMesh))
{
	curTess.tessPoints = g_array_new(FALSE, FALSE, sizeof(Point3d));
	curTess.meshType = type;
}

void TESS_CALLBACK
tcbEnd( /*lint -e{818} */ Mesh* pMesh)
{                               /* Declaring pMesh as constant isn't useful as pointer member is modified (error 818) */
	g_array_append_val(pMesh->tesselations, curTess);
}

#include "ShimOGL.h"
#include "BoardDimensions.h"

extern void
RenderString3d(const OGLFont* pFont, const char* str, float scale, int MAA)
{
	glScalef(scale, scale * pFont->heightRatio, 1.f);

	while (*str) {
		int offset = *str - '0';
		if (!MAA)
		{
			/* Draw character */
			OglModelDraw(&pFont->modelManager, offset, NULL);
		}
		else
		{
			/* AA outline of character */
#if !GTK_CHECK_VERSION(3,0,0)
			RenderCharAA(pFont->AAglyphs + (unsigned int)offset);
#endif
		}
		/* Move on to next place */
		glTranslatef((float)(pFont->advance + GetKern(pFont, str[0], str[1])) / 64.0f, 0.f, 0.f);

		str++;
	}
}

extern void
glPrintCube(const OGLFont* cubeFont, const char* text, int MAA)
{
	/* Align horizontally and vertically */
	float scale = cubeFont->scale;
	float heightOffset = cubeFont->height;
	float xFactor = 1.0;

	switch (strlen(text)) {     /* Make font smaller for multi-digit cube numbers */
	case 1:
		/* base values are fine */
		break;
	case 2:
		scale *= CUBE_TWODIGIT_FACTOR;
		heightOffset *= CUBE_TWODIGIT_FACTOR;
		xFactor = CUBE_TWODIGIT_FACTOR;
		break;
	case 3:
		scale *= CUBE_THREEDIGIT_FACTOR;
		heightOffset *= CUBE_THREEDIGIT_FACTOR;
		xFactor = CUBE_THREEDIGIT_FACTOR;
		break;
	case 4:
		scale *= CUBE_FOURDIGIT_FACTOR;
		heightOffset *= CUBE_FOURDIGIT_FACTOR;
		xFactor = CUBE_FOURDIGIT_FACTOR;
		break;
	default:
		g_assert_not_reached();
	}

	glTranslatef(-getTextLen3d(cubeFont, text) * xFactor /2.0f, -heightOffset / 2.0f, 0.f);
	RenderString3d(cubeFont, text, scale, MAA);
}

extern void
glPrintPointNumbers(const OGLFont* numberFont, const char* text, int MAA)
{
	/* Align horizontally */
	glPushMatrix();
	glTranslatef(-getTextLen3d(numberFont, text) / 2.0f, 0.f, 0.f);
	RenderString3d(numberFont, text, numberFont->scale, MAA);
	glPopMatrix();
}

void
DrawNumbers(const OGLFont* numberFont, unsigned int sides, int swapNumbers, int MAA)
{
	int i;
	char num[3];
	float x;
	float textHeight = GetFontHeight3d(numberFont);
	int n;

	glPushMatrix();
	glTranslatef(0.f, (EDGE_HEIGHT - textHeight) / 2.0f, BASE_DEPTH + EDGE_DEPTH);
	x = TRAY_WIDTH - PIECE_HOLE / 2.0f;

	for (i = 0; i < 12; i++) {
		x += PIECE_HOLE;
		if (i == 6)
			x += BAR_WIDTH;

		if ((i < 6 && (sides & 1)) || (i >= 6 && (sides & 2))) {
			glPushMatrix();
			glTranslatef(x, 0.f, 0.f);
			if (!fClockwise)
				n = 12 - i;
			else
				n = i + 1;

			if (swapNumbers)
				n = 25 - n;

			sprintf(num, "%d", n);
			glPrintPointNumbers(numberFont, num, MAA);

			glPopMatrix();
		}
	}

	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.f, TOTAL_HEIGHT - textHeight - (EDGE_HEIGHT - textHeight) / 2.0f, BASE_DEPTH + EDGE_DEPTH);
	x = TRAY_WIDTH - PIECE_HOLE / 2.0f;

	for (i = 0; i < 12; i++) {
		x += PIECE_HOLE;
		if (i == 6)
			x += BAR_WIDTH;
		if ((i < 6 && (sides & 1)) || (i >= 6 && (sides & 2))) {
			glPushMatrix();
			glTranslatef(x, 0.f, 0.f);
			if (!fClockwise)
				n = 13 + i;
			else
				n = 24 - i;

			if (swapNumbers)
				n = 25 - n;

			sprintf(num, "%d", n);
			glPrintPointNumbers(numberFont, num, MAA);

			glPopMatrix();
		}
	}
	glPopMatrix();
}
