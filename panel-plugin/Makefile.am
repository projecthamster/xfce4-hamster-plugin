plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libhamster.la

GENERATED_CODE = \
	hamster.c hamster.h				\
	windowserver.c windowserver.h

THIRD_PARTY_CODE = \
	button.c button.h

OWN_CODE = \
	plugin.c						\
	view.c view.h					\
	util.c util.h					\
	settings.c settings.h

libhamster_la_SOURCES = $(GENERATED_CODE) $(THIRD_PARTY_CODE) $(OWN_CODE)



libhamster_la_CFLAGS =	-Wall			\
	-I$(top_builddir)						\
	-I$(top_srcdir)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(LIBX11_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCE4UI_CFLAGS)						\
	$(LIBXFCE4GUI_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(LIBXFCONF_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libhamster_la_LIBADD =							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(GTK_LIBS)							\
	$(LIBX11_LDFLAGS)						\
	$(LIBX11_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCE4UI_LIBS)						\
	$(LIBXFCE4GUI_LIBS)						\
	$(LIBXFCE4PANEL_LIBS)						\
	$(LIBXFCONF_LIBS)						\
	$(LIBX11_LIBS)							

libhamster_la_LDFLAGS = \
	-avoid-version \
	-module \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'

#
# xfce4-popup-hamsterymenu script
#
bin_SCRIPTS = \
	xfce4-popup-hamstermenu

xfce4-popup-hamstermenu: xfce4-popup-hamstermenu.sh Makefile
	$(AM_V_GEN) $(SED) -e "s,\@bindir\@,$(bindir),g" \
	-e "s,\@localedir\@,$(localedir),g" $< >$@

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = hamster.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-hook:
	sed -i 's:@ICON_NAME@:$(with_icon_name):g' $(datadir)/xfce4/panel/plugins/hamster.desktop

EXTRA_DIST = hamster.desktop.in

distclean-local:
	rm -f hamster.desktop

splint:
	splint -weak -stats -badflag \
	$(OWN_CODE) ../config.h \
	$(libhamster_la_CFLAGS)


# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
