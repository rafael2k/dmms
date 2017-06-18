/*
 * about.c
 *
 * Dialog box for when user clicks on 'about' button
 *
 */

#include "Sun.h"
#include "xmms/i18n.h"
#include "libxmms/util.h"

void aboutSunAudio(void)
{
	static GtkWidget *about_dialog;

	if (about_dialog != NULL)
		return;

	about_dialog = xmms_show_message(
		_("About Solaris Audio Driver"),
		_("XMMS Solaris Audio Driver\n\n"
		  "Written by John Riddoch (jr@scms.rgu.ac.uk)\n"
		  "with help from many contributors."),
		_("OK"), FALSE, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(about_dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &about_dialog);
}
