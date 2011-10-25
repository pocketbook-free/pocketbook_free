# Process this file with automake to create Makefile.in.

SUBDIRS=libgail-util gail tests docs po

EXTRA_DIST =			\
	MAINTAINERS		\
	gail-uninstalled.pc.in	\
	gail.pc.in		\
	gail-zip.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= gail.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DISTCLEANFILES =		\
	gail-unistalled.pc	\
	gail.pc
