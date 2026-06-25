/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2003-2021 the AUTHORS
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

#ifndef FORMAT_H
#define FORMAT_H

#include "drawboard.h"

#define MAX_OUTPUT_DIGITS 6

extern int fOutputMWC, fOutputWinPC, fOutputMatchPC;
extern int fOutputDigits;
extern float rErrorRateFactor;

/* misc. output routines used by text and HTML export */

extern char *OutputPercents(const float ar[], int f);

extern char *OutputPercent(float r);

extern char *OutputMWC(float r, const cubeinfo * pci, int f);

extern char *OutputEquity(float r, const cubeinfo * pci, int f);

extern char *OutputRolloutContext(const char *szIndent, const rolloutcontext * prc);

extern char *OutputEvalContext(const evalcontext * pec, int fChequer);

extern char *OutputEquityDiff(float r1, float r2, const cubeinfo * pci);

extern char *OutputEquityScale(float r, const cubeinfo * pci, const cubeinfo * pciBase, int f);

extern char *OutputRolloutResult(const char *szIndent,
                                 char asz[][FORMATEDMOVESIZE],
                                 float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                                 float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                                 const cubeinfo aci[], int alt, int cci, int fCubeful);

extern char *OutputCubeAnalysisFull(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                                    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                                    const evalsetup * pes, const cubeinfo * pci,
                                    int fDouble, int fTake, skilltype stDouble, skilltype stTake);

extern char *OutputCubeAnalysis(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                                float aarStdDev[2][NUM_ROLLOUT_OUTPUTS], const evalsetup * pes, const cubeinfo * pci, int fTake);

extern char *OutputMoneyEquity(const float ar[], int f);

extern char *FormatCubePosition(char *sz, cubeinfo * pci);
extern void FormatCubePositions(const cubeinfo * pci, char asz[2][FORMATEDMOVESIZE]);

#endif                          /* FORMAT_H */
