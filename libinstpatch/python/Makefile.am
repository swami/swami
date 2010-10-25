# Path to PyGtk defs utilities
H2DEF=$(PYGTK_CODEGEN_DIR)/h2def.py

AUTOMAKE_OPTIONS=1.5

INCLUDES = -I$(top_srcdir) $(PYTHON_CFLAGS) $(PYGTK_CFLAGS) $(GOBJECT_CFLAGS)

if PYTHON_SUPPORT
ipatchmodule_cond = ipatchmodule.la
DEFPATH=`pkg-config --variable=defsdir pygtk-2.0`

defsdir = $(DEFPATH)
defs_DATA = ipatch.defs ipatch-types.defs
endif

EXTRA_DIST = ipatch.override ipatch.defs ipatch-types.defs ipatchmodule.c

pyexec_LTLIBRARIES = $(ipatchmodule_cond)
ipatchmodule_la_LDFLAGS = -module -avoid-version -export-symbols-regex \
    initipatch
ipatchmodule_la_LIBADD = ../libinstpatch/libinstpatch-1.0.la @PYGTK_LIBS@ \
    @GOBJECT_LIBS@
ipatchmodule_la_SOURCES = ipatchmodule.c
nodist_ipatchmodule_la_SOURCES = ipatch.c
ipatch.c: ipatch.defs ipatch-types.defs ipatch.override

CLEANFILES = ipatch.c

.defs.c:
	(cd $(srcdir)\
	  && pygtk-codegen-2.0 \
	  --register $(DEFPATH)/gdk-types.defs \
	  --register $(DEFPATH)/gtk-types.defs \
	  --register ipatch-types.defs \
	  --override $*.override \
	  --prefix py$* $*.defs) > gen-$*.c \
	  && cp gen-$*.c $*.c \
	  && rm -f gen-$*.c

# Auto generation of defs file for merging into ipatch.defs
# To be run manually to aid the developer in adding new bindings
ipatch-auto.defs:
	(cd $(srcdir)\
          && $(PYTHON) ${H2DEF} `cat ../libinstpatch/libinstpatch.h | grep "^#include [<]libinstpatch" | sed 's/#include <libinstpatch\///;s/>//;s/^/\.\.\/libinstpatch\//'`) \
  | sed 's/DL_S2/DLS2/g;s/IpatchSf2/IpatchSF2/g;s/IpatchDls2/IpatchDLS2/g;s/Izone/IZone/g;s/Pzone/PZone/g;s/IpatchDlsFile/IpatchDLSFile/g;s/IpatchDlsReader/IpatchDLSReader/g;s/IpatchDlsWriter/IpatchDLSWriter/g' \
  > ipatch-auto.defs

.PHONY: ipatch-auto.defs
