dnl To rebuild the `configure' script from this, execute the command
dnl     autoconf
dnl in the directory containing this script.

dnl Copyright (C) 1999-2002 Gary Wong <gtw@gnu.org>
dnl Copyright (C) 2002-2024 the AUTHORS

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <https://www.gnu.org/licenses/>.


AC_INIT([GNU Backgammon],m4_esyscmd(echo 1.08.003$VERSION_EXT | tr -d '\n'),[bug-gnubg@gnu.org],[gnubg])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CONFIG_SRCDIR(gnubg.c)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([config.h])

dnl
dnl parse default options
dnl

if test -f config.options; then
  . ./config.options
fi

dnl
dnl host specifics
dnl

dnl Only relevant for win32 but the macro has to be executed unconditionally
dnl -Wl since libtool uses gcc as linker
dnl
AX_APPEND_LINK_FLAGS([-Wl,--large-address-aware],,,)

win32=no

AC_MSG_CHECKING([for host])
case "$host" in
  *-*-mingw*)
    win32=yes
    CFLAGS="$CFLAGS -mms-bitfields"
    LDFLAGS="$LDFLAGS -lws2_32 -lwinmm"
    if [[[ "$(uname -or)" == *Msys ]]]; then
        AC_DEFINE(USABLE_UNDER_MSYS, 1, [Using Msys])
    fi
    ;;
  *-*-darwin*)
    darwin=yes
    LDFLAGS="$LDFLAGS -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib"
    AC_DEFINE(USE_APPLE_OPENGL, 1, [Define if using Apple OpenGL])
    ;;
  *-*-*bsd*)
    CPPFLAGS="$CPPFLAGS -I${LOCALBASE:-/usr/local}/include"
    LDFLAGS="$LDFLAGS -L${LOCALBASE:-/usr/local}/lib"
    ;;
esac
AM_CONDITIONAL(WIN32, test "x$win32" = "xyes")
AC_MSG_RESULT([$host])

if test "x$win32" = "xyes"; then
AC_CHECK_TOOL(WINDRES, windres)
fi

dnl
dnl Compiler setup
dnl

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AM_PROG_CC_C_O
dnl Deprecated but RHEL6/gcc 4.4 needs it
AC_PROG_CC_STDC

AM_CONDITIONAL(CROSS_COMPILING, test "x$cross_compiling" = "xyes")

dnl
dnl Checks for programs.
dnl

AC_PROG_AWK
AC_PROG_LN_S
AC_CHECK_PROGS(DOCBOOK2TEXI, docbook2x-texi db2x_docbook2texi, false)
AC_CHECK_PROGS(DOCBOOK2MAN, docbook2x-man db2x_docbook2man, false)
AC_CHECK_PROGS(DBLATEX, dblatex, false)
AC_CHECK_PROGS(XSLTPROC, xsltproc, false)
AC_CHECK_PROGS(GMSGFMT, msgfmt, false)
AC_ARG_VAR([DOCBOOK2TEXI], [path for docbook2texi from docbook2x package])
AC_ARG_VAR([DOCBOOK2MAN], [path for docbook2man from docbook2x package])
AC_ARG_VAR([DBLATEX], [path for dblatex used to build pdf documentation])
AC_ARG_VAR([XSLTPROC], [path for xsltproc used to build html documentation])
AC_ARG_VAR([GMSGFMT], [path for msgfmt used to compile message catalogs])
LIBCURL_CHECK_CONFIG( , , , )

dnl
dnl Setup libtool
dnl

AC_CONFIG_MACRO_DIR([m4])
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

dnl
dnl yacc and lex setup
dnl

# Sometimes Flex is installed as Lex, e.g., NetBSD.
AC_CHECK_PROG([FLEX], [flex lex], [flex])
# Force the use of `missing' to wrap Flex invocations.
AM_MISSING_PROG([LEX], [$FLEX])

AM_PROG_LEX
AX_PROG_FLEX_VERSION([2.5.9],,AC_MSG_ERROR([flex 2.5.9 or higher is needed to build GNU Backgammon]))
AC_PROG_YACC
AM_YFLAGS='-d'
AC_SUBST(AM_YFLAGS)


dnl
dnl check for compiler support
dnl

glib2minrev=22.0

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.$glib2minrev], have_glib="yes", AC_MSG_ERROR([You need to have glib2 version 2.$glib2minrev or higher to compile GNU Backgammon]))

