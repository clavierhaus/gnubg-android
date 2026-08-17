/*
 * Copyright (C) 2003-2007 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: matrix.h,v 1.13 2013/06/16 02:16:22 mdpetch Exp $
 */

typedef const float (*ConstMatrix)[4];

void setIdMatrix(float m[4][4]);
#define copyMatrix(to, from) memcpy(to, from, sizeof(float[4][4]))

void makeInverseTransposeMatrix(float m[4][4], const float v[3]);
void makeInverseRotateMatrixX(float m[4][4], float degRot);
void makeInverseRotateMatrixY(float m[4][4], float degRot);
void makeInverseRotateMatrixZ(float m[4][4], float degRot);

void mult_matrix_vec(const float mat[4][4], const float src[4], float dst[4]);
void matrixmult(float m[4][4], const float b[4][4]);
