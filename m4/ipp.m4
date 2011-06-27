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
      IPP_PREFIX="/Developer/opt/intel/ipp/"
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
        if test -f "$IPP_PREFIX/include/ipp.h"; then
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
      IPP_PATH="${IPP_PREFIX}"
      IPP_INCLUDES="-I${IPP_PATH}/include"
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

  dnl ippmerged needs to know wheter to link or not when building a static or dynamic library
  AM_CONDITIONAL(BUILD_STATIC, test "x$enable_static" = "xyes" -a "x$enable_shared" = "xno")

])

dnl Given a list of ipp libraries to link with, set the needed variables for building
dnl Usage: AG_NEED_IPP([IPP LIBS], [FUNCTION LIST PATH])
AC_DEFUN([AG_NEED_IPP],
[
  HAVE_IPP=false
  AG_CHECK_IPP
  if test "$HAVE_IPP" = "false"; then
    AC_MSG_WARN([Intel Performance Primitives not found in $IPP_PREFIX])
  else
    NEED_LIST=$1
    IPP_LIST="ippcore "
    if test "x$BUILD_IN_MACOS" = "xtrue"; then
      IPP_LIST=${NEED_LIST}
      IPP_SUFFIX="_l"
    else
      for lib in ${NEED_LIST}; do
        IPP_LIST+="${lib}merged "
        IPP_TRAMPOLINE_LIST+="${lib}emerged "
      done
    fi
    IPP_LIBS=""
    IPP_ARCHIVES=""

    for lib in ${IPP_LIST}; do
      IPP_LIBS+=" -l${lib}${IPP_SUFFIX}"
      IPP_ARCHIVES+=" lib${lib}${IPP_SUFFIX}.a"
    done
    for lib in ${IPP_TRAMPOLINE_LIST}; do
      IPP_TRAMPOLINE_LIBS+=" -l${lib}${IPP_SUFFIX}"
    done
  fi
  AC_SUBST(IPP_PATH)      dnl source directory
  AC_SUBST(IPP_INCLUDES)    dnl cflags
  AC_SUBST(IPP_LIBS)      dnl ldflags
  AC_SUBST(IPP_TRAMPOLINE_LIBS)      dnl ldflags
  AC_SUBST(IPP_ARCHIVES)  dnl to iterate
  if test -z $2; then
    IPP_FUNC_PATH="src"
  else
    IPP_FUNC_PATH=$2
  fi
  AC_SUBST(IPP_FUNC_PATH)
  AM_CONDITIONAL(USE_IPP, test "x$HAVE_IPP" = "xtrue")

  dnl Permit patching IPP library to avoid reallocation problems
  ipp_patch_realloc=false
  AC_ARG_WITH(ipp-patch-realloc,
    AC_HELP_STRING([--with-ipp-patch-realloc],
        [Turn on/off patching of IPP for reallocatable code (default=no)]),
        [if test "x$withval" = "xyes"; then ipp_patch_realloc=true; fi])
  dnl Disable patching on 64 bits
  if test "x$HAVE_CPU_X86_64" = "xyes" ; then
    ipp_patch_realloc=false
  fi  
  AM_CONDITIONAL(IPP_PATCH_REALLOC, ${ipp_patch_realloc})
])