PKG_CHECK_MODULES(GOBJECT, [gobject-2.0], have_gobject="yes", AC_MSG_WARN([no gobject support in glib]))
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0], have_gthread="yes", AC_MSG_WARN([no thread support in glib]))
PKG_CHECK_MODULES(FREETYPE2, [freetype2], have_freetype="yes", AC_MSG_WARN([no freetype support]))
PKG_CHECK_MODULES(LIBPNG, [libpng >= 1.2], have_libpng="yes", AC_MSG_WARN([no png support]))
PKG_CHECK_MODULES(CAIRO, [cairo >= 1.2], have_cairo="yes", [AC_MSG_WARN([no cairo support])])
PKG_CHECK_MODULES(PANGOCAIRO, [pangocairo >= 1.0], have_pangocairo="yes", [AC_MSG_WARN([no pangocairo support])])
PKG_CHECK_MODULES(SQLITE, [sqlite3], have_sqlite="yes", AC_MSG_WARN([no sqlite support]))

if test "x$win32" = "xyes"; then
    have_canberra="no"
    have_gl="yes"
    have_glu="yes"
else
    if test "x$darwin" = "xyes"; then
        have_canberra="no"
    elif test "x$with_gtk" != "xno"; then
        if test "x$with_gtk3" = "xyes"; then
            PKG_CHECK_MODULES(CANBERRA, [libcanberra-gtk3], have_canberra="yes", AC_MSG_WARN([no libcanberra-gtk3 support]))
        else
            PKG_CHECK_MODULES(CANBERRA, [libcanberra-gtk], have_canberra="yes", AC_MSG_WARN([no libcanberra-gtk support]))
        fi
    fi
    PKG_CHECK_MODULES(GL, [gl], have_gl="yes", AC_MSG_WARN([no gl support]))
    PKG_CHECK_MODULES(GLU, [glu], have_glu="yes", AC_MSG_WARN([no glu support]))
fi

if test "x$with_gtk" != "xno"; then
    if test "x$with_gtk3" = "xyes"; then
        PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.22.0], have_gtk="yes", AC_MSG_WARN([no gtk3 support]))
    else
        PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.18.0], have_gtk="yes", AC_MSG_WARN([no gtk2 support]))
    fi

    if test "x$with_board3d" != "xno"; then
        if test "x$with_gtk3" = "xyes"; then
            PKG_CHECK_MODULES(EPOXY, [epoxy], have_epoxy="yes", AC_MSG_WARN([no epoxy support]))
        else
            PKG_CHECK_MODULES(GTKGLEXT, [gtkglext-1.0 >= 1.0], have_gtkglext="yes" , AC_MSG_WARN([no gtkglext support]))
        fi
    fi
fi

if test "x$have_freetype" = "xyes"; then
	AC_DEFINE(HAVE_FREETYPE, 1, [Define if you have freetype2])
fi

if test "x$have_libpng" = "xyes"; then
	AC_DEFINE(HAVE_LIBPNG, 1, [Define if you have libpng])
fi

if test "x$have_cairo" = "xyes"; then
	AC_DEFINE(HAVE_CAIRO,1, [Define if you want to use libcairo])
fi

if test "x$have_pangocairo" = "xyes"; then
	AC_DEFINE(HAVE_PANGOCAIRO,1, [Define if you want to use pangocairo])
fi

AC_ARG_WITH(sqlite,[  --with-sqlite           use sqlite (Default if found)])
if test "$with_sqlite" != "no" && test "x$have_sqlite" = "xyes"; then
	AC_DEFINE(USE_SQLITE,1, [Define if you want to use sqlite])
fi
AM_CONDITIONAL(USE_SQLITE, test "$with_sqlite" != "no" && test "$have_sqlite" != "no" )

dnl If OSX version < 10.6 (Prior to Snow Leopard) then try to build
dnl with Quicktime, otherwise use CoreAudio if it is available
if test "x$darwin" = "xyes"; then
	needquicktime=no
	AS_VERSION_COMPARE("$(uname -r)", "10.0", needquicktime=yes, [], [])

	if test "x$needquicktime" = "xyes"; then
		quicktime=no
	 	AC_CHECK_HEADER([QuickTime/QuickTime.h], [quicktime=yes])

		if test "x$quicktime" = "xyes"; then
			AC_DEFINE(HAVE_APPLE_QUICKTIME,1, [Define if you have Apple QuickTime])
			LDFLAGS="$LDFLAGS -framework QuickTime"
		fi
	else
		coreaudio=no
		AC_CHECK_HEADER([AudioToolbox/AudioToolbox.h], [AC_CHECK_HEADER([CoreAudio/CoreAudioTypes.h],[coreaudio=yes])])

		if test "x$coreaudio" = "xyes"; then
			AC_DEFINE(HAVE_APPLE_COREAUDIO,1, [Define if you have Apple CoreAudio])
			LDFLAGS="$LDFLAGS -framework AudioUnit -framework CoreServices -framework AudioToolBox -framework CoreAudio"
		fi
    fi
