EXTRA_DIST = gailutil.def
if OS_WIN32
export_symbols = -export-symbols gailutil.def
no_undefined = -no-undefined

install-libtool-import-lib:
	$(INSTALL) .libs/libgailutil.dll.a $(DESTDIR)$(libdir)
uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgailutil.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gailutil.lib

install-ms-lib:
	$(INSTALL) gailutil.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gailutil.lib
else
install-ms-lib:
uninstall-ms-lib:
endif


lib_LTLIBRARIES = libgailutil.la

util_c_sources =		\
	gailmisc.c		\
	gailtextutil.c

libgailutilincludedir=$(includedir)/gail-1.0/libgail-util

util_public_h_sources =		\
	gailmisc.h		\
	gailtextutil.h		\
	gail-util.h

libgailutil_la_SOURCES = 	\
	$(util_c_sources)

libgailutilinclude_HEADERS =	\
	$(util_public_h_sources)

INCLUDES = \
	-I$(top_srcdir)		 \
	-DVERSION=\"$(VERSION)\" \
	@DEP_CFLAGS@

libgailutil_la_LIBADD = \
	@DEP_LIBS@

libgailutil_la_LDFLAGS =    \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(export_symbols)					\
	$(no_undefined)						\
        @LDFLAGS@

gailutil.lib: libgailutil.la gailutil.def
	lib -name:libgailutil-@LT_CURRENT_MINUS_AGE@.dll -def:gailutil.def -out:$@

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

