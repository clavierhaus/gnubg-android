/*
 * Copyright (C) 2007-2009 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: mtsupport.c,v 1.23 2022/03/12 21:05:54 plm Exp $
 */

/*
 * Multithreaded support functions, moved out of multithread.c
 */

#include "config.h"
#include "multithread.h"

#include <stdlib.h>
#if defined (DEBUG_MULTITHREADED)
#include <stdio.h>
#endif
#include <string.h>

#include "lib/simd.h"

SSE_ALIGN(ThreadData td);

extern ThreadLocalData *
MT_CreateThreadLocalData(int id)
{
    ThreadLocalData *tld = (ThreadLocalData *) g_malloc(sizeof(ThreadLocalData));
    tld->id = id;
    tld->pnnState = (NNState *) g_malloc(sizeof(NNState) * 3);
    memset(tld->pnnState, 0, sizeof(NNState) * 3);
    // cppcheck-suppress duplicateExpression
    tld->pnnState[CLASS_RACE - CLASS_RACE].savedBase = g_malloc(nnRace.cHidden * sizeof(float));
    // cppcheck-suppress duplicateExpression
    memset(tld->pnnState[CLASS_RACE - CLASS_RACE].savedBase, 0, nnRace.cHidden * sizeof(float));
    // cppcheck-suppress duplicateExpression
    tld->pnnState[CLASS_RACE - CLASS_RACE].savedIBase = g_malloc(nnRace.cInput * sizeof(float));
    // cppcheck-suppress duplicateExpression
    memset(tld->pnnState[CLASS_RACE - CLASS_RACE].savedIBase, 0, nnRace.cInput * sizeof(float));
    tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedBase = g_malloc(nnCrashed.cHidden * sizeof(float));
    memset(tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedBase, 0, nnCrashed.cHidden * sizeof(float));
    tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedIBase = g_malloc(nnCrashed.cInput * sizeof(float));
    memset(tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedIBase, 0, nnCrashed.cInput * sizeof(float));
    tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedBase = g_malloc(nnContact.cHidden * sizeof(float));
    memset(tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedBase, 0, nnContact.cHidden * sizeof(float));
    tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedIBase = g_malloc(nnContact.cInput * sizeof(float));
    memset(tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedIBase, 0, nnContact.cInput * sizeof(float));

    tld->aMoves = (move *) g_malloc(sizeof(move) * MAX_INCOMPLETE_MOVES);
    memset(tld->aMoves, 0, sizeof(move) * MAX_INCOMPLETE_MOVES);
    return tld;
}

#if defined(USE_MULTITHREAD)

#if defined(DEBUG_MULTITHREADED) && defined(WIN32)
unsigned int mainThreadID;
#endif

#if GLIB_CHECK_VERSION (2,32,0)
static GMutex condMutex;        /* Extra mutex needed for waiting */
#else
static GMutex *condMutex = NULL;        /* Extra mutex needed for waiting */
#endif

#if GLIB_CHECK_VERSION (2,32,0)
/* Dynamic allocation of GPrivate is deprecated */
static GPrivate private_item = G_PRIVATE_INIT(free);

extern void
TLSCreate(TLSItem * pItem)
{
    *pItem = &private_item;
}
#else
extern void
TLSCreate(TLSItem * pItem)
{
    *pItem = g_private_new(g_free);
}
#endif

extern void
TLSSetValue(TLSItem pItem, size_t value)
{
    size_t *pNew = (size_t *) g_malloc(sizeof(size_t));
    *pNew = value;
    g_private_set(pItem, (gpointer) pNew);
}

#define TLSGet(item) *((size_t*)g_private_get(item))

extern void
InitManualEvent(ManualEvent * pME)
{
    ManualEvent pNewME = g_malloc(sizeof(*pNewME));
#if GLIB_CHECK_VERSION (2,32,0)
    g_cond_init(&pNewME->cond);
#else
    pNewME->cond = g_cond_new();
#endif
    pNewME->signalled = FALSE;
    *pME = pNewME;
}

