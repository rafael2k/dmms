if HAVE_OPENGL

oglspectrumltlibs = libogl_spectrum.la
oglspectrumsources = opengl_spectrum.c opengl_spectrum.h configure.c 

else

oglspectrumltlibs = 
oglspectrumsources = 

endif

EXTRA_DIST = opengl_spectrum.c configure.c opengl_spectrum.h

lib_LTLIBRARIES = $(oglspectrumltlibs)

libdir = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

libogl_spectrum_la_LDFLAGS = @PLUGIN_LDFLAGS@
libogl_spectrum_la_LIBADD = @GTK_LIBS@ @OPENGL_LIBS@

libogl_spectrum_la_SOURCES = $(oglspectrumsources)
INCLUDES = @GTK_CFLAGS@ @XMMS_DEFINES@ -I$(top_builddir)/intl -I$(top_srcdir)
