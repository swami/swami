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
    ${GUI_INCLUDEDIR} 
    ${GUI_INCLUDE_DIRS} 
    ${LIBGLADE_INCLUDEDIR} 
    ${LIBGLADE_INCLUDE_DIRS} 
    ${LIBINSTPATCH_INCLUDEDIR} 
    ${LIBINSTPATCH_INCLUDE_DIRS} 
)

# ************ swamigui Library ************

set ( libswamigui_public_HEADERS
    SwamiguiBar.h
    SwamiguiBarPtr.h
    SwamiguiCanvasMod.h
    SwamiguiComboEntry.h
    SwamiguiControl.h
    SwamiguiControlAdj.h
    SwamiguiControlMidiKey.h
    SwamiguiItemMenu.h
    SwamiguiKnob.h
    SwamiguiLoopFinder.h
    SwamiguiMenu.h
    SwamiguiModEdit.h
    SwamiguiMultiSave.h
    SwamiguiNoteSelector.h
    SwamiguiPanel.h
    SwamiguiPanelSelector.h
    SwamiguiPanelSF2Gen.h
    SwamiguiPanelSF2GenEnv.h
    SwamiguiPanelSF2GenMisc.h
    SwamiguiPaste.h
    SwamiguiPiano.h
    SwamiguiPref.h
    SwamiguiProp.h
    SwamiguiRoot.h
    SwamiguiSampleCanvas.h
    SwamiguiSampleEditor.h
    SwamiguiSpectrumCanvas.h
    SwamiguiSpinScale.h
    SwamiguiSplits.h
    SwamiguiStatusbar.h
    SwamiguiTree.h
    SwamiguiTreeStore.h
    SwamiguiTreeStoreConfig.h
    SwamiguiTreeStorePatch.h
    help.h
    icons.h
    patch_funcs.h
    splash.h
    util.h
    widgets/combo-box.h
    widgets/icon-combo.h
) 

set ( libswamigui_SOURCES
    SwamiguiBar.c
    SwamiguiBarPtr.c
    SwamiguiCanvasMod.c
    SwamiguiComboEntry.c
    SwamiguiControl.c
    SwamiguiControl_widgets.c
    SwamiguiControlAdj.c
    SwamiguiControlMidiKey.c
    SwamiguiItemMenu.c
    SwamiguiItemMenu_actions.c
    SwamiguiKnob.c
    SwamiguiLoopFinder.c
    SwamiguiMenu.c
    SwamiguiModEdit.c
    SwamiguiMultiSave.c
    SwamiguiNoteSelector.c
    SwamiguiPanel.c
    SwamiguiPanelSelector.c
    SwamiguiPanelSF2Gen.c
    SwamiguiPanelSF2GenEnv.c
    SwamiguiPanelSF2GenMisc.c
    SwamiguiPaste.c
    SwamiguiPiano.c
    SwamiguiPref.c
    SwamiguiProp.c
    SwamiguiRoot.c
    SwamiguiSampleCanvas.c
    SwamiguiSampleEditor.c
    SwamiguiSpectrumCanvas.c
    SwamiguiSpinScale.c
    SwamiguiSplits.c
    SwamiguiStatusbar.c
    SwamiguiTree.c
    SwamiguiTreeStore.c
    SwamiguiTreeStoreConfig.c
    SwamiguiTreeStorePatch.c
    help.c
    icons.c
    patch_funcs.c
    splash.c
    util.c
    widgets/combo-box.c
    widgets/icon-combo.c
)


set ( public_main_HEADER swamigui.h )

link_directories (
    ${GUI_LIBDIR} 
    ${GUI_LIBRARY_DIRS}
    ${LIBGLADE_LIBDIR} 
    ${LIBGLADE_LIBRARY_DIRS}
    ${LIBINSTPATCH_LIBDIR}
    ${LIBINSTPATCH_LIBRARY_DIRS}
)

add_definitions (
    -DLOCALEDIR=\"${DATA_INSTALL_DIR}/locale\"
    -DG_LOG_DOMAIN=\"libswamigui\"
)

add_library ( libswamigui
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
    ${libswamigui_SOURCES}
)

if (UNIX)
    set(MATH_LIB m)
endif (UNIX)

