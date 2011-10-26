
SUBDIRS = include lib glib apps ircp doc

pkgconfigdir = $(libdir)/pkgconfig

if GLIB
pkgconfig_DATA = openobex.pc openobex-glib.pc
else
pkgconfig_DATA = openobex.pc
endif

DISTCHECK_CONFIGURE_FLAGS = --enable-apps

EXTRA_DIST = openobex.m4 CMakeLists.txt \
		CMakeModules/FindBluetooth.cmake CMakeModules/FindGlib.cmake \
		CMakeModules/FindIrda.cmake CMakeModules/FindLibUSB.cmake

DISTCLEANFILES = $(pkgconfig_DATA)

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp missing install-sh mkinstalldirs
