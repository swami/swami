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

AC_DEFUN([AC_DEBUGGING],
[
  AC_ARG_ENABLE(debug,
    [  --disable-debug	  disable debugging compiler options],
    enable_debug=$enableval, enable_debug="yes")

  if test "$enable_debug" = "yes" ; then
  	AC_MSG_RESULT([enable debugging compiler flags ...])
  	CFLAGS="-g ${CFLAGS}"
  else
	AC_MSG_RESULT([disable debugging compiler flags ...])
	CFLAGS="-O2 ${CFLAGS}"
  fi

])

AC_DEFUN([AC_PROFILING],
[
  AC_ARG_ENABLE(profile,
    [  --enable-profile   enable profiling compiler options],
    enable_profile=$enableval, enable_profile="no")

  if test "${enable_profile}" = "yes" ; then
    AC_MSG_RESULT([enable profile compiler flags ...])
    if test "$GCC" = "yes" ; then
      CFLAGS="-pg ${CFLAGS}"
    else
      AC_MSG_RESULT(we do not have gcc, hope we have a nice profiler...)
      CFLAGS="-p ${CFLAGS}"
    fi
  fi
])
