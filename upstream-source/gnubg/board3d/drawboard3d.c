/*
 * Copyright (C) 2003-2021 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2004-2018 the AUTHORS
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
 * $Id: drawboard3d.c,v 1.133 2022/09/05 19:44:07 plm Exp $
 */

#include "config.h"

#include "fun3d.h"
#include "boardpos.h"
#include "gtklocdefs.h"
#include "BoardDimensions.h"
#include "ShimOGL.h"
#include "gtkboard.h"

// Basic shapes
static void
drawRect(float x, float y, float z, float w, float h, const Texture* texture)
{                               /* Draw a rectangle */
	glPushMatrix();

	glTranslatef(x + w / 2, y + h / 2, z);
	glScalef(w / 2.0f, h / 2.0f, 1.f);
	glNormal3f(0.f, 0.f, 1.f);

	if (texture) {
		float tuv = TEXTURE_SCALE / (float)texture->width;
		float repX = w * tuv;
		float repY = h * tuv;

		glBegin(GL_QUADS);
		glTexCoord2f(0.f, 0.f);
		glVertex3f(-1.f, -1.f, 0.f);
		glTexCoord2f(repX, 0.f);
		glVertex3f(1.f, -1.f, 0.f);
		glTexCoord2f(repX, repY);
		glVertex3f(1.f, 1.f, 0.f);
		glTexCoord2f(0.f, repY);
		glVertex3f(-1.f, 1.f, 0.f);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
		glVertex3f(-1.f, -1.f, 0.f);
		glVertex3f(1.f, -1.f, 0.f);
		glVertex3f(1.f, 1.f, 0.f);
		glVertex3f(-1.f, 1.f, 0.f);
		glEnd();
	}

	glPopMatrix();
}

static void
drawSplitRect(float x, float y, float z, float w, float h, const Texture* texture)
{                               /* Draw a rectangle in 2 bits */
	glPushMatrix();

	glTranslatef(x + w / 2, y + h / 2, z);
	glScalef(w / 2.0f, h / 2.0f, 1.f);
	glNormal3f(0.f, 0.f, 1.f);

	if (texture) {
		float tuv = TEXTURE_SCALE / (float)texture->width;
		float repX = w * tuv;
		float repY = h * tuv;

		glBegin(GL_QUADS);
		glTexCoord2f(0.f, 0.f);
		glVertex3f(-1.f, -1.f, 0.f);
		glTexCoord2f(repX, 0.f);
		glVertex3f(1.f, -1.f, 0.f);
		glTexCoord2f(repX, repY / 2.0f);
		glVertex3f(1.f, 0.f, 0.f);
		glTexCoord2f(0.f, repY / 2.0f);
		glVertex3f(-1.f, 0.f, 0.f);

		glTexCoord2f(0.f, repY / 2.0f);
		glVertex3f(-1.f, 0.f, 0.f);
		glTexCoord2f(repX, repY / 2.0f);
		glVertex3f(1.f, 0.f, 0.f);
		glTexCoord2f(repX, repY);
		glVertex3f(1.f, 1.f, 0.f);
		glTexCoord2f(0.f, repY);
		glVertex3f(-1.f, 1.f, 0.f);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
		glVertex3f(-1.f, -1.f, 0.f);
		glVertex3f(1.f, -1.f, 0.f);
		glVertex3f(1.f, 0.f, 0.f);
		glVertex3f(-1.f, 0.f, 0.f);

		glVertex3f(-1.f, 0.f, 0.f);
		glVertex3f(1.f, 0.f, 0.f);
		glVertex3f(1.f, 1.f, 0.f);
		glVertex3f(-1.f, 1.f, 0.f);
		glEnd();
	}

	glPopMatrix();
}

static void
drawChequeredRect(float x, float y, float z, float w, float h, int across, int down, const Texture* texture)
{                               /* Draw a rectangle split into (across x down) chequers */
	int i, j;
	float hh = h / (float)down;
	float ww = w / (float)across;

	glPushMatrix();
	glTranslatef(x, y, z);
	glNormal3f(0.f, 0.f, 1.f);

	if (texture) {
		float tuv = TEXTURE_SCALE / (float)texture->width;
		float tw = ww * tuv;
		float th = hh * tuv;
		float ty = 0.f;

		for (i = 0; i < down; i++) {
			float xx = 0, tx = 0;
			glPushMatrix();
			glTranslatef(0.f, hh * (float)i, 0.f);
			glBegin(GL_QUAD_STRIP);
			for (j = 0; j <= across; j++) {
				glTexCoord2f(tx, ty + th);
				glVertex2f(xx, hh);
				glTexCoord2f(tx, ty);
				glVertex2f(xx, 0.f);
				xx += ww;
				tx += tw;
			}
			ty += th;
			glEnd();
			glPopMatrix();
		}
	}
	else {
		for (i = 0; i < down; i++) {
			float xx = 0;
			glPushMatrix();
			glTranslatef(0.f, hh * (float)i, 0.f);
			glBegin(GL_QUAD_STRIP);
			for (j = 0; j <= across; j++) {
				glVertex2f(xx, hh);
				glVertex2f(xx, 0.f);
				xx += ww;
			}
			glEnd();
			glPopMatrix();
		}
	}
	glPopMatrix();
}

static void
QuarterCylinder(float radius, float len, unsigned int accuracy, const Texture* texture)
{
	unsigned int i;
	float d;
	float dInc = 0;

	/* texture unit value */
	float tuv;
	if (texture) {
		float st = sinf((2 * F_PI) / (float)accuracy) * radius;
		float ct = (cosf((2 * F_PI) / (float)accuracy) - 1.0f) * radius;
		dInc = sqrtf(st * st + ct * ct);
		tuv = TEXTURE_SCALE / (float)texture->width;
	}
	else
		tuv = 0.0f;

	d = 0;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < accuracy / 4 + 1; i++) {
		float sar, car;
		float angle = ((float)i * 2.f * F_PI) / (float)accuracy;

		glNormal3f(sinf(angle), 0.f, cosf(angle));

		sar = sinf(angle) * radius;
		car = cosf(angle) * radius;

		if (tuv != 0.0f)
			glTexCoord2f(len * tuv, d * tuv);
		glVertex3f(sar, len, car);

		if (tuv != 0.0f) {
			glTexCoord2f(0.f, d * tuv);
			d -= dInc;
		}
		glVertex3f(sar, 0.f, car);
	}
	glEnd();
}

static void
QuarterCylinderSplayedRev(float radius, float len, unsigned int accuracy, const Texture* texture)
{
	unsigned int i;
	float d;
	float dInc = 0;

	/* texture unit value */
	float tuv;
	if (texture) {
		float st = sinf((2 * F_PI) / (float)accuracy) * radius;
		float ct = (cosf((2 * F_PI) / (float)accuracy) - 1.0f) * radius;
		dInc = sqrtf(st * st + ct * ct);
		tuv = TEXTURE_SCALE / (float)texture->width;
	}
	else
		tuv = 0;

	d = 0;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < accuracy / 4 + 1; i++) {
		float sar, car;
		float angle = ((float)i * 2.f * F_PI) / (float)accuracy;

		glNormal3f(sinf(angle), 0.f, cosf(angle));

		sar = sinf(angle) * radius;
		car = cosf(angle) * radius;

		if (tuv != 0.0f)
			glTexCoord2f((len + car) * tuv, d * tuv);
		glVertex3f(sar, len + car, car);

		if (tuv != 0.0f) {
			glTexCoord2f(-car * tuv, d * tuv);
			d -= dInc;
		}
		glVertex3f(sar, -car, car);
	}
	glEnd();
}

static void
QuarterCylinderSplayed(float radius, float len, unsigned int accuracy, const Texture* texture)
{
	unsigned int i;
	float d;
	float dInc = 0;

	/* texture unit value */
	float tuv;
	if (texture) {
		float st = sinf((2 * F_PI) / (float)accuracy) * radius;
		float ct = (cosf((2 * F_PI) / (float)accuracy) - 1.0f) * radius;
		dInc = sqrtf(st * st + ct * ct);
		tuv = TEXTURE_SCALE / (float)texture->width;
	}
	else
		tuv = 0;

	d = 0;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < accuracy / 4 + 1; i++) {
		float sar, car;
		float angle = ((float)i * 2.f * F_PI) / (float)accuracy;

		glNormal3f(sinf(angle), 0.f, cosf(angle));

		sar = sinf(angle) * radius;
		car = cosf(angle) * radius;

		if (tuv != 0.0f)
			glTexCoord2f((len - car) * tuv, d * tuv);
		glVertex3f(sar, len - car, car);

		if (tuv != 0.0f) {
			glTexCoord2f(car * tuv, d * tuv);
			d -= dInc;
		}
		glVertex3f(sar, car, car);
	}
	glEnd();
}

static void
drawCornerEigth(const EigthPoints* eigthPoints, float radius, const Texture* texture)
{
	unsigned int i;
	int j;

	/* texture unit value */
	float tuv;
	if (texture)
		tuv = TEXTURE_SCALE / (float)texture->width;
	else
		tuv = 0.0f;

	for (i = 0; i < eigthPoints->accuracy / 4; i++) {
		unsigned int ns = (eigthPoints->accuracy / 4) - (i + 1);

		glBegin(GL_TRIANGLE_STRIP);
		glNormal3f(eigthPoints->points[i][ns + 1][0] / radius, eigthPoints->points[i][ns + 1][1] / radius,
			eigthPoints->points[i][ns + 1][2] / radius);
		if (tuv != 0.0f)
			glTexCoord2f(eigthPoints->points[i][ns + 1][0] * tuv, eigthPoints->points[i][ns + 1][1] * tuv);     // Here and below - approximate texture co-ords
		glVertex3f(eigthPoints->points[i][ns + 1][0], eigthPoints->points[i][ns + 1][1],
			eigthPoints->points[i][ns + 1][2]);
		for (j = (int)ns; j >= 0; j--) {
			glNormal3f(eigthPoints->points[i + 1][j][0] / radius, eigthPoints->points[i + 1][j][1] / radius,
				eigthPoints->points[i + 1][j][2] / radius);
			if (tuv != 0.0f)
				glTexCoord2f(eigthPoints->points[i + 1][j][0] * tuv, eigthPoints->points[i + 1][j][1] * tuv);
			glVertex3f(eigthPoints->points[i + 1][j][0], eigthPoints->points[i + 1][j][1],
				eigthPoints->points[i + 1][j][2]);
			glNormal3f(eigthPoints->points[i][j][0] / radius, eigthPoints->points[i][j][1] / radius,
				eigthPoints->points[i][j][2] / radius);
			if (tuv != 0.0f)
				glTexCoord2f(eigthPoints->points[i][j][0] * tuv, eigthPoints->points[i][j][1] * tuv);
			glVertex3f(eigthPoints->points[i][j][0], eigthPoints->points[i][j][1], eigthPoints->points[i][j][2]);
		}
		glEnd();
	}
}

