#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(libjoy,
	m4_esyscmd([build-aux/git-version-gen --prefix libjoy- .tarbazll-version]),
	[w@uter.be])
AC_CONFIG_SRCDIR([joy/joystick.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign dist-xz])
AM_MAINTAINER_MODE([enable])
LT_INIT
GTK_DOC_CHECK([1.14], [--flavour no-tmpl])
GOBJECT_INTROSPECTION_CHECK([1.30.0])

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.
PKG_CHECK_MODULES(GOBJECT, [gobject-2.0 >= 2.36])
AC_ARG_WITH([gtk], AS_HELP_STRING([--with-gtk], [enable the graphical joystick tester and Gtk+ helper library (requires Gtk+3)]))
AS_IF([test "x$with_gtk" = "xyes"], [
	PKG_CHECK_MODULES(GTK, [gtk+-3.0])
])
AM_CONDITIONAL([GTK_ON], [test "x$with_gtk" = "xyes"])

PKG_CHECK_MODULES(UDEV, [libudev])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile joy/Makefile docs/Makefile docs/joy-docs.xml])
AC_OUTPUT
