#!/usr/bin/env python3
"""Blank the DT_RUNPATH / DT_RPATH string of an ELF64 shared object in place.

    tools/blank_runpath.py <lib.so>...

Why: meson links its internal subprojects with a build-time RUNPATH
($ORIGIN/../subprojects/libffi-.../src:$ORIGIN/../glib:...), and what it
does with that string at install time is version-dependent -- meson 1.11
overwrites it with 'X' characters of the same length, meson 1.7 (Debian
trixie, F-Droid's builder) leaves it. Same source and compiler, different
bytes (2026-09-11: libgobject, libgmodule, libgthread differed in exactly
those 96 bytes). This does what the newer meson does, on every host, so
both sides converge on the same bytes. The dynamic entry is kept; only the
string is blanked. Android's linker ignores DT_RUNPATH anyway.

Pure Python, ELF64 little-endian only (arm64-v8a is all this port ships).
"""
import struct
import sys

DT_NULL, DT_RPATH, DT_RUNPATH, DT_STRTAB = 0, 15, 29, 5
SHT_DYNAMIC, SHT_STRTAB = 6, 3


def blank(path):
    with open(path, 'r+b') as f:
        data = bytearray(f.read())
        if data[:4] != b'\x7fELF' or data[4] != 2 or data[5] != 1:
            sys.exit("%s: not a little-endian ELF64 file" % path)
        e_shoff, = struct.unpack_from('<Q', data, 0x28)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', data, 0x3a)
        secs = []
        for i in range(e_shnum):
            off = e_shoff + i * e_shentsize
            name, typ, flags, addr, offset, size, link = struct.unpack_from('<IIQQQQI', data, off)
            secs.append((typ, addr, offset, size, link))
        dyn = next((s for s in secs if s[0] == SHT_DYNAMIC), None)
        if dyn is None:
            print("%s: no .dynamic section, nothing to do" % path)
            return
        _, _, dyn_off, dyn_size, link = dyn
        strtab = secs[link]
        str_off, str_size = strtab[2], strtab[3]
        changed = 0
        for i in range(dyn_size // 16):
            tag, val = struct.unpack_from('<qQ', data, dyn_off + i * 16)
            if tag == DT_NULL:
                break
            if tag in (DT_RPATH, DT_RUNPATH):
                start = str_off + val
                end = data.index(b'\x00', start, str_off + str_size)
                n = end - start
                if n and data[start:end] != b'X' * n:
                    data[start:end] = b'X' * n
                    changed += n
        # Both meson generations may also drop the DT_RUNPATH entry itself and
        # leave the dead string behind in .dynstr (observed: entry absent on
        # both sides, strings different). Dead or live, an $ORIGIN path in
        # .dynstr is build-rpath residue: blank it wherever it sits.
        pos = str_off
        end_tab = str_off + str_size
        while pos < end_tab:
            e = data.index(b'\x00', pos, end_tab + 1) if b'\x00' in data[pos:end_tab + 1] else end_tab
            if data[pos:pos + 7] == b'$ORIGIN':
                n = e - pos
                if data[pos:e] != b'X' * n:
                    data[pos:e] = b'X' * n
                    changed += n
            pos = e + 1
        f.seek(0)
        f.write(data)
        print("%s: %s" % (path, ("blanked %d RUNPATH bytes" % changed) if changed else "RUNPATH already blank or absent"))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        blank(p)
