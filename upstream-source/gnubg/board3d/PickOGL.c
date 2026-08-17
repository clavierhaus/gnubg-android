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
 * $Id: PickOGL.c,v 1.16 2021/10/30 13:56:08 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "BoardDimensions.h"
#include "boardpos.h"
#include "gtkboard.h"

typedef void (*PickDrawFun) (const BoardData* bd, void* data);

#if !GTK_CHECK_VERSION(3,0,0)
static void SetPickObject(int value)
{
	glLoadName(value);
}
#else
static void SetPickObject(int value)
{
	SetPickColour(((value + 1) & 0xFF) / (float)0xFF);
}
#endif

#ifndef TEST_HARNESS
static
#endif
void drawPickObjects(const BoardData* bd, void* UNUSED(data))
{
	BoardData3d* bd3d = bd->bd3d;
	renderdata* prd = bd->rd;

	if (bd->resigned) {         /* Flag showing - just pick this */
		waveFlag(bd3d->flagWaved);

		SetPickObject(POINT_RESIGN);

		const ModelManager* modelHolder = &bd3d->modelHolder;
		renderFlag(modelHolder, bd3d, prd->curveAccuracy, bd->turn, bd->resigned);
	}
	else
	{
		unsigned int i, j;
		/* pieces */
		int separateTop = (bd->rd->ChequerMat[0].pTexture && bd->rd->pieceTextureType == PTT_TOP);
		const ModelManager* modelHolder = &bd3d->modelHolder;

		for (i = 0; i < 28; i++) {
			SetPickObject(i);
			for (j = 1; j <= Abs(bd->points[i]); j++)
				drawPiece(modelHolder, bd3d, i, j, TRUE, (bd->rd->pieceType == PT_ROUNDED), bd->rd->curveAccuracy, separateTop);
		}

		/* Dice */
		if (DiceShowing(bd)) {
			SetPickObject(POINT_DICE);
			drawDie(&bd3d->modelHolder, bd, bd3d, prd, GetCurrentMaterial(), 0, FALSE);
			drawDie(&bd3d->modelHolder, bd, bd3d, prd, GetCurrentMaterial(), 1, FALSE);
		}

		/* Double Cube */
		SetPickObject(POINT_CUBE);
		drawDC(&bd3d->modelHolder, bd, bd3d, GetCurrentMaterial(), 0);
	}
}

NTH_STATIC void
drawPickAreas(const BoardData* bd, void* UNUSED(data))
{                               /* Draw main board areas to see where user clicked */
	BoardData3d* bd3d = bd->bd3d;
	float barHeight;

	/* boards */
	SetPickObject(fClockwise ? 2 : 1);
	renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE * 6, POINT_HEIGHT);

	SetPickObject(fClockwise ? 1 : 2);
	renderRect(&bd3d->modelHolder, TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE * 6, POINT_HEIGHT);

	SetPickObject(fClockwise ? 4 : 3);
	renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH, -PIECE_HOLE * 6, -POINT_HEIGHT);

	SetPickObject(fClockwise ? 3 : 4);
	renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH, -PIECE_HOLE * 6, -POINT_HEIGHT);

	/* bars + homes */
	barHeight = (PIECE_HOLE + PIECE_GAP_HEIGHT) * 4;
	SetPickObject(0);
	renderRect(&bd3d->modelHolder, TOTAL_WIDTH / 2.0f - PIECE_HOLE / 2.0f,
		TOTAL_HEIGHT / 2.0f - (DOUBLECUBE_SIZE / 2.0f + barHeight + PIECE_HOLE / 2.0f), BASE_DEPTH + EDGE_DEPTH,
		PIECE_HOLE, barHeight);
	SetPickObject(25);
	renderRect(&bd3d->modelHolder, TOTAL_WIDTH / 2.0f - PIECE_HOLE / 2.0f, TOTAL_HEIGHT / 2.0f + (DOUBLECUBE_SIZE / 2.0f + PIECE_HOLE / 2.0f),
		BASE_DEPTH + EDGE_DEPTH, PIECE_HOLE, barHeight);

	/* Home/unused trays depend on direction */
	SetPickObject((!fClockwise) ? 26 : POINT_UNUSED0);
	renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2,
		TRAY_HEIGHT - EDGE_HEIGHT);
	SetPickObject((fClockwise) ? 26 : POINT_UNUSED0);
	renderRect(&bd3d->modelHolder, EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT);

	SetPickObject((!fClockwise) ? 27 : POINT_UNUSED1);
	renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT - POINT_HEIGHT, BASE_DEPTH,
		TRAY_WIDTH - EDGE_WIDTH * 2, POINT_HEIGHT);
	SetPickObject((fClockwise) ? 27 : POINT_UNUSED1);
	renderRect(&bd3d->modelHolder, EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT - POINT_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2,
		POINT_HEIGHT);

	/* Dice areas */
	SetPickObject(POINT_LEFT);
	renderRect(&bd3d->modelHolder, TRAY_WIDTH, (TOTAL_HEIGHT - DICE_AREA_HEIGHT) / 2.0f, BASE_DEPTH, DICE_AREA_CLICK_WIDTH, DICE_AREA_HEIGHT);
	SetPickObject(POINT_RIGHT);
	renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, (TOTAL_HEIGHT - DICE_AREA_HEIGHT) / 2.0f, BASE_DEPTH,
		DICE_AREA_CLICK_WIDTH, DICE_AREA_HEIGHT);
}

