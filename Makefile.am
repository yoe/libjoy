lib_LTLIBRARIES = libjoy.la
libjoy_la_SOURCES = joy-marshallers.h libjoy.c joy-marshallers.c libjoy.h
pkginclude_HEADERS = libjoy.h
libjoy_la_CPPFLAGS = @CFLAGS@ @GOBJECT_CFLAGS@
libjoy_la_LIBADD = @GOBJECT_LIBS@
libjoy_gtk_la_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
libjoy_gtk_la_LIBADD = @GTK_LIBS@
libjoy_gtk_la_SOURCES = libjoy-gtk.c libjoy-gtk.h
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = gmarshal.list
DISTCLEANFILES = joy-marshallers.h joy-marshallers.c
joy-marshallers.c: gmarshal.list
	glib-genmarshal --body --prefix=joy_cclosure_marshal < $^ > $@
gjs-marshallers.h: gmarshal.list
	glib-genmarshal --header --prefix=joy_cclosure_marshal < $^ > $@
if GTK_ON
bin_PROGRAMS = joytest
lib_LTLIBRARIES += libjoy-gtk.la
joytest_SOURCES = joytest.c
joytest_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
joytest_LDADD = libjoy.la @GTK_LIBS@ libjoy-gtk.la
endif
