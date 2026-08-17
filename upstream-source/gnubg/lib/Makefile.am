# Copyright (C) 1999-2000 Gary Wong <gtw@gnu.org>
# Copyright (C) 2004-2021 the AUTHORS

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
# $Id: Makefile.am,v 1.29 2021/10/06 21:17:00 plm Exp $
# 

##
## All targets have been declared "noinst" for inclusion with gnubg.  As
## a standalone library, it would be better for them to be installed.
##


AM_CPPFLAGS = -I$(srcdir)/.. @GLIB_CFLAGS@

LIBADD = @GLIB_LIBS@

noinst_LTLIBRARIES = libevent.la libsimd.la

libsimd_la_SOURCES = neuralnetsse.c inputs.c output.c
libsimd_la_CFLAGS = $(AM_CFLAGS) $(SIMD_CFLAGS)

libevent_la_SOURCES = list.c neuralnet.c SFMT.c isaac.c md5.c simd.h cache.c \
		      cache.h list.h neuralnet.h SFMT.h SFMT-common.h \
                      SFMT-params.h SFMT-params19937.h isaac.h isaacs.h md5.h \
                      $(srcdir)/../eval.h gnubg-types.h sigmoid.h
libevent_la_LIBADD = libsimd.la

noinst_HEADERS = cache.h list.h neuralnet.h SFMT.h SFMT-common.h \
                 SFMT-params.h SFMT-params19937.h isaac.h isaacs.h md5.h \
                 simd.h $(srcdir)/../eval.h $(srcdir)/../output.h 

