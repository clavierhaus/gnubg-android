/*
 * Copyright (C) 1999-2002 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2015-2022 the AUTHORS
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
 * $Id: output.h,v 1.6 2022/01/27 22:14:47 plm Exp $
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>
#include <glib.h>
#include "common.h"

/* Initialize output module */
extern void output_initialize(void);
/* Write a string to stdout/status bar/popup window */
extern void output(const char *sz);
/* Write a string to stdout/status bar/popup window, and append \n */
extern void outputl(const char *sz);
/* Write a character to stdout/status bar/popup window */
extern void outputc(const char ch);
/* Write an error message, perror() style */
extern void outputerr(const char *sz);

#if defined(HAVE_FUNC_ATTRIBUTE_FORMAT)
/* Write a string to stdout/status bar/popup window, printf style */
extern void outputf(const char *sz, ...)
    __attribute__ ((format(printf, 1, 2)));
/* Write a string to stdout/status bar/popup window, vprintf style */
extern void outputv(const char *sz, va_list val)
    __attribute__ ((format(printf, 1, 0)));
/* Write an error message, fprintf() style */
extern void outputerrf(const char *sz, ...)
    __attribute__ ((format(printf, 1, 2)));
/* Write an error message, vfprintf() style */
extern void outputerrv(const char *sz, va_list val)
    __attribute__ ((format(printf, 1, 0)));
#else
/* Write a string to stdout/status bar/popup window, printf style */
extern void outputf(const char *sz, ...);
/* Write a string to stdout/status bar/popup window, vprintf style */
extern void outputv(const char *sz, va_list val);
/* Write an error message, fprintf() style */
extern void outputerrf(const char *sz, ...);
/* Write an error message, vfprintf() style */
extern void outputerrv(const char *sz, va_list val);
#endif

/* Signifies that all output for the current command is complete */
extern void outputx(void);
/* Temporarily disable outputx() calls */
extern void outputpostpone(void);
/* Re-enable outputx() calls */
extern void outputresume(void);
/* Signifies that subsequent output is for a new command */
extern void outputnew(void);
/* Disable output */
extern void outputoff(void);
/* Enable output */
extern void outputon(void);

extern int cOutputDisabled;
extern int cOutputPostponed;
extern int foutput_on;

#if defined(USE_PYTHON)
/* output-family functions allocate with g_strdup_printf(), caller g_free()s */
extern char* szMemOutput;

extern int foutput_to_mem;
#endif

extern void print_utf8_to_locale(const gchar *sz);

#endif
