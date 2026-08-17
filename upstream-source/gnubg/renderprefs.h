/*
 * Copyright (C) 2000-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2003-2009 the AUTHORS
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
 * $Id: renderprefs.h,v 1.12 2019/04/26 23:08:44 plm Exp $
 */

#ifndef RENDERPREFS_H
#define RENDERPREFS_H

#ifndef RENDER_H
#include "render.h"
#endif

extern const char *aszWoodName[];
extern renderdata *GetMainAppearance(void);
extern void CopyAppearance(renderdata * prd);

extern void RenderPreferencesParam(renderdata * prd, const char *szParam, char *szValue);
extern void SaveRenderingSettings(FILE * pf);

#if defined(USE_BOARD3D)
char *WriteMaterial(Material * pMat);
char *WriteMaterialDice(renderdata * prd, int num);
#endif

#endif