NTH_STATIC void
drawPickBoard(const BoardData* bd, void* data)
{                               /* Draw points and piece objects for selected board */
	BoardData3d* bd3d = bd->bd3d;
	int selectedBoard = GPOINTER_TO_INT(data);
	unsigned int i;

	/* points */
	if (fClockwise) {
		const int SwapBoard[4] = { 2, 1, 4, 3 };
		selectedBoard = SwapBoard[selectedBoard - 1];
	}
	for (i = 0; i < 6; i++) {
		switch (selectedBoard) {
		case 1:
			SetPickObject(fClockwise ? i + 7 : 6 - i);
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH + PIECE_HOLE * (float)i, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE,
				POINT_HEIGHT);
			break;
		case 2:
			SetPickObject(fClockwise ? i + 1 : 12 - i);
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + PIECE_HOLE * (float)i, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE, POINT_HEIGHT);
			break;
		case 3:
			SetPickObject(fClockwise ? i + 19 : 18 - i);
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH - (PIECE_HOLE * (float)i), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH, -PIECE_HOLE,
				-POINT_HEIGHT);
			break;
		case 4:
			SetPickObject(fClockwise ? i + 13 : 24 - i);
			renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH - (PIECE_HOLE * (float)i), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH, -PIECE_HOLE,
				-POINT_HEIGHT);
			break;
		}
	}
}

#ifdef TEST_HARNESS
static void
DrawPickIndividualPoint(const BoardData* bd, int point)
{
	BoardData3d* bd3d = bd->bd3d;
	unsigned int j;
	SetPickObject(point);
	for (j = 1; j <= Abs(bd->points[point]); j++)
		drawPiece(&bd3d->modelHolder, bd3d, point, j, FALSE, (bd->rd->pieceType == PT_ROUNDED), bd->rd->curveAccuracy, FALSE);

	if (fClockwise) {
		point--;
		if (point < 6)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + PIECE_HOLE * point, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE, POINT_HEIGHT);
		else if (point < 12)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH + PIECE_HOLE * (point - 6), EDGE_HEIGHT, BASE_DEPTH,
				PIECE_HOLE, POINT_HEIGHT);
		else if (point < 18)
			renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH - (PIECE_HOLE * (point - 12)), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH,
				-PIECE_HOLE, -POINT_HEIGHT);
		else if (point < 24)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH - (PIECE_HOLE * (point - 18)), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH,
				-PIECE_HOLE, -POINT_HEIGHT);
	}
	else {
		if (point <= 6)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH + PIECE_HOLE * (6 - point), EDGE_HEIGHT, BASE_DEPTH,
				PIECE_HOLE, POINT_HEIGHT);
		else if (point <= 12)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + PIECE_HOLE * (12 - point), EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE, POINT_HEIGHT);
		else if (point <= 18)
			renderRect(&bd3d->modelHolder, TRAY_WIDTH + BOARD_WIDTH - (PIECE_HOLE * (18 - point)), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH,
				-PIECE_HOLE, -POINT_HEIGHT);
		else if (point <= 24)
			renderRect(&bd3d->modelHolder, TOTAL_WIDTH - TRAY_WIDTH - (PIECE_HOLE * (24 - point)), TOTAL_HEIGHT - EDGE_HEIGHT, BASE_DEPTH,
				-PIECE_HOLE, -POINT_HEIGHT);
	}
}

NTH_STATIC void
drawPickPointCheck(const BoardData* bd, void* data)
{                               /* Draw two points to see which is selected */
	int pointA = (GPOINTER_TO_INT(data)) / 100;
	int pointB = (GPOINTER_TO_INT(data)) - pointA * 100;

	/* pieces on both points */
	DrawPickIndividualPoint(bd, pointA);
	DrawPickIndividualPoint(bd, pointB);
}
#endif

#if !GTK_CHECK_VERSION(3,0,0)
/* 20 allows for 5 hit records (more than enough) */
#define BUFSIZE 20
static unsigned int selectBuf[BUFSIZE];

static int
NearestHit(int hits, const unsigned int* ptr)
{
	if (hits == 1) {            /* Only one hit - just return this one */
		return (int)ptr[3];
	}
	else {                    /* Find the highest/closest object */
		int i, sel = -1;
		float minDepth = FLT_MAX;

		for (i = 0; i < hits; i++) {    /* for each hit */
			unsigned int names;
			float depth;

			names = *ptr++;

/*
 * Used to be (float)*ptr++ / 0x7fffffff
 *
 * This may be a bit pedantic but avoids clang builds getting
 * "implicit conversion from 'int' to 'float' changes value
 *  from 2147483647 to 2147483648" warning
 * with default-ish "-Wall -Wextra" CFLAGS
 */
			depth = (float)*ptr++ / 2147483648.f;
                        

			ptr++;              /* Skip max depth value */
			/* Ignore clicks on the board base as other objects must be closer */
			if (/* *ptr >= POINT_DICE && !(*ptr == POINT_LEFT || *ptr == POINT_RIGHT) && */ depth < minDepth) {
				minDepth = depth;
				sel = (int)*ptr;
			}
			ptr += names;
		}
		return sel;
	}
}

