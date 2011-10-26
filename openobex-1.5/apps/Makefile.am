
if APPS
noinst_LIBRARIES = libmisc.a

libmisc_a_SOURCES = \
	obex_io.c obex_io.h \
	obex_put_common.c obex_put_common.h

bin_PROGRAMS = irxfer obex_tcp irobex_palm3 obex_test

obex_test_SOURCES = \
	obex_test.c obex_test.h \
	obex_test_client.c obex_test_client.h \
	obex_test_server.c obex_test_server.h \
	obex_test_cable.c obex_test_cable.h

obex_test_LDADD = libmisc.a $(top_builddir)/lib/libopenobex.la @BLUETOOTH_LIBS@

AM_CFLAGS = @OPENOBEX_CFLAGS@

LDADD = libmisc.a $(top_builddir)/lib/libopenobex.la

INCLUDES = @BLUETOOTH_CFLAGS@ -I$(top_srcdir)/include
endif

EXTRA_DIST = CMakeLists.txt

MAINTAINERCLEANFILES = Makefile.in
