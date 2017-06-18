AUTOMAKE_OPTIONS = foreign
xmmsincludedir = $(includedir)/xmms

lib_LTLIBRARIES = libxmms.la

libxmms_la_LDFLAGS = -export-dynamic -version-info $(LIBXMMS_MAJOR_VERSION):$(LIBXMMS_MINOR_VERSION):$(LIBXMMS_MICRO_VERSION)
libxmms_la_LIBADD = @GTK_LIBS@ @POSIX_LIBS@
INCLUDES = @GTK_CFLAGS@ -I../intl -I..

libxmms_la_SOURCES = \
configfile.c configfile.h \
xmmsctrl.c xmmsctrl.h \
dirbrowser.c dirbrowser.h \
util.c util.h \
formatter.c formatter.h \
titlestring.c titlestring.h \
xentry.c xentry.h \
xconvert.c xconvert.h \
charset.c charset.h

xmmsinclude_HEADERS = configfile.h xmmsctrl.h dirbrowser.h util.h formatter.h titlestring.h

