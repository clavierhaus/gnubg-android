/*
 * pilot_stubs.c -- link stubs for the §9.4 pilot harness.
 *
 * The harness calls ONLY PipCount, ClassifyPosition and CalculateHalfInputs
 * -- pure board arithmetic. eval.c nevertheless references its full world at
 * link time: neural-net loading, bearoff databases, the MET, RNG seeding.
 * None of that is reachable from the three calls above, so every stub here
 * ABORTS LOUDLY: if a future edit routes execution into one, the pilot
 * crashes with the symbol's name instead of returning silent garbage.
 * Data-only externs are zeroed definitions for the same reason: never read
 * on the pure paths, wrong-by-construction if they ever are.
 */

#include <stdio.h>
#include <stdlib.h>

#define LOUD(name) \
    fprintf(stderr, "pilot stub reached: %s -- the pure-path assumption is " \
                    "broken, fix the harness\n", name); \
    abort();

void *BearoffInit(const char *sz, const int bo, void (*p)(unsigned int))
{ (void)sz; (void)bo; (void)p; LOUD("BearoffInit") }
void BearoffClose(void *p) { (void)p; LOUD("BearoffClose") }
int BearoffStatus(void *p, char *sz) { (void)p; (void)sz; LOUD("BearoffStatus") }
int BearoffEval(const void *p, const unsigned int (*b)[25], float *ar)
{ (void)p; (void)b; (void)ar; LOUD("BearoffEval") }
int BearoffDist(const void *p, unsigned int n, float *a, float *b,
                float *c, float *d, unsigned short int *e)
{ (void)p; (void)n; (void)a; (void)b; (void)c; (void)d; (void)e;
  LOUD("BearoffDist") }
int BearoffCubeful(const void *p, unsigned int n, float *ar, float *aq)
{ (void)p; (void)n; (void)ar; (void)aq; LOUD("BearoffCubeful") }
int BearoffHyper(const void *p, unsigned int n, float *ar, float *aq)
{ (void)p; (void)n; (void)ar; (void)aq; LOUD("BearoffHyper") }
int isBearoff(const void *p, const unsigned int (*b)[25])
{
    /* The harness loads no bearoff database, so the db pointer is NULL and
     * "not a database bearoff" is the TRUTHFUL answer -- this is what lets
     * ClassifyPosition return CLASS_RACE for race boards. A non-NULL db
     * would mean someone wired real bearoff in; then this stub is a bug. */
    if (p == NULL) { (void)b; return 0; }
    LOUD("isBearoff-with-db")
}

void GetPoints(float *ar, const void *pci, float *aq)
{ (void)ar; (void)pci; (void)aq; LOUD("GetPoints") }
float LogCube(const float x, const float y)
{ (void)x; (void)y; LOUD("LogCube") }

float getME(const int s0, const int s1, const int mt, const int f,
            const int a, const int b, const int c,
            const float aaME[2][50][50], const float aaMEPC[2][50])
{ (void)s0; (void)s1; (void)mt; (void)f; (void)a; (void)b; (void)c;
  (void)aaME; (void)aaMEPC; LOUD("getME") }
void getMEMultiple(const int a, const int b, const int c, const int d,
                   const int e, const int f, const int g,
                   const float aaME[2][50][50], const float aaMEPC[2][50],
                   float *p, float *q, float *r)
{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
  (void)aaME; (void)aaMEPC; (void)p; (void)q; (void)r;
  LOUD("getMEMultiple") }

void getRaceBGprobs(const unsigned int b[6], float *ar)
{ (void)b; (void)ar; LOUD("getRaceBGprobs") }

void PrintError(const char *sz) { (void)sz; LOUD("PrintError") }
void *TLSGet(const int key) { (void)key; LOUD("TLSGet") }
char *getPkgDataDir(void) { LOUD("getPkgDataDir") }
void irandinit(void *p, const int f) { (void)p; (void)f; LOUD("irandinit") }
unsigned long isaac(void *p) { (void)p; LOUD("isaac") }
void *md5_buffer(const char *b, unsigned long n, void *r)
{ (void)b; (void)n; (void)r; LOUD("md5_buffer") }
void baseInputs(const unsigned int (*b)[25], float *ar)
{ (void)b; (void)ar; LOUD("baseInputs") }

/* Data-only externs: zeroed, never read on the pure paths. */
float aafMET[50][50];
float aafMETPostCrawford[2][50];
float aaaafGammonPrices[4][50][50][4];
float aaaafGammonPricesPostCrawford[4][50][4];

/* Surfaced by compiling eval.c at -O0 (kept for the ComputeTable symbol):
 * error-output printf, unreachable from the pure paths. */
void outputerrf(const char *sz, ...) { (void)sz; LOUD("outputerrf") }
