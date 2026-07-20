#!/usr/bin/env python3
# insight_log.py -- sift gnubg-coach / gnubg-insight logcat into per-verdict
# digests with problem alerts. Part of the log rule (maintainer order,
# 2026-07-20): pinpointing starts from captured lines, never from theory.
#
# Usage:  adb logcat -d -s gnubg-coach gnubg-insight | python3 tools/insight_log.py
#         python3 tools/insight_log.py captured.txt
# License: GPL-3.0-or-later.
import re, sys

def main(lines):
    verdicts, cur = [], None
    for ln in lines:
        ln = ln.rstrip("\n")
        m = re.search(r'verdict\(pre\) \d+ms rank=(\d+) of=(\d+) skill=(\d+)', ln)
        if m:
            cur = {"rank": int(m.group(1)), "of": int(m.group(2)),
                   "skill": int(m.group(3)), "raw": []}
            verdicts.append(cur)
        if cur is None:
            continue
        cur["raw"].append(ln)
        m = re.search(r"decoded rank=\d+ of=\d+ played='([^']*)'", ln)
        if m: cur["played"] = m.group(1)
        m = re.search(r'APPLYPROBE fpPre=(\S+) fpP=(\S+) fpB=(\S+)', ln)
        if m: cur["fp"] = (m.group(1), m.group(2), m.group(3))
        m = re.search(r'match skill=(\S+(?: bad)?) .* cand=(\d+) fired=(\d+)', ln)
        if m: cur["match"] = (m.group(1), int(m.group(2)), int(m.group(3)))
        if "misses: " in ln:
            cur["misses"] = re.findall(r'(\S+)\[(\S+) (\w+) vp=([-\d.,]+) vb=([-\d.,]+) d=([-\d.,]+)\]',
                                       ln.split("misses: ",1)[1])
        m = re.search(r'narrator: consulted=\d+ candidates=(\d+) narrated=(\d+)(?: \[([^\]]*)\])?', ln)
        if m: cur["narr"] = (int(m.group(1)), int(m.group(2)), m.group(3) or "")
        if "QUORUMPROBE" in ln:
            cur["quorum"] = ln.split("QUORUMPROBE ",1)[1]

    skillword = {0: "very bad", 1: "bad", 2: "doubtful", 3: "ok"}
    alerts = 0
    for i, v in enumerate(verdicts):
        flagged = v["skill"] != 3
        print(f"== verdict {i+1}: played '{v.get('played','?')}'  rank {v['rank']} of {v['of']}  [{skillword[v['skill']]}]")
        if "fp" in v:
            pre, fp, fb = v["fp"]
            if fp == fb or "EMPTY" in (fp, fb):
                print(f"   !! GHOST: derived boards fpP={fp} fpB={fb} (pre={pre}) -- apply/decode defect")
                alerts += 1
        if flagged:
            mt = v.get("match")
            nr = v.get("narr")
            if mt: print(f"   corpus: candidates={mt[1]} fired={mt[2]}")
            if nr: print(f"   narrator: candidates={nr[0]} narrated={nr[1]}" + (f" [{nr[2]}]" if nr[2] else ""))
            if mt is None and nr is None:
                print("   !! WHY-LAYER ABSENT: flagged verdict, but no match/narrator/probe line at all --")
                print("      the insight effect never ran (composition or gating defect, not a rules gap)")
                alerts += 1
            elif mt and mt[2] == 0 and (not nr or nr[1] == 0):
                print("   !! SILENCE on a flagged move -- nearest misses:")
                for e, t, gate, vp, vb, d in (v.get("misses") or [])[:4]:
                    print(f"        {e:26} {t:22} gate={gate:4} vp={vp} vb={vb} d={d}")
                alerts += 1
        if "quorum" in v:
            print(f"   quorum: {v['quorum'][:160]}")
    print(f"\n{len(verdicts)} verdicts, {alerts} alert(s).")

if __name__ == "__main__":
    src = open(sys.argv[1]) if len(sys.argv) > 1 else sys.stdin
    main(src.readlines())
