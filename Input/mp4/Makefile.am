if HAVE_MP4

lib_LTLIBRARIES = libmp4.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ -DUSE_TAGGING -I$(top_builddir)/intl -I$(top_srcdir) -Imp4ff
libmp4_la_LDFLAGS = @PLUGIN_LDFLAGS@
libmp4_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @MP4_LIBS@

libmp4_la_SOURCES = \
libmp4.c \
mp4_utils.c \
aac_utils.c \
mp4ff/mp4ff.c \
mp4ff/mp4atom.c \
mp4ff/mp4meta.c \
mp4ff/mp4sample.c \
mp4ff/mp4tagupdate.c \
mp4ff/mp4util.c
