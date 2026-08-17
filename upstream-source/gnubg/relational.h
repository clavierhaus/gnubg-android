/*
 * Copyright (C) 2004 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2004-2023 the AUTHORS
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
 * $Id: relational.h,v 1.17 2019/12/24 09:02:43 plm Exp $
 */

#ifndef RELATIONAL_H
#define RELATIONAL_H

#include <stddef.h>
#include <sys/types.h>
#include "analysis.h"
#include "dbprovider.h"

#define DB_VERSION 1

extern int RelationalUpdatePlayerDetails(const char *oldName, const char *newName, const char *newNotes);
extern statcontext *relational_player_stats_get(const char *player0, const char *player1);

static inline float Ratiof(float a, int b)
{
    return b ? a / (float) b : 0.0f;
}

static inline double Ratio(double a, int b)
{
    return b ? a / (double) b : 0.0f;
}

#endif                          /* RELATIONAL_H */
