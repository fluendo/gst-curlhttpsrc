dnl Determine the Operating System
dnl sets and defines HAVE_OS_WINDOWS, HAVE_OS_LINUX, HAVE_OS_DARWIN, HAVE_OS_ANDROID

AC_DEFUN([AG_PLATFORM],
[
AC_MSG_CHECKING([for the OS type])
ostype=""
case "x${host_os}" in
  *darwin*)
    AC_DEFINE_UNQUOTED(HAVE_OS_DARWIN, 1, [Indicate we are building for OSX])
    ostype=Darwin
    ;;
  *linux*)
    AC_DEFINE_UNQUOTED(HAVE_OS_LINUX, 1, [Indicate we are building for Linux])
    ostype=Linux
    ;;
  *cygwin*|*mingw*|*msvc*|*mks*)
    AC_DEFINE_UNQUOTED(HAVE_OS_WINDOWS, 1, [Indicate we are building for Windows])
    ostype=Windows
    ;;
  *android*)
    AC_DEFINE_UNQUOTED(HAVE_OS_ANDROID, 1, [Indicate we are building for Android])
    ostype=Android
    ;;
  *solaris*)
    AC_DEFINE_UNQUOTED(HAVE_OS_SOLARIS, 1, [Indicate we are building for Solaris])
    ostype=Solaris
    ;;
esac
AM_CONDITIONAL(HAVE_OS_WINDOWS, test x$ostype = xWindows)
AM_CONDITIONAL(HAVE_OS_DARWIN, test x$ostype = xDarwin)
AM_CONDITIONAL(HAVE_OS_LINUX, test x$ostype = xLinux)
AM_CONDITIONAL(HAVE_OS_ANDROID, test x$ostype = xAndroid)
AM_CONDITIONAL(HAVE_OS_SOLARIS, test x$ostype = xSolaris)
AC_MSG_RESULT($ostype)
])
