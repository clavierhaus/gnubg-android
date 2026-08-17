/*
 * Copyright (C) 2003-2021 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2003-2019 the AUTHORS
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
 * $Id: misc3d.c,v 1.148 2023/07/04 20:51:08 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "renderprefs.h"
#include "sound.h"
#include "export.h"
#include "gtkgame.h"
#include "util.h"
#include <glib/gstdio.h>
#include "gtklocdefs.h"
#include "gtkboard.h"

#define MAX_FRAMES 10
#define DOT_SIZE 32
#define MAX_DIST ((DOT_SIZE / 2) * (DOT_SIZE / 2))
#define MIN_DIST ((DOT_SIZE / 2) * .70f * (DOT_SIZE / 2) * .70f)
#define TEXTURE_FILE "textures.txt"
#define TEXTURE_FILE_VERSION 3
#define BUF_SIZE 100

/* Performance test data */
static double testStartTime;
static int numFrames;
#define TEST_TIME 3000.0f

static int stopNextTime;
static int slide_move;
NTH_STATIC double animStartTime = 0;

static guint idleId = 0;
static idleFunc *pIdleFun;
static BoardData *pIdleBD;

Flag3d flag;                    /* Only one flag */

static gboolean
idle(BoardData3d * bd3d)
{
    if (pIdleFun(bd3d))
        DrawScene3d(bd3d);

    return TRUE;
}

void
StopIdle3d(const BoardData * bd, BoardData3d * bd3d)
{                               /* Animation has finished (or could have been interruptted) */
    /* If interruptted - reset dice/moving piece details */
    if (bd3d->shakingDice) {
        bd3d->shakingDice = 0;
        updateDiceOccPos(bd, bd3d);
        gtk_main_quit();
    }
    if (bd3d->moving) {
        bd3d->moving = 0;
        updatePieceOccPos(bd, bd3d);
        animation_finished = TRUE;
        gtk_main_quit();
    }

    if (idleId) {
        g_source_remove(idleId);
        idleId = 0;
    }
}

NTH_STATIC void
setIdleFunc(BoardData * bd, idleFunc * pFun)
{
    if (idleId) {
        g_source_remove(idleId);
        idleId = 0;
    }
    pIdleFun = pFun;
    pIdleBD = bd;

    idleId = g_idle_add((GSourceFunc) idle, bd->bd3d);
}

/* Test function to show normal direction */
#if 0
static void
CheckNormal()
{
    float len;
    GLfloat norms[3];
    glGetFloatv(GL_CURRENT_NORMAL, norms);

    len = sqrtf(norms[0] * norms[0] + norms[1] * norms[1] + norms[2] * norms[2]);
    if (fabs(len - 1) > 0.000001)
        len = len;              /*break here */
    norms[0] *= .1f;
    norms[1] *= .1f;
    norms[2] *= .1f;

    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(norms[0], norms[1], norms[2]);
    glEnd();
    glPointSize(5);
    glBegin(GL_POINTS);
    glVertex3f(norms[0], norms[1], norms[2]);
    glEnd();
}
#endif

NTH_STATIC void
SetupLight3d(BoardData3d * bd3d, const renderdata * prd)
{
    float lp[4];
#if !GTK_CHECK_VERSION(3,0,0)
    float al[4], dl[4], sl[4];
#endif

    copyPoint(lp, prd->lightPos);
    lp[3] = prd->lightType == LT_POSITIONAL ? 1.0f : 0.0f;

    /* If directioinal vector is from origin */
    if (prd->lightType != LT_POSITIONAL) {
        lp[0] -= getBoardWidth() / 2.0f;
        lp[1] -= getBoardHeight() / 2.0f;
    }

#if !GTK_CHECK_VERSION(3,0,0)
    al[0] = al[1] = al[2] = (float) prd->lightLevels[0] / 100.0f;
    al[3] = 1;

    dl[0] = dl[1] = dl[2] = (float) prd->lightLevels[1] / 100.0f;
    dl[3] = 1;

    sl[0] = sl[1] = sl[2] = (float) prd->lightLevels[2] / 100.0f;
    sl[3] = 1;

    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightfv(GL_LIGHT0, GL_AMBIENT, al);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dl);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sl);

    UpdateShadowLightPosition(bd3d, lp);
#else
    SetLightPos(lp);
    (void)  bd3d;	/* silence compiler warning */
#endif
}

#ifdef WIN32
/* Determine if a particular extension is supported */

typedef char *(WINAPI * fGetExtStr) (HDC);

extern int
extensionSupported(const char *extension)
{
    static const char *extensionsString = NULL;
    if (extensionsString == NULL)
        extensionsString = (const char *) glGetString(GL_EXTENSIONS);

    if ((extensionsString != NULL) && strstr(extensionsString, extension) != 0)
        return TRUE;

    if (StrNCaseCmp(extension, "WGL_", (gsize) strlen("WGL_")) == 0) {  /* Look for wgl extension */
        static const char *wglExtString = NULL;
        if (wglExtString == NULL) {
            fGetExtStr wglGetExtensionsStringARB;
            wglGetExtensionsStringARB = (fGetExtStr) wglGetProcAddress("wglGetExtensionsStringARB");
            if (!wglGetExtensionsStringARB)
                return FALSE;

            wglExtString = wglGetExtensionsStringARB(wglGetCurrentDC());
        }
        if (strstr(wglExtString, extension) != 0)
            return TRUE;
    }

    return FALSE;
}

#ifndef GL_VERSION_1_2
#ifndef GL_EXT_separate_specular_color
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT    0x81F8
#define GL_SEPARATE_SPECULAR_COLOR_EXT      0x81FA
#endif
#endif

typedef BOOL(APIENTRY * PFNWGLSWAPINTERVALFARPROC) (int);

