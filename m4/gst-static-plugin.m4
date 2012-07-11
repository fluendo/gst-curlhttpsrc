dnl Disable pic build and 

AC_DEFUN([AG_GST_ARG_STATIC_PLUGIN],
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
    AC_SUBST(CFLAGS,"${CFLAGS} -DENABLE_STATIC_PLUGIN")
  fi
])

AC_DEFUN([AG_GST_ARG_STATIC_PLUGINS],
[
  dnl build static plugins or not
  AC_MSG_CHECKING([whether to build static plugins or not])
  AC_ARG_ENABLE(
    static-plugins,
    AC_HELP_STRING(
      [--enable-static-plugins],
      [build static plugins @<:@default=no@:>@]),
    [AS_CASE(
      [$enableval], [no], [], [yes], [],
      [AC_MSG_ERROR([bad value "$enableval" for --enable-static-plugins])])],
    [enable_static_plugins=no])
  AC_MSG_RESULT([$enable_static_plugins])
  if test "x$enable_static_plugins" = xyes; then
    AC_ENABLE_STATIC(yes) dnl --enable-static
    AC_ENABLE_SHARED(no) dnl --disable-shared
    AC_LIBTOOL_PICMODE(default) dnl --without-pic
    AC_DEFINE(GST_PLUGIN_BUILD_STATIC, 1,
      [Define if static plugins should be built])
  fi
  AM_CONDITIONAL(GST_PLUGIN_BUILD_STATIC, test "x$enable_static_plugins" = "xyes")
])
