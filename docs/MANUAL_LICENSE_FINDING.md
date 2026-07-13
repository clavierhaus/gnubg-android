# The gnubg manual is GFDL 1.3 with NO Invariant Sections (verified)

Verified 2026-07-13 against the manual's own source: the DocBook master
`doc/allabout.xml` in gnubg-release-1.08.003-sources.tar.gz (ftp.gnu.org),
the file the gnubg.org wiki manual mirrors. Its licence declaration, quoted
verbatim from the source:

  "Permission is granted to copy, distribute and/or modify this document
   under the terms of the GNU Free Documentation License, Version 1.3 or
   any later version published by the Free Software Foundation; with no
   Invariant Sections, no Front-Cover Texts, and no Back-Cover Texts. A
   copy of the license is included in the section entitled 'GNU Free
   Documentation License'."

The manual is Albert Silver's introduction to GNU Backgammon, GFDL docbook
version; the full GFDL 1.3 text is embedded in the file (section "GNU Free
Documentation License"). The tarball's top-level COPYING is GPLv3 (the
program); the docs carry no separate licence file because the notice is
inline, as above.

## Why "no Invariant Sections" is the whole story

The vague GFDL/GPL "incompatibility" is entirely about Invariant Sections:
unmodifiable mandatory text is what makes GFDL content un-mergeable into
modifiable GPL source. This manual has NONE, plus no cover texts. It is
therefore GFDL 1.3-or-later, fully modifiable, redistributable, with the
licence carried in-document.

## Consequences for the insight layer

- **Variant A (bundle as a separate work, serve locally, display/link):**
  unambiguously clean -- GPLv3 sec.5 mere aggregation; two works, two
  licences, one device, touching only through a local request. This is the
  recommended path (the manual_ref link mechanism, CORPUS_HARVEST_PLAN
  Part 4H, given a body). Ship allabout.xml with its embedded GFDL section
  intact.
- **Variant B (extract/adapt manual text into app output):** now LEGALLY
  PERMITTED too -- with no Invariant Sections, GFDL 1.3 allows modify and
  redistribute, PROVIDED the result carries the GFDL notice and is itself
  GFDL. NOT recommended: it creates a dual-licence seam inside a single
  asset (authored GPLv3 phrase + GFDL excerpt), muddying the clean
  per-asset provenance the corpus has kept. Keep the two bodies of text in
  two files under two licences.

## Status of the maintainer's earlier decision

CORPUS_HARVEST_PLAN Section 0 recorded the maintainer's decision to use the
manual and to pull GFDL content should Richard's reply contradict it. This
finding SUPPORTS that decision on the merits: the specific barrier the
firewall feared (Invariant Sections) is absent from this manual. The
remediation commitment stands regardless; nothing here depends on it.
