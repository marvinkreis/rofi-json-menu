ACLOCAL_AMFLAGS=-I m4 -I submodules/libnkutils
plugindir=@rofi_PLUGIN_INSTALL_DIR@

noinst_LTLIBRARIES =
noinst_PROGRAMS=
EXTRA_DIST=
check_PROGRAMS=
TESTS=
include submodules/libnkutils/libnkutils.mk

plugin_LTLIBRARIES = jsonmenu.la

jsonmenu_la_SOURCES=\
		 src/jsonmenu.c

jsonmenu_la_CFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@ @jsonc_CFLAGS@ $(NKUTILS_CFLAGS)
jsonmenu_la_LIBADD= @glib_LIBS@ @rofi_LIBS@ @jsonc_LIBS@ $(NKUTILS_LIBS)
jsonmenu_la_LDFLAGS= -module -avoid-version
