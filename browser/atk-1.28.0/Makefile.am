# Process this file with automake to create Makefile.in.

SUBDIRS=atk tests docs po

EXTRA_DIST = 			\
	MAINTAINERS		\
	atk-uninstalled.pc.in	\
	atk.pc.in		\
	atk.spec.in		\
	atk.spec		\
	atk-zip.sh.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = atk.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DISTCLEANFILES =		\
	atk-uninstalled.pc	\
	atk.pc			\
	atk.pc.spec		\
	atk-zip.sh