static int
PickDraw(int x, int y, PickDrawFun drawFun, const BoardData* bd, void* data)
{                               /* Identify if anything is below point (x,y) */
	int hits;
	int viewport[4];
	float *projMat, *modelMat;

	glSelectBuffer(BUFSIZE, selectBuf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	glGetIntegerv(GL_VIEWPORT, viewport);

	getPickMatrices((float)x, (float)y, bd, viewport, &projMat, &modelMat);

	/* Setup openGL legacy matrices */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(projMat);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelMat);

	drawFun(bd, data);

	glPopName();

	hits = glRenderMode(GL_RENDER);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	return NearestHit(hits, (unsigned int*)selectBuf);
}
#else

static int
PickDraw(int x, int y, PickDrawFun drawFun, const BoardData* bd, void* data)
{                               /* Identify if anything is below point (x,y) */
	SelectPickProgram();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawFun(bd, data);

	// This doesn't seem necessary...
	//	glFinish();
	//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	unsigned char pixelValue[3];
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixelValue);
	return pixelValue[0] - 1;
}

#endif

static void
drawPickPoint(const BoardData* bd, void* data)
{                               /* Draw sub parts of point to work out which part of point clicked */
	unsigned int point = GPOINTER_TO_UINT(data);

	if (point <= 25) {          /* Split first point into 2 parts for zero and one selection */
		BoardData3d* bd3d = bd->bd3d;

#define SPLIT_PERCENT .40f
#define SPLIT_HEIGHT (PIECE_HOLE * SPLIT_PERCENT)
		unsigned int i;
		float pos[3];

		getPiecePos(point, 1, pos);

		SetPickObject(0);
		if (point > 12)
			renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1] + (PIECE_HOLE / 2.0f) + PIECE_GAP_HEIGHT - SPLIT_HEIGHT,
				pos[2], PIECE_HOLE, SPLIT_HEIGHT);
		else
			renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1] - (PIECE_HOLE / 2.0f), pos[2], PIECE_HOLE, SPLIT_HEIGHT);

		SetPickObject(1);
		if (point > 12)
			renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1] - (PIECE_HOLE / 2.0f), pos[2], PIECE_HOLE,
				PIECE_HOLE + PIECE_GAP_HEIGHT - SPLIT_HEIGHT);
		else
			renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1] - (PIECE_HOLE / 2.0f) + SPLIT_HEIGHT, pos[2], PIECE_HOLE,
				PIECE_HOLE + PIECE_GAP_HEIGHT - SPLIT_HEIGHT);

		for (i = 2; i < 6; i++) {
			/* Only 3 places for bar */
			if ((point == 0 || point == 25) && i == 4)
				break;
			getPiecePos(point, i, pos);
			SetPickObject(i);
			renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1] - (PIECE_HOLE / 2.0f), pos[2], PIECE_HOLE,
				PIECE_HOLE + PIECE_GAP_HEIGHT);
		}
		/* extra bit */
		/*              SetPickObject(i);
		 * if (point > 12)
		 * {
		 * pos[1] = pos[1] - (PIECE_HOLE / 2.0f);
		 * renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), TOTAL_HEIGHT - EDGE_HEIGHT - POINT_HEIGHT, pos[2], PIECE_HOLE, pos[1] - (TOTAL_HEIGHT - EDGE_HEIGHT - POINT_HEIGHT));
		 * }
		 * else
		 * {
		 * pos[1] = pos[1] + (PIECE_HOLE / 2.0f) + PIECE_GAP_HEIGHT;
		 * renderRect(&bd3d->modelHolder, pos[0] - (PIECE_HOLE / 2.0f), pos[1], pos[2], PIECE_HOLE, EDGE_HEIGHT + POINT_HEIGHT - pos[1]);
		 * }
		 */
	}
}

int
BoardSubPoint3d(const BoardData* bd, int x, int y, unsigned int point)
{
	return PickDraw(x, y, drawPickPoint, bd, GUINT_TO_POINTER(point));
}

int
BoardPoint3d(const BoardData* bd, int x, int y)
{
	MakeCurrent3d(bd->bd3d);

	int picked = PickDraw(x, y, drawPickObjects, bd, NULL);
	if (picked != -1)
		return picked;	/* Object clicked */

	/* Next check for areas on the board - 2 dice areas, 4 boards with 6 lots of 6 places */

	picked = PickDraw(x, y, drawPickAreas, bd, NULL);
	if (picked <= 0 || picked > 24)
		return picked;      /* Not a point so actual hit */
	g_assert(picked >= 1 && picked <= 4);   /* one of 4 boards */

	/* Work out which point in board was clicked */
	return PickDraw(x, y, drawPickBoard, bd, GINT_TO_POINTER(picked));
}