target_link_libraries ( libswamigui
    libswami
    ${GUI_LIBRARIES}
    ${LIBGLADE_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES}
    ${COREFOUNDATION}
    ${MATH_LIB}
)

install ( FILES
    ${libswamigui_public_HEADERS}
    ${public_main_HEADER}
    DESTINATION ${INCLUDE_INSTALL_DIR}/swami/libswamigui )

if (APPLE)
  set_property ( TARGET libswamigui
      APPEND PROPERTY INCLUDE_DIRECTORIES "${COREFOUNDATION}/Headers"
  )
endif (APPLE)

# ************ Swami GUI application ************

add_executable ( swami main.c )

if ( SWAMI_CPPFLAGS )
  set_target_properties ( swami
    PROPERTIES COMPILE_FLAGS ${SWAMI_CPPFLAGS} )
endif ( SWAMI_CPPFLAGS )

target_link_libraries ( swami
    libswamigui
    libswami
    ${GUI_LIBRARIES}
    ${LIBGLADE_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES}
)


# Process subdirectories
add_subdirectory ( images )


find_program (GLIB2_MKENUMS glib-mkenums)
find_program (SED sed)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#ifndef __SWAMIGUI_BUILTIN_ENUMS_H__\\n\"
    --fhead \"\#define __SWAMIGUI_BUILTIN_ENUMS_H__\\n\\n\"
    --fhead \"\#include <glib-object.h>\\n\\n\"
    --fhead \"G_BEGIN_DECLS\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\\n\"
    --vhead \"GType @enum_name@_get_type \(void\)\;\\n\"
    --vhead \"\#define SWAMIGUI_TYPE_@ENUMSHORT@ \(@enum_name@_get_type\(\)\)\\n\"
    --ftail \"G_END_DECLS\\n\\n\"
    --ftail \"\#endif /* __SWAMIGUI_BUILTIN_ENUMS_H__ */\"
    ${libswamigui_public_HEADERS}
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${libswamigui_public_HEADERS}
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#include \\"swamigui.h\\"\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\"
    --vhead \"static const G@Type@Value _@enum_name@_values[] = {\"
    --vprod \"  { @VALUENAME@, \\"@VALUENAME@\\", \\"@valuenick@\\" },\"
    --vtail \"  { 0, NULL, NULL }\\n}\;\\n\\n\"
    --vtail \"GType\\n@enum_name@_get_type \(void\)\\n{\\n\"
    --vtail \"  static GType type = 0\;\\n\\n\"
    --vtail \"  if \(G_UNLIKELY \(type == 0\)\)\\n\"
    --vtail \"    type = g_\@type\@_register_static \(\\"@EnumName@\\", _@enum_name@_values\)\;\\n\\n\"
    --vtail \"  return type\;\\n}\\n\\n\"
    ${libswamigui_public_HEADERS}
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${libswamigui_public_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
)


find_program (GLIB2_GENMARSHAL glib-genmarshal)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --body --prefix=swamigui_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --header --prefix=swamigui_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list
)

if ( MACOSX_FRAMEWORK )
     set_property ( SOURCE ${libswami_public_HEADERS} 
         PROPERTY MACOSX_PACKAGE_LOCATION Headers/libswamigui
     )
    set_target_properties ( libswamigui
      PROPERTIES
        OUTPUT_NAME "libswamigui"
        FRAMEWORK TRUE
        PUBLIC_HEADER "${public_main_HEADER}"
        FRAMEWORK_VERSION "${LIB_VERSION_CURRENT}"
        INSTALL_NAME_DIR ${FRAMEWORK_INSTALL_DIR}
        VERSION ${LIB_VERSION_INFO}
        SOVERSION ${LIB_VERSION_CURRENT}
    )
else ( MACOSX_FRAMEWORK )
  set_target_properties ( libswamigui
    PROPERTIES
      PREFIX "lib"
      OUTPUT_NAME "swamigui"
      VERSION ${LIB_VERSION_INFO}
      SOVERSION ${LIB_VERSION_CURRENT}
  )
endif ( MACOSX_FRAMEWORK )

install ( TARGETS swami libswamigui
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
  BUNDLE DESTINATION ${BUNDLE_INSTALL_DIR}
)

install ( FILES
          swami-2.ui
          DESTINATION ${UIXML_DIR} )

