/*
 * Copyright (C) 2006-2009 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2006-2023 the AUTHORS
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
 * $Id: gtkfile.h,v 1.13 2023/09/05 20:34:21 plm Exp $
 */

#ifndef GTKFILE_H
#define GTKFILE_H
extern void GTKOpen(gpointer p, guint n, GtkWidget * pw);
extern void GTKCommandsOpen(gpointer p, guint n, GtkWidget * pw);
extern void GTKSave(gpointer p, guint n, GtkWidget * pw);
extern char *GTKFileSelect(const gchar * prompt, const gchar * extension, const gchar * folder,
                           const gchar * name, GtkFileChooserAction action);
extern void SetDefaultFileName(char *path);
extern void GTKAnalyzeCurrent(void);
extern void GTKAnalyzeFile(void);
extern void GTKBatchAnalyse(gpointer p, guint n, GtkWidget * pw);

#endif
