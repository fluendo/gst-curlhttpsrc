dnl
dnl Check compiler and linker flags
dnl
dnl Sergi Alvarez <salvarez@fluendo.com>
dnl
dnl Last modification: 2008-10-15
dnl

AC_DEFUN([AG_TUNE_CFLAGS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AS_COMPILER_FLAG])

  TUNE_CFLAGS=""
  TUNE_LDFLAGS=""

  dnl Check for "no gnu stack" flag for the linker
  dnl --------------------------------------------
  dnl If -xspace flag is can be used with compiler we are using
  dnl SunForte which should not support the -znoexecstack of GNU
  dnl setup $ALIASING_CFLAGS here

  AS_COMPILER_FLAG(-xspace, TUNE_CFLAGS=-xspace,
	TUNE_CFLAGS=-fno-strict-aliasing
	TUNE_LDFLAGS=-znoexecstack)

  AC_SUBST(TUNE_CFLAGS)
  AC_SUBST(TUNE_LDFLAGS)

  AC_MSG_NOTICE([set TUNE_CFLAGS to $TUNE_CFLAGS])
  AC_MSG_NOTICE([set TUNE_LDFLAGS to $TUNE_LDFLAGS])
])
