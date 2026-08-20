if HAVE_PNG

vumeterltlibs = libvumeter.la
vumetersources = plugin_main.c plugin_dialogs.c plugin_configs.c plugin_skin.c \
		 plugin_helper.c plugin_worker.c plugin_image.c

else

vumeterltlibs =
vumetersources =

endif

EXTRA_DIST = plugin_main.c plugin_dialogs.c plugin_configs.c plugin_skin.c \
	     plugin_helper.c plugin_worker.c plugin_image.c \
	     plugin_main.h plugin_dialogs.h plugin_skin.h plugin_helper.h \
	     plugin_worker.h plugin_image.h plugin_icon.xpm \
	     skins/Default skins/Led_o_meter

lib_LTLIBRARIES = $(vumeterltlibs)

libdir = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

libvumeter_la_LDFLAGS = @PLUGIN_LDFLAGS@
libvumeter_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @PNG_LIBS@

libvumeter_la_SOURCES = $(vumetersources)

noinst_HEADERS = plugin_main.h plugin_dialogs.h plugin_skin.h plugin_helper.h \
		 plugin_worker.h plugin_image.h plugin_icon.xpm

INCLUDES = @GTK_CFLAGS@ @PNG_CFLAGS@ @XMMS_DEFINES@ -DSKINDIR=\"$(datadir)/xmms\" \
	   -I$(top_builddir)/intl -I$(top_srcdir)

vumeterskindir = $(datadir)/xmms/VU_Meter_skins

install-data-local:
	@if test "x$(vumeterltlibs)" != "x"; then \
		for d in Default Led_o_meter; do \
			$(mkinstalldirs) $(DESTDIR)$(vumeterskindir)/$$d; \
			for f in $(srcdir)/skins/$$d/*; do \
				$(INSTALL_DATA) $$f $(DESTDIR)$(vumeterskindir)/$$d/; \
			done; \
		done; \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(vumeterskindir)
