dnl - Macros for configure script

dnl Summary print function
dnl
dnl FEATURE_SUMMARY(FEATURE-NAME, FEATURE-TEST, FAILURE-NOTE)
AC_DEFUN([FEATURE_SUMMARY],
[
if test [$2]; then
  _SUMMARY_ENABLED="$_SUMMARY_ENABLED$1\n"
else
  _SUMMARY_DISABLED="$_SUMMARY_DISABLED$1\n"
  ifelse([$3], , :, _SUMMARY_DISABLED="$_SUMMARY_DISABLED\t** Note: $3\n")
fi
])

dnl Ripped from pygtk
dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(yes)
$1],dnl
[AC_MSG_RESULT(no)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

dnl Configure Paths for zlib (Josh Green 2003-11-22)
dnl
dnl AM_PATH_ZLIB([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for zlib, and define ZLIB_CFLAGS and
dnl ZLIB_LIBS as appropriate.
dnl enables arguments --with-zlib-prefix=

AC_DEFUN([AM_PATH_ZLIB],
[dnl Save the original CFLAGS, and LIBS
save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
zlib_found=yes

dnl
dnl Setup configure options
dnl
AC_ARG_WITH(zlib-prefix,
  [  --with-zlib-prefix=PATH  Path where zlib is (optional)],
  [zlib_prefix="$withval"], [zlib_prefix=""])

AC_MSG_CHECKING(for zlib)

dnl Add zlib to the LIBS path
ZLIB_LIBS="-lz"

if test "${zlib_prefix}" != "" ; then
  ZLIB_LIBS="-L${zlib_prefix}/lib $ZLIB_LIBS"
  ZLIB_CFLAGS="-I${zlib_prefix}/include"
else
  ZLIB_CFLAGS=""
fi

LIBS="$ZLIB_LIBS $LIBS"
CFLAGS="$ZLIB_CFLAGS $CFLAGS"

AC_TRY_COMPILE([
#include <stdio.h>
#include <zlib.h>
], [
#ifndef deflateInit
   return (1);
#else
   return (0);
#endif
],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   zlib_found=no]
)

CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

if test "x$zlib_found" = "xyes" ; then
   ifelse([$1], , :, [$1])
else
   ZLIB_CFLAGS=""
   ZLIB_LIBS=""
   ifelse([$2], , :, [$2])
fi

dnl That should be it.  Now just export out symbols:
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
])

AC_DEFUN([AC_DEBUGGING],
[
  AC_ARG_ENABLE(debug,
    [  --enable-debug	  enable debugging compiler options],
    enable_debug=$enableval, enable_debug="no")

  if test "$enable_debug" = "yes" ; then
	AC_DEFINE(IPATCH_DEBUG, 1, [Define to enable debugging])
  	CFLAGS="-g -O0"
  fi
])
