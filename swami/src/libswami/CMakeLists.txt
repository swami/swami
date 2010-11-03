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

include_directories (
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GOBJECT_INCLUDEDIR} 
    ${GOBJECT_INCLUDE_DIRS} 
    ${LIBINSTPATCH_INCLUDEDIR} 
    ${LIBINSTPATCH_INCLUDE_DIRS} 
)

# ************ library ************

set ( libswami_public_HEADERS
    SwamiContainer.h
    SwamiControl.h
    SwamiControlEvent.h
    SwamiControlFunc.h
    SwamiControlHub.h
    SwamiControlMidi.h
    SwamiControlProp.h
    SwamiControlQueue.h
    SwamiControlValue.h
    SwamiEvent_ipatch.h
    SwamiLock.h
    SwamiLog.h
    SwamiLoopFinder.h
    SwamiLoopResults.h
    SwamiMidiDevice.h
    SwamiMidiEvent.h
    SwamiObject.h
    SwamiParam.h
    SwamiPlugin.h
    SwamiPropTree.h
    SwamiRoot.h
    SwamiWavetbl.h
    util.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
) 

set ( libswami_SOURCES
    SwamiContainer.c
    SwamiControl.c
    SwamiControlEvent.c
    SwamiControlFunc.c
    SwamiControlHub.c
    SwamiControlMidi.c
    SwamiControlProp.c
    SwamiControlQueue.c
    SwamiControlValue.c
    SwamiEvent_ipatch.c
    SwamiLock.c
    SwamiLog.c
    SwamiLoopFinder.c
    SwamiLoopResults.c
    SwamiMidiDevice.c
    SwamiMidiEvent.c
    SwamiObject.c
    SwamiParam.c
    SwamiPlugin.c
    SwamiPropTree.c
    SwamiRoot.c
    SwamiWavetbl.c
    libswami.c
    util.c
    value_transform.c
)


set ( public_main_HEADER libswami.h )

link_directories (
    ${GOBJECT_LIBDIR} 
    ${GOBJECT_LIBRARY_DIRS}
    ${LIBINSTPATCH_LIBDIR}
    ${LIBINSTPATCH_LIBRARY_DIRS}
)

add_definitions (
    -DLOCALEDIR="${DATA_INSTALL_DIR}/locale"
    -DG_LOG_DOMAIN=\"libswami\"
)

add_library ( libswami
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
    ${libswami_SOURCES} 
)

# Version file
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/version.h )

install ( FILES
          ${libswami_public_HEADERS}
          ${public_main_HEADER}
          DESTINATION include/swami/libswami )

find_program (GLIB2_MKENUMS glib-mkenums)
find_program (SED sed)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#ifndef __SWAMI_BUILTIN_ENUMS_H__\\n\"
    --fhead \"\#define __SWAMI_BUILTIN_ENUMS_H__\\n\\n\"
    --fhead \"\#include <glib-object.h>\\n\\n\"
    --fhead \"G_BEGIN_DECLS\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\\n\"
    --vhead \"GType @enum_name@_get_type \(void\)\;\\n\"
    --vhead \"\#define SWAMI_TYPE_@ENUMSHORT@ \(@enum_name@_get_type\(\)\)\\n\"
    --ftail \"G_END_DECLS\\n\\n\"
    --ftail \"\#endif /* __SWAMI_BUILTIN_ENUMS_H__ */\"
    ${libswami_public_HEADERS}
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${libswami_public_HEADERS}
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#include \\"libswami.h\\"\\n\"
    --fhead \"\#include \\"swami_priv.h\\"\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\"
    --vhead \"static const G@Type@Value _@enum_name@_values[] = {\"
    --vprod \"  { @VALUENAME@, \\"@VALUENAME@\\", \\"@valuenick@\\" },\"
    --vtail \"  { 0, NULL, NULL }\\n}\;\\n\\n\"
    --vtail \"GType\\n@enum_name@_get_type \(void\)\\n{\\n\"
    --vtail \"  static GType type = 0\;\\n\\n\"
    --vtail \"  if \(G_UNLIKELY \(type == 0\)\)\\n\"
    --vtail \"    type = g_\@type\@_register_static \(\\"@EnumName@\\", _@enum_name@_values\)\;\\n\\n\"
    --vtail \"  return type\;\\n}\\n\\n\"
    ${libswami_public_HEADERS}
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${libswami_public_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
)


find_program (GLIB2_GENMARSHAL glib-genmarshal)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --body --prefix=swami_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --header --prefix=swami_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list
)


target_link_libraries ( libswami
    ${GOBJECT_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES}
)

if ( MACOSX_FRAMEWORK )
     set_property ( SOURCE ${libswami_public_HEADERS} 
         PROPERTY MACOSX_PACKAGE_LOCATION Headers/libswami
     )
    set_target_properties ( libswami
      PROPERTIES
        OUTPUT_NAME "libswami"
        FRAMEWORK TRUE
        PUBLIC_HEADER "${public_main_HEADER}"
        FRAMEWORK_VERSION "${LIB_VERSION_CURRENT}"
        INSTALL_NAME_DIR ${FRAMEWORK_INSTALL_DIR}
        VERSION ${LIB_VERSION_INFO}
        SOVERSION ${LIB_VERSION_CURRENT}
    )
else ( MACOSX_FRAMEWORK )
  set_target_properties ( libswami
    PROPERTIES
      PREFIX "lib"
      OUTPUT_NAME "swami"
      VERSION ${LIB_VERSION_INFO}
      SOVERSION ${LIB_VERSION_CURRENT}
  )
endif ( MACOSX_FRAMEWORK )

install ( TARGETS libswami
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
  BUNDLE DESTINATION ${BUNDLE_INSTALL_DIR}
)

