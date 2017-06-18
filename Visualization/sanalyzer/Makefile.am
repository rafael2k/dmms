lib_LTLIBRARIES = libsanalyzer.la

libdir = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

libsanalyzer_la_LDFLAGS = @PLUGIN_LDFLAGS@
libsanalyzer_la_LIBADD = @GTK_LIBS@

libsanalyzer_la_SOURCES =  spectrum.c
INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)

EXTRA_DIST = xmms_logo.xpm
