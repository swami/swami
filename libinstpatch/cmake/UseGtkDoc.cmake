# CMake macros to use the GtkDoc documentation system

find_package(GtkDoc)

# gtk_doc_add_module(doc_prefix sourcedir 
#                    [XML xmlfile] 
#                    [FIXXREFOPTS fixxrefoption1...]
#                    [IGNOREHEADERS header1...]
#                    [DEPENDS depend1...] )
#
# sourcedir must be the *full* path to the source directory.
#
# If omitted, sgmlfile defaults to the auto generated ${doc_prefix}/${doc_prefix}-docs.xml.
function(gtk_doc_add_module _doc_prefix _doc_sourcedir)
	set (_multi_value DEPENDS XML FIXXREFOPTS IGNOREHEADERS CFLAGS LDFLAGS LDPATH SUFFIXES TYPEINITFUNC EXTRAHEADERS)
	cmake_parse_arguments (ARG "" "" "${_multi_value}" ${ARGN})

    list(LENGTH ARG_XML _xml_file_length)

    if(ARG_SUFFIXES)
        set(_doc_source_suffixes "")
        foreach(_suffix ${ARG_SUFFIXES})
            if(_doc_source_suffixes)
                set(_doc_source_suffixes "${_doc_source_suffixes},${_suffix}")
            else(_doc_source_suffixes)
                set(_doc_source_suffixes "${_suffix}")
            endif(_doc_source_suffixes)
        endforeach(_suffix)
    else(ARG_SUFFIXES)
        set(_doc_source_suffixes "h")
    endif(ARG_SUFFIXES)

    set(_do_all ALL)

    set(_opts_valid 1)
    if(NOT _xml_file_length LESS 2)
        message(SEND_ERROR "Must have at most one sgml file specified.")
        set(_opts_valid 0)
    endif(NOT _xml_file_length LESS 2)

    if(_opts_valid)
        # a directory to store output.
        set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/${_doc_prefix}")
        set(_output_dir_stamp "${_output_dir}/dir.stamp")
        
        # set default sgml file if not specified
        set(_default_xml_file "${_output_dir}/${_doc_prefix}-docs.xml")
        get_filename_component(_default_xml_file ${_default_xml_file} ABSOLUTE)
        
        # a directory to store html output.
        set(_output_html_dir "${_output_dir}/html")
        set(_output_html_dir_stamp "${_output_dir}/html_dir.stamp")

        # The output files
        set(_output_decl_list "${_output_dir}/${_doc_prefix}-decl-list.txt")
        set(_output_decl "${_output_dir}/${_doc_prefix}-decl.txt")
        set(_output_overrides "${_output_dir}/${_doc_prefix}-overrides.txt")
        set(_output_sections "${_output_dir}/${_doc_prefix}-sections.txt")
        set(_output_types "${_output_dir}/${_doc_prefix}.types")

        set(_output_signals "${_output_dir}/${_doc_prefix}.signals")

        set(_output_unused "${_output_dir}/${_doc_prefix}-unused.txt")
        set(_output_undeclared "${_output_dir}/${_doc_prefix}-undeclared.txt")
        set(_output_undocumented "${_output_dir}/${_doc_prefix}-undocumented.txt")
        set(_output_tmpl_dir "${_output_dir}/tmpl")
        set(_output_tmpl_stamp "${_output_dir}/tmpl.stamp")

        set(_output_xml_dir "${_output_dir}/xml")
        set(_output_sgml_stamp "${_output_dir}/sgml.stamp")

        set(_output_html_stamp "${_output_dir}/html.stamp")

        # add a command to create output directory
        add_custom_command(
            OUTPUT "${_output_dir_stamp}" "${_output_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_output_dir}"
            COMMAND ${CMAKE_COMMAND} -E touch ${_output_dir_stamp}
            VERBATIM)

        set(_ignore_headers_opt "")
        if(ARG_IGNOREHEADERS)
            set(_ignore_headers_opt "--ignore-headers=")
            foreach(_header ${ARG_IGNOREHEADERS})
                set(_ignore_headers_opt "${_ignore_headers_opt}${_header} ")
            endforeach(_header ${ARG_IGNOREHEADERS})
        endif(ARG_IGNOREHEADERS)

        # add a command to scan the input
        add_custom_command(
            OUTPUT
                "${_output_decl_list}"
                "${_output_decl}"
                "${_output_decl}.bak"
                "${_output_overrides}"
                "${_output_sections}"
                "${_output_types}"
                "${_output_types}.bak"
            DEPENDS
                "${_output_dir}"
                ${ARG_DEPENDS}
            COMMAND ${GTKDOC_SCAN_EXE}
                "--module=${_doc_prefix}"
                "${_ignore_headers_opt}"
                "--rebuild-sections"
                "--rebuild-types"
                "--source-dir=${_doc_sourcedir}"
                ${ARG_EXTRAHEADERS}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        # add a command to scan the input via gtkdoc-scangobj
        # This is such a disgusting hack!
        add_custom_command(
            OUTPUT
                "${_output_signals}"
            DEPENDS
                "${_output_types}"
                "${ARG_DEPENDS}"
            COMMAND ${CMAKE_COMMAND} 
                -D "GTKDOC_SCANGOBJ_EXE:STRING=${GTKDOC_SCANGOBJ_EXE}"
                -D "doc_prefix:STRING=${_doc_prefix}"
                -D "output_types:STRING=${_output_types}"
                -D "output_dir:STRING=${_output_dir}"
                -D "type_init_func:STRING=${ARG_TYPEINITFUNC}"
                -D "EXTRA_CFLAGS:STRING=${ARG_CFLAGS}"
                -D "EXTRA_LDFLAGS:STRING=${ARG_LDFLAGS}"
                -D "EXTRA_LDPATH:STRING=${ARG_LDPATH}"
                -P ${GTKDOC_SCANGOBJ_WRAPPER}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        # add a command to make the templates
        add_custom_command(
            OUTPUT
                "${_output_unused}"
                "${_output_undeclared}"
                "${_output_undocumented}"
                "${_output_tmpl_dir}"
                "${_output_tmpl_stamp}"
            DEPENDS
                "${_output_types}"
                "${_output_signals}"
                "${_output_sections}"
                "${_output_overrides}"
                ${ARG_DEPENDS}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${_output_tmpl_dir}
            COMMAND ${GTKDOC_MKTMPL_EXE}
                "--module=${_doc_prefix}"
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        set(_copy_xml_if_needed "")
        if(ARG_XML)
            get_filename_component(ARG_XML ${ARG_XML} ABSOLUTE)
            set(_copy_xml_if_needed 
                COMMAND ${CMAKE_COMMAND} -E copy "${ARG_XML}" "${_default_xml_file}")
        endif(ARG_XML)

        set(_remove_xml_if_needed "")
        if(ARG_XML)
            set(_remove_xml_if_needed 
                COMMAND ${CMAKE_COMMAND} -E remove ${_default_xml_file})
        endif(ARG_XML)

        # add a command to make the database
        add_custom_command(
            OUTPUT
                "${_output_sgml_stamp}"
                "${_default_xml_file}"
            DEPENDS
                "${_output_tmpl_stamp}"
                "${_output_unused}"
                "${_output_undeclared}"
                "${_output_undocumented}"
                ${ARG_DEPENDS}
                ${_remove_xml_if_needed}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${_output_xml_dir}
            COMMAND ${GTKDOC_MKDB_EXE}
                "--module=${_doc_prefix}"
                "--source-dir=${_doc_sourcedir}"
                "--source-suffixes=${_doc_source_suffixes}"
                "--output-format=xml"
                "--main-sgml-file=${_default_xml_file}"
            ${_copy_xml_if_needed}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        # add a command to create html directory
        add_custom_command(
            OUTPUT "${_output_html_dir_stamp}" "${_output_html_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_output_html_dir}
            COMMAND ${CMAKE_COMMAND} -E touch ${_output_html_dir_stamp}
            VERBATIM)

        # add a command to output HTML
        add_custom_command(
            OUTPUT
                "${_output_html_stamp}" 
            DEPENDS
                "${_output_html_dir_stamp}"
                "${_output_sgml_stamp}"
                "${_output_tmpl_stamp}"
                "${ARG_XML}"
                ${ARG_DEPENDS}
            ${_copy_xml_if_needed}
            COMMAND ${GTKDOC_MKHTML_EXE}
                "${_doc_prefix}"
                "${_default_xml_file}"
            WORKING_DIRECTORY "${_output_html_dir}"
            VERBATIM)

        # fix output refs
        add_custom_target("${_doc_prefix}-gtxdoc-fixxref" 
            DEPENDS
                "${_output_html_stamp}"
                ${ARG_DEPENDS}
            COMMAND ${GTKDOC_FIXXREF_EXE}
                "--module=${_doc_prefix}"
                "--module-dir=."
                ${ARG_FIXXREFOPTS}
            #${_remove_xml_if_needed}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        add_custom_target(doc-${_doc_prefix} ${_do_all} 
            DEPENDS
                "${_doc_prefix}-gtxdoc-fixxref"
                ${ARG_DEPENDS})

        add_test(doc-${_doc_prefix}-check ${GTKDOC_CHECK_EXE})
        set_tests_properties(doc-${_doc_prefix}-check PROPERTIES
          ENVIRONMENT "DOC_MODULE=${_doc_prefix};DOC_MAIN_SGML_FILE=${_doc_prefix}-docs.xml;SRCDIR=${_doc_sourcedir};BUILDDIR=${_output_dir}"
        )
    endif(_opts_valid)
endfunction(gtk_doc_add_module)
