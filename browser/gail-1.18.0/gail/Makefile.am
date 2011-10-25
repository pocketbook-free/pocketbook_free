if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libgail.la

gail_c_sources =			\
	gail-private-macros.h		\
	gail.c				\
	gailadjustment.c		\
	gailarrow.c			\
	gailbooleancell.c		\
	gailbooleancellfactory.c	\
	gailbox.c			\
	gailbutton.c			\
	gailcalendar.c			\
	gailcanvas.c			\
	gailcanvasgroup.c		\
	gailcanvasgroupfactory.c	\
	gailcanvasitem.c		\
	gailcanvasitemfactory.c		\
	gailcanvastext.c		\
	gailcanvastextfactory.c		\
	gailcanvaswidget.c		\
	gailcanvaswidgetfactory.c	\
	gailcell.c			\
	gailcellparent.c		\
	gailcheckmenuitem.c		\
	gailchecksubmenuitem.c		\
	gailclist.c			\
	gailclistcell.c			\
	gailcombo.c			\
	gailcombobox.c			\
	gailcontainer.c			\
	gailcontainercell.c		\
	gailentry.c			\
	gailexpander.c			\
	gailframe.c			\
	gailimage.c			\
	gailimagecell.c			\
	gailimagecellfactory.c		\
	gailitem.c			\
	gaillabel.c			\
	gaillist.c			\
	gailmenu.c			\
	gailmenushell.c			\
	gailmenuitem.c			\
	gailnotebook.c			\
	gailnotebookpage.c		\
	gailobject.c			\
	gailobjectfactory.c		\
	gailoptionmenu.c		\
	gailpaned.c			\
	gailpixmap.c			\
	gailprogressbar.c		\
	gailradiobutton.c		\
	gailradiomenuitem.c		\
	gailradiosubmenuitem.c		\
	gailrange.c			\
	gailrenderercell.c		\
	gailrenderercellfactory.c	\
	gailscale.c			\
	gailscrollbar.c			\
	gailscrolledwindow.c		\
	gailseparator.c			\
	gailspinbutton.c		\
	gailsubmenuitem.c		\
	gailstatusbar.c			\
	gailtextcell.c			\
	gailtextcellfactory.c		\
	gailtextview.c			\
	gailtogglebutton.c		\
	gailtoplevel.c			\
	gailtreeview.c			\
	gailutil.c			\
	gailwidget.c			\
	gailwindow.c

libgailincludedir=$(includedir)/gail-1.0/gail

gail_private_h_sources =		\
	gail.h				\
	gailadjustment.h		\
	gailarrow.h			\
	gailbooleancell.h		\
	gailbooleancellfactory.h	\
	gailbox.h			\
	gailbutton.h			\
	gailcalendar.h			\
	gailcanvas.h			\
	gailcanvasgroup.h		\
	gailcanvasgroupfactory.h	\
	gailcanvasitem.h		\
	gailcanvasitemfactory.h		\
	gailcanvastext.h		\
	gailcanvastextfactory.h		\
	gailcanvaswidget.h		\
	gailcanvaswidgetfactory.h	\
	gailcell.h			\
	gailcellparent.h		\
	gailcheckmenuitem.h		\
	gailchecksubmenuitem.h		\
	gailclist.h			\
	gailclistcell.h			\
	gailcombo.h			\
	gailcombobox.h			\
	gailcontainercell.h		\
	gailcontainer.h			\
	gailentry.h			\
	gailexpander.h			\
	gailfactory.h			\
	gailframe.h			\
	gailimage.h			\
	gailimagecell.h			\
	gailimagecellfactory.h		\
	gailintl.h			\
	gailitem.h			\
	gaillabel.h			\
	gaillist.h			\
	gailmenu.h			\
	gailmenushell.h			\
	gailmenuitem.h			\
	gailnotebook.h			\
	gailnotebookpage.h		\
	gailobject.h			\
	gailobjectfactory.h		\
	gailoptionmenu.h		\
	gailpaned.h			\
	gailpixmap.h			\
	gailprogressbar.h		\
	gailradiobutton.h		\
	gailradiomenuitem.h		\
	gailradiosubmenuitem.h		\
	gailrange.h			\
	gailrenderercell.h		\
	gailrenderercellfactory.h	\
	gailscale.h			\
	gailscrollbar.h			\
	gailscrolledwindow.h		\
	gailseparator.h			\
	gailspinbutton.h		\
	gailsubmenuitem.h		\
	gailstatusbar.h			\
	gailtextcell.h			\
	gailtextcellfactory.h		\
	gailtextview.h			\
	gailtogglebutton.h		\
	gailtoplevel.h			\
	gailtreeview.h			\
	gailutil.h			\
	gailwindow.h

gail_public_h_sources =	\
	gailwidget.h

libgail_la_SOURCES = 			\
	$(gail_c_sources)		\
	$(gail_public_h_sources)	\
	$(gail_private_h_sources)

libgailinclude_HEADERS =		\
	$(gail_public_h_sources)

INCLUDES = \
	-I$(top_srcdir)		 \
	-DVERSION=\"$(VERSION)\" \
	@DEP_CFLAGS@

libgail_la_LIBADD =     \
        @DEP_LIBS@	\
	$(INTLLIBS)

moduledir = $(libdir)/gtk-2.0/modules

libgail_la_LDFLAGS =    \
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined)	\
        @LDFLAGS@	\
	$(top_builddir)/libgail-util/libgailutil.la
