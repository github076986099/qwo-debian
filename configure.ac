AC_PREREQ(2.53)
AC_INIT(qwo, 0.10, http://projects.openmoko.org/projects/qwo/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/qwo.c)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC
AC_STDC_HEADERS

PKG_CHECK_MODULES(DEPENDENCIES, gtk+-2.0)

AC_CHECK_LIB(config, config_read)

AC_ARG_ENABLE(standalone,
	AC_HELP_STRING([--enable-standalone], [run with a text view window]),
	enable_standalone=$enableval, enable_standalone=no)

if test x$enable_standalone != xno ; then
	AC_DEFINE(TEXT_VIEW, [1], [run with a text view window])
fi

AC_SUBST(DEPENDENCIES)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
])

