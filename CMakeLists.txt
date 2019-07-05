# 
# Swami
#
# Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
#
# See COPYING license file for distribution details
#

project ( Swami C )
cmake_minimum_required ( VERSION 2.6.3 )
set ( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# Swami package name
set ( PACKAGE "swami" )

# Swami package version
set ( SWAMI_VERSION_MAJOR 2 )
set ( SWAMI_VERSION_MINOR 1 )
set ( SWAMI_VERSION_MICRO 0 )
set ( VERSION "${SWAMI_VERSION_MAJOR}.${SWAMI_VERSION_MINOR}.${SWAMI_VERSION_MICRO}" )
set ( SWAMI_VERSION "\"${VERSION}\"" )

# libswami/libswamigui - Library versions
# *** NOTICE ***
# Update library version upon each release (follow these steps in order)
# if any source code changes: REVISION++
# if any interfaces added/removed/changed: REVISION=0
# if any interfaces removed/changed (compatibility broken): CURRENT++
# if any interfaces have been added: AGE++
# if any interfaces have been removed/changed (compatibility broken): AGE=0
# This is not exactly the same algorithm as the libtool one, but the results are the same.
set ( LIB_VERSION_CURRENT 0 )
set ( LIB_VERSION_AGE 0 )
set ( LIB_VERSION_REVISION 0 )
set ( LIB_VERSION_INFO 
      "${LIB_VERSION_CURRENT}.${LIB_VERSION_AGE}.${LIB_VERSION_REVISION}" )

# Options disabled by default
option ( enable-debug "enable debugging (default=no)" off )
option ( enable-source-build "enable source build - load resources from source dir (default=no)" off )
option ( GTKDOC_ENABLED "Create Gtk-Doc API reference (default=no)" off )

# Options enabled by default
option ( BUILD_SHARED_LIBS "Build a shared object or DLL (default=yes)" on )
option ( enable-fluidsynth "enable FluidSynth plugin - needed for sound (if it is available)" on )
option ( enable-fftw "enable fftw support for the FFTune plugin (if it is available)" on )

# Initialize the library directory name suffix.
if ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set ( _init_lib_suffix "64" )
else ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set ( _init_lib_suffix "" )
endif ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
set ( LIB_SUFFIX ${_init_lib_suffix} CACHE STRING 
      "library directory name suffix (32/64/nothing)" )
mark_as_advanced ( LIB_SUFFIX )

# Default install directory names
include ( DefaultDirs )

# Basic C library checks
include ( CheckSTDC )
include ( CheckIncludeFile )
check_include_file ( string.h HAVE_STRING_H )
check_include_file ( stdlib.h HAVE_STDLIB_H ) 
check_include_file ( stdio.h HAVE_STDIO_H ) 
check_include_file ( math.h HAVE_MATH_H ) 
check_include_file ( errno.h HAVE_ERRNO_H ) 
check_include_file ( stdarg.h HAVE_STDARG_H ) 
check_include_file ( unistd.h HAVE_UNISTD_H ) 

unset ( SWAMI_LIBS CACHE )

# Options for the GNU C compiler only
if ( CMAKE_COMPILER_IS_GNUCC )
  if ( NOT APPLE )
    set ( CMAKE_EXE_LINKER_FLAGS 
          "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed" )
    set ( CMAKE_SHARED_LINKER_FLAGS 
          "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
  endif ( NOT APPLE )
  set ( GNUCC_WARNING_FLAGS "-Wall")
  set ( CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${GNUCC_WARNING_FLAGS}" )
  set ( CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG ${GNUCC_WARNING_FLAGS}" )
  set ( CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG ${GNUCC_WARNING_FLAGS}" )
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( enable-debug )  
    set ( CMAKE_BUILD_TYPE "Debug" CACHE STRING
          "Choose the build type, options: Debug Release RelWithDebInfo" FORCE )
else ( enable-debug )
    set ( CMAKE_BUILD_TYPE "Release" CACHE STRING
          "Choose the build type, options: Debug Release RelWithDebInfo" FORCE )
endif ( enable-debug )

unset ( MINGW32 CACHE )
if ( WIN32 )
  # MinGW compiler (a Windows GCC port)
  if ( MINGW ) 
    set ( MINGW32 1 )
    add_definitions ( -mms-bitfields )
  endif  ( MINGW )
else ( WIN32 )
  set ( SWAMI_LIBS "m" )
endif ( WIN32 )

unset ( DEBUG CACHE )
if ( CMAKE_BUILD_TYPE MATCHES "Debug" )
    set ( DEBUG 1 )
endif ( CMAKE_BUILD_TYPE MATCHES "Debug" )

unset ( SOURCE_BUILD CACHE )
unset ( SOURCE_DIR CACHE )
if ( enable-source-build )
    set ( SOURCE_BUILD 1 )
    set ( SOURCE_DIR ${CMAKE_SOURCE_DIR} )
    set ( PLUGINS_DIR ${CMAKE_BINARY_DIR}/src/plugins )
endif ( enable-source-build )


# Mandatory tool: pkg-config
find_package ( PkgConfig REQUIRED )

# Mandatory library libinstpatch
pkg_check_modules ( LIBINSTPATCH REQUIRED libinstpatch-1.0>=1.1 )

# Mandatory libraries: GTK+ and libgnomecanvas
pkg_check_modules ( GUI REQUIRED gtk+-2.0>=2.12 libgnomecanvas-2.0>=2.0 )

# Mandatory libraries: gobject, glib, gmodule and gthread
pkg_check_modules ( GOBJECT REQUIRED gobject-2.0>=2.12 glib-2.0>=2.12 gmodule-2.0>=2.12 gthread-2.0>=2.12 )

# Disable deprecation warnings for now (fixed in master)
add_definitions ( -DGLIB_DISABLE_DEPRECATION_WARNINGS )

# Needed on OS X
if (APPLE)
  find_library ( COREFOUNDATION NAMES CoreFoundation )
  mark_as_advanced ( COREFOUNDATION )
endif (APPLE)


include ( UnsetPkgConfig )

# Optional library FluidSynth
unset ( FLUIDSYNTH_SUPPORT CACHE )
if ( enable-fluidsynth )
  pkg_check_modules ( FLUIDSYNTH fluidsynth>=1.0 )
  set ( FLUIDSYNTH_SUPPORT ${FLUIDSYNTH_FOUND} )
else ( enable-fluidsynth )
  unset_pkg_config ( FLUIDSYNTH )
endif ( enable-fluidsynth )

# Optional library fftw3
unset ( FFTW_SUPPORT CACHE )
if ( enable-fftw )
  pkg_check_modules ( FFTW fftw3f>=3.0 )
  set ( FFTW_SUPPORT ${FFTW_FOUND} )
else ( enable-fftw )
  unset_pkg_config ( FFTW )
endif ( enable-fftw )

# Check for Gtk-Doc
if (GTKDOC_ENABLED)
  include (FindGtkDoc)
endif ()

# General configuration file
configure_file ( ${CMAKE_SOURCE_DIR}/config.h.cmake 
                 ${CMAKE_CURRENT_BINARY_DIR}/config.h )
add_definitions ( -DHAVE_CONFIG_H )

# Process subdirectories
add_subdirectory ( src )
add_subdirectory ( docs )

# Extra targets for Unix build environments
if ( UNIX )
    # uninstall custom target
    configure_file ( "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
    add_custom_target ( uninstall
      "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

    # Install XDG mime type, application icon and .desktop file
    install ( FILES swami.desktop DESTINATION ${DATA_INSTALL_DIR}/applications )
    install ( FILES swami.xml DESTINATION ${DATA_INSTALL_DIR}/mime/packages )
    install ( FILES swami.png DESTINATION ${DATA_INSTALL_DIR}/icons/hicolor/48x48/apps )
    install ( FILES swami.svg DESTINATION ${DATA_INSTALL_DIR}/icons/hicolor/scalable/apps )
endif ( UNIX )

# Extra targets for Windows
if ( WIN32 )
    install ( FILES swami.ico DESTINATION ${BIN_INSTALL_DIR} )
endif ( WIN32 )

message( "\n**************************************************************\n" )

if ( FLUIDSYNTH_SUPPORT )
  message ( "FluidSynth:            yes" )
else ( FLUIDSYNTH_SUPPORT )
  message ( "FluidSynth:            no (there will be no sound!)" )
endif ( FLUIDSYNTH_SUPPORT )

if ( FFTW_SUPPORT )
  message ( "FFTW:                  yes" )
else ( FFTW_SUPPORT )
  message ( "FFTW:                  no (there will be no FFTune plugin!)" )
endif ( FFTW_SUPPORT )

if (GTKDOC_FOUND)
  message ( "Gtk-Doc API reference: yes" ) 
else (GTKDOC_FOUND)
  message ( "Gtk-Doc API reference: no" ) 
endif(GTKDOC_FOUND)

if ( DEBUG )
  message ( "Debug:                 yes" )
else ( DEBUG )
  message ( "Debug:                 no" )
endif ( DEBUG )

if ( SOURCE_BUILD )
  message ( "Source build:          yes (resources loaded from source dir)" )
else ( SOURCE_BUILD )
  message ( "Source build:          no" )
endif ( SOURCE_BUILD )

message ( "**************************************************************\n\n" )

# CPack support 
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Swami instrument editor" )
set ( CPACK_PACKAGE_VENDOR "swami.sourceforge.net" )
set ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README" )
set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING" )
set ( CPACK_PACKAGE_VERSION_MAJOR ${SWAMI_VERSION_MAJOR} )
set ( CPACK_PACKAGE_VERSION_MINOR ${SWAMI_VERSION_MINOR} )
set ( CPACK_PACKAGE_VERSION_PATCH ${SWAMI_VERSION_MICRO} )

# source packages
set ( CPACK_SOURCE_GENERATOR TGZ;TBZ2;ZIP )
set ( CPACK_SOURCE_IGNORE_FILES "/.svn/;~$;.cproject;.project;/.settings/;${CPACK_SOURCE_IGNORE_FILES}" )
set ( CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE}-${VERSION}" ) 
set ( CPACK_SOURCE_STRIP_FILES OFF )

# binary packages
include ( InstallRequiredSystemLibraries )
set ( CPACK_GENERATOR STGZ;TGZ;TBZ2;ZIP )
set ( CPACK_PACKAGE_NAME ${PACKAGE} )
set ( CPACK_STRIP_FILES ON )

include ( CPack )

file(GLOB_RECURSE
     ALL_SOURCE_FILES
     LIST_DIRECTORIES false
     ${CMAKE_SOURCE_DIR}/*.[chi]
     ${CMAKE_SOURCE_DIR}/*.[chi]pp
     ${CMAKE_SOURCE_DIR}/*.[chi]xx
     ${CMAKE_SOURCE_DIR}/*.cc
     ${CMAKE_SOURCE_DIR}/*.hh
     ${CMAKE_SOURCE_DIR}/*.ii
     ${CMAKE_SOURCE_DIR}/*.[CHI]
     )
     
find_program ( ASTYLE "astyle" )
if ( ASTYLE )
  add_custom_target(
    format
    COMMAND ${ASTYLE}
    -A1
    -xb
    -j
    -k3
    -p
    -f
    -n
    -U
    ${ALL_SOURCE_FILES}
    )
endif(ASTYLE)
