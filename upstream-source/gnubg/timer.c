/*
 * Copyright (C) 2003 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2004-2019 the AUTHORS
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
 * $Id: timer.c,v 1.21 2019/01/05 15:15:36 plm Exp $
 */

#include "config.h"

#include <time.h>

#if !HAVE_CLOCK_GETTIME
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#endif

#include "backgammon.h"

#ifdef WIN32
#include <windows.h>

static double perFreq = 0;

static int
setup_timer()
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {    /* Timer not supported */
        return 0;
    } else {
        perFreq = ((double) freq.QuadPart) / 1000;
        return 1;
    }
}

double
get_time()
{                               /* Return elapsed time in milliseconds */
    LARGE_INTEGER timer;

    if (!perFreq) {
        if (!setup_timer())
            return clock() / 1000.0;
    }

    QueryPerformanceCounter(&timer);
    return timer.QuadPart / perFreq;
}

#elif HAVE_CLOCK_GETTIME

extern double
get_time(void)
{                               /* Return elapsed time in milliseconds */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return 1000.0 * (double) ts.tv_sec + 0.000001 * (double) ts.tv_nsec;
}

#else

extern double
get_time(void)
{                               /* Return elapsed time in milliseconds */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return 1000.0 * tv.tv_sec + 0.001 * tv.tv_usec;
}

#endif
