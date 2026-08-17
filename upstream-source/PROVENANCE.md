# Vendored upstream — GNU Backgammon 1.08.003

`upstream-source/gnubg/` is the pinned, verified upstream source that every
port into `engine-core/` derives from. The single-source law requires porting
from the whole original, read in-tree — this is that original.

| | |
|---|---|
| Source | https://ftp.gnu.org/gnu/gnubg/gnubg-release-1.08.003-sources.tar.gz |
| Version | 1.08.003 (matches `engine-core/config.h`: `VERSION`) |
| Tarball sha256 | `6f7d969b13cfff786fba90ff8cc5e5d564b97f4f0aa69afe4f3838f18c445979` |
| GPG | **Good signature**, GNU keyring — Philippe Michel `<philippe.michel7@free.fr>`, RSA `39FC530C20B9B8C627E71BAC973B63D4ECB3B8BD` |

## What is here

The complete C source (`*.c`, `*.h`) plus the hand-written build inputs
(`configure.ac`, `Makefile.am`) and the licence (`COPYING`). Nothing in
`upstream-source/gnubg/` is modified from upstream — it is verbatim.

Pruned as *derived, not source*: generated autotools scaffolding (`*.in`,
`*.m4`, `aclocal`), `ChangeLog` files, and generated GUI pixbuf data.

## Reproduce

    curl -O https://ftp.gnu.org/gnu/gnubg/gnubg-release-1.08.003-sources.tar.gz
    curl -O https://ftp.gnu.org/gnu/gnubg/gnubg-release-1.08.003-sources.tar.gz.sig
    sha256sum gnubg-release-1.08.003-sources.tar.gz   # == the sha256 above
    curl -O https://ftp.gnu.org/gnu/gnu-keyring.gpg
    gpg --no-default-keyring --keyring ./gnu-keyring.gpg --verify \
        gnubg-release-1.08.003-sources.tar.gz.sig gnubg-release-1.08.003-sources.tar.gz

## Porting from here

A file copied into `engine-core/` is de-GTK'd exactly as the rest of the
engine already was: GTK code is `#if defined(USE_GTK)`-guarded (and `USE_GTK`
is undefined in the mobile build), and any bare `#include "gtkgame.h"` becomes
`#if defined(USE_GTK)` / `#include "gtk/gtkgame.h"` / `#endif`. The port copies
gnubg's own code and applies only that mechanical transform — never a rewrite.
