# Opening-book conformance check

Does this engine reproduce the numbers in the BackgammonDB opening book?

The book (github.com/chalkstreamdev/bgdb-opening-book, data CC-BY-4.0,
code GPL-3.0) holds gnubg 1.08.003's 3-ply evaluation of the first three
decisions of a game at every match score to 25-away: 4,230,443 entries,
351 score contexts, top-8 plays per position with cubeless probabilities
and cubeful equity. Its reader gates on the `gnubg --version` string. This
tool gates on **agreement** instead: the book is trusted for a build only
where that build reproduces it, with gnubg as the authority on both sides.

## What it does

`conformance.c` is a host build of the port's exact engine subset (the
source list is derived from `jni-bridge/CMakeLists.txt` at build time, as
`tools/rollout_harness` does). It installs a position through the facade's
own `gnubg_mobile_set_gnubg_id` (gnubg's SetGNUbgID / SetXGID), loads the
app's default match-equity table (Kazaross-XG2 -- also desktop gnubg's
default, and so the book's), then calls gnubg's `GenerateMoves` and
`ScoreMove` twice per legal play with the book's two published contexts
(worker.py `eval_context`: plies 3, deterministic, prune, noise 0; cubeful
off for the probabilities, cubeful on for the equity -- exactly what the
book's grinder takes from gnubg's `hint`). It prints every play with its
probabilities and equities. It reads no board and interprets nothing.

`check.py` runs it over one or more book context files
(`book-gnubg-1.08.003-depth<N>-<ctx>.jsonl.gz`, ~9 KB each at depth 1)
and reports, per play, the absolute difference to the book.

## Run

    cd /home/erweitert/gnubg-android
    make -C tools/book_conformance
    mkdir -p tmp/book
    # any context file from the book's v1.0 release, verified against its SHA256SUMS
    python3 tools/book_conformance/check.py tmp/book/book-gnubg-1.08.003-depth1-16a10a.jsonl.gz

One 3-ply position takes ~7 s on one core.

## First measurement (2026-09-26, assistant sandbox, 1 core, x86-64)

Context 16a10a, depth 1 (30 positions, 187 plays matched; 51 book plays
unmatched only because the book writes collapsed notation `[[24,18]]`
where gnubg's anMove records the hops -- a matcher gap, not a
disagreement, to be closed):

    entries=30 plays=187 max_dprob=0.000951 max_dequity=0.002778

150 of 187 plays agree within 2e-5 in every probability and in the
cubeful equity (float32 rounding at 6 dp). 37 plays differ by 2e-5 .. 2.8e-3
in equity or probability. The worst: 61 rolled at 6-0/16, play 24/18 6/5 --
ours 0.002209, book 0.004987, with identical probabilities to 6 dp.

Mechanisms tested and killed for the residual, all on that play:
- the MET: with the built-in Zadeh table every equity was off by ~0.010;
  with Kazaross-XG2 the equities of the clean plays match to 6 dp.
- the upstream sync (engine-core at ccfe40a~1 vs HEAD): byte-identical.
- SIMD arithmetic order: scalar, SSE2, AVX and FMA3 builds of the harness
  all give 0.002209 (last digit moves at most).
- the grinder's movefilter override: the lookahead uses the prune nets
  (`FindBestMoveInEval`) when `fUsePrune` is set, not the filters.
- the eval cache: `EvalKey` carries the cubeful bit, so the grinder's
  cubeless-then-cubeful passes cannot cross-contaminate.

Decided the same evening on the maintainer's desktop gnubg (Fedora
`1.08.003 20260116`): 24/18 6/5 = +0.005 at 3-ply, the book's number. Ply
bisection, desktop vs this harness, cubeful equities: 0-ply and 1-ply
match to 3 dp on every play; 2-ply diverges on that one play (+0.003 vs
+0.0000) and the offset carries to 3-ply.

## Pinpointed (2026-09-26, later the same evening)

`engine-core/lib/neuralnetsse.c`, `sigmoid_positive_ps`, lines 204-226:
gnubg's SIMD sigmoid computes `1/(1+e^-x)` with the hardware
**reciprocal approximation** (`_mm_rcp_ps` / `_mm256_rcp_ps`, ~12 bits)
when the compiler defines `__FAST_MATH__`, and with an exact division
otherwise. Upstream's `configure.ac` (line 594) appends `-ffast-math` to
`AM_CFLAGS` unconditionally for gcc, so every desktop gnubg -- Fedora's,
Ubuntu's, the book's grinder -- runs the approximate branch. Our device
build (`jni-bridge/CMakeLists.txt`, `-O2`, no fast-math) runs the exact
branch. The net outputs then differ at ~1e-4 relative; at 2 plies and
deeper the cubeful lookahead's reply pruning occasionally flips on that,
and the play's equity moves by up to ~3e-3. On NEON the same function has
its own two branches (one Newton-Raphson step under fast-math, two
otherwise) with upstream's own comment: "TODO: Check how many
Newton-Raphson iterations are needed to match x86 rcp and div accuracy"
-- upstream's ARM builds do not match its x86 builds either.

Evidence, all on 24/18 6/5 at 6-0/16, 2-ply cubeful (book: +0.003):

    build                                              result
    upstream b1b2772c, gcc, --enable-simd=no           +0.000
    upstream b1b2772c, gcc, --enable-threads, simd=no  +0.000
    upstream b1b2772c, gcc, simd=avx (configure adds
      -O3 -ffast-math)                                 +0.003
    same, bearoff databases removed                    +0.003
    this harness, gcc -O2, scalar                      0.000010
    this harness, gcc -O2 -DUSE_AVX / SSE2 / FMA3      0.000010
    this harness, gcc -O3 -DUSE_AVX                    0.000010
    this harness, gcc -O3 -ffast-math, scalar          0.000010
    this harness, gcc -O2 -ffast-math -DUSE_AVX        0.003241
    this harness, gcc -O2 -ffast-math -DUSE_SSE2       0.003241
    this harness, clang -O2 -ffast-math -DUSE_AVX      0.003241
    this harness, clang -O2 -DUSE_AVX                  0.000010
    upstream 76e5ef70 (2026-01-05), gcc, simd=no       +0.000

Killed on the way, each by a run: the MET (Zadeh vs Kazaross shifts every
equity by ~0.010 -- the first thing found, and the reason a version
string is not a gate), the upstream sync (pre-sync engine identical), the
tie-comparator commit 0faafabb (reverted: identical), the grinder's
movefilter override (lookahead prunes by net), eval-cache cross-talk
(EvalKey carries fCubeful), the bearoff databases (removed from an
upstream build: identical), threads, -O3, SIMD without fast-math,
gcc-vs-clang, hint's FindnSaveBestMoves path vs ScoreMove per play
(BOOK_HINT=1 mode: identical).

So: the port's code is upstream's; the port's ARITHMETIC differs from the
desktop's by one compiler flag that selects one of gnubg's own two sigmoid
branches. Which branch "is gnubg" is a maintainer ruling (docs/HANDOVER.md).
