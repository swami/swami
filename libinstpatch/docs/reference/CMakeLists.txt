# 
# libInstPatch
#
# Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
#
# See COPYING license file for distribution details
#

set (ignore_headers
    IpatchConvert_Gig.h
    IpatchConvert_SF2.h
    IpatchConvert_DLS2.h
    IpatchConverter_priv.h
    IpatchSF2VoiceCache_DLS.h
    IpatchSF2VoiceCache_Gig.h
    IpatchSF2VoiceCache_SF2.h
    IpatchSF2VoiceCache_VBank.h
    IpatchState.h
    IpatchStateGroup.h
    IpatchStateItem.h
    IpatchState_types.h
    builtin_enums.h
    compat.h
    config.h
    i18n.h
    ipatch_priv.h
    libinstpatch.h
    marshals.h
)

if (GTKDOC_FOUND)
  include (UseGtkDoc)

  gtk_doc_add_module (libinstpatch ${CMAKE_SOURCE_DIR}/libinstpatch
    XML libinstpatch-docs.xml
    SUFFIXES c h
    IGNOREHEADERS ${ignore_headers}
    EXTRAHEADERS ${CMAKE_CURRENT_BINARY_DIR}/../../libinstpatch/version.h
    TYPEINITFUNC ipatch_init\(\)
    CFLAGS -I${CMAKE_SOURCE_DIR}
    LDFLAGS -L${CMAKE_CURRENT_BINARY_DIR}/../../libinstpatch -linstpatch-1.0
    LDPATH ${CMAKE_CURRENT_BINARY_DIR}/../../libinstpatch
    DEPENDS instpatch-1.0
  )
endif ()

