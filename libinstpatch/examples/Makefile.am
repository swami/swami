## Process this file with automake to produce Makefile.in
EXTRA_DIST = create_sf2.py

noinst_PROGRAMS = create_sf2

create_sf2_SOURCES = create_sf2.c
create_sf2_LDADD = $(top_srcdir)/libinstpatch/libinstpatch-1.0.la

INCLUDES = -I$(top_srcdir) @GOBJECT_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in
