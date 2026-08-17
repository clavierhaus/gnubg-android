/*
 * Copyright (C) 2007-2008 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2007-2018 the AUTHORS
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
 * $Id: multithread.h,v 1.62 2021/12/18 19:37:38 plm Exp $
 */

#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include "config.h"

#if defined(WIN32)
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#endif

#define UI_UPDATETIME 250

#if defined(USE_MULTITHREAD)
#include <glib.h>
#endif

#include "backgammon.h"

/* #define DEBUG_MULTITHREADED 1 */

#if defined (USE_MULTITHREAD) && defined(DEBUG_MULTITHREADED)
void multi_debug(const char *str, ...);
#else
#define multi_debug(x)
#endif

typedef struct Task {
    AsyncFun fun;
    void *data;
    struct Task *pLinkedTask;
} Task;

typedef struct {
    Task task;
    moverecord *pmr;
    listOLD *plGame;
    statcontext *psc;
    matchstate ms;
} AnalyseMoveTask;

typedef struct {
    int id;
    move *aMoves;
    NNState *pnnState;
} ThreadLocalData;

typedef struct {
#if GLIB_CHECK_VERSION (2,32,0)
    GCond cond;
#else
    GCond *cond;
#endif
    int signalled;
} * ManualEvent;	/* a ManualEvent is a pointer to this struct */

typedef GPrivate *TLSItem;

#if GLIB_CHECK_VERSION (2,32,0)
typedef GMutex Mutex;
#else
typedef GMutex *Mutex;
#endif

typedef struct {
    GList *tasks;
    int doneTasks;
    int result;
    ThreadLocalData *tld;

#if defined(USE_MULTITHREAD)
    ManualEvent activity;
    TLSItem tlsItem;
    Mutex queueLock;
    Mutex multiLock;
    ManualEvent syncStart;
    ManualEvent syncEnd;

    int addedTasks;
    int totalTasks;

    int closingThreads;
    unsigned int numThreads;
#endif
} ThreadData;

extern int MT_GetDoneTasks(void);
extern void MT_AbortTasks(void);
extern void MT_AddTask(Task * pt, gboolean lock);
extern void mt_add_tasks(unsigned int num_tasks, AsyncFun pFun, void *taskData, gpointer linked);
extern int MT_WaitForTasks(gboolean(*pCallback) (gpointer), int callbackTime, int autosave);
extern void MT_InitThreads(void);
extern void MT_Close(void);
extern void MT_CloseThreads(void);
extern void CloseThread(void *unused);
extern ThreadLocalData *MT_CreateThreadLocalData(int id);

extern ThreadData td;

#if defined(USE_MULTITHREAD)

extern void ResetManualEvent(ManualEvent ME);
extern void Mutex_Lock(Mutex *mutex);
extern void Mutex_Release(Mutex *mutex);
extern void WaitForManualEvent(ManualEvent ME);
extern void SetManualEvent(ManualEvent ME);
extern void TLSSetValue(TLSItem pItem, size_t value);
extern void InitManualEvent(ManualEvent * pME);
extern void FreeManualEvent(ManualEvent ME);
extern void InitMutex(Mutex * pMutex);
extern void FreeMutex(Mutex * mutex);

#define TLSGet(item) *((size_t*)g_private_get(item))

#if !defined(MAX_NUMTHREADS)
#define MAX_NUMTHREADS 48
#endif

extern void MT_Release(void);
extern void MT_Exclusive(void);
extern void MT_StartThreads(void);
extern void MT_SetNumThreads(unsigned int num);
extern void MT_SyncInit(void);
extern void MT_SyncStart(void);
extern double MT_SyncEnd(void);
extern void MT_SetResultFailed(void);
extern void TLSCreate(TLSItem * pItem);
extern unsigned int MT_GetNumThreads(void);

#define MT_GetTLD() ((ThreadLocalData *)TLSGet(td.tlsItem))
#define MT_GetThreadID() ((ThreadLocalData *)TLSGet(td.tlsItem))->id
#define MT_Get_nnState() ((ThreadLocalData *)TLSGet(td.tlsItem))->pnnState
#define MT_Get_aMoves() ((ThreadLocalData *)TLSGet(td.tlsItem))->aMoves

#if GLIB_CHECK_VERSION (2,30,0)
#define MT_SafeIncValue(x) (g_atomic_int_add(x, 1) + 1)
#define MT_SafeIncCheck(x) (g_atomic_int_add(x, 1))
#else
#define MT_SafeIncValue(x) (g_atomic_int_exchange_and_add(x, 1) + 1)
#define MT_SafeIncCheck(x) (g_atomic_int_exchange_and_add(x, 1))
#endif
/*
 * We don't use the value returned by the next three
 * Pre-2.30 void atomic_int_add() is ok
 */
#define MT_SafeInc(x) g_atomic_int_add(x, 1)
#define MT_SafeAdd(x, y) g_atomic_int_add(x, y)
#define MT_SafeDec(x) g_atomic_int_add(x, -1)
#define MT_SafeDecCheck(x) g_atomic_int_dec_and_test(x)
#define MT_SafeGet(x) g_atomic_int_get(x)
#define MT_SafeSet(x, y) g_atomic_int_set(x, y)
#define MT_SafeCompare(x, y) g_atomic_int_compare_and_exchange(x, y, y)

#else                           /*USE_MULTITHREAD */
#if !defined(MAX_NUMTHREADS)
#define MAX_NUMTHREADS 1
#endif
extern int asyncRet;
#define MT_Exclusive() {}
#define MT_Release() {}
#define MT_GetNumThreads() 1
#define MT_SetResultFailed() asyncRet = -1
#define MT_SafeInc(x) (++(*x))
#define MT_SafeIncValue(x) (++(*x))
#define MT_SafeIncCheck(x) ((*x)++)
#define MT_SafeAdd(x, y) ((*x) += y)
#define MT_SafeDec(x) (--(*x))
#define MT_SafeDecCheck(x) ((--(*x)) == 0)
#define MT_SafeGet(x) (*x)
#define MT_SafeSet(x, y) ((*x) = y)
#define MT_SafeCompare(x, y) ((*x) == y)
#define MT_GetThreadID() 0
#define MT_Get_nnState() td.tld->pnnState
#define MT_Get_aMoves() td.tld->aMoves
#define MT_GetTLD() td.tld

#endif

#endif