extern int
setVSync(int state)
{
    static PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;

    if (state == -1)
        return FALSE;

    if (!wglSwapIntervalEXT) {
        if (!extensionSupported("WGL_EXT_swap_control"))
            return FALSE;

        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC) wglGetProcAddress("wglSwapIntervalEXT");
    }
    if ((wglSwapIntervalEXT == NULL) || (wglSwapIntervalEXT(state) == 0))
        return FALSE;

    return TRUE;
}
#endif

static void
CreateTexture(unsigned int *pID, int width, int height, const unsigned char *bits)
{
    /* Create texture */
    glGenTextures(1, (unsigned int *) pID);
    glBindTexture(GL_TEXTURE_2D, *pID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* Read bits */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bits);
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
CreateDotTexture(unsigned int *pDotTexture)
{
    unsigned int i, j;
    unsigned char *data = g_malloc(sizeof(*data) * DOT_SIZE * DOT_SIZE * 3);
    unsigned char *pData = data;

    for (i = 0; i < DOT_SIZE; i++) {
        for (j = 0; j < DOT_SIZE; j++) {
            float xdiff = ((float) i) + .5f - DOT_SIZE / 2;
            float ydiff = ((float) j) + .5f - DOT_SIZE / 2;
            float dist = xdiff * xdiff + ydiff * ydiff;
            float percentage = 1.0f - ((dist - MIN_DIST) / (MAX_DIST - MIN_DIST));
            unsigned char value;
            if (percentage <= 0.0f)
                value = 0;
            else if (percentage >= 1.0f)
                value = 255;
            else
                value = (unsigned char) (255 * percentage);
            pData[0] = pData[1] = pData[2] = value;
            pData += 3;
        }
    }
    CreateTexture(pDotTexture, DOT_SIZE, DOT_SIZE, data);
    g_free(data);
}
#endif

int
CreateFonts(BoardData3d * bd3d)
{
    if (!CreateNumberFont(&bd3d->numberFont, FONT_VERA, FONT_PITCH, FONT_SIZE, FONT_HEIGHT_RATIO))
        return FALSE;

    if (!CreateNumberFont(&bd3d->cubeFont, FONT_VERA_SERIF_BOLD, CUBE_FONT_PITCH, CUBE_FONT_SIZE, CUBE_FONT_HEIGHT_RATIO))
        return FALSE;

    return TRUE;
}

void
InitGL(const BoardData * bd)
{
#if !GTK_CHECK_VERSION(3,0,0)
    float gal[4];
    /* Turn on light 0 */
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /* No global ambient light */
    gal[0] = gal[1] = gal[2] = gal[3] = 0;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gal);

    /* Local specular viewpoint */
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    /* Default to <= depth testing */
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Back face culling */
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    /* Nice hints */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    /* Default blend function for alpha-blending */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Generate normal co-ords for nurbs */
    glEnable(GL_AUTO_NORMAL);
#endif
    if (bd) {
        BoardData3d *bd3d = bd->bd3d;
        /* Setup some 3d things */
        if (!CreateFonts(bd3d))
            g_printerr(_("Error creating fonts\n"));

        shadowInit(bd3d, bd->rd);
#if !GTK_CHECK_VERSION(3,0,0)
#ifdef GL_VERSION_1_2
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#else
        if (extensionSupported("GL_EXT_separate_specular_color"))
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);
#endif
        CreateDotTexture(&bd3d->dotTexture);
#endif
    }
}

const Material* currentMat = NULL;

const Material* GetCurrentMaterial(void)
{
    return currentMat;
}

#if !GTK_CHECK_VERSION(3,0,0)
void
setMaterial(const Material * pMat)
{
	if (pMat != NULL && pMat != currentMat)
	{
		currentMat = pMat;

		glMaterialfv(GL_FRONT, GL_AMBIENT, pMat->ambientColour);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, pMat->diffuseColour);
		glMaterialfv(GL_FRONT, GL_SPECULAR, pMat->specularColour);
		glMateriali(GL_FRONT, GL_SHININESS, pMat->shine);

		if (pMat->pTexture) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, pMat->pTexture->texID);
		}
		else {
			glDisable(GL_TEXTURE_2D);
		}
	}
}
void
setMaterialReset(const Material* pMat)
{
    currentMat = NULL;
    setMaterial(pMat);
}
#endif

float
Dist2d(float a, float b)
{                               /* Find 3rd side size */
    float sqdD = a * a - b * b;
    if (sqdD > 0)
        return sqrtf(sqdD);
    else
        return 0;
}

/* Texture functions */

static GList *textures;

static const char *TextureTypeStrs[TT_COUNT] = { "general", "piece", "hinge" };

GList *
GetTextureList(TextureType type)
{
    GList *glist = NULL;
    GList *pl;
    glist = g_list_append(glist, NO_TEXTURE_STRING);

    for (pl = textures; pl; pl = pl->next) {
        TextureInfo *text = (TextureInfo *) pl->data;
        if (text->type == type)
            glist = g_list_append(glist, text->name);
    }
    return glist;
}

void
FindNamedTexture(TextureInfo ** textureInfo, char *name)
{
    GList *pl;
    for (pl = textures; pl; pl = pl->next) {
        TextureInfo *text = (TextureInfo *) pl->data;
        if (!StrCaseCmp(text->name, name)) {
            *textureInfo = text;
            return;
        }
    }
    *textureInfo = 0;
    /* Only warn user if textures.txt file has been loaded */
    if (g_list_length(textures) > 0)
        g_printerr(_("Texture %s not in texture info file\n"), name);
}

