/*
 * Copyright (C) 2007-2009 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2007-2023 the AUTHORS
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

#include "config.h"

#if defined(WIN32)
#include <process.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#if defined(USE_GTK)
#include "gtkgame.h"
#endif

#include "multithread.h"
#include "rollout.h"
#include "util.h"
#include "drawboard.h" /*for FormatMove()*/
#include "lib/simd.h"

#if defined(USE_MULTITHREAD)

static GThread* thread[MAX_NUMTHREADS];

extern unsigned int
MT_GetNumThreads(void)
{
    return td.numThreads;
}

extern void
MT_CloseThreads(void)
{
    unsigned int i;

    MT_SafeSet(&td.closingThreads, TRUE);
    mt_add_tasks(td.numThreads, CloseThread, NULL, NULL);
    if (MT_WaitForTasks(NULL, 0, FALSE) != (int) td.numThreads)
        g_print(_("Error closing threads!\n"));
    for (i = 0; i < td.numThreads; i++)
        g_thread_join(thread[i]);
}

static void
MT_TaskDone(Task * pt)
{
    MT_SafeInc(&td.doneTasks);

    if (pt) {
        free(pt->pLinkedTask);
        g_free(pt);
    }
}

static Task *
MT_GetTask(void)
{
    Task *task = NULL;

    multi_debug("get task asks lock (queueLock)");
    Mutex_Lock(&td.queueLock);
    multi_debug("get task gets lock (queueLock)");

    if (g_list_length(td.tasks) > 0) {
        task = (Task *) g_list_first(td.tasks)->data;
        td.tasks = g_list_delete_link(td.tasks, g_list_first(td.tasks));
        if (g_list_length(td.tasks) == 0) {
            ResetManualEvent(td.activity);
        }
    }

    Mutex_Release(&td.queueLock);
    multi_debug("get task unlocks (queueLock)");

    return task;
}

extern void
MT_AbortTasks(void)
{
    Task *task;
    /* Remove tasks from list */
    while ((task = MT_GetTask()) != NULL)
        MT_TaskDone(task);

    MT_SafeSet(&td.result, -1);
}

static SIMD_STACKALIGN gpointer
MT_WorkerThreadFunction(void *tld)
{
#if 0
    /* why do we need this align ? - because of a gcc bug */
#if __GNUC__ && defined(WIN32)
    /* Align stack pointer on 16/32 byte boundary so SSE/AVX variables work correctly */
    int align_offset;
#if defined(USE_AVX)
    __asm__ __volatile__("andl $-32, %%esp":::"%esp");
    align_offset = ((int) (&align_offset)) % 32;
#else
    __asm__ __volatile__("andl $-16, %%esp":::"%esp");
    align_offset = ((int) (&align_offset)) % 16;
#endif
#endif

#endif
    {
        ThreadLocalData *pTLD = (ThreadLocalData *) tld;
        TLSSetValue(td.tlsItem, (size_t) pTLD);

        MT_SafeInc(&td.result);
        MT_TaskDone(NULL);      /* Thread created */
        do {
            Task *task;
            WaitForManualEvent(td.activity);
            task = MT_GetTask();
            if (task) {
                task->fun(task->data);
                MT_TaskDone(task);
            }
        } while (MT_SafeCompare(&td.closingThreads, FALSE));

#if 0
#if __GNUC__ && defined(WIN32)
        /* De-align stack pointer to avoid crash on exit */
        __asm__ __volatile__("addl %0, %%esp"::"r"(align_offset):"%esp");
#endif
#endif
        return NULL;
    }
}

static gboolean
WaitingForThreads(gpointer UNUSED(unused))
{                               /* Unlikely to be called */
    multi_debug("waiting for threads to be created!");
    return FALSE;
}

