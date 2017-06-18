lib_LTLIBRARIES = libir.la

libdir = $(plugindir)/$(GENERAL_PLUGIN_DIR)

libir_la_LDFLAGS = @PLUGIN_LDFLAGS@
libir_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@

libir_la_SOURCES =    \
ir.c \
ir.h \
irman.h \
configure.c \
about.c \
irio.c \
irfunc.c 

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