void
FindTexture(TextureInfo ** textureInfo, const char *file)
{
    GList *pl;
    for (pl = textures; pl; pl = pl->next) {
        TextureInfo *text = (TextureInfo *) pl->data;
        if (!StrCaseCmp(text->file, file)) {
            *textureInfo = text;
            return;
        }
    }
    {                           /* Not in texture list, see if old texture on disc */
        char *szFile = BuildFilename(file);
        if (szFile && g_file_test(szFile, G_FILE_TEST_IS_REGULAR)) {
            /* Add entry for unknown texture */
            TextureInfo text;
            strcpy(text.file, file);
            strcpy(text.name, file);
            text.type = TT_NONE;        /* Don't show in lists */

            *textureInfo = (TextureInfo *) g_malloc(sizeof(TextureInfo));
            **textureInfo = text;

            textures = g_list_append(textures, *textureInfo);

            return;
        }
        g_free(szFile);
    }

    *textureInfo = 0;
    /* Only warn user if in 3d */
    if (display_is_3d(GetMainAppearance()))
        g_printerr(_("Texture %s not in texture info file\n"), file);
}

void
LoadTextureInfo(void)
{
    FILE *fp;
    gchar *szFile;
    char buf[BUF_SIZE];

    textures = NULL;

    szFile = BuildFilename(TEXTURE_FILE);
    fp = g_fopen(szFile, "r");
    g_free(szFile);

    if (!fp) {
        g_printerr(_("Error: Texture file (%s) not found\n"), TEXTURE_FILE);
        return;
    }

    if (!fgets(buf, BUF_SIZE, fp) || atoi(buf) != TEXTURE_FILE_VERSION) {
        g_printerr(_("Error: Texture file (%s) out of date\n"), TEXTURE_FILE);
        fclose(fp);
        return;
    }

    do {
        int err, found, i, val;
        size_t len;
        TextureInfo text;

        err = 0;

        /* filename */
        if (!fgets(buf, BUF_SIZE, fp))
            break;              /* finished */

        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            len--;
            buf[len] = '\0';
        }
        if (len > FILENAME_SIZE) {
            g_printerr(_("Texture filename %s too long, maximum length %d.  Entry ignored.\n"), buf, FILENAME_SIZE);
            err = 1;
            strcpy(text.file, "");
        } else
            strcpy(text.file, buf);

        /* name */
        if (!fgets(buf, BUF_SIZE, fp)) {
            g_printerr(_("Error in texture file info.\n"));
            fclose(fp);
            return;
        }
        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            len--;
            buf[len] = '\0';
        }
        if (len > NAME_SIZE) {
            g_printerr(_("Texture name %s too long, maximum length %d.  Entry ignored.\n"), buf, NAME_SIZE);
            err = 1;
        } else
            strcpy(text.name, buf);

        /* type */
        if (!fgets(buf, BUF_SIZE, fp)) {
            g_printerr(_("Error in texture file info.\n"));
            fclose(fp);
            return;
        }
        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            len--;
            buf[len] = '\0';
        }
        found = -1;
        val = 2;
        for (i = 0; i < TT_COUNT; i++) {
            if (!StrCaseCmp(buf, TextureTypeStrs[i])) {
                found = i;
                break;
            }
            val *= 2;
        }
        if (found == -1) {
            g_printerr(_("Unknown texture type %s.  Entry ignored.\n"), buf);
            err = 1;
        } else
            text.type = (TextureType) val;

        if (!err) {             /* Add texture type */
            TextureInfo *pNewText = (TextureInfo *) g_malloc(sizeof(TextureInfo));
            *pNewText = text;
            textures = g_list_append(textures, pNewText);
        }
    } while (!feof(fp));
    fclose(fp);
}

static void
DeleteTexture(Texture * texture)
{
    if (texture->texID)
        glDeleteTextures(1, (unsigned int *) & texture->texID);

    texture->texID = 0;
}

int
LoadTexture(Texture * texture, const char *filename)
{
    unsigned char *bits = 0;
    GError *pix_error = NULL;
    GdkPixbuf *fpixbuf, *pixbuf;

    if (!filename)
        return 0;

    if (g_file_test(filename, G_FILE_TEST_EXISTS))
        fpixbuf = gdk_pixbuf_new_from_file(filename, &pix_error);
    else {
        gchar *tmp = BuildFilename(filename);
        fpixbuf = gdk_pixbuf_new_from_file(tmp, &pix_error);
        g_free(tmp);
    }

    if (pix_error) {
        g_printerr(_("Failed to open texture: %s, %s\n"), filename, pix_error->message);
	g_object_unref(fpixbuf);
        return 0;               /* failed to load file */
    }

    pixbuf = gdk_pixbuf_flip(fpixbuf, FALSE);
    g_object_unref(fpixbuf);

    bits = gdk_pixbuf_get_pixels(pixbuf);

    texture->width = gdk_pixbuf_get_width(pixbuf);
    texture->height = gdk_pixbuf_get_height(pixbuf);

    if (!bits) {
        g_printerr(_("Failed to load texture: %s\n"), filename);
        g_object_unref(pixbuf);
        return 0;               /* failed to load file */
    }

    if (texture->width != texture->height) {
        g_printerr(_("Failed to load texture %s. width (%d) different to height (%d)\n)"),
                filename, texture->width, texture->height);
        g_object_unref(pixbuf);
        return 0;               /* failed to load file */
    }
    /* Check size is a power of 2 */
    if (texture->width <= 0 || (texture->width & (texture->width -1))) {
        g_printerr(_("Failed to load texture %s, size (%d) isn't a power of 2\n"), filename, texture->width);
        g_object_unref(pixbuf);
        return 0;               /* failed to load file */
    }

    CreateTexture(&texture->texID, texture->width, texture->height, bits);

    g_object_unref(pixbuf);

    return 1;
}

