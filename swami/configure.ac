dnl --------------------------------------------------
dnl configure.in for Swami
dnl --------------------------------------------------

AC_INIT(src/swamigui/main.c)

SWAMI_VERSION_MAJOR=2
SWAMI_VERSION_MINOR=0
SWAMI_VERSION_MICRO=0
SWAMI_VERSION=$SWAMI_VERSION_MAJOR.$SWAMI_VERSION_MINOR.$SWAMI_VERSION_MICRO

AC_SUBST(SWAMI_VERSION_MAJOR)
AC_SUBST(SWAMI_VERSION_MINOR)
AC_SUBST(SWAMI_VERSION_MICRO)
AC_SUBST(SWAMI_VERSION)

dnl - Check build environment
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(swami, $SWAMI_VERSION)
AM_CONFIG_HEADER(config.h)

SWAMI_VERSION="\"$VERSION\""
CFLAGS="$CFLAGS -Wall"

dnl - Program checks
AC_PROG_CC

dnl International language support
ALL_LINGUAS=""
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


dnl - Check if we are building for Apple Darwin
unset APPLECF_CFLAGS
case "$target" in
  *-*-darwin*)
    dnl Try to make sure we are using the framework from the right SDK
    if echo "$CFLAGS" | grep -q "\-isysroot"; then
        APPLECF_CFLAGS=$(echo "$CFLAGS" | sed -e 's/.*-isysroot[[ ]]*\([[^ ]]*\).*/\1/')
    fi
    APPLECF_CFLAGS=-I$APPLECF_CFLAGS/System/Library/Frameworks/CoreFoundation.framework/Headers
    ;;
esac
AC_SUBST(APPLECF_CFLAGS)


dnl - Check for debugging and profiling flags
AC_DEBUGGING
AC_PROFILING

PKG_CHECK_MODULES(LIBINSTPATCH, libinstpatch-1.0 >= 1.0)
AC_SUBST(LIBINSTPATCH_CFLAGS)
AC_SUBST(LIBINSTPATCH_LIBS)

PKG_CHECK_MODULES(GUI, gtk+-2.0 >= 2.12 librsvg-2.0 >= 2.8 libgnomecanvas-2.0 >= 2.0)
AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)

PKG_CHECK_MODULES(GOBJECT,
  gobject-2.0 >= 2.12 glib-2.0 >= 2.12 gmodule-2.0 >= 2.12 gthread-2.0 >= 2.12)
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

PKG_CHECK_MODULES(LIBGLADE, libglade-2.0)
AC_SUBST(LIBGLADE_CFLAGS)
AC_SUBST(LIBGLADE_LIBS)

PKG_CHECK_MODULES(MACINTEGRATION, gtk-mac-integration >= 2.0,
    MAC_INTEGRATION=1, MAC_INTEGRATION=0)
AC_SUBST(MACINTEGRATION_CFLAGS)
AC_SUBST(MACINTEGRATION_LIBS)

dnl - Check for FluidSynth
PKG_CHECK_MODULES(FLUIDSYNTH, fluidsynth >= 1.0,
    FLUIDSYNTH_SUPPORT=1, FLUIDSYNTH_SUPPORT=0)
AC_SUBST(FLUIDSYNTH_CFLAGS)
AC_SUBST(FLUIDSYNTH_LIBS)
AM_CONDITIONAL(FLUIDSYNTH_SUPPORT, test "$FLUIDSYNTH_SUPPORT" = "1")

dnl Check for FFTW
PKG_CHECK_MODULES(FFTW, fftw3f >= 3.0, FFTW_SUPPORT=1, FFTW_SUPPORT=0)
AC_SUBST(FFTW_CFLAGS)
AC_SUBST(FFTW_LIBS)
AM_CONDITIONAL(FFTW_SUPPORT, test "$FFTW_SUPPORT" = "1")

PKG_CHECK_MODULES(PYGTK, pygtk-2.0, PYGTK_SUPPORT=1, PYGTK_SUPPORT=0)
AC_SUBST(PYGTK_CFLAGS)
AC_SUBST(PYGTK_LIBS)

AC_ARG_ENABLE(python, AS_HELP_STRING([--disable-python], [disable python binding]),
  enable_python=$enableval, enable_python="yes")

