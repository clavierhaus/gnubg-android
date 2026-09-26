# Host build of the port's engine subset for the opening-book conformance
# check (tools/book_conformance/README.md). Same derivation as
# tools/rollout_harness/Makefile: the source list is DERIVED FROM
# jni-bridge/CMakeLists.txt AT BUILD TIME; only native-lib.c (JNI marshalling)
# is swapped for conformance.c + the shared host_compat.c.

ROOT    := ../..
ENGINE  := $(ROOT)/engine-core
BRIDGE  := $(ROOT)/jni-bridge

CC      ?= gcc
GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0 gobject-2.0)
GLIB_LIBS   := $(shell pkg-config --libs glib-2.0 gobject-2.0)

# Extract the add_library source lines from the CMake file verbatim,
# then map CMake variables onto host paths.
CMAKE_SRC := $(shell sed -n '/^add_library(/,/^)/p' $(BRIDGE)/CMakeLists.txt \
    | grep -E '\.c$$' \
    | sed -e 's/^ *//' \
          -e 's|^\$${ENGINE}|$(ENGINE)|' \
          -e 's|^\$${CMAKE_CURRENT_SOURCE_DIR}|$(BRIDGE)|' \
          -e 's|^src/|$(BRIDGE)/src/|')

# The one deliberate substitution: JNI marshalling out, harness in.
ENGINE_SRC := $(filter-out $(BRIDGE)/src/native-lib.c,$(CMAKE_SRC))

CFLAGS  := -O2 -DHAVE_CONFIG_H \
           -DAC_DATADIR='"./tmp"' -DAC_PKGDATADIR='"./tmp"' -DAC_DOCDIR='"./tmp"' \
           -I $(ROOT)/tools/shim -I $(BRIDGE) -I $(BRIDGE)/include \
           -I $(ENGINE) -I $(ENGINE)/lib $(GLIB_CFLAGS)

SRC := conformance.c ../rollout_harness/host_compat.c $(ENGINE_SRC)

conformance: $(SRC)
	@echo "engine sources derived from CMakeLists.txt: $(words $(ENGINE_SRC)) files"
	$(CC) $(CFLAGS) -o $@ $(SRC) $(GLIB_LIBS) -lm -lpthread

clean:
	rm -f conformance

.PHONY: clean
