
lib_LTLIBRARIES = libopenobex.la

libopenobex_la_SOURCES = \
	obex.c \
	obex_main.c obex_main.h \
	obex_connect.c obex_connect.h \
	obex_header.c obex_header.h \
	obex_object.c obex_object.h \
	obex_transport.c obex_transport.h \
	obex_server.c obex_server.h \
	obex_client.c obex_client.h \
	irobex.c irobex.h \
	inobex.c inobex.h \
	btobex.c btobex.h \
	databuffer.c databuffer.h \
	irda.h irda_wrap.h \
	usbobex.c usbobex.h \
	bluez_compat.h

libopenobex_la_CFLAGS = @CFLAG_VISIBILITY@

libopenobex_la_LDFLAGS = \
	-no-undefined \
	-version-info 5:1:4 \
	-export-symbols $(top_srcdir)/lib/obex.sym

libopenobex_la_LIBADD = @USB_LIBS@ @EXTRA_LIBS@

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = obex.sym win32compat.c CMakeLists.txt openobex.rc.in

MAINTAINERCLEANFILES = Makefile.in
