lib_LTLIBRARIES = libgjs.la
libgjs_la_SOURCES = gjs-marshallers.h gjsjoystick.c gjs-marshallers.c gjsjoystick.h
#gjsjoystick_o_DEPENDENCIES = gjs-marshallers.h
pkginclude_HEADERS = gjsjoystick.h
libgjs_la_CPPFLAGS = @CFLAGS@ @GOBJECT_CFLAGS@
libgjs_la_LIBADD = @GOBJECT_LIBS@
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = gmarshal.list
DISTCLEANFILES = gjs-marshallers.h gjs-marshallers.c
gjs-marshallers.c: gmarshal.list
	glib-genmarshal --body --prefix=gjs_cclosure_marshal < $^ > $@
gjs-marshallers.h: gmarshal.list
	glib-genmarshal --header --prefix=gjs_cclosure_marshal < $^ > $@
bin_PROGRAMS = gjstest
gjstest_SOURCES = gjstest.c
gjstest_CPPFLAGS = @CFLAGS@ @GOBJECT_CFLAGS@
gjstest_LDADD = libgjs.la @GOBJECT_LIBS@