static void
SetTexture(BoardData3d * bd3d, Material * pMat, const char *filename)
{
    /* See if already loaded */
    int i;
    const char *nameStart = filename;
    /* Find start of name in filename */
    const char *newStart = 0;

    do {
        if ((newStart = strchr(nameStart, '\\')) == NULL)
            newStart = strchr(nameStart, '/');

        if (newStart)
            nameStart = newStart + 1;
    } while (newStart);

    /* Search for name in cached list */
    for (i = 0; i < bd3d->numTextures; i++) {
        if (!StrCaseCmp(nameStart, bd3d->textureName[i])) {     /* found */
            pMat->pTexture = &bd3d->textureList[i];
            return;
        }
    }

    /* Not found - Load new texture */
    if (bd3d->numTextures == MAX_TEXTURES - 1) {
        g_printerr(_("Error: Too many textures loaded...\n"));
        return;
    }

    if (LoadTexture(&bd3d->textureList[bd3d->numTextures], filename)) {
        /* Remember name */
        bd3d->textureName[bd3d->numTextures] = g_strdup(nameStart);

        pMat->pTexture = &bd3d->textureList[bd3d->numTextures];
        bd3d->numTextures++;
    }
}

static void
GetTexture(BoardData3d * bd3d, Material * pMat)
{
    if (pMat->textureInfo) {
        char buf[100];
        strcpy(buf, TEXTURE_PATH);
        strcat(buf, pMat->textureInfo->file);
        SetTexture(bd3d, pMat, buf);
    } else
        pMat->pTexture = 0;
}

void
GetTextures(BoardData3d * bd3d, renderdata * prd)
{
    GetTexture(bd3d, &prd->ChequerMat[0]);
    GetTexture(bd3d, &prd->ChequerMat[1]);
    GetTexture(bd3d, &prd->BaseMat);
    GetTexture(bd3d, &prd->PointMat[0]);
    GetTexture(bd3d, &prd->PointMat[1]);
    GetTexture(bd3d, &prd->BoxMat);
    GetTexture(bd3d, &prd->HingeMat);
    GetTexture(bd3d, &prd->BackGroundMat);
}

void
Set3dSettings(renderdata * prdnew, const renderdata * prd)
{
    unsigned int i;
    prdnew->pieceType = prd->pieceType;
    prdnew->pieceTextureType = prd->pieceTextureType;
    prdnew->fHinges3d = prd->fHinges3d;
    prdnew->showMoveIndicator = prd->showMoveIndicator;
    prdnew->showShadows = prd->showShadows;
    prdnew->roundedEdges = prd->roundedEdges;
    prdnew->bgInTrays = prd->bgInTrays;
    prdnew->roundedPoints = prd->roundedPoints;
    prdnew->shadowDarkness = prd->shadowDarkness;
    prdnew->curveAccuracy = prd->curveAccuracy;
    prdnew->skewFactor = prd->skewFactor;
    prdnew->boardAngle = prd->boardAngle;
    prdnew->diceSize = prd->diceSize;
    prdnew->planView = prd->planView;

    prdnew->lightType = prd->lightType;
    for (i = 0; i < 3; i++) {
      prdnew->lightPos[i] = prd->lightPos[i];
      prdnew->lightLevels[i] = prd->lightLevels[i];
    }

    memcpy(prdnew->ChequerMat, prd->ChequerMat, sizeof(Material[2]));
    memcpy(&prdnew->DiceMat[0], prd->afDieColour3d[0] ? &prd->ChequerMat[0] : &prd->DiceMat[0], sizeof(Material));
    memcpy(&prdnew->DiceMat[1], prd->afDieColour3d[1] ? &prd->ChequerMat[1] : &prd->DiceMat[1], sizeof(Material));
    prdnew->DiceMat[0].textureInfo = prdnew->DiceMat[1].textureInfo = 0;
    prdnew->DiceMat[0].pTexture = prdnew->DiceMat[1].pTexture = 0;
    memcpy(prdnew->DiceDotMat, prd->DiceDotMat, sizeof(Material[2]));

    memcpy(&prdnew->CubeMat, &prd->CubeMat, sizeof(Material));
    memcpy(&prdnew->CubeNumberMat, &prd->CubeNumberMat, sizeof(Material));

    memcpy(&prdnew->BaseMat, &prd->BaseMat, sizeof(Material));
    memcpy(prdnew->PointMat, prd->PointMat, sizeof(Material[2]));

    memcpy(&prdnew->BoxMat, &prd->BoxMat, sizeof(Material));
    memcpy(&prdnew->HingeMat, &prd->HingeMat, sizeof(Material));
    memcpy(&prdnew->PointNumberMat, &prd->PointNumberMat, sizeof(Material));
    memcpy(&prdnew->BackGroundMat, &prd->BackGroundMat, sizeof(Material));
}

