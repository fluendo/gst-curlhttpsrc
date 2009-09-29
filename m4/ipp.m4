dnl Detection of Intel Integrated Performance Primitives IPP
AC_DEFUN([AG_CHECK_IPP],
[
  BUILD_IN_MACOS=false
  case "$host_os" in
    *darwin*)
      BUILD_IN_MACOS=true
      ;;
  esac
  AM_CONDITIONAL(BUILD_IN_MACOS, test "x$BUILD_IN_MACOS" = "xtrue")
  
  dnl Setup for finding IPP libraries. Attempt to detect by default.
  test_ipp=true
  AC_MSG_CHECKING([for Intel Performance Primitives library])

  AC_ARG_WITH(ipp,
      AC_HELP_STRING([--with-ipp],
          [Turn on/off use of Intel Programming Primitives (default=yes)]),
          [if test "x$withval" = "xno"; then test_ipp=false; fi])

  AC_ARG_WITH(ipp-path,
     AC_HELP_STRING([--with-ipp-path],
         [manually set location of IPP files, point to the directory just beneath the directory using the IPP version number]),
         [export IPP_PATH="${withval}"])

  AC_ARG_WITH(ipp-arch,
    AC_HELP_STRING([--with-ipp-arch],
         [to include only one ipp implementation/architecture, valid values are: 0=all,1=px,2=a6,3=w7,4=t7,5=v8,6=p8,7=mx,8=m7,9=u8,10=y8,11=s8(lp32)]),
    [IPP_ARCH="${withval}"],
    [IPP_ARCH="0"])

  AC_DEFINE_UNQUOTED(USE_SINGLE_IPP_ARCH, $IPP_ARCH, [Specify one IPP implementation])  

  if test -n "$IPP_PATH"; then
    IPP_PREFIX="$IPP_PATH"
  else
    if test "x$BUILD_IN_MACOS" = "xtrue"; then
      IPP_PREFIX="/Library/Frameworks/Intel_IPP.framework/Versions"
    else
      IPP_PREFIX="/opt/intel/ipp"
    fi
  fi

  dnl List available ipp versions
  if test -n "$IPP_VERSION"; then
    IPP_AVAIL="$IPP_VERSION"
  else
    dnl Assumes that the latest directory created is the one with the correct
    dnl version to use.
    IPP_AVAIL="`ls -vrd $IPP_PREFIX/* | sed 's|.*/||' 2>/dev/null`"
  fi
 
  if test "x$test_ipp" = "xtrue"; then
    if test "x$BUILD_IN_MACOS" = "xtrue"; then
      HAVE_IPP=false
      # Loop over IPP versions 
      for ver in $IPP_AVAIL; do
        if test -f "$IPP_PREFIX/$ver/Include/ipp.h"; then
          HAVE_IPP=true
          AC_DEFINE(USE_IPP, TRUE, [Define whether IPP is available])
          break
        fi
      done
      IPP_SUFFIX=""
    else
      if test "x$IPP_ARCH" = "x11" ; then
        IPP_CPU="lp32"
        IPP_SUFFIX=""
      else
        if test "x$host_cpu" = "xamd64" -o "x$host_cpu" = "xx86_64" ; then
          IPP_CPU="em64t"
          IPP_SUFFIX="em64t"
        else
          IPP_CPU="ia32"
          IPP_SUFFIX=""
        fi
      fi
      HAVE_IPP=false
      # Loop over IPP versions 
      for ver in $IPP_AVAIL; do
        if test -f "$IPP_PREFIX/$ver/$IPP_CPU/include/ipp.h"; then
          HAVE_IPP=true
          AC_DEFINE(USE_IPP, TRUE, [Define whether IPP is available])
          break
        fi
      done
    fi
  else
    HAVE_IPP=false
  fi
  AM_CONDITIONAL(USE_IPP, test "x$HAVE_IPP" = "xtrue")

  if test "x$HAVE_IPP" = "xtrue"; then
    if test "x$BUILD_IN_MACOS" = "xtrue"; then
      IPP_PATH="${IPP_PREFIX}/${ver}"
      IPP_INCLUDES="-I${IPP_PATH}/Include"
    else
      IPP_PATH="${IPP_PREFIX}/${ver}/${IPP_CPU}"
      IPP_INCLUDES="-I${IPP_PATH}/include"
      AC_DEFINE(USE_IPP_MERGED, TRUE, [Define whether USE_IPP_MERGED could be used])
    fi
    AC_MSG_RESULT([yes (using version $ver)])
  else
    if test "x$test_ipp" = "xtrue"; then
      AC_MSG_RESULT([no]) 
    else
      AC_MSG_RESULT([disabled]) 
    fi
  fi


])