fi

if test "x$have_canberra" = "xyes"; then
	AC_DEFINE(HAVE_CANBERRA,1, [Define if you want to use gstreamer])
fi

dnl have_gthread, have_gtk and have_gtk are handled later, not having glib is fatal

AX_C___ATTRIBUTE__()

if test "x$ax_cv___attribute__" = "xyes"; then
AX_GCC_FUNC_ATTRIBUTE(const)
AX_GCC_FUNC_ATTRIBUTE(fallthrough)
AX_GCC_FUNC_ATTRIBUTE(format)
AX_GCC_FUNC_ATTRIBUTE(pure)
AX_GCC_FUNC_ATTRIBUTE(unused)
fi

dnl
dnl check for pkg-config controlled libs
dnl


dnl
dnl Checks for libraries.
dnl

AC_CHECK_LIB(m,sqrt)
AC_SEARCH_LIBS(gethostbyname,nsl)
AC_SEARCH_LIBS(inet_aton,resolv)
if test "x$win32" = "xyes"; then
AC_DEFINE(HAVE_SOCKETS,1,Define if the system supports sockets.)
else
AC_SEARCH_LIBS(connect,socket,AC_DEFINE(HAVE_SOCKETS,1,Define if the system supports sockets.))
fi

dnl
dnl Check for readline
dnl

bcrl=n
AC_CHECK_LIB(ncurses,tparm,TERMLIB=-lncurses, AC_CHECK_LIB(termcap,tgetent,TERMLIB=-ltermcap))
AC_CHECK_LIB(readline,readline,
             [AC_CHECK_HEADER(readline/readline.h,
              READLINE_LIBS="-lreadline $TERMLIB";bcrl=y)],
              READLINE_LIBS="")
if test "$bcrl" = "y" ; then
    AC_DEFINE([HAVE_LIB_READLINE],1,Define if you have libreadline)
    AC_CHECK_HEADERS(stdarg.h)
    fi
AC_SUBST(READLINE_LIBS)

dnl
dnl Check for gmp
dnl

bcgmp=n
AC_CHECK_LIB(gmp,__gmpz_import,
             [AC_CHECK_HEADER(gmp.h,
              GMP_LIB="-lgmp";bcgmp=y)],
              GMP_LIB="")
if test "$bcgmp" = "y" ; then
    AC_DEFINE([HAVE_LIBGMP],1,Define if you have libgmp)
    fi
AC_SUBST(GMP_LIB)

dnl
dnl Check for cglm
dnl
have_cglm="no"
CPPFLAGS_bak="$CPPFLAGS"
CPPFLAGS="-I. $CPPFLAGS"
AC_CHECK_HEADER([cglm/version.h],[have_cglm="yes"])
CPPFLAGS="$CPPFLAGS_bak"

dnl
dnl Checks for header files.
dnl

