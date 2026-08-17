/*
 * Copyright (C) 2019 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: Occluders.c,v 1.6 2021/02/21 10:56:47 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "BoardDimensions.h"
#include "gtkboard.h"

enum OcculderType { OCC_BOARD, OCC_CUBE, OCC_DICE1, OCC_DICE2, OCC_FLAG, OCC_HINGE1, OCC_HINGE2, OCC_PIECE };
#define FIRST_PIECE (int)OCC_PIECE
#define LAST_PIECE ((int)OCC_PIECE + 29)
#define NUM_OCC (LAST_PIECE + 1)

extern void initOccluders(BoardData3d* bd3d)
{
	int i;
	for (i = 0; i < NUM_OCC; i++)
		bd3d->Occluders[i].handle = NULL;
}

extern void
DrawShadows(const BoardData3d* bd3d)
{
#if !GTK_CHECK_VERSION(3,0,0)
	for (int i = 0; i < NUM_OCC; i++) {
		if (bd3d->Occluders[i].show)
			glCallList(bd3d->Occluders[i].shadow_list);
	}
#else
	(void)bd3d;	/* suppress unused parameter compiler warning */
#endif
}

extern void
UpdateShadowLightPosition(BoardData3d* bd3d, float lp[4])
{
	/* Shadow light position */
	memcpy(bd3d->shadow_light_position, lp, sizeof(float[4]));
	if (ShadowsInitilised(bd3d)) {
		int i;
		for (i = 0; i < NUM_OCC; i++)
			draw_shadow_volume_extruded_edges(&bd3d->Occluders[i], bd3d->shadow_light_position, GL_QUADS);
	}
}

extern void
TidyShadows(BoardData3d* bd3d)
{
	freeOccluder(&bd3d->Occluders[OCC_BOARD]);
	freeOccluder(&bd3d->Occluders[OCC_CUBE]);
	freeOccluder(&bd3d->Occluders[OCC_DICE1]);
	freeOccluder(&bd3d->Occluders[OCC_FLAG]);
	freeOccluder(&bd3d->Occluders[OCC_HINGE1]);
	freeOccluder(&bd3d->Occluders[OCC_PIECE]);
}

static void
updateDieOccPos(const BoardData* bd, const BoardData3d* bd3d, Occluder* pOcc, int num)
{
	float id[4][4];

	if (bd3d->shakingDice) {
		copyPoint(pOcc->trans, bd3d->diceMovingPos[num]);

		pOcc->rot[0] = bd3d->diceRotation[num].xRot;
		pOcc->rot[1] = bd3d->diceRotation[num].yRot;
		pOcc->rot[2] = bd3d->dicePos[num][2];

		makeInverseRotateMatrixZ(pOcc->invMat, pOcc->rot[2]);

		makeInverseRotateMatrixX(id, pOcc->rot[1]);
		matrixmult(pOcc->invMat, (ConstMatrix)id);

		makeInverseRotateMatrixY(id, pOcc->rot[0]);
		matrixmult(pOcc->invMat, (ConstMatrix)id);

		makeInverseTransposeMatrix(id, pOcc->trans);
		matrixmult(pOcc->invMat, (ConstMatrix)id);
	}
	else {
		getDicePos(bd, num, pOcc->trans);

		makeInverseTransposeMatrix(id, pOcc->trans);

		if (bd->diceShown == DICE_ON_BOARD) {
			pOcc->rot[0] = pOcc->rot[1] = 0;
			pOcc->rot[2] = bd3d->dicePos[num][2];

			makeInverseRotateMatrixZ(pOcc->invMat, pOcc->rot[2]);
			matrixmult(pOcc->invMat, (ConstMatrix)id);
		}
		else {
			pOcc->rot[0] = pOcc->rot[1] = pOcc->rot[2] = 0;
			copyMatrix(pOcc->invMat, id);
		}
	}
	if (ShadowsInitilised(bd3d))
		draw_shadow_volume_extruded_edges(pOcc, bd3d->shadow_light_position, GL_QUADS);
}

void
updateDiceOccPos(const BoardData* bd, BoardData3d* bd3d)
{
	int show = DiceShowing(bd);

	bd3d->Occluders[OCC_DICE1].show = bd3d->Occluders[OCC_DICE2].show = show;
	if (show) {
		updateDieOccPos(bd, bd3d, &bd3d->Occluders[OCC_DICE1], 0);
		updateDieOccPos(bd, bd3d, &bd3d->Occluders[OCC_DICE2], 1);
	}
}

