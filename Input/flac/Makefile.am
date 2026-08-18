if HAVE_FLAC

lib_LTLIBRARIES = libflac.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir) -Isrc
libflac_la_LDFLAGS = @PLUGIN_LDFLAGS@
libflac_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @FLAC_LIBS@

libflac_la_SOURCES = \
plugin.c plugin.h \
configure.c configure.h \
fileinfo.c \
http.c http.h \
tag.c tag.h \
charset.c charset.h \
src/plugin_common/charset.c \
src/plugin_common/tags.c \
src/plugin_common/replaygain.c \
src/plugin_common/dither.c \
src/share/grabbag/alloc.c \
src/share/replaygain_analysis.c \
src/share/replaygain_synthesis.c \
src/share/grabbag/replaygain.c \
src/share/grabbag/file.c \
src/share/grabbag/snprintf.c
