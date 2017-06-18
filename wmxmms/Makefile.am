bin_PROGRAMS = wmxmms

wmxmms_LDADD = @GTK_LIBS@ $(top_builddir)/libxmms/libxmms.la @LTLIBINTL@

wmxmms_SOURCES = 	\
getopt.c \
getopt1.c \
getopt.h \
wmxmms.c

INCLUDES = @GTK_CFLAGS@ @XMMS_DEFINES@ -I$(top_builddir)/intl -I$(top_srcdir)

wmxmmsdir = $(pkgdatadir)

wmxmms_DATA = wmxmms.xpm

EXTRA_DIST = xmms-dock-master.xpm wmxmms.xpm
