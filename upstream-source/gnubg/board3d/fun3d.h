/*
 * Copyright (C) 2006-2021 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2007-2021 the AUTHORS
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
 */

#ifndef FUN3D_H
#define FUN3D_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <glib.h>

#include <cglm/vec3.h>
#include "common.h"
#include "inc3d.h"
#include "matrix.h"
#include "gtkwindows.h"

#include "types3d.h"

#include "model.h"

extern int fClockwise;          /* Player 1 moves clockwise */

extern const Material* currentMat;

void InitMatStacks(void);

void OglModelInit(ModelManager* modelHolder, int modelNumber);
void OglModelAlloc(ModelManager* modelHolder, int modelNumber);
void OglModelDraw(const ModelManager* modelManager, int modelNumber, const Material* pMat);

void ModelManagerInit(ModelManager* modelHolder);
void ModelManagerStart(ModelManager* modelHolder);
void ModelManagerCreate(ModelManager* modelHolder);
void ModelManagerCopyModelToBuffer(ModelManager* modelHolder, int modelNumber);

#define VERTEX_STRIDE 8

/* TODO: Tidy this up... (i.e. remove tricky macro) */
extern OglModel* curModel;
#define CALL_OGL(modelManager, modelNumber, fun, ...) \
{ \
	curModel = modelManager.models[modelNumber];\
	OglModelInit(modelManager, modelNumber);\
	fun(__VA_ARGS__);\
	OglModelAlloc(modelManager, modelNumber);\
	fun(__VA_ARGS__); \
}
#define UPDATE_OGL(modelManager, modelNumber, fun, ...) \
{ \
    curModel = modelManager.models[modelNumber]; \
    curModel->data = g_malloc(sizeof(float) * curModel->dataLength); \
    curModel->dataLength = 0; \
	fun(__VA_ARGS__);\
    ModelManagerCopyModelToBuffer((ModelManager*)modelManager, modelNumber); \
}

typedef struct {
	double data[3];
} Point3d;

#if defined(WIN32)
/* Need to set the callback calling convention for windows */
#define TESS_CALLBACK __stdcall
#else
#define TESS_CALLBACK
#endif

typedef struct {
	GArray* conPoints;
} Contour;

typedef struct {
	GArray* contours;
	unsigned int numPoints;
} Vectoriser;

typedef struct {
	GArray* tessPoints;
	GLenum meshType;
} Tesselation;

typedef struct {
	GArray* tesselations;
} Mesh;

#define TT_COUNT 3              /* 3 texture types: general, piece and hinge */

typedef int idleFunc(BoardData3d* bd3d);

extern void freeEigthPoints(EigthPoints* eigthPoints);
void calculateEigthPoints(EigthPoints* eigthPoints, float radius, unsigned int accuracy);

/* Used to calculate correct viewarea for board/fov angles */
typedef struct {
	float top;
	float bottom;
	float width;
} viewArea;

typedef struct {
#define S_NUMSEGMENTS 11
    /* Control points for the flag. The Z values are modified to make it wave */
    vec3 ctlpoints[S_NUMSEGMENTS][2];
} Flag3d;

/* Font info */

#define FONT_PITCH 24
#define FONT_SIZE (base_unit / 20.0f)
#define FONT_HEIGHT_RATIO 1.f

#define CUBE_FONT_PITCH 34
#define CUBE_FONT_SIZE (base_unit / 24.0f)
#define CUBE_FONT_HEIGHT_RATIO 1.25f

#define FONT_VERA "fonts/Vera.ttf"
#define FONT_VERA_SERIF_BOLD "fonts/VeraSeBd.ttf"

/* Setup functions */
void InitGL(const BoardData* bd);

/* Drawing functions */
void drawBoard(const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);
extern void Draw3d(const BoardData* bd);
float getBoardWidth(void);
float getBoardHeight(void);
void calculateBackgroundSize(BoardData3d* bd3d, const int viewport[4]);

extern void getPiecePos(unsigned int point, unsigned int pos, float v[3]);

/* Misc functions */
void SetupSimpleMat(Material* pMat, float r, float g, float b);
void SetupMat(Material* pMat, float r, float g, float b, float dr, float dg, float db, float sr, float sg, float sb,
	int shin, float a);
void setMaterial(const Material* pMat);
void SetPickColour(float colour);
void setMaterialReset(const Material* pMat);
void SetColour3d(float r, float g, float b, float a);
float randRange(float range);
void setupPath(const BoardData* bd, Path* p, float* pRotate, unsigned int fromPoint, unsigned int fromDepth,
	unsigned int toPoint, unsigned int toDepth);
