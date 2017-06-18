solarissources = \
Sun.c Sun.h \
about.c \
configure.c \
init.c \
audio.c

if HAVE_SOLARIS

solarislib = libSolaris.la
solarisbuildsources = $(solarissources)

else

solarislib =
solarisbuildsources =

endif

EXTRA_DIST = $(solarissources) README.solaris

lib_LTLIBRARIES = $(solarislib)
libSolaris_la_LDFLAGS = @PLUGIN_LDFLAGS@
libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)
libSolaris_la_SOURCES = $(solarisbuildsources)
INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
