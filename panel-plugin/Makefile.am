plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libhamster.la

libhamster_la_SOURCES =				\
	hamster.c hamster.h				\
	windowserver.c windowserver.h	\
	plugin.c								\
	button.c								\
	view.c view.h						\
	util.c util.h						\
	settings.c settings.h



libhamster_la_CFLAGS =							\
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
	$(LIBXFCE4PANEL_LIBS)						\
	$(LIBXFCONF_LIBS)						\
	$(LIBX11_LIBS)							

libhamster_la_LDFLAGS = \
	-avoid-version \
	-module \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = hamster.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