/* Return v position, d distance along path segment */
static float
moveAlong(float d, PathType type, const float start[3], const float end[3], float v[3], float *rotate)
{
    float per, lineLen;

    if (type == PATH_LINE) {
        float xDiff = end[0] - start[0];
        float yDiff = end[1] - start[1];
        float zDiff = end[2] - start[2];

        lineLen = sqrtf(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
        if (d <= lineLen) {
            per = d / lineLen;
            v[0] = start[0] + xDiff * per;
            v[1] = start[1] + yDiff * per;
            v[2] = start[2] + zDiff * per;

            return -1;
        }
    } else if (type == PATH_PARABOLA) {
        float dist = end[0] - start[0];
        lineLen = fabsf(dist);
        if (d <= lineLen) {
            v[0] = start[0] + d * (dist / lineLen);
            v[1] = start[1];
            v[2] = start[2] + 10 * (-d * d + lineLen * d);

            return -1;
        }
    } else if (type == PATH_PARABOLA_12TO3) {
        float dist = end[0] - start[0];
        lineLen = fabsf(dist);
        if (d <= lineLen) {
            v[0] = start[0] + d * (dist / lineLen);
            v[1] = start[1];
            d += lineLen;
            v[2] = start[2] + 10 * (-d * d + lineLen * 2 * d) - 10 * lineLen * lineLen;
            return -1;
        }
    } else {
        float yDiff = end[1] - start[1];

        float xRad = end[0] - start[0];
        float zRad = end[2] - start[2];

        lineLen = F_PI *((fabsf(xRad) + fabsf(zRad)) / 2.0f) / 2.0f;
        if (d <= lineLen) {
            float xCent, zCent;
            float yOff;

            per = d / lineLen;
            if (rotate)
                *rotate = per;

            if (type == PATH_CURVE_9TO12) {
                xCent = end[0];
                zCent = start[2];
                yOff = yDiff * cosf(F_PI_2 * per);
            } else {
                xCent = start[0];
                zCent = end[2];
                yOff = yDiff * sinf(F_PI_2 * per);
            }

            if (type == PATH_CURVE_9TO12) {
                v[0] = xCent - xRad * cosf(F_PI_2 * per);
                v[1] = end[1] - yOff;
                v[2] = zCent + zRad * sinf(F_PI_2 * per);
            } else {
                v[0] = xCent + xRad * sinf(F_PI_2 * per);
                v[1] = start[1] + yOff;
                v[2] = zCent - zRad * cosf(F_PI_2 * per);
            }
            return -1;
        }
    }
    /* Finished path segment */
    return lineLen;
}

/* Return v position, d distance along path p */
int
movePath(Path * p, float d, float *rotate, float v[3])
{
    float done;
    d -= p->mileStone;

    while (!finishedPath(p) && (done = moveAlong(d, p->pathType[p->state], p->pts[p->state], p->pts[p->state + 1], v, rotate)) >= 0.0f) { /* Next path segment */
        d -= done;
        p->mileStone += done;
        p->state++;
    }
    return !finishedPath(p);
}

void
initPath(Path * p, const float start[3])
{
    p->state = 0;
    p->numSegments = 0;
    p->mileStone = 0;
    copyPoint(p->pts[0], start);
}

int
finishedPath(const Path * p)
{
    return (p->state == p->numSegments);
}

void
addPathSegment(Path * p, PathType type, const float point[3])
{
    if (type == PATH_PARABOLA_12TO3) {  /* Move start y up to top of parabola */
        float l = p->pts[p->numSegments][0] - point[0];
        p->pts[p->numSegments][2] += 10 * l * l;
    }

    p->pathType[p->numSegments] = type;
    p->numSegments++;
    copyPoint(p->pts[p->numSegments], point);
}

/* Return a random number in 0-range */
float
randRange(float range)
{
    return range * ((float) rand() / (float) RAND_MAX);
}

/* Setup dice test to initial pos */
void
initDT(diceTest * dt, int x, int y, int z)
{
    dt->top = 0;
    dt->bottom = 5;

    dt->side[0] = 3;
    dt->side[1] = 1;
    dt->side[2] = 2;
    dt->side[3] = 4;

    /* Simulate rotations to determine actual dice position */
    while (x--) {
        int temp = dt->top;
        dt->top = dt->side[0];
        dt->side[0] = dt->bottom;
        dt->bottom = dt->side[2];
        dt->side[2] = temp;
    }
    while (y--) {
        int temp = dt->top;
        dt->top = dt->side[1];
        dt->side[1] = dt->bottom;
        dt->bottom = dt->side[3];
        dt->side[3] = temp;
    }
    while (z--) {
        int temp = dt->side[0];
        dt->side[0] = dt->side[3];
        dt->side[3] = dt->side[2];
        dt->side[2] = dt->side[1];
        dt->side[1] = temp;
    }
}

float ***
Alloc3d(unsigned int x, unsigned int y, unsigned int z)
{                               /* Allocate 3d array */
    unsigned int i, j;
    float ***array = (float ***) g_malloc(sizeof(float **) * x);
    for (i = 0; i < x; i++) {
        array[i] = (float **) g_malloc(sizeof(float *) * y);
        for (j = 0; j < y; j++)
            array[i][j] = (float *) g_malloc(sizeof(float) * z);
    }
    return array;
}

void
Free3d(float ***array, unsigned int x, unsigned int y)
{                               /* Free 3d array */
    unsigned int i, j;
    for (i = 0; i < x; i++) {
        for (j = 0; j < y; j++)
            g_free(array[i][j]);
        g_free(array[i]);
    }
    g_free(array);
}

void
calculateEigthPoints(EigthPoints* eigthPoints, float radius, unsigned int accuracy)
{
    unsigned int i, j;

    float lat_angle;
    float lat_step;
    float step = 0;
    unsigned int corner_steps = (accuracy / 4) + 1;

	eigthPoints->points = Alloc3d(corner_steps, corner_steps, 3);
	eigthPoints->accuracy = accuracy;

    lat_angle = 0;
    lat_step = (2 * F_PI) / (float) accuracy;

    /* Calculate corner 1/8th sphere points */
    for (i = 0; i < (accuracy / 4) + 1; i++) {
        float latitude = sinf(lat_angle) * radius;
        float new_radius = Dist2d(radius, latitude);
        float angle = 0.0f ;
        unsigned int ns = (accuracy / 4) - i;

        if (ns > 0)
            step = (2.f * F_PI) / ((float) ns * 4.f);

        for (j = 0; j <= ns; j++) {
            eigthPoints->points[i][j][0] = sinf(angle) * new_radius;
            eigthPoints->points[i][j][1] = latitude;
            eigthPoints->points[i][j][2] = cosf(angle) * new_radius;

            angle += step;
        }
        lat_angle += lat_step;
    }
}

void
freeEigthPoints(EigthPoints* eigthPoints)
{
	if (eigthPoints->points != NULL)
	{
		unsigned int corner_steps = (eigthPoints->accuracy / 4) + 1;

		Free3d(eigthPoints->points, corner_steps, corner_steps);
		eigthPoints->points = NULL;
	}
}

void
SetColour3d(float r, float g, float b, float a)
{
    static Material col;
    SetupSimpleMatAlpha(&col, r, g, b, a);
    setMaterialReset(&col);
}

void
SetMovingPieceRotation(const BoardData * bd, BoardData3d * bd3d, unsigned int pt)
{                               /* Make sure piece is rotated correctly while dragging */
    bd3d->movingPieceRotation = bd3d->pieceRotation[pt][Abs(bd->points[pt])];
}

void
PlaceMovingPieceRotation(const BoardData * bd, BoardData3d * bd3d, unsigned int dest, unsigned int src)
{                               /* Make sure rotation is correct in new position */
    bd3d->pieceRotation[src][Abs(bd->points[src])] = bd3d->pieceRotation[dest][Abs(bd->points[dest] - 1)];
    bd3d->pieceRotation[dest][Abs(bd->points[dest]) - 1] = bd3d->movingPieceRotation;
}

int
MouseMove3d(const BoardData * bd, BoardData3d * bd3d, const renderdata * UNUSED(prd), int x, int y)
{
    if (bd->drag_point >= 0) {

        MakeCurrent3d(bd3d);
        getProjectedPieceDragPos(x, y, bd3d->dragPos);
        updateMovingPieceOccPos(bd, bd3d);
        return 1;
    } else
        return 0;
}

static void
updateDicePos(Path * path, DiceRotation * diceRot, float dist, float pos[3])
{
    if (movePath(path, dist, 0, pos)) {
        diceRot->xRot = (dist * diceRot->xRotFactor + diceRot->xRotStart) * 360;
        diceRot->yRot = (dist * diceRot->yRotFactor + diceRot->yRotStart) * 360;
    } else {                    /* Finished - set to end point */
        copyPoint(pos, path->pts[path->numSegments]);
        diceRot->xRot = diceRot->yRot = 0;
    }
}

static void
SetupMove(BoardData * bd, BoardData3d * bd3d)
{
    unsigned int destDepth;
    unsigned int target = convert_point(animate_move_list[slide_move], animate_player);
    unsigned int dest = convert_point(animate_move_list[slide_move + 1], animate_player);
    int dir = animate_player ? 1 : -1;

    bd->points[target] -= dir;

    animStartTime = get_time();

    destDepth = Abs(bd->points[dest]) + 1;
    if ((Abs(bd->points[dest]) == 1) && (dir != SGN(bd->points[dest])))
        destDepth--;

    setupPath(bd, &bd3d->piecePath, &bd3d->rotateMovingPiece, target, Abs(bd->points[target]) + 1, dest, destDepth);
    copyPoint(bd3d->movingPos, bd3d->piecePath.pts[0]);

    SetMovingPieceRotation(bd, bd3d, target);

    updatePieceOccPos(bd, bd3d);
}

static int
idleAnimate(BoardData3d * bd3d)
{
    BoardData *bd = pIdleBD;
    float elapsedTime = (float) (get_time() - animStartTime) / 1000.0f;
    float vel = .2f + (float) nGUIAnimSpeed * .3f;
    float animateDistance = elapsedTime * vel;

    if (stopNextTime) {         /* Stop now - last animation frame has been drawn */
        StopIdle3d(bd, bd->bd3d);
        gtk_main_quit();
        return 1;
    }

    if (bd3d->moving) {
        float old_pos[3];
        float *pRotate = 0;
        if (bd3d->rotateMovingPiece >= 0.0f && bd3d->piecePath.state == 2)
            pRotate = &bd3d->rotateMovingPiece;

        copyPoint(old_pos, bd3d->movingPos);

        if (!movePath(&bd3d->piecePath, animateDistance, pRotate, bd3d->movingPos)) {
            TanBoard points;
            unsigned int moveStart = convert_point(animate_move_list[slide_move], animate_player);
            unsigned int moveDest = convert_point(animate_move_list[slide_move + 1], animate_player);

            if ((Abs(bd->points[moveDest]) == 1) && (bd->turn != SGN(bd->points[moveDest]))) {  /* huff */
                unsigned int bar;
                if (bd->turn == 1)
                    bar = 0;
                else
                    bar = 25;
                bd->points[bar] -= bd->turn;
                bd->points[moveDest] = 0;
            }

            bd->points[moveDest] += bd->turn;

            /* Update pip-count mid move */
            read_board(bd, points);
            update_pipcount(bd, (ConstTanBoard) points);

            PlaceMovingPieceRotation(bd, bd3d, moveDest, moveStart);

            /* Next piece */
            slide_move += 2;

            if (slide_move >= 8 || animate_move_list[slide_move] < 0) { /* All done */
                bd3d->moving = 0;
                updatePieceOccPos(bd, bd3d);
                animation_finished = TRUE;
                stopNextTime = 1;
            } else
                SetupMove(bd, bd3d);

            playSound(SOUND_CHEQUER);
        } else {
            updateMovingPieceOccPos(bd, bd3d);
        }
    }

    if (bd3d->shakingDice) {
        if (!finishedPath(&bd3d->dicePaths[0]))
            updateDicePos(&bd3d->dicePaths[0], &bd3d->diceRotation[0], animateDistance / 2.0f, bd3d->diceMovingPos[0]);
        if (!finishedPath(&bd3d->dicePaths[1]))
            updateDicePos(&bd3d->dicePaths[1], &bd3d->diceRotation[1], animateDistance / 2.0f, bd3d->diceMovingPos[1]);

        if (finishedPath(&bd3d->dicePaths[0]) && finishedPath(&bd3d->dicePaths[1])) {
            bd3d->shakingDice = 0;
            stopNextTime = 1;
        }
        updateDiceOccPos(bd, bd3d);
    }

    return 1;
}

void
RollDice3d(BoardData * bd, BoardData3d * bd3d, const renderdata * prd)
{                               /* animate the dice roll if not below board */
    setDicePos(bd, bd3d);
    GTKSuspendInput();

    if (prd->animateRoll) {
        animStartTime = get_time();

        bd3d->shakingDice = 1;
        stopNextTime = 0;
        setIdleFunc(bd, idleAnimate);

        setupDicePaths(bd, bd3d->dicePaths, bd3d->diceMovingPos, bd3d->diceRotation);
        /* Make sure shadows are in correct place */
        UpdateShadows(bd->bd3d);
		gtk_main();
    } else {
        /* Show dice on board */
        gtk_widget_queue_draw(bd3d->drawing_area3d);
        while (gtk_events_pending())
            gtk_main_iteration();
    }
    GTKResumeInput();
}

void
AnimateMove3d(BoardData * bd, BoardData3d * bd3d)
{
    slide_move = 0;
    bd3d->moving = 1;

    SetupMove(bd, bd3d);

    stopNextTime = 0;
    setIdleFunc(bd, idleAnimate);
    GTKSuspendInput();
    gtk_main();
    GTKResumeInput();
}

NTH_STATIC int
idleWaveFlag(BoardData3d * bd3d)
{
    BoardData *bd = pIdleBD;
    float elapsedTime = (float) (get_time() - animStartTime);
    bd3d->flagWaved = elapsedTime / 200;
    updateFlagOccPos(bd, bd3d);
    return 1;
}

void
ShowFlag3d(BoardData * bd, BoardData3d * bd3d, const renderdata * prd)
{
    bd3d->flagWaved = 0;

    if (prd->animateFlag && bd->resigned && ms.gs == GAME_PLAYING && bd->playing && (ap[bd->turn == 1 ? 0 : 1].pt == PLAYER_HUMAN)) {   /* not for computer turn */
        animStartTime = get_time();
        setIdleFunc(bd, idleWaveFlag);
    } else
        StopIdle3d(bd, bd->bd3d);

    waveFlag(bd3d->flagWaved);
    updateFlagOccPos(bd, bd3d);
}

static int
idleTestPerformance(BoardData3d * bd3d)
{
    BoardData *bd = pIdleBD;
    float elapsedTime = (float) (get_time() - testStartTime);
    if (elapsedTime > TEST_TIME) {
        StopIdle3d(bd, bd3d);
        gtk_main_quit();
        return 0;
    }
    numFrames++;
    return 1;
}

float
TestPerformance3d(BoardData * bd)
{
    float elapsedTime;

    setIdleFunc(bd, idleTestPerformance);
    testStartTime = get_time();
    numFrames = 0;
    gtk_main();
    elapsedTime = (float) (get_time() - testStartTime);

    return ((float) numFrames / (elapsedTime / 1000.0f));
}

#ifdef TEST_HARNESS
NTH_STATIC void
EmptyPos(BoardData * bd)
{                               /* All checkers home */
    int ip[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, -15 };
    memcpy(bd->points, ip, sizeof(bd->points));
    updatePieceOccPos(bd, bd->bd3d);
}
#endif

void
SetupViewingVolume3d(const BoardData * bd, BoardData3d * bd3d, const renderdata * prd, int viewport[4])
{
   	float *projMat, *modelMat;

	SetupViewingVolume3dNew(bd, bd3d, prd, &projMat, &modelMat, viewport);

#if !GTK_CHECK_VERSION(3,0,0)
	/* Setup openGL legacy matrices */
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projMat);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelMat);
#else
    SetViewPos();
