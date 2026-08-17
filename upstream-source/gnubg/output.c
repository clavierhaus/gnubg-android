/*
 * Copyright (C) 1998-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2013-2022 the AUTHORS
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
 * $Id: output.c,v 1.8 2022/03/27 18:05:58 plm Exp $
 */

#include "config.h"
#include "output.h"
#include "backgammon.h"

#include <sys/types.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#if defined(WIN32)
#include <io.h>
#endif

#if defined(USE_GTK_UNDEF)
#undef USE_GTK
#endif

#if defined(USE_GTK)
#include "gtkgame.h"
#endif

int cOutputDisabled;
int cOutputPostponed;
int foutput_on;

#if defined(USE_PYTHON)
char* szMemOutput = NULL;
int foutput_to_mem = FALSE;
#endif

extern void
output_initialize(void)
{
    cOutputDisabled = FALSE;
    cOutputPostponed = FALSE;
    foutput_on = TRUE;
}

/* Write a string to stdout/status bar/popup window */
extern void
output(const char *sz)
{
#if defined(USE_PYTHON)
    if (foutput_to_mem) {
        char *szt = g_strdup_printf("%s", sz);

        if (szMemOutput)
            szMemOutput = g_strconcat(szMemOutput, szt, NULL);
        else
            szMemOutput = g_strdup(szt);

        g_free(szt);
        return;
    }
#endif

    if (cOutputDisabled || !foutput_on)
        return;

#if defined(USE_GTK)
    if (fX) {
        GTKOutput(sz);
        return;
    }
#endif
    g_print("%s", sz);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);
}

/* Write a string to stdout/status bar/popup window, and append \n */
extern void
outputl(const char *sz)
{
#if defined(USE_PYTHON)
    if (foutput_to_mem) {
        char *szt = g_strdup_printf("%s\n", sz);

        szMemOutput = g_strconcat("", szMemOutput, szt, NULL);
        g_free(szt);
        return;
    }
#endif

    if (cOutputDisabled || !foutput_on)
        return;

#if defined(USE_GTK)
    if (fX) {
        char *szOut = g_strdup_printf("%s\n", sz);
        GTKOutput(szOut);
        g_free(szOut);
        return;
    }
#endif
    g_print("%s\n", sz);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);
}

/* Write a character to stdout/status bar/popup window */
extern void
outputc(const char ch)
{
    char sz[2];
    sz[0] = ch;
    sz[1] = '\0';

    output(sz);
}

/* Write a string to stdout/status bar/popup window, printf style */
extern void
outputf(const char *sz, ...)
{
    va_list val;

    va_start(val, sz);
    outputv(sz, val);
    va_end(val);
}

/* Write a string to stdout/status bar/popup window, vprintf style */
extern void
outputv(const char *sz, va_list val)
{
    char *szFormatted;
    if (cOutputDisabled || !foutput_on)
        return;
    szFormatted = g_strdup_vprintf(sz, val);
    output(szFormatted);
    g_free(szFormatted);
}

/* Write an error message, perror() style */
extern void
outputerr(const char *sz)
{
    /* FIXME we probably shouldn't convert the charset of strerror() - yuck! */

    outputerrf("%s: %s", sz, strerror(errno));
}

/* Write an error message, fprintf() style */
extern void
outputerrf(const char *sz, ...)
{
    va_list val;

    va_start(val, sz);
    outputerrv(sz, val);
    va_end(val);
}

/* Write an error message, vfprintf() style */
extern void
outputerrv(const char *sz, va_list val)
{
    char *szFormatted;
    szFormatted = g_strdup_vprintf(sz, val);

#if defined(USE_GTK)
    if (fX)
        GTKOutputErr(szFormatted);
#endif
    fprintf(stderr, "%s", szFormatted);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);
    putc('\n', stderr);
    g_free(szFormatted);
}

/* Signifies that all output for the current command is complete */
extern void
outputx(void)
{
    if (cOutputDisabled || cOutputPostponed || !foutput_on)
        return;

#if defined(USE_GTK)
    if (fX)
        GTKOutputX();
#endif
}

/* Signifies that subsequent output is for a new command */
extern void
outputnew(void)
{
    if (cOutputDisabled || !foutput_on)
        return;

#if defined(USE_GTK)
    if (fX)
        GTKOutputNew();
#endif
}

/* Disable output */
extern void
outputoff(void)
{
    cOutputDisabled++;
}

/* Enable output */
extern void
outputon(void)
{
    g_assert(cOutputDisabled);

    cOutputDisabled--;
}

/* Temporarily disable outputx() calls */
extern void
outputpostpone(void)
{
    cOutputPostponed++;
}

/* Re-enable outputx() calls */
extern void
outputresume(void)
{
    g_assert(cOutputPostponed);

    if (!--cOutputPostponed) {
        outputx();
    }
}

extern void
print_utf8_to_locale(const gchar *sz)
{
    GError *error = NULL;
    gchar *szl = g_locale_from_utf8(sz, -1, NULL, NULL, &error);

    if (error) {
#if 0
        g_printerr("g_locale_from_utf8 failed: %s\n", error->message);
#endif
        g_error_free(error);
    }

    if (szl != NULL)
        printf("%s", szl);
    else
        /* conversion failed, showing original string seems better than nothing */
        printf("%s", sz);

    g_free(szl);
}
