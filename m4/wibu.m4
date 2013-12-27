dnl AG_WIBU([needs_wupi], [needs_codemeter])
dnl check for AxProtector and CodeMeter headers and libraries
dnl sets WITH_WIBU, WUPI_LIBS, WUPI_FLAGS, CODEMETER_LIBS, CODEMETER_FLAGS
dnl defines BUILD_WIBU

AC_DEFUN([AG_WIBU],
[
# Requires AG_GST_ARCH and AG_PLATFORM
# WIBU CodeMeter (note we need arch to figure out library name...)

needs_wupi=$1
needs_codemeter=$2
build_wibu=no

AC_ARG_WITH(wibu,
   AS_HELP_STRING([--with-wibu],
       [use WIBU protection system]),
       [with_wibu_val="${withval}"], [with_wibu_val="no"])

if [test "x$with_wibu_val" == "xyes"]; then
  case "x${ostype}" in
    xDarwin)
      _WUPI_LIBS="-lwupienginemac -L/Developer/WIBU-SYSTEMS/AxProtector/lib"
      _WUPI_CFLAGS='-I/Developer/WIBU-SYSTEMS/AxProtector/include'
      _CODEMETER_LIBS="-framework WibuCmMacX "
      _CODEMETER_CFLAGS='-I/Developer/WIBU-SYSTEMS/CodeMeter/include'
      ;;
    xLinux)
      if test "x$HAVE_CPU_X86_64" = "xyes"; then
        _WUPI_LIBS="-lwupienginelin64"
        _CODEMETER_LIBS="-lwibucmlin64"
      else
        _WUPI_LIBS="-lwupienginelin"
        _CODEMETER_LIBS="-lwibucmlin"
      fi
      ;;
    xWindows)
      if test "x$HAVE_CPU_X86_64" = "xyes"; then
        _WUPI_LIBS="-lWupiEngine64"
        _CODEMETER_LIBS="-lWibuCm64"
      else
        _WUPI_LIBS="-lWupiEngine32"
        _CODEMETER_LIBS="-lWibuCm32"
      fi
      ;;
  esac

  if [ test "x$needs_wupi" = "xyes" ]; then
    save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $_WUPI_CFLAGS"
    save_LIBS="$LIBS"
    LIBS="$LIBS $_WUPI_LIBS"

    dnl Check for wibuixap.h headers
    AC_CHECK_HEADERS(wibuixap.h, [have_wupi_h="yes"], [have_wupi_h="no"])
    dnl Check for WupiGetLastError()
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #include <wibuixap.h>],[
      WupiGetLastError();])], [have_wupi="yes"],[have_wupi="no"])

    if [ test "x$have_wupi" = "xno" ]; then
      AC_MSG_ERROR([you need to have AxProtector installed ])
    fi
    LIBS=$save_LIBS
    CPPFLAGS=$save_CPPFLAGS
  fi

  if [ test "x$needs_codemeter" = "xyes" ]; then
    save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $_CODEMETER_CFLAGS"
    save_LIBS="$LIBS"
    LIBS="$LIBS $_CODEMETER_LIBS"
    dnl Check for CodeMeter.h headers
    AC_CHECK_HEADERS(CodeMeter.h, [have_codemeter_h="yes"], [have_codemeter_h="no"])

    dnl Check for WupiGetLastError()
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #include <CodeMeter.h>],[
      CmGetLastErrorCode();])], [have_codemeter="yes"],[have_codemeter="no"])

    if [ test "x$have_codemeter" = "xno" ]; then
      AC_MSG_ERROR([you need to have CodeMeter installed ])
    fi
    LIBS=$save_LIBS
    CPPFLAGS=$save_CPPFLAGS
  fi

  build_wibu="yes"
  WUPI_LIBS=$_WUPI_LIBS
  WUPI_CFLAGS=$_WUPI_CFLAGS
  CODEMETER_LIBS=$_CODEMETER_LIBS
  CODEMETER_CFLAGS=$_CODEMETER_CFLAGS
fi

if [test "x$build_wibu" == "xyes" ]; then
  AC_DEFINE(BUILD_WIBU, 1, [Build with WIBU CodeMeter support])
fi
AC_SUBST(WUPI_CFLAGS)
AC_SUBST(WUPI_LIBS)
AC_SUBST(CODEMETER_CFLAGS)
AC_SUBST(CODEMETER_LIBS)
AM_CONDITIONAL(WITH_WIBU, [test "x$build_wibu" = "xyes"])
])

