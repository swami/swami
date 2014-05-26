# 
# Swami
#
# Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
#
# See COPYING license file for distribution details
#

set (ignore_headers
    builtin_enums.h
    i18n.h
    swamigui.h
    swamigui_priv.h
    SwamiguiPythonView.h
)

if (GTKDOC_FOUND)
  include (UseGtkDoc)

  gtk_doc_add_module (swamigui ${CMAKE_SOURCE_DIR}/src/swamigui
    XML swamigui-docs.xml
    SUFFIXES c h
    IGNOREHEADERS ${ignore_headers}
    TYPEINITFUNC swamigui_init\(0,NULL\)
    CFLAGS -I${CMAKE_SOURCE_DIR}/src
    LDFLAGS -L${CMAKE_BINARY_DIR}/src/swamigui -lswamigui
    LDPATH ${CMAKE_BINARY_DIR}/src/swamigui
    DEPENDS libswamigui
  )
endif ()

