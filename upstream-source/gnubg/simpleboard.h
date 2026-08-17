/*
 * Copyright (C) 2007 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2009-2013 the AUTHORS
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
 * $Id: simpleboard.h,v 1.10 2021/02/07 20:54:21 plm Exp $
 */

#if HAVE_PANGOCAIRO

#define SIZE_1PERPAGE 250.0f
#define SIZE_2PERPAGE 150.0f

/** \brief struct to hold the colors for points, dice, checkers, etc.*/
typedef struct {
    float fill[3];
    float stroke[3];
    float text[3];
} SimpleBoardColor;

typedef struct {
    SimpleBoardColor color_checker[2];
    SimpleBoardColor color_point[2];
    SimpleBoardColor color_cube;
    matchstate ms;
    gint text_size;
    gchar *annotation;
    gchar *header;
    gdouble size;
    gdouble surface_x;
    gdouble surface_y;
    cairo_t *cr;
} SimpleBoard;

extern gint simple_board_draw(SimpleBoard * board);

extern SimpleBoard *simple_board_new(matchstate * ms, cairo_t * cr, float simple_board_size);

extern void simple_board_destroy(SimpleBoard * board);
#endif
