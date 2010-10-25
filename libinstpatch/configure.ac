dnl --------------------------------------------------
dnl configure.ac for libInstPatch
dnl --------------------------------------------------

AC_INIT(libinstpatch/misc.c)

# Making releases:
#   IPATCH_VERSION_MICRO += 1;
#   IPATCH_INTERFACE_AGE += 1;
#   IPATCH_BINARY_AGE += 1;
# if any functions have been added, set IPATCH_INTERFACE_AGE to 0.
# if backwards compatibility has been broken (removed or changed functions),
#   set IPATCH_BINARY_AGE and IPATCH_INTERFACE_AGE to 0.
#
IPATCH_VERSION_MAJOR=1
IPATCH_VERSION_MINOR=0
IPATCH_VERSION_MICRO=0
IPATCH_INTERFACE_AGE=0
IPATCH_BINARY_AGE=0
IPATCH_VERSION=$IPATCH_VERSION_MAJOR.$IPATCH_VERSION_MINOR.$IPATCH_VERSION_MICRO
AC_SUBST(IPATCH_VERSION_MAJOR)
AC_SUBST(IPATCH_VERSION_MINOR)
AC_SUBST(IPATCH_VERSION_MICRO)
AC_SUBST(IPATCH_INTERFACE_AGE)
AC_SUBST(IPATCH_BINARY_AGE)
AC_SUBST(IPATCH_VERSION)

LT_CURRENT=`expr 100 '*' $IPATCH_VERSION_MINOR + $IPATCH_VERSION_MICRO - $IPATCH_INTERFACE_AGE`
LT_REVISION=$IPATCH_INTERFACE_AGE
LT_AGE=`expr $IPATCH_BINARY_AGE - $IPATCH_INTERFACE_AGE`
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Extra m4 macro directory
AC_CONFIG_MACRO_DIR(m4)

# Check build environment
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(libinstpatch, $IPATCH_VERSION)
AM_CONFIG_HEADER(config.h)

IPATCH_VERSION="\"$VERSION\""

# Check for debugging flags (before AC_PROG_CC to override defaults)
AC_DEBUGGING

# Program checks
AC_PROG_CC

# Set this after AC_PROG_CC so that we inherit the defaults from it
CFLAGS="$CFLAGS -Wall"

# International language support
AM_GNU_GETTEXT([external])

AC_PROG_INTLTOOL
AC_PROG_INSTALL
AC_HEADER_STDC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl - Check for mingw Win32 gcc environment
AC_MINGW32
if test "${MINGW32}" == "yes" ; then
  AC_DEFINE(MINGW32, 1, [Define if using the mingw environment])
fi

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, test "$native_win32" = yes)

AC_MSG_CHECKING([for Win32 platform in general])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT($platform_win32)
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = yes)

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc. GTK+ uses this convention, so we must, too.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
if test x"$native_win32" = xyes; then
  if test x"$GCC" = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
	if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
	  msnative_struct='-fnative-struct'
	fi
	;;
      *)
	if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
	  msnative_struct='-mms-bitfields'
	fi
	;;
    esac
    if test x"$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([produced libraries will be incompatible with prebuilt GTK+ DLLs])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
fi

PKG_CHECK_MODULES(GOBJECT,
  gobject-2.0 >= 2.14 glib-2.0 >= 2.14 gthread-2.0 >= 2.14)
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

# Do libsndfile check
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)


# Check for python 2.2 (defines PYTHON_CFLAGS, PYTHON_LIBS, PYTHON_SITE_PKG,
#    PYTHON_EXTRA_LIBS)
AC_PYTHON_DEVEL

# Try for pygobject first (removes gtk dependency)
PKG_CHECK_MODULES(PYGTK, pygobject-2.0, PYGTK_SUPPORT=1, PYGTK_SUPPORT=0)

# If no pygobject check for full pygtk
if test ! "$PYGTK_SUPPORT" = "1" ; then
  PKG_CHECK_MODULES(PYGTK, pygtk-2.0, PYGTK_SUPPORT=1, PYGTK_SUPPORT=0)
  PYGTK_CODEGEN_DIR=`pkg-config pygtk-2.0 --variable=codegendir`
else
  PYGTK_CODEGEN_DIR=`pkg-config pygobject-2.0 --variable=codegendir`
fi

AC_SUBST(PYGTK_CFLAGS)
AC_SUBST(PYGTK_LIBS)

AC_ARG_ENABLE(python, AS_HELP_STRING([--disable-python], [disable python binding]),
  enable_python=$enableval, enable_python="yes")

if test "x${enable_python}" = "xyes" -a "$PYGTK_SUPPORT" = "1" ; then
  AC_DEFINE(PYTHON_SUPPORT, 1, [Define to build Python binding])

  if test "x$PYGTK_CODEGEN_DIR" = "x" ; then
      PYGTK_CODEGEN_DIR=/usr/share/pygtk/2.0/codegen
  fi

  AC_SUBST(PYGTK_CODEGEN_DIR)
else
  enable_python="no"
fi

AM_CONDITIONAL(PYTHON_SUPPORT, test "x$enable_python" = "xyes")


# Check for gtk-doc.
GTK_DOC_CHECK(1.9)


AC_OUTPUT([
	Makefile
	docs/Makefile
	docs/reference/Makefile
	examples/Makefile
	libinstpatch/libinstpatch.h
	libinstpatch/Makefile
	libinstpatch/version.h
	libinstpatch-1.0.pc
	m4/Makefile
	po/Makefile.in
	python/Makefile
	tests/Makefile
	utils/Makefile])

_SUMMARY_ENABLED=
_SUMMARY_DISABLED=

FEATURE_SUMMARY([  Python support], ["x${enable_python}" = "xyes"])

FEATURE_SUMMARY([  Generate gtk-doc API docs],
  ["x${enable_gtk_doc}" == "xyes"])

FEATURE_SUMMARY([  Debugging],
  ["${enable_debug}" == "yes"])


echo
echo "**************************************************************"
echo "Enabled features:"
echo -e -n "$_SUMMARY_ENABLED"
echo
echo "Disabled features:"
echo -e -n "$_SUMMARY_DISABLED"
echo
echo "**************************************************************"
echo