#endif

    SetupLight3d(bd3d, prd);

	calculateBackgroundSize(bd3d, viewport);
#if !GTK_CHECK_VERSION(3,0,0)
    if (bd3d->modelHolder.vertexData != NULL)
#endif
	{	/* Update background to new size */
        UPDATE_OGL(&bd3d->modelHolder, MT_BACKGROUND, drawBackground, prd, bd3d->backGroundPos, bd3d->backGroundSize);
	}
}

void
SetupMat(Material * pMat, float r, float g, float b, float dr, float dg, float db, float sr, float sg, float sb,
         int shin, float a)
{
    pMat->ambientColour[0] = r;
    pMat->ambientColour[1] = g;
    pMat->ambientColour[2] = b;
    pMat->ambientColour[3] = a;

    pMat->diffuseColour[0] = dr;
    pMat->diffuseColour[1] = dg;
    pMat->diffuseColour[2] = db;
    pMat->diffuseColour[3] = a;

    pMat->specularColour[0] = sr;
    pMat->specularColour[1] = sg;
    pMat->specularColour[2] = sb;
    pMat->specularColour[3] = a;
    pMat->shine = shin;

    pMat->alphaBlend = (a < 1.0f) && (a > 0.0f);

    pMat->textureInfo = NULL;
    pMat->pTexture = NULL;
}

