/*
 * Copyright (C) 1996-1999 Gary Wong <gtw@gnu.org>
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
 * $Id: list.h,v 1.9 2013/06/16 02:16:24 mdpetch Exp $
 */

#ifndef LIST_H
#define LIST_H

typedef struct list {
    struct list *plPrev;
    struct list *plNext;
    void *p;
} listOLD;
/* Renamed to listOLD - use GList instead (hopefullly replace existing usage eventually */

extern int ListCreate(listOLD * pl);
/* #define ListDestroy( pl ) ( assert( ListEmpty( pl ) ) ) */

#define ListEmpty( pl ) ( (pl)->plNext == (pl) )
extern listOLD *ListInsert(listOLD * pl, void *p);
extern void ListDelete(listOLD * pl);
extern void ListDeleteAll(const listOLD * pl);

#endif
