/*
 * Copyright (C) 2007-2009 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2007-2013 the AUTHORS
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
 * $Id: util.h,v 1.15 2013/07/11 19:16:07 mdpetch Exp $
 */

#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <stdio.h>

extern char *prefsdir;
extern char *datadir;
extern char *pkg_datadir;
extern char *docdir;
extern char *getDataDir(void);
extern char *getPkgDataDir(void);
extern char *getDocDir(void);

#define BuildFilename(file) g_build_filename(getPkgDataDir(), file, NULL)
#define BuildFilename2(file1, file2) g_build_filename(getPkgDataDir(), file1, file2, NULL)

extern void PrintSystemError(const char *message);
extern void PrintError(const char *message);
extern FILE *GetTemporaryFile(const char *nameTemplate, char **retName);

#endif
