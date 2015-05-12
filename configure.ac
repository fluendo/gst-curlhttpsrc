dnl required version of autoconf
AC_PREREQ([2.53])

AC_INIT([gst-curlhttpsrc],[1.0.0])

AG_GST_INIT

AC_CONFIG_SRCDIR([src/gstcurlhttpsrc.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl required version of automake
AM_INIT_AUTOMAKE([1.10])

dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl make aclocal work in maintainer mode
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)

dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR([you need to have pkgconfig installed !])
fi

AG_GST_DETECT_VERSION([1.0.0], [0.10.3])
AG_GST_CHECK_GST($GST_MAJORMINOR, [$GST_REQ])
AG_GST_CHECK_GST_BASE($GST_MAJORMINOR, [$GST_REQ])
AG_GST_CHECK_GST_CONTROLLER($GST_MAJORMINOR, [$GST_REQ])

dnl Check for libcurl-dev
dnl 
dnl As a lot of the SSL/HTTP2 stuff is determined as available at run time,
dnl I'm not going to bother with it here. Just need to test for curl existing,
dnl everything else is going to get sorted out by the curl library itself.
dnl
dnl Ideally, we'd love to be able to compile-time check for HTTP/2 support,
dnl but libcurl's m4 file doesn't yet have that as a template.
LIBCURL_CHECK_CONFIG([], [], [],
  [AC_MSG_ERROR([
    You need to install a developer library of libcurl on your system.
    This is usually a case of installing your libcurl-dev package.
    Alternatively, you may compile curl from scratch and link against
    the generated libraries instead.
  ])
])

dnl check if compiler understands -Wall (if yes, add -Wall to GST_CFLAGS)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([ ], [ ])], [
  GST_CFLAGS="$GST_CFLAGS -Wall"
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])

dnl make GST_MAJORMINOR available in Makefile.am
AC_SUBST(GST_MAJORMINOR)

dnl set the plugindir where plugins should be installed (for src/Makefile.am)
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-1.0/plugins"
else
  plugindir="\$(libdir)/gstreamer-1.0"
fi
AC_SUBST(plugindir)

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

dnl Option to enable a specific protocol
AC_ARG_WITH([protocols],
            [AS_HELP_STRING([--with-protocols],
              [Set specific protocols  @<:@default="https","http"@:>@])],
            [],
            [with_protocols="\"https\", \"http\""])

AC_DEFINE_UNQUOTED([PROTOCOLS], [$with_protocols], [Protocols to register with curl])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

echo "
$PACKAGE-$VERSION

        prefix:                           ${prefix}
        compiler:                         ${CC}
        Building for GStreamer-${GST_MAJORMINOR}
	Protocols to use                  ${with_protocols}
"

