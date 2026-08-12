/*
 * harness.c -- host test harness for timecontrol.c
 *
 * Builds anywhere a C99 compiler exists; no dependencies.  Every test
 * drives the module with simulated timestamps only.  Exit code is the
 * verdict.
 *
 * T1 is the maintainer's field scenario and the module's founding
 * invariant: a 10-point match under the tournament regulation (12 s
 * delay, 2 min/pt reserve) in which every move is completed within
 * 11 seconds must end with both reserves BIT-IDENTICAL to their
 * initial values -- hundreds of turns, zero milliseconds charged.
 */

#include <stdio.h>
#include <string.h>

#include "timecontrol.h"

static int cFail = 0;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            ++cFail; \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

#define MS_DELAY   12000ULL
#define MS_RESERVE (2ULL * 60000ULL * 10ULL)   /* 2 min/pt x 10 points */

/* T1: the maintainer's 10-point/11-second match.  400 turns,
 * alternating; multiple poll settles inside each turn to exercise I2
 * as well.  Both reserves must be bit-identical to the start. */
static void
test_winter_match(void)
{
    timecontrol tc;
    uint64_t now = 1000;
    int i;

    tc_init(&tc, MS_DELAY, MS_RESERVE, MS_RESERVE);
    for (i = 0; i < 400; ++i) {
        CHECK(tc_start_turn(&tc, i % 2, now) == TC_EVENT_NONE);
        CHECK(tc_settle(&tc, now + 3000) == TC_EVENT_NONE);   /* poll */
        CHECK(tc_settle(&tc, now + 7500) == TC_EVENT_NONE);   /* poll */
        now += 11000;                                         /* act at 11 s */
        CHECK(tc_settle(&tc, now) == TC_EVENT_NONE);
    }
    CHECK(tc_start_turn(&tc, -1, now) == TC_EVENT_NONE);      /* match ends */
    CHECK(tc.amsReserve[0] == MS_RESERVE);                    /* bit-identical */
    CHECK(tc.amsReserve[1] == MS_RESERVE);
    CHECK(!tc.afFlagged[0] && !tc.afFlagged[1]);
}

/* T2: boundaries.  Acting at exactly the delay charges nothing;
 * 1 ms past the delay charges exactly 1 ms. */
