/*
 * Copyright (C) 1996-2001 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2004-2007 the AUTHORS
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
 * $Id: list.c,v 1.14 2021/06/09 21:11:55 plm Exp $
 */

/* No configuration used in this file
 * #include "config.h" */
#include "list.h"
#include <stddef.h>
#include <stdlib.h>

#include <glib.h>

int
ListCreate(listOLD * pl)
{

    pl->plPrev = pl->plNext = pl;
    pl->p = NULL;

    return 0;
}

listOLD *
ListInsert(listOLD * pl, void *p)
{

    listOLD *plNew = g_malloc(sizeof(listOLD));

    plNew->p = p;

    plNew->plNext = pl;
    plNew->plPrev = pl->plPrev;

    pl->plPrev = plNew;
    plNew->plPrev->plNext = plNew;

    return plNew;
}

void
ListDelete(listOLD * pl)
{

    pl->plPrev->plNext = pl->plNext;
    pl->plNext->plPrev = pl->plPrev;

    g_free(pl);
}

void
ListDeleteAll(const listOLD * pl)
{

    while (pl->plNext->p) {
        g_free(pl->plNext->p);
        ListDelete(pl->plNext);
    }
}
