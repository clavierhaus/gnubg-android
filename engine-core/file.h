/*
 * Copyright (C) 2007 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2007-2013 the AUTHORS
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

#ifndef FILE_H
#define FILE_H

typedef enum {
    EXPORT_SGF,
    EXPORT_HTML,
    EXPORT_GAM,
    EXPORT_MAT,
    EXPORT_POS,
    EXPORT_LATEX,
    EXPORT_PDF,
    EXPORT_TEXT,
    EXPORT_PNG,
    EXPORT_PS,
    EXPORT_SNOWIETXT,
    EXPORT_SVG,
    N_EXPORT_TYPES
} ExportType;

typedef enum {
    IMPORT_SGF,
    IMPORT_SGG,
    IMPORT_MAT,
    IMPORT_OLDMOVES,
    IMPORT_POS,
    IMPORT_SNOWIETXT,
    IMPORT_TMG,
    IMPORT_EMPIRE,
    IMPORT_PARTY,
    IMPORT_BGROOM,
    N_IMPORT_TYPES
} ImportType;

typedef struct {
    ExportType type;
    const char *extension;
    const char *description;
    const char *clname;
    int exports[3];
} ExportFormat;

typedef struct {
    ImportType type;
    const char *extension;
    const char *description;
    const char *clname;
} ImportFormat;

extern ExportFormat export_format[];
extern ImportFormat import_format[];

typedef struct {
    ImportType type;
} FilePreviewData;

extern char *GetFilename(int CheckForCurrent, ExportType type, int extens);
extern FilePreviewData *ReadFilePreview(const char *filename);

#endif
