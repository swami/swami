#
# Swami
# Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License only.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA or point your web browser to http://www.gnu.org.
#

project ( Swami C )
cmake_minimum_required ( VERSION 2.6.3 )
set ( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# Swami package name
set ( PACKAGE "swami" )

# Swami package version
set ( SWAMI_VERSION_MAJOR 2 )
set ( SWAMI_VERSION_MINOR 0 )
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
  set ( GNUCC_WARNING_FLAGS "-Wall -W -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wstrict-prototypes -Wno-unused-parameter -Wno-cast-qual")
  set ( CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${GNUCC_WARNING_FLAGS}" )
  set ( CMAKE_C_FLAGS_RELEASE "-O2 -fomit-frame-pointer -funroll-all-loops -finline-functions -DNDEBUG ${GNUCC_WARNING_FLAGS}" )
  set ( CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -fomit-frame-pointer -funroll-all-loops -finline-functions -DNDEBUG ${GNUCC_WARNING_FLAGS}" )
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( enable-debug )  
    set ( CMAKE_BUILD_TYPE "Debug" CACHE STRING
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
pkg_check_modules ( LIBINSTPATCH REQUIRED libinstpatch-1.0>=1.0 )

# Mandatory libraries: GTK+, librsvg and libgnomecanvas
pkg_check_modules ( GUI REQUIRED gtk+-2.0>=2.12 librsvg-2.0>=2.8 libgnomecanvas-2.0>=2.0 )

# Mandatory libraries: gobject, glib, gmodule and gthread
pkg_check_modules ( GOBJECT REQUIRED gobject-2.0>=2.12 glib-2.0>=2.12 gmodule-2.0>=2.12 gthread-2.0>=2.12 )

# Mandatory library libglade
pkg_check_modules ( LIBGLADE REQUIRED libglade-2.0 )


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
  pkg_check_modules ( FFTW fftw3>=3.0 )
  set ( FFTW_SUPPORT ${FFTW_FOUND} )
else ( enable-fftw )
  unset_pkg_config ( FFTW )
endif ( enable-fftw )


# General configuration file
configure_file ( ${CMAKE_SOURCE_DIR}/config.h.cmake 
                 ${CMAKE_CURRENT_BINARY_DIR}/config.h )
add_definitions ( -DHAVE_CONFIG_H )

# Process subdirectories
add_subdirectory ( src )

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