NTH_STATIC void
updateCubeOccPos(const BoardData* bd, BoardData3d* bd3d)
{
	getDoubleCubePos(bd, bd3d->Occluders[OCC_CUBE].trans);
	makeInverseTransposeMatrix(bd3d->Occluders[OCC_CUBE].invMat, bd3d->Occluders[OCC_CUBE].trans);

	bd3d->Occluders[OCC_CUBE].show = (bd->cube_use && !bd->crawford_game);
	if (ShadowsInitilised(bd3d))
		draw_shadow_volume_extruded_edges(&bd3d->Occluders[OCC_CUBE], bd3d->shadow_light_position, GL_QUADS);
}

void
updateMovingPieceOccPos(const BoardData* bd, BoardData3d* bd3d)
{
	if (bd->drag_point >= 0) {
		copyPoint(bd3d->Occluders[LAST_PIECE].trans, bd3d->dragPos);
		makeInverseTransposeMatrix(bd3d->Occluders[LAST_PIECE].invMat, bd3d->Occluders[LAST_PIECE].trans);
	}
	else {                    /* if (bd3d->moving) */

		copyPoint(bd3d->Occluders[LAST_PIECE].trans, bd3d->movingPos);

		if (bd3d->rotateMovingPiece > 0) {      /* rotate shadow as piece is put in bear off tray */
			float id[4][4];

			bd3d->Occluders[LAST_PIECE].rotator = 1;
			bd3d->Occluders[LAST_PIECE].rot[1] = -90 * bd3d->rotateMovingPiece * (float) bd->turn;
			makeInverseTransposeMatrix(id, bd3d->Occluders[LAST_PIECE].trans);
			makeInverseRotateMatrixX(bd3d->Occluders[LAST_PIECE].invMat, bd3d->Occluders[LAST_PIECE].rot[1]);
			matrixmult(bd3d->Occluders[LAST_PIECE].invMat, (ConstMatrix)id);
		}
		else
			makeInverseTransposeMatrix(bd3d->Occluders[LAST_PIECE].invMat, bd3d->Occluders[LAST_PIECE].trans);
	}
	if (ShadowsInitilised(bd3d))
		draw_shadow_volume_extruded_edges(&bd3d->Occluders[LAST_PIECE], bd3d->shadow_light_position, GL_QUADS);
}

void
updatePieceOccPos(const BoardData* bd, BoardData3d* bd3d)
{
	unsigned int i, j, p = FIRST_PIECE;

	for (i = 0; i < 28; i++) {
		for (j = 1; j <= Abs(bd->points[i]); j++) {
			if (p > LAST_PIECE)
				break;          /* Found all pieces */
			getPiecePos(i, j, bd3d->Occluders[p].trans);

			if (i == 26 || i == 27) {   /* bars */
				float id[4][4];

				bd3d->Occluders[p].rotator = 1;
				if (i == 26)
					bd3d->Occluders[p].rot[1] = -90;
				else
					bd3d->Occluders[p].rot[1] = 90;
				makeInverseTransposeMatrix(id, bd3d->Occluders[p].trans);
				makeInverseRotateMatrixX(bd3d->Occluders[p].invMat, bd3d->Occluders[p].rot[1]);
				matrixmult(bd3d->Occluders[p].invMat, (ConstMatrix)id);
			}
			else {
				makeInverseTransposeMatrix(bd3d->Occluders[p].invMat, bd3d->Occluders[p].trans);
				bd3d->Occluders[p].rotator = 0;
			}
			if (ShadowsInitilised(bd3d))
				draw_shadow_volume_extruded_edges(&bd3d->Occluders[p], bd3d->shadow_light_position, GL_QUADS);

			p++;
		}
	}
	if (p == LAST_PIECE) {
		bd3d->Occluders[p].rotator = 0;
		updateMovingPieceOccPos(bd, bd3d);
	}
}

