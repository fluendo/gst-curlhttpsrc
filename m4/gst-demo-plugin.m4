dnl Options to build a demo codec

AC_DEFUN([AG_GST_ARG_DEMO_PLUGIN],
[
  AC_ARG_ENABLE(demo-plugin,
    AC_HELP_STRING([--enable-demo-plugin],
      [Build a demo plugin, the decoding or encoding will be limited]),
    [DEMO_PLUGIN=yes],
    [DEMO_PLUGIN=no])

  AC_ARG_ENABLE(demo-percent,
    AC_HELP_STRING([--enable-demo-percent[=number] Specify the percentage
    of the video that can be played]),
      [ if test $enableval -lt 0 -o $enableval -gt 100 ; then
          DEMO_PERCENT=10
        else
          DEMO_PERCENT=$enableval
        fi
      ],
      [DEMO_PERCENT=10])

  if test "x$DEMO_PLUGIN" = xyes; then
    AC_DEFINE(ENABLE_DEMO_PLUGIN, 1, [Demo plugin support])
    AC_DEFINE_UNQUOTED(DEMO_PERCENT, ${DEMO_PERCENT}, [Demo plugin percentage])
  fi
])