static void
MT_CreateThreads(void)
{
    unsigned int i;
#if defined(DEBUG_MULTITHREADED)
    gchar *buf;

    buf = g_strdup_printf("creating %u thread%s", td.numThreads, (td.numThreads > 1 ? "s" : ""));
    multi_debug(buf);
    g_free(buf);
#endif
    MT_SafeSet(&td.result, 0);
    MT_SafeSet(&td.closingThreads, FALSE);
    for (i = 0; i < td.numThreads; i++) {
        ThreadLocalData *pTLD = MT_CreateThreadLocalData(i);

#if GLIB_CHECK_VERSION (2,32,0)
        if (!(thread[i] = g_thread_try_new(NULL, MT_WorkerThreadFunction, pTLD, NULL)))
#else
        if (!(thread[i] = g_thread_create(MT_WorkerThreadFunction, pTLD, TRUE, NULL)))
#endif
            printf(_("Failed to create thread\n"));
#if defined(DEBUG_MULTITHREADED)
        else {
            buf = g_strdup_printf("thread %u created", i);
            multi_debug(buf);
            g_free(buf);
        }
#endif
    }
    td.addedTasks = td.numThreads;
    /* Wait for all the threads to be created (timeout after 1 second) */
    if (MT_WaitForTasks(WaitingForThreads, 1000, FALSE) != (int) td.numThreads)
        g_print(_("Error creating threads!\n"));
}

void
MT_SetNumThreads(unsigned int num)
{
    if (num != td.numThreads) {
        if (td.numThreads != 0)
            MT_CloseThreads();
        td.numThreads = num;
        MT_CreateThreads();
        if (num == 1) {         /* No locking in evals */
            EvaluatePosition = EvaluatePositionNoLocking;
            GeneralCubeDecisionE = GeneralCubeDecisionENoLocking;
            GeneralEvaluationE = GeneralEvaluationENoLocking;
            ScoreMove = ScoreMoveNoLocking;
            FindBestMove = FindBestMoveNoLocking;
            FindnSaveBestMoves = FindnSaveBestMovesNoLocking;
            BasicCubefulRollout = BasicCubefulRolloutNoLocking;
        } else {                /* Locking version of evals */
            EvaluatePosition = EvaluatePositionWithLocking;
            GeneralCubeDecisionE = GeneralCubeDecisionEWithLocking;
            GeneralEvaluationE = GeneralEvaluationEWithLocking;
            ScoreMove = ScoreMoveWithLocking;
            FindBestMove = FindBestMoveWithLocking;
            FindnSaveBestMoves = FindnSaveBestMovesWithLocking;
            BasicCubefulRollout = BasicCubefulRolloutWithLocking;
        }
    }
}

extern void
MT_StartThreads(void)
{
    if (td.numThreads == 0) {
        /* We could set it to something else (the number of cores or some
         * fraction of that ?) but it is probably not a good idea to hog
         * a lot of resources by default.
         */
        td.numThreads = 1;

        MT_CreateThreads();
    }
}

void
MT_AddTask(Task * pt, gboolean lock)
{
    if (lock) {
        multi_debug("add task asks lock (queueLock)");
        Mutex_Lock(&td.queueLock);
        multi_debug("add task gets lock (queueLock)");
    }
    if (td.addedTasks == 0)
        MT_SafeSet(&td.result, 0);          /* Reset result for new tasks */
    td.addedTasks++;
    td.tasks = g_list_append(td.tasks, pt);
    if (g_list_length(td.tasks) == 1) { /* New tasks */
        SetManualEvent(td.activity);
    }
    if (lock) {
        Mutex_Release(&td.queueLock);
        multi_debug("add task unlocks");
    }
}

extern void
mt_add_tasks(unsigned int num_tasks, AsyncFun pFun, void *taskData, gpointer linked)
{
    unsigned int i;
    {
#if defined(DEBUG_MULTITHREADED)
        multi_debug("add %u task%s asks lock (queueLock)", num_tasks, (num_tasks > 1 ? "s" : ""));
        Mutex_Lock(&td.queueLock);
        multi_debug("add tasks gets lock (queueLock)");
#else
        Mutex_Lock(&td.queueLock);
#endif
    }
    for (i = 0; i < num_tasks; i++) {
        Task *pt = (Task *) g_malloc(sizeof(Task));
        pt->fun = pFun;
        pt->data = taskData;
        pt->pLinkedTask = linked;
        MT_AddTask(pt, FALSE);
    }
    Mutex_Release(&td.queueLock);
    multi_debug("add tasks unlocks (queueLock)");
}

static gboolean
WaitForAllTasks(int time)
{
    int j = 0;

    while (!MT_SafeCompare(&td.doneTasks, td.totalTasks)) {
        
        // if (pmrCurAnn != NULL) {
        //     g_message("in the loop: pmrCurAnn exists");
        // }
        // SetAnnotation(pmrCurAnn);
        // ChangeGame(NULL);
        if (j == 10)
            return FALSE;       /* Not done yet */

        j++;
        g_usleep(100 * time);
    }
    return TRUE;
}