int finishedPath(const Path* p);
void getProjectedPieceDragPos(int x, int y, float pos[3]);
void updateMovingPieceOccPos(const BoardData* bd, BoardData3d* bd3d);
void LoadTextureInfo(void);
GList* GetTextureList(TextureType type);
extern void FindNamedTexture(TextureInfo** textureInfo, char* name);
float Dist2d(float a, float b);
float*** Alloc3d(unsigned int x, unsigned int y, unsigned int z);
void Free3d(float*** array, unsigned int x, unsigned int y);
int LoadTexture(Texture* texture, const char* filename);
void CheckOpenglError(void);

/* Functions for 3d board */
extern void InitBoard3d(BoardData* bd, BoardData3d* bd3d);
extern void SetupVisual(void);

typedef struct {
	unsigned int width;
	unsigned int height;
	BoardData* bd;
	unsigned char* puch;
} RenderToBufferData;

gboolean RenderToBuffer3d(GtkWidget* widget, GdkEventExpose* eventData, void* data);
void DeleteTextureList(void);

void updateOccPos(const BoardData* bd);

int ShadowsInitilised(const BoardData3d* bd3d);
void shadowInit(BoardData3d* bd3d, renderdata* prd);
void shadowDisplay(const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);

/* font-related global variables */
extern GList* combineList;

/* font functions */
int CreateFonts(BoardData3d* bd3d);
int CreateFontText(OGLFont* ppFont, char* text, const char* fontFile, int pitch, float size, float heightRatio);
int CreateOGLFont(FT_Library ftLib, OGLFont* pFont, const char* pPath, int pointSize, float size, float heightRatio);
float GetFontHeight3d(const OGLFont* font);
void PopulateVectoriser(Vectoriser* pVect, const FT_Outline* pOutline);
void TidyMemory(const Vectoriser* pVect, const Mesh* pMesh);
void DrawNumbers(const OGLFont* numberFont, unsigned int sides, int swapNumbers, int MAA);
void TESS_CALLBACK tcbError(GLenum error);
void TESS_CALLBACK tcbVertex(void* data, Mesh* UNUSED(pMesh));
void TESS_CALLBACK tcbCombine(const double coords[3], const double* UNUSED(vertex_data[4]), float UNUSED(weight[4]), void** outData);
void TESS_CALLBACK tcbBegin(GLenum type, Mesh* UNUSED(pMesh));
void TESS_CALLBACK tcbEnd( /*lint -e{818} */ Mesh* pMesh);
void glPrintPointNumbers(const OGLFont* numberFont, const char* text, int MAA);
void glPrintCube(const OGLFont* cubeFont, const char* text, int MAA);
void glPrintNumbersRA(const OGLFont* numberFont, const char* text);
void glDrawText(const OGLFont* font);
float getTextLen3d(const OGLFont* pFont, const char* str);
FT_Pos GetKern(const OGLFont* pFont, char cur, char next);
void FreeNumberFont(OGLFont* ppFont);
void FreeTextFont(OGLFont* ppFont);

extern int extensionSupported(const char* extension);

void RenderString3d(const OGLFont* pFont, const char* str, float scale, int MAA);
extern void drawTable(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd);
extern void getMoveIndicatorPos(int turn, float pos[3]);
extern void drawMoveIndicator(void);
extern void drawFlagPole(unsigned int curveAccuracy);
extern void drawPoint(const renderdata* prd, float tuv, unsigned int i, int p, int outline);
extern void drawBackground(const renderdata* prd, const float* bd3dbackGroundPos, const float* bd3dbackGroundSize);
extern int CreateNumberFont(OGLFont* ppFont, const char* fontFile, int pitch, float size, float heightRatio);
extern void drawDie(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd, const Material* diceMat, int num, int drawDots);
extern void drawDC(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const Material* cubeMat, int drawNumbers);
extern const Material* GetCurrentMaterial(void);
extern void renderRect(const ModelManager* modelHolder, float x, float y, float z, float w, float h);

/* Clipping planes */
#define zNearVAL .1f
#define zFarVAL 70.0f

extern Flag3d flag;

/* Define relative sizes of objects from arbitrary unit .05 */
#define base_unit .05f

/* Piece/point size */
#define PIECE_HOLE (base_unit * 3.0f)
#define PIECE_DEPTH base_unit

/* Scale textures by this amount */
#define TEXTURE_SCALE (10.0f / base_unit)

#define copyPoint(to, from) memcpy(to, from, sizeof(float[3]))

#define TEXTURE_PATH "textures/"
#define NO_TEXTURE_STRING _("No texture")

#define HINGE_SEGMENTS 6.f

#define CUBE_TWODIGIT_FACTOR .9f
#define CUBE_THREEDIGIT_FACTOR .6f
#define CUBE_FOURDIGIT_FACTOR .45f

/* Draw board parts (boxes) specially */
enum { /*boxType */ BOX_ALL = 0, BOX_NOSIDES = 1, BOX_NOENDS = 2, BOX_SPLITTOP = 4, BOX_SPLITWIDTH = 8 };

