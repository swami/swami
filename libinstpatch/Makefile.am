## Process this file with automake to produce Makefile.in

SUBDIRS = m4 libinstpatch python utils examples po docs tests
EXTRA_DIST = \
	autogen.sh \
	ABOUT-NLS \
	config.guess \
	config.sub \
	CVS-HOWTO \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libinstpatch-1.0.pc

rpms: dist
	cd package && ./rpmpkg.sh ../${PACKAGE}-${VERSION}.tar.gz

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
