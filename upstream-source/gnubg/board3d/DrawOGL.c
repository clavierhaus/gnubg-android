/*
 * Copyright (C) 2019-2021 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: DrawOGL.c,v 1.24 2022/04/21 20:50:21 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"

#include "fun3d.h"
#include "BoardDimensions.h"
#include "gtkboard.h"

#include <cglm/affine.h>
#include <cglm/cam.h>

static void drawNumbers(const BoardData* bd, int MAA);
#if !GTK_CHECK_VERSION(3,0,0)
static void MAAtidyEdges(const renderdata* prd);
#endif
extern void drawPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);
static void drawSpecialPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);

///////////////////////////////////////
// Legacy Opengl board rendering code

#if !GTK_CHECK_VERSION(3,0,0)
void LegacyStartAA(float width)
{
	glLineWidth(width);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
}

void LegacyEndAA(void)
{
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

void RenderCharAA(unsigned int glyph)
{
	glPushMatrix();
		glLoadMatrixf(GetModelViewMatrix());
		glCallList(glyph);
	glPopMatrix();
}

void SetLineDrawingmode(int enable)
{
	if (enable == GL_TRUE)
		glPolygonMode(GL_FRONT, GL_LINE);
	else
		glPolygonMode(GL_FRONT, GL_FILL);
}
#endif

void
drawBoard(const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd)
{
	const ModelManager* modelHolder = &bd3d->modelHolder;

	if (!bd->grayBoard)
		drawTable(modelHolder, bd3d, prd);
	else
		drawTableGrayed(modelHolder, bd3d, *prd);

	if (prd->fLabels)
	{
		drawNumbers(bd, 0);

#if !GTK_CHECK_VERSION(3,0,0)
		LegacyStartAA(0.5f);
		drawNumbers(bd, 1);
		LegacyEndAA();
#endif
	}

#if !GTK_CHECK_VERSION(3,0,0)
	MAAtidyEdges(prd);
#endif

	if (bd->cube_use && !bd->crawford_game)
		drawDC(modelHolder, bd, bd3d, &prd->CubeMat, 1);

	if (prd->showMoveIndicator)
	{
		glDisable(GL_DEPTH_TEST);
		ShowMoveIndicator(modelHolder, bd);
		glEnable(GL_DEPTH_TEST);
	}

	/* Draw things in correct order so transparency works correctly */
	/* First pieces, then dice, then moving pieces */

	int transparentPieces = (prd->ChequerMat[0].alphaBlend) || (prd->ChequerMat[1].alphaBlend);
	if (transparentPieces) {	/* Draw back of piece separately */
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		drawPieces(modelHolder, bd, bd3d, prd);
		glCullFace(GL_BACK);
	}

	drawPieces(modelHolder, bd, bd3d, prd);

	if (transparentPieces)
		glDisable(GL_BLEND);

	if (bd->DragTargetHelp) {   /* highlight target points */
		SetLineDrawingmode(GL_TRUE);

		SetColour3d(0.f, 1.f, 0.f, 0.f);        /* Nice bright green... */

		for (int i = 0; i <= 3; i++) {
			int target = bd->iTargetHelpPoints[i];
			if (target != -1) { /* Make sure texturing is disabled */
				int separateTop = (prd->ChequerMat[0].pTexture && prd->pieceTextureType == PTT_TOP);

#if !GTK_CHECK_VERSION(3,0,0)
				if (prd->ChequerMat[0].pTexture)
					glDisable(GL_TEXTURE_2D);
#endif

				drawPiece(modelHolder, bd3d, (unsigned int)target, Abs(bd->points[target]) + 1, TRUE, (prd->pieceType == PT_ROUNDED), prd->curveAccuracy, separateTop);
			}
		}
		SetLineDrawingmode(GL_FALSE);
	}

	if (DiceShowing(bd)) {
		const Material* diceMat = &prd->DiceMat[(bd->turn == 1)];
		drawDie(modelHolder, bd, bd3d, prd, diceMat, 0, TRUE);
		drawDie(modelHolder, bd, bd3d, prd, diceMat, 1, TRUE);
	}

	if (bd3d->moving || bd->drag_point >= 0)
		drawSpecialPieces(modelHolder, bd, bd3d, prd);

	if (bd->resigned)
	{
		int isStencil = glIsEnabled(GL_STENCIL_TEST);
		if (isStencil)
			glDisable(GL_STENCIL_TEST);
		drawFlag(modelHolder, bd, bd3d, prd);
		if (isStencil)
			glEnable(GL_STENCIL_TEST);
	}
}

