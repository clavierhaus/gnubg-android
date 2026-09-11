cmake_minimum_required(VERSION 3.22.1)
project(gnubg_engine C)

set(ENGINE   "${CMAKE_CURRENT_SOURCE_DIR}/../engine-core")

# Warning policy (maintainer ruling 2026-08-12): a perfect build log is one
# where every warning is actionable, and therefore zero exist. Port-owned
# code compiles -Wall -Wextra -- its warnings are ours to fix. Vendored
# upstream sources (the gnubg engine, exactly as shipped) compile quiet:
# their warnings belong upstream, and "fixing" them here would mean
# forking upstream, which this port refuses. Dependencies (glib, pcre2)
# get the same treatment in build_glib_android.sh.
set(GLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glib")

add_library(gnubg-engine SHARED
    src/stubs.c
    src/native-lib.c
    src/gnubg_mobile.c
    ${ENGINE}/eval.c
    ${ENGINE}/dice.c
    ${ENGINE}/bearoff.c
    ${ENGINE}/bearoffgammon.c
    ${ENGINE}/positionid.c
    ${ENGINE}/matchequity.c
    ${ENGINE}/mec.c
    ${ENGINE}/util.c
    ${ENGINE}/file.c
    ${ENGINE}/boardpos.c
    ${ENGINE}/timecontrol.c
    ${ENGINE}/multithread.c
    ${ENGINE}/lib/output.c
    ${ENGINE}/lib/SFMT.c
    ${ENGINE}/lib/neuralnet.c
    ${ENGINE}/lib/neuralnetsse.c
    ${ENGINE}/lib/cache.c
    ${ENGINE}/lib/md5.c
    ${ENGINE}/lib/isaac.c
    ${ENGINE}/lib/list.c
    ${ENGINE}/lib/inputs.c
    ${ENGINE}/rollout.c
    ${ENGINE}/sgf.c
    ${ENGINE}/sgf_y.c
    ${ENGINE}/sgf_l.c
    ${ENGINE}/play.c
    ${ENGINE}/analysis.c
    ${ENGINE}/format.c
    ${ENGINE}/formatgs.c
    ${ENGINE}/drawboard.c
    ${ENGINE}/external.c
    ${ENGINE}/glib-ext.c
    ${ENGINE}/matchid.c
    ${ENGINE}/set.c
    ${ENGINE}/renderprefs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/android-app.c
)

target_include_directories(gnubg-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ENGINE}
    ${ENGINE}/lib
    ${GLIB_DIR}/include
    ${GLIB_DIR}/include/glib-2.0
    ${GLIB_DIR}/lib/glib-2.0/include
)

target_compile_definitions(gnubg-engine PRIVATE
    HAVE_CONFIG_H
    AC_DATADIR="/data/data/com.clavierhaus.gnubg/files"
    AC_PKGDATADIR="/data/data/com.clavierhaus.gnubg/files"
    AC_DOCDIR="/data/data/com.clavierhaus.gnubg/files"
    USE_SIMD_INSTRUCTIONS
    HAVE_NEON
    USE_NEON
)

target_link_libraries(gnubg-engine
    ${GLIB_DIR}/lib/libglib-2.0.so
    ${GLIB_DIR}/lib/libgobject-2.0.so
    ${GLIB_DIR}/lib/libintl.so
    log
    m
)

file(GLOB _vendored "${ENGINE}/*.c" "${ENGINE}/lib/*.c")
list(REMOVE_ITEM _vendored "${ENGINE}/timecontrol.c")
set_source_files_properties(${_vendored} PROPERTIES COMPILE_OPTIONS "-w")
set_source_files_properties(
    src/stubs.c src/native-lib.c src/gnubg_mobile.c "${ENGINE}/timecontrol.c"
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra")

# REPRODUCIBLE BUILD -- flags bound to the TARGET, not to CMAKE_*_FLAGS.
# The NDK toolchain file re-assigns CMAKE_C_FLAGS / CMAKE_SHARED_LINKER_FLAGS
# after the command line is read, and CMake 3.31 (F-Droid's) and CMake 4.x
# (the maintainer's) resolve that differently: on 2026-09-11 the maintainer's
# engine kept a .note.gnu.build-id and 118 KB more .text than F-Droid's build
# of the same commit. Target options are applied last, per target, by every
# CMake version identically. REPRO_ROOT and REPRO_NDK come from
# build_native_android.sh.
#   -O2 explicitly: the last -O wins in clang, so the toolchain's per-type
#       default (which differs between its legacy and new files) is moot.
#   --build-id=none: last --build-id wins in lld.
if(NOT DEFINED REPRO_ROOT OR NOT DEFINED REPRO_NDK)
    message(FATAL_ERROR "REPRO_ROOT and REPRO_NDK must be given (build_native_android.sh does)")
endif()
target_compile_options(gnubg-engine PRIVATE
    -O2
    "-ffile-prefix-map=${REPRO_ROOT}=."
    "-ffile-prefix-map=${REPRO_NDK}=/ndk"
    -Wno-builtin-macro-redefined -D__DATE__= -D__TIME__= -D__TIMESTAMP__=)
target_link_options(gnubg-engine PRIVATE
    -Wl,--build-id=none -Wl,-z,max-page-size=16384)
