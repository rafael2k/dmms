lib_LTLIBRARIES = libecho.la

libdir = $(plugindir)/$(EFFECT_PLUGIN_DIR)

libecho_la_LDFLAGS = @PLUGIN_LDFLAGS@
libecho_la_LIBADD = @GTK_LIBS@

libecho_la_SOURCES =    \
echo.c \
echo.h \
gui.c

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
