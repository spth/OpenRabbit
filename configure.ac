AC_INIT([openrabbit], [0.1.2])

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

#AC_ARG_ENABLE(coldboot, AS_HELP_STRING([--disable-coldboot], [Do not build coldboot loader firmware]))
#AM_CONDITIONAL([OPT_DISABLE_COLDBOOT], [test x$debug = xtrue])

AC_ARG_ENABLE(coldboot,
  AS_HELP_STRING([--disable-coldboot], [Do not build coldboot loader firmware]),
  [], [enable_coldboot=yes])
AM_CONDITIONAL(ENABLE_COLDBOOT, test x"$enable_coldboot" = xyes)

AC_LANG_C
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_CHECK_PROG(SDCC,sdcc,sdcc, :)
AC_CHECK_PROG(ASRAB,sdasrab,sdasrab, :)

AC_DEFUN([REQUIRE_PROG],
[if test "$1" = ":"; then
  AC_MSG_ERROR([Cannot find required program $2.])
 fi
])

if test x"$enable_coldboot" = xyes; then
  REQUIRE_PROG($SDCC, sdcc)
  REQUIRE_PROG($ASRAB, sdasrab)
fi

AC_CONFIG_FILES([Makefile src/Makefile coldboot/Makefile])

AC_OUTPUT