void
SetupSimpleMatAlpha(Material * pMat, float r, float g, float b, float a)
{
    SetupMat(pMat, r, g, b, r, g, b, 0.f, 0.f, 0.f, 0, a);
}

void
SetupSimpleMat(Material * pMat, float r, float g, float b)
{
    SetupMat(pMat, r, g, b, r, g, b, 0.f, 0.f, 0.f, 0, 0.f);
}

/* Not currently used
 * void RemoveTexture(Material* pMat)
 * {
 * if (pMat->pTexture)
 * {
 * int i = 0;
 * while (&bd->textureList[i] != pMat->pTexture)
 * i++;
 * 
 * DeleteTexture(&bd->textureList[i]);
 * g_free(bd->textureName[i]);
 * 
 * while (i != bd->numTextures - 1)
 * {
 * bd->textureList[i] = bd->textureList[i + 1];
 * bd->textureName[i] = bd->textureName[i + 1];
 * i++;
 * }
 * bd->numTextures--;
 * pMat->pTexture = 0;
 * }
 * }
 */

void
ClearTextures(BoardData3d * bd3d)
{
    int i;
    if (!bd3d->numTextures)
        return;

    MakeCurrent3d(bd3d);

    for (i = 0; i < bd3d->numTextures; i++) {
        DeleteTexture(&bd3d->textureList[i]);
        g_free(bd3d->textureName[i]);
    }
    bd3d->numTextures = 0;
}

