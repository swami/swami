## Process this file with automake to produce Makefile.in
bin_PROGRAMS = riff_dump

riff_dump_SOURCES = riff_dump.c
riff_dump_LDADD = @GOBJECT_LIBS@ \
  $(top_srcdir)/libinstpatch/libinstpatch-1.0.la -lm

INCLUDES = -I$(top_srcdir) @GOBJECT_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in
