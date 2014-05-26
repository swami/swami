# 
# Swami
#
# Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
#
# See COPYING license file for distribution details
#

include_directories (
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GOBJECT_INCLUDEDIR}
    ${GOBJECT_INCLUDE_DIRS}
    ${GUI_INCLUDEDIR}
    ${GUI_INCLUDE_DIRS}
    ${LIBINSTPATCH_INCLUDEDIR}
    ${LIBINSTPATCH_INCLUDE_DIRS}
    ${FLUIDSYNTH_INCLUDEDIR}
    ${FLUIDSYNTH_INCLUDE_DIRS}
    ${FFTW_INCLUDEDIR}
    ${FFTW_INCLUDE_DIRS}
)


if ( FLUIDSYNTH_SUPPORT )

link_directories ( ${GOBJECT_LIBDIR} ${GOBJECT_LIBRARY_DIRS}
    ${LIBINSTPATCH_LIBDIR} ${LIBINSTPATCH_LIBRARY_DIRS} ${FLUIDSYNTH_LIBDIR}
    ${FLUIDSYNTH_LIBRARY_DIRS}
)

add_library ( fluidsynth_plugin MODULE fluidsynth.c )

target_link_libraries ( fluidsynth_plugin libswami libswamigui ${GOBJECT_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES} ${FLUIDSYNTH_LIBRARIES}
)

link_directories ( ${GUI_LIBDIR} ${GUI_LIBRARY_DIRS} ${LIBINSTPATCH_LIBDIR}
    ${LIBINSTPATCH_LIBRARY_DIRS} ${FLUIDSYNTH_LIBDIR} ${FLUIDSYNTH_LIBRARY_DIRS}
)

add_library ( fluidsynth_gui MODULE fluidsynth_gui.c )

target_link_libraries ( fluidsynth_gui libswami libswamigui ${GUI_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES} ${FLUIDSYNTH_LIBRARIES}
)

set_target_properties ( fluidsynth_plugin PROPERTIES PREFIX "" )
set_target_properties ( fluidsynth_gui PROPERTIES PREFIX "" )

install ( TARGETS fluidsynth_plugin fluidsynth_gui
    RUNTIME DESTINATION ${PLUGINS_DIR}
    LIBRARY DESTINATION ${PLUGINS_DIR}
)

endif ( FLUIDSYNTH_SUPPORT )


if ( FFTW_SUPPORT )

link_directories ( ${GOBJECT_LIBDIR} ${GOBJECT_LIBRARY_DIRS}
    ${LIBINSTPATCH_LIBDIR} ${LIBINSTPATCH_LIBRARY_DIRS} ${FFTW_LIBDIR} ${FFTW_LIBRARY_DIRS}
)

add_library ( fftune MODULE fftune.c )

if (UNIX)
    set(MATH_LIB m)
endif (UNIX)

target_link_libraries ( fftune libswami ${GOBJECT_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES} ${FFTW_LIBRARIES} ${MATH_LIB}
)

link_directories ( ${GUI_LIBDIR} ${GUI_LIBRARY_DIRS} ${LIBINSTPATCH_LIBDIR}
    ${LIBINSTPATCH_LIBRARY_DIRS}
)

add_library ( fftune_gui MODULE fftune_gui.c )

target_link_libraries ( fftune_gui libswami libswamigui ${GUI_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES}
)

set_target_properties ( fftune PROPERTIES PREFIX "" )
set_target_properties ( fftune_gui PROPERTIES PREFIX "" )

install ( TARGETS fftune fftune_gui
    RUNTIME DESTINATION ${PLUGINS_DIR}
    LIBRARY DESTINATION ${PLUGINS_DIR}
)

endif ( FFTW_SUPPORT )
