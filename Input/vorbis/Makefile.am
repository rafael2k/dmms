if HAVE_OGGVORBIS

lib_LTLIBRARIES = libvorbis.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ @OGG_CFLAGS@ @VORBIS_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libvorbis_la_LDFLAGS = @PLUGIN_LDFLAGS@
libvorbis_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @VORBIS_LIBS@ @OGG_LIBS@ @VORBISFILE_LIBS@ \
@LTLIBICONV@

libvorbis_la_SOURCES = \
vorbis.c vorbis.h \
configure.c \
http.c http.h \
fileinfo.c \
vcedit.c vcedit.h
