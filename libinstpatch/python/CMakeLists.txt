#FIND_PACKAGE(PythonInterp)
FIND_PACKAGE(PythonLibs)

include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GOBJECT_INCLUDEDIR}
    ${GOBJECT_INCLUDE_DIRS}
    ${PYGOBJECT_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
)

find_program (PYGOBJECT_CODEGEN pygobject-codegen-2.0)

add_custom_command (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ipatch.c
    COMMAND ${PYGOBJECT_CODEGEN}
    ARGS
        --register ${CMAKE_CURRENT_SOURCE_DIR}/ipatch-types.defs
        --override ${CMAKE_CURRENT_SOURCE_DIR}/ipatch.override
        --prefix pyipatch ${CMAKE_CURRENT_SOURCE_DIR}/ipatch.defs
        > ${CMAKE_CURRENT_BINARY_DIR}/ipatch.c
)

add_library ( ipatchmodule ipatchmodule.c ${CMAKE_CURRENT_BINARY_DIR}/ipatch.c )
add_dependencies ( ipatchmodule libinstpatch-1.0 )
target_link_libraries ( ipatchmodule 
		        libinstpatch-1.0
		        ${PYTHON_LIBRARIES}
)
set_target_properties ( ipatchmodule PROPERTIES PREFIX "" )

execute_process ( COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1, 0, '${CMAKE_INSTALL_PREFIX}')"
                  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
install ( TARGETS ipatchmodule
    RUNTIME DESTINATION ${PYTHON_SITE_PACKAGES}
    LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}
)
