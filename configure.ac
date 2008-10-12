AC_PREREQ(2.53)
AC_INIT(qwo, 0.3, http://projects.openmoko.org/projects/qwo/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/qwo.c)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC
AC_STDC_HEADERS

AC_CHECK_LIB(X11, XOpenDisplay)

AC_CHECK_LIB(Xext, XShapeQueryExtension)

AC_CHECK_LIB(Xtst, XTestFakeKeyEvent)

AC_CHECK_LIB(config, config_read)

dnl Check for imlib2 library

AC_PATH_PROG(IMLIB2_CONFIG, imlib2-config, no)

if test "$IMLIB2_CONFIG" = "no" ; then
    AC_MSG_ERROR([
*** Unable to find Imlib2 library.])
else
    CFLAGS="$CFLAGS `$IMLIB2_CONFIG --cflags`"
    LIBS="$LIBS `$IMLIB2_CONFIG --libs`"
fi


AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
])