AC_CHECK_HEADERS(sys/resource.h sys/socket.h sys/time.h sys/types.h unistd.h)
AC_CHECK_HEADERS(mcheck.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl

AC_C_INLINE
AC_TYPE_SIZE_T

dnl
dnl Checks for library functions.
dnl

AC_CHECK_FUNCS(sigaction sigvec break)
AC_CHECK_FUNCS(strptime setpriority)
AC_CHECK_FUNCS(mtrace)
AC_CHECK_FUNCS(clock_gettime)

dnl 
dnl Check for aligned allocation functions
dnl 

AC_CHECK_FUNCS(posix_memalign _aligned_malloc)

dnl
dnl Checks for compiler builtins
dnl

AX_GCC_BUILTIN(__builtin_clz)
AX_GCC_BUILTIN(__builtin_expect)

dnl *******************
dnl optional components
dnl *******************

AC_MSG_CHECKING([wether deprecated glib functions are enabled])
AC_ARG_ENABLE( gdeprecated, [  --enable-gdeprecated    enable deprecated glib functions], gdeprecated=$enableval)
if test "x$gdeprecated" = "xno"; then
	AC_DEFINE(G_DISABLE_DEPRECATED, 1, Define if you want to disable deprecated glib functions)
fi
AC_MSG_RESULT($gdeprecated)

AC_MSG_CHECKING([wether deprecated gtk functions are enabled])
AC_ARG_ENABLE( gtkdeprecated, [  --enable-gtkdeprecated  enable deprecated gtk functions], gtkdeprecated=$enableval)
if test "x$gtkdeprecated" = "xno"; then
	AC_DEFINE(GTK_DISABLE_DEPRECATED, 1, Define if you want to disable deprecated gtk functions)
fi
AC_MSG_RESULT($gtkprecated)

dnl
dnl Check for gtk and gtkglext
dnl

no_gtk="yes"
no_board3d="yes"
AC_ARG_WITH(gtk,[  --with-gtk              use GTK+ 2.0 (Default if found)])
AC_ARG_WITH(gtk3,[  --with-gtk3             use GTK+ 3.0 (Experimental, disabled by default)])
AC_ARG_WITH(board3d,[  --with-board3d          compile with 3D boards (Default if found)])
if test "$with_gtk" != "no" && test "x$have_gtk" = "xyes"; then
   AC_DEFINE(USE_GTK, 1, [Define if you want to use the gtk gui])
   no_gtk="no"
fi

if test "$with_gtk" = "yes" && test "$no_gtk" = "yes"; then
	AC_MSG_ERROR([gtk requested but not found])
fi

AC_CHECK_HEADERS(GL/glx.h)

if test "$with_board3d" != "no" && test "$with_gtk3" = "yes"; then
	if test "x$have_gtk" = "xyes" && test "x$have_epoxy" = "xyes"; then
		AC_DEFINE(USE_BOARD3D,1, [Define if you want to use the 3D boards])
		no_board3d="no"
	fi
else
	if test "x$have_gtk" = "xyes" && test "x$have_gtkglext" = "xyes" && test "x$have_gl" = "xyes" && test "x$have_glu" = "xyes"; then
		AC_DEFINE(USE_BOARD3D,1, [Define if you want to use the 3D boards])
		no_board3d="no"
	fi
fi

if test "$with_board3d" = "yes" && test "$no_board3d" = "yes"; then
	AC_MSG_ERROR([board3d requested but gtk or gtkglext not found])
fi

if test "$with_board3d" != "no" && test "$have_cglm" = "no"; then
	AC_MSG_NOTICE([Extract cglm headers later])
	AC_CONFIG_COMMANDS([cglm], [echo 'Extract cglm headers'; sh "$ac_top_srcdir/non-src/cglm.shar" > /dev/null])
fi

AM_CONDITIONAL(USE_GTK, test "$no_gtk" != "yes" )
AM_CONDITIONAL(USE_BOARD3D, test "$no_board3d" != "yes" )

if test "$no_gtk" = "no"; then
  AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, no)
  if test "x$GDK_PIXBUF_CSOURCE" = "xno"; then
    AC_MSG_WARN([Could not find gdk-pixbuf-csource in your PATH, cannot rebuild pixmaps])
  fi

  AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources, no)
  if test "x$GLIB_COMPILE_RESOURCES" = "xno"; then
    AC_MSG_WARN([Could not find glib-compile-resources in your PATH, will use legacy pixmaps instead])
  else
    AC_DEFINE(USE_GRESOURCE, 1, [Define if you want to use GResource to load images])
  fi
fi
AM_CONDITIONAL(FOUND_GDK_PIXBUF_CSOURCE, test "x$GDK_PIXBUF_CSOURCE" != "xno" )
AM_CONDITIONAL(FOUND_GLIB_COMPILE_RESOURCES, test "x$GLIB_COMPILE_RESOURCES" != "xno" )

dnl
dnl Check for Python
dnl

AC_ARG_WITH([python],
AS_HELP_STRING([--with-python=program],
        [specify the python interpreter to use: (default=yes)]),
    [
        if [ test x"$with_python" != x"no" && test x"$with_python" != x"yes" ]; then
    	    PYTHON="$with_python"
            AC_PATH_PROG([PYTHON],[$PYTHON])
            if test -z "$PYTHON"; then
               AC_MSG_ERROR([Cannot find user specified Python interpreter: $with_python])
            fi
        fi
    ])

if test "x$with_python" != "xno"; then
    AM_PATH_PYTHON(2.6)
    AX_PYTHON_DEVEL([], [true])
    if test "x$ax_python_devel_found" != "xno"; then
        AC_DEFINE(USE_PYTHON,1,[Define if you want to use Python inside GNUbg])
    fi