int
MT_WaitForTasks(gboolean(*pCallback) (gpointer), int callbackTime, int autosave)
{
    int callbackLoops = callbackTime / UI_UPDATETIME;
    int waits = 0;
    int polltime = callbackLoops ? UI_UPDATETIME : callbackTime;
    guint as_source = 0;

    char tmp1[200];
    char tmp2[200];
    // TanBoard anBoard;
    moverecord * pmr1;
    moverecord * pmr2;
    int start1 = 1;
    int start2 = 1;
    //int myPage;
    int i=0;

    /* Set total tasks to wait for */
    td.totalTasks = td.addedTasks;
#if defined(USE_GTK)
        // g_message("MT_WaitForTasks\n");
    GTKSuspendInput();
        // GTKResumeInput();
#endif

    if (autosave)
        as_source = g_timeout_add(nAutoSaveTime * 60000, save_autosave, NULL);
    multi_debug("waiting for all tasks");
    while (!WaitForAllTasks(polltime)) {
        waits++;
        if (pCallback && waits >= callbackLoops) {
            waits = 0;
            pCallback(NULL);
        }
        ProcessEvents();
        /* 
        VERSION 1: When analysis runs in the background:
        Every so often, we make sure it displays the result of the analysis, 
        else the analysis result of the first move is not shown automatically.
        Likewise it automatically shows the colors of the marked (wrong) moves,
        which looks really nice.
        - This could probably be improved, no need to check so often,
        it may slow down gnubg
        - It is less frequent than in the loop of the above function: 
            WaitForAllTasks(); so maybe it's not too bad

        VERSION 2: periodically (every 3 times we go through the loop):
        (1) at the start, we record pmr1 of our move. (We call start1 
        this single step.)
        (2) As long as the user doesn't move, i.e. we stay at pmr2==pmr1, then 
        we keep running ChangeGame(). (We call this start2.)
        (3) If the user moves, i.e. gets to some pmr2 !=pmr1, and the move analysis 
        is available there, then we stop. It may be a move with cube+move decisions, and
        if the user looks at some cube decision, ChangeGame() will change the page 
        to the move decision instead, bothering the user.
        In this last step, we don't update colors anymore. We could modulate 
        in the future based on what the user is doing.
        */
        // if (pwMoveAnalysis!=NULL)       
        //        g_message("sensitive:%d", gtk_widget_is_sensitive(pwAnalysis));
        if(fAnalysisRunning) {
            i++;
            if (i==3) {
                i=0;
                if (start2) {
                    if(start1) {
                        pmr1 = get_current_moverecord(NULL);
                        if (pmr1 != NULL)
                            FormatMove(tmp1, msBoard(), pmr1->n.anMove);
                        start1 = 0;
                        // g_message("pmr1: move index i=%u; move=%s\n",pmr1->n.iMove, tmp1);
                        ChangeGame(NULL);
                    } else {
                        pmr2 = get_current_moverecord(NULL);
                        if (pmr2 != NULL)
                            FormatMove(tmp2, msBoard(), pmr2->n.anMove);
                        // g_message("pmr2: move index i=%u; move=%s\n",pmr2->n.iMove, tmp2);
                        // if (pwMoveAnalysis!=NULL)
                        //     g_message("new results");
#if defined(USE_GTK)
                        if (strcmp(tmp1, tmp2) != 0 && pwMoveAnalysis!=NULL) {
#else
                        if (strcmp(tmp1, tmp2) != 0) {
#endif
                            // g_message("STOP");
                            start2 = 0;
                        } else {
                            // g_message("change");
                            ChangeGame(NULL); 
                        }
                    }
                }
            }
        }
        // myPage=gtk_notebook_get_current_page(GTK_NOTEBOOK(gtk_widget_get_parent(pwAnalysis)));
        // g_message("notebook page=%d",myPage);


            // if (pwMoveAnalysis!=NULL)
            // g_message("yeah!");
            // if (pwCubeAnalysis!=NULL)
            // g_message("oop cube!");
            /* at the start pmr_cur->n.iMove is not valid and returns an
            arbitrary large number (even though pmr_cur is not NULL), then 
            the first time it's valid it looks like it returns smaller numbers...
            also it seems that it's valid for analysed games 
            */
            // if(pmr_cur->n.iMove<100){
                // ChangeGame(NULL); 
                // waitToDisplay=0;
            // }  



        //         // if (pmrCurAnn != NULL) {
        //     // g_message("in the loop: pmrCurAnn exists");
        //                 // FormatMove(sz + strlen(_("Moved ")), msBoard(), pmr->n.anMove);
        //                 
        //                 // FormatMove(tmp, msBoard(), pmrCurAnn->n.anMove);
        // // g_message("move=%s\n", tmp);
        // 
        // }
        // SetAnnotation(pmrCurAnn);

    }
    if (autosave) {
        g_source_remove(as_source);
        save_autosave(NULL);
    }
    multi_debug("done waiting for all tasks");

    MT_SafeSet(&td.doneTasks, 0);
    td.addedTasks = 0;
    td.totalTasks = -1;

#if defined(USE_GTK)
    GTKResumeInput();
#endif
    return MT_SafeGet(&td.result);
}

extern void
MT_SetResultFailed(void)
{
    MT_SafeSet(&td.result, -1);
}

extern int
MT_GetDoneTasks(void)
{
    return MT_SafeGet(&td.doneTasks);
}

/* Code below used in calibrate to try and get a resonable figure for multiple threads */

static double start;            /* used for timekeeping */

extern void
MT_SyncInit(void)
{
    ResetManualEvent(td.syncStart);
    ResetManualEvent(td.syncEnd);
}

extern void
MT_SyncStart(void)
{
    static int count = 0;

    /* Wait for all threads to get here */
    if (MT_SafeIncValue(&count) == (int) td.numThreads) {
        count--;
        start = get_time();
        SetManualEvent(td.syncStart);
    } else {
        WaitForManualEvent(td.syncStart);
        if (MT_SafeDecCheck(&count))
            ResetManualEvent(td.syncStart);
    }
}

extern double
MT_SyncEnd(void)
{
    static int count = 0;

    /* Wait for all threads to get here */
    if (MT_SafeIncValue(&count) == (int) td.numThreads) {
        const double now = get_time();
        count--;
        SetManualEvent(td.syncEnd);
        return now - start;
    } else {
        WaitForManualEvent(td.syncEnd);
        if (MT_SafeDecCheck(&count))
            ResetManualEvent(td.syncEnd);
        return 0;
    }
}

#else	/* !defined(USE_MULTITHREAD) */
#include "multithread.h"
#include <stdlib.h>
#if defined(USE_GTK)
#include <gtkgame.h>
#endif

int asyncRet;
void
MT_AddTask(Task * pt, gboolean lock)
{
    (void) lock;                /* silence compiler warning */
    td.result = 0;              /* Reset result for new tasks */
    td.tasks = g_list_append(td.tasks, pt);
}

void
mt_add_tasks(unsigned int num_tasks, AsyncFun pFun, void *taskData, gpointer linked)
{
    unsigned int i;
    for (i = 0; i < num_tasks; i++) {
        Task *pt = (Task *) malloc(sizeof(Task));
        pt->fun = pFun;
        pt->data = taskData;
        pt->pLinkedTask = linked;
        MT_AddTask(pt, FALSE);
    }
}

extern int
MT_GetDoneTasks(void)
{
    return MT_SafeGet(&td.doneTasks);
}

int
MT_WaitForTasks(gboolean(*pCallback) (gpointer), int callbackTime, int autosave)
{
    GList *member;
    guint as_source = 0, cb_source = 0;

    (void) callbackTime;        /* silence compiler warning */
    MT_SafeSet(&td.doneTasks, 0);

#if defined(USE_GTK)
    GTKSuspendInput();
#endif

    multi_debug("waiting for all tasks");

    if (pCallback)
        pCallback(NULL);

    cb_source = g_timeout_add(1000, pCallback, NULL);
    if (autosave)
        as_source = g_timeout_add(nAutoSaveTime * 60000, save_autosave, NULL);
    for (member = g_list_first(td.tasks); member; member = member->next, MT_SafeInc(&td.doneTasks)) {
        Task *task = member->data;
        task->fun(task->data);
        free(task->pLinkedTask);
        free(task);
        ProcessEvents();
    }
    g_list_free(td.tasks);
    if (autosave) {
        g_source_remove(as_source);
        save_autosave(NULL);
    }

    g_source_remove(cb_source);
    td.tasks = NULL;

#if defined(USE_GTK)
    GTKResumeInput();
#endif
    return td.result;
}

extern void
MT_AbortTasks(void)
{
    td.result = -1;
}

#endif
