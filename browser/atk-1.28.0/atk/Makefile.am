# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH.
GPATH = $(srcdir)

if PLATFORM_WIN32
no_undefined = -no-undefined
else
no_undefined =
endif

INCLUDES = \
	-I$(top_srcdir)		\
	-DG_DISABLE_DEPRECATED	\
	-DATK_DISABLE_DEPRECATED	\
	-DATK_COMPILATION	\
	-DATK_LOCALEDIR="\"$(datadir)/locale\""	\
	$(DEP_CFLAGS)

lib_LTLIBRARIES = libatk-1.0.la

libatk_1_0_la_LDFLAGS = 			\
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)
libatk_1_0_la_LIBADD = $(DEP_LIBS)

libatk_1_0_la_SOURCES = 	\
	atkaction.c		\
	atkcomponent.c		\
	atkdocument.c		\
	atkeditabletext.c	\
	atkgobjectaccessible.c	\
	atkhyperlink.c		\
	atkhyperlinkimpl.c	\
	atkhypertext.c		\
	atkimage.c		\
	atknoopobject.c		\
	atknoopobjectfactory.c	\
	atkobject.c		\
	atkobjectfactory.c	\
	atkregistry.c		\
	atkrelation.c		\
	atkrelationset.c	\
	atkselection.c		\
	atkstate.c		\
	atkstateset.c		\
	atkstreamablecontent.c	\
	atktable.c		\
	atktext.c		\
	atkutil.c		\
	atkmisc.c		\
	atkvalue.c		\
	atk-enum-types.c

libatkincludedir=$(includedir)/atk-1.0/atk

atk_headers = \
        atk.h			\
        atkaction.h		\
        atkcomponent.h		\
        atkdocument.h		\
        atkeditabletext.h	\
        atkgobjectaccessible.h	\
        atkhyperlink.h		\
        atkhyperlinkimpl.h	\
        atkhypertext.h		\
        atknoopobject.h		\
        atknoopobjectfactory.h	\
        atkobject.h		\
	atkobjectfactory.h	\
        atkimage.h		\
        atkregistry.h		\
        atkrelation.h		\
        atkrelationtype.h	\
        atkrelationset.h	\
        atkselection.h		\
        atkstate.h		\
        atkstateset.h		\
        atkstreamablecontent.h	\
        atktable.h		\
        atktext.h		\
        atkutil.h		\
        atkmisc.h		\
        atkvalue.h

libatkinclude_HEADERS =		\
	$(atk_headers)		\
	atk-enum-types.h


# ---------- Handle built sources ----------

BUILT_SOURCES = atkmarshal.h atkmarshal.c atk-enum-types.h atk-enum-types.c

atkmarshal.h: stamp-atkmarshal.h
	@true
stamp-atkmarshal.h: @REBUILD@ atkmarshal.list
	$(GLIB_GENMARSHAL) --prefix=atk_marshal $(srcdir)/atkmarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh atkmarshal.h || cp xgen-gmh atkmarshal.h) \
	&& rm -f xgen-gmh xgen-gmh~					\
	&& echo timestamp > $(@F)

atkmarshal.c: stamp-atkmarshal.c
	@true
stamp-atkmarshal.c: @REBUILD@ atkmarshal.list
	$(GLIB_GENMARSHAL) --prefix=atk_marshal $(srcdir)/atkmarshal.list --body >> xgen-gmc \
	&& (cmp -s xgen-gmc atkmarshal.c || cp xgen-gmc atkmarshal.c) \
	&& rm -f xgen-gmc xgen-gmc~					\
	&& echo timestamp > $(@F)

atk-enum-types.h: s-enum-types-h
	@true
s-enum-types-h: @REBUILD@ $(atk_headers) Makefile
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#if defined(ATK_DISABLE_SINGLE_INCLUDES) && !defined (__ATK_H_INSIDE__) && !defined (ATK_COMPILATION)\n#error \"Only <atk/atk.h> can be included directly.\"\n#endif\n\n#ifndef __ATK_ENUM_TYPES_H__\n#define __ATK_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define ATK_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n"         \
			--ftail "G_END_DECLS\n\n#endif /* __ATK_ENUM_TYPES_H__ */" \
		$(atk_headers) ) > tmp-atk-enum-types.h \
	&& (cmp -s tmp-atk-enum-types.h atk-enum-types.h || cp tmp-atk-enum-types.h atk-enum-types.h ) \
	&& rm -f tmp-atk-enum-types.h \
	&& echo timestamp > $(@F)

atk-enum-types.c: s-enum-types-c
	@true
s-enum-types-c: @REBUILD@ $(atk_headers) Makefile
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include <atk.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {"     \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(atk_headers) ) > tmp-atk-enum-types.c \
	&& (cmp -s tmp-atk-enum-types.c atk-enum-types.c || cp tmp-atk-enum-types.c atk-enum-types.c ) \
	&& rm -f tmp-atk-enum-types.c \
	&& echo timestamp > $(@F)

# ---------- Win32 stuff ----------

if OS_WIN32
libatk_1_0_la_LDFLAGS += -export-symbols $(srcdir)/atk.def -no-undefined -Wl,atk-win32-res.o
libatk_1_0_la_DEPENDENCIES = atk-win32-res.o atk.def

install-def-file:
	$(INSTALL) $(srcdir)/atk.def $(DESTDIR)$(libdir)/atk-1.0.def
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/atk-1.0.def
else
install-def-file:
uninstall-def-file:
endif

atk-win32-res.o: atk.rc
	$(WINDRES) $< $@

if MS_LIB_AVAILABLE
noinst_DATA = atk-$(ATK_API_VERSION).lib

install-ms-lib:
	$(INSTALL) atk-$(ATK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/atk-$(ATK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

atk.def: atk.symbols
	(echo -e EXPORTS; $(CPP) -P -DINCLUDE_VARIABLES -DG_OS_WIN32 -DALL_FILES - <$(srcdir)/atk.symbols | sed -e '/^$$/d' -e 's/^/	/' -e 's/G_GNUC_[^ ]*//g') > atk.def

atk-$(ATK_API_VERSION).lib: libatk-$(ATK_API_VERSION).la atk.def
	lib -machine:$(LIB_EXE_MACHINE_FLAG) -name:libatk-$(ATK_API_VERSION)-$(LT_CURRENT_MINUS_AGE).dll -def:atk.def -out:$@


EXTRA_DIST = atk.symbols atk.rc.in atkmarshal.list atkintl.h

DISTCLEANFILES = \
  stamp-atkmarshal.h stamp-atkmarshal.c \
  s-enum-types-h s-enum-types-c

distclean-local:
	if test $(srcdir) = .; then :; else \
	  rm -f atkmarshal.h atkmarshal.c atk-enum-types.h atk-enum-types.c; \
	fi

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file
