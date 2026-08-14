# Host build of the port's engine subset (docs/MULTICORE_ANALYSIS.md 2.10).
#
# The source list is DERIVED FROM jni-bridge/CMakeLists.txt AT BUILD TIME --
# the same add_library block that builds the device binary -- so this
# harness can never silently drift from the real build. Only native-lib.c
# (JNI marshalling) is swapped for harness.c + host_compat.c.

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

SRC := harness.c host_compat.c $(ENGINE_SRC)

harness: $(SRC)
	@echo "engine sources derived from CMakeLists.txt: $(words $(ENGINE_SRC)) files"
	$(CC) $(CFLAGS) -o $@ $(SRC) $(GLIB_LIBS) -lm -lpthread

clean:
	rm -f harness

.PHONY: clean
