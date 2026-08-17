# Copyright (C) 2002 Gary Wong <gtw@gnu.org>
# Copyright (C) 2007 Christian Anthon <anthon@kiku.dk>

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
# $Id: $
# 

soundsdir = $(pkgdatadir)/sounds
sounds_DATA = chequer.wav dance.wav double.wav drop.wav fanfare.wav \
	gameover.wav haere-ra.wav matchover.wav move.wav resign.wav roll.wav \
	take.wav

EXTRA_DIST = $(sounds_DATA)

#%.wav: %.sco music.orc gnubg.orc
#	if [ $< = haere-ra.sco -o $< = fanfare.sco ]; then orc=music.orc; \
#	else orc=gnubg.orc; fi; $(CSOUND) $$orc $< -W -o $@.tmp && \
#	$(SRCONV) -W -r 22050 -o $@ $@.tmp; $(RM) $@.tmp
