/*
 * Copyright (C) 1998-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 1999-2025 the AUTHORS
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
 */

/* Default movefilters */

#define MOVEFILTER_NORMAL \
  { { { .Accept = 0, .Extra = 12, .Threshold = 0.24f }, { .Accept = 0, .Extra =  0                     }, { .Accept = 0, .Extra = 0                     }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 12, .Threshold = 0.24f }, { .Accept = 0, .Extra =  6, .Threshold = 0.12f }, { .Accept = 0, .Extra = 0                     }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 16, .Threshold = 0.32f }, { .Accept = 0, .Extra =  8, .Threshold = 0.16f }, { .Accept = 0, .Extra = 4, .Threshold = 0.06f }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 16, .Threshold = 0.32f }, { .Accept = 0, .Extra = 12, .Threshold = 0.16f }, { .Accept = 0, .Extra = 6, .Threshold = 0.08f }, { .Accept = 0, .Extra = 4, .Threshold = 0.06f } } }

#define MOVEFILTER_LARGE \
  { { { .Accept = 0, .Extra = 16, .Threshold = 0.32f }, { .Accept = 0, .Extra =  0                     }, { .Accept = 0, .Extra = 0                     }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 16, .Threshold = 0.32f }, { .Accept = 0, .Extra =  8, .Threshold = 0.16f }, { .Accept = 0, .Extra = 0                     }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 24, .Threshold = 0.40f }, { .Accept = 0, .Extra = 12, .Threshold = 0.20f }, { .Accept = 0, .Extra = 6, .Threshold = 0.08f }, { .Accept = 0, .Extra = 0                     } }, \
    { { .Accept = 0, .Extra = 32, .Threshold = 0.40f }, { .Accept = 0, .Extra = 16, .Threshold = 0.20f }, { .Accept = 0, .Extra = 8, .Threshold = 0.10f }, { .Accept = 0, .Extra = 6, .Threshold = 0.08f } } }
