#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <gtk/gtk.h>

#ifdef HAVE_SCHED_H
#include <sched.h>
#elif defined HAVE_SYS_SCHED_H
#include <sys/sched.h>
#endif

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifndef HAVE_NANOSLEEP
# include <sys/types.h>
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#endif


GtkWidget *xmms_show_message(gchar * title, gchar * text, gchar * button_text, gboolean modal, GtkSignalFunc button_action, gpointer action_data)
{
	GtkWidget *dialog, *vbox, *label, *bbox, *button;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_set_modal(GTK_WINDOW(dialog), modal);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox, TRUE, TRUE, 0);

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gtk_widget_show(label);
	gtk_widget_show(vbox);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), bbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label(button_text);
	if (button_action)
		gtk_signal_connect(GTK_OBJECT(button), "clicked", button_action, action_data);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_widget_show(bbox);
	gtk_widget_show(dialog);

	return dialog;
}

gboolean xmms_check_realtime_priority(void)
{
#ifdef HAVE_SCHED_SETSCHEDULER
#ifdef __FreeBSD__
	/*
	 * Check if priority scheduling is enabled in the kernel
	 * before sched_getschedule() (so that we don't get
	 * non-present syscall warnings in kernel log).
	 */
	int val = 0;
	size_t len;

	len = sizeof(val);
	sysctlbyname("p1003_1b.priority_scheduling", &val, &len, NULL, 0);
	if ( !val )
		return FALSE;
#endif
	if (sched_getscheduler(0) == SCHED_RR)
		return TRUE;
	else
#endif
		return FALSE;
}

void xmms_usleep(gint usec)
{
#ifdef HAVE_NANOSLEEP
	struct timespec req;

	req.tv_sec = usec / 1000000;
	usec -= req.tv_sec * 1000000;
	req.tv_nsec = usec * 1000;

	nanosleep(&req, NULL);
#else
	struct timeval tv;
	
	tv.tv_sec = usec / 1000000;
	usec -= tv.tv_sec * 1000000;
	tv.tv_usec = usec;
	select(0, NULL, NULL, NULL, &tv);
#endif
}