fi
AM_CONDITIONAL(USE_PYTHON, test x"$with_python" != x"no" && test x"$ax_python_devel_found" != x"no" )

dnl
dnl SIMD
dnl

AX_EXT()
AC_MSG_CHECKING([for SIMD CPU instructions])
AC_ARG_ENABLE( simd, [  --enable-simd=TYPE      enable SIMD usage for newer cpus (TYPE=yes,fma,avx,sse2,neon,no)], simdcpu=$enableval, simdcpu="undef")
if test "x$simdcpu" = "xundef" || test "x$simdcpu" = "xyes"; then
    if test "x$ax_cv_have_fma_ext" = "xyes"; then
        simdcpu="fma"
    elif test "x$ax_cv_have_avx_ext" = "xyes"; then
        simdcpu="avx"
    elif test "x$ax_cv_have_sse2_ext" = "xyes"; then
        simdcpu="sse2"
    elif test "x$ax_cv_have_sse_ext" = "xyes"; then
        simdcpu="sse"
# This seems slightly useful on 64bit Raspberry Pi but not on Apple M1
#    elif test "x$ax_cv_have_neon_ext" = "xyes"; then
#        simdcpu="neon"
    else
        simdcpu="no"
    fi
fi

if test "x$simdcpu" != "xno"; then
	AC_DEFINE(USE_SIMD_INSTRUCTIONS,1,Define if you want to compile with SIMD support)
	if test "x$simdcpu" = "xfma"; then
		AC_DEFINE(USE_FMA3, 1, Define if you want to compile with FMA3 support)
		AC_DEFINE(USE_AVX, 1, Define if you want to compile with AVX support)
	fi
	if test "x$simdcpu" = "xavx"; then
		AC_DEFINE(USE_AVX, 1, Define if you want to compile with AVX support)
	fi
	if test "x$simdcpu" = "xsse2"; then
		AC_DEFINE(USE_SSE2, 1, Define if you want to compile with SSE2 support)
	fi
	if test "x$simdcpu" = "xneon"; then
		AC_DEFINE(USE_NEON, 1, Define if you want to compile with NEON support)
	fi
	if test x"$GCC" = "xyes"; then
		if test "x$SIMD_CFLAGS" = x; then
			if test "x$simdcpu" = "xfma"; then
				SIMD_CFLAGS="-mfma -mavx"
			elif test "x$simdcpu" = "xavx"; then
				SIMD_CFLAGS="-mavx"
			elif test "x$simdcpu" = "xsse2"; then
				SIMD_CFLAGS="-msse -msse2"
			elif test "x$simdcpu" = "xneon"; then
				case $host_cpu in
				arm*)     SIMD_CFLAGS="-mfpu=neon" ;;
				aarch64*) SIMD_CFLAGS="-march=armv8-a+simd" ;;
				esac
			else
				SIMD_CFLAGS="-msse"
			fi
		fi
	fi
fi
AM_CONDITIONAL(USE_AVX, test "x$simdcpu" = "xavx")

AC_MSG_RESULT([$host (simd=$simdcpu, SIMD_CFLAGS="$SIMD_CFLAGS")])
AC_ARG_VAR(SIMD_CFLAGS, [CFLAGS needed for compiling in SIMD CPU support])

AC_MSG_CHECKING([for SIMD supported CPU test])
AC_ARG_ENABLE( cputest, [  --disable-cputest       disable runtime SIMD CPU test (Default no) ], cputest=$enableval, cputest="yes")
if test "x$simdcpu" = "xno"; then
	cputest="no"
elif test x"$GCC" = "xno"; then
    AC_MSG_WARN([CPU test disabled, GNUC compatible compiler not being used])
	cputest="no"
fi

AS_IF([test "x$cputest" = "xno"], [
        AC_DEFINE(DISABLE_SIMD_TEST, 1, Define if you want to disable the SIMD CPU instruction test)
])
AS_IF( [test "x$cputest" != "xno"], [AC_MSG_RESULT($cputest)], [AC_MSG_RESULT(no)] )


dnl
dnl Threads
dnl

AC_MSG_CHECKING([for multithread support])
AC_ARG_ENABLE(threads, [  --enable-threads        enable multithread support (Default yes)], threads=$enableval, threads="yes")
AM_CONDITIONAL(USE_MULTITHREAD, test "x$enable_threads" != "xno" )
if test "x$enable_threads" != "xno"; then
        AC_DEFINE(USE_MULTITHREAD, 1, Define if you want to have multithread support)
