# 
# libInstPatch
#
# Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
#
# See COPYING license file for distribution details
#

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

# ************ library ************

set ( instpatch_public_HEADERS
    IpatchBase.h
    IpatchContainer.h
    IpatchConverter.h
    IpatchConverterSF2VoiceCache.h
    IpatchConvert_DLS2.h
    IpatchConvert_Gig.h
    IpatchConvert_SF2.h
    IpatchConvert_SLI.h
    IpatchDLS2Conn.h
    IpatchDLS2.h
    IpatchDLS2Info.h
    IpatchDLS2Inst.h
    IpatchDLS2Region.h
    IpatchDLS2Sample.h
    IpatchDLSFile.h
    IpatchDLSReader.h
    IpatchDLSWriter.h
    IpatchFile.h
    IpatchGig.h
    IpatchGigDimension.h
    IpatchGigEffects.h
    IpatchGigFile.h
    IpatchGigInst.h
    IpatchGigRegion.h
    IpatchGigSample.h
    IpatchGigSubRegion.h
    IpatchItem.h
    IpatchIter.h
    IpatchList.h
    IpatchParamProp.h
    IpatchPaste.h
    IpatchRange.h
    IpatchRiff.h
    IpatchSample.h
    IpatchSampleData.h
    IpatchSampleList.h
    IpatchSampleStore.h
    IpatchSampleStoreCache.h
    IpatchSampleStoreFile.h
    IpatchSampleStoreRam.h
    IpatchSampleStoreRom.h
    IpatchSampleStoreSndFile.h
    IpatchSampleStoreSplit24.h
    IpatchSampleStoreSwap.h
    IpatchSampleStoreVirtual.h
    IpatchSampleTransform.h
    IpatchSF2.h
    IpatchSF2File.h
    IpatchSF2Gen.h
    IpatchSF2GenItem.h
    IpatchSF2Inst.h
    IpatchSF2IZone.h
    IpatchSF2Mod.h
    IpatchSF2ModItem.h
    IpatchSF2ModList.h
    IpatchSF2Preset.h
    IpatchSF2PZone.h
    IpatchSF2Reader.h
    IpatchSF2Sample.h
    IpatchSF2VoiceCache.h
    IpatchSF2VoiceCache_DLS.h
    IpatchSF2VoiceCache_SF2.h
    IpatchSF2VoiceCache_SLI.h
    IpatchSF2VoiceCache_Gig.h
    IpatchSF2VoiceCache_VBank.h
    IpatchSF2Writer.h
    IpatchSF2Zone.h
    IpatchSLI.h
    IpatchSLIFile.h
    IpatchSLIInst.h
    IpatchSLIReader.h
    IpatchSLISample.h
    IpatchSLIWriter.h
    IpatchSLIZone.h
    IpatchSndFile.h
    IpatchTypeProp.h
    IpatchUnit.h
    IpatchUnit_generic.h
    IpatchUnit_DLS.h
    IpatchUnit_SF2.h
    IpatchVBank.h
    IpatchVBankInst.h
    IpatchVBankRegion.h
    IpatchVirtualContainer.h
    IpatchVirtualContainer_types.h
    IpatchXml.h
    IpatchXmlObject.h
    misc.h
    sample.h
    util.h
) 

set ( instpatch_private_HEADERS
    IpatchDLSFile_priv.h
    IpatchGigFile_priv.h
    IpatchSF2File_priv.h
    IpatchSLIFile_priv.h
    compat.h
)

