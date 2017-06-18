if HAVE_SUN
sunltlibs = libsun.la
sunsources =	sun.c	    \
		sun.h	    \
		audio.c	    \
		convert.c   \
		mixer.c	    \
		mixer.h     \
		configure.c \
		about.c     \
		resample.h
else
sunltlibs =
sunsources =
endif

EXTRA_DIST = sun.c sun.h audio.c convert.c mixer.c mixer.h configure.c about.c resample.h

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)
lib_LTLIBRARIES = $(sunltlibs)

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libsun_la_LDFLAGS = @PLUGIN_LDFLAGS@
libsun_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@
libsun_la_SOURCES= $(sunsources)

