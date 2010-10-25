## Process this file with automake to create Makefile.in

# We build a static libtool convenience library

noinst_LTLIBRARIES = libwidgets.la
libwidgets_la_SOURCES = \
	combo-box.c \
	combo-box.h \
	icon-combo.c \
	icon-combo.h

libwidgets_la_LDFLAGS = -static
INCLUDES = -I$(top_srcdir)/src/swamigui @GUI_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in
