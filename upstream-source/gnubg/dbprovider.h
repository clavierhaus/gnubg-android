/*
 * Copyright (C) 2008-2009 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2008-2018 the AUTHORS
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
 * $Id: dbprovider.h,v 1.21 2022/01/19 22:42:28 plm Exp $
 */

#ifndef DBPROVIDER_H
#define DBPROVIDER_H

#include <stdio.h>
#include <glib.h>
extern int storeGameStats;

typedef struct {
    size_t cols, rows;
    char ***data;
    size_t *widths;
} RowSet;

typedef struct {
    int (*Connect) (const char *database, const char *user, const char *password, const char *hostname);
    void (*Disconnect) (void);
    RowSet *(*Select) (const char *str);
    int (*UpdateCommand) (const char *str);
    void (*Commit) (void);
    GList *(*GetDatabaseList) (const char *user, const char *password, const char *hostname);
    int (*DeleteDatabase) (const char *database, const char *user, const char *password, const char *hostname);

    const char *name;
    const char *shortname;
    const char *desc;
    int HasUserDetails;
    int storeGameStats;
    char *database;
    char *username;
    char *password;
    char *hostname;
} DBProvider;

typedef enum {
    INVALID_PROVIDER = -1,
#if defined(USE_SQLITE)
    SQLITE,
#endif
#if defined(USE_PYTHON)
#if !defined(USE_SQLITE)
    PYTHON_SQLITE,
#endif
    PYTHON_MYSQL,
    PYTHON_POSTGRES
#endif
} DBProviderType;

#if defined(USE_PYTHON)
#define NUM_PROVIDERS 3
#elif defined(USE_SQLITE)
#define NUM_PROVIDERS 1
#else
#define NUM_PROVIDERS 0
#endif

extern DBProviderType dbProviderType;

DBProvider *GetDBProvider(DBProviderType dbType);
const char *TestDB(DBProviderType dbType);
DBProvider *ConnectToDB(DBProviderType dbType);

void DefaultDBSettings(void);
void SetDBType(const char *type);
void SetDBSettings(DBProviderType dbType, const char *database, const char *user, const char *password,
                   const char *hostname);
void RelationalSaveSettings(FILE * pf);
void SetDBParam(const char *db, const char *key, const char *value);
extern int CreateDatabase(DBProvider * pdb);
const char *GetProviderName(int i);
extern RowSet *RunQuery(const char *sz);
extern int RunQueryValue(const DBProvider * pdb, const char *query);
extern void FreeRowset(RowSet * pRow);
#endif
