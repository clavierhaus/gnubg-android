/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2003-2009 the AUTHORS
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
 * $Id: progress.h,v 1.13 2019/12/16 22:21:25 plm Exp $
 */

#ifndef PROGRESS_H
#define PROGRESS_H

#include "eval.h"
#include "rollout.h"
#include "drawboard.h"

extern void
RolloutProgressStart(const cubeinfo * pci, const int n,
                     rolloutstat aars[2][2], rolloutcontext * pes, char asz[][FORMATEDMOVESIZE], gboolean multiple,
                     void **pp);

extern void
RolloutProgress(float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                const rolloutcontext * prc,
                const cubeinfo aci[],
                unsigned int initial_game_count,
                const int iGame,
                const int iAlternative,
                const int nRank,
                const float rJsd, const int fStopped, const int fShowRanks, int fCubeRollout, void *pUserData);

extern int
RolloutProgressEnd(void **pp, gboolean destroy);

#endif                          /* PROGRESS_H */
