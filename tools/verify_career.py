#!/usr/bin/env python3
"""Verify a CBG career ledger. FOSS -- verification is everyone's.

Spec: docs/CRYPTOGRAPHY.md (public). A career folder contains:
    career-pubkey.pem     the player's public key (X.509 SPKI, PEM)
    career-ledger.jsonl   one line per match:  entryJSON \t base64(DER sig)
    *.sgf                 the plain gnubg match files the entries reference

Checks, per entry, in order:
  1. SIGNATURE  -- SHA256withECDSA over the entry's exact bytes (before the
                   tab), verified with stock openssl. No CBG code involved.
  2. CHAIN      -- entry.prev == sha256 of the previous complete line's bytes
                   (before its newline); first entry must say "genesis".
  3. MATCH FILE -- sha256 of the referenced .sgf equals entry.sha256.

The verdicts distinguish tamper from damage honestly:
  - a bad signature or broken back-link or altered sgf  -> CHAIN BROKEN
  - a truncated final line (no newline)                 -> incomplete last
    entry (crash during append; not a chain break)
  - an .sgf present but referenced by no entry          -> saved but
    uncollected (collection was interrupted; not a chain break)
  - a referenced .sgf missing                           -> MISSING FILE

Requires: python3, openssl. Nothing else.
Usage: tools/verify_career.py /path/to/career-folder
"""
import base64
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def sha256_hex(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def openssl_verify(pubkey: Path, payload: bytes, der_sig: bytes) -> bool:
    with tempfile.NamedTemporaryFile() as pf, tempfile.NamedTemporaryFile() as sf:
        pf.write(payload); pf.flush()
        sf.write(der_sig); sf.flush()
        r = subprocess.run(
            ["openssl", "dgst", "-sha256", "-verify", str(pubkey),
             "-signature", sf.name, pf.name],
            capture_output=True, text=True)
        # openssl prints "Verified OK" on success; exit code 0.
        return r.returncode == 0 and "Verified OK" in r.stdout


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    folder = Path(sys.argv[1])
    pub = folder / "career-pubkey.pem"
    ledger = folder / "career-ledger.jsonl"
    if not pub.is_file():
        print(f"MISSING: {pub.name} -- cannot verify anything without the public key")
        return 1
    if not ledger.is_file():
        print(f"MISSING: {ledger.name} -- no ledger to verify")
        return 1

    raw = ledger.read_bytes()
    body, sep, tail = raw.rpartition(b"\n")
    incomplete_tail = tail != b""     # bytes after the last newline: truncated append
    lines = body.split(b"\n") if body else []

    referenced: set[str] = set()
    prev_expected = "genesis"
    broken = 0

    for i, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        entry_bytes, tab, sig_b64 = line.partition(b"\t")
        if tab != b"\t":
            print(f"entry {i}: CHAIN BROKEN -- malformed line (no signature separator)")
            broken += 1
            prev_expected = sha256_hex(line)
            continue
        # 1. signature over the exact stored bytes
        try:
            der = base64.b64decode(sig_b64, validate=True)
        except Exception:
            print(f"entry {i}: CHAIN BROKEN -- signature not decodable")
            broken += 1
            prev_expected = sha256_hex(line)
            continue
        if not openssl_verify(pub, entry_bytes, der):
            print(f"entry {i}: CHAIN BROKEN -- signature does not verify")
            broken += 1
        # 2. parse AFTER verifying; the bytes are the authority
        try:
            e = json.loads(entry_bytes)
        except Exception:
            print(f"entry {i}: CHAIN BROKEN -- entry is not valid JSON")
            broken += 1
            prev_expected = sha256_hex(line)
            continue
        if e.get("prev") != prev_expected:
            print(f"entry {i}: CHAIN BROKEN -- back-link mismatch "
                  f"(says {str(e.get('prev'))[:16]}…, chain expects {prev_expected[:16]}…)")
            broken += 1
        # 3. the match file itself
        name = e.get("match", "")
        referenced.add(name)
        sgf = folder / name
        if not sgf.is_file():
            print(f"entry {i}: MISSING FILE -- {name}")
            broken += 1
        elif sha256_hex(sgf.read_bytes()) != e.get("sha256"):
            print(f"entry {i}: CHAIN BROKEN -- {name} does not match its recorded hash")
            broken += 1
        prev_expected = sha256_hex(line)

    if incomplete_tail:
        print("note: incomplete last entry (append was interrupted; not a chain break)")
    for sgf in sorted(folder.glob("*.sgf")):
        if sgf.name not in referenced:
            print(f"note: {sgf.name} saved but uncollected (not a chain break)")

    n = sum(1 for l in lines if l.strip())
    if broken == 0:
        print(f"CHAIN INTACT -- {n} entr{'y' if n == 1 else 'ies'} verified "
              f"(signatures, back-links, match files)")
        return 0
    print(f"RESULT: {broken} problem(s) across {n} entries")
    return 1


if __name__ == "__main__":
    sys.exit(main())
