# Copyright (C) 2012 Michael Petch <mpetch@gnubg.org>

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
# $Id: Makefile.am,v 1.3 2013/05/31 17:19:04 plm Exp $
#

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -DAC_DOCDIR='"$(datadir)/doc/gnubg/"' -DAC_DATADIR='"$(datadir)"' -DAC_PKGDATADIR='"$(datadir)/gnubg"'

GUIRESOURCES = gnubg-gui.rc gnubg-gui.ico
CLIRESOURCES = gnubg-nogui.rc gnubg-nogui.ico

EXTRA_DIST = $(GUIRESOURCES) $(CLIRESOURCES)

if WIN32

if USE_GTK
WINRESOURCES = $(GUIRESOURCES)
else
WINRESOURCES = $(CLIRESOURCES)
endif

BUILT_SOURCES = win32res.o

win32res.o: $(WINRESOURCES) Makefile
	$(WINDRES) $< -o $@

CLEANFILES=win32res.o

endif
