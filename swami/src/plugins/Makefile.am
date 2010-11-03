## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined -export-symbols
fluidsynth_la_def = fluidsynth.def
endif


EXTRA_DIST = \
	fftune.c \
	fftune.h \
	fftune_gui.c \
	fftune_gui.h \
	fftune_i18n.h \
	fluidsynth.c \
	fluidsynth.glade \
	fluidsynth_i18n.c \
	fluidsynth_i18n.h \
	fluidsynth_gui.c \
	fluidsynth_gui_i18n.h

if FLUIDSYNTH_SUPPORT
fluidsynth_lib = fluidsynth_plugin.la
fluidsynth_gui_lib = fluidsynth_gui.la
endif

if FFTW_SUPPORT
fftune_lib = fftune.la
fftune_gui_lib = fftune_gui.la
endif

pkglib_LTLIBRARIES = $(fluidsynth_lib) $(fluidsynth_gui_lib) \
	$(fftune_lib) $(fftune_gui_lib)

fluidsynth_plugin_la_SOURCES = fluidsynth.c fluidsynth_i18n.h
fluidsynth_plugin_la_CFLAGS = @FLUIDSYNTH_CFLAGS@ @LIBINSTPATCH_CFLAGS@ @GUI_CFLAGS@
fluidsynth_plugin_la_LDFLAGS =  $(no_undefined) $(fluidsynth_la_def) \
    -module -avoid-version @FLUIDSYNTH_LIBS@ @GUI_LIBS@ \
    ../libswami/libswami.la

fluidsynth_gui_la_SOURCES = fluidsynth_gui.c fluidsynth_gui_i18n.h
fluidsynth_gui_la_CFLAGS = @GUI_CFLAGS@ @LIBINSTPATCH_CFLAGS@
fluidsynth_gui_la_LDFLAGS = -module -avoid-version @GUI_LIBS@ @LIBINSTPATCH_LIBS@

fftune_la_SOURCES = fftune.c fftune.h fftune_i18n.h
fftune_la_CFLAGS = @FFTW_CFLAGS@ @LIBINSTPATCH_CFLAGS@
fftune_la_LDFLAGS = -module -avoid-version
fftune_la_LIBADD = @FFTW_LIBS@ @LIBINSTPATCH_LIBS@

fftune_gui_la_SOURCES = fftune_gui.c fftune_gui.h fftune_i18n.h
fftune_gui_la_CFLAGS = @GUI_CFLAGS@ @LIBINSTPATCH_CFLAGS@
fftune_gui_la_LDFLAGS = -module -avoid-version @GUI_LIBS@ @LIBINSTPATCH_LIBS@

INCLUDES = -I$(top_srcdir)/src/libinstpatch -I$(top_srcdir)/src \
	-I$(top_srcdir)/intl -DLOCALEDIR=\"$(datadir)/locale\"

MAINTAINERCLEANFILES = Makefile.in