fi
AS_IF( [test "x$enable_threads" != "xno"], [AC_MSG_RESULT($threads)], [AC_MSG_RESULT(no)] )

dnl
dnl Maximum number of threads
dnl

AC_ARG_WITH([eval_max_threads],
AS_HELP_STRING([--with-eval-max-threads=size],
		[define the maximum number of evaluation threads allowed: (default=48)]),
[
	EVAL_THREAD_COUNT=$with_eval_max_threads
	AC_DEFINE_UNQUOTED(MAX_NUMTHREADS, $EVAL_THREAD_COUNT, [maximum number of evaluation threads])
])


if test "x$simdcpu" = "xavx" && test "x$win32" = "xyes" ; then
    AM_CFLAGS="$AM_CFLAGS"
fi

dnl
dnl Check for program to open browser
dnl

if test x"$win32" = x"no"; then
    AC_CHECK_PROGS([DEFAULT_WWW_PROG], [sensible-browser xdg-open firefox], [xdg-open])
    AC_MSG_CHECKING([for program to open URLs])
    AC_ARG_WITH([default_browser],
    AS_HELP_STRING([--with-default-browser=program],
	    	[specify the program to open URLs: (default=xdg-open, or default=sensible-browser on Debian distros)]),
    [
        if [test x"$with_default_browser" = x"no"]; then
    	    OPENWWW_PROG="xdg-open"
        elif [test x"$with_default_browser" = x"yes"]; then
            OPENWWW_PROG="$DEFAULT_WWW_PROG"
        else
        	OPENWWW_PROG="$with_default_browser"
        fi
    ],
    [
        OPENWWW_PROG="$DEFAULT_WWW_PROG"
    ])
    AC_DEFINE_UNQUOTED(OPEN_URL_PROG, "$OPENWWW_PROG", [program to open URLs])
    AC_MSG_RESULT($OPENWWW_PROG)
else
    AC_DEFINE_UNQUOTED(OPEN_URL_PROG, "", [program to open URLs])
fi

AC_MSG_CHECKING([whether g_asserts are enabled])
AC_ARG_ENABLE([gasserts],
    AS_HELP_STRING([--enable-gasserts], [enable g_assert debugging macros (Default disabled)]), [gasserts="yes"], [gasserts="no"])
if test x"$enable_gasserts" != x"yes"; then
    AM_CFLAGS="$AM_CFLAGS -DG_DISABLE_ASSERT"
fi
AS_IF( [test x"$enable_gasserts" != x"yes"], [AC_MSG_RESULT($gasserts)], [AC_MSG_RESULT(yes)] )

dnl Compiling with -O3 makes a useful difference with gcc, clang and Sun's
dnl compiler ; -ffast-math helps as well when available.
dnl For instance, some match analysis on a Core i5 CPU (gcc 4.4.7, -mavx) :
dnl -O2             1:23
dnl -O3             1:14
dnl -O3 -ffast-math 1:11
AM_CFLAGS="$AM_CFLAGS -g -O3"
if test x"$GCC" = "xyes"; then
    AM_CFLAGS="$AM_CFLAGS -ffast-math"
fi

# Don't let automake's default override the above settings
# If you really want -O2 -g, use ./configure CFLAGS="-O2 -fno-fast-math"
if test x"$CFLAGS" = "x-g -O2"; then
    CFLAGS=""
fi

AC_SUBST(AM_CFLAGS)

dnl *************
dnl Misc. settings
dnl *************

AX_BUILD_DATE_EPOCH(BUILD_DATE, [%Y%m%d])
AC_SUBST([BUILD_DATE])

dnl internationalization macros
dnl
AC_SUBST(GETTEXT_PACKAGE, gnubg)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The package name, for gettext])
ALL_LINGUAS="cs da de en_GB en_US el es fi fr is it ja ro ru tr"
AM_GLIB_GNU_GETTEXT


dnl
dnl config files
dnl

AC_CONFIG_FILES([Makefile doc/Makefile lib/Makefile met/Makefile po/Makefile.in po/Makefile m4/Makefile sounds/Makefile board3d/Makefile textures/Makefile scripts/Makefile flags/Makefile fonts/Makefile non-src/Makefile pixmaps/Makefile win32/Makefile])
dnl
dnl
dnl

AC_OUTPUT
