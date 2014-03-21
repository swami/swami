## Process this file with automake to produce Makefile.in
EXTRA_DIST = create_sf2.py

noinst_PROGRAMS = create_sf2

create_sf2_SOURCES = create_sf2.c
create_sf2_LDADD = $(top_builddir)/libinstpatch/libinstpatch-1.0.la @GOBJECT_LIBS@

AM_CPPFLAGS = -I$(top_srcdir) @GOBJECT_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in
