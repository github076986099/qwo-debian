AC_PREREQ(2.53)
AC_INIT(qwo, 0.2, http://projects.openmoko.org/projects/qwo/)
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

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
])