/* functions used in test harness (static in gnubg) */
#ifndef TEST_HARNESS
#define NTH_STATIC static
#else
#define NTH_STATIC extern
extern void TestHarnessDraw(const BoardData * bd);
#endif

/* Some 3d functions */
extern float getDiceSize(const renderdata * prd);
extern void SetupFlag(void);
extern void setupDicePaths(const BoardData * bd, Path dicePaths[2], float diceMovingPos[2][3], DiceRotation diceRotation[2]);
extern void waveFlag(float wag);

/* Other functions */
void initPath(Path * p, const float start[3]);
void addPathSegment(Path * p, PathType type, const float point[3]);
void initDT(diceTest * dt, int x, int y, int z);

typedef void (*RealizeCB)(void*);
typedef void (*ConfigureCB)(GtkWidget*, void*);
typedef gboolean (*ExposeCB)(GtkWidget*, GdkEventExpose*, void*);
extern GtkWidget* GLWidgetCreate(RealizeCB realizeCB, ConfigureCB configureCB, ExposeCB exposeCB, void* data);
extern void GLWidgetMakeCurrent(GtkWidget* widget);
extern void SelectPickProgram(void);
extern void SetLightPos(float* lp);
extern void SetViewPos(void);
gboolean GLWidgetRender(GtkWidget* widget, ExposeCB exposeCB, GdkEventExpose* eventDetails, void* data);
gboolean GLInit(int* argc, char*** argv);
extern void SetLineDrawingmode(int enable);

// Functions required for 3d test harness
void SetupSimpleMatAlpha(Material* pMat, float r, float g, float b, float a);
int movePath(Path* p, float d, float* rotate, float v[3]);

extern void drawDots(const ModelManager* modelHolder, const BoardData3d* bd3d, float diceSize, float dotOffset, const diceTest* dt, int showFront);
extern void DrawDots(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd, diceTest* dt, int diceCol);

extern void drawTableGrayed(const ModelManager* modelHolder, const BoardData3d* bd3d, renderdata tmp);
extern int DiceShowing(const BoardData* bd);
extern void getFlagPos(int turn, float v[3]);
extern void getDicePos(const BoardData* bd, int num, float v[3]);
extern float getViewAreaHeight(const viewArea* pva);
extern float getAreaRatio(const viewArea* pva);
extern void WorkOutViewArea(const BoardData* bd, viewArea* pva, float* pHalfRadianFOV, float aspectRatio);
extern void getDoubleCubePos(const BoardData* bd, float v[3]);
extern void TidyShadows(BoardData3d* bd3d);
extern void MakeShadowModel(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd);
extern void initOccluders(BoardData3d* bd3d);
extern void DrawShadows(const BoardData3d* bd3d);
extern void UpdateShadowLightPosition(BoardData3d* bd3d, float lp[4]);
extern void getPickMatrices(float x, float y, const BoardData* bd, int* viewport, float** projMat, float** modelMat);
extern void drawTableBase(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd);
extern void drawHinges(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd);
extern int RenderGlyph(const FT_Outline* pOutline);
extern void glSetViewport(int viewport[4]);
extern void SetupViewingVolume3d(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd, int viewport[4]);
extern void SetupViewingVolume3dNew(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd, float** projMat, float** modelMat, int viewport[4]);
extern void ClearScreen(const renderdata* prd);
extern float* GetModelViewMatrix(void);
extern float* GetProjectionMatrix(void);
extern void GetTextureMatrix(mat3 *textureMat);
extern void GetModelViewMatrixMat(mat4 ret);
extern void GetProjectionMatrixMat(mat4 ret);
extern void LegacyStartAA(float width);
extern void LegacyEndAA(void);
extern void ShowMoveIndicator(const ModelManager* modelHolder, const BoardData* bd);
extern void MAAmoveIndicator(void);
extern void drawPiece(const ModelManager* modelHolder, const BoardData3d* bd3d, unsigned int point, unsigned int pos, int rotate, int roundPiece, int curveAccuracy, int separateTop);
extern void renderSpecialPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);
extern void DrawDotTemp(const ModelManager* modelHolder, float dotSize, float ds, float zOffset, int* dp, int c);
extern void MAAdie(const renderdata* prd);
extern void renderFlag(const ModelManager* modelHolder, const BoardData3d* bd3d, int curveAccuracy, int turn, int resigned);
extern void drawFlag(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd);
extern void MoveToFlagMiddle(void);
extern void MAApiece(int roundPiece, int curveAccuracy);
extern void renderPiece(const ModelManager* modelHolder, int separateTop);
extern void DrawBackDice(const ModelManager* modelHolder, const BoardData3d* bd3d, const renderdata* prd, diceTest* dt, int diceCol);
extern void RenderCharAA(unsigned int glyph);
extern void renderFlagNumbers(const BoardData3d* bd3d, int resignedValue);
extern void computeNormal(vec3 a, vec3 b, vec3 c, vec3 ret);

#endif
