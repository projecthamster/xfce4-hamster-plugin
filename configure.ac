#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xfce4-hamster-plugin], [0.0.1], [hakan@erduman.de])
AM_INIT_AUTOMAKE([silent-rules])
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([panel-plugin/hamster.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
#AC_PROG_INSTALL()
AC_PROG_INTLTOOL()

AC_CONFIG_FILES([Makefile panel-plugin/Makefile po/Makefile.in])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.22.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.22.0])
PKG_CHECK_MODULES([GMODULE], [gmodule-2.0 >= 2.22.0])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.22.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.22.0])
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.14.0])
PKG_CHECK_MODULES([LIBXFCE4UTIL], [libxfce4util-1.0 >= 4.8.0])
PKG_CHECK_MODULES([LIBXFCE4UI], [libxfce4ui-1 >= 4.8.0])
PKG_CHECK_MODULES([LIBXFCE4PANEL], [libxfce4panel-1.0 >= 4.8.0])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
