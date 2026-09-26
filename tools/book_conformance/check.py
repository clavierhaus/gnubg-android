#!/usr/bin/env python3
# check.py -- compare BackgammonDB opening-book entries with this engine.
#
# Copyright (C) 2026 clavierhaus. GPL-3.0-or-later (see COPYING).
#
# Reads one or more book context files (book-gnubg-<ver>-depth<N>-<ctx>.jsonl
# or .jsonl.gz, CC-BY-4.0, github.com/chalkstreamdev/bgdb-opening-book), runs
# ./tools/book_conformance/conformance on every checker entry, and reports per
# play the absolute difference between the book's numbers and ours:
# probabilities (5), cubeful equity. Cube entries (dice 00) are skipped.
#
# The book stores plays as [from, to] pairs in the on-roll player's numbering
# (25 = bar, 0 = off); the harness prints gnubg's anMove (0-based, 24 = bar,
# -1 = off). Matching is on the multiset of (from-1, to-1) pairs, off mapped
# to -1 -- a re-encoding of the same move, not an interpretation of it.
#
# Usage, from the repo root:
#   python3 tools/book_conformance/check.py [--plies 3] [--limit N] FILE...
# Exit 0 when every compared play is within --eps-prob / --eps-equity,
# 1 otherwise, 2 on a harness failure. Prints a summary line last:
#   SUMMARY entries=<n> plays=<n> max_dprob=<x> max_dequity=<x> missing=<n>

import argparse
import gzip
import json
import subprocess
import sys

HARNESS = './tools/book_conformance/conformance'


def open_book(path):
    if path.endswith('.gz'):
        return gzip.open(path, 'rt', encoding='ascii')
    return open(path, 'r', encoding='ascii')


def book_pairs(move):
    """[[from,to],...] in on-roll numbering -> sorted tuple of gnubg pairs."""
    out = []
    for f, t in move:
        out.append((f - 1, -1 if t == 0 else t - 1))
    return tuple(sorted(out))


def harness_pairs(anmove):
    out = []
    i = 0
    while i < 8 and anmove[i] >= 0:
        out.append((anmove[i], anmove[i + 1]))
        i += 2
    return tuple(sorted(out))


def run_harness(xgid, plies):
    p = subprocess.run([HARNESS, xgid, str(plies)], capture_output=True, text=True)
    if p.returncode != 0:
        sys.stderr.write(p.stderr)
        raise RuntimeError('harness rc=%d for %s' % (p.returncode, xgid))
    plays = {}
    for line in p.stdout.splitlines():
        t = line.split()
        if not t or t[0] != 'PLAY':
            continue
        anmove = [int(x) for x in t[1:9]]
        probs = [float(x) for x in t[10:15]]
        cf = float(t[18])
        plays[harness_pairs(anmove)] = (probs, cf)
    return plays


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('files', nargs='+')
    ap.add_argument('--plies', type=int, default=3)
    ap.add_argument('--limit', type=int, default=0, help='entries per file (0 = all)')
    ap.add_argument('--eps-prob', type=float, default=1e-5)
    ap.add_argument('--eps-equity', type=float, default=1e-5)
    a = ap.parse_args()

    n_entries = n_plays = n_missing = 0
    max_dp = max_de = 0.0
    worst = None
    for path in a.files:
        done = 0
        with open_book(path) as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                e = json.loads(line)
                if 'plays' not in e:
                    continue          # cube entry or dance marker
                xgid = 'XGID=' + e['xgid']
                try:
                    ours = run_harness(xgid, a.plies)
                except RuntimeError as ex:
                    print('HARNESS-FAIL %s: %s' % (xgid, ex))
                    return 2
                n_entries += 1
                for k, play in enumerate(e['plays']):
                    key = book_pairs(play['move'])
                    if key not in ours:
                        n_missing += 1
                        print('MISSING %s play %d %s' % (xgid, k, play['move']))
                        continue
                    probs, cf = ours[key]
                    bp = play['probabilities']
                    bprobs = [bp['win'], bp['winGammon'], bp['winBackgammon'],
                              bp['loseGammon'], bp['loseBackgammon']]
                    dp = max(abs(x - y) for x, y in zip(probs, bprobs))
                    de = abs(cf - play['cubefulEquity'])
                    n_plays += 1
                    if dp > max_dp:
                        max_dp = dp
                    if de > max_de:
                        max_de = de
                        worst = (xgid, play['move'], cf, play['cubefulEquity'])
                    print('PLAY %s %s dprob=%.6f dequity=%.6f' % (xgid, play['move'], dp, de))
                done += 1
                if a.limit and done >= a.limit:
                    break
    print('SUMMARY entries=%d plays=%d max_dprob=%.6f max_dequity=%.6f missing=%d'
          % (n_entries, n_plays, max_dp, max_de, n_missing))
    if worst:
        print('WORST-EQUITY %s %s ours=%.6f book=%.6f' % worst)
    ok = n_missing == 0 and max_dp <= a.eps_prob and max_de <= a.eps_equity
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
