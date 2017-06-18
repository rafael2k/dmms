SUBDIRS = defskin

xmmsincludedir = $(includedir)/xmms

bin_PROGRAMS = xmms

xmms_LDFLAGS = -export-dynamic
xmms_LDADD = @GTK_LIBS@ @PTHREAD_LIBS@ @SM_LIBS@ @VM_LIBS@ \
@POSIX_LIBS@ $(top_builddir)/libxmms/libxmms.la @LTLIBINTL@

INCLUDES = @GTK_CFLAGS@ @XMMS_DEFINES@ @ARCH_DEFINES@ \
-I$(top_builddir)/intl -I$(top_srcdir)


xmms_SOURCES =  bmp.c bmp.h \
skin.c skin.h \
util.c util.h \
output.c output.h \
fft.c fft.h \
input.c input.h \
effect.c effect.h \
general.c general.h \
visualization.c visualization.h \
fullscreen.c fullscreen.h \
pluginenum.c pluginenum.h \
playlist.c playlist.h \
controlsocket.c controlsocket.h \
dock.c dock.h \
widget.c widget.h \
sbutton.c sbutton.h \
pbutton.c pbutton.h \
tbutton.c tbutton.h \
textbox.c textbox.h \
menurow.c menurow.h \
hslider.c hslider.h \
monostereo.c monostereo.h \
vis.c vis.h \
svis.c svis.h \
number.c number.h \
playstatus.c playstatus.h \
playlist_list.c playlist_list.h \
playlist_slider.c playlist_slider.h \
playlist_popup.c playlist_popup.h \
eq_graph.c eq_graph.h \
eq_slider.c eq_slider.h \
main.c main.h \
skinwin.c skinwin.h \
prefswin.c prefswin.h \
playlistwin.c playlistwin.h \
equalizer.c equalizer.h \
hints.c hints.h \
about.c about.h \
sm.c sm.h \
plugin.h \
xmms.h \
getopt.c getopt1.c getopt.h \
urldecode.c urldecode.h \
dnd.h \
mkdtemp.c

EXTRA_DIST= xmms_logo.xpm xmms_mini.xpm xmms.desktop xmms.wmconfig

xmmsinclude_HEADERS = plugin.h fullscreen.h