set ( instpatch_SOURCES
    IpatchBase.c
    IpatchContainer.c
    IpatchContainer_notify.c
    IpatchConverter.c
    IpatchConverterSF2VoiceCache.c
    IpatchConverter_priv.h
    IpatchConvert_DLS2.c
    IpatchConvert_Gig.c
    IpatchConvert_SF2.c
    IpatchConvert_SLI.c
    IpatchDLS2.c
    IpatchDLS2Conn.c
    IpatchDLS2Info.c
    IpatchDLS2Inst.c
    IpatchDLS2Region.c
    IpatchDLS2Sample.c
    IpatchDLSFile.c
    IpatchDLSReader.c
    IpatchDLSWriter.c
    IpatchFile.c
    IpatchFileBuf.c
    IpatchGig.c
    IpatchGigDimension.c
    IpatchGigEffects.c
    IpatchGigFile.c
    IpatchGigInst.c
    IpatchGigRegion.c
    IpatchGigSample.c
    IpatchGigSubRegion.c
    IpatchItem.c
    IpatchItemProp.c
    IpatchIter.c
    IpatchList.c
    IpatchParamProp.c
    IpatchPaste.c
    IpatchRange.c
    IpatchRiff.c
    IpatchSample.c
    IpatchSampleData.c
    IpatchSampleList.c
    IpatchSampleStore.c
    IpatchSampleStoreCache.c
    IpatchSampleStoreFile.c
    IpatchSampleStoreRam.c
    IpatchSampleStoreRom.c
    IpatchSampleStoreSndFile.c
    IpatchSampleStoreSplit24.c
    IpatchSampleStoreSwap.c
    IpatchSampleStoreVirtual.c
    IpatchSampleTransform.c
    IpatchSF2.c
    IpatchSF2File.c
    IpatchSF2Gen.c
    IpatchSF2GenItem.c
    IpatchSF2Gen_tables.c
    IpatchSF2Inst.c
    IpatchSF2IZone.c
    IpatchSF2Mod.c
    IpatchSF2ModItem.c
    IpatchSF2ModList.c
    IpatchSF2Preset.c
    IpatchSF2PZone.c
    IpatchSF2Reader.c
    IpatchSF2Sample.c
    IpatchSF2VoiceCache.c
    IpatchSF2VoiceCache_DLS.c
    IpatchSF2VoiceCache_SF2.c
    IpatchSF2VoiceCache_SLI.c
    IpatchSF2VoiceCache_Gig.c
    IpatchSF2VoiceCache_VBank.c
    IpatchSF2Writer.c
    IpatchSF2Zone.c
    IpatchSLI.c
    IpatchSLIFile.c
    IpatchSLIInst.c
    IpatchSLIInst_CatMaps.c
    IpatchSLIReader.c
    IpatchSLISample.c
    IpatchSLIWriter.c
    IpatchSLIZone.c
    IpatchSndFile.c
    IpatchTypeProp.c
    IpatchUnit.c
    IpatchUnit_generic.c
    IpatchUnit_DLS.c
    IpatchUnit_SF2.c
    IpatchVBank.c
    IpatchVBankInst.c
    IpatchVBankRegion.c
    IpatchVirtualContainer.c
    IpatchVirtualContainer_types.c
    IpatchXml.c
    IpatchXmlObject.c
    compat.c
    i18n.h
    md5.c
    md5.h
    misc.c
    sample.c
    util.c
)


set ( public_main_HEADER ${CMAKE_CURRENT_BINARY_DIR}/libinstpatch.h )

link_directories (
    ${GOBJECT_LIBDIR}
    ${GOBJECT_LIBRARY_DIRS}
    ${SNDFILE_LIBDIR}
    ${SNDFILE_LIBRARY_DIRS}
)

add_definitions ( -DLOCALEDIR="${DATA_INSTALL_DIR}/locale" )

add_library ( instpatch-1.0
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${instpatch_SOURCES}
    ipatch_priv.h
)

target_link_libraries ( instpatch-1.0
    ${GOBJECT_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${IPATCH_LIBS}
)

install ( FILES
          ${instpatch_public_HEADERS}
          ${public_main_HEADER}
          ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
          ${CMAKE_CURRENT_BINARY_DIR}/version.h
          DESTINATION include/libinstpatch-1.0/libinstpatch )

find_program (GLIB2_MKENUMS glib-mkenums)
find_program (SED sed)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#ifndef __IPATCH_BUILTIN_ENUMS_H__\\n\"
    --fhead \"\#define __IPATCH_BUILTIN_ENUMS_H__\\n\\n\"
    --fhead \"\#include <glib-object.h>\\n\\n\"
    --fhead \"G_BEGIN_DECLS\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\\n\"
    --vhead \"GType @enum_name@_get_type \(void\)\;\\n\"
    --vhead \"\#define IPATCH_TYPE_@ENUMSHORT@ \(@enum_name@_get_type\(\)\)\\n\"
    --ftail \"G_END_DECLS\\n\\n\"
    --ftail \"\#endif /* __IPATCH_BUILTIN_ENUMS_H__ */\"
    ${instpatch_public_HEADERS}
    | ${SED} 's/dl_s2/dls2/g\;s/DL_S2/DLS2/g'
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${instpatch_public_HEADERS}
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --fhead \"\#include \\"libinstpatch.h\\"\\n\"
    --fhead \"\#include \\"ipatch_priv.h\\"\\n\\n\"
    --fprod \"/* enumerations from \\"@filename@\\" */\"
    --vhead \"static const G@Type@Value _@enum_name@_values[] = {\"
    --vprod \"  { @VALUENAME@, \\"@VALUENAME@\\", \\"@valuenick@\\" },\"
    --vtail \"  { 0, NULL, NULL }\\n}\;\\n\\n\"
    --vtail \"GType\\n@enum_name@_get_type \(void\)\\n{\\n\"
    --vtail \"  static GType type = 0\;\\n\\n\"
    --vtail \"  if \(G_UNLIKELY \(type == 0\)\)\\n\"
    --vtail \"    type = g_\@type\@_register_static \(\\"@EnumName@\\", _@enum_name@_values\)\;\\n\\n\"
    --vtail \"  return type\;\\n}\\n\\n\"
    ${instpatch_public_HEADERS}
    | ${SED} 's/dl_s2/dls2/g\;s/DL_S2/DLS2/g'
    > ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${instpatch_public_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
)


find_program (GLIB2_GENMARSHAL glib-genmarshal)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --body --prefix=ipatch_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
)

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  COMMAND ${GLIB2_GENMARSHAL}
  ARGS
    --header --prefix=ipatch_marshal
    ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list >${CMAKE_CURRENT_BINARY_DIR}/marshals.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshals.list
)

