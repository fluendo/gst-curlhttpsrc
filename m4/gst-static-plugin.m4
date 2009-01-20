dnl Disable pic build and 

AC_DEFUN([AG_GST_STATIC_PLUGIN],
[
  STATIC_PLUGIN_CFLAGS=""
  AC_ARG_ENABLE(static-plugin,
    AC_HELP_STRING([--enable-static-plugin],
      [Build a non relocatable object archive for static linking]),
    [STATICPLUGIN=yes],
    [STATICPLUGIN=no])

  if test "x$STATICPLUGIN" = xyes; then
    AC_ENABLE_STATIC(yes) dnl --enable-static
    AC_LIBTOOL_PICMODE(default) dnl --without-pic
    AS_COMPILER_FLAG(-DENABLE_STATIC_PLUGIN,
      STATIC_PLUGIN_CFLAGS="-DENABLE_STATIC_PLUGIN")
    AC_SUBST(CFLAGS,"-DENABLE_STATIC_PLUGIN")
  fi
])