extern void
FreeManualEvent(ManualEvent ME)
{
#if GLIB_CHECK_VERSION (2,32,0)
    g_cond_clear(&ME->cond);
#else
    g_cond_free(ME->cond);
#endif
    g_free(ME);
}

extern void
WaitForManualEvent(ManualEvent ME)
{
#if GLIB_CHECK_VERSION (2,32,0)
    gint64 end_time;
#else
    GTimeVal tv;
#endif
    multi_debug("wait for manual event asks lock (condMutex)");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    end_time = g_get_monotonic_time() + 10 * G_TIME_SPAN_SECOND;
#else
    g_mutex_lock(condMutex);
#endif
    multi_debug("wait for manual event gets lock (condMutex)");
    while (!ME->signalled) {
        multi_debug("waiting for manual event");
#if GLIB_CHECK_VERSION (2,32,0)
        if (!g_cond_wait_until(&ME->cond, &condMutex, end_time))
#else
        g_get_current_time(&tv);
        g_time_val_add(&tv, 10 * 1000 * 1000);
        if (g_cond_timed_wait(ME->cond, condMutex, &tv))
#endif
            break;
        else {
            multi_debug("still waiting for manual event");
        }
    }

#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_unlock(&condMutex);
#else
    g_mutex_unlock(condMutex);
#endif
    multi_debug("wait for manual event unlocks (condMutex)");
}

extern void
ResetManualEvent(ManualEvent ME)
{
    multi_debug("reset manual event asks lock (condMutex)");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    multi_debug("reset manual event gets lock (condMutex)");
    ME->signalled = FALSE;
    g_mutex_unlock(&condMutex);
#else
    g_mutex_lock(condMutex);
    multi_debug("reset manual event gets lock (condMutex)");
    ME->signalled = FALSE;
    g_mutex_unlock(condMutex);
#endif
    multi_debug("reset manual event unlocks (condMutex)");
}

extern void
SetManualEvent(ManualEvent ME)
{
    multi_debug("reset manual event asks lock (condMutex)");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    multi_debug("reset manual event gets lock (condMutex)");
    ME->signalled = TRUE;
    g_cond_broadcast(&ME->cond);
    g_mutex_unlock(&condMutex);
#else
    g_mutex_lock(condMutex);
    multi_debug("reset manual event gets lock (condMutex)");
    ME->signalled = TRUE;
    g_cond_broadcast(ME->cond);
    g_mutex_unlock(condMutex);
#endif
    multi_debug("reset manual event unlocks (condMutex)");
}

#if GLIB_CHECK_VERSION (2,32,0)
extern void
InitMutex(Mutex * pMutex)
{
    g_mutex_init(pMutex);
}

extern void
FreeMutex(Mutex * mutex)
{
    g_assert(MT_GetThreadID() == -1);
    g_mutex_clear(mutex);
}
#else
extern void
InitMutex(Mutex * pMutex)
{
    *pMutex = g_mutex_new();
}

extern void
FreeMutex(Mutex * mutex)
{
    g_assert(MT_GetThreadID() == -1);
    g_mutex_free(*mutex);
}
#endif

extern void
Mutex_Lock(Mutex * mutex)
{
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(mutex);
#else
    g_mutex_lock(*mutex);
#endif
}

extern void
Mutex_Release(Mutex * mutex)
{
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_unlock(mutex);
#else
    g_mutex_unlock(*mutex);
#endif
}

