/*
 * Copyright (C) 2002-2004 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2003-2017 the AUTHORS
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
 * $Id: openurl.c,v 1.31 2017/09/10 11:57:41 plm Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "backgammon.h"
#include "openurl.h"
#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#else
#include <string.h>
#endif                          /* _WIN32 */
static gchar *web_browser = NULL;

extern const gchar *
get_web_browser(void)
{
    const gchar *pch;
#if defined(_WIN32)
    if (!web_browser || !*web_browser)
        return ("");
    else
#else
    if (web_browser && *web_browser)
#endif
        return web_browser;
    if ((pch = g_getenv("BROWSER")) == NULL) {
#if defined(__APPLE__)
        pch = "open";
#else
        pch = OPEN_URL_PROG;
#endif
    }
    return pch;
}


extern char *
set_web_browser(const char *sz)
{
    g_free(web_browser);
    web_browser = g_strdup(sz ? sz : "");
    return web_browser;
}

extern void
OpenURL(const char *szURL)
{
    const gchar *browser = get_web_browser();
    gchar *commandString;
    GError *error = NULL;
    if (!(browser) || !(*browser)) {
#if defined(_WIN32)
        ptrdiff_t win_error;
        gchar *url = g_filename_to_uri(szURL, NULL, NULL);
        win_error = (ptrdiff_t) ShellExecute(NULL, TEXT("open"), url ? url : szURL, NULL, ".\\", SW_SHOWNORMAL);
        if (win_error >= 0 && win_error < 33)
            outputerrf(_("Failed to perform default action on " "%s. Error code was %d"), url, (int)win_error);
        g_free(url);
        return;
#endif
    }
    commandString = g_strdup_printf("'%s' '%s'", browser, szURL);
    if (!g_spawn_command_line_async(commandString, &error)) {
        outputerrf(_("Browser couldn't open file (%s): %s\n"), commandString, error->message);
        g_error_free(error);
    }
    return;
}
