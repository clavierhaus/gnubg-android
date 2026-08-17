/*
 * Copyright (C) 2019-2020 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2019-2021 the AUTHORS
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
 * $Id: inc3d.h,v 1.7 2021/09/18 22:13:14 plm Exp $
 */

#ifndef INC3D3D_H
#define INC3D3D_H

#include <glib.h>
#include <gtk/gtk.h>

#include "analysis.h"	/* for statcontext */
#include "gtkboard.h"	/* for BoardData */ 
#include "render.h"	/* for renderdata */

#include "board3d/types3d.h"

#define DF_VARIABLE_OPACITY 1
#define DF_NO_ALPHA 2
#define DF_FULL_ALPHA 4

extern void StopIdle3d(const BoardData* bd, BoardData3d* bd3d);
extern void Tidy3dObjects(BoardData3d* bd3d, const renderdata* prd);
extern int setVSync(int state);
extern char* MaterialGetTextureFilename(const Material* pMat);
extern void FindTexture(TextureInfo** textureInfo, const char* file);
GtkWidget* GetDrawingArea3d(const BoardData3d* bd3d);
extern void updateDiceOccPos(const BoardData* bd, BoardData3d* bd3d);
extern void updatePieceOccPos(const BoardData* bd, BoardData3d* bd3d);
extern void updateHingeOccPos(BoardData3d* bd3d, int show3dHinges);
extern void updateFlagOccPos(const BoardData* bd, BoardData3d* bd3d);
extern void SetMovingPieceRotation(const BoardData* bd, BoardData3d* bd3d, unsigned int pt);
extern int MouseMove3d(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd, int x, int y);
extern void PlaceMovingPieceRotation(const BoardData* bd, BoardData3d* bd3d, unsigned int dest, unsigned int src);
extern void DrawScene3d(const BoardData3d* bd3d);
extern int Animating3d(const BoardData3d* bd3d);
extern void RollDice3d(BoardData* bd, BoardData3d* bd3d, const renderdata* prd);
extern void setDicePos(BoardData* bd, BoardData3d* bd3d);
extern int BoardPoint3d(const BoardData* bd, int x, int y);
extern int BoardSubPoint3d(const BoardData* bd, int x, int y, unsigned int point);
extern void RecalcViewingVolume(const BoardData* bd);
extern void ShowFlag3d(BoardData* bd, BoardData3d* bd3d, const renderdata* prd);
extern void AnimateMove3d(BoardData* bd, BoardData3d* bd3d);
extern void UpdateShadows(BoardData3d* bd3d);
extern int CreateGLWidget(BoardData* bd, int useMouseEvents);
extern void InitColourSelectionDialog(void);
extern void MakeCurrent3d(const BoardData3d* bd3d);
extern void preDraw3d(const BoardData* bd, BoardData3d* bd3d, renderdata* prd);
extern gboolean InitGTK3d(int* argc, char*** argv);
extern void DisplayCorrectBoardType(BoardData* bd, BoardData3d* bd3d, renderdata* prd);
/* Graph functions */
GtkWidget* StatGraph(GraphData* pgd);
void SetNumGames(GraphData* pgd, unsigned int numGames);
void AddGameData(GraphData* pgd, int game, const statcontext* psc);
void TidyGraphData(GraphData* pgd);
GraphData* CreateGraphData(void);

extern void ResetPreviews(void);
extern void UpdateColPreviews(void);
extern int GetPreviewId(void);
extern void UpdateColPreview(int ID);
extern void SetPreviewLightLevel(const int levels[3]);
extern int MaterialCompare(Material* pMat1, Material* pMat2);
extern void GetTextures(BoardData3d* bd3d, renderdata* prd);
extern void ClearTextures(BoardData3d* bd3d);
extern int DiceTooClose(const BoardData3d* bd3d, const renderdata* prd);
extern GtkWidget* gtk_colour_picker_new3d(Material* pMat, int opacity, TextureType textureType);
extern float TestPerformance3d(BoardData* bd);
extern void Set3dSettings(renderdata* prdnew, const renderdata* prd);

#endif