/* Define position of dots on dice */
static int dots1[] = { 2, 2, 0 };
static int dots2[] = { 1, 1, 3, 3, 0 };
static int dots3[] = { 1, 3, 2, 2, 3, 1, 0 };
static int dots4[] = { 1, 1, 1, 3, 3, 1, 3, 3, 0 };
static int dots5[] = { 1, 1, 1, 3, 2, 2, 3, 1, 3, 3, 0 };
static int dots6[] = { 1, 1, 1, 3, 2, 1, 2, 3, 3, 1, 3, 3, 0 };
static int* dots[6] = { dots1, dots2, dots3, dots4, dots5, dots6 };
#if !GTK_CHECK_VERSION(3,0,0)
static float dot_pos[] = { 0, 20, 50, 80 };       /* percentages across face */
#endif

extern void
drawDots(const ModelManager* modelHolder, const BoardData3d* bd3d, float diceSize, float dotOffset, const diceTest* dt, int showFront)
{
	int dot;
	int c;
	float radius;
	float ds = (diceSize * 5.0f / 7.0f);
	float hds = (ds / 2);
	float dotSize = diceSize / 10.0f;

#if !GTK_CHECK_VERSION(3,0,0)
	float x, y;
	int* dp;
	/* Remove specular effects */
	float zero[4] = { 0, 0, 0, 0 };
	glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
	glPushMatrix();
#endif

	radius = diceSize / 7.0f;

	for (c = 0; c < 6; c++) {
		int nd;

		if (c < 3)
			dot = c;
		else
			dot = 8 - c;

		/* Make sure top dots looks nice */
		nd = !bd3d->shakingDice && (dot == dt->top);

#if !GTK_CHECK_VERSION(3,0,0)
		if (bd3d->shakingDice || (showFront && dot != dt->bottom && dot != dt->side[0])
			|| (!showFront && dot != dt->top && dot != dt->side[2]))
#endif
		{
#if GTK_CHECK_VERSION(3,0,0)
			(void)dotOffset;	/* suppress unused parameter compiler warning */
			(void)showFront;
			(void)nd;

			DrawDotTemp(modelHolder, dotSize, ds, hds + radius + LIFT_OFF, dots[dot], c);
#else
			(void)modelHolder;	/* suppress unused parameter compiler warning */

			if (nd)
				glDisable(GL_DEPTH_TEST);

			glPushMatrix();
			glTranslatef(0.f, 0.f, hds + radius);

			glNormal3f(0.f, 0.f, 1.f);

			/* Show all the dots for this number */
			dp = dots[dot];
			do {
				x = (dot_pos[dp[0]] * ds) / 100;
				y = (dot_pos[dp[1]] * ds) / 100;

				glPushMatrix();
				glTranslatef(x - hds, y - hds, 0.f);

				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, bd3d->dotTexture);

				glBegin(GL_QUADS);
				glTexCoord2f(0.f, 1.f);
				glVertex3f(dotSize, dotSize, dotOffset);
				glTexCoord2f(1.f, 1.f);
				glVertex3f(-dotSize, dotSize, dotOffset);
				glTexCoord2f(1.f, 0.f);
				glVertex3f(-dotSize, -dotSize, dotOffset);
				glTexCoord2f(0.f, 0.f);
				glVertex3f(dotSize, -dotSize, dotOffset);
				glEnd();

				glPopMatrix();

				dp += 2;
			} while (*dp);

			glPopMatrix();
			if (nd)
				glEnable(GL_DEPTH_TEST);
#endif
		}

#if !GTK_CHECK_VERSION(3,0,0)
		if (c % 2 == 0)
			glRotatef(-90.f, 0.f, 1.f, 0.f);
		else
			glRotatef(90.f, 1.f, 0.f, 0.f);
#endif
	}
