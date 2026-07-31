# Cryptography in CBG — the doctrine, the stack, and the reasoning

**Scope: the whole project.** This document governs every cryptographic
decision in CBG, free edition and Plus alike. It exists so that the most
demanding reader — the GPLv3 advocate who trusts nothing that cannot be
checked with free tools — can audit not only what we chose but *how we
choose*. The method is stated first because it outlives any single decision.

Status date: 2026-07-30.

---

## 1. The decision method

Every cryptographic choice in this project is made the same way, in order:

1. **Constraints come from the project's standing rulings, not from
   preference.** Before candidates are considered, the constraints the
   decision must satisfy are written down, each traceable to a ruling that
   predates the decision.
2. **Candidates are limited to official, standard mechanisms.** Platform
   facilities and open standards first; libraries only when the platform
   cannot satisfy a constraint; custom cryptography never.
3. **Disqualification is by constraint, not taste.** A candidate is removed
   only by naming the constraint it violates. "We prefer X" is not a verdict;
   "X's defining property contradicts ruling Y" is.
4. **Ties break toward the most standard and most independently
   verifiable option.** Between two candidates that satisfy all constraints,
   the one whose artifacts can be checked by the oldest, most universal free
   tools wins.
5. **The verification burden must fall on tools the user already has.**
   If checking our work requires our software, we have failed the test the
   project set for itself: *we do not ask to be believed, we ask to be
   checked.*

## 2. The standing constraints

Each of these predates and governs the stack decision below:

- **User-owned, exportable keys.** "Nothing of value gets ever lost — save
  wherever you want, recover with your device-generated key, export the key
  to another device." A key the user cannot extract is a key the user does
  not own.
- **Plaintext records.** Signed, never encrypted. The purpose is
  tamper-evidence, not secrecy. The user's data remains readable by the user
  with any text editor, forever, with or without CBG.
- **Nothing network, ever.** No online timestamping, no remote attestation,
  no key escrow. Everything works on an offline device.
- **Verification is everyone's.** The free edition, and independent free
  tools, must be able to verify everything Plus produces. The convenience of
  automatic collection may be Plus; the freedom to check is unconditional.
- **Honest limits, stated.** Tamper-*evident*, not unforgeable. The owner of
  the device and key can, by construction, sign whatever they choose. The
  chain proves the record as kept; it does not prove the keeper's virtue.
  This is stated wherever the feature is described, never hidden.

## 3. The candidates and the verdicts

Considered for signing the career record (hash-chained, signed match ledger):

**Android Keystore** — the platform's official key-protection system.
Its defining property: key material never enters the application process and
cannot be extracted from the device, even by the app that created it.
*Verdict: disqualified as the key's home, by constraint 1.* A non-extractable
key contradicts user ownership — the user could never move their identity to
a new device. Keystore remains *permitted* in exactly one optional role:
wrapping the at-rest copy of a key that canonically lives in a standard,
exportable format. The key's portability must never depend on it.

**Google Tink** — Google's open-source (Apache-2.0), cross-platform
cryptography library, increasingly integrated into AndroidX. Open source,
so software freedom is not the objection. *Verdict: declined, by rules 4 and
5.* Tink adds a dependency and stores keys in library-specific keyset
formats; an independent verifier would need Tink to check our signatures.
That makes verification *heavier* than the platform baseline, for no
capability we need — we sign plaintext; we encrypt nothing.

**The platform's Java Cryptography Architecture (JCA)** — the documented
baseline of Android's own cryptography guidance: `java.security.Signature`
and standard key encodings, shipped in the platform since the beginning.
*Verdict: chosen.* Zero added dependencies, and every artifact it produces
is a pure, decades-old open standard.

## 4. The stack

- **Signature algorithm:** ECDSA over NIST P-256, `SHA256withECDSA` (JCA
  standard name). Signatures in standard ASN.1/DER.
- **Key encodings:** private key in PKCS#8; public key in X.509
  SubjectPublicKeyInfo, distributed as PEM in the user's own career folder.
- **Hashing:** SHA-256, over exact stored bytes.
- **Chain:** each ledger entry records the SHA-256 of the match file, the
  analysis figures as computed (with the app version that computed them),
  and the SHA-256 of the previous entry's complete stored line; the
  signature covers the entry's exact bytes as stored — never a
  re-serialization. Editing, deleting, or reordering any entry breaks the
  chain visibly.
- **Verification:** requires no CBG software. The reference check is an
  `openssl dgst -sha256 -verify` invocation, or a few lines of standard
  Python `cryptography` — tools older than this project and owned by no one.

## 5. Why this survives the GPLv3 test

The hardline reader's suspicion of signing is earned: GPLv3 §§3 and 6 exist
because cryptographic signing has been used to lock users out of their own
devices and data. This design is the inverse of that pattern, point by point:

- **The key is the user's property.** Generated on the user's device, owned
  by the user, exportable in a standard format. The project holds no copy,
  no escrow, no revocation power. There is no "our" key anywhere in the
  design.
- **Signing restricts nothing.** Nothing checks a signature before running,
  opening, or importing anything. An unsigned or chain-broken record remains
  a fully usable, fully readable record; the chain's only power is to make a
  true statement about history checkable. This is attestation *by* the
  owner, never control *over* the owner — the opposite of tivoization.
- **The data outlives the software.** The record is plaintext gnubg match
  files plus plaintext ledger lines. Delete CBG and everything remains
  readable, analysable by desktop GNU Backgammon, and verifiable with
  openssl. No format, no key, no check depends on our continued existence.
