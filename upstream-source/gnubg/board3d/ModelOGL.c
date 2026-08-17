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
 * $Id: ModelOGL.c,v 1.9 2021/02/21 10:56:47 plm Exp $
 */

#include "config.h"
#include "legacyGLinc.h"
#include <stdlib.h>
#include "fun3d.h"

void OglModelInit(ModelManager* modelHolder, int modelNumber)
{
	modelHolder->models[modelNumber].data = NULL;
	modelHolder->models[modelNumber].dataLength = 0;

	modelHolder->numModels++;
}

void OglModelAlloc(ModelManager* modelHolder, int modelNumber)
{
	modelHolder->totalNumVertices += modelHolder->models[modelNumber].dataLength;

	modelHolder->models[modelNumber].data = g_malloc(sizeof(float) * modelHolder->models[modelNumber].dataLength);
	modelHolder->models[modelNumber].dataLength = 0;
}

void ModelManagerInit(ModelManager* modelHolder)
{
	modelHolder->allocNumVertices = 0;
	modelHolder->vertexData = NULL;

#if GTK_CHECK_VERSION(3,0,0)
	modelHolder->vao = GL_INVALID_VALUE;
#endif
}

void ModelManagerStart(ModelManager* modelHolder)
{
	InitMatStacks();

	modelHolder->totalNumVertices = 0;
	modelHolder->numModels = 0;
}

#if !GTK_CHECK_VERSION(3,0,0)
void OglModelDraw(const ModelManager* modelManager, int modelNumber, const Material* pMat)
{
	float* data = &modelManager->vertexData[modelManager->models[modelNumber].dataStart];
	int dataLength = modelManager->models[modelNumber].dataLength;

	int usesTexture = (pMat && pMat->pTexture);
	setMaterial(pMat);

	glPushMatrix();
	glLoadMatrixf(GetModelViewMatrix());

	glBegin(GL_TRIANGLES);
	for (int vertex = 0; vertex < dataLength / VERTEX_STRIDE; vertex++)
	{
		if (usesTexture)
		{
			glTexCoord2f(data[0], data[1]);
		}
		glNormal3f(data[2], data[3], data[4]);
		glVertex3f(data[5], data[6], data[7]);
		data += VERTEX_STRIDE;
	}
	glEnd();

	glPopMatrix();
}

void ModelManagerCopyModelToBuffer(ModelManager* modelHolder, int modelNumber)
{
	memcpy(&modelHolder->vertexData[modelHolder->models[modelNumber].dataStart], modelHolder->models[modelNumber].data, modelHolder->models[modelNumber].dataLength * sizeof(float));
	g_free(modelHolder->models[modelNumber].data);
	modelHolder->models[modelNumber].data = NULL;
}

void ModelManagerCreate(ModelManager* modelHolder)
{
	int model;
	if (modelHolder->totalNumVertices > modelHolder->allocNumVertices)
	{
		modelHolder->allocNumVertices = modelHolder->totalNumVertices;
		g_free(modelHolder->vertexData);
		modelHolder->vertexData = g_malloc(sizeof(float) * modelHolder->allocNumVertices);
	}
	/* Copy data into one big buffer */
	int vertexPos = 0;
	for (model = 0; model < modelHolder->numModels; model++)
	{
		modelHolder->models[model].dataStart = vertexPos;
		ModelManagerCopyModelToBuffer(modelHolder, model);
		vertexPos += modelHolder->models[model].dataLength;
	}
	g_assert(vertexPos == modelHolder->totalNumVertices);
}
#endif
