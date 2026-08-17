# Copyright (C) 2003-2021 Jon Kinsey <jonkinsey@gmail.com>
# Copyright (C) 2006-2021 the AUTHORS

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

#
# $Id: Makefile.am,v 1.41 2021/02/21 08:45:52 plm Exp $
# 

##
## All targets have been declared "noinst" for inclusion with gnubg.  As
## a standalone library, it would be better for them to be installed.
##

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../lib \
	-DLOCALEDIR=\"$(localedir)\" \
	-DAC_DATADIR='"$(datadir)"' -DAC_PKGDATADIR=='"$(datadir)/gnubg"'\
	@FREETYPE2_CFLAGS@ @GTK_CFLAGS@ @GTKGLEXT_CFLAGS@ $(GLU_CFLAGS)

if USE_BOARD3D
noinst_LTLIBRARIES = libboard3d.la
libboard3d_la_LIBADD = @FREETYPE2_LIBS@ 
endif

libboard3d_la_SOURCES = DrawOGL.c font3dOGL.c ModelOGL.c Occluders.c PickOGL.c \
			ShimOGL.c ShimOGL.h legacyGLinc.h BoardDimensions.h \
			GLwidget.c \
			drawboard3d.c font3d.c fun3d.h graph.c gtkcolour3d.c \
			matrix.c matrix.h misc3d.c model.c model.h \
			shadow.c widget3d.c tr.c tr.h types3d.h
			
libboard3d_la_CFLAGS = $(AM_CFLAGS)

noinst_HEADERS = fun3d.h matrix.h model.h

shadersfiles = Shaders/basic-fragment.glsl Shaders/basic-vertex.glsl Shaders/main-fragment.glsl Shaders/main-vertex.glsl
shadersdir = $(pkgdatadir)/Shaders
shaders_DATA = $(shadersfiles)
EXTRA_DIST = $(shadersfiles)

