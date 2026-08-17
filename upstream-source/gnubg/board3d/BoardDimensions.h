/*
 * Copyright (C) 2019 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: $
 */

/* All the board element sizes - based on base_unit size */

/* Widths */

/* Side edge width of bearoff trays */
#define EDGE_WIDTH base_unit

#define TRAY_WIDTH (EDGE_WIDTH * 2.0f + PIECE_HOLE)
#define BOARD_WIDTH (PIECE_HOLE * 6.0f)
#define BAR_WIDTH (PIECE_HOLE * 1.7f)
#define DICE_AREA_CLICK_WIDTH BOARD_WIDTH

#define TAKI_WIDTH .9f

#define TOTAL_WIDTH ((TRAY_WIDTH + BOARD_WIDTH) * 2.0f + BAR_WIDTH)

/* Heights */

/* Bottom + top edge */
#define EDGE_HEIGHT (base_unit * 1.5f)

#define POINT_HEIGHT (PIECE_HOLE * 6)
#define TRAY_HEIGHT (EDGE_HEIGHT + POINT_HEIGHT)
#define MID_SIDE_GAP_HEIGHT (base_unit * 3.5f)
#define DICE_AREA_HEIGHT MID_SIDE_GAP_HEIGHT
/* Vertical gap between pieces */
#define PIECE_GAP_HEIGHT (base_unit / 5.0f)

#define TOTAL_HEIGHT (TRAY_HEIGHT * 2.0f + MID_SIDE_GAP_HEIGHT)

/* Depths */

#define EDGE_DEPTH (base_unit * 1.95f)
#define BASE_DEPTH base_unit

/* Other objects */

#define BOARD_FILLET (base_unit / 3.0f)

#define DOUBLECUBE_SIZE (PIECE_HOLE * .9f)

/* Dice animation step size */
#define DICE_STEP_SIZE0 (base_unit * 1.3f)
#define DICE_STEP_SIZE1 (base_unit * 1.7f)

#define HINGE_GAP (base_unit / 13.0f)
#define HINGE_WIDTH (base_unit / 2.0f)
#define HINGE_HEIGHT (base_unit * 7.0f)

#undef ARROW_SIZE
#define ARROW_SIZE (EDGE_HEIGHT * .8f)

#define FLAG_HEIGHT (base_unit * 3)
#define FLAG_WIDTH (FLAG_HEIGHT * 1.4f)
#define FLAG_WAG (FLAG_HEIGHT * .3f)
#define FLAGPOLE_WIDTH (base_unit * .2f)
#define FLAGPOLE_HEIGHT (FLAG_HEIGHT * 2.05f)

/* Slight offset from surface - avoid using unless necessary */
#define LIFT_OFF (base_unit / 50.0f)

/* ARROW_UNIT used to draw sub-bits of arrow */
#define ARROW_UNIT (ARROW_SIZE / 4.0f)
