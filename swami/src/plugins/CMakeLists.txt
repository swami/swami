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

target_link_libraries ( fluidsynth_plugin libswami ${GOBJECT_LIBRARIES}
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

install ( TARGETS fluidsynth_plugin fluidsynth_gui LIBRARY DESTINATION ${PLUGINS_DIR} )

endif ( FLUIDSYNTH_SUPPORT )


if ( FFTW_SUPPORT )

link_directories ( ${GOBJECT_LIBDIR} ${GOBJECT_LIBRARY_DIRS}
    ${LIBINSTPATCH_LIBDIR} ${LIBINSTPATCH_LIBRARY_DIRS} ${FFTW_LIBDIR} ${FFTW_DIRS}
)

add_library ( fftune MODULE fftune.c )

target_link_libraries ( fftune libswami ${GOBJECT_LIBRARIES}
    ${LIBINSTPATCH_LIBRARIES} ${FFTW_LIBRARIES}
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

install ( TARGETS fftune fftune_gui LIBRARY DESTINATION ${PLUGINS_DIR} )

endif ( FFTW_SUPPORT )
