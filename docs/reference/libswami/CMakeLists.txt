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
    marshals.h
    swami_priv.h
    SwamiState_types.h
)

if (GTKDOC_FOUND)
  include (UseGtkDoc)

  gtk_doc_add_module (libswami ${CMAKE_SOURCE_DIR}/src/libswami
    XML libswami-docs.xml
    SUFFIXES c h
    IGNOREHEADERS ${ignore_headers}
    EXTRAHEADERS ${CMAKE_BINARY_DIR}/src/libswami/version.h
    TYPEINITFUNC swami_init\(\)
    CFLAGS -I${CMAKE_SOURCE_DIR}/src
    LDFLAGS -L${CMAKE_BINARY_DIR}/src/libswami -lswami
    LDPATH ${CMAKE_BINARY_DIR}/src/libswami
    DEPENDS libswami
  )
endif ()

