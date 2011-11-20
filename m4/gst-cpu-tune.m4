dnl This macros is intended to define platform specific tuned CFLAGS/CCASFLAGS
AC_DEFUN([AG_GST_CPU_TUNE],
[
  CPU_TUNE_CFLAGS=""
  CPU_TUNE_CCASFLAGS=""
  CPU_TUNE_LDFLAGS=""

  dnl tune build for Atom
  AC_ARG_ENABLE(cpu-tune-atom,
    AC_HELP_STRING([--enable-cpu-tune-atom], 
      [enable CFLAGS/CCASFLAGS tuned for Intel Atom]),
    [TUNE=yes],
    [TUNE=no]) dnl Default value
    
  if test "x$TUNE" = xyes; then
    AC_MSG_NOTICE(Build will be tuned for Intel Atom)
    AS_COMPILER_FLAG(-O3, 
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -O3")
    AS_COMPILER_FLAG(-march=atom, [march_atom=yes], [march_atom=no])
    if test "x$march_atom" = xyes; then
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -march=atom"
    else    
      AS_COMPILER_FLAG(-msse3, 
        CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -msse3")
      AS_COMPILER_FLAG(-march=nocona, 
        CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -march=nocona")
    fi
    AS_COMPILER_FLAG(-mfpmath=sse, 
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -mfpmath=sse")
  fi
  
  dnl tune build for Nokia N800   
  AC_ARG_ENABLE(cpu-tune-n800,
    AC_HELP_STRING([--enable-cpu-tune-n800], 
      [enable CFLAGS/CCASFLAGS tuned for Nokia N800]),
    [TUNE=yes],
    [TUNE=no]) dnl Default value
     
  if test "x$TUNE" = xyes; then
    NEW_FLAGS=""
    AC_MSG_NOTICE(Build will be tuned for Nokia N800)
    AS_COMPILER_FLAG(-march=armv6j, 
      NEW_FLAGS="$NEW_FLAGS -march=armv6j")
    AS_COMPILER_FLAG(-mtune=arm1136j-s, 
      NEW_FLAGS="$NEW_FLAGS -mtune=arm1136j-s")
    dnl Some assembly code requires -fomit-frame-pointer
    AS_COMPILER_FLAG(-fomit-frame-pointer, 
      NEW_FLAGS="$NEW_FLAGS -fomit-frame-pointer")
    CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS $NEW_FLAGS"
    CPU_TUNE_CCASFLAGS="$CPU_TUNE_CCASFLAGS $NEW_FLAGS"
  fi
  
  dnl tune build using softfp
  AC_ARG_ENABLE(cpu-tune-softfp,
    AC_HELP_STRING([--enable-cpu-tune-softfp], 
      [enable build with softfp and vfp]),
    [TUNE=yes],
    [TUNE=no]) dnl Default value
     
  if test "x$TUNE" = xyes; then
    NEW_FLAGS=""
    AS_COMPILER_FLAG(-mfloat-abi=softfp, 
      NEW_FLAGS="$NEW_FLAGS -mfloat-abi=softfp")
    AS_COMPILER_FLAG(-mfpu=vfp, 
      NEW_FLAGS="$NEW_FLAGS -mfpu=vfp")
    CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS $NEW_FLAGS"
    CPU_TUNE_CCASFLAGS="$CPU_TUNE_CCASFLAGS $NEW_FLAGS"
    AC_DEFINE(USE_ARM_VFP, TRUE, [Build with ARM vfp optimizations])
  fi
  AM_CONDITIONAL(USE_ARM_VFP, test "x$TUNE" = "xyes")

  dnl tune build using arm/thumb
  AC_ARG_ENABLE(cpu-tune-thumb,
    AC_HELP_STRING([--enable-cpu-tune-thumb], 
      [enable generation of thumb code for arm devices]),
    [TUNE=yes],
    [TUNE=no]) dnl Default value
     
  if test "x$TUNE" = xyes; then
    AS_COMPILER_FLAG(-mthumb, 
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -mthumb")
  fi

  dnl tune build on Solaris with Sun Forte CC
  AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
  if test "x$SUNCC" == "xyes"; then
    AS_COMPILER_FLAG([-xO5],
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -xO5")
    AS_COMPILER_FLAG([-xspace],
      CPU_TUNE_CFLAGS="$CPU_TUNE_CFLAGS -xspace")
  fi

  dnl No execstack depends on the platform
  case "$host" in
    *darwin*)
      ;;
    *-sun-* | *pc-solaris* )
      AC_CHECK_FILE([/usr/lib/ld/map.noexstk],
        [CPU_TUNE_LDFLAGS="${CPU_TUNE_LDFLAGS} -Wl,-M/usr/lib/ld/map.noexstk -static-libgcc"])
      ;;
    *)
      AS_COMPILER_FLAG([-Wl,-znoexecstack], 
        [CPU_TUNE_LDFLAGS="$CPU_TUNE_LDFLAGS -Wl,-znoexecstack"])
      ;;
  esac
      
  AC_SUBST(CPU_TUNE_CFLAGS)
  AC_SUBST(CPU_TUNE_CCASFLAGS)
  AC_SUBST(CPU_TUNE_LDFLAGS)

  if test "x$CPU_TUNE_CFLAGS" != "x"; then  
    AC_MSG_NOTICE(CPU_TUNE_CFLAGS   : $CPU_TUNE_CFLAGS)
    AC_MSG_NOTICE(CPU_TUNE_CCASFLAGS: $CPU_TUNE_CCASFLAGS)
    AC_MSG_NOTICE(CPU_TUNE_LDFLAGS  : $CPU_TUNE_LDFLAGS)
  fi  
])

