/*
 * Copyright (C) 2006-2009 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2006-2017 the AUTHORS
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
 * $Id: common.h,v 1.35 2019/12/02 10:56:00 plm Exp $
 */

/*! \file common.h
 * \brief Odd definitions
 */

#ifndef COMMON_H
#define COMMON_H
#include <math.h>
#include "config.h"


#ifndef HAVE___ATTRIBUTE__
#define __attribute__(Spec) /* Empty */
#endif /* HAVE___ATTRIBUTE__ */

#define F_PI (float)G_PI
#define F_PI_2 (float)G_PI_2

/*! \brief Safe error value
 */
#if defined(HUGE_VALF)
#define ERR_VAL (-HUGE_VALF)
#else
#define ERR_VAL (-FLT_MAX)
#endif

/*! \brief Macro to extract sign
 */
#ifndef SGN
#define SGN(x) ((x) > 0 ? 1 : -1)
#endif

/*! \brief typedef of signal handler function
 */
#if HAVE_SIGACTION
typedef struct sigaction psighandler;
#elif HAVE_SIGVEC
typedef struct sigvec psighandler;
#else
typedef void (*psighandler) (int);
#endif

/* abs returns unsigned int by definition */
#define Abs(a) ((unsigned int)abs(a))

/* Do we need to use g_utf8_casefold() for utf8 anywhere? */
#define StrCaseCmp(s1, s2) g_ascii_strcasecmp(s1, s2)
#define StrNCaseCmp(s1, s2, n) g_ascii_strncasecmp(s1, s2, (gsize)n)
/* if the following are defined as macros undefine them */
#ifdef strcasecmp
#undef strcasecmp
#endif
#ifdef strncasecmp
#undef strncasecmp
#endif
/* Avoid new code using strcase functions */
#define strcasecmp strcasecmp_error_use_StrCaseCmp
#define strncasecmp strncasecmp_error_use_StrNCaseCmp


/* Macro to mark parameters that aren't used in the function */
#ifdef UNUSED
#elif defined(HAVE_FUNC_ATTRIBUTE_UNUSED)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(_lint)
#define UNUSED(x) /*lint -e{715, 818}*/ _unused_##x
#else
#define UNUSED(x) _unused_##x
#endif

/* Helper macros for __builtin_expect */
#ifdef HAVE___BUILTIN_EXPECT
#define likely(expression)	__builtin_expect(!!(expression), 1)
#define unlikely(expression)	__builtin_expect(!!(expression), 0)
#else
#define likely(expression)	(expression)
#define unlikely(expression)	(expression)
#endif

#endif
