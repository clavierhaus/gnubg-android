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