- **The spec is public; the freedom to check is unconditional.** This
  document and the verifier live in the GPL repository. Automatic
  collection is a Plus convenience; verification requires nothing from us
  and costs nothing.
- **No cryptographic dependency is added at all.** The platform's own JCA
  and open standards. There is no library to audit beyond what every
  Android device already runs, and nothing in the chain that free tools
  cannot reproduce.

A signature scheme passes the GPLv3 test when removing the signer's company
from the world removes nothing from the user. This one passes.

## 6. Crypto and the user — the harmony doctrine

Cryptography and user-friendliness are rarely compatible, and it is worth
naming why before claiming to solve it: crypto UX fails when it makes the
user perform the cryptographer's job — name a key, choose an algorithm,
guard a passphrase, interpret a fingerprint. Harmony comes from a different
assignment of labor: **the user handles life events, the software handles
cryptography, and the two meet only at moments the user already
understands.** This section is binding on every user-facing surface of the
signed career, present and future.

**1. Invisible by default.** The best crypto UX in the career design is that
there is none: the key generates itself silently, every match signs itself
silently, and the user *discovers* they have a verified career rather than
being asked to administer one. There is never a ceremony at first launch
("set up your signing identity") — that is the moment harmony dies. The
rule: crypto appears only when its *benefit* is being collected, never when
its *machinery* is running.

**2. Crypto surfaces only at life events, named as life events.** The user
meets the key exactly three times in their life, and each has a human name
that is not "key": *Move my career to a new phone.* *Restore my career.*
*Check my record.* Those are the menu entries. The words key, signature,
hash, ECDSA appear nowhere in the primary UI — not because they are hidden
(the honesty laws forbid hiding), but because they are mechanism, and
mechanism belongs one layer down.

**3. Progressive disclosure serves both audiences at once.** The primary
line speaks human; a tap below it speaks cryptographer. "Your record checks
out — nothing has been changed since it was played" up top; expand, and
there is the real sentence: the P-256 fingerprint, the entry count, and the
literal `openssl dgst -sha256 -verify …` command a skeptic can run
themselves. The casual player never scrolls; the auditor gets the receipts.
One screen, both constituencies, no dishonesty — the friendly words are
*summaries* of the true ones, never substitutes.

**4. Kill the passphrase wherever a physical act can replace it.**
Passphrases are where crypto UX goes to die: chosen badly, forgotten
reliably. For the phone-to-phone move, the QR transfer is strictly better
*and* more secure in practice — physical proximity of the two devices is
the authentication, and there is no secret to invent or remember. The
passphrase exists only where it is irreplaceable: the *file* backup of the
key (which may sit in a folder the user syncs elsewhere), and even there it
is offered honestly — protect the backup with a passphrase, or keep the
file itself somewhere safe; either is fine, and the consequences of each
are stated in two sentences. One decision, at the moment of backup. Never a
passphrase to *use* the feature — only to *back it up*.

**5. The recovery story is designed first; it is the whole point.**
"Nothing of value gets ever lost" is a UX specification, not only a
cryptographic one. New device: install CBG, point it at your folder, scan
the QR from the old phone (or open the pass file) — career alive, verified,
continuing. The flow is *rehearsable*: the settings screen describes what
recovery will look like before it is ever needed, so the user is not
learning the procedure on the day their phone died. Confidence about
recovery, given in advance, is what makes silent crypto trustworthy rather
than spooky.

**6. The in-app check: friendly front, auditable back.** One tap runs the
same verification the public script performs, on-device: "Chain intact — 47
matches, nothing altered." That is *reassurance* for the owner. It is not
*proof* for a skeptic — an app vouching for itself never is — and the
honest design says so on the same screen: "for independent verification,
run this on any computer:" followed by the command. The self-check and the
external check are different products for different needs; conflating them
is the one dishonesty this design cannot afford.

**7. When the chain breaks, speak in events, not errors.** The verifier
distinguishes tampering from damage; the UI carries that through in human
terms and without panic. "One match file was changed after it was recorded
(match-2026-07-31.sgf)" — with the honest continuation: the chain before it
still verifies; the record is not destroyed, it is *marked*. Never a red
SIGNATURE VERIFICATION FAILED. The whole feature exists to make history
legible; its failure messages must be the most legible thing in it.

**8. Key loss is stated calmly, up front, because it is genuinely okay.**
The design made key loss survivable: the record and its checkability
persist; only the *continuation* of the signed identity ends, and a new key
starts a new chapter with an honest seam. The settings text says so
plainly — lose the key and you lose nothing you played; a new key simply
signs from here on. Removing that fear is what lets users touch a crypto
feature at all.

The through-line: this project already holds the principle under another
name. **Correct-or-silent is a UX doctrine as much as an honesty
doctrine** — the coach speaks only when it has something true to say, and
the cryptography behaves identically: silent while working, articulate at
exactly the moments the user has a question, honest about its limits when
asked. Same house style, applied to keys instead of moves.

Concretely, this resolves into one settings surface — **"Your career"** —
with three actions: *Move to another phone* (QR, no passphrase), *Back up
your career pass* (file, optional passphrase, consequences in two
sentences), *Check my record* (on-device verify plus the printed openssl
line) — and one calm paragraph covering recovery and key loss. Nothing at
first launch, nothing during play, ever.
