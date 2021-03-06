m4_define([MAJOR], 0)
m4_define([MINOR], 9)
m4_define([MICRO], 8)

dnl Process this file with autoconf to produce a configure script.
AC_INIT([xbmc-addon-xvdr], [MAJOR.MINOR.MICRO], [alexander.pipelka@gmail.com])

AC_CONFIG_AUX_DIR(autotools)
AC_PREFIX_DEFAULT(/usr/lib/xbmc)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

dnl Setup for automake
AM_INIT_AUTOMAKE([foreign])

dnl Check for tools

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_EXEEXT

dnl Check for zlib
zlib_found=yes
ZLIB_LIBS=
AC_CHECK_HEADER(zlib.h,,[zlib_found="no"])
if test x$zlib_found = xyes; then
        AC_SEARCH_LIBS(compress, z, [AC_DEFINE([HAVE_ZLIB], 1, [haev zlib compression library installed])])
        ZLIB_LIBS="-lz"
        AC_SUBST(ZLIB_LIBS)
fi

dnl Check for libpthread
PTHREAD_LIBS=
AC_SEARCH_LIBS(pthread_create, pthread, [if test "$ac_res" != "none required"; then PTHREAD_LIBS="-lpthread"; fi])
AC_SUBST(PTHREAD_LIBS)

AC_SUBST(VERSIONEXT)

ISMINGW32=false
ADDONNAME=XBMC_VDR_xvdr.pvr
SOEXT=so
ISAPPLE=false

case $host in
  *-*-cygwin* | *-*-mingw32*)
    LDFLAGS+="-no-undefined"
    ISMINGW32=true
    ADDONNAME=XBMC_VDR_xvdr_WIN32.pvr
    SOEXT=dll
    AC_DEFINE(TARGET_WINDOWS, 1)
    ;;
  arm*-*-linux-gnu*)
    AC_SUBST(ARCH_DEFINES, "-D_ARMEL")
    ;;
  powerpc-*-linux-gnu*)
    AC_SUBST(ARCH_DEFINES, "-D_POWERPC")
    ;;
  powerpc64-*-linux-gnu*)
    AC_SUBST(ARCH_DEFINES, "-D_POWERPC64")
    ;;
  *-apple-*)
    ADDONNAME=XBMC_VDR_xvdr_OSX.pvr
    SOEXT=dylib
    ISAPPLE=true
    ;;
  *-androideabi)
    AC_MSG_ERROR([for Android please use the "xvdr-addon-android" repository !

You may clone the repository with:
git clone --recursive git://github.com/pipelka/xvdr-addon-android.git
]);
    ;;
esac

AM_CONDITIONAL([MINGW32], $ISMINGW32)
AM_CONDITIONAL([APPLE], $ISAPPLE)
AC_SUBST(ADDONNAME)
AC_SUBST(SOEXT)

# Finally create all the generated files
AC_OUTPUT([
Makefile
addons/Makefile
addons/pvr.vdr.xvdr/Makefile
addons/pvr.vdr.xvdr/addon.xml
data/addons.xml
data/Makefile
src/Makefile
src/libxvdr/Makefile
src/libxvdr/include/Makefile
src/libxvdr/src/Makefile
src/libxvdr/tests/Makefile
src/xvdr/dialogs/Makefile
src/xvdr/Makefile
])
