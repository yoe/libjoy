lib_LTLIBRARIES = libjoy-1.0.la
libjoy_1_0_la_SOURCES = joy-marshallers.h joy-marshallers.c joystick.h joystick.c
pkginclude_HEADERS = joystick.h
libjoy_1_0_la_CPPFLAGS = @CFLAGS@ @GOBJECT_CFLAGS@ @UDEV_CFLAGS@ -I$(top_srcdir)
libjoy_1_0_la_LIBADD = @GOBJECT_LIBS@ @UDEV_LIBS@
libjoy_gtk_1_0_la_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
libjoy_gtk_1_0_la_LIBADD = @GTK_LIBS@ @UDEV_LIBS@ libjoy-1.0.la
libjoy_gtk_1_0_la_SOURCES = joymodel.c joymodel.h
EXTRA_DIST = gmarshal.list joy-marshallers.c joy-marshallers.h
DISTCLEANFILES = joy-marshallers.h joy-marshallers.c
BUILT_SOURCES = joy-marshallers.c joy-marshallers.h joytest-iface.h
joy-marshallers.c: gmarshal.list
	glib-genmarshal --body --prefix=joy_cclosure_marshal < $^ > $@
joy-marshallers.h: gmarshal.list
	glib-genmarshal --header --prefix=joy_cclosure_marshal < $^ > $@
if GTK_ON
bin_PROGRAMS = joytest
lib_LTLIBRARIES += libjoy-gtk-1.0.la
joytest_SOURCES = joytest.c joytest-iface.h
joytest_CPPFLAGS = @CFLAGS@ @GTK_CFLAGS@
joytest_LDADD = libjoy-1.0.la @GTK_LIBS@ libjoy-gtk-1.0.la

joytest-iface.h: joytest.glade gen.sh
	$(srcdir)/gen.sh JOYTEST_IFACE_H JOYTEST_IFACE < $^ > $@
endif
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
Joy-1.0.gir: libjoy-1.0.la
Joy_1_0_gir_INCLUDES = GObject-2.0
Joy_1_0_gir_CFLAGS = $(libjoy_1_0_la_CPPFLAGS)
Joy_1_0_gir_LIBS = libjoy-1.0.la
Joy_1_0_gir_FILES = $(libjoy_1_0_la_SOURCES)
INTROSPECTION_GIRS += Joy-1.0.gir
if GTK_ON
Joy_1_0_gir_LIBS += libjoy-gtk-1.0.la
Joy_1_0_gir_FILES += $(libjoy_gtk_1_0_la_SOURCES)
Joy_1_0_gir_CFLAGS += $(libjoy_gtk_1_0_la_CPPFLAGS)
endif

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
CLEANFILES = $(gir_DATA) $(typelib_DATA)
endif
