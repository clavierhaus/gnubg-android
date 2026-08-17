/* $Id: tr.h,v 1.8 2019/11/09 22:23:55 Superfly_Jon Exp $ */

/*
 * $originalLog: tr.h,v $
 * Revision 1.5  1997/07/21  17:34:07  brianp
 * added tile borders, incremented version to 1.1
 *
 * Revision 1.4  1997/07/21  15:47:35  brianp
 * renamed all "near" and "far" variables
 *
 * Revision 1.3  1997/04/26  21:23:25  brianp
 * added trRasterPos3f function
 *
 * Revision 1.2  1997/04/19  23:26:10  brianp
 * many API changes
 *
 * Revision 1.1  1997/04/18  21:53:05  brianp
 * Initial revision
 *
 */


/*
 * Tiled Rendering library
 * Version 1.1
 * Copyright (C) 1997 Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Notice added by Russ Allbery on 2013-07-21 based on the license information
 * in tr-1.3.tar.gz retrieved from <http://www.mesa3d.org/brianp/TR.html>.
 *
 *
 * This library allows one to render arbitrarily large images with OpenGL.
 * The basic idea is to break the image into tiles which are rendered one
 * at a time.  The tiles are assembled together to form the final, large
 * image.  Tiles and images can be of any size.
 *
 * Basic usage:
 *
 * 1. Allocate a tile rendering context:
 *       TRcontext t = trNew();
 *
 * 2. Specify the final image buffer and tile size:
 *       GLubyte image[W][H][4]
 *       trImageSize(t, W, H);
 *       trImageBuffer(t, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *) image);
 *
 * 3. Setup your projection:
 *       trFrustum(t, left, right, bottom top, near, far);
 *    or
 *       trOrtho(t, left, right, bottom top, near, far);
 *    or
 *       trPerspective(t, fovy, aspect, near, far);
 *
 * 4. Render the tiles:
 *       do {
 *           trBeginTile(t);
 *           DrawMyScene();
 *       } while (trEndTile(t));
 *
 *    You provide the DrawMyScene() function which calls glClear() and
 *    draws all your stuff.
 *
 * 5. The image array is now complete.  Display it, write it to a file, etc.
 *
 * 6. Delete the tile rendering context when finished:
 *       trDelete(t);
 *
 */


#ifndef TR_H
#define TR_H



#ifdef __cplusplus
extern "C" {
#endif


#define TR_VERSION "1.1"
#define TR_MAJOR_VERSION 1
#define TR_MINOR_VERSION 1


    typedef enum {
        TR_TILE_WIDTH = 100,
        TR_TILE_HEIGHT,
        TR_TILE_BORDER,
        TR_IMAGE_WIDTH,
        TR_IMAGE_HEIGHT,
        TR_ROWS,
        TR_COLUMNS,
        TR_CURRENT_ROW,
        TR_CURRENT_COLUMN,
        TR_CURRENT_TILE_WIDTH,
        TR_CURRENT_TILE_HEIGHT,
        TR_ROW_ORDER,
        TR_TOP_TO_BOTTOM,
        TR_BOTTOM_TO_TOP
    } TRenum;



    typedef struct {
        /* Final image parameters */
        int ImageWidth, ImageHeight;
        GLenum ImageFormat, ImageType;
        GLvoid *ImageBuffer;

        /* Tile parameters */
        int TileWidth, TileHeight;
        int TileWidthNB, TileHeightNB;
        int TileBorder;
        GLenum TileFormat, TileType;
        GLvoid *TileBuffer;

        /* Projection parameters */
        GLboolean Perspective;
        GLdouble Left;
        GLdouble Right;
        GLdouble Bottom;
        GLdouble Top;
        GLdouble Near;
        GLdouble Far;

        /* Misc */
        TRenum RowOrder;
        int Rows, Columns;
        int CurrentTile;
        int CurrentTileWidth, CurrentTileHeight;
        int CurrentRow, CurrentColumn;

        int ViewportSave[4];
    } TRcontext;


    extern TRcontext *trNew(void);

    extern void trDelete(TRcontext * tr);


    extern void trTileSize(TRcontext * tr, int width, int height, int border);

#if 0
    extern void trTileBuffer(TRcontext * tr, GLenum format, GLenum type, GLvoid * image);
#endif

    extern void trImageSize(TRcontext * tr, unsigned int width, unsigned int height);

    extern void trImageBuffer(TRcontext * tr, GLenum format, GLenum type, GLvoid * image);

#if 0
    extern void trRowOrder(TRcontext * tr, TRenum order);


    extern int trGet(const TRcontext * tr, TRenum param);
#endif

    extern void trOrtho(TRcontext * tr,
                        GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble znear, GLdouble zfar);

    extern void trFrustum(TRcontext * tr,
                          GLdouble left, GLdouble right,
                          GLdouble bottom, GLdouble top, GLdouble znear, GLdouble zfar);

#if 0
    extern void trPerspective(TRcontext * tr, GLdouble fovy, GLdouble aspect, GLdouble zNearx, GLdouble zFarx);
#endif

    extern void trBeginTile(TRcontext * tr);

    extern int trEndTile(TRcontext * tr);

#if 0
    extern void trRasterPos3d(const TRcontext * tr, GLdouble x, GLdouble y, GLdouble z);
#endif


#ifdef __cplusplus
}
#endif
#endif
