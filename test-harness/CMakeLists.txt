cmake_minimum_required(VERSION 3.22.1)
project(gnubg_test C)

set(ENGINE   "${CMAKE_CURRENT_SOURCE_DIR}/../engine-core")
set(GLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jni-bridge/external/glib")
set(JNI_BR   "${CMAKE_CURRENT_SOURCE_DIR}/../jni-bridge")

# Standalone executable (not a shared library)
add_executable(test_runner
    src/main.c
    src/stubs.c
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
)

target_include_directories(test_runner PRIVATE
    ${JNI_BR}
    ${ENGINE}
    ${ENGINE}/lib
    ${GLIB_DIR}/include
    ${GLIB_DIR}/include/glib-2.0
    ${GLIB_DIR}/lib/glib-2.0/include
)

target_compile_definitions(test_runner PRIVATE
    HAVE_CONFIG_H
    AC_DATADIR="/data/local/tmp"
    AC_PKGDATADIR="/data/local/tmp"
    AC_DOCDIR="/data/local/tmp"
)

target_link_libraries(test_runner
    ${GLIB_DIR}/lib/libglib-2.0.so
    ${GLIB_DIR}/lib/libintl.so
    log
    m
)
