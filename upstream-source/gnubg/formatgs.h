/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2006-2015 the AUTHORS
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
 * $Id: formatgs.h,v 1.8 2019/12/21 23:33:11 plm Exp $
 */

#ifndef FORMATGS_H
#define FORMATGS_H

#include <glib.h>

#include "analysis.h"

typedef enum {
    FORMATGS_ALL = -1,
    FORMATGS_CHEQUER = 0,
    FORMATGS_CUBE = 1,
    FORMATGS_LUCK = 2,
    FORMATGS_OVERALL = 3
} formatgs;

extern GList *formatGS(const statcontext * psc, const int nMatchTo, const formatgs fg);

extern void freeGS(GList * list);

#endif                          /* FORMATGS_H */
