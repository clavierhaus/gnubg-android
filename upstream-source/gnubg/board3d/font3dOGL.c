/*
 * Copyright (C) 2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2024 the AUTHORS
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

#include "config.h"
#include "legacyGLinc.h"

#include <GL/glu.h>	/* Used for vertex tesselation */

#include "fun3d.h"
#include "util.h"
#include "output.h"
#include <glib/gi18n.h>

#include <ft2build.h>
#include FT_FREETYPE_H

void
CheckOpenglError(void)
{
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR)
		g_print(_("OpenGL error: %s\n"), gluErrorString(glErr));
}

int MAArenderGlyph(const FT_Outline* pOutline, int AA);
void PopulateMesh(const Vectoriser* pVect, Mesh* pMesh);

#if !GTK_CHECK_VERSION(3,0,0)
static int
RenderText(char* text, FT_Library ftLib, OGLFont* pFont, const char* pPath, int pointSize, float scale,
	float heightRatio)
{
	FT_Pos len = 0;
	FT_Face face;

	if (FT_New_Face(ftLib, pPath, 0, &face))
		return 0;

	g_assert(face->charmap->encoding == FT_ENCODING_UNICODE);

	if (FT_Set_Char_Size(face, 0, pointSize * 64 /* 26.6 fractional points */, 0, 0))
		return 0;

	/* Create text glyphs */
	pFont->textGlyphs = glGenLists(1);
	glNewList(pFont->textGlyphs, GL_COMPILE);

	pFont->scale = scale;
	pFont->height = 0;

	while (*text) {
	        gunichar codepoint;
		FT_ULong charCode;
		unsigned int glyphIndex;

		codepoint = g_utf8_get_char(text);
		text = g_utf8_next_char(text);

		/* Draw character */
		charCode = (FT_ULong)codepoint;
		glyphIndex = FT_Get_Char_Index(face, charCode);
		if (!glyphIndex)
			return 0;

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING))
			return 0;
		g_assert(face->glyph->format == ft_glyph_format_outline);
		if (pFont->height == 0)
			pFont->height = (float)face->glyph->metrics.height * scale * heightRatio / 64;

		if (!MAArenderGlyph(&face->glyph->outline, 0))
			return 0;

		len += face->glyph->advance.x;
		/* Move on to next place */
		if (*text) {
			FT_Pos kern = 0;
			FT_Vector kernAdvance;
			unsigned int nextGlyphIndex;

			codepoint = g_utf8_get_char(text);

			charCode = (FT_ULong)codepoint;
			nextGlyphIndex = FT_Get_Char_Index(face, charCode);
			if (nextGlyphIndex && !FT_Get_Kerning(face, glyphIndex, nextGlyphIndex, ft_kerning_unfitted, &kernAdvance))
				kern = kernAdvance.x;

			glTranslatef((float) (face->glyph->advance.x + kern) / 64.0f, 0.f, 0.f);
			len += kern;
		}
	}
	glEndList();

	pFont->length = ((float) len / 64.0f) * pFont->scale;

	return !FT_Done_Face(face);
}

int
CreateFontText(OGLFont* ppFont, char* text, const char* fontFile, int pitch, float size, float heightRatio)
{
	char* filename;

	FT_Library ftLib;
	if (FT_Init_FreeType(&ftLib))
		return 0;

	filename = BuildFilename(fontFile);
	if (!RenderText(text, ftLib, ppFont, filename, pitch, size, heightRatio)) {
		outputerrf(_("Failed to create font from (%s)\n"), filename);
		g_free(filename);
		return 0;
	}
	g_free(filename);
	return !FT_Done_FreeType(ftLib);
}
#endif

int
CreateOGLFont(FT_Library ftLib, OGLFont* pFont, const char* pPath, int pointSize, float scale, float heightRatio)
{
	unsigned int i, j;
	FT_Face face;

	memset(pFont, 0, sizeof(OGLFont));
	pFont->scale = scale;
	pFont->heightRatio = heightRatio;

	ModelManagerInit(&pFont->modelManager);
	ModelManagerStart(&pFont->modelManager);

	if (FT_New_Face(ftLib, pPath, 0, &face))
		return 0;

	if (FT_Set_Char_Size(face, 0, pointSize * 64 /* 26.6 fractional points */, 0, 0))
		return 0;

	{                           /* Find height of "1" */
		unsigned int glyphIndex = FT_Get_Char_Index(face, '1');
		if (!glyphIndex)
			return 0;

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING) || (face->glyph->format != ft_glyph_format_outline))
			return 0;

		pFont->height = (float)face->glyph->metrics.height * scale * heightRatio / 64;
		pFont->advance = face->glyph->advance.x;
	}

	/* Create digit glyphs */
#if !GTK_CHECK_VERSION(3,0,0)
	pFont->AAglyphs = glGenLists(10);
#endif

	for (i = 0; i < 10; i++) {
		unsigned int glyphIndex = FT_Get_Char_Index(face, '0' + i);
		if (!glyphIndex)
			return 0;

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING))
			return 0;
		g_assert(face->glyph->format == ft_glyph_format_outline);

		CALL_OGL(&pFont->modelManager, i, RenderGlyph, &face->glyph->outline);

#if !GTK_CHECK_VERSION(3,0,0)
		glNewList(pFont->AAglyphs + i, GL_COMPILE);
		if (!MAArenderGlyph(&face->glyph->outline, 1))
			return 0;
		glEndList();