extern void
MT_InitThreads(void)
{
#if !GLIB_CHECK_VERSION (2,32,0)
    if (!g_thread_supported())
        g_thread_init(NULL);
    g_assert(g_thread_supported());
#endif
    td.tasks = NULL;
    MT_SafeSet(&td.doneTasks, 0);
    td.addedTasks = 0;
    td.totalTasks = -1;
    InitManualEvent(&td.activity);
    TLSCreate(&td.tlsItem);
    TLSSetValue(td.tlsItem, (size_t) MT_CreateThreadLocalData(-1));

#if defined(DEBUG_MULTITHREADED) && defined(WIN32)
    mainThreadID = GetCurrentThreadId();
#endif
    InitMutex(&td.multiLock);
    InitMutex(&td.queueLock);
    InitManualEvent(&td.syncStart);
    InitManualEvent(&td.syncEnd);
#if !GLIB_CHECK_VERSION (2,32,0)
    if (condMutex == NULL)
        condMutex = g_mutex_new();
#endif
    td.numThreads = 0;
}

extern void
CloseThread(void *UNUSED(unused))
{
    ThreadLocalData *pTLD;
    NNState *pnnState;

    g_assert(MT_SafeCompare(&td.closingThreads, TRUE));

    pTLD = (ThreadLocalData *) TLSGet(td.tlsItem);
    pnnState = pTLD->pnnState;

    g_free(pTLD->aMoves);

    for (int i = 0; i < 3; i++) {
        g_free(pnnState[i].savedBase);
        g_free(pnnState[i].savedIBase);
    }

    g_free(pnnState);
    g_free(pTLD);

    MT_SafeInc(&td.result);
}

extern void
MT_Close(void)
{
    MT_CloseThreads();

    FreeManualEvent(td.activity);
    FreeMutex(&td.multiLock);
    FreeMutex(&td.queueLock);

    FreeManualEvent(td.syncStart);
    FreeManualEvent(td.syncEnd);
}

extern void
MT_Exclusive(void)
{
    multi_debug("exclusive asks lock (multiLock)");
    Mutex_Lock(&td.multiLock);
    multi_debug("exclusive gets lock (multiLock)");
}

extern void
MT_Release(void)
{
    Mutex_Release(&td.multiLock);
    multi_debug("release unlocks (multiLock)");
}

#if defined(DEBUG_MULTITHREADED)
extern void
multi_debug(const char *str, ...)
{
    int id;
    va_list vl;
    char tn[5];
    char buf[1024];

    /* Sync output so order makes some sense */
#if defined(WIN32)
    WaitForSingleObject(td.multiLock, INFINITE);
#endif
    /* With glib threads, locking here doesn't seem to work :
     * GLib (gthread-posix.c): Unexpected error from C library during 'pthread_mutex_lock': Resource deadlock avoided.  Aborting.
     * Timestamp the output instead.
     * Maybe the g_get_*_time() calls will sync output as well anyway... */

    va_start(vl, str);
    vsprintf(buf, str, vl);

    id = MT_GetThreadID();
#if defined(WIN32)
    if (id == -1 && GetCurrentThreadId() == mainThreadID)
#else
    if (id == -1)
#endif
        strcpy(tn, "MT");
    else
        sprintf(tn, "T%d", id);

#if GLIB_CHECK_VERSION (2,28,0)
    printf("%" G_GINT64_FORMAT " %s: %s\n", g_get_monotonic_time(), tn, buf);
#else
    {
        GTimeVal tv;

        g_get_current_time(&tv);
        printf("%lu %s: %s\n", 1000000UL * tv.tv_sec + tv.tv_usec, tn, buf);
    }
#endif

    va_end(vl);

#if defined(WIN32)
    ReleaseMutex(td.multiLock);
#endif
}

#endif

#else	/* !defined(USE_MULTITHREAD) */

extern void
MT_InitThreads(void)
{
    td.tld = MT_CreateThreadLocalData(-1);
}

extern void
MT_Close(void)
{
    int i;
    NNState *pnnState;

    if (!td.tld)
        return;

    g_free(td.tld->aMoves);
    pnnState = td.tld->pnnState;
    for (i = 0; i < 3; i++) {
        g_free(pnnState[i].savedBase);
        g_free(pnnState[i].savedIBase);
    }
    g_free(pnnState);
    g_free(td.tld);
}

#endif
