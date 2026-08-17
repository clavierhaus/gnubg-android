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
 * $Id: legacyGLinc.h,v 1.2 2019/12/24 09:49:23 plm Exp $
 */

//#define TEST_LEGACY_OGL
#ifdef TEST_LEGACY_OGL
	/* Debug no legacy OGL functions */
	#define GL_GLEXT_PROTOTYPES
	#include <gl\glcorearb.h>
#else

#if defined(WIN32)
/* MS gl.h needs windows.h to be included first */
#include <windows.h>
#endif

#if defined(USE_APPLE_OPENGL)
#include <gl.h>
#else
#include <GL/gl.h>
#endif

#if defined(HAVE_GL_GLX_H)
#include <GL/glx.h>             /* x-windows file */
#endif

#endif