#endif

		/* Calculate kerning table */
		for (j = 0; j < 10; j++) {
			FT_Vector kernAdvance;
			unsigned int nextGlyphIndex = FT_Get_Char_Index(face, '0' + j);
			if (!nextGlyphIndex || FT_Get_Kerning(face, glyphIndex, nextGlyphIndex, ft_kerning_unfitted, &kernAdvance))
				return 0;
			pFont->kern[i][j] = kernAdvance.x;
		}
	}

	ModelManagerCreate(&pFont->modelManager);

	return !FT_Done_Face(face);
}

#if !GTK_CHECK_VERSION(3,0,0)
int
MAArenderGlyph(const FT_Outline* pOutline, int AA)
{
	Mesh mesh;
	unsigned int index, point;

	Vectoriser vect;
	PopulateVectoriser(&vect, pOutline);
	if ((vect.contours->len < 1) || (vect.numPoints < 3))
		return 0;

	/* Solid font */
	PopulateMesh(&vect, &mesh);

	glNormal3f(0.f, 0.f, 1.f);
	if (!AA)
	{	// Inner part of font
		for (index = 0; index < mesh.tesselations->len; index++) {
			Tesselation* subMesh = &g_array_index(mesh.tesselations, Tesselation, index);

			glBegin(subMesh->meshType);
			for (point = 0; point < subMesh->tessPoints->len; point++) {
				Point3d* pPoint = &g_array_index(subMesh->tessPoints, Point3d, point);

				g_assert(pPoint->data[2] == 0);
				glVertex2f((float)pPoint->data[0] / 64.0f, (float)pPoint->data[1] / 64.0f);
			}
			glEnd();
		}
	}
	else
	{	/* Outline font - used to anti-alias edges */
		for (index = 0; index < vect.contours->len; index++) {
			Contour* contour = &g_array_index(vect.contours, Contour, index);

			glBegin(GL_LINE_LOOP);
			for (point = 0; point < contour->conPoints->len; point++) {
				Point3d* pPoint = &g_array_index(contour->conPoints, Point3d, point);
				glVertex2f((float)pPoint->data[0] / 64.0f, (float)pPoint->data[1] / 64.0f);
			}
			glEnd();
		}
	}
	TidyMemory(&vect, &mesh);

	return 1;
}
#endif

void
FreeNumberFont(OGLFont* ppFont)
{
#if !GTK_CHECK_VERSION(3,0,0)
	if (ppFont->AAglyphs != 0)
		glDeleteLists(ppFont->AAglyphs, 10);
#endif
	ppFont->AAglyphs = 0;
}

#if !GTK_CHECK_VERSION(3,0,0)
void
FreeTextFont(OGLFont* ppFont)
{
	if (ppFont->textGlyphs != 0) {
		glDeleteLists(ppFont->textGlyphs, 1);
		ppFont->textGlyphs = 0;
	}
}
#endif

#if defined(USE_APPLE_OPENGL)
#define GLUFUN(X) X
#elif defined(__GNUC__)
#if defined(WIN32)
typedef APIENTRY GLvoid(*_GLUfuncptr) ();
#endif
#define GLUFUN(X) (_GLUfuncptr)X
#else
#define GLUFUN(X) X
#endif


void
PopulateMesh(const Vectoriser* pVect, Mesh* pMesh)
{
	unsigned int c, p;
	GLUtesselator* tobj = gluNewTess();

	combineList = NULL;

	pMesh->tesselations = g_array_new(FALSE, FALSE, sizeof(Tesselation));

	gluTessCallback(tobj, GLU_TESS_BEGIN_DATA, GLUFUN(tcbBegin));
	gluTessCallback(tobj, GLU_TESS_VERTEX_DATA, GLUFUN(tcbVertex));
	gluTessCallback(tobj, GLU_TESS_COMBINE, GLUFUN(tcbCombine));
	gluTessCallback(tobj, GLU_TESS_END_DATA, GLUFUN(tcbEnd));
	gluTessCallback(tobj, GLU_TESS_ERROR, GLUFUN(tcbError));

	gluTessProperty(tobj, GLU_TESS_WINDING_RULE, (double)GLU_TESS_WINDING_ODD);

	gluTessNormal(tobj, 0.0, 0.0, 1.0);

	gluTessBeginPolygon(tobj, pMesh);

	for (c = 0; c < pVect->contours->len; ++c) {
		Contour* contour = &g_array_index(pVect->contours, Contour, c);

		gluTessBeginContour(tobj);

		for (p = 0; p < contour->conPoints->len; p++) {
			Point3d* point = &g_array_index(contour->conPoints, Point3d, p);
			gluTessVertex(tobj, point->data, point->data);
		}

		gluTessEndContour(tobj);
	}

	gluTessEndPolygon(tobj);

	gluDeleteTess(tobj);
}

#if !GTK_CHECK_VERSION(3,0,0)
extern void
glPrintNumbersRA(const OGLFont* numberFont, const char* text)
{
	/* Right align */
	glTranslatef(-getTextLen3d(numberFont, text), 0.f, 0.f);
	RenderString3d(numberFont, text, numberFont->scale, 0);
}

extern void
glDrawText(const OGLFont* font)
{
	glTranslatef(-font->length / 2.f, -font->height / 2.f, 0.f);
	glScalef(font->scale, font->scale, 1.f);
	glCallList(font->textGlyphs);
}
#endif

#if GTK_CHECK_VERSION(3,0,0)
int CreateFontText(OGLFont* UNUSED(ppFont), char* UNUSED(text), const char* UNUSED(fontFile), int UNUSED(pitch), float UNUSED(size), float UNUSED(heightRatio)) { return 0; }
void FreeTextFont(OGLFont* UNUSED(ppFont)) {}
void glPrintNumbersRA(const OGLFont* UNUSED(numberFont), const char* UNUSED(text)) {}
void glDrawText(const OGLFont* UNUSED(font)) {}
#endif
