#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xfce4-hamster-plugin], [1.10.0], [hakan@erduman.de])
AM_INIT_AUTOMAKE([silent-rules])
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([panel-plugin/plugin.c])
AC_CONFIG_HEADERS([config.h])

GETTEXT_PACKAGE=xfce4-hamster-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the Gettext package name])
AC_DEFINE_UNQUOTED(PLUGIN_NAME, "$PACKAGE", [Define plugin name])


AC_ARG_ENABLE(debug,[  --enable-debug          disable debug support (default: disable)],[enable_debug=$enableval],[enable_debug=no])
if test x$enable_debug = xyes; then
    AC_DEFINE([DEBUG], [1], [Set this to 1 to enable debug output.])
fi
AC_ARG_ENABLE(debug_trace,[  --enable-debug-trace    disable trace support (default: disable)],[enable_debug_trace=$enableval],[enable_debug_trace=no])
if test x$enable_debug_trace = xyes; then
    AC_DEFINE([DEBUG_TRACE], [1], [Set this to 1 to enable full trace debugging.])
fi

# Checks for programs.
AC_PROG_CC
#AC_PROG_INSTALL()
AC_PROG_INTLTOOL()
AC_SUBST(XGETTEXT_ARGS)

AC_CONFIG_FILES([Makefile panel-plugin/Makefile po/Makefile.in])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GIO], [gio-2.0])
PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0])
PKG_CHECK_MODULES([GMODULE], [gmodule-2.0])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([LIBXFCE4UTIL], [libxfce4util-1.0])
PKG_CHECK_MODULES([LIBXFCE4UI], [libxfce4ui-2])
PKG_CHECK_MODULES([LIBXFCE4PANEL], [libxfce4panel-2.0])
PKG_CHECK_MODULES([LIBXFCONF], [libxfconf-0])


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