if ( MACOSX_FRAMEWORK )
     set_property ( SOURCE ${instpatch_public_HEADERS} 
         PROPERTY MACOSX_PACKAGE_LOCATION Headers/libinstpatch
     )
    set_target_properties ( instpatch-1.0
      PROPERTIES
        OUTPUT_NAME "libinstpatch-1.0"
        FRAMEWORK TRUE
        PUBLIC_HEADER "${public_main_HEADER}"
        FRAMEWORK_VERSION "${LIB_VERSION_CURRENT}"
        INSTALL_NAME_DIR ${FRAMEWORK_INSTALL_DIR}
        VERSION ${LIB_VERSION_INFO}
        SOVERSION ${LIB_VERSION_CURRENT}
    )
else ( MACOSX_FRAMEWORK )
  set_target_properties ( instpatch-1.0
    PROPERTIES
      PREFIX "lib"
      OUTPUT_NAME "instpatch-1.0"
      VERSION ${LIB_VERSION_INFO}
      SOVERSION ${LIB_VERSION_CURRENT}
  )
endif ( MACOSX_FRAMEWORK )

if ( IPATCH_CPPFLAGS )
  set_target_properties ( instpatch-1.0
    PROPERTIES COMPILE_FLAGS ${IPATCH_CPPFLAGS} )
endif ( IPATCH_CPPFLAGS ) 

install ( TARGETS instpatch-1.0
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${LIB_SUFFIX}
  FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
  BUNDLE DESTINATION ${BUNDLE_INSTALL_DIR}
)

macro(_list_prefix _outvar _listvar _prefix)
  set(${_outvar})
  foreach(_item IN LISTS ${_listvar})
    list(APPEND ${_outvar} ${_prefix}${_item})
  endforeach()
endmacro(_list_prefix)

# GObject Introspection
if (INTROSPECTION_FOUND)

  # libInstPatch has to be initialized, so we build an introspection program which does this
  add_executable ( gir_prog gir_prog.c )

  target_link_libraries ( gir_prog
    instpatch-1.0
    ${GOBJECT_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${IPATCH_LIBS}
    ${INTROSPECTION_LIBRARIES}
  )

  string (REPLACE ";" " " INTROSPECTION_CFLAGS_STR "${INTROSPECTION_CFLAGS}")
  set_target_properties (gir_prog
                         PROPERTIES COMPILE_FLAGS "${INTROSPECTION_CFLAGS_STR}")

  include(GObjectIntrospectionMacros)
  set(INTROSPECTION_GIRS)
  set(INTROSPECTION_SCANNER_ARGS "--add-include-path=${CMAKE_CURRENT_SOURCE_DIR}")
  set(INTROSPECTION_COMPILER_ARGS "--includedir=${CMAKE_CURRENT_SOURCE_DIR}")

  set(introspection_files ${instpatch_SOURCES} ${instpatch_public_HEADERS})
  set(Ipatch_1_1_gir instpatch-1.0)
  set(Ipatch_1_1_gir_INCLUDES GObject-2.0)
  set(Ipatch_1_1_gir_LIBS instpatch-1.0)
  get_directory_property(_tmp_includes INCLUDE_DIRECTORIES)
  _list_prefix(_includes _tmp_includes "-I")
  set(Ipatch_1_1_gir_CFLAGS ${_includes})
  set(Ipatch_1_1_gir_PROGRAM ${CMAKE_CURRENT_BINARY_DIR}/gir_prog)
  _list_prefix(_abs_introspection_files introspection_files "${CMAKE_CURRENT_SOURCE_DIR}/")
  list(APPEND _abs_introspection_files
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.c
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_enums.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
  )
  set(Ipatch_1_1_gir_NAMESPACE Ipatch)
  set(Ipatch_1_1_gir_VERSION 1.1)
  set(Ipatch_1_1_gir_FILES ${_abs_introspection_files})
  set(Ipatch_1_1_gir_SCANNERFLAGS --c-include ${public_main_HEADER})

  list(APPEND INTROSPECTION_GIRS Ipatch-1.1.gir)

  gir_add_introspections(INTROSPECTION_GIRS)

  add_dependencies (gir-girs gir_prog)  # Add introspection program as a dependency of gir-girs
endif (INTROSPECTION_FOUND)

