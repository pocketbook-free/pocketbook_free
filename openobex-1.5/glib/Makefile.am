
includedir = @includedir@/openobex

if GLIB
include_HEADERS = obex-client.h

lib_LTLIBRARIES = libopenobex-glib.la

libopenobex_glib_la_SOURCES = obex-lowlevel.h obex-lowlevel.c obex-client.c obex-debug.h obex-error.c obex-error.h

libopenobex_glib_la_LDFLAGS = -no-undefined -version-info 1:0:0

libopenobex_glib_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/lib/libopenobex.la @EXTRA_LIBS@

BUILT_SOURCES = obex-marshal.h obex-marshal.c

nodist_libopenobex_glib_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

noinst_PROGRAMS = test-lowlevel test-client

test_lowlevel_LDADD = libopenobex-glib.la

test_client_LDADD = @GLIB_LIBS@ libopenobex-glib.la
endif

AM_CFLAGS = @GLIB_CFLAGS@

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = obex-marshal.list CMakeLists.txt

MAINTAINERCLEANFILES = Makefile.in

if GLIB
obex-marshal.h: obex-marshal.list
	$(GLIB_GENMARSHAL) --prefix=obex_marshal $< --header > $@

obex-marshal.c: obex-marshal.list
	$(GLIB_GENMARSHAL) --prefix=obex_marshal $< --body > $@
endif
