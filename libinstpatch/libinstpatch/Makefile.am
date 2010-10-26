## Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = libinstpatch-1.0.la

if PLATFORM_WIN32
no_undefined = -no-undefined -export-symbols
libinstpatch_la_def = libinstpatch.def
endif

if OS_WIN32
install-libtool-import-lib:
	$(INSTALL) .libs/libinstpatch-1.0.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libinstpatch-1.0.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

BUILT_SOURCES = \
	builtin_enums.c \
	builtin_enums.h \
	marshals.c \
	marshals.h

# correctly clean the generated source files
CLEANFILES = $(BUILT_SOURCES)

# extra files and optional sources
EXTRA_DIST = \
	marshals.list \
	version.h.in \
	libinstpatch.def

ipatch_public_h_sources = \
	IpatchBase.h \
	IpatchContainer.h \
	IpatchConverter.h \
	IpatchConverterSF2VoiceCache.h \
	IpatchConvert_DLS2.h \
	IpatchConvert_Gig.h \
	IpatchConvert_SF2.h \
	IpatchDLS2Conn.h \
	IpatchDLS2.h \
	IpatchDLS2Info.h \
	IpatchDLS2Inst.h \
	IpatchDLS2Region.h \
	IpatchDLS2Sample.h \
	IpatchDLSFile.h \
	IpatchDLSReader.h \
	IpatchDLSWriter.h \
	IpatchFile.h \
	IpatchGig.h \
	IpatchGigDimension.h \
	IpatchGigEffects.h \
	IpatchGigFile.h \
	IpatchGigInst.h \
	IpatchGigRegion.h \
	IpatchGigSample.h \
	IpatchGigSubRegion.h \
	IpatchItem.h \
	IpatchIter.h \
	IpatchList.h \
	IpatchParamProp.h \
	IpatchPaste.h \
	IpatchRange.h \
	IpatchRiff.h \
	IpatchSample.h \
	IpatchSampleData.h \
	IpatchSampleList.h \
	IpatchSampleStore.h \
	IpatchSampleStoreCache.h \
	IpatchSampleStoreFile.h \
	IpatchSampleStoreRam.h \
	IpatchSampleStoreRom.h \
	IpatchSampleStoreSndFile.h \
	IpatchSampleStoreSplit24.h \
	IpatchSampleStoreSwap.h \
	IpatchSampleStoreVirtual.h \
	IpatchSampleTransform.h \
	IpatchSF2.h \
	IpatchSF2File.h \
	IpatchSF2Gen.h \
	IpatchSF2GenItem.h \
	IpatchSF2Inst.h \
	IpatchSF2IZone.h \
	IpatchSF2Mod.h \
	IpatchSF2ModItem.h \
	IpatchSF2Preset.h \
	IpatchSF2PZone.h \
	IpatchSF2Reader.h \
	IpatchSF2Sample.h \
	IpatchSF2VoiceCache.h \
	IpatchSF2VoiceCache_DLS.h \
	IpatchSF2VoiceCache_SF2.h \
	IpatchSF2VoiceCache_Gig.h \
	IpatchSF2VoiceCache_VBank.h \
	IpatchSF2Writer.h \
	IpatchSF2Zone.h \
	IpatchSndFile.h \
	IpatchTypeProp.h \
	IpatchUnit.h \
	IpatchUnit_generic.h \
	IpatchUnit_DLS.h \
	IpatchUnit_SF2.h \
	IpatchVBank.h \
	IpatchVBankInst.h \
	IpatchVBankRegion.h \
	IpatchVirtualContainer.h \
	IpatchVirtualContainer_types.h \
	IpatchXml.h \
	IpatchXmlObject.h \
	misc.h \
	sample.h \
	util.h \
	version.h