static void
free_texture(gpointer data, gpointer UNUSED(userdata))
{
    g_free(data);
}

void
DeleteTextureList(void)
{
    g_list_foreach(textures, free_texture, NULL);
    g_list_free(textures);
}

void
InitBoard3d(BoardData * bd, BoardData3d * bd3d)
{                               /* Initialize 3d parts of boarddata */
    int i, j;
    /* Assign random rotation to each board position */
    for (i = 0; i < 28; i++)
        for (j = 0; j < 15; j++)
            bd3d->pieceRotation[i][j] = rand() % 360;

    bd3d->shadowsInitialised = FALSE;
    bd3d->shadowsOutofDate = FALSE;
    bd3d->moving = 0;
    bd3d->shakingDice = 0;
    bd->drag_point = -1;
    bd->DragTargetHelp = 0;

    SetupSimpleMat(&bd3d->gapColour, 0.f, 0.f, 0.f);
    SetupSimpleMat(&bd3d->flagMat, 1.f, 1.f, 1.f);    /* White flag */
    SetupSimpleMat(&bd3d->flagPoleMat, .2f, .2f, .4f);	/* Blue pole */
    SetupMat(&bd3d->flagNumberMat, 0.f, 0.f, .4f, 0.f, 0.f, .4f, 1.f, 1.f, 1.f, 100, 1.f);

    bd3d->numTextures = 0;

    bd3d->boardPoints.points = NULL;

    bd3d->fBuffers = FALSE;

    ModelManagerInit(&bd3d->modelHolder);
}

#if defined(HAVE_LIBPNG)

/* Used by ../export.c to export positions to PNG format */

void
GenerateImage3d(const char *szName, unsigned int nSize, unsigned int nSizeX, unsigned int nSizeY)
{
    RenderToBufferData renderToBufferData;

    if (!fX) {
	outputerrf(_("PNG file creation failed: %s\n"), _("exporting a 3D image is only supported from the GUI"));
	return;
    }

    renderToBufferData.bd = BOARD(pwBoard)->board_data;
    renderToBufferData.width = nSize * nSizeX;
    renderToBufferData.height = nSize * nSizeY;

    /* Allocate buffer for image, height + 1 as extra line needed to invert image (opengl renders 'upside down') */
    renderToBufferData.puch = (unsigned char *) g_malloc(renderToBufferData.width * (renderToBufferData.height + 1) * 3);

    GLWidgetRender(renderToBufferData.bd->bd3d->drawing_area3d, RenderToBuffer3d, NULL, &renderToBufferData);

    GdkPixbuf* pixbuf;
    GError* error = NULL;
    unsigned int line;

    /* invert image (y axis) */
    for (line = 0; line < renderToBufferData.height / 2; line++) {
        unsigned int lineSize = renderToBufferData.width * 3;
        /* Swap two lines at a time */
        memcpy(renderToBufferData.puch + renderToBufferData.height * lineSize, renderToBufferData.puch + line * lineSize, lineSize);
        memcpy(renderToBufferData.puch + line * lineSize, renderToBufferData.puch + ((renderToBufferData.height - line) - 1) * lineSize, lineSize);
        memcpy(renderToBufferData.puch + ((renderToBufferData.height - line) - 1) * lineSize, renderToBufferData.puch + renderToBufferData.height * lineSize, lineSize);
    }

    pixbuf = gdk_pixbuf_new_from_data(renderToBufferData.puch, GDK_COLORSPACE_RGB, FALSE, 8,
                                      (int)renderToBufferData.width, (int)renderToBufferData.height, (int)renderToBufferData.width * 3, NULL, NULL);

    gdk_pixbuf_save(pixbuf, szName, "png", &error, NULL);

    g_object_unref(pixbuf);
    g_free(renderToBufferData.puch);

    if (error) {
        outputerrf(_("PNG file creation failed: %s\n"), error->message);
        g_error_free(error);
    }
}

#endif

extern GtkWidget *
GetDrawingArea3d(const BoardData3d * bd3d)
{
    return bd3d->drawing_area3d;
}

extern char *
MaterialGetTextureFilename(const Material * pMat)
{
    return pMat->textureInfo->file;
}

extern void
DrawScene3d(const BoardData3d * bd3d)
{
    gtk_widget_queue_draw(bd3d->drawing_area3d);
}

extern int
Animating3d(const BoardData3d * bd3d)
{
    return (bd3d->shakingDice || bd3d->moving);
}

extern void
Draw3d(const BoardData * bd)
{                               /* Render board: standard or 2 passes for shadows */
	drawBoard(bd, bd->bd3d, bd->rd);
	if (bd->rd->showShadows)
		shadowDisplay(bd, bd->bd3d, bd->rd);
}

void
ClearScreen(const renderdata* prd)
{
	glClearColor(prd->BackGroundMat.ambientColour[0], prd->BackGroundMat.ambientColour[1],
		prd->BackGroundMat.ambientColour[2], 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
glSetViewport(int viewport[4])
{
	glViewport(viewport[0], viewport[1], viewport[2] - viewport[0], viewport[3] - viewport[1]);
}

void RecalcViewingVolume(const BoardData* bd)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	SetupViewingVolume3d(bd, bd->bd3d, bd->rd, viewport);
}

void computeNormal(vec3 a, vec3 b, vec3 c, vec3 ret)
{
    vec3 diff1, diff2;
    glm_vec3_sub(c, a, diff1);
    glm_vec3_sub(b, a, diff2);
    glm_vec3_cross(diff1, diff2, ret);
    glm_vec3_normalize(ret);
    ret[2] = -ret[2];
}
