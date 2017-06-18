lib_LTLIBRARIES = libstereo.la

libdir = $(plugindir)/$(EFFECT_PLUGIN_DIR)

libstereo_la_LDFLAGS = @PLUGIN_LDFLAGS@
libstereo_la_LIBADD = @GTK_LIBS@

libstereo_la_SOURCES = 	\
stereo.c

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
