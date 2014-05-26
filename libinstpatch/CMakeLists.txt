# 
# libInstPatch
#
# Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License
# as published by the Free Software Foundation; version 2.1 of the
# License only.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307, USA
#

project ( libInstPatch C )
cmake_minimum_required ( VERSION 2.6.3 )
set ( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# libInstPatch package name
set ( PACKAGE "libinstpatch" )

# libInstPatch package version
set ( IPATCH_VERSION_MAJOR 1 )
set ( IPATCH_VERSION_MINOR 1 )
set ( IPATCH_VERSION_MICRO 0 )
set ( VERSION "${IPATCH_VERSION_MAJOR}.${IPATCH_VERSION_MINOR}.${IPATCH_VERSION_MICRO}" )      
set ( IPATCH_VERSION "\"${VERSION}\"" )

# libinstpatch - Library version
# *** NOTICE ***
# Update library version upon each release (follow these steps in order)
# if any source code changes: REVISION++
# if any interfaces added/removed/changed: REVISION=0
# if any interfaces removed/changed (compatibility broken): CURRENT++
# if any interfaces have been added: AGE++
# if any interfaces have been removed/changed (compatibility broken): AGE=0
# This is not exactly the same algorithm as the libtool one, but the results are the same.
set ( LIB_VERSION_CURRENT 0 )
set ( LIB_VERSION_AGE 1 )
set ( LIB_VERSION_REVISION 0 )
set ( LIB_VERSION_INFO 
      "${LIB_VERSION_CURRENT}.${LIB_VERSION_AGE}.${LIB_VERSION_REVISION}" )

# Options disabled by default
option ( enable-debug "enable debugging (default=no)" off )
option ( GTKDOC_ENABLED "Create Gtk-Doc API reference (default=no)" off )

# Options enabled by default
option ( BUILD_SHARED_LIBS "Build a shared object or DLL" on )

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

unset ( IPATCH_CPPFLAGS CACHE )
unset ( IPATCH_LIBS CACHE )

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
    set ( IPATCH_DEBUG 1 )
endif ( enable-debug )

unset ( MINGW32 CACHE )
if ( WIN32 )
  # MinGW compiler (a Windows GCC port)
  if ( MINGW ) 
    set ( MINGW32 1 )
    add_definitions ( -mms-bitfields )
  endif  ( MINGW )
else ( WIN32 )
  set ( IPATCH_LIBS "m" )
endif ( WIN32 )

unset ( ENABLE_DEBUG CACHE )
unset ( DEBUG CACHE )
if ( CMAKE_BUILD_TYPE MATCHES "Debug" )
    set ( ENABLE_DEBUG 1 )
    set ( DEBUG 1 )
endif ( CMAKE_BUILD_TYPE MATCHES "Debug" )

# Mandatory tool: pkg-config
find_package ( PkgConfig REQUIRED )

# Mandatory libraries: gobject, glib and gthread
pkg_check_modules ( GOBJECT REQUIRED gobject-2.0>=2.12 glib-2.0>=2.12 gthread-2.0>=2.12 )

# Disable deprecation warnings for now (fixed in master)
add_definitions ( -DGLIB_DISABLE_DEPRECATION_WARNINGS )

include ( UnsetPkgConfig )

# Check for libsndfile
pkg_check_modules ( SNDFILE REQUIRED sndfile>=1.0.0 )

# Check for Gtk-Doc
if (GTKDOC_ENABLED)
  include (FindGtkDoc)
endif ()

# General configuration file
configure_file ( ${CMAKE_SOURCE_DIR}/config.h.cmake 
                 ${CMAKE_BINARY_DIR}/config.h )
add_definitions ( -DHAVE_CONFIG_H )

# Version and master libinstpatch.h file
configure_file ( ${CMAKE_SOURCE_DIR}/libinstpatch/version.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/libinstpatch/version.h )
configure_file ( ${CMAKE_SOURCE_DIR}/libinstpatch/libinstpatch.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/libinstpatch/libinstpatch.h )

if ( UNIX OR MINGW )
    # pkg-config support
    set ( prefix "${CMAKE_INSTALL_PREFIX}" )
    set ( exec_prefix "${CMAKE_INSTALL_PREFIX}" )
    if (LIB_INSTALL_DIR STREQUAL ${CMAKE_INSTALL_PREFIX}/lib)
        set(LIBINSTPATCH_INSTALL_DIR ${LIB_INSTALL_DIR}${LIB_SUFFIX})
    else (LIB_INSTALL_DIR STREQUAL ${CMAKE_INSTALL_PREFIX}/lib)
        set(LIBINSTPATCH_INSTALL_DIR ${LIB_INSTALL_DIR})
    endif (LIB_INSTALL_DIR STREQUAL ${CMAKE_INSTALL_PREFIX}/lib)
    set ( libdir "${LIBINSTPATCH_INSTALL_DIR}" )
    set ( includedir "${INCLUDE_INSTALL_DIR}" )
    configure_file ( libinstpatch-1.0.pc.in 
        ${CMAKE_BINARY_DIR}/libinstpatch-1.0.pc IMMEDIATE @ONLY )
    install ( FILES ${CMAKE_BINARY_DIR}/libinstpatch-1.0.pc
              DESTINATION ${LIBINSTPATCH_INSTALL_DIR}/pkgconfig )
endif ( UNIX OR MINGW )

# Extra targets for Unix build environments
if ( UNIX )
    # uninstall custom target
    configure_file ( "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
    add_custom_target ( uninstall
      "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif ( UNIX )

# Process subdirectories 
add_subdirectory ( libinstpatch )
add_subdirectory ( utils )
add_subdirectory ( docs )

unset ( PYGOBJECT_FOUND )
if (BUILD_PYTHON_BINDING)
    pkg_check_modules ( PYGOBJECT pygobject-2.0 )

    if ( PYGOBJECT_FOUND )
        add_subdirectory ( python )
    endif ( PYGOBJECT_FOUND )
endif (BUILD_PYTHON_BINDING)

# CPack support 
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "libInstPatch instrument editing library" )
set ( CPACK_PACKAGE_VENDOR "swami.sourceforge.net" )
set ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README" )
set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING" )
set ( CPACK_PACKAGE_VERSION_MAJOR ${IPATCH_VERSION_MAJOR} )
set ( CPACK_PACKAGE_VERSION_MINOR ${IPATCH_VERSION_MINOR} )
set ( CPACK_PACKAGE_VERSION_PATCH ${IPATCH_VERSION_MICRO} )

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

message ( "Build options:" )

if ( ENABLE_DEBUG )
  message ( "Debug:                 yes" )
else ( ENABLE_DEBUG )
  message ( "Debug:                 no" )
endif ( ENABLE_DEBUG )

if (GTKDOC_FOUND)
  message ( "Gtk-Doc API reference: yes" ) 
else (GTKDOC_FOUND)
  message ( "Gtk-Doc API reference: no" ) 
endif(GTKDOC_FOUND)

