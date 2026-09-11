#!/usr/bin/env python3
"""Append (or replace) one build block in an F-Droid metadata recipe.

    tools/fdroid_recipe_append.py <recipe.yml> <versionName> <versionCode> <commit> [template.yml]

The recipe keeps every published version. The NEW block's shape (sudo:,
gradle:, build:, ndk:) comes from the template's first block when a
template is given -- the repository's fdroid/<appid>.yml, the single source
of how this app is built -- otherwise from the recipe's own last block. Its
versionName / versionCode / commit are set and any 'disable:' dropped; an
existing block for the same versionCode is replaced, so re-runs are
idempotent. CurrentVersion / CurrentVersionCode follow. Everything above is
untouched. (A global sed over versionName/versionCode once rewrote all three
published blocks to the new version -- 'Builds has non-unique elements', no
APK -- 2026-09-10.) Single source: release_fdroid.sh and the local F-Droid
build both call this.
"""
import re
import sys

def main():
    if len(sys.argv) not in (5, 6):
        sys.exit(__doc__)
    meta, ver, code, sha = sys.argv[1:5]
    template = sys.argv[5] if len(sys.argv) == 6 else None
    s = open(meta).read()
    m = re.search(r'^Builds:\n', s, re.M)
    if not m:
        sys.exit("no Builds: key in " + meta)
    body_start = m.end()
    tail = re.search(r'^\S', s[body_start:], re.M)
    body_end = body_start + tail.start() if tail else len(s)
    blocks = [b for b in re.split(r'(?=^  - versionName:)', s[body_start:body_end], flags=re.M) if b.strip()]
    if not blocks:
        sys.exit("no build blocks in " + meta)
    blocks = [b for b in blocks if not re.search(r'^    versionCode: %s$' % code, b, re.M)]
    if not blocks:
        sys.exit("no earlier build block to copy in " + meta)
    new = blocks[-1]
    if template:
        t = open(template).read()
        tm = re.search(r'^Builds:\n', t, re.M)
        if not tm:
            sys.exit("no Builds: key in template " + template)
        ttail = re.search(r'^\S', t[tm.end():], re.M)
        tbody = t[tm.end():tm.end() + ttail.start()] if ttail else t[tm.end():]
        tblocks = [b for b in re.split(r'(?=^  - versionName:)', tbody, flags=re.M) if b.strip()]
        if not tblocks:
            sys.exit("no build block in template " + template)
        new = tblocks[0]
    new = re.sub(r'^  - versionName: .*$', '  - versionName: %s' % ver, new, count=1, flags=re.M)
    new = re.sub(r'^    versionCode: .*$', '    versionCode: %s' % code, new, count=1, flags=re.M)
    new = re.sub(r'^    commit: .*$', '    commit: %s' % sha, new, count=1, flags=re.M)
    new = re.sub(r'^    disable:.*\n', '', new, flags=re.M)
    if not new.endswith('\n\n'):
        new = new.rstrip('\n') + '\n\n'
    blocks.append(new)
    s = s[:body_start] + ''.join(blocks) + s[body_end:]
    s = re.sub(r'^CurrentVersion: .*$', 'CurrentVersion: %s' % ver, s, flags=re.M)
    s = re.sub(r'^CurrentVersionCode: .*$', 'CurrentVersionCode: %s' % code, s, flags=re.M)
    open(meta, 'w').write(s)
    print("%s: build block %s (%s) at %s" % (meta, ver, code, sha[:12]))

if __name__ == '__main__':
    main()
