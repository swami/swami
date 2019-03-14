## Process this file with automake to produce Makefile.in

SUBDIRS = m4 src po docs package

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh \
	ABOUT-NLS \
	CVS-HOWTO \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	swami.desktop \
	swami.png \
	swami.svg \
	swami.xml

desktopdir = $(datadir)/applications
desktop_DATA = swami.desktop

mimexmldir = $(datadir)/mime/packages
mimexml_DATA = swami.xml

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = swami.png

appsvgdir = $(datadir)/icons/hicolor/scalable/apps
appsvg_DATA = swami.svg

DISTCLEANFILES = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

rpms: dist
	cd package && ./rpmpkg.sh ../${PACKAGE}-${VERSION}.tar.gz

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc


gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
                $(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
                echo "***   $(gtk_update_icon_cache)"; \
	fi


UPDATE_MIME = \
  if [ -f $(DESTDIR)$(datadir)/mime/packages/freedesktop.org.xml ] ; then \
    if which update-mime-database>/dev/null 2>&1; then \
      update-mime-database $(DESTDIR)$(datadir)/mime; \
    fi \
  fi

UPDATE_DESKTOP = \
  if [ -f $(DESTDIR)$(datadir)/applications/defaults.list ] ; then \
    if which update-desktop-database>/dev/null 2>&1 ; then \
      update-desktop-database; \
    fi \
  fi

install-data-hook: update-icon-cache
	$(UPDATE_MIME)
	$(UPDATE_DESKTOP)

uninstall-hook: update-icon-cache
	$(UPDATE_MIME)
	$(UPDATE_DESKTOP)