#if !GTK_CHECK_VERSION(3,0,0)
	glPopMatrix();
#endif
}

void DrawDots(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd, diceTest* dt, int diceCol)
{
	Material whiteMat;
	SetupSimpleMat(&whiteMat, 1.f, 1.f, 1.f);

#if !GTK_CHECK_VERSION(3,0,0)
	glPushMatrix();
	glLoadMatrixf(GetModelViewMatrix());
	/* Draw (front) dots */
	glEnable(GL_BLEND);
#endif
	/* First blank out space for dots */
	setMaterial(&whiteMat);
#if !GTK_CHECK_VERSION(3,0,0)
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
#endif
	drawDots(modelHolder, bd3d, getDiceSize(prd), LIFT_OFF, dt, 1);

#if !GTK_CHECK_VERSION(3,0,0)
	/* Now fill space with coloured dots */
	setMaterial(&prd->DiceDotMat[diceCol]);
	glBlendFunc(GL_ONE, GL_ONE);
	drawDots(modelHolder, bd3d, getDiceSize(prd), LIFT_OFF, dt, 1);

	/* Restore blending defaults */
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();
#else
	(void)diceCol;	/* suppress unused parameter compiler warning */
#endif
}

#if !GTK_CHECK_VERSION(3,0,0)
void DrawBackDice(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd, diceTest* dt, int diceCol)
{
	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);

	/* Draw dice */
	OglModelDraw(modelHolder, MT_DICE, &prd->DiceMat[diceCol]);

	/* Place back dots inside dice */
	setMaterial(&prd->DiceDotMat[diceCol]);
	glEnable(GL_BLEND);     /* NB. Disabled in diceList */
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	drawDots(modelHolder, bd3d, getDiceSize(prd), -LIFT_OFF, dt, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	// NB. GL_BLEND still enabled here (so front blends with back)
}
#else
void DrawBackDice(const ModelManager* UNUSED(modelHolder), const BoardData3d* UNUSED(bd3d), const renderdata* UNUSED(prd), diceTest* UNUSED(dt), int UNUSED(diceCol)) {}
#endif

