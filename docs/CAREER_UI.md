# "Your career" — the UI of the signed record (CBG Pro)

**This documents a CBG Pro surface. The document itself is public by
principle: no CBG documentation ever resides behind the paywall.**

This is the binding UI design for the career's user-facing surface,
implementing `docs/CRYPTOGRAPHY.md` §6 (the harmony doctrine) inside the
app's existing visual language. Every visible string is specified here
verbatim; implementation may not invent copy. The doctrine's test applies to
every element: the user handles life events, the software handles
cryptography, and the two meet only at moments the user already understands.

Status date: 2026-07-31.

---

## 1. Where it lives, and where it deliberately does not

- **One surface: a "Career" tab in Settings**, Plus edition only. The tab
  label renders in `PlusUi.Interactive` orange — by the standing convention,
  orange means "this interaction exists only in Plus," so the tab is its own
  honest designation. The free edition shows no Career tab at all: absence is
  the signal, never a greyed teaser (the hub convention, applied here).
- **Nowhere else.** No first-launch ceremony, no dialog after the first
  collected match, no badge, no prompt during play, ever. The career
  announces itself the way Plus announces everything: it is simply there,
  orange, when the user next opens Settings. (§6.1: crypto appears when its
  benefit is collected, never while its machinery runs.)

## 2. The surface

Standard Settings structure (`SettingsSection`, existing row metrics), four
blocks top to bottom:

### 2a. Status — the human sentence first

A single plain line, panel font, no heading above it:

> Your record: 47 matches, chain intact — nothing has been changed since it
> was played.

Empty state (no matches collected yet):

> Your record starts with your next finished Play match. Coach matches are
> practice — they are never recorded.

Below the status line, a collapsed **Details** row. Expanded, it speaks
cryptographer (§6.3 — progressive disclosure; this is the only place the
mechanism vocabulary appears):

> Record: 47 entries in career/career-ledger.jsonl
> Key: P-256, fingerprint `SHA256:3f9a…c41d`
> Public key: career/career-pubkey.pem
>
> Independent verification, on any computer:
> `tools/verify_career.py /path/to/your/career`
> or per entry: `openssl dgst -sha256 -verify career-pubkey.pem -signature …`

Monospace, selectable text. The friendly sentence above is a summary of
these lines, never a substitute for them.

### 2b. The three life events (§6.2)

Three action rows, labels in `PlusUi.Interactive` orange — these are the
only actions on the surface, named as life events, the word "key" absent:

**Move to another phone** →
  Screen 1 (this phone): a QR code filling the height, above it one line:
  > Scan this with CBG on your new phone.
  And beneath, small, the honest warning — stated because it is true, not
  because it is likely:
  > Anyone who scans this code can sign your record. Show it only to your
  > own phone.
  Screen 2 (new phone, reached from the same row): the scanner, one line:
  > Point the camera at the code on your old phone.
  On success: *Your career moved. 47 matches, chain intact.* Physical
  proximity is the authentication; there is no passphrase in this flow
  (§6.4).

**Back up your career pass** →
  A single screen. One paragraph, then two choices:
  > Your career pass lets a future phone continue your record. Keep the
  > file somewhere safe — with a passphrase, the file alone is useless to
  > anyone who finds it; without one, the file itself must stay private.
  Buttons: `Save with passphrase` / `Save without`. Either lands in the
  system file picker, default name `career-pass.cbgkey`, saved wherever the
  user chooses. One decision, at the moment of backup, consequences in two
  sentences (§6.4). Passphrase entry, when chosen, is one field plus
  confirmation — no strength meter, no rules, no expiry.

**Check my record** →
  Runs the same verification the public script performs, on-device, with a
  progress line ("checking 47 entries…"). Result per §2d below, and under
  any result:
  > This check ran on this phone. For proof that needs no one's word —
  > including ours — run the same check on any computer:
  > `tools/verify_career.py /path/to/your/career`
  (§6.6: self-check is reassurance; independent proof is the script; the
  two are never conflated.)

### 2c. The calm paragraph (§6.5, §6.8)

Plain text at the bottom of the surface, always visible, italic per the
app-voice convention:

> *If this phone is ever lost: install CBG on a new one, point it at your
> folder, and use your career pass or the code from your old phone — your
> record continues, verified. And if the pass is lost too, nothing you
> played is gone: every match and its whole history stay readable and
> checkable forever. A new pass simply signs from that day on.*

Recovery rehearsed before it is needed; key loss named as survivable, in
advance, in the same breath.

### 2d. When the check does not say "intact" (§6.7)

The verifier's verdicts map to event sentences — never error styling, never
red, no stack of failures. The chain's power is to make history legible;
these lines are history:

| Verifier verdict            | Surface copy                                                                 |
|-----------------------------|------------------------------------------------------------------------------|
| sgf hash mismatch           | One match file was changed after it was recorded (match-2026-07-31.sgf). Everything before it still checks out. |
| signature failure           | One entry was rewritten after it was recorded (entry 12). Everything before it still checks out. |
| back-link mismatch          | The record's order was changed after entry 11. Everything up to there still checks out. |
| missing referenced sgf      | One recorded match file is missing (match-2026-07-31.sgf).                    |
| incomplete last entry       | The last entry wasn't fully written — likely an interruption, not a change. Your next match continues the record. |
| sgf saved but uncollected   | One match was saved but never entered the record — likely an interruption.    |

Multiple findings collapse to the earliest, plus "…and N further findings —
the full list is in the computer check." The marked record remains fully
usable: nothing locks, nothing hides (attestation, never control).

## 3. What is never on this surface

- The words key, signature, hash, ECDSA, chain — outside the Details
  disclosure and this document.
- Any action during play or at match end. Collection is silent, always.
- Any red, any exclamation mark, any "FAILED".
- Any network anything.

## 4. Implementation notes (decisions flagged, not made)

- **QR needs two capabilities the app has never had**: QR *generation* (a
  small Apache-2.0 dependency, e.g. zxing-core, or a self-contained
  generator) and QR *scanning* on the receiving phone — which means the
  app's first-ever runtime permission (camera), requested only inside that
  screen, at the moment of scanning. Both are maintainer decisions before
  build: dependency policy and permission policy are project identity, not
  implementation detail. The pass-file flow is complete without either —
  the QR flow can ship second.
- The move flow transfers the PKCS#8 private key + SPKI public key; the
  wire format (QR payload / .cbgkey file layout, with and without
  passphrase wrapping) gets its own short spec in `docs/CRYPTOGRAPHY.md`
  before implementation, so the pass file is documented and readable by
  standard tools like everything else.
- SettingsScreen.kt is currently shared with FOSS; the Career tab moves it
  into the plus overlay (the GameLayout/GameViewModel precedent — merge
  burden accepted knowingly or the tab is injected by another mechanism;
  decide at build).
