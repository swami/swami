# Path to PyGtk defs utilities
H2DEF=$(PYGTK_CODEGEN_DIR)/h2def.py
MERGEDEFS=$(PYGTK_CODEGEN_DIR)/mergedefs.py

AUTOMAKE_OPTIONS=1.5

AM_CPPFLAGS = $(PYTHON_CFLAGS) $(PYGTK_CFLAGS) -I$(top_srcdir)/src \
    $(GOBJECT_CFLAGS) $(LIBINSTPATCH_CFLAGS) $(GTK_CFLAGS)

if PYTHON_SUPPORT
swamimodule_cond = swamimodule.la swamiguimodule.la
DEFPATH := $(shell pkg-config --variable=defsdir pygtk-2.0)

defsdir = $(DEFPATH)
defs_DATA = swami.defs swamigui.defs
endif

EXTRA_DIST = swami.override swami.defs swamimodule.c \
  swamigui.override swamigui.defs swamiguimodule.c

pyexec_LTLIBRARIES = $(swamimodule_cond)

swamimodule_la_LDFLAGS = -module -avoid-version -export-symbols-regex \
    initswami
swamimodule_la_LIBADD = ../libswami/libswami.la @PYGTK_LIBS@ \
    @GOBJECT_LIBS@ @LIBINSTPATCH_LIBS@
swamimodule_la_SOURCES = swamimodule.c swami_missing.c swami_missing.h
nodist_swamimodule_la_SOURCES = swami.c
swami.c: swami.defs swami.override

swamiguimodule_la_LDFLAGS = -module -avoid-version -export-symbols-regex \
    initswamigui
swamiguimodule_la_LIBADD = \
    ../swamigui/libswamigui.la ../libswami/libswami.la \
    @PYGTK_LIBS@ @GOBJECT_LIBS@ @LIBINSTPATCH_LIBS@ \
    @GUI_LIBS@ @GTK_SOURCE_VIEW_LIBS@
swamiguimodule_la_CFLAGS = @GUI_CFLAGS@ @GTK_SOURCE_VIEW_CFLAGS@
swamiguimodule_la_SOURCES = swamiguimodule.c swamigui_missing.c \
  swamigui_missing.h
nodist_swamiguimodule_la_SOURCES = swamigui.c
swamigui.c: swamigui.defs swamigui.override


CLEANFILES = swami.c swamigui.c

swami.c:
	(cd $(srcdir)\
	  && pygtk-codegen-2.0 \
	  --register $(DEFPATH)/ipatch.defs \
	  --register $(DEFPATH)/ipatch-types.defs \
	  --override swami.override \
	  --prefix pyswami swami.defs) > gen-swami.c \
	  && cp gen-swami.c swami.c \
	  && rm -f gen-swami.c

swamigui.c:
	(cd $(srcdir)\
	  && pygtk-codegen-2.0 \
	  --register $(DEFPATH)/ipatch.defs \
	  --register $(DEFPATH)/ipatch-types.defs \
	  --register $(DEFPATH)/gtk-base.defs \
	  --register $(DEFPATH)/gtk-base-types.defs \
	  --register $(DEFPATH)/gdk-base.defs \
	  --register $(DEFPATH)/gdk-base-types.defs \
	  --override swamigui.override \
	  --prefix pyswamigui swamigui.defs) > gen-swamigui.c \
	  && cp gen-swamigui.c swamigui.c \
	  && rm -f gen-swamigui.c

swami.defs:
	(cd $(srcdir)\
          && $(PYTHON) ${H2DEF} `cat ../libswami/libswami.h | grep "^#include [<]libswami" | sed 's/#include <libswami\///;s/>//;s/^/\.\.\/libswami\//'`) > swami.defs

swamigui.defs:
	(cd $(srcdir)\
          && $(PYTHON) ${H2DEF} `cat ../swamigui/swamigui.h | grep "^#include [<]swamigui" | sed 's/#include <swamigui\///;s/>//;s/^/\.\.\/swamigui\//'`) > swamigui.defs

# Not merging old defs currently just re-generating
#	(cd $(srcdir) && cp -f swami.defs swami.defs.bak \
#	 && $(PYTHON) ${MERGEDEFS} swami.defs.gen swami.defs.bak) \
#         >swami.defs
