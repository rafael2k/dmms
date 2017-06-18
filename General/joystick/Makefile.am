if HAVE_LINUX_JOYSTICK

joyltlibs = libjoy.la
joysources = joy.c \
joy.h \
configure.c \
about.c

else

joyltlibs = 
joysources = 

endif

EXTRA_DIST = joy.c joy.h configure.c about.c

lib_LTLIBRARIES = $(joyltlibs)

libdir = $(plugindir)/$(GENERAL_PLUGIN_DIR)

libjoy_la_LDFLAGS = @PLUGIN_LDFLAGS@
libjoy_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@

libjoy_la_SOURCES = $(joysources)

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