if test "x${enable_python}" = "xyes" -a "$PYGTK_SUPPORT" = "1" ; then
  AC_DEFINE(PYTHON_SUPPORT, 1, [Define to build Python binding])

  # Get the codegen directory
  PYGTK_CODEGEN_DIR=`pkg-config pygtk-2.0 --variable=codegendir`

  if test "x$PYGTK_CODEGEN_DIR" = "x" ; then
      PYGTK_CODEGEN_DIR=/usr/share/pygtk/2.0/codegen
  fi

  AC_SUBST(PYGTK_CODEGEN_DIR)

  dnl Check for python 2.2 (defines PYTHON_CFLAGS, PYTHON_LIBS, PYTHON_SITE_PKG, PYTHON_EXTRA_LIBS)
  AC_PYTHON_DEVEL
else
  enable_python="no"
fi

AM_CONDITIONAL(PYTHON_SUPPORT, test "x$enable_python" = "xyes")


dnl - Check for GtkSourceView
PKG_CHECK_MODULES(GTK_SOURCE_VIEW, gtksourceview-1.0 >= 1.0,
    GTK_SOURCE_VIEW_SUPPORT=1, GTK_SOURCE_VIEW_SUPPORT=0)
AC_SUBST(GTK_SOURCE_VIEW_CFLAGS)
AC_SUBST(GTK_SOURCE_VIEW_LIBS)

AC_ARG_ENABLE(gtk-source-view, AS_HELP_STRING([--disable-gtk-source-view],
                                              [disable GtkSourceView support]),
  enable_gtk_source_view=$enableval, enable_gtk_source_view="yes")

if test "${enable_gtk_source_view}" = "yes" \
 -a "$GTK_SOURCE_VIEW_SUPPORT" = "1" \
 -a "${enable_python}" = "yes" ; then
  AC_DEFINE(GTK_SOURCE_VIEW_SUPPORT, 1,
    [Define to enable GtkSourceView support])
else
  enable_gtk_source_view="no"
fi


dnl *** Plugin stuff ***

AC_ARG_ENABLE(source-build,
[--enable-source-build] [Enable source build (use source path for externals],
[case "${enableval}" in
  yes) SOURCE_BUILD=yes ;;
  no)  SOURCE_BUILD=no ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-source-build) ;;
esac],
[SOURCE_BUILD=no]) dnl Default value

if test "x$SOURCE_BUILD" = "xyes"; then
  AC_DEFINE(SOURCE_BUILD, 1, [Define to enable use of source path for externals])
fi

AM_CONDITIONAL(SOURCE_BUILD, test "x$SOURCE_BUILD" = "xyes")


dnl Check for Gtk-doc
GTK_DOC_CHECK(1.9)

dnl Add m4 macro include directory
AC_CONFIG_MACRO_DIR(m4)


AC_OUTPUT([
	Makefile
	docs/Makefile
	docs/reference/Makefile
	docs/reference/libswami/Makefile
	docs/reference/swamigui/Makefile
	m4/Makefile
	src/Makefile
	src/libswami/Makefile
	src/libswami/version.h
	src/swamigui/Makefile
	src/swamigui/images/Makefile
	src/swamigui/widgets/Makefile
	src/plugins/Makefile
	src/python/Makefile
	po/Makefile.in
	package/Makefile
	package/osxbundle/Makefile])


_SUMMARY_ENABLED=
_SUMMARY_DISABLED=

FEATURE_SUMMARY([  Mac integration - Mac OS X integration for Gtk+],
  ["${MAC_INTEGRATION}" = "1"])

FEATURE_SUMMARY([  FluidSynth - Soft synth wavetable plugin],
  ["${FLUIDSYNTH_SUPPORT}" = "1"])

FEATURE_SUMMARY([  FFTW support - Auto sample tuning plugin (not available yet)],
  ["${FFTW_SUPPORT}" = "1"])

FEATURE_SUMMARY([  Python support], ["${enable_python}" = "yes"])

FEATURE_SUMMARY([  GtkSourceView support (extended Python script editing features)],["${enable_gtk_source_view}" = "yes"])

FEATURE_SUMMARY([  Generate gtk-doc API docs],
  ["x${enable_gtk_doc}" == "xyes"])

FEATURE_SUMMARY([  Debugging (compiler flags)],
  ["${disable_debug}" != "yes"])

FEATURE_SUMMARY([  Source build (load external files from source code path)],
  ["${SOURCE_BUILD}" == "yes"])

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
