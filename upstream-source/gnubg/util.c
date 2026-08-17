/*
 * Copyright (C) 2007-2009 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2007-2019 the AUTHORS
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
 * $Id: util.c,v 1.43 2022/02/26 20:31:23 plm Exp $
 */

#include "config.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"

char *prefsdir = NULL;
char *datadir = NULL;
char *pkg_datadir = NULL;
char *docdir = NULL;

#if defined(WIN32)
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <direct.h>

/* Default build on WIN32, including msys, installs something not
 * nearly usable as is (it is rather destined to be repackaged in a
 * standalone installer).
 * Define this for the binaries to be more similar to a GNU/Linux build.
 */
/* #define USABLE_UNDER_MSYS 1 */

extern void
PrintSystemError(const char *message)
{
    LPVOID lpMsgBuf;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) & lpMsgBuf, 0, NULL) != 0) {
        g_printerr("Windows error while %s!\n", message);
        g_printerr(": %s", (LPCTSTR) lpMsgBuf);

        if (LocalFree(lpMsgBuf) != NULL)
            g_printerr("LocalFree() failed\n");
    }
}
#else
extern void
PrintSystemError(const char *message)
{
    g_printerr("Unknown system error while %s!\n", message);
}
#endif

extern char *
getDataDir(void)
{
    if (!datadir) {
#if !defined(WIN32)
        datadir = g_strdup(AC_DATADIR);
#else
        char buf[FILENAME_MAX];
        if (GetModuleFileName(NULL, buf, sizeof(buf)) != 0) {
            char *p1 = strrchr(buf, '/'), *p2 = strrchr(buf, '\\');
            int pos1 = (p1 != NULL) ? (int) (p1 - buf) : -1;
            int pos2 = (p2 != NULL) ? (int) (p2 - buf) : -1;
            int pos = MAX(pos1, pos2);
            if (pos > 0)
                buf[pos] = '\0';

            datadir = g_strdup(buf);

            char *testFile = BuildFilename("gnubg.wd");
            if (access(testFile, F_OK) != 0) {
                if (_getcwd(buf, FILENAME_MAX) != 0) {  // Try the current working directory instead
                    g_free(pkg_datadir);
                    pkg_datadir = NULL; // Reset this (set in BuildFilename)
                    g_free(datadir);
                    datadir = g_strdup(buf);
                }
            }
            g_free(testFile);
        }
#endif
    }
    return datadir;
}

extern char *
getPkgDataDir(void)
{
    if (!pkg_datadir)
#if !defined(WIN32) || defined(USABLE_UNDER_MSYS)
        pkg_datadir = g_strdup(AC_PKGDATADIR);
#else
        pkg_datadir = g_build_filename(getDataDir(), NULL);
#endif
    return pkg_datadir;
}

extern char *
getDocDir(void)
{
    if (!docdir)
#if !defined(WIN32) || defined(USABLE_UNDER_MSYS)
        docdir = g_strdup(AC_DOCDIR);
#else
        docdir = g_build_filename(getDataDir(), "doc", NULL);
#endif
    return docdir;
}


void
PrintError(const char *message)
{
    g_printerr("%s: %s", message, strerror(errno));
}

/* Non-Ansi compliant function */
#if defined(__STRICT_ANSI__)
FILE *fdopen(int, const char *);
#endif

extern FILE *
GetTemporaryFile(const char *nameTemplate, char **retName)
{
    FILE *pf;
    int tmpd = g_file_open_tmp(nameTemplate, retName, NULL);

    if (tmpd < 0)
        return NULL;

    pf = fdopen(tmpd, "wb+");

    if (pf == NULL)
        g_free(retName);

    return pf;
}
