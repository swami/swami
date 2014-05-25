include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GOBJECT_INCLUDEDIR}
    ${GOBJECT_INCLUDE_DIRS}
    ${SNDFILE_INCLUDEDIR}
    ${SNDFILE_INCLUDE_DIRS}
)

link_directories (
    ${GOBJECT_LIBDIR}
    ${GOBJECT_LIBRARY_DIRS}
    ${SNDFILE_LIBDIR}
    ${SNDFILE_LIBRARY_DIRS}
)

add_executable ( riff_dump riff_dump.c )
add_dependencies( riff_dump instpatch-1.0 )
target_link_libraries( riff_dump instpatch-1.0 )