static void
drawBox(int type, float x, float y, float z, float w, float h, float d, const Texture* texture)
{                               /* Draw a box with normals and optional textures */
	float normX, normY, normZ;
	float w2 = w / 2.0f, h2 = h / 2.0f, d2 = d / 2.0f;

	glPushMatrix();
	glTranslatef(x + w2, y + h2, z + d2);
	glScalef(w2, h2, d2);

	/* Scale normals */
	normX = w2;
	normY = h2;
	normZ = d2;

	glBegin(GL_QUADS);

	if (texture) {
		float repX = (w * TEXTURE_SCALE) / (float)texture->width;
		float repY = (h * TEXTURE_SCALE) / (float)texture->height;

		/* Front Face */
		glNormal3f(0.f, 0.f, normZ);
		if (type & BOX_SPLITTOP) {
			glTexCoord2f(0.f, 0.f);
			glVertex3f(-1.f, -1.f, 1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(1.f, -1.f, 1.f);
			glTexCoord2f(repX, repY / 2.0f);
			glVertex3f(1.f, 0.f, 1.f);
			glTexCoord2f(0.f, repY / 2.0f);
			glVertex3f(-1.f, 0.f, 1.f);

			glTexCoord2f(0.f, repY / 2.0f);
			glVertex3f(-1.f, 0.f, 1.f);
			glTexCoord2f(repX, repY / 2.0f);
			glVertex3f(1.f, 0.f, 1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(1.f, 1.f, 1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(-1.f, 1.f, 1.f);
		}
		else if (type & BOX_SPLITWIDTH) {
			glTexCoord2f(0.f, 0.f);
			glVertex3f(-1.f, -1.f, 1.f);
			glTexCoord2f(repX / 2.0f, 0.f);
			glVertex3f(0.f, -1.f, 1.f);
			glTexCoord2f(repX / 2.0f, repY);
			glVertex3f(0.f, 1.f, 1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(-1.f, 1.f, 1.f);

			glTexCoord2f(repX / 2.0f, 0.f);
			glVertex3f(0.f, -1.f, 1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(1.f, -1.f, 1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(1.f, 1.f, 1.f);
			glTexCoord2f(repX / 2.0f, repY);
			glVertex3f(0.f, 1.f, 1.f);
		}
		else {
			glTexCoord2f(0.f, 0.f);
			glVertex3f(-1.f, -1.f, 1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(1.f, -1.f, 1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(1.f, 1.f, 1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(-1.f, 1.f, 1.f);
		}
		if (!(type & BOX_NOENDS)) {
			/* Top Face */
			glNormal3f(0.f, normY, 0.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(-1.f, 1.f, -1.f);
			glTexCoord2f(0.f, 0.f);
			glVertex3f(-1.f, 1.f, 1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(1.f, 1.f, 1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(1.f, 1.f, -1.f);
			/* Bottom Face */
			glNormal3f(0.f, -normY, 0.f);
			glTexCoord2f(repX, repY);
			glVertex3f(-1.f, -1.f, -1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(1.f, -1.f, -1.f);
			glTexCoord2f(0.f, 0.f);
			glVertex3f(1.f, -1.f, 1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(-1.f, -1.f, 1.f);
		}
		if (!(type & BOX_NOSIDES)) {
			/* Right face */
			glNormal3f(normX, 0.f, 0.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(1.f, -1.f, -1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(1.f, 1.f, -1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(1.f, 1.f, 1.f);
			glTexCoord2f(0.f, 0.f);
			glVertex3f(1.f, -1.f, 1.f);
			/* Left Face */
			glNormal3f(-normX, 0.f, 0.f);
			glTexCoord2f(0.f, 0.f);
			glVertex3f(-1.f, -1.f, -1.f);
			glTexCoord2f(repX, 0.f);
			glVertex3f(-1.f, -1.f, 1.f);
			glTexCoord2f(repX, repY);
			glVertex3f(-1.f, 1.f, 1.f);
			glTexCoord2f(0.f, repY);
			glVertex3f(-1.f, 1.f, -1.f);
		}
	}
	else {                    /* no texture co-ords */
	 /* Front Face */
		glNormal3f(0.f, 0.f, normZ);
		if (type & BOX_SPLITTOP) {
			glVertex3f(-1.f, -1.f, 1.f);
			glVertex3f(1.f, -1.f, 1.f);
			glVertex3f(1.f, 0.f, 1.f);
			glVertex3f(-1.f, 0.f, 1.f);

			glVertex3f(-1.f, 0.f, 1.f);
			glVertex3f(1.f, 0.f, 1.f);
			glVertex3f(1.f, 1.f, 1.f);
			glVertex3f(-1.f, 1.f, 1.f);
		}
		else if (type & BOX_SPLITWIDTH) {
			glVertex3f(-1.f, -1.f, 1.f);
			glVertex3f(0.f, -1.f, 1.f);
			glVertex3f(0.f, 1.f, 1.f);
			glVertex3f(-1.f, 1.f, 1.f);

			glVertex3f(0.f, -1.f, 1.f);
			glVertex3f(1.f, -1.f, 1.f);
			glVertex3f(1.f, 1.f, 1.f);
			glVertex3f(0.f, 1.f, 1.f);
		}
		else {
			glVertex3f(-1.f, -1.f, 1.f);
			glVertex3f(1.f, -1.f, 1.f);
			glVertex3f(1.f, 1.f, 1.f);
			glVertex3f(-1.f, 1.f, 1.f);
		}

		if (!(type & BOX_NOENDS)) {
			/* Top Face */
			glNormal3f(0.f, normY, 0.f);
			glVertex3f(-1.f, 1.f, -1.f);
			glVertex3f(-1.f, 1.f, 1.f);
			glVertex3f(1.f, 1.f, 1.f);
			glVertex3f(1.f, 1.f, -1.f);
			/* Bottom Face */
			glNormal3f(0.f, -normY, 0.f);
			glVertex3f(-1.f, -1.f, -1.f);
			glVertex3f(1.f, -1.f, -1.f);
			glVertex3f(1.f, -1.f, 1.f);
			glVertex3f(-1.f, -1.f, 1.f);
		}
		if (!(type & BOX_NOSIDES)) {
			/* Right face */
			glNormal3f(normX, 0.f, 0.f);
			glVertex3f(1.f, -1.f, -1.f);
			glVertex3f(1.f, 1.f, -1.f);
			glVertex3f(1.f, 1.f, 1.f);
			glVertex3f(1.f, -1.f, 1.f);
			/* Left Face */
			glNormal3f(-normX, 0.f, 0.f);
			glVertex3f(-1.f, -1.f, -1.f);
			glVertex3f(-1.f, -1.f, 1.f);
			glVertex3f(-1.f, 1.f, 1.f);
			glVertex3f(-1.f, 1.f, -1.f);
		}
	}
	glEnd();
	glPopMatrix();
}

static void
circle(float radius, float height, unsigned int accuracy)
{                               /* Draw a disc in current z plane */
	unsigned int i;
	float angle, step;

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.f, 0.f, height);
	for (i = 0; i <= accuracy; i++) {
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle -= step;
	}
	glEnd();
}

static void
circleSloped(float radius, float startHeight, float endHeight, unsigned int accuracy)
{                               /* Draw a disc in sloping z plane */
	unsigned int i;
	float angle, step;

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.f, 0.f, startHeight);
	for (i = 0; i <= accuracy; i++) {
		float height = ((cosf(angle) + 1) / 2) * (endHeight - startHeight);
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, startHeight + height);
		angle -= step;
	}
	glEnd();
}

static void
circleRev(float radius, float height, unsigned int accuracy)
{                               /* Draw a disc with reverse winding in current z plane */
	unsigned int i;
	float angle, step;

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.f, 0.f, height);
	for (i = 0; i <= accuracy; i++) {
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle += step;
	}
	glEnd();
}

static void
circleTex(float radius, float height, unsigned int accuracy, const Texture* texture)
{                               /* Draw a disc in current z plane with a texture */
	unsigned int i;
	float angle, step;

	if (!texture) {
		circle(radius, height, accuracy);
		return;
	}

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(.5f, .5f);
	glVertex3f(0.f, 0.f, height);
	for (i = 0; i <= accuracy; i++) {
		glTexCoord2f((sinf(angle) * radius + radius) / (radius * 2), (cosf(angle) * radius + radius) / (radius * 2));
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle -= step;
	}
	glEnd();
}

static void
circleRevTex(float radius, float height, unsigned int accuracy, const Texture* texture)
{                               /* Draw a disc with reverse winding in current z plane with a texture */
	unsigned int i;
	float angle, step;

	if (!texture) {
		circleRev(radius, height, accuracy);
		return;
	}

	step = (2 * F_PI) / (float)accuracy;
	angle = 0;
	glNormal3f(0.f, 0.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(.5f, .5f);
	glVertex3f(0.f, 0.f, height);
	for (i = 0; i <= accuracy; i++) {
		glTexCoord2f((sinf(angle) * radius + radius) / (radius * 2), (cosf(angle) * radius + radius) / (radius * 2));
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);
		angle += step;
	}
	glEnd();
}

static void
cylinder(float radius, float height, unsigned int accuracy, const Texture* texture)
{
	unsigned int i;
	float angle = 0;
	float circum = F_PI * radius * 2 / (float)(accuracy + 1);
	float step = (2 * F_PI) / (float)accuracy;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < accuracy + 1; i++) {
		glNormal3f(sinf(angle), cosf(angle), 0.f);
		if (texture)
			glTexCoord2f(circum * (float)i / (radius * 2), 0.f);
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, 0.f);

		if (texture)
			glTexCoord2f(circum * (float)i / (radius * 2), height / (radius * 2));
		glVertex3f(sinf(angle) * radius, cosf(angle) * radius, height);

		angle += step;
	}
	glEnd();
}

#define CACHE_SIZE	240
static void
gluCylinderMine(GLfloat baseRadius, GLfloat topRadius, GLfloat height, int slices, int stacks, int textureCoords)
{
	int i, j;
	GLfloat sinCache[CACHE_SIZE];
	GLfloat cosCache[CACHE_SIZE];
	GLfloat sinCache3[CACHE_SIZE];
	GLfloat cosCache3[CACHE_SIZE];
	GLfloat angle;
	GLfloat zLow, zHigh;
	GLfloat length;
	GLfloat deltaRadius;
	GLfloat zNormal;
	GLfloat xyNormalRatio;
	GLfloat radiusLow, radiusHigh;

	if (slices >= CACHE_SIZE)
		slices = CACHE_SIZE - 1;

	if (slices < 2 || stacks < 1 || baseRadius < 0.0 || topRadius < 0.0 || height < 0.0) {
		return;
	}

	/* Compute length (needed for normal calculations) */
	deltaRadius = baseRadius - topRadius;
	length = sqrtf(deltaRadius * deltaRadius + height * height);
	if (length == 0.0) {
		return;
	}

	/* Cache is the vertex locations cache */
	/* Cache3 is the various normals for the faces */

	zNormal = deltaRadius / length;
	xyNormalRatio = height / length;

	for (i = 0; i < slices; i++) {
		angle = 2 * F_PI * (float)i / (float)slices;
		sinCache[i] = sinf(angle);
		cosCache[i] = cosf(angle);
	}

	for (i = 0; i < slices; i++) {
		angle = 2 * F_PI * ((float)i - 0.5f) / (float)slices;
		sinCache3[i] = xyNormalRatio * sinf(angle);
		cosCache3[i] = xyNormalRatio * cosf(angle);
	}

	sinCache[slices] = sinCache[0];
	cosCache[slices] = cosCache[0];
	sinCache3[slices] = sinCache3[0];
	cosCache3[slices] = cosCache3[0];

	/* Note:
	 ** An argument could be made for using a TRIANGLE_FAN for the end
	 ** of the cylinder of either radii is 0.0 (a cone).  However, a
	 ** TRIANGLE_FAN would not work in smooth shading mode (the common
	 ** case) because the normal for the apex is different for every
	 ** triangle (and TRIANGLE_FAN doesn't let me respecify that normal).
	 ** Now, my choice is GL_TRIANGLES, or leave the GL_QUAD_STRIP and
	 ** just let the GL trivially reject one of the two triangles of the
	 ** QUAD.  GL_QUAD_STRIP is probably faster, so I will leave this code
	 ** alone.
	 */
	for (j = 0; j < stacks; j++) {
		zLow = (float)j * height / (float)stacks;
		zHigh = (float)(j + 1) * height / (float)stacks;
		radiusLow = baseRadius - deltaRadius * ((float)j / (float)stacks);
		radiusHigh = baseRadius - deltaRadius * ((float)(j + 1) / (float)stacks);

		glBegin(GL_QUAD_STRIP);
		for (i = 0; i <= slices; i++) {
			glNormal3f(sinCache3[i], cosCache3[i], zNormal);
			if (textureCoords)
				glTexCoord2f(1 - (float)i / (float)slices, (float)j / (float)stacks);

			glVertex3f(radiusLow * sinCache[i], radiusLow * cosCache[i], zLow);
			if (textureCoords)
				glTexCoord2f(1 - (float)i / (float)slices, (float)(j + 1) / (float)stacks);

			glVertex3f(radiusHigh * sinCache[i], radiusHigh * cosCache[i], zHigh);
		}
		glEnd();
	}
}

static void
gluDiskMine(GLfloat innerRadius, GLfloat outerRadius, int slices, int loops, int textureCoords)
{
	GLfloat startAngle = 0.0f;
	GLfloat sweepAngle = 360.0f;

	int i, j;
	GLfloat sinCache[CACHE_SIZE];
	GLfloat cosCache[CACHE_SIZE];
	GLfloat angle;
	GLfloat deltaRadius;
	GLfloat radiusLow, radiusHigh;
	GLfloat texLow = 0.0, texHigh = 0.0;
	GLfloat angleOffset;
	int finish;

	if (slices >= CACHE_SIZE)
		slices = CACHE_SIZE - 1;
	if (slices < 2 || loops < 1 || outerRadius <= 0.0 || innerRadius < 0.0 || innerRadius > outerRadius) {
		return;
	}

	if (sweepAngle < -360.0)
		sweepAngle = 360.0;
	if (sweepAngle > 360.0)
		sweepAngle = 360.0;
	if (sweepAngle < 0) {
		startAngle += sweepAngle;
		sweepAngle = -sweepAngle;
	}

	/* Compute length (needed for normal calculations) */
	deltaRadius = outerRadius - innerRadius;

	/* Cache is the vertex locations cache */

	angleOffset = startAngle / 180.0f * F_PI;
	for (i = 0; i <= slices; i++) {
		angle = angleOffset + ((F_PI * sweepAngle) / 180.0f) * (float)i / (float)slices;
		sinCache[i] = sinf(angle);
		cosCache[i] = cosf(angle);
	}

	if (sweepAngle == 360.0) {
		sinCache[slices] = sinCache[0];
		cosCache[slices] = cosCache[0];
	}

	glNormal3f(0.0, 0.0, 1.0);

	if (innerRadius == 0.0) {
		finish = loops - 1;
		/* Triangle strip for inner polygons */
		glBegin(GL_TRIANGLE_FAN);
		if (textureCoords)
			glTexCoord2f(0.5, 0.5);

		glVertex3f(0.0, 0.0, 0.0);
		radiusLow = outerRadius - deltaRadius * ((float)(loops - 1) / (float)loops);
		if (textureCoords)
			texLow = radiusLow / outerRadius / 2;

		for (i = slices; i >= 0; i--) {
			if (textureCoords)
				glTexCoord2f(texLow * sinCache[i] + 0.5f, texLow * cosCache[i] + 0.5f);

			glVertex3f(radiusLow * sinCache[i], radiusLow * cosCache[i], 0.0);
		}

		glEnd();
	}
	else {
		finish = loops;
	}
	for (j = 0; j < finish; j++) {
		radiusLow = outerRadius - deltaRadius * ((float)j / (float)loops);
		radiusHigh = outerRadius - deltaRadius * ((float)(j + 1) / (float)loops);
		if (textureCoords) {
			texLow = radiusLow / outerRadius / 2;
			texHigh = radiusHigh / outerRadius / 2;
		}

		glBegin(GL_QUAD_STRIP);
		for (i = 0; i <= slices; i++) {
			if (textureCoords)
				glTexCoord2f(texLow * sinCache[i] + 0.5f, texLow * cosCache[i] + 0.5f);

			glVertex3f(radiusLow * sinCache[i], radiusLow * cosCache[i], 0.0);

			if (textureCoords)
				glTexCoord2f(texHigh * sinCache[i] + 0.5f, texHigh * cosCache[i] + 0.5f);

			glVertex3f(radiusHigh * sinCache[i], radiusHigh * cosCache[i], 0.0);
		}
		glEnd();
	}
}

#if 0
static void
drawCube(float size)
{                               /* Draw a simple cube */
	glPushMatrix();
	glScalef(size / 2.0f, size / 2.0f, size / 2.0f);

	glBegin(GL_QUADS);
	/* Front Face */
	glVertex3f(-1.f, -1.f, 1.f);
	glVertex3f(1.f, -1.f, 1.f);
	glVertex3f(1.f, 1.f, 1.f);
	glVertex3f(-1.f, 1.f, 1.f);
	/* Top Face */
	glVertex3f(-1.f, 1.f, -1.f);
	glVertex3f(-1.f, 1.f, 1.f);
	glVertex3f(1.f, 1.f, 1.f);
	glVertex3f(1.f, 1.f, -1.f);
	/* Bottom Face */
	glVertex3f(-1.f, -1.f, -1.f);
	glVertex3f(1.f, -1.f, -1.f);
	glVertex3f(1.f, -1.f, 1.f);
	glVertex3f(-1.f, -1.f, 1.f);
	/* Right face */
	glVertex3f(1.f, -1.f, -1.f);
	glVertex3f(1.f, 1.f, -1.f);
	glVertex3f(1.f, 1.f, 1.f);
	glVertex3f(1.f, -1.f, 1.f);
	/* Left Face */
	glVertex3f(-1.f, -1.f, -1.f);
	glVertex3f(-1.f, -1.f, 1.f);
	glVertex3f(-1.f, 1.f, 1.f);
	glVertex3f(-1.f, 1.f, -1.f);
	glEnd();

	glPopMatrix();
}
#endif

/////////////////////
// Main drawing code

extern int LogCube(unsigned int n);

float
getBoardWidth(void)
{
    return TOTAL_WIDTH;
}

float
getBoardHeight(void)
{
    return TOTAL_HEIGHT;
}

float
getDiceSize(const renderdata * prd)
{
    return prd->diceSize * base_unit;
}

void
Tidy3dObjects(BoardData3d * bd3d, const renderdata * UNUSED(prd))
{
	if (widget3dValid)
	{
		bd3d->shadowsInitialised = FALSE;

		FreeNumberFont(&bd3d->numberFont);
		FreeNumberFont(&bd3d->cubeFont);

		freeEigthPoints(&bd3d->boardPoints);

		TidyShadows(bd3d);
		ClearTextures(bd3d);
		DeleteTextureList();
	}
}

static void
drawPoints(const renderdata* prd, int side)
{
	/* texture unit value */
	float tuv;

	if (prd->PointMat[side].pTexture)
		tuv = TEXTURE_SCALE / (float)prd->PointMat[side].pTexture->width;
	else
		tuv = 0;

	for (int point = 0; point < 6; point++)
		drawPoint(prd, tuv, (point / 2) * 2 + side, point % 2, 0);
}

static void
drawBoardBase(const renderdata* prd)
{
	if (prd->bgInTrays)
		drawChequeredRect(EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, BOARD_WIDTH + TRAY_WIDTH - EDGE_WIDTH,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, prd->acrossCheq, prd->downCheq, prd->BaseMat.pTexture);
	else
		drawChequeredRect(TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH, BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT * 2,
			prd->acrossCheq, prd->downCheq, prd->BaseMat.pTexture);
}

static void
drawRectTextureMatched(float x, float y, float z, float w, float h, const Texture* texture)
{
	if (texture)
	{
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		float tuv = TEXTURE_SCALE / (float)texture->width;
		glTranslatef(x * tuv, y * tuv, 0.f);
		glMatrixMode(GL_MODELVIEW);
	}
	drawRect(x, y, z, w, h, texture);

	if (texture)
	{
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
}

void
drawBackground(const renderdata* prd, const float* bd3dbackGroundPos, const float* bd3dbackGroundSize)
{
	glPushMatrix();
	glLoadIdentity();

	//Bottom
	drawRectTextureMatched(bd3dbackGroundPos[0], bd3dbackGroundPos[1], 0.f, bd3dbackGroundSize[0], -bd3dbackGroundPos[1], prd->BackGroundMat.pTexture);
	//Left
	drawRectTextureMatched(bd3dbackGroundPos[0], 0.f, 0.f, -bd3dbackGroundPos[0], TOTAL_HEIGHT, prd->BackGroundMat.pTexture);
	//Top
	drawRectTextureMatched(bd3dbackGroundPos[0], TOTAL_HEIGHT, 0.f, bd3dbackGroundSize[0], bd3dbackGroundSize[1] - TOTAL_HEIGHT + bd3dbackGroundPos[1], prd->BackGroundMat.pTexture);
	//Right
	drawRectTextureMatched(TOTAL_WIDTH, 0.f, 0.f, -bd3dbackGroundPos[0], TOTAL_HEIGHT, prd->BackGroundMat.pTexture);

	glPopMatrix();
}

static void
preDrawPiece0(const renderdata* prd, int display, PieceTextureType ptt)
{
	unsigned int i, j;
	float angle2, step;

	float radius = PIECE_HOLE / 2.0f;
	float discradius = radius * 0.8f;
	float lip = radius - discradius;
	float height = PIECE_DEPTH - 2 * lip;
	float*** p;
	float*** n;

	step = (2 * F_PI) /(float) prd->curveAccuracy;

	/* Draw top/bottom of piece */
	if (display) {
		if (ptt != PTT_BOTTOM)
			circleTex(discradius, PIECE_DEPTH, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
		if (ptt == PTT_TOP)
			return;

		circleRevTex(discradius, 0.f, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
	}
	else {
		circleSloped(radius, 0.f, PIECE_DEPTH, prd->curveAccuracy);
		return;
	}
	/* Draw side of piece */
	glPushMatrix();
	glTranslatef(0.f, 0.f, lip);
	cylinder(radius, height, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
	glPopMatrix();

	/* Draw edges of piece */
	p = Alloc3d(prd->curveAccuracy + 1, prd->curveAccuracy / 4 + 1, 3);
	n = Alloc3d(prd->curveAccuracy + 1, prd->curveAccuracy / 4 + 1, 3);

	angle2 = 0;
	for (j = 0; j <= prd->curveAccuracy / 4; j++) {
		float latitude = sinf(angle2) * lip;
		float new_radius = Dist2d(lip, latitude);
		float angle = 0;

		for (i = 0; i < prd->curveAccuracy; i++) {
			n[i][j][0] = sinf(angle) * new_radius;
			p[i][j][0] = sinf(angle) * (discradius + new_radius);
			n[i][j][1] = cosf(angle) * new_radius;
			p[i][j][1] = cosf(angle) * (discradius + new_radius);
			p[i][j][2] = latitude + lip + height;
			n[i][j][2] = latitude;

			angle += step;
		}
		p[i][j][0] = p[0][j][0];
		p[i][j][1] = p[0][j][1];
		p[i][j][2] = p[0][j][2];
		n[i][j][0] = n[0][j][0];
		n[i][j][1] = n[0][j][1];
		n[i][j][2] = n[0][j][2];

		angle2 += step;
	}

	for (j = 0; j < prd->curveAccuracy / 4; j++) {
		glBegin(GL_QUAD_STRIP);
		for (i = 0; i < prd->curveAccuracy + 1; i++) {
			glNormal3f((n[i][j][0]) / lip, (n[i][j][1]) / lip, n[i][j][2] / lip);
			if (prd->ChequerMat[0].pTexture)
				glTexCoord2f((p[i][j][0] + discradius) / (discradius * 2),
				(p[i][j][1] + discradius) / (discradius * 2));
			glVertex3f(p[i][j][0], p[i][j][1], p[i][j][2]);

			glNormal3f((n[i][j + 1][0]) / lip, (n[i][j + 1][1]) / lip, n[i][j + 1][2] / lip);
			if (prd->ChequerMat[0].pTexture)
				glTexCoord2f((p[i][j + 1][0] + discradius) / (discradius * 2),
				(p[i][j + 1][1] + discradius) / (discradius * 2));
			glVertex3f(p[i][j + 1][0], p[i][j + 1][1], p[i][j + 1][2]);
		}
		glEnd();

		glBegin(GL_QUAD_STRIP);
		for (i = 0; i < prd->curveAccuracy + 1; i++) {
			glNormal3f((n[i][j + 1][0]) / lip, (n[i][j + 1][1]) / lip, n[i][j + 1][2] / lip);
			if (prd->ChequerMat[0].pTexture)
				glTexCoord2f((p[i][j + 1][0] + discradius) / (discradius * 2),
				(p[i][j + 1][1] + discradius) / (discradius * 2));
			glVertex3f(p[i][j + 1][0], p[i][j + 1][1], PIECE_DEPTH - p[i][j + 1][2]);

			glNormal3f((n[i][j][0]) / lip, (n[i][j][1]) / lip, n[i][j][2] / lip);
			if (prd->ChequerMat[0].pTexture)
				glTexCoord2f((p[i][j][0] + discradius) / (discradius * 2),
				(p[i][j][1] + discradius) / (discradius * 2));
			glVertex3f(p[i][j][0], p[i][j][1], PIECE_DEPTH - p[i][j][2]);
		}
		glEnd();
	}

	Free3d(p, prd->curveAccuracy + 1, prd->curveAccuracy / 4 + 1);
	Free3d(n, prd->curveAccuracy + 1, prd->curveAccuracy / 4 + 1);
}

static void
preDrawPiece1(const renderdata* prd, int display, PieceTextureType ptt)
{
	float pieceRad = PIECE_HOLE / 2.0f;

	/* Draw top/bottom of piece */
	if (display) {
		if (ptt != PTT_BOTTOM)
			circleTex(pieceRad, PIECE_DEPTH, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
		if (ptt == PTT_TOP)
			return;

		circleRevTex(pieceRad, 0.f, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
	}
	else {
		circleSloped(pieceRad, 0.f, PIECE_DEPTH, prd->curveAccuracy);
		return;
	}

	/* Edge of piece */
	cylinder(pieceRad, PIECE_DEPTH, prd->curveAccuracy, prd->ChequerMat[0].pTexture);
}

static void
preRenderPiece(const renderdata* prd, int display, PieceTextureType ptt)
{
	switch (prd->pieceType) {
	case PT_ROUNDED:
		preDrawPiece0(prd, display, ptt);
		break;
	case PT_FLAT:
		preDrawPiece1(prd, display, ptt);
		break;
	default:
		g_assert_not_reached();
	}
}

static void
preDrawPiece(const BoardData3d* UNUSED(bd3d), const renderdata* prd, PieceTextureType ptt)
{
	preRenderPiece(prd, TRUE, ptt);
}

static void
UnitNormal(float x, float y, float z)
{
	/* Calculate the length of the vector */
	float length = sqrtf((x * x) + (y * y) + (z * z));

	/* Dividing each element by the length will result in a unit normal vector */
	glNormal3f(x / length, y / length, z / length);
}

static void
renderDice(const renderdata* prd)
{
	unsigned int ns, lns;
	unsigned int i, j;
	int c;
	float lat_angle;
	float lat_step;
	float radius;
	float step = 0;
	float size = getDiceSize(prd);

	unsigned int corner_steps = (prd->curveAccuracy / 4) + 1;
	float*** corner_points = Alloc3d(corner_steps, corner_steps, 3);

	radius = size / 2.0f;
	step = (2 * F_PI) / (float)prd->curveAccuracy;

	glPushMatrix();

	/* Draw 6 faces */
	for (c = 0; c < 6; c++) {
		circle(radius, radius, prd->curveAccuracy);

		if (c % 2 == 0)
			glRotatef(-90.f, 0.f, 1.f, 0.f);
		else
			glRotatef(90.f, 1.f, 0.f, 0.f);
	}

	lat_angle = 0;
	lns = (prd->curveAccuracy / 4);
	lat_step = F_PI_2 / (float)lns;

	/* Calculate corner points */
	for (i = 0; i < lns + 1; i++) {
		float angle = 0.0f;

		ns = (prd->curveAccuracy / 4) - i;
		if (ns > 0)
			step = (F_PI_2 - lat_angle) / (float)ns;

		for (j = 0; j <= ns; j++) {
			corner_points[i][j][0] = cosf(lat_angle) * radius;
			corner_points[i][j][1] = cosf(angle) * radius;
			corner_points[i][j][2] = sinf(angle + lat_angle) * radius;

			angle += step;
		}
		lat_angle += lat_step;
	}

	/* Draw 8 corners */
	for (c = 0; c < 8; c++) {
		glPushMatrix();

		glRotatef((float)c * 90, 0.f, 0.f, 1.f);

		for (i = 0; i < prd->curveAccuracy / 4; i++) {
			ns = (prd->curveAccuracy / 4) - (i + 1);

			glBegin(GL_TRIANGLE_STRIP);
			UnitNormal(corner_points[i][0][0], corner_points[i][0][1], corner_points[i][0][2]);
			glVertex3f(corner_points[i][0][0], corner_points[i][0][1], corner_points[i][0][2]);
			for (j = 0; j <= ns; j++) {
				UnitNormal(corner_points[i + 1][j][0], corner_points[i + 1][j][1], corner_points[i + 1][j][2]);
				glVertex3f(corner_points[i + 1][j][0], corner_points[i + 1][j][1], corner_points[i + 1][j][2]);
				UnitNormal(corner_points[i][j + 1][0], corner_points[i][j + 1][1], corner_points[i][j + 1][2]);
				glVertex3f(corner_points[i][j + 1][0], corner_points[i][j + 1][1], corner_points[i][j + 1][2]);
			}
			glEnd();
		}

		glPopMatrix();
		if (c == 3)
			glRotatef(180.f, 1.f, 0.f, 0.f);
	}

	glPopMatrix();

	Free3d(corner_points, corner_steps, corner_steps);
}

static void
renderDot(const renderdata* prd)
{
	circle(1, 0, prd->curveAccuracy);
}

static void
drawSimpleRect(void)
{
	glBegin(GL_QUADS);
	glVertex3f(-1.f, -1.f, 0.f);
	glVertex3f(1.f, -1.f, 0.f);
	glVertex3f(1.f, 1.f, 0.f);
	glVertex3f(-1.f, 1.f, 0.f);
	glEnd();
}

static void
renderCube(const renderdata* prd, float size)
{
	int i, c;
	EigthPoints corner_points;
	float radius = size / 7.0f;
	float ds = (size * 5.0f / 7.0f);
	float hds = (ds / 2);

	glPushMatrix();

	/* Draw 6 faces */
	for (c = 0; c < 6; c++) {
		glPushMatrix();
		glTranslatef(0.f, 0.f, hds + radius);

		glNormal3f(0.f, 0.f, 1.f);

		glBegin(GL_QUADS);
		glVertex3f(-hds, -hds, 0.f);
		glVertex3f(hds, -hds, 0.f);
		glVertex3f(hds, hds, 0.f);
		glVertex3f(-hds, hds, 0.f);
		glEnd();

		/* Draw 12 edges */
		for (i = 0; i < 2; i++) {
			glPushMatrix();
			glRotatef((float)i * 90, 0.f, 0.f, 1.f);

			glTranslatef(hds, -hds, -radius);
			QuarterCylinder(radius, ds, prd->curveAccuracy, 0);
			glPopMatrix();
		}
		glPopMatrix();
		if (c % 2 == 0)
			glRotatef(-90.f, 0.f, 1.f, 0.f);
		else
			glRotatef(90.f, 1.f, 0.f, 0.f);
	}

	calculateEigthPoints(&corner_points, radius, prd->curveAccuracy);

	/* Draw 8 corners */
	for (c = 0; c < 8; c++) {
		glPushMatrix();
		glTranslatef(0.f, 0.f, hds + radius);

		glRotatef((float)c * 90, 0.f, 0.f, 1.f);

		glTranslatef(hds, -hds, -radius);
		glRotatef(-90.f, 0.f, 0.f, 1.f);

		drawCornerEigth(&corner_points, radius, NULL);

		glPopMatrix();
		if (c == 3)
			glRotatef(180.f, 1.f, 0.f, 0.f);
	}
	glPopMatrix();

	freeEigthPoints(&corner_points);
}

static void
drawFlagVertices(unsigned int UNUSED(curveAccuracy))
{
	glPushMatrix();
	glLoadIdentity();	/* Always draw flag from origin */

	glBegin(GL_TRIANGLES);
	for (int s = 1; s < S_NUMSEGMENTS; s++) {
		vec3 normal1, normal2;
		if (s == 1)
			glm_vec3_copy(GLM_ZUP, normal1);
		else
			glm_vec3_copy(normal2, normal1);

		computeNormal(flag.ctlpoints[s - 1][1], flag.ctlpoints[s - 1][0], flag.ctlpoints[s][0], normal2);

		glNormal3fv(normal1);
		glVertex3fv(flag.ctlpoints[s - 1][1]);
		glNormal3fv(normal1);
		glVertex3fv(flag.ctlpoints[s - 1][0]);
		glNormal3fv(normal2);
		glVertex3fv(flag.ctlpoints[s][0]);
		glNormal3fv(normal1);
		glVertex3fv(flag.ctlpoints[s - 1][1]);
		glNormal3fv(normal2);
		glVertex3fv(flag.ctlpoints[s][0]);
		glNormal3fv(normal2);
		glVertex3fv(flag.ctlpoints[s][1]);
	}
	glEnd();

	glPopMatrix();
}

void drawFlagPole(unsigned int curveAccuracy)
{
	glPushMatrix();

	glTranslatef(-FLAGPOLE_WIDTH, -FLAG_HEIGHT, 0.f);

	glRotatef(-90.f, 1.f, 0.f, 0.f);
	gluCylinderMine(FLAGPOLE_WIDTH, FLAGPOLE_WIDTH, FLAGPOLE_HEIGHT, (int)curveAccuracy, 1, 0);

	circleRev(FLAGPOLE_WIDTH, 0.f, curveAccuracy);
	circleRev(FLAGPOLE_WIDTH * 2, FLAGPOLE_HEIGHT, curveAccuracy);

	glPopMatrix();
}

extern void
getDoubleCubePos(const BoardData * bd, float v[3])
{
    v[2] = BASE_DEPTH + DOUBLECUBE_SIZE / 2.0f; /* Cube on board most of time */

    if (bd->doubled != 0) {
        v[0] = TRAY_WIDTH + BOARD_WIDTH / 2;
        if (bd->doubled != 1)
            v[0] = TOTAL_WIDTH - v[0];

        v[1] = TOTAL_HEIGHT / 2.0f;
    } else {
        if (fClockwise)
            v[0] = TOTAL_WIDTH - TRAY_WIDTH / 2.0f;
        else
            v[0] = TRAY_WIDTH / 2.0f;

        switch (bd->cube_owner) {
        case 0:
            v[1] = TOTAL_HEIGHT / 2.0f;
            v[2] = BASE_DEPTH + EDGE_DEPTH + DOUBLECUBE_SIZE / 2.0f;
            break;
        case -1:
            v[1] = EDGE_HEIGHT + DOUBLECUBE_SIZE / 2.0f;
            break;
        case 1:
            v[1] = (TOTAL_HEIGHT - EDGE_HEIGHT) - DOUBLECUBE_SIZE / 2.0f;
            break;
        default:
            v[1] = 0;           /* error */
        }
    }
}

extern void
getDicePos(const BoardData * bd, int num, float v[3])
{
    float size = getDiceSize(bd->rd);
    if (bd->diceShown == DICE_BELOW_BOARD) {    /* Show below board */
        v[0] = size * 1.5f;
        v[1] = -size / 2.0f;
        v[2] = size / 2.0f;

        if (bd->turn == 1)
            v[0] += TOTAL_WIDTH - size * 4;
        if (num == 1)
            v[0] += size;       /* Place 2nd dice by 1st */
    } else {
        v[0] = bd->bd3d->dicePos[num][0];
        if (bd->turn == 1)
            v[0] = TOTAL_WIDTH - v[0];  /* Dice on right side */

        v[1] = (TOTAL_HEIGHT - DICE_AREA_HEIGHT) / 2.0f + bd->bd3d->dicePos[num][1];
        v[2] = BASE_DEPTH + LIFT_OFF + size / 2.0f;
    }
}

void
getPiecePos(unsigned int point, unsigned int pos, float v[3])
{
    if (point == 0 || point == 25) {    /* bars */
        v[0] = TOTAL_WIDTH / 2.0f;
        v[1] = TOTAL_HEIGHT / 2.0f;
        v[2] = BASE_DEPTH + EDGE_DEPTH + (float)((pos - 1) / 3) * PIECE_DEPTH;
        pos = ((pos - 1) % 3) + 1;

        if (point == 25) {
            v[1] += DOUBLECUBE_SIZE / 2.0f + (PIECE_HOLE + PIECE_GAP_HEIGHT) * ((float)pos + .5f);
        } else {
            v[1] -= DOUBLECUBE_SIZE / 2.0f + (PIECE_HOLE + PIECE_GAP_HEIGHT) * ((float)pos + .5f);
        }
        v[1] -= PIECE_HOLE / 2.0f;
    } else if (point >= 26) {   /* homes */
        v[2] = BASE_DEPTH;
        if (fClockwise)
            v[0] = TRAY_WIDTH / 2.0f;
        else
            v[0] = TOTAL_WIDTH - TRAY_WIDTH / 2.0f;

        if (point == 26)
            v[1] = EDGE_HEIGHT + (PIECE_DEPTH * 1.2f * (float)(pos - 1));      /* 1.3 gives a gap between pieces */
        else
            v[1] = TOTAL_HEIGHT - EDGE_HEIGHT - PIECE_DEPTH - (PIECE_DEPTH * 1.2f * (float)(pos - 1));
    } else {
        v[2] = BASE_DEPTH + (float)((pos - 1) / 5) * PIECE_DEPTH;

        if (point < 13) {
            if (fClockwise)
                point = 13 - point;

            if (pos > 10)
                pos -= 10;

            if (pos > 5)
                v[1] = EDGE_HEIGHT + (PIECE_HOLE / 2.0f) + (PIECE_HOLE + PIECE_GAP_HEIGHT) * (float)(pos - 5 - 1);
            else
                v[1] = EDGE_HEIGHT + (PIECE_HOLE + PIECE_GAP_HEIGHT) * (float)(pos - 1);

            v[0] = TRAY_WIDTH + PIECE_HOLE * (float)(12 - point);
            if (point < 7)
                v[0] += BAR_WIDTH;
        } else {
            if (fClockwise)
                point = (24 + 13) - point;

            if (pos > 10)
                pos -= 10;

            if (pos > 5)
                v[1] =
                    TOTAL_HEIGHT - EDGE_HEIGHT - (PIECE_HOLE / 2.0f) - PIECE_HOLE - (PIECE_HOLE +
                                                                                     PIECE_GAP_HEIGHT) * (float)(pos - 5 - 1);
            else
                v[1] = TOTAL_HEIGHT - EDGE_HEIGHT - PIECE_HOLE - (PIECE_HOLE + PIECE_GAP_HEIGHT) * (float)(pos - 1);

            v[0] = TRAY_WIDTH + PIECE_HOLE * (float)(point - 13);
            if (point > 18)
                v[0] += BAR_WIDTH;
        }
        v[0] += PIECE_HOLE / 2.0f;
    }
    v[2] += LIFT_OFF * 2;

    /* Move to centre of piece */
    if (point < 26) {
        v[1] += PIECE_HOLE / 2.0f;
    } else {                    /* Home pieces are sideways */
        if (point == 27)
            v[1] += PIECE_DEPTH;
        v[2] += PIECE_HOLE / 2.0f;
    }
}

void
getMoveIndicatorPos(int turn, float pos[3])
{
    if (!fClockwise)
        pos[0] = TOTAL_WIDTH - TRAY_WIDTH + ARROW_SIZE / 2.0f;
    else
        pos[0] = TRAY_WIDTH - ARROW_SIZE / 2.0f;

    if (turn == 1)
        pos[1] = EDGE_HEIGHT / 2.0f;
    else
        pos[1] = TOTAL_HEIGHT - EDGE_HEIGHT / 2.0f;

    pos[2] = BASE_DEPTH + EDGE_DEPTH;
}

extern void
drawPoint(const renderdata* prd, float tuv, unsigned int i, int p, int outline)
{                               /* Draw point with correct texture co-ords */
	float w = PIECE_HOLE;
	float h = POINT_HEIGHT;
	float x, y;

	if (p) {
		x = TRAY_WIDTH - EDGE_WIDTH + (PIECE_HOLE * (float)i);
		y = -LIFT_OFF;
	}
	else {
		x = TRAY_WIDTH - EDGE_WIDTH + BOARD_WIDTH - (PIECE_HOLE * (float)i);
		y = TOTAL_HEIGHT - EDGE_HEIGHT * 2 + LIFT_OFF;
		w = -w;
		h = -h;
	}

	glPushMatrix();
	if (prd->bgInTrays)
		glTranslatef(EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH);
	else {
		x -= TRAY_WIDTH - EDGE_WIDTH;
		glTranslatef(TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH);
	}

	if (prd->roundedPoints) {   /* Draw rounded point ends */
		float xoff;

		w = w * TAKI_WIDTH;
		y += w / 2.0f;
		h -= w / 2.0f;

		if (p)
			xoff = x + (PIECE_HOLE / 2.0f);
		else
			xoff = x - (PIECE_HOLE / 2.0f);

		/* Draw rounded semi-circle end of point (with correct texture co-ords) */
		{
			unsigned int j;
			float angle, step;
			float radius = w / 2.0f;

			step = (2 * F_PI) / (float)prd->curveAccuracy;
			angle = -step * (float)(prd->curveAccuracy / 4);
			glNormal3f(0.f, 0.f, 1.f);
			glBegin(outline ? GL_LINE_STRIP : GL_TRIANGLE_FAN);
			if (tuv != 0)
				glTexCoord2f(xoff * tuv, y * tuv);

			glVertex3f(xoff, y, 0.f);
			for (j = 0; j <= prd->curveAccuracy / 2; j++) {
				if (tuv != 0)
					glTexCoord2f((xoff + sinf(angle) * radius) * tuv, (y + cosf(angle) * radius) * tuv);
				glVertex3f(xoff + sinf(angle) * radius, y + cosf(angle) * radius, 0.f);
				angle -= step;
			}
			glEnd();
		}
		/* Move rest of point in slighlty */
		if (p)
			x -= -((PIECE_HOLE * (1 - TAKI_WIDTH)) / 2.0f);
		else
			x -= ((PIECE_HOLE * (1 - TAKI_WIDTH)) / 2.0f);
	}

	glBegin(outline ? GL_LINE_STRIP : GL_TRIANGLES);
	glNormal3f(0.f, 0.f, 1.f);
	if (tuv != 0)
		glTexCoord2f((x + w) * tuv, y * tuv);
	glVertex3f(x + w, y, 0.f);
	if (tuv != 0)
		glTexCoord2f((x + w / 2) * tuv, (y + h) * tuv);
	glVertex3f(x + w / 2, y + h, 0.f);
	if (tuv != 0)
		glTexCoord2f(x * tuv, y * tuv);
	glVertex3f(x, y, 0.f);
	glEnd();

	glPopMatrix();
}

void
drawMoveIndicator(void)
{
	glBegin(GL_QUADS);
	glNormal3f(0.f, 0.f, 1.f);
	glVertex2f(-ARROW_UNIT * 2, -ARROW_UNIT);
	glVertex2f(LIFT_OFF, -ARROW_UNIT);
	glVertex2f(LIFT_OFF, ARROW_UNIT);
	glVertex2f(-ARROW_UNIT * 2, ARROW_UNIT);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex2f(0.f, ARROW_UNIT * 2);
	glVertex2f(0.f, -ARROW_UNIT * 2);
	glVertex2f(ARROW_UNIT * 2, 0.f);
	glEnd();
}

static void
drawHinge(const renderdata* prd)
{
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glScalef(1.f, HINGE_SEGMENTS, 1.f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glRotatef(-90.f, 1.f, 0.f, 0.f);
	gluCylinderMine(HINGE_WIDTH, HINGE_WIDTH, HINGE_HEIGHT, prd->curveAccuracy, 1, (prd->HingeMat.pTexture != NULL));

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glRotatef(180.f, 1.f, 0.f, 0.f);
	gluDiskMine(0.f, HINGE_WIDTH, prd->curveAccuracy, 1, (prd->HingeMat.pTexture != NULL));

	glPopMatrix();
}

static void
drawHingeGap(void)
{
	drawRect((TOTAL_WIDTH - HINGE_GAP * 1.5f) / 2.0f, 0.f, BASE_DEPTH + EDGE_DEPTH, HINGE_GAP * 1.5f, TOTAL_HEIGHT + LIFT_OFF, 0);
}

#define M_X(x, y, z) if (tuv != 0.0f) glTexCoord2f((z) * tuv, (y) * tuv); glVertex3f(x, y, z);
#define M_Z(x, y, z) if (tuv != 0.0f) glTexCoord2f((x) * tuv, (y) * tuv); glVertex3f(x, y, z);

#define DrawBottom(x, y, z, w, h)\
	glNormal3f(0.f, -1.f, 0.f);\
	glBegin(GL_QUADS);\
	if (tuv != 0.0f) glTexCoord2f((x) * tuv, ((y) + BOARD_FILLET - curveTextOff - (h)) * tuv);\
	glVertex3f(x, y, z);\
	if (tuv != 0.0f) glTexCoord2f(((x) + (w)) * tuv, ((y) + BOARD_FILLET - curveTextOff - (h)) * tuv);\
	glVertex3f((x) + (w), y, z);\
	if (tuv != 0.0f) glTexCoord2f(((x) + (w)) * tuv, ((y) + BOARD_FILLET - curveTextOff) * tuv);\
	glVertex3f((x) + (w), y, (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f((x) * tuv, ((y) + BOARD_FILLET - curveTextOff) * tuv);\
	glVertex3f(x, y, (z) + (h));\
	glEnd();

#define DrawTop(x, y, z, w, h)\
	glNormal3f(0.f, 1.f, 0.f);\
	glBegin(GL_QUADS);\
	if (tuv != 0.0f) glTexCoord2f((x) * tuv, ((y) - (BOARD_FILLET - curveTextOff) + (h)) * tuv);\
	glVertex3f(x, y, z);\
	if (tuv != 0.0f) glTexCoord2f((x) * tuv, ((y) - (BOARD_FILLET - curveTextOff)) * tuv);\
	glVertex3f(x, y, (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f(((x) + (w)) * tuv, ((y) - (BOARD_FILLET - curveTextOff)) * tuv);\
	glVertex3f((x) + (w), y, (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f(((x) + (w)) * tuv, ((y) - (BOARD_FILLET - curveTextOff) + (h)) * tuv);\
	glVertex3f((x) + (w), y, z);\
	glEnd();

#define DrawLeft(x, y, z, w, h)\
	glNormal3f(-1.f, 0.f, 0.f);\
	glBegin(GL_QUADS);\
	if (tuv != 0.0f) glTexCoord2f(((x) + BOARD_FILLET - curveTextOff - (h)) * tuv, (y) * tuv);\
	glVertex3f(x, y, z);\
	if (tuv != 0.0f) glTexCoord2f(((x) + BOARD_FILLET - curveTextOff) * tuv, (y) * tuv);\
	glVertex3f(x, y, (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f(((x) + BOARD_FILLET - curveTextOff) * tuv, ((y) + (w)) * tuv);\
	glVertex3f(x, (y) + (w), (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f(((x) + BOARD_FILLET - curveTextOff - (h)) * tuv, ((y) + (w)) * tuv);\
	glVertex3f(x, (y) + (w), z);\
	glEnd();

#define DrawRight(x, y, z, w, h)\
	glNormal3f(1.f, 0.f, 0.f);\
	glBegin(GL_QUADS);\
	if (tuv != 0.0f) glTexCoord2f(((x) - (BOARD_FILLET - curveTextOff) + (h)) * tuv, (y) * tuv);\
	glVertex3f(x, y, z);\
	if (tuv != 0.0f) glTexCoord2f(((x) - (BOARD_FILLET - curveTextOff) + (h)) * tuv, ((y) + (w)) * tuv);\
	glVertex3f(x, (y) + (w), z);\
	if (tuv != 0.0f) glTexCoord2f(((x) - (BOARD_FILLET - curveTextOff)) * tuv, ((y) + (w)) * tuv);\
	glVertex3f(x, (y) + (w), (z) + (h));\
	if (tuv != 0.0f) glTexCoord2f(((x) - (BOARD_FILLET - curveTextOff)) * tuv, (y) * tuv);\
	glVertex3f(x, y, (z) + (h));\
	glEnd();

static void
InsideFillet(float x, float y, float z, float w, float h, float radius, unsigned int accuracy, const Texture* texture, float tuv,
	float curveTextOff)
{
	/* Left */
	DrawRight(x + BOARD_FILLET, y + BOARD_FILLET, BASE_DEPTH, h, EDGE_DEPTH - BOARD_FILLET);
	/* Top */
	DrawBottom(x + BOARD_FILLET, y + h + BOARD_FILLET, BASE_DEPTH, w, EDGE_DEPTH - BOARD_FILLET + LIFT_OFF);
	/* Bottom */
	DrawTop(x + BOARD_FILLET, y + BOARD_FILLET, BASE_DEPTH, w, EDGE_DEPTH - BOARD_FILLET)
		/* Right */
		DrawLeft(x + w + BOARD_FILLET, y + BOARD_FILLET, BASE_DEPTH + LIFT_OFF, h, EDGE_DEPTH - BOARD_FILLET);

	if (tuv != 0.0f) {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glTranslatef((x + curveTextOff) * tuv, (y + h + radius * 2) * tuv, 0.f);
		glRotatef(-90.f, 0.f, 0.f, 1.f);
		glMatrixMode(GL_MODELVIEW);
	}
	glPushMatrix();
	glTranslatef(x, y + h + radius * 2, z - radius);
	glRotatef(-180.f, 1.f, 0.f, 0.f);
	glRotatef(90.f, 0.f, 1.f, 0.f);
	QuarterCylinderSplayed(radius, h + radius * 2, accuracy, texture);
	glPopMatrix();
	if (tuv != 0.0f) {
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(x * tuv, (y + curveTextOff) * tuv, 0.f);
		glMatrixMode(GL_MODELVIEW);
	}
	glPushMatrix();
	glTranslatef(x, y, z - radius);
	glRotatef(-90.f, 1.f, 0.f, 0.f);
	glRotatef(-90.f, 0.f, 0.f, 1.f);
	QuarterCylinderSplayed(radius, w + radius * 2, accuracy, texture);
	glPopMatrix();
	if (tuv != 0.0f) {
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();

		glPushMatrix();
		glTranslatef((x + w + radius * 2 - curveTextOff) * tuv, y * tuv, 0.f);
		glRotatef(90.f, 0.f, 0.f, 1.f);
		glMatrixMode(GL_MODELVIEW);
	}
	glPushMatrix();
	glTranslatef(x + w + radius * 2, y, z - radius);
	glRotatef(-90.f, 0.f, 1.f, 0.f);
	QuarterCylinderSplayed(radius, h + radius * 2, accuracy, texture);
	glPopMatrix();
	if (tuv != 0.0f) {
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();

		glPushMatrix();
		glTranslatef((x + radius) * tuv, (y + h + radius * 2) * tuv, 0.f);
		glMatrixMode(GL_MODELVIEW);
	}
	glPushMatrix();
	glTranslatef(x + radius, y + h + radius * 2, z - radius);
	glRotatef(-90.f, 0.f, 0.f, 1.f);
	QuarterCylinderSplayedRev(radius, w, accuracy, texture);
	glPopMatrix();
	if (tuv != 0.0f) {
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
}

static void
drawTableModel(const renderdata* prd, const EigthPoints* bd3dboardPoints)
{
	float curveTextOff = 0;
	float tuv = 0;

	if (!prd->bgInTrays)
		drawSplitRect(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE + LIFT_OFF,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, prd->BoxMat.pTexture);

	if (prd->roundedEdges) {
		if (prd->BoxMat.pTexture) {
			float st, ct, dInc;

			tuv = (TEXTURE_SCALE) / (float)prd->BoxMat.pTexture->width;
			st = sinf((2 * F_PI) / (float)prd->curveAccuracy) * BOARD_FILLET;
			ct = (cosf((2 * F_PI) / (float)prd->curveAccuracy) - 1) * BOARD_FILLET;
			dInc = sqrtf(st * st + ct * ct);
			curveTextOff = (float)(prd->curveAccuracy / 4) * dInc;
		}

		/* Right edge */
		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);

		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - BOARD_FILLET + LIFT_OFF, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		glEnd();

		/* Right face */
		DrawRight(TOTAL_WIDTH, BOARD_FILLET, 0.f, TOTAL_HEIGHT - BOARD_FILLET * 2,
			BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef((TOTAL_WIDTH - BOARD_FILLET) * tuv,
				(BOARD_FILLET - curveTextOff - (BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET)) * tuv, 0.f);
			glRotatef(90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(TOTAL_WIDTH - BOARD_FILLET, BOARD_FILLET, 0.f);
		glRotatef(90.f, 1.f, 0.f, 0.f);
		QuarterCylinder(BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef((TOTAL_WIDTH - BOARD_FILLET) * tuv, (TOTAL_HEIGHT - (BOARD_FILLET - curveTextOff)) * tuv, 0.f);
			glRotatef(90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(TOTAL_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-90.f, 1.f, 0.f, 0.f);
		QuarterCylinder(BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef((TOTAL_WIDTH - BOARD_FILLET + curveTextOff) * tuv, (TOTAL_HEIGHT - BOARD_FILLET) * tuv, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(TOTAL_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-180.f, 1.f, 0.f, 0.f);
		glRotatef(90.f, 0.f, 1.f, 0.f);
		QuarterCylinder(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET * 2, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(TOTAL_WIDTH - BOARD_FILLET * tuv, BOARD_FILLET * tuv, 0.f);
			glRotatef(90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(TOTAL_WIDTH - BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-90.f, 0.f, 0.f, 1.f);
		drawCornerEigth(bd3dboardPoints, BOARD_FILLET, prd->BoxMat.pTexture);
		glPopMatrix();

		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(TOTAL_WIDTH - BOARD_FILLET * tuv, TOTAL_HEIGHT - BOARD_FILLET * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(TOTAL_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		drawCornerEigth(bd3dboardPoints, BOARD_FILLET, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
		}

		/* Top + bottom edges */
		if (!prd->fHinges3d) {
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(EDGE_WIDTH - BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(EDGE_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);

			M_Z(TOTAL_WIDTH / 2.0f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			glEnd();

			DrawBottom(BOARD_FILLET, 0.f, 0.f, TOTAL_WIDTH - BOARD_FILLET * 2, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(EDGE_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(EDGE_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);

			M_Z(TOTAL_WIDTH / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH / 2.0f, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			glEnd();
			/* Top Face */
			DrawTop(BOARD_FILLET, TOTAL_HEIGHT, 0.f, TOTAL_WIDTH - BOARD_FILLET * 2,
				BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, BOARD_FILLET * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);

			glPushMatrix();
			glTranslatef(BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, TOTAL_WIDTH - BOARD_FILLET * 2, prd->curveAccuracy, prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, (TOTAL_HEIGHT - (BOARD_FILLET - curveTextOff)) * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 1.f, 0.f, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, TOTAL_WIDTH - BOARD_FILLET * 2, prd->curveAccuracy, prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}
		else {
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z((TOTAL_WIDTH + HINGE_GAP) / 2.0f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH + HINGE_GAP) / 2.0f, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			glEnd();

			DrawBottom((TOTAL_WIDTH + HINGE_GAP) / 2.0f, 0.f, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z((TOTAL_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			glEnd();
			/* Top Face */
			DrawTop((TOTAL_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT, 0.f,
				(TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef(((TOTAL_WIDTH + HINGE_GAP) / 2.0f) * tuv, BOARD_FILLET * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef((TOTAL_WIDTH + HINGE_GAP) / 2.0f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET, prd->curveAccuracy,
				prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(((TOTAL_WIDTH + HINGE_GAP) / 2.0f) * tuv, (TOTAL_HEIGHT - (BOARD_FILLET - curveTextOff)) * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef((TOTAL_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 1.f, 0.f, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET, prd->curveAccuracy,
				prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}

		/* Bar */
		if (!prd->fHinges3d) {
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF,
				TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			glEnd();
		}
		else {
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH + HINGE_GAP) / 2.0f, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);

			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET + LIFT_OFF,
				TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			glEnd();
		}
		/* Bear-off edge */
		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + BOARD_FILLET - LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

		M_Z(TOTAL_WIDTH - TRAY_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		glEnd();

		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - LIFT_OFF - BOARD_FILLET, TRAY_HEIGHT + BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TRAY_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - EDGE_WIDTH + BOARD_FILLET, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - LIFT_OFF - BOARD_FILLET,
			TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		glEnd();

		InsideFillet(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, BOARD_FILLET,
			prd->curveAccuracy, prd->BoxMat.pTexture, tuv, curveTextOff);
		InsideFillet(TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - BOARD_FILLET,
			TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, BOARD_FILLET, prd->curveAccuracy,
			prd->BoxMat.pTexture, tuv, curveTextOff);

		InsideFillet(TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH, BOARD_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT * 2, BOARD_FILLET,
			prd->curveAccuracy, prd->BoxMat.pTexture, tuv, curveTextOff);
	}
	else {
		/* Right edge */
		drawBox(BOX_SPLITTOP, TOTAL_WIDTH - EDGE_WIDTH, 0.f, 0.f, EDGE_WIDTH, TOTAL_HEIGHT, BASE_DEPTH + EDGE_DEPTH,
			prd->BoxMat.pTexture);

		/* Top + bottom edges */
		if (!prd->fHinges3d) {
			drawBox(BOX_NOSIDES | BOX_SPLITWIDTH, EDGE_WIDTH, 0.f, 0.f, TOTAL_WIDTH - EDGE_WIDTH * 2, EDGE_HEIGHT,
				BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);
			drawBox(BOX_NOSIDES | BOX_SPLITWIDTH, EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, 0.f,
				TOTAL_WIDTH - EDGE_WIDTH * 2, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);
		}
		else {
			drawBox(BOX_ALL, (TOTAL_WIDTH + HINGE_GAP) / 2.0f, 0.f, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - EDGE_WIDTH,
				EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);
			drawBox(BOX_ALL, (TOTAL_WIDTH + HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT, 0.f,
				(TOTAL_WIDTH - HINGE_GAP) / 2.0f - EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH,
				prd->BoxMat.pTexture);
		}

		/* Bar */
		if (!prd->fHinges3d)
			drawBox(BOX_NOENDS | BOX_SPLITTOP, TRAY_WIDTH + BOARD_WIDTH, EDGE_HEIGHT, BASE_DEPTH, BAR_WIDTH,
				TOTAL_HEIGHT - EDGE_HEIGHT * 2, EDGE_DEPTH, prd->BoxMat.pTexture);
		else
			drawBox(BOX_NOENDS | BOX_SPLITTOP, TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH + HINGE_GAP) / 2.0f, EDGE_HEIGHT,
				0.f, (BAR_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT * 2, BASE_DEPTH + EDGE_DEPTH,
				prd->BoxMat.pTexture);

		/* Bear-off edge */
		drawBox(BOX_NOENDS | BOX_SPLITTOP, TOTAL_WIDTH - TRAY_WIDTH, EDGE_HEIGHT, BASE_DEPTH, EDGE_WIDTH,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, EDGE_DEPTH, prd->BoxMat.pTexture);
		drawBox(BOX_NOSIDES, TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH - LIFT_OFF, TRAY_HEIGHT, BASE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2 + LIFT_OFF * 2, MID_SIDE_GAP_HEIGHT, EDGE_DEPTH, prd->BoxMat.pTexture);
	}

	/* Left side of board */
	glPushMatrix();

	if (!prd->bgInTrays)
		drawSplitRect(EDGE_WIDTH - LIFT_OFF, EDGE_HEIGHT, BASE_DEPTH, PIECE_HOLE, TOTAL_HEIGHT - EDGE_HEIGHT * 2,
			prd->BoxMat.pTexture);

	if (prd->roundedEdges) {
		/* Left edge */
		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);
		M_Z(BOARD_FILLET - LIFT_OFF, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

		M_Z(BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(EDGE_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		glEnd();
		/* Left Face */
		DrawLeft(0.f, BOARD_FILLET, 0.f, TOTAL_HEIGHT - BOARD_FILLET * 2, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, (BOARD_FILLET - curveTextOff) * tuv, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-90.f, 1.f, 0.f, 0.f);
		glRotatef(180.f, 0.f, 1.f, 0.f);
		QuarterCylinder(BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef((BOARD_FILLET - curveTextOff) * tuv, (TOTAL_HEIGHT - (BOARD_FILLET - curveTextOff)) * tuv,
				0.f);
			glRotatef(90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-90.f, 1.f, 0.f, 0.f);
		glRotatef(-90.f, 0.f, 1.f, 0.f);
		QuarterCylinder(BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef((BOARD_FILLET - curveTextOff) * tuv, BOARD_FILLET * tuv, 0.f);
			glRotatef(90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(-90.f, 0.f, 1.f, 0.f);
		QuarterCylinder(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET * 2, prd->curveAccuracy, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, BOARD_FILLET * tuv, 0.f);
			glRotatef(180.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(180.f, 0.f, 0.f, 1.f);
		drawCornerEigth(bd3dboardPoints, BOARD_FILLET, prd->BoxMat.pTexture);
		glPopMatrix();

		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, BOARD_FILLET * tuv, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			glMatrixMode(GL_MODELVIEW);
		}
		glPushMatrix();
		glTranslatef(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
		glRotatef(90.f, 0.f, 0.f, 1.f);
		drawCornerEigth(bd3dboardPoints, BOARD_FILLET, prd->BoxMat.pTexture);
		glPopMatrix();
		if (tuv != 0.0f) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}

		if (prd->fHinges3d) {   /* Top + bottom edges and bar */
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(EDGE_WIDTH - BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH - HINGE_GAP) / 2.0f, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH - HINGE_GAP) / 2.0f, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(EDGE_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			glEnd();

			DrawBottom(BOARD_FILLET, 0.f, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(EDGE_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z((TOTAL_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			M_Z(EDGE_WIDTH - BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
			/* Top Face */
			glEnd();

			DrawTop(BOARD_FILLET, TOTAL_HEIGHT, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);

			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, BOARD_FILLET * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(BOARD_FILLET, BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET, prd->curveAccuracy,
				prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(BOARD_FILLET * tuv, (TOTAL_HEIGHT - (BOARD_FILLET - curveTextOff)) * tuv, 0.f);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(BOARD_FILLET, TOTAL_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH - BOARD_FILLET);
			glRotatef(-90.f, 1.f, 0.f, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
			QuarterCylinder(BOARD_FILLET, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - BOARD_FILLET, prd->curveAccuracy,
				prd->BoxMat.pTexture);
			glPopMatrix();

			glMatrixMode(GL_TEXTURE);
			glPopMatrix();

			glMatrixMode(GL_MODELVIEW);
			glBegin(GL_QUADS);
			/* Front Face */
			glNormal3f(0.f, 0.f, 1.f);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH - HINGE_GAP) / 2.0f, EDGE_HEIGHT - BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT / 2.0f,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + (BAR_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			M_Z(TRAY_WIDTH + BOARD_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
				BASE_DEPTH + EDGE_DEPTH);
			glEnd();
		}

		/* Bear-off edge */
		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);
		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - BOARD_FILLET + LIFT_OFF, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);

		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - BOARD_FILLET + LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET - LIFT_OFF, TOTAL_HEIGHT - EDGE_HEIGHT + BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		glEnd();

		glBegin(GL_QUADS);
		/* Front Face */
		glNormal3f(0.f, 0.f, 1.f);
		M_Z(EDGE_WIDTH - LIFT_OFF - BOARD_FILLET, TRAY_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET + LIFT_OFF * 2, TRAY_HEIGHT + BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH);
		M_Z(TRAY_WIDTH - EDGE_WIDTH + BOARD_FILLET + LIFT_OFF * 2, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		M_Z(EDGE_WIDTH - LIFT_OFF - BOARD_FILLET, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH);
		glEnd();

		InsideFillet(EDGE_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH,
			TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, BOARD_FILLET, prd->curveAccuracy,
			prd->BoxMat.pTexture, tuv, curveTextOff);
		InsideFillet(EDGE_WIDTH - BOARD_FILLET, TRAY_HEIGHT + MID_SIDE_GAP_HEIGHT - BOARD_FILLET,
			BASE_DEPTH + EDGE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2, TRAY_HEIGHT - EDGE_HEIGHT, BOARD_FILLET,
			prd->curveAccuracy, prd->BoxMat.pTexture, tuv, curveTextOff);

		InsideFillet(TRAY_WIDTH - BOARD_FILLET, EDGE_HEIGHT - BOARD_FILLET, BASE_DEPTH + EDGE_DEPTH, BOARD_WIDTH,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, BOARD_FILLET, prd->curveAccuracy, prd->BoxMat.pTexture, tuv,
			curveTextOff);
	}
	else {
		/* Left edge */
		drawBox(BOX_SPLITTOP, 0.f, 0.f, 0.f, EDGE_WIDTH, TOTAL_HEIGHT, BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);

		if (prd->fHinges3d) {   /* Top + bottom edges and bar */
			drawBox(BOX_ALL, EDGE_WIDTH, 0.f, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - EDGE_WIDTH, EDGE_HEIGHT,
				BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);
			drawBox(BOX_ALL, EDGE_WIDTH, TOTAL_HEIGHT - EDGE_HEIGHT, 0.f, (TOTAL_WIDTH - HINGE_GAP) / 2.0f - EDGE_WIDTH,
				EDGE_HEIGHT, BASE_DEPTH + EDGE_DEPTH, prd->BoxMat.pTexture);
		}

		if (prd->fHinges3d)
			drawBox(BOX_NOENDS | BOX_SPLITTOP, TRAY_WIDTH + BOARD_WIDTH, EDGE_HEIGHT, 0.f,
			(BAR_WIDTH - HINGE_GAP) / 2.0f, TOTAL_HEIGHT - EDGE_HEIGHT * 2, BASE_DEPTH + EDGE_DEPTH,
				prd->BoxMat.pTexture);

		/* Bear-off edge */
		drawBox(BOX_NOENDS | BOX_SPLITTOP, TRAY_WIDTH - EDGE_WIDTH, EDGE_HEIGHT, BASE_DEPTH, EDGE_WIDTH,
			TOTAL_HEIGHT - EDGE_HEIGHT * 2, EDGE_DEPTH, prd->BoxMat.pTexture);
		drawBox(BOX_NOSIDES, EDGE_WIDTH - LIFT_OFF, TRAY_HEIGHT, BASE_DEPTH, TRAY_WIDTH - EDGE_WIDTH * 2 + LIFT_OFF * 2,
			MID_SIDE_GAP_HEIGHT, EDGE_DEPTH, prd->BoxMat.pTexture);
	}

	glPopMatrix();
}

static void
GrayScaleCol(float *pCols)
{
    float gs = (pCols[0] + pCols[1] + pCols[2]) / 2.5f; /* Slightly lighter gray */
    pCols[0] = pCols[1] = pCols[2] = gs;
}

static void
GrayScale3D(Material * pMat)
{
    GrayScaleCol(pMat->ambientColour);
    GrayScaleCol(pMat->diffuseColour);
    GrayScaleCol(pMat->specularColour);
}

extern void
drawTableGrayed(const ModelManager* modelHolder, const BoardData3d * bd3d, renderdata tmp)
{
    GrayScale3D(&tmp.BaseMat);
    GrayScale3D(&tmp.PointMat[0]);
    GrayScale3D(&tmp.PointMat[1]);
    GrayScale3D(&tmp.BoxMat);

	drawTable(modelHolder, bd3d, &tmp);
}

extern int
DiceShowing(const BoardData * bd)
{
    return ((bd->diceShown == DICE_ON_BOARD && bd->diceRoll[0]) ||
            (bd->rd->fDiceArea && bd->diceShown == DICE_BELOW_BOARD));
}

extern void
getFlagPos(int turn, float v[3])
{
	v[0] = TRAY_WIDTH + (BOARD_WIDTH - FLAG_WIDTH) / 2.0f;
    v[1] = TOTAL_HEIGHT / 2.0f;
    v[2] = BASE_DEPTH + EDGE_DEPTH;

    if (turn == -1)         /* Move to other side of board */
        v[0] += BOARD_WIDTH + BAR_WIDTH;
}

void
waveFlag(float wag)
{
    int i, j;

    /* wave the flag by rotating Z coords though a sine wave */
	for (i = 1; i < S_NUMSEGMENTS; i++)
	{
		for (j = 0; j < 2; j++)
		{
			flag.ctlpoints[i][j][2] = sinf((float)i / 3.0f + wag) * FLAG_WAG;
			/* Fixed at pole and more wavy towards end */
			flag.ctlpoints[i][j][2] *= (1.0f / S_NUMSEGMENTS) * (float)i;
		}
	}
}

void
setupPath(const BoardData * bd, Path * p, float *pRotate, unsigned int fromPoint, unsigned int fromDepth,
          unsigned int toPoint, unsigned int toDepth)
{                               /* Work out the animation path for a moving piece */
    float point[3];
    float w, h, xDist, yDist;
    float xDiff, yDiff;
    float bar1Dist, bar1Ratio;
    float bar2Dist, bar2Ratio;
    float start[3], end[3], obj1, obj2, objHeight;
    unsigned int fromBoard = (fromPoint + 5) / 6;
    unsigned int toBoard = (toPoint - 1) / 6 + 1;
    int yDir = 0;

    /* First work out were piece has to move */
    /* Get start and end points */
    getPiecePos(fromPoint, fromDepth, start);
    getPiecePos(toPoint, toDepth, end);

    /* Only rotate piece going home */
    *pRotate = -1;

    /* Swap boards if displaying other way around */
    if (fClockwise) {
        const unsigned int swapBoard[] = { 0, 2, 1, 4, 3, 5 };
        fromBoard = swapBoard[fromBoard];
        toBoard = swapBoard[toBoard];
    }

    /* Work out what obstacle needs to be avoided */
    if ((fromBoard == 2 || fromBoard == 3) && (toBoard == 1 || toBoard == 4)) { /* left side to right side */
        obj1 = TRAY_WIDTH + BOARD_WIDTH;
        obj2 = obj1 + BAR_WIDTH;
        objHeight = BASE_DEPTH + EDGE_DEPTH + DOUBLECUBE_SIZE;
    } else if ((fromBoard == 1 || fromBoard == 4) && (toBoard == 2 || toBoard == 3)) {  /* right side to left side */
        obj2 = TRAY_WIDTH + BOARD_WIDTH;
        obj1 = obj2 + BAR_WIDTH;
        objHeight = BASE_DEPTH + EDGE_DEPTH + DOUBLECUBE_SIZE;
    } else if ((fromBoard == 1 && toBoard == 4) || (fromBoard == 2 && toBoard == 3)) {  /* up same side */
        obj1 = (TOTAL_HEIGHT - DICE_AREA_HEIGHT) / 2.0f;
        obj2 = (TOTAL_HEIGHT + DICE_AREA_HEIGHT) / 2.0f;
        objHeight = BASE_DEPTH + getDiceSize(bd->rd);
        yDir = 1;
    } else if ((fromBoard == 4 && toBoard == 1) || (fromBoard == 3 && toBoard == 2)) {  /* down same side */
        obj1 = (TOTAL_HEIGHT + DICE_AREA_HEIGHT) / 2.0f;
        obj2 = (TOTAL_HEIGHT - DICE_AREA_HEIGHT) / 2.0f;
        objHeight = BASE_DEPTH + getDiceSize(bd->rd);
        yDir = 1;
    } else if (fromBoard == toBoard) {
        if (fromBoard <= 2) {
            if (fClockwise) {
                fromPoint = 13 - fromPoint;
                toPoint = 13 - toPoint;
            }

            if (fromPoint < toPoint)
                toPoint--;
            else
                fromPoint--;

            fromPoint = 12 - fromPoint;
            toPoint = 12 - toPoint;
        } else {
            if (fClockwise) {
                fromPoint = 24 + 13 - fromPoint;
                toPoint = 24 + 13 - toPoint;
            }

            if (fromPoint < toPoint)
                fromPoint++;
            else
                toPoint++;
            fromPoint = fromPoint - 13;
            toPoint = toPoint - 13;
        }
        obj1 = TRAY_WIDTH + PIECE_HOLE * (float)fromPoint;
        obj2 = TRAY_WIDTH + PIECE_HOLE * (float)toPoint;
        if ((fromBoard == 1) || (fromBoard == 4)) {
            obj1 += BAR_WIDTH;
            obj2 += BAR_WIDTH;
        }

        objHeight = BASE_DEPTH + PIECE_DEPTH * 3;
    } else {
        if (fromPoint == 0 || fromPoint == 25) {        /* Move from bar */
            if (!fClockwise) {
                obj1 = TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH;
                if (fromPoint == 0) {
                    obj2 = TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH;
                    if (toPoint > 20)
                        obj2 += PIECE_HOLE * (float)(toPoint - 20);
                } else {
                    obj2 = TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH;
                    if (toPoint < 5)
                        obj2 += PIECE_HOLE * (float)(5 - toPoint);
                }
            } else {
                obj1 = TRAY_WIDTH + BOARD_WIDTH;
                if (fromPoint == 0) {
                    obj2 = TRAY_WIDTH + PIECE_HOLE * (float)(25 - toPoint);
                    if (toPoint > 19)
                        obj2 += PIECE_HOLE;
                } else {
                    obj2 = TRAY_WIDTH + PIECE_HOLE * (float)toPoint;
                    if (toPoint < 6)
                        obj2 += PIECE_HOLE;
                }
            }
            objHeight = BASE_DEPTH + EDGE_DEPTH + DOUBLECUBE_SIZE;
        } else {                /* Move home */
            if (!fClockwise) {
                if (toPoint == 26)
                    obj1 = TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH + PIECE_HOLE * (float)(7 - fromPoint);
                else            /* (toPoint == 27) */
                    obj1 = TRAY_WIDTH + BOARD_WIDTH + BAR_WIDTH + PIECE_HOLE * (float)(fromPoint - 18);
            } else {
                if (toPoint == 26)
                    obj1 = TRAY_WIDTH + PIECE_HOLE * (float)(fromPoint - 1);
                else            /* (toPoint == 27) */
                    obj1 = TRAY_WIDTH + PIECE_HOLE * (float)(24 - fromPoint);
            }

            if (!fClockwise)
                obj2 = TOTAL_WIDTH - TRAY_WIDTH + EDGE_WIDTH;
            else
                obj2 = TRAY_WIDTH - EDGE_WIDTH;

            *pRotate = 0;
            objHeight = BASE_DEPTH + EDGE_DEPTH + getDiceSize(bd->rd);
        }
    }
    if ((fromBoard == 3 && toBoard == 4) || (fromBoard == 4 && toBoard == 3)) { /* Special case when moving along top of board */
        if ((bd->cube_owner != 1) && (fromDepth <= 2) && (toDepth <= 2))
            objHeight = BASE_DEPTH + EDGE_DEPTH + PIECE_DEPTH;
    }

    /* Now setup path object */
    /* Start point */
    initPath(p, start);

    /* obstacle height */
    point[2] = objHeight;

    if (yDir) {                 /* barriers are along y axis */
        xDiff = end[0] - start[0];
        yDiff = end[1] - start[1];
        bar1Dist = obj1 - start[1];
        bar1Ratio = bar1Dist / yDiff;
        bar2Dist = obj2 - start[1];
        bar2Ratio = bar2Dist / yDiff;

        /* 2nd point - moved up from 1st one */
        /* Work out whether 2nd point is further away than height required */
        xDist = xDiff * bar1Ratio;
        yDist = obj1 - start[1];
        h = objHeight - start[2];
        w = sqrtf(xDist * xDist + yDist * yDist);

        if (h > w) {
            point[0] = start[0] + xDiff * bar1Ratio;
            point[1] = obj1;
        } else {
            point[0] = start[0] + xDist * (h / w);
            point[1] = start[1] + yDist * (h / w);
        }
        addPathSegment(p, PATH_CURVE_9TO12, point);

        /* Work out whether 3rd point is further away than height required */
        yDist = end[1] - obj2;
        xDist = xDiff * (yDist / yDiff);
        w = sqrtf(xDist * xDist + yDist * yDist);
        h = objHeight - end[2];

        /* 3rd point - moved along from 2nd one */
        if (h > w) {
            point[0] = start[0] + xDiff * bar2Ratio;
            point[1] = obj2;
        } else {
            point[0] = end[0] - xDist * (h / w);
            point[1] = end[1] - yDist * (h / w);
        }
        addPathSegment(p, PATH_LINE, point);
    } else {                    /* barriers are along x axis */
        xDiff = end[0] - start[0];
        yDiff = end[1] - start[1];
        bar1Dist = obj1 - start[0];
        bar1Ratio = bar1Dist / xDiff;
        bar2Dist = obj2 - start[0];
        bar2Ratio = bar2Dist / xDiff;

        /* Work out whether 2nd point is further away than height required */
        xDist = obj1 - start[0];
        yDist = yDiff * bar1Ratio;
        h = objHeight - start[2];
        w = sqrtf(xDist * xDist + yDist * yDist);

        /* 2nd point - moved up from 1st one */
        if (h > w) {
            point[0] = obj1;
            point[1] = start[1] + yDiff * bar1Ratio;
        } else {
            point[0] = start[0] + xDist * (h / w);
            point[1] = start[1] + yDist * (h / w);
        }
        addPathSegment(p, PATH_CURVE_9TO12, point);

        /* Work out whether 3rd point is further away than height required */
        xDist = end[0] - obj2;
        yDist = yDiff * (xDist / xDiff);
        w = sqrtf(xDist * xDist + yDist * yDist);
        h = objHeight - end[2];

        /* 3rd point - moved along from 2nd one */
        if (h > w) {
            point[0] = obj2;
            point[1] = start[1] + yDiff * bar2Ratio;
        } else {
            point[0] = end[0] - xDist * (h / w);
            point[1] = end[1] - yDist * (h / w);
        }
        addPathSegment(p, PATH_LINE, point);
    }
    /* End point */
    addPathSegment(p, PATH_CURVE_12TO3, end);
}

static void
setupDicePath(int num, float stepSize, Path * p, const BoardData * bd)
{
    float point[3];
    getDicePos(bd, num, point);

    point[0] -= stepSize * 5;
    initPath(p, point);

    point[0] += stepSize * 2;
    addPathSegment(p, PATH_PARABOLA_12TO3, point);

    point[0] += stepSize * 2;
    addPathSegment(p, PATH_PARABOLA, point);

    point[0] += stepSize;
    addPathSegment(p, PATH_PARABOLA, point);
}

static void
randomDiceRotation(const Path * p, DiceRotation * diceRotation, float dir)
{
    /* Dice rotation range values */
#define XROT_MIN 1
#define XROT_RANGE 1.5f

#define YROT_MIN -.5f
#define YROT_RANGE 1.f

    diceRotation->xRotFactor = XROT_MIN + randRange(XROT_RANGE);
    diceRotation->xRotStart = 1 - (p->pts[p->numSegments][0] - p->pts[0][0]) * diceRotation->xRotFactor;
    diceRotation->xRot = diceRotation->xRotStart * 360;

    diceRotation->yRotFactor = YROT_MIN + randRange(YROT_RANGE);
    diceRotation->yRotStart = 1 - (p->pts[p->numSegments][0] - p->pts[0][0]) * diceRotation->yRotFactor;
    diceRotation->yRot = diceRotation->yRotStart * 360;

    if (dir < 0.f) {
        diceRotation->xRotFactor = -diceRotation->xRotFactor;
        diceRotation->yRotFactor = -diceRotation->yRotFactor;
    }
}

void
setupDicePaths(const BoardData * bd, Path dicePaths[2], float diceMovingPos[2][3], DiceRotation diceRotation[2])
{
    int firstDie = (bd->turn == 1);
    int secondDie = !firstDie;
    float dir = (bd->turn == 1) ? -1.0f : 1.0f;

    setupDicePath(firstDie, dir * DICE_STEP_SIZE0, &dicePaths[firstDie], bd);
    setupDicePath(secondDie, dir * DICE_STEP_SIZE1, &dicePaths[secondDie], bd);

    randomDiceRotation(&dicePaths[firstDie], &diceRotation[firstDie], dir);
    randomDiceRotation(&dicePaths[secondDie], &diceRotation[secondDie], dir);

    copyPoint(diceMovingPos[0], dicePaths[0].pts[0]);
    copyPoint(diceMovingPos[1], dicePaths[1].pts[0]);
}

int
DiceTooClose(const BoardData3d * bd3d, const renderdata * prd)
{
    float dist;
    float orgX[2];
    int firstDie = bd3d->dicePos[0][0] > bd3d->dicePos[1][0];
    int secondDie = !firstDie;

    orgX[0] = bd3d->dicePos[firstDie][0] - DICE_STEP_SIZE0 * 5;
    orgX[1] = bd3d->dicePos[secondDie][0] - DICE_STEP_SIZE1 * 5;
    dist = sqrtf((orgX[1] - orgX[0]) * (orgX[1] - orgX[0])
                 + (bd3d->dicePos[secondDie][1] - bd3d->dicePos[firstDie][1]) * (bd3d->dicePos[secondDie][1] -
                                                                                 bd3d->dicePos[firstDie][1]));
    return (dist < getDiceSize(prd) * 1.1f);
}

void
setDicePos(BoardData * bd, BoardData3d * bd3d)
{
    int iter = 0;
    float diceSize = getDiceSize(bd->rd);
    float firstX = TRAY_WIDTH + DICE_STEP_SIZE0 * 3 + diceSize * .75f;
    int firstDie = (bd->turn == 1);
    int secondDie = !firstDie;

    bd3d->dicePos[firstDie][0] = firstX + randRange(BOARD_WIDTH + TRAY_WIDTH - firstX - diceSize * 2);
    bd3d->dicePos[firstDie][1] = randRange(DICE_AREA_HEIGHT);

    do {                        /* insure dice are not too close together */
        if (iter++ > 20) {      /* Trouble placing 2nd dice - replace 1st dice */
            setDicePos(bd, bd3d);
            return;
        }

        firstX = bd3d->dicePos[firstDie][0] + diceSize;
        bd3d->dicePos[secondDie][0] = firstX + randRange(BOARD_WIDTH + TRAY_WIDTH - firstX - diceSize * .7f);
        bd3d->dicePos[secondDie][1] = randRange(DICE_AREA_HEIGHT);
    }
    while (DiceTooClose(bd3d, bd->rd));

    bd3d->dicePos[firstDie][2] = (float) (rand() % 360);
    bd3d->dicePos[secondDie][2] = (float) (rand() % 360);

    if (ShadowsInitilised(bd3d))
        updateDiceOccPos(bd, bd3d);
}

static void
initViewArea(viewArea * pva)
{                               /* Initialize values to extremes */
    pva->top = -base_unit * 1000;
    pva->bottom = base_unit * 1000;
    pva->width = 0;
}

extern float
getViewAreaHeight(const viewArea * pva)
{
    return pva->top - pva->bottom;
}

static float
getViewAreaWidth(const viewArea * pva)
{
    return pva->width;
}

extern float
getAreaRatio(const viewArea * pva)
{
    return getViewAreaWidth(pva) / getViewAreaHeight(pva);
}

static void
addViewAreaHeightPoint(viewArea * pva, float halfRadianFOV, float boardRadAngle, float inY, float inZ)
{                               /* Rotate points by board angle */
    float adj;
    float y, z;

    y = inY * cosf(boardRadAngle) - inZ * sinf(boardRadAngle);
    z = inZ * cosf(boardRadAngle) + inY * sinf(boardRadAngle);

    /* Project height to zero depth */
    adj = z * tanf(halfRadianFOV);
    if (y > 0)
        y += adj;
    else
        y -= adj;

    /* Store max / min heights */
    if (pva->top < y)
        pva->top = y;
    if (pva->bottom > y)
        pva->bottom = y;
}

static void
workOutWidth(viewArea * pva, float halfRadianFOV, float boardRadAngle, float aspectRatio, const float ip[3])
{
    float halfRadianXFOV;
    float w = getViewAreaHeight(pva) * aspectRatio;
    float w2, l;

    float p[3];

    /* Work out X-FOV from Y-FOV */
    float halfViewHeight = getViewAreaHeight(pva) / 2;
    l = halfViewHeight / tanf(halfRadianFOV);
    halfRadianXFOV = atanf((halfViewHeight * aspectRatio) / l);

    /* Rotate z coord by board angle */
    copyPoint(p, ip);
    p[2] = ip[2] * cosf(boardRadAngle) + ip[1] * sinf(boardRadAngle);

    /* Adjust x coord by projected z value at relevant X-FOV */
    w2 = w / 2 - p[2] * tanf(halfRadianXFOV);
    l = w2 / tanf(halfRadianXFOV);
    p[0] += p[2] * (p[0] / l);

    if (pva->width < p[0] * 2)
        pva->width = p[0] * 2;
}

NTH_STATIC float
GetFOVAngle(const BoardData * bd)
{
    float temp = bd->rd->boardAngle / 20.0f;
    float skewFactor = (bd->rd->skewFactor / 100.0f) * (4 - .6f) + .6f;
    /* Magic numbers to normalize viewing distance */
    return (47 - 2.3f * temp * temp) / skewFactor;
}

extern void
WorkOutViewArea(const BoardData * bd, viewArea * pva, float *pHalfRadianFOV, float aspectRatio)
{
    float p[3];
    float boardRadAngle;
    initViewArea(pva);

    boardRadAngle = (bd->rd->boardAngle * F_PI) / 180.0f;
    *pHalfRadianFOV = ((GetFOVAngle(bd) * F_PI) / 180.0f) / 2.0f;

    /* Sort out viewing area */
    addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, -getBoardHeight() / 2, 0.f);
    addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, -getBoardHeight() / 2, BASE_DEPTH + EDGE_DEPTH);

    if (bd->rd->fDiceArea) {
        float diceSize = getDiceSize(bd->rd);
        addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, getBoardHeight() / 2 + diceSize, 0.f);
        addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, getBoardHeight() / 2 + diceSize,
                               BASE_DEPTH + diceSize);
    } else {                    /* Bottom edge is defined by board */
        addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, getBoardHeight() / 2, 0.f);
        addViewAreaHeightPoint(pva, *pHalfRadianFOV, boardRadAngle, getBoardHeight() / 2, BASE_DEPTH + EDGE_DEPTH);
    }

    p[0] = getBoardWidth() / 2;
    p[1] = getBoardHeight() / 2;
    p[2] = BASE_DEPTH + EDGE_DEPTH;
    workOutWidth(pva, *pHalfRadianFOV, boardRadAngle, aspectRatio, p);
}

extern void
SetupFlag(void)
{
    int i;
    float width = FLAG_WIDTH;
    float height = FLAG_HEIGHT;

	for (i = 0; i < S_NUMSEGMENTS; i++) {
		flag.ctlpoints[i][0][0] = width / (S_NUMSEGMENTS - 1) * (float)i;
		flag.ctlpoints[i][1][0] = width / (S_NUMSEGMENTS - 1) * (float)i;
		flag.ctlpoints[i][0][1] = 0;
		flag.ctlpoints[i][1][1] = height;
		flag.ctlpoints[i][0][2] = 0;
		flag.ctlpoints[i][1][2] = 0;
	}
}

static void
getCheqSize(renderdata * prd)
{
    unsigned int i, accuracy = (prd->curveAccuracy / 4) - 1;
    prd->acrossCheq = prd->downCheq = 1;
    for (i = 1; i < accuracy; i++) {
        if (2 * prd->acrossCheq > prd->downCheq)
            prd->downCheq++;
        else
            prd->acrossCheq++;
    }
}

static void
Create3dModels(BoardData3d* bd3d, renderdata* prd)
{
	ModelManagerStart(&bd3d->modelHolder);

	CALL_OGL(&bd3d->modelHolder, MT_BACKGROUND, drawBackground, prd, bd3d->backGroundPos, bd3d->backGroundSize);
	CALL_OGL(&bd3d->modelHolder, MT_BASE, drawBoardBase, prd);
	CALL_OGL(&bd3d->modelHolder, MT_ODDPOINTS, drawPoints, prd, 0);
	CALL_OGL(&bd3d->modelHolder, MT_EVENPOINTS, drawPoints, prd, 1);
	CALL_OGL(&bd3d->modelHolder, MT_TABLE, drawTableModel, prd, &bd3d->boardPoints);
	if (prd->fHinges3d)
	{
		CALL_OGL(&bd3d->modelHolder, MT_HINGE, drawHinge, prd);
		CALL_OGL(&bd3d->modelHolder, MT_HINGEGAP, drawHingeGap);
	}
	else
	{
		OglModelInit(&bd3d->modelHolder, MT_HINGE);
		OglModelInit(&bd3d->modelHolder, MT_HINGEGAP);
	}

	CALL_OGL(&bd3d->modelHolder, MT_CUBE, renderCube, prd, DOUBLECUBE_SIZE);
	CALL_OGL(&bd3d->modelHolder, MT_MOVEINDICATOR, drawMoveIndicator);

	if (prd->ChequerMat[0].pTexture && prd->pieceTextureType == PTT_TOP)
	{
		CALL_OGL(&bd3d->modelHolder, MT_PIECETOP, preDrawPiece, bd3d, prd, PTT_TOP);
		CALL_OGL(&bd3d->modelHolder, MT_PIECE, preDrawPiece, bd3d, prd, PTT_BOTTOM);
	}
	else
	{
		OglModelInit(&bd3d->modelHolder, MT_PIECETOP);
		CALL_OGL(&bd3d->modelHolder, MT_PIECE, preDrawPiece, bd3d, prd, PTT_ALL);
	}

	CALL_OGL(&bd3d->modelHolder, MT_DICE, renderDice, prd);
	CALL_OGL(&bd3d->modelHolder, MT_DOT, renderDot, prd);

	CALL_OGL(&bd3d->modelHolder, MT_FLAG, drawFlagVertices, prd->curveAccuracy);
	CALL_OGL(&bd3d->modelHolder, MT_FLAGPOLE, drawFlagPole, prd->curveAccuracy);

	CALL_OGL(&bd3d->modelHolder, MT_RECT, drawSimpleRect);

	ModelManagerCreate(&bd3d->modelHolder);
}

void
preDraw3d(const BoardData * bd, BoardData3d * bd3d, renderdata * prd)
{
	freeEigthPoints(&bd3d->boardPoints);
	calculateEigthPoints(&bd3d->boardPoints, BOARD_FILLET, prd->curveAccuracy);

	getCheqSize(prd);
	Create3dModels(bd3d, prd);

#if !GTK_CHECK_VERSION(3,0,0)
	MakeShadowModel(bd, bd3d, prd);
#endif
	RecalcViewingVolume(bd);
}

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glib.h>

extern void PopulateMesh(const Vectoriser* pVect, Mesh* pMesh);

int RenderGlyph(const FT_Outline* pOutline)
{
	Mesh mesh;
	unsigned int index, point;

	Vectoriser vect;
	PopulateVectoriser(&vect, pOutline);
	if ((vect.contours->len < 1) || (vect.numPoints < 3))
		return 0;

	/* Solid font */
	PopulateMesh(&vect, &mesh);

	glNormal3f(0.0f, 0.0f, 1.0f);

	for (index = 0; index < mesh.tesselations->len; index++) {
		Tesselation* subMesh = &g_array_index(mesh.tesselations, Tesselation, index);

		glBegin(subMesh->meshType);
		for (point = 0; point < subMesh->tessPoints->len; point++) {
			Point3d* pPoint = &g_array_index(subMesh->tessPoints, Point3d, point);

			g_assert(pPoint->data[2] == 0);
			glVertex2f((float)pPoint->data[0] / 64.0f, (float)pPoint->data[1] / 64.0f);
		}
		glEnd();
	}

	TidyMemory(&vect, &mesh);

	return 1;
}

static void
SetupPerspVolume(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd, float** projMat, float** modelMat, float aspectRatio)
{
	if (!prd->planView) {
		viewArea va;
		float halfRadianFOV;
		float fovScale;
		float zoom;

		/* Workout how big the board is (in 3d space) */
		WorkOutViewArea(bd, &va, &halfRadianFOV, aspectRatio);

		fovScale = zNearVAL * tanf(halfRadianFOV);

		if (aspectRatio > getAreaRatio(&va)) {
			bd3d->vertFrustrum = fovScale;
			bd3d->horFrustrum = bd3d->vertFrustrum * aspectRatio;
		}
		else {
			bd3d->horFrustrum = fovScale * getAreaRatio(&va);
			bd3d->vertFrustrum = bd3d->horFrustrum / aspectRatio;
		}

		/* Setup projection matrix */
		glFrustum(-bd3d->horFrustrum, bd3d->horFrustrum, -bd3d->vertFrustrum, bd3d->vertFrustrum, zNearVAL, zFarVAL);

		/* Setup modelview matrix */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		/* Zoom back so image fills window */
		zoom = (getViewAreaHeight(&va) / 2) / tanf(halfRadianFOV);
		glTranslatef(0.f, 0.f, -zoom);

		/* Offset from centre because of perspective */
		glTranslatef(0.f, getViewAreaHeight(&va) / 2 + va.bottom, 0.f);

		/* Rotate board */
		glRotatef((float)prd->boardAngle, -1.f, 0.f, 0.f);

		/* Origin is bottom left, so move from centre */
		glTranslatef(-(getBoardWidth() / 2.0f), -((getBoardHeight()) / 2.0f), 0.f);
	}
	else {
		float size;

		if (aspectRatio > getBoardWidth() / getBoardHeight()) {
			size = (getBoardHeight() / 2);
			bd3d->horFrustrum = size * aspectRatio;
			bd3d->vertFrustrum = size;
		}
		else {
			size = (getBoardWidth() / 2);
			bd3d->horFrustrum = size;
			bd3d->vertFrustrum = size / aspectRatio;
		}
		glOrtho(-bd3d->horFrustrum, bd3d->horFrustrum, -bd3d->vertFrustrum, bd3d->vertFrustrum, 0.0, 5.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(-(getBoardWidth() / 2.0f), -(getBoardHeight() / 2.0f), -3.f);
	}

	*projMat = GetProjectionMatrix();
	*modelMat = GetModelViewMatrix();
}

void
SetupViewingVolume3dNew(const BoardData* bd, BoardData3d* bd3d, const renderdata* prd, float** projMat, float** modelMat, int viewport[4])
{
	float aspectRatio = (float)viewport[2] / (float)viewport[3];
	if (!prd->planView) {
		if (aspectRatio < .5f) {        /* Viewing area to high - cut down so board rendered correctly */
			int newHeight = viewport[2] * 2;
			viewport[1] = (viewport[3] - newHeight) / 2;
			viewport[3] = newHeight;
			glSetViewport(viewport);
			aspectRatio = .5f;
			/* Clear screen so top + bottom outside board shown ok */
			ClearScreen(prd);
		}
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	SetupPerspVolume(bd, bd3d, prd, projMat, modelMat, aspectRatio);
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static void gluPickMatrix(GLfloat x, GLfloat y, GLfloat deltax, GLfloat deltay, const int* viewport)
{
	if (deltax <= 0 || deltay <= 0) {
		return;
	}

	/* Translate and scale the picked region to the entire window */
	glTranslatef(((GLfloat)viewport[2] - 2 * (x - (GLfloat)viewport[0])) / deltax, ((GLfloat)viewport[3] - 2 * (y - (GLfloat)viewport[1])) / deltay, 0);
	glScalef((GLfloat)viewport[2] / deltax, (GLfloat)viewport[3] / deltay, 1.0);
}

void getPickMatrices(float x, float y, const BoardData* bd, int* viewport, float** projMat, float** modelMat)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPickMatrix(x, y, 1.0f, 1.0f, viewport);

	SetupPerspVolume(bd, bd->bd3d, bd->rd, projMat, modelMat, (float)viewport[2] / (float)viewport[3]);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
}
#endif

void drawTableBase(const ModelManager* modelHolder, const BoardData3d* UNUSED(bd3d), const renderdata* prd)
{
	OglModelDraw(modelHolder, MT_BACKGROUND, &prd->BackGroundMat);

	/* Left hand side points */
	OglModelDraw(modelHolder, MT_BASE, &prd->BaseMat);
	OglModelDraw(modelHolder, MT_ODDPOINTS, &prd->PointMat[0]);
	OglModelDraw(modelHolder, MT_EVENPOINTS, &prd->PointMat[1]);

	/* Rotate around for right board */
	glPushMatrix();
	glTranslatef(TOTAL_WIDTH, TOTAL_HEIGHT, 0.f);
	glRotatef(180.f, 0.f, 0.f, 1.f);
	OglModelDraw(modelHolder, MT_BASE, &prd->BaseMat);
	OglModelDraw(modelHolder, MT_ODDPOINTS, &prd->PointMat[0]);
	OglModelDraw(modelHolder, MT_EVENPOINTS, &prd->PointMat[1]);
	glPopMatrix();
}

extern void drawHinges(const ModelManager* modelHolder, const BoardData3d* UNUSED(bd3d), const renderdata* prd)
{
	glPushMatrix();
		glTranslatef((TOTAL_WIDTH) / 2.0f, ((TOTAL_HEIGHT / 2.0f) - HINGE_HEIGHT) / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		OglModelDraw(modelHolder, MT_HINGE, &prd->HingeMat);
	glPopMatrix();
	glPushMatrix();
		glTranslatef((TOTAL_WIDTH) / 2.0f, ((TOTAL_HEIGHT / 2.0f) - HINGE_HEIGHT + TOTAL_HEIGHT) / 2.0f, BASE_DEPTH + EDGE_DEPTH);
		OglModelDraw(modelHolder, MT_HINGE, &prd->HingeMat);
	glPopMatrix();
}

#ifndef TEST_HARNESS
static 
#endif
void drawDCNumbers(const BoardData* bd, const diceTest* dt, int MAA, int nCube)
{
	int c;
	float radius = DOUBLECUBE_SIZE / 7.0f;
	float ds = (DOUBLECUBE_SIZE * 5.0f / 7.0f);
	float hds = (ds / 2);
	float depth = hds + radius;

	const char* sides[2][6] = {{ "4", "16", "32", "64", "8", "2" }, { "256", "1024", "2048", "4096", "512", "128" }};
	int side;

	glPushMatrix();
	for (c = 0; c < 6; c++) {
		if (c < 3)
			side = c;
		else
			side = 8 - c;

		/* Don't draw bottom number or back number (unless cube at bottom) */
		if (side != dt->bottom && !(side == dt->side[0] && bd->cube_owner != -1)) {
			glPushMatrix();
			glTranslatef(0.f, 0.f, depth + LIFT_OFF);

			glPrintCube(&bd->bd3d->cubeFont, sides[nCube][side], MAA);

			glPopMatrix();
		}
		if (c % 2 == 0)
			glRotatef(-90.f, 0.f, 1.f, 0.f);
		else
			glRotatef(90.f, 1.f, 0.f, 0.f);
	}
	glPopMatrix();
}

static void
DrawDCNumbers(const BoardData* bd)
{
	diceTest dt;
	int extraRot = 0;
	int rotDC[6][3] = { {1, 0, 0}, {2, 0, 3}, {0, 0, 0}, {0, 3, 1}, {0, 1, 0}, {3, 0, 3} };

	int nCube, cubeIndex;

	/* Rotate to correct number + rotation */
	if (!bd->doubled) {
		cubeIndex = LogCube(bd->cube);
		extraRot = bd->cube_owner + 1;
	}
	else {
		cubeIndex = LogCube(bd->cube * 2);      /* Show offered cube value */
		extraRot = bd->turn + 1;
	}

	g_assert(cubeIndex <= 12);

        if (cubeIndex > 6)
		nCube = 1;
	else
		nCube = 0;

        cubeIndex = cubeIndex % 6;

	glRotatef((float)(rotDC[cubeIndex][2] + extraRot) * 90.0f, 0.f, 0.f, 1.f);
	glRotatef((float)rotDC[cubeIndex][0] * 90.0f, 1.f, 0.f, 0.f);
	glRotatef((float)rotDC[cubeIndex][1] * 90.0f, 0.f, 1.f, 0.f);

	initDT(&dt, rotDC[cubeIndex][0], rotDC[cubeIndex][1], rotDC[cubeIndex][2] + extraRot);

	setMaterial(&bd->rd->CubeNumberMat);
	drawDCNumbers(bd, &dt, 0, nCube);

#if !GTK_CHECK_VERSION(3,0,0)
	LegacyStartAA(0.5f);
	drawDCNumbers(bd, &dt, 1, nCube);
	LegacyEndAA();
#endif
}

static void
moveToDoubleCubePos(const BoardData* bd)
{
	float v[3];
	getDoubleCubePos(bd, v);
	glTranslatef(v[0], v[1], v[2]);
}

static void
moveToDicePos(const BoardData* bd, int num, float diceRotation)
{
	float v[3];
	getDicePos(bd, num, v);
	glTranslatef(v[0], v[1], v[2]);

	if (diceRotation != 0)
		glRotatef(diceRotation, 0.f, 0.f, 1.f); /* Spin dice to required rotation if on board */
}

extern void
drawDC(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* UNUSED(bd3d), const Material* cubeMat, int drawNumbers)
{
	glPushMatrix();
	moveToDoubleCubePos(bd);

	OglModelDraw(modelHolder, MT_CUBE, cubeMat);

	if (drawNumbers)
		DrawDCNumbers(bd);

	glPopMatrix();
}

void ShowMoveIndicator(const ModelManager* modelHolder, const BoardData* bd)
{
	float pos[3];

	glPushMatrix();
	getMoveIndicatorPos(bd->turn, pos);
	glTranslatef(pos[0], pos[1], pos[2]);
	if (fClockwise)
		glRotatef(180.f, 0.f, 0.f, 1.f);

	OglModelDraw(modelHolder, MT_MOVEINDICATOR, &bd->rd->ChequerMat[(bd->turn == 1) ? 1 : 0]);

#if !GTK_CHECK_VERSION(3,0,0)
	//TODO: Enlarge and draw black outline before overlaying arrow in modern OGL...
	MAAmoveIndicator();
#endif

	glPopMatrix();
}

extern void
drawPiece(const ModelManager* modelHolder, const BoardData3d* bd3d, unsigned int point, unsigned int pos, int rotate, int roundPiece, int curveAccuracy, int separateTop)
{
	float v[3];
	glPushMatrix();

	getPiecePos(point, pos, v);
	glTranslatef(v[0], v[1], v[2]);

	/* Home pieces are sideways */
	if (point == 26)
		glRotatef(-90.f, 1.f, 0.f, 0.f);
	if (point == 27)
		glRotatef(90.f, 1.f, 0.f, 0.f);

	if (rotate)
		glRotatef((float)bd3d->pieceRotation[point][pos - 1], 0.f, 0.f, 1.f);

	renderPiece(modelHolder, separateTop);

#if !GTK_CHECK_VERSION(3,0,0)
	MAApiece(roundPiece, curveAccuracy);
#else
	(void)roundPiece;	/* suppress unused parameter compiler warning */
	(void)curveAccuracy;	/* suppress unused parameter compiler warning */
#endif

	glPopMatrix();
}

extern void
renderSpecialPieces(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd)
{
	int separateTop = (prd->ChequerMat[0].pTexture && prd->pieceTextureType == PTT_TOP);

	if (bd->drag_point >= 0) {
		glPushMatrix();
		glTranslatef(bd3d->dragPos[0], bd3d->dragPos[1], bd3d->dragPos[2]);
		glRotatef((float)bd3d->movingPieceRotation, 0.f, 0.f, 1.f);
		setMaterial(&prd->ChequerMat[(bd->drag_colour == 1) ? 1 : 0]);
		renderPiece(modelHolder, separateTop);
		glPopMatrix();
	}

	if (bd3d->moving) {
		glPushMatrix();
		glTranslatef(bd3d->movingPos[0], bd3d->movingPos[1], bd3d->movingPos[2]);
		if (bd3d->rotateMovingPiece > 0)
			glRotatef(-90 * bd3d->rotateMovingPiece * (float)bd->turn, 1.f, 0.f, 0.f);
		glRotatef((float)bd3d->movingPieceRotation, 0.f, 0.f, 1.f);
		setMaterial(&prd->ChequerMat[(bd->turn == 1) ? 1 : 0]);
		renderPiece(modelHolder, separateTop);
		glPopMatrix();
	}
}

#if GTK_CHECK_VERSION(3, 0, 0)
extern void DrawDotTemp(const ModelManager* modelHolder, float dotSize, float ds, float zOffset, int* dp, int c)
{
	float hds = (ds / 2);

	if (c == 0)
		glPushMatrix();

	glPushMatrix();
	glTranslatef(0.f, 0.f, zOffset);

	glNormal3f(0.f, 0.f, 1.f);
	/* Show all the dots for this number */
	do {
		const float dot_pos[] = { 0, 20, 50, 80 };       /* percentages across face */
		float x = (dot_pos[dp[0]] * ds) / 100;
		float y = (dot_pos[dp[1]] * ds) / 100;

		glPushMatrix();
		glTranslatef(x - hds, y - hds, 0.f);

		glScalef(dotSize, dotSize, dotSize);
		OglModelDraw(modelHolder, MT_DOT, GetCurrentMaterial());

		glPopMatrix();

		dp += 2;
	} while (*dp);
	glPopMatrix();

	if (c % 2 == 0)
		glRotatef(-90.f, 0.f, 1.f, 0.f);
	else
		glRotatef(90.f, 1.f, 0.f, 0.f);

	if (c == 5)
		glPopMatrix();
}
#endif

extern void
drawDie(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* prd, const Material* diceMat, int num, int drawDots)
{
	diceTest dt;
	int z;
	int rotDice[6][2] = { {0, 0}, {0, 1}, {3, 0}, {1, 0}, {0, 3}, {2, 0} };
	unsigned int value = bd->diceRoll[num];
	int diceCol = (bd->turn == 1);
	/* During program startup value may be zero, if so don't draw */
	if (!value)
		return;
	value--;	/* Zero based for array access */

	/* Get dice rotation */
	if (bd->diceShown == DICE_BELOW_BOARD)
		z = 0;
	else
		z = ((int)bd3d->dicePos[num][2] + 45) / 90;
	/* DT = dice test, use to work out which way up the dice is */
	initDT(&dt, rotDice[value][0], rotDice[value][1], z);

	glPushMatrix();
	/* Move to correct position for die */
	if (bd3d->shakingDice) {
		glTranslatef(bd3d->diceMovingPos[num][0], bd3d->diceMovingPos[num][1], bd3d->diceMovingPos[num][2]);
		glRotatef(bd3d->diceRotation[num].xRot, 0.f, 1.f, 0.f);
		glRotatef(bd3d->diceRotation[num].yRot, 1.f, 0.f, 0.f);
		glRotatef(bd3d->dicePos[num][2], 0.f, 0.f, 1.f);
	}
	else
		moveToDicePos(bd, num, (bd->diceShown == DICE_ON_BOARD) ? bd->bd3d->dicePos[num][2] : 0);

	/* Orientate dice correctly */
	glRotatef(90.0f * (float)rotDice[value][0], 1.f, 0.f, 0.f);
	glRotatef(90.0f * (float)rotDice[value][1], 0.f, 1.f, 0.f);

	if (diceMat->alphaBlend) { /* Draw back of dice separately */
		DrawBackDice(modelHolder, bd3d, prd, &dt, diceCol);
	}
	/* Draw dice */
	OglModelDraw(modelHolder, MT_DICE, diceMat);

#if !GTK_CHECK_VERSION(3,0,0)
	MAAdie(prd);
#endif

	if (drawDots)
		DrawDots(modelHolder, bd3d, prd, &dt, diceCol);

	glPopMatrix();
}

extern void MoveToFlagMiddle(void)
{
	float v[3];
	/* Move to middle of flag */
	int midPoint = (S_NUMSEGMENTS - 1) / 2;
	v[0] = flag.ctlpoints[midPoint][0][0];
	v[1] = (flag.ctlpoints[0][0][1] + flag.ctlpoints[0][1][1]) / 2.0f;
	v[2] = flag.ctlpoints[midPoint][0][2];
	glTranslatef(v[0], v[1], v[2]);

	/* Work out approx angle of number based on control points */
	float ang =
		atanf(-(flag.ctlpoints[midPoint + 1][0][2] - flag.ctlpoints[midPoint][0][2]) /
		(flag.ctlpoints[midPoint + 1][0][0] - flag.ctlpoints[midPoint][0][0]));
	float degAng = (ang) * 180 / F_PI;

	glRotatef(degAng, 0.f, 1.f, 0.f);
}

static void
MoveToFlagPos(int turn)
{
	float v[3];
	getFlagPos(turn, v);
	glTranslatef(v[0], v[1], v[2]);
}

extern void
renderFlag(const ModelManager* modelHolder, const BoardData3d* bd3d, int UNUSED(curveAccuracy), int turn, int resigned)
{
	glPushMatrix();
		MoveToFlagPos(turn);

		/* Draw flag surface */
		OglModelDraw(modelHolder, MT_FLAG, &bd3d->flagMat);

		/* Draw flag pole */
		OglModelDraw(modelHolder, MT_FLAGPOLE, &bd3d->flagPoleMat);

		renderFlagNumbers(bd3d, resigned);
	glPopMatrix();	/* Move back to origin */
}

extern void
drawFlag(const ModelManager* modelHolder, const BoardData* bd, const BoardData3d* bd3d, const renderdata* UNUSED(prd))
{
	if (bd->resigned)
	{
		waveFlag(bd->bd3d->flagWaved);
		UPDATE_OGL(&bd->bd3d->modelHolder, MT_FLAG, drawFlagVertices, bd->rd->curveAccuracy);	/* Update waving flag */
	}

	renderFlag(modelHolder, bd3d, bd->rd->curveAccuracy, bd->turn, bd->resigned);
}

extern void
renderRect(const ModelManager* modelHolder, float x, float y, float z, float w, float h)
{
	glPushMatrix();

	glTranslatef(x + w / 2, y + h / 2, z);
	glScalef(w / 2.0f, h / 2.0f, 1.f);
	glNormal3f(0.f, 0.f, 1.f);

	OglModelDraw(modelHolder, MT_RECT, NULL);

	glPopMatrix();
}
