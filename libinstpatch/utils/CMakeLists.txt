include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GOBJECT_INCLUDEDIR}
    ${GOBJECT_INCLUDE_DIRS}
)

add_executable ( riff_dump riff_dump.c )
add_dependencies( riff_dump libinstpatch-1.0 )
target_link_libraries( riff_dump libinstpatch-1.0 )