static void
test_boundaries(void)
{
    timecontrol tc;

    tc_init(&tc, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_start_turn(&tc, 0, 0);
    CHECK(tc_settle(&tc, MS_DELAY) == TC_EVENT_NONE);
    CHECK(tc.amsReserve[0] == MS_RESERVE);

    tc_start_turn(&tc, 1, MS_DELAY);
    CHECK(tc_settle(&tc, MS_DELAY + MS_DELAY + 1) == TC_EVENT_NONE);
    CHECK(tc.amsReserve[1] == MS_RESERVE - 1);
}

/* T3: settle frequency is irrelevant (I2): the same interval charged
 * in many pieces equals one piece. */
static void
test_frequency_invariance(void)
{
    timecontrol a, b;
    uint64_t t;

    tc_init(&a, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_init(&b, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_start_turn(&a, 0, 0);
    tc_start_turn(&b, 0, 0);
    for (t = 250; t <= 20000; t += 250)
        tc_settle(&a, t);                 /* 80 small settles */
    tc_settle(&b, 20000);                 /* one large settle */
    CHECK(a.amsReserve[0] == b.amsReserve[0]);
    CHECK(a.amsReserve[0] == MS_RESERVE - (20000 - MS_DELAY));
}

/* T4: the flag fires once, at the right millisecond, for the right
 * player; it is latched; the other clock keeps working. */
static void
test_flag(void)
{
    timecontrol tc;

    tc_init(&tc, 1000, 5000, MS_RESERVE);
    tc_start_turn(&tc, 0, 0);
    CHECK(tc_settle(&tc, 5999) == TC_EVENT_NONE);       /* 1 ms of reserve left */
    CHECK(tc.amsReserve[0] == 1);
    CHECK(tc_settle(&tc, 6000) == TC_EVENT_FLAG_PLAYER0);
    CHECK(tc.amsReserve[0] == 0 && tc.afFlagged[0]);
    CHECK(tc_settle(&tc, 9000) == TC_EVENT_NONE);       /* latched (I3) */
    CHECK(tc.amsReserve[0] == 0);

    CHECK(tc_start_turn(&tc, 1, 9000) == TC_EVENT_NONE);
    CHECK(tc_settle(&tc, 9000 + 1000 + 500) == TC_EVENT_NONE);
    CHECK(tc.amsReserve[1] == MS_RESERVE - 500);        /* other side serves on */
}

/* T4b: a flag raised exactly at the hand-over is reported by
 * tc_start_turn itself. */
static void
test_flag_at_handover(void)
{
    timecontrol tc;

    tc_init(&tc, 1000, 2000, 2000);
    tc_start_turn(&tc, 0, 0);
    CHECK(tc_start_turn(&tc, 1, 10000) == TC_EVENT_FLAG_PLAYER0);
    CHECK(tc.afFlagged[0] && !tc.afFlagged[1]);
}

/* T5: paused time is charged to nobody, across delay and bank alike. */
static void
test_pause(void)
{
    timecontrol tc;

    tc_init(&tc, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_start_turn(&tc, 0, 0);
    tc_settle(&tc, 5000);                                /* inside the delay */
    CHECK(tc_pause(&tc, 6000) == TC_EVENT_NONE);
    tc_resume(&tc, 1006000);                             /* 1000 s pass */
    CHECK(tc_settle(&tc, 1006000 + 5000) == TC_EVENT_NONE);
    CHECK(tc.amsReserve[0] == MS_RESERVE);               /* 6+5 s < 12 s delay */
    CHECK(tc.msDelayLeft == MS_DELAY - 11000);
}

/* T6: a decreasing timestamp (broken caller) charges nothing and
 * corrupts nothing (I4). */
static void
test_monotonic_tolerance(void)
{
    timecontrol tc;

    tc_init(&tc, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_start_turn(&tc, 0, 100000);
    CHECK(tc_settle(&tc, 50000) == TC_EVENT_NONE);       /* time "goes back" */
    CHECK(tc.amsReserve[0] == MS_RESERVE);
    CHECK(tc.msDelayLeft == MS_DELAY);
    CHECK(tc_settle(&tc, 100000 + 11000) == TC_EVENT_NONE);
    CHECK(tc.amsReserve[0] == MS_RESERVE);               /* 11 s inside delay */
}

/* T7: serialize/deserialize round-trips mid-turn and the restored
 * clock continues bit-identically. */
static void
test_serialization(void)
{
    timecontrol a, b;
    char sz[256];
    uint64_t t = 0;
    int i;

    tc_init(&a, MS_DELAY, MS_RESERVE, MS_RESERVE);
    tc_start_turn(&a, 0, t);
    tc_settle(&a, t + 15000);                            /* 3 s into the bank */

    CHECK(tc_serialize(&a, sz, sizeof sz) > 0);
    memset(&b, 0xA5, sizeof b);
    CHECK(tc_deserialize(&b, sz) == 0);
    CHECK(memcmp(&a, &b, sizeof a) == 0);                /* byte-equal state */

    for (i = 0, t = 15000; i < 50; ++i, t += 11000) {    /* play on, twinned */
        tc_start_turn(&a, i % 2, t);
        tc_start_turn(&b, i % 2, t);
        tc_settle(&a, t + 11000);
        tc_settle(&b, t + 11000);
    }
    CHECK(memcmp(&a, &b, sizeof a) == 0);

    CHECK(tc_deserialize(&b, "tc1 nonsense") == -1);     /* malformed input */
    CHECK(memcmp(&a, &b, sizeof a) == 0);                /* ...left untouched */
}

int
main(void)
{
    test_winter_match();
    test_boundaries();
    test_frequency_invariance();
    test_flag();
    test_flag_at_handover();
    test_pause();
    test_monotonic_tolerance();
    test_serialization();

    if (cFail) {
        fprintf(stderr, "clock harness: %d FAILURE(S)\n", cFail);
        return 1;
    }
    printf("clock harness: all tests passed\n");
    return 0;
}
