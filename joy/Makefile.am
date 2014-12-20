lib_LTLIBRARIES = libjoy.la
libjoy_la_SOURCES = joy-marshallers.h joystick.c joy-marshallers.c joystick.h
pkginclude_HEADERS = joystick.h
libjoy_la_CPPFLAGS = @CFLAGS@ @GOBJECT_CFLAGS@ @UDEV_CFLAGS@
libjoy_la_LIBADD = @GOBJECT_LIBS@ @UDEV_LIBS@
libjoy_gtk_la_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
libjoy_gtk_la_LIBADD = @GTK_LIBS@
libjoy_gtk_la_SOURCES = joymodel.c joymodel.h
EXTRA_DIST = gmarshal.list joy-marshallers.c joy-marshallers.h
MAINTAINERCLEANFILES = joy-marshallers.h joy-marshallers.c
joy-marshallers.c: gmarshal.list
	glib-genmarshal --body --prefix=joy_cclosure_marshal < $^ > $@
joy-marshallers.h: gmarshal.list
	glib-genmarshal --header --prefix=joy_cclosure_marshal < $^ > $@
if GTK_ON
bin_PROGRAMS = joytest
lib_LTLIBRARIES += libjoy-gtk.la
joytest_SOURCES = joytest.c joytest-iface.h
joytest_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
joytest_LDADD = libjoy.la @GTK_LIBS@ libjoy-gtk.la

joytest-iface.h: joytest.glade gen.sh
	$(srcdir)/gen.sh JOYTEST_IFACE_H JOYTEST_IFACE < $^ > $@
endif
