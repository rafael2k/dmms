lib_LTLIBRARIES = libvoice.la

libdir = $(plugindir)/$(EFFECT_PLUGIN_DIR)

libvoice_la_LDFLAGS = @PLUGIN_LDFLAGS@
libvoice_la_LIBADD = @GTK_LIBS@

libvoice_la_SOURCES = 	\
about.c \
voice.c \
voice.h

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