ipatch_sources = \
	$(BUILT_SOURCES) \
	IpatchBase.c \
	IpatchContainer.c \
	IpatchContainer_notify.c \
	IpatchConverter.c \
	IpatchConverterSF2VoiceCache.c \
	IpatchConverter_priv.h \
	IpatchConvert_DLS2.c \
	IpatchConvert_Gig.c \
	IpatchConvert_SF2.c \
	IpatchDLS2.c \
	IpatchDLS2Conn.c \
	IpatchDLS2Info.c \
	IpatchDLS2Inst.c \
	IpatchDLS2Param_tables.c \
	IpatchDLS2Region.c \
	IpatchDLS2Sample.c \
	IpatchDLSFile.c \
	IpatchDLSReader.c \
	IpatchDLSWriter.c \
	IpatchFile.c \
	IpatchFileBuf.c \
	IpatchGig.c \
	IpatchGigDimension.c \
	IpatchGigEffects.c \
	IpatchGigFile.c \
	IpatchGigInst.c \
	IpatchGigRegion.c \
	IpatchGigSample.c \
	IpatchGigSubRegion.c \
	IpatchItem.c \
	IpatchItemProp.c \
	IpatchIter.c \
	IpatchList.c \
	IpatchParamProp.c \
	IpatchPaste.c \
	IpatchRange.c \
	IpatchRiff.c \
	IpatchSample.c \
	IpatchSampleData.c \
	IpatchSampleList.c \
	IpatchSampleStore.c \
	IpatchSampleStoreCache.c \
	IpatchSampleStoreFile.c \
	IpatchSampleStoreRam.c \
	IpatchSampleStoreRom.c \
	IpatchSampleStoreSndFile.c \
	IpatchSampleStoreSplit24.c \
	IpatchSampleStoreSwap.c \
	IpatchSampleStoreVirtual.c \
	IpatchSampleTransform.c \
	IpatchSF2.c \
	IpatchSF2File.c \
	IpatchSF2Gen.c \
	IpatchSF2GenItem.c \
	IpatchSF2Gen_tables.c \
	IpatchSF2Inst.c \
	IpatchSF2IZone.c \
	IpatchSF2Mod.c \
	IpatchSF2ModItem.c \
	IpatchSF2Preset.c \
	IpatchSF2PZone.c \
	IpatchSF2Reader.c \
	IpatchSF2Sample.c \
	IpatchSF2VoiceCache.c \
	IpatchSF2VoiceCache_DLS.c \
	IpatchSF2VoiceCache_SF2.c \
	IpatchSF2VoiceCache_Gig.c \
	IpatchSF2VoiceCache_VBank.c \
	IpatchSF2Writer.c \
	IpatchSF2Zone.c \
	IpatchSndFile.c \
	IpatchTypeProp.c \
	IpatchUnit.c \
	IpatchUnit_generic.c \
	IpatchUnit_DLS.c \
	IpatchUnit_SF2.c \
	IpatchVBank.c \
	IpatchVBankInst.c \
	IpatchVBankRegion.c \
	IpatchVirtualContainer.c \
	IpatchVirtualContainer_types.c \
	IpatchXml.c \
	IpatchXmlObject.c \
	i18n.h \
	ipatch_priv.h \
	md5.c \
	md5.h \
	misc.c \
	sample.c \
	util.c

libinstpatch_1_0_la_SOURCES = $(ipatch_public_h_sources) $(ipatch_sources)
libinstpatch_1_0_la_LDFLAGS = $(no_undefined) $(libinstpatch_la_def) \
  -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-dynamic
libinstpatch_1_0_la_LIBADD = @SNDFILE_LIBS@ @GOBJECT_LIBS@ -lm

ipatchincdir = $(includedir)/libinstpatch-1.0/libinstpatch
ipatchinc_HEADERS = $(ipatch_public_h_sources) builtin_enums.h libinstpatch.h

INCLUDES = @GOBJECT_CFLAGS@ @SNDFILE_CFLAGS@ -I.. -DLIBINSTPATCH_COMPILATION \
   -DG_LOG_DOMAIN=\"libInstPatch\" -DLOCALEDIR=\"$(datadir)/locale\"

marshals.c: marshals.list
	echo "/* Autogenerated file (add to marshals.list and 'make update-marshals') */" >marshals.c
	glib-genmarshal --body --prefix=ipatch_marshal \
	  marshals.list >>marshals.c

marshals.h: marshals.list
	echo "/* Autogenerated file (add to marshals.list and 'make update-marshals') */" >marshals.h
	glib-genmarshal --header --prefix=ipatch_marshal \
	  marshals.list >>marshals.h

builtin_enums.c: s-builtin-enums-c
	@true
s-builtin-enums-c: $(ipatch_public_h_sources) Makefile
	( cd $(srcdir) && glib-mkenums \
	  --fhead "#include \"libinstpatch.h\"\n#include \"ipatch_priv.h\"" \
	  --fprod "\n/* enumerations from \"@filename@\" */" \
	  --vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
	  --vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
	  $(ipatch_public_h_sources) ) \
        | sed 's/dl_s2/dls2/g;s/DL_S2/DLS2/g' \
        > xgen-enumc && cp xgen-enumc $(srcdir)/builtin_enums.c  \
	&& rm -f xgen-enumc

builtin_enums.h: s-builtin-enums-h
	@true
s-builtin-enums-h: $(ipatch_public_h_sources) Makefile
	( cd $(srcdir) && glib-mkenums \
	  --fhead "#ifndef __IPATCH_BUILTIN_ENUMS_H__\n#define __IPATCH_BUILTIN_ENUMS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
	  --fprod "/* enumerations from \"@filename@\" */\n" \
	  --vhead "GType @enum_name@_get_type (void);\n#define IPATCH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
	  --ftail "G_END_DECLS\n\n#endif /* __IPATCH_BUILTIN_ENUMS_H__ */" \
	  $(ipatch_public_h_sources) ) \
        | sed 's/dl_s2/dls2/g;s/DL_S2/DLS2/g' \
	> xgen-enumh && (cmp -s xgen-enumh $(srcdir)/builtin_enums.h \
                         || cp xgen-enumh $(srcdir)/builtin_enums.h ) \
	&& rm -f xgen-enumh

install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib
