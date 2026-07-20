/*
 * quorum_stubs.c -- the few edges eval.c/stubs.c reference but candidate
 * generation never reaches (rollout, package-data-dir lookup, error UI).
 * The EVALUATION path is linked for real. Aborts loudly if reached. GPLv3+.
 */
#include <stdio.h>
#include <stdlib.h>
#define LOUD(name) \
    fprintf(stderr, "quorum stub reached: %s (candidate gen should not hit this)\n", name); abort();

/* getPkgDataDir: eval.c calls it to locate data files by relative path; the
 * harness passes the weights path explicitly to EvalInitialise, so this is
 * never the lookup route. Return "." so any incidental use is harmless. */
char *getPkgDataDir(void) { return "."; }

void PrintError(const char *sz) { (void)sz; fprintf(stderr, "gnubg: %s\n", sz); }
void outputerrf(const char *sz, ...) { (void)sz; }

/* SetRNG: dice-roll setup, unreachable from chequer-candidate ranking. */
int SetRNG(void *rng, void *rngctx, int r, const char *seed)
{ (void)rng; (void)rngctx; (void)r; (void)seed; return 0; }

/* Rollout is not part of chequer-candidate ranking. */
int BasicCubefulRolloutNoLocking(unsigned int b[][2][25], float p[][7],
    int a, int c, void *ci, void *rc, void *aa, int d)
{ (void)b;(void)p;(void)a;(void)c;(void)ci;(void)rc;(void)aa;(void)d;
  LOUD("BasicCubefulRolloutNoLocking") }