void
updateFlagOccPos(const BoardData* bd, BoardData3d* bd3d)
{
	if (bd->resigned) {
		freeOccluder(&bd3d->Occluders[OCC_FLAG]);
		initOccluder(&bd3d->Occluders[OCC_FLAG]);

		bd3d->Occluders[OCC_FLAG].show = 1;

		getFlagPos(bd->turn, bd3d->Occluders[OCC_FLAG].trans);
		makeInverseTransposeMatrix(bd3d->Occluders[OCC_FLAG].invMat, bd3d->Occluders[OCC_FLAG].trans);

		/* Flag pole */
		addCube(&bd3d->Occluders[OCC_FLAG], -FLAGPOLE_WIDTH * 1.95f, -FLAG_HEIGHT, -LIFT_OFF,
			FLAGPOLE_WIDTH * 1.95f, FLAGPOLE_HEIGHT, LIFT_OFF * 2);

		/* Flag surface */
		{
			int segment;
			for (segment = 0; segment < S_NUMSEGMENTS - 1; segment++) {
				float h = (flag.ctlpoints[segment][1][1] - flag.ctlpoints[segment][0][1]);
				float w = flag.ctlpoints[segment + 1][0][0] - flag.ctlpoints[segment][0][0];
				float s = flag.ctlpoints[segment + 1][0][2] - flag.ctlpoints[segment][0][2];
				int full = (segment == 0) ? 0 : (segment < S_NUMSEGMENTS - 2) ? 1 : 2;
				addWonkyCube(&bd3d->Occluders[OCC_FLAG], flag.ctlpoints[segment][0][0], flag.ctlpoints[segment][0][1], flag.ctlpoints[segment][0][2],
					w, h, base_unit / 10.0f, s, full);
			}
		}

		if (ShadowsInitilised(bd3d))
			draw_shadow_volume_extruded_edges(&bd3d->Occluders[OCC_FLAG], bd3d->shadow_light_position, GL_QUADS);
	}
	else {
		bd3d->Occluders[OCC_FLAG].show = 0;
	}
}

void
updateHingeOccPos(BoardData3d* bd3d, int show3dHinges)
{
	if (!ShadowsInitilised(bd3d))
		return;
	bd3d->Occluders[OCC_HINGE1].show = bd3d->Occluders[OCC_HINGE2].show = show3dHinges;
	draw_shadow_volume_extruded_edges(&bd3d->Occluders[OCC_HINGE1], bd3d->shadow_light_position, GL_QUADS);
	draw_shadow_volume_extruded_edges(&bd3d->Occluders[OCC_HINGE2], bd3d->shadow_light_position, GL_QUADS);
}

void
updateOccPos(const BoardData* bd)
{                               /* Make sure shadows are in correct place */
	if (ShadowsInitilised(bd->bd3d)) {
		updateCubeOccPos(bd, bd->bd3d);
		updateDiceOccPos(bd, bd->bd3d);
		updatePieceOccPos(bd, bd->bd3d);
	}
}

