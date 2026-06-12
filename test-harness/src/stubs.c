/* No internal gnubg includes! This prevents conflicting type errors. */
#include <stddef.h>

/* Simple Variables */
int fAnalysisRunning = 0;
int save_autosave = 0;
int nAutoSaveTime = 0;
char *szCurrentFileName = NULL;

/* Complex Structs (Allocated as generic memory blocks to satisfy the linker) */
char td[8192] = {0};
char ms[8192] = {0};
char ap[8192] = {0};
char positions[8192] = {0};
char msBoard[8192] = {0};
char mec[8192] = {0};
char mec_pc[8192] = {0};
float baseInputs[256] = {0};

/* Dummy Functions (Intercepting GTK/Threading calls) */
void ProcessEvents(void) {}
void MT_Exclusive(void) {}
void MT_Release(void) {}
void LogCube(void) {}
void GetManualDice(void) {}
void SetRNG(void) {}

/* Threading Stubs */
void CloseThread(void) {}
void Mutex_Lock(void) {}
void Mutex_Release(void) {}
void ResetManualEvent(void) {}
void SetManualEvent(void) {}
void WaitForManualEvent(void) {}
void TLSSetValue(void) {}
void MT_CreateThreadLocalData(void) {}

/* UI / Formatting Stubs */
void get_current_moverecord(void) {}
void FormatMove(void) {}
void ChangeGame(void) {}
void get_time(void) {}

/* High-Level UI Logic Stubs */
void BasicCubefulRollout(void) {}
void BasicCubefulRolloutNoLocking(void) {}
void EvaluatePositionWithLocking(void) {}
void GeneralCubeDecisionEWithLocking(void) {}
void GeneralEvaluationEWithLocking(void) {}
void ScoreMoveWithLocking(void) {}
void FindBestMoveWithLocking(void) {}
void FindnSaveBestMovesWithLocking(void) {}
void BasicCubefulRolloutWithLocking(void) {}
