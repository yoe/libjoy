#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(libgjs, 0.1, w@uter.be)
AC_CONFIG_SRCDIR([gjsjoystick.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE([enable])

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.
LT_INIT
PKG_CHECK_MODULES(GOBJECT, [gobject-2.0 >= 2.36])
AC_ARG_WITH([gtk], AS_HELP_STRING([--with-gtk], [enable the graphical joystick tester (requires Gtk+ 2 or above)]))
AS_IF([test "x$with_gtk" = "xyes"], [
	PKG_CHECK_MODULES(GTK, [gtk+-2.0])
])
AM_CONDITIONAL([GTK_ON], [test "x$with_gtk" = "xyes"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