extern void
MakeShadowModel(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd)
{
	int i;
	if (!ShadowsInitilised(bd3d))
		return;
	TidyShadows(bd3d);

	initOccluder(&bd3d->Occluders[OCC_BOARD]);

	if (prd->roundedEdges) {
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], EDGE_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2 + BOARD_FILLET * 2, TRAY_HEIGHT - EDGE_HEIGHT + BOARD_FILLET * 2,
			EDGE_DEPTH - LIFT_OFF);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], EDGE_WIDTH - BOARD_FILLET,
			TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2 + BOARD_FILLET * 2, TRAY_HEIGHT - EDGE_HEIGHT + BOARD_FILLET * 2,
			EDGE_DEPTH - LIFT_OFF);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET,
			EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2 + BOARD_FILLET * 2,
			TRAY_HEIGHT - EDGE_HEIGHT + BOARD_FILLET * 2, EDGE_DEPTH - LIFT_OFF);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET,
			TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2 + BOARD_FILLET * 2, TRAY_HEIGHT - EDGE_HEIGHT + BOARD_FILLET * 2,
			EDGE_DEPTH - LIFT_OFF);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TRAY_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH,
			BOARD_WIDTH + BOARD_FILLET * 2, TOTAL_HEIGHT - EDGE_HEIGHT * 2 + BOARD_FILLET * 2,
			EDGE_DEPTH - LIFT_OFF);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET,
			EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH, BOARD_WIDTH + BOARD_FILLET * 2,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2 + BOARD_FILLET * 2, EDGE_DEPTH - LIFT_OFF);
		addSquare(&bd3d->Occluders[OCC_BOARD], BOARD_FILLET, BOARD_FILLET, 0.f, TOTAL_WIDTH - BOARD_FILLET * 2,
			TOTAL_HEIGHT - BOARD_FILLET * 2, BASE_DEPTH + EDGE_DEPTH - LIFT_OFF);
	}
	else {
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2,
			TRAY_HEIGHT - EDGE_HEIGHT, EDGE_DEPTH);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], EDGE_WIDTH, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, EDGE_DEPTH);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, EDGE_DEPTH);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH,
			TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2,
			TRAY_HEIGHT - EDGE_HEIGHT, EDGE_DEPTH);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH, BOARD_WIDTH,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, EDGE_DEPTH);
		addClosedSquare(&bd3d->Occluders[OCC_BOARD], TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH, EDGE_HEIGHT, BASE_DEPTH,
			BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT * 2, EDGE_DEPTH);
		addSquare(&bd3d->Occluders[OCC_BOARD], 0.f, 0.f, 0.f, TOTAL_WIDTH, TOTAL_HEIGHT, BASE_DEPTH + EDGE_DEPTH);
	}
	setIdMatrix(bd3d->Occluders[OCC_BOARD].invMat);
	bd3d->Occluders[OCC_BOARD].trans[0] = bd3d->Occluders[OCC_BOARD].trans[1] = bd3d->Occluders[OCC_BOARD].trans[2] = 0;
	draw_shadow_volume_extruded_edges(&bd3d->Occluders[OCC_BOARD], bd3d->shadow_light_position, GL_QUADS);

	initOccluder(&bd3d->Occluders[OCC_HINGE1]);
	copyOccluder(&bd3d->Occluders[OCC_HINGE1], &bd3d->Occluders[OCC_HINGE2]);

	addHalfTube(&bd3d->Occluders[OCC_HINGE1], HINGE_WIDTH, HINGE_HEIGHT, prd->curveAccuracy / 2);

	bd3d->Occluders[OCC_HINGE1].trans[0] = bd3d->Occluders[OCC_HINGE2].trans[0] = (TOTAL_WIDTH) / 2.0f;
	bd3d->Occluders[OCC_HINGE1].trans[2] = bd3d->Occluders[OCC_HINGE2].trans[2] = BASE_DEPTH + EDGE_DEPTH;
	bd3d->Occluders[OCC_HINGE1].trans[1] = ((TOTAL_HEIGHT / 2.0f) - HINGE_HEIGHT) / 2.0f;
	bd3d->Occluders[OCC_HINGE2].trans[1] = ((TOTAL_HEIGHT / 2.0f) - HINGE_HEIGHT + TOTAL_HEIGHT) / 2.0f;

	makeInverseTransposeMatrix(bd3d->Occluders[OCC_HINGE1].invMat, bd3d->Occluders[OCC_HINGE1].trans);
	makeInverseTransposeMatrix(bd3d->Occluders[OCC_HINGE2].invMat, bd3d->Occluders[OCC_HINGE2].trans);

	updateHingeOccPos(bd3d, prd->fHinges3d);

	initOccluder(&bd3d->Occluders[OCC_CUBE]);
	addSquareCentered(&bd3d->Occluders[OCC_CUBE], 0.f, 0.f, 0.f, DOUBLECUBE_SIZE * .88f, DOUBLECUBE_SIZE * .88f,
		DOUBLECUBE_SIZE * .88f);

	updateCubeOccPos(bd, bd3d);

	initOccluder(&bd3d->Occluders[OCC_DICE1]);
	addDice(&bd3d->Occluders[OCC_DICE1], getDiceSize(prd) / 2.0f);
	copyOccluder(&bd3d->Occluders[OCC_DICE1], &bd3d->Occluders[OCC_DICE2]);
	bd3d->Occluders[OCC_DICE1].rotator = bd3d->Occluders[OCC_DICE2].rotator = 1;
	updateDiceOccPos(bd, bd3d);

	initOccluder(&bd3d->Occluders[OCC_PIECE]);
	{
		float radius = PIECE_HOLE / 2.0f;
		float discradius = radius * 0.8f;
		float lip = radius - discradius;
		float height = PIECE_DEPTH - 2 * lip;

		addCylinder(&bd3d->Occluders[OCC_PIECE], 0.f, 0.f, lip, PIECE_HOLE / 2.0f - LIFT_OFF, height,
			prd->curveAccuracy);
	}
	for (i = FIRST_PIECE; i <= LAST_PIECE; i++) {
		bd3d->Occluders[i].rot[0] = 0;
		bd3d->Occluders[i].rot[2] = 0;
		if (i != FIRST_PIECE)
			copyOccluder(&bd3d->Occluders[OCC_PIECE], &bd3d->Occluders[i]);
	}

	updatePieceOccPos(bd, bd3d);
	updateFlagOccPos(bd, bd3d);
}