static void
drawNumbers(const BoardData* bd, int MAA)
{
	int swapNumbers = (bd->turn == -1 && bd->rd->fDynamicLabels);

	/* No need to depth test as on top of board (depth test could cause alias problems too) */
	glDisable(GL_DEPTH_TEST);

	/* Draw inside then anti-aliased outline of numbers */
	setMaterial(&bd->rd->PointNumberMat);

	DrawNumbers(&bd->bd3d->numberFont, 1, swapNumbers, MAA);
	DrawNumbers(&bd->bd3d->numberFont, 2, swapNumbers, MAA);

	glEnable(GL_DEPTH_TEST);
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
MAAtidyEdges(const renderdata* prd)
{                               /* Anti-alias board edges */
	setMaterial(&prd->BoxMat);

	LegacyStartAA(1.0f);
	glDepthMask(GL_FALSE);

	glNormal3f(0.f, 0.f, 1.f);

	glBegin(GL_LINES);
	if (prd->roundedEdges) {
		/* bar */
		glNormal3f(-1.f, 0.f, 0.f);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		glNormal3f(1.f, 0.f, 0.f);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT,
			BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		/* left bear off tray */
		glNormal3f(-1.f, 0.f, 0.f);
		glVertex3f(0.f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(0.f, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		/* right bear off tray */
		glNormal3f(1.f, 0.f, 0.f);
		glVertex3f(TOTAL_WIDTH, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TOTAL_WIDTH, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT,
			BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT,
			BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

	}
	else {
		/* bar */
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		/* left bear off tray */
		glVertex3f(0.f, 0.f, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(0.f, TOTAL_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH - EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TRAY_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		/* right bear off tray */
		glVertex3f(TOTAL_WIDTH, 0.f, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH, TOTAL_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TOTAL_WIDTH - EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TRAY_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);

		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
		glVertex3f(TOTAL_WIDTH - TRAY_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
	}

	/* inner edges (sides) */
	glNormal3f(1.f, 0.f, 0.f);
	glVertex3f(EDGE_WIDTH + LIFT_OFF, EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(EDGE_WIDTH + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(TRAY_WIDTH + LIFT_OFF, EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(TRAY_WIDTH + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);

	glNormal3f(-1.f, 0.f, 0.f);
	glVertex3f(TOTAL_WIDTH - EDGE_WIDTH + LIFT_OFF, EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(TOTAL_WIDTH - EDGE_WIDTH + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + LIFT_OFF, EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glVertex3f(TOTAL_WIDTH - TRAY_WIDTH + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH + LIFT_OFF);
	glEnd();
	glDepthMask(GL_TRUE);
	LegacyEndAA();
}

extern void
MAAmoveIndicator(void)
{
	glPushMatrix();
	glLoadMatrixf(GetModelViewMatrix());
	/* Outline arrow */
	SetColour3d(0.f, 0.f, 0.f, 1.f);    /* Black outline */

	LegacyStartAA(0.5f);

	glBegin(GL_LINE_LOOP);
	glVertex2f(-ARROW_UNIT * 2, -ARROW_UNIT);
	glVertex2f(-ARROW_UNIT * 2, ARROW_UNIT);
	glVertex2f(0.f, ARROW_UNIT);
	glVertex2f(0.f, ARROW_UNIT * 2);
	glVertex2f(ARROW_UNIT * 2, 0.f);
	glVertex2f(0.f, -ARROW_UNIT * 2);
	glVertex2f(0.f, -ARROW_UNIT);
	glEnd();

	LegacyEndAA();
	glPopMatrix();
}

static void
circleOutlineOutward(float radius, float height, unsigned int accuracy)
{                               /* Draw an ouline of a disc in current z plane with outfacing normals */
	unsigned int i;
	float angle, step;

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i <= accuracy; i++) {
		glNormal3f(sinf(angle), cosf(angle), 0.f);
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle -= step;
	}
	glEnd();
}

static void
circleOutline(float radius, float height, unsigned int accuracy)
{                               /* Draw an ouline of a disc in current z plane */
	unsigned int i;
	float angle, step;

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i <= accuracy; i++) {
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle -= step;
	}
	glEnd();
}

extern void MAAdie(const renderdata* prd)
{
	/* Anti-alias dice edges */
	LegacyStartAA(1.0f);
	glDepthMask(GL_FALSE);

	float size = getDiceSize(prd);
	float radius = size / 2.0f;
	int c;

	glPushMatrix();
	glLoadMatrixf(GetModelViewMatrix());

	for (c = 0; c < 6; c++) {
		circleOutline(radius, radius + LIFT_OFF, prd->curveAccuracy);

		if (c % 2 == 0)
			glRotatef(-90.f, 0.f, 1.f, 0.f);
		else
			glRotatef(90.f, 1.f, 0.f, 0.f);
	}
	glPopMatrix();

	glDepthMask(GL_TRUE);
	LegacyEndAA();
}
#endif

void renderPiece(const ModelManager* modelHolder, int separateTop)
{
	const Material* mat = currentMat;
	if (separateTop)
	{
		glEnable(GL_TEXTURE_2D);
		OglModelDraw(modelHolder, MT_PIECETOP, mat);
		glDisable(GL_TEXTURE_2D);
		mat = NULL;
	}

	OglModelDraw(modelHolder, MT_PIECE, mat);
}

static void
drawSpecialPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd)
{                               /* Draw animated or dragged pieces */
	int transparentPieces = (prd->ChequerMat[0].alphaBlend) || (prd->ChequerMat[1].alphaBlend);
	if (transparentPieces) {                /* Draw back of piece separately */
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		renderSpecialPieces(modelHolder, bd, bd3d, prd);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
	}

	renderSpecialPieces(modelHolder, bd, bd3d, prd);

	if (transparentPieces)
		glDisable(GL_BLEND);
}

#if !GTK_CHECK_VERSION(3,0,0)
void MAApiece(int roundPiece, int curveAccuracy)
{
	/* Anti-alias piece edges */
	float lip = 0;
	float radius = PIECE_HOLE / 2.0f;
	float discradius = radius * 0.8f;

	if (roundPiece)
		lip = radius - discradius;

	GLboolean textureEnabled;
	glGetBooleanv(GL_TEXTURE_2D, &textureEnabled);

	GLboolean blendEnabled;
	glGetBooleanv(GL_BLEND, &blendEnabled);

	if (textureEnabled)
		glDisable(GL_TEXTURE_2D);

	LegacyStartAA(1.0f);
	glDepthMask(GL_FALSE);

	glPushMatrix();
	glLoadMatrixf(GetModelViewMatrix());
		circleOutlineOutward(radius, PIECE_DEPTH - lip, curveAccuracy);
		circleOutlineOutward(radius, lip, curveAccuracy);
	glPopMatrix();

	glDepthMask(GL_TRUE);
	LegacyEndAA();

	if (blendEnabled)
		glEnable(GL_BLEND);

	if (textureEnabled)
		glEnable(GL_TEXTURE_2D);
}
#endif

extern void
drawPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd)
{
	unsigned int i;
	unsigned int j;
	int separateTop = (prd->ChequerMat[0].pTexture && prd->pieceTextureType == PTT_TOP);

	setMaterial(&prd->ChequerMat[0]);
	for (i = 0; i < 28; i++) {
		if (bd->points[i] < 0) {
			unsigned int numPieces = (unsigned int)(-bd->points[i]);
			for (j = 1; j <= numPieces; j++) {
				drawPiece(modelHolder, bd3d, i, j, TRUE, (prd->pieceType == PT_ROUNDED), prd->curveAccuracy, separateTop);
			}
		}
	}
	setMaterial(&prd->ChequerMat[1]);
	for (i = 0; i < 28; i++) {
		if (bd->points[i] > 0) {
			unsigned int numPieces = (unsigned int)bd->points[i];
			for (j = 1; j <= numPieces; j++) {
				drawPiece(modelHolder, bd3d, i, j, TRUE, (prd->pieceType == PT_ROUNDED), prd->curveAccuracy, separateTop);
			}
		}
	}
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
drawPointLegacy(const renderdata* prd, float tuv, unsigned int i, int p, int outline)
{                               /* Draw point with correct texture co-ords */
	float w = PIECE_HOLE;
	float h = POINT_HEIGHT;
	float x, y;

	if (p) {
		x = TRAY_WIDTH - EDGE_WIDTH + (PIECE_HOLE * (float)i);
		y = -LIFT_OFF;
	}
	else {
		x = TRAY_WIDTH - EDGE_WIDTH + BOARD_WIDTH - (PIECE_HOLE * (float)i);
		y = TOTAL_HEIGHT - EDGE_HEIGHT * 2 + LIFT_OFF;
		w = -w;
		h = -h;
	}

	glPushMatrix();
	if (prd->bgInTrays)
		glTranslatef(EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH);
	else {
		x -= TRAY_WIDTH - EDGE_WIDTH;
		glTranslatef(TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH);
	}

	if (prd->roundedPoints) {   /* Draw rounded point ends */
		float xoff;

		w = w * TAKI_WIDTH;
		y += w / 2.0f;
		h -= w / 2.0f;

		if (p)
			xoff = x + (PIECE_HOLE / 2.0f);
		else
			xoff = x - (PIECE_HOLE / 2.0f);

		/* Draw rounded semi-circle end of point (with correct texture co-ords) */
		{
			unsigned int j;
			float angle, step;
			float radius = w / 2.0f;

			step = (2 * F_PI) / (float)prd->curveAccuracy;
			angle = -step * (float)(prd->curveAccuracy / 4);
			glNormal3f(0.f, 0.f, 1.f);
			glBegin(outline ? GL_LINE_STRIP : GL_TRIANGLE_FAN);
			glTexCoord2f(xoff * tuv, y * tuv);

			glVertex3f(xoff, y, 0.f);
			for (j = 0; j <= prd->curveAccuracy / 2; j++) {
				glTexCoord2f((xoff + sinf(angle) * radius) * tuv, (y + cosf(angle) * radius) * tuv);
				glVertex3f(xoff + sinf(angle) * radius, y + cosf(angle) * radius, 0.f);
				angle -= step;
			}
			glEnd();
		}
		/* Move rest of point in slighlty */
		if (p)
			x -= -((PIECE_HOLE * (1 - TAKI_WIDTH)) / 2.0f);
		else
			x -= ((PIECE_HOLE * (1 - TAKI_WIDTH)) / 2.0f);
	}

	glBegin(outline ? GL_LINE_STRIP : GL_TRIANGLES);
	glNormal3f(0.f, 0.f, 1.f);
	glTexCoord2f((x + w) * tuv, y * tuv);
	glVertex3f(x + w, y, 0.f);
	glTexCoord2f((x + w / 2) * tuv, (y + h) * tuv);
	glVertex3f(x + w / 2, y + h, 0.f);
	glTexCoord2f(x * tuv, y * tuv);
	glVertex3f(x, y, 0.f);
	glEnd();

	glPopMatrix();
}

static void MAApoints(const renderdata* prd)
{
	float tuv;

	setMaterial(&prd->PointMat[0]);

	if (prd->PointMat[0].pTexture)
		tuv = (TEXTURE_SCALE) / (float)prd->PointMat[0].pTexture->width;
	else
		tuv = 0;

	LegacyStartAA(1.0f);

	drawPointLegacy(prd, tuv, 0, 0, 1);
	drawPointLegacy(prd, tuv, 0, 1, 1);
	drawPointLegacy(prd, tuv, 2, 0, 1);
	drawPointLegacy(prd, tuv, 2, 1, 1);
	drawPointLegacy(prd, tuv, 4, 0, 1);
	drawPointLegacy(prd, tuv, 4, 1, 1);

	setMaterial(&prd->PointMat[1]);

	if (prd->PointMat[1].pTexture)
		tuv = (TEXTURE_SCALE) / (float)prd->PointMat[1].pTexture->width;
	else
		tuv = 0;

	drawPointLegacy(prd, tuv, 1, 0, 1);
	drawPointLegacy(prd, tuv, 1, 1, 1);
	drawPointLegacy(prd, tuv, 3, 0, 1);
	drawPointLegacy(prd, tuv, 3, 1, 1);
	drawPointLegacy(prd, tuv, 5, 0, 1);
	drawPointLegacy(prd, tuv, 5, 1, 1);

	LegacyEndAA();
}
#endif

void drawTable(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd)
{
	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_ALWAYS);

	drawTableBase(modelHolder, bd3d, prd);

#if !GTK_CHECK_VERSION(3,0,0)
	MAApoints(prd);
	glPushMatrix();
	glTranslatef(TOTAL_WIDTH, TOTAL_HEIGHT, 0.f);
	glRotatef(180.f, 0.f, 0.f, 1.f);
	MAApoints(prd);
	glPopMatrix();
#endif

	glDepthFunc(GL_LEQUAL);

	OglModelDraw(modelHolder, MT_TABLE, &prd->BoxMat);

	if (prd->fHinges3d)
	{
		/* Shade in gap between boards */
		glDepthFunc(GL_ALWAYS);
		OglModelDraw(modelHolder, MT_HINGEGAP, &bd3d->gapColour);
		glDepthFunc(GL_LEQUAL);

		drawHinges(modelHolder, bd3d, prd);
	}
}

static GLint 
gluUnProjectMine(GLfloat winx, GLfloat winy, GLfloat winz,
	mat4 modelMatrix,
	mat4 projmatrix,
	const GLint viewport[4],
	GLfloat* objx, GLfloat* objy, GLfloat* objz)
{
	float Fin[4];
	float Fout[4];

	mat4 finalMatrix;
	glm_mat4_mul(projmatrix, modelMatrix, finalMatrix);
	glm_mat4_inv(finalMatrix, finalMatrix);

	Fin[0] = winx;
	Fin[1] = winy;
	Fin[2] = winz;
	Fin[3] = 1.0;

	/* Map x and y from window coordinates */
	Fin[0] = (Fin[0] - (float)viewport[0]) / (float)viewport[2];
	Fin[1] = (Fin[1] - (float)viewport[1]) / (float)viewport[3];

	/* Map to range -1 to 1 */
	Fin[0] = Fin[0] * 2 - 1;
	Fin[1] = Fin[1] * 2 - 1;
	Fin[2] = Fin[2] * 2 - 1;
	glm_mat4_mulv(finalMatrix, Fin, Fout);

	if (Fout[3] == 0.0f) {
		g_assert_not_reached();
        	/* avoid -Wmaybe-uninitialized warnings in caller */
		*objx = 0.0f;
		*objy = 0.0f;
		*objz = 1.0f;
        	return(GL_FALSE);
        }

	Fout[0] /= Fout[3];
	Fout[1] /= Fout[3];
	Fout[2] /= Fout[3];
	*objx = Fout[0];
	*objy = Fout[1];
	*objz = Fout[2];
	return(GL_TRUE);
}

static void
getProjectedPos(int x, int y, float atDepth, float pos[3])
{                               /* Work out where point (x, y) projects to at depth atDepth */
	int viewport[4];
	mat4 mvmatrix, projmatrix;
	float nearX, nearY, nearZ, farX, farY, farZ, zRange;
	int ret;

	glGetIntegerv(GL_VIEWPORT, viewport);
	GetModelViewMatrixMat(mvmatrix);
	GetProjectionMatrixMat(projmatrix);

	ret = gluUnProjectMine((GLfloat)x, (GLfloat)y, 0.0, mvmatrix, projmatrix, viewport, &nearX, &nearY, &nearZ);
	g_assert(ret == GL_TRUE);	/* Should always work */
	ret = gluUnProjectMine((GLfloat)x, (GLfloat)y, 1.0, mvmatrix, projmatrix, viewport, &farX, &farY, &farZ);
	g_assert(ret == GL_TRUE);	/* Should always work */

#if defined(G_DISABLE_ASSERT)
	(void)ret;	/* silence warning about unused variable */
#endif

	zRange = (fabsf(nearZ) - atDepth) / (fabsf(farZ) + fabsf(nearZ));
	pos[0] = nearX - (-farX + nearX) * zRange;
	pos[1] = nearY - (-farY + nearY) * zRange;
	pos[2] = nearZ - (-farZ + nearZ) * zRange;
}

void
getProjectedPieceDragPos(int x, int y, float pos[3])
{
	getProjectedPos(x, y, BASE_DEPTH + EDGE_DEPTH + DOUBLECUBE_SIZE + LIFT_OFF * 3, pos);
}

void
calculateBackgroundSize(BoardData3d* bd3d, const int viewport[4])
{
	float pos1[3], pos2[3], pos3[3];

	getProjectedPos(0, viewport[1] + viewport[3], 0.f, pos1);
	getProjectedPos(viewport[2], viewport[1] + viewport[3], 0.f, pos2);
	getProjectedPos(0, viewport[1], 0.f, pos3);

	bd3d->backGroundPos[0] = pos1[0];
	bd3d->backGroundPos[1] = pos3[1];
	bd3d->backGroundSize[0] = pos2[0] - pos1[0];
	bd3d->backGroundSize[1] = pos1[1] - pos3[1];
}

void renderFlagNumbers(const BoardData3d* bd3d, int resignedValue)
{
	OGLFont flagfont;

	/* Draw number on flag */
	setMaterial(&bd3d->flagNumberMat);

	MoveToFlagMiddle();

	glDisable(GL_DEPTH_TEST);
#if !GTK_CHECK_VERSION(3,0,0)
	/* No specular light */
	float zero[4] = { 0, 0, 0, 0 };
	float specular[4];
	glGetLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
#endif
	/* Draw number */
	char flagValue[2] = " ";

	flagValue[0] = '0' + (char)abs(resignedValue);

	memcpy(&flagfont, &bd3d->cubeFont, sizeof(OGLFont));

	flagfont.scale *= 1.3f;

	glLineWidth(.5f);
	glPrintCube(&flagfont, flagValue, /*MAA*/0);

#if !GTK_CHECK_VERSION(3,0,0)
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
#endif
	glEnable(GL_DEPTH_TEST);
}
