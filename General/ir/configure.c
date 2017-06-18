/*  IRman plugin for xmms by Charles Sielski (stray@teklabs.net) ..
 *  XMMS is Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <stdlib.h> /* We use atoi() here.. */
#include "xmms/i18n.h"
#include "ir.h"

static gboolean keepConfGoing;
gboolean irconf_is_going = FALSE;
static GtkWidget *irconf_mainwin, *irconf_controlwin, *irconf_playlistwin;
static GtkWidget *dev_entry, *ircode_entry, *playlist_entry, *playlist_spin;
static GtkWidget *codelen_entry;

static gchar *ir_control[13] =
{
	N_("Play"),	N_("Stop"),	N_("Pause"),
	N_("Prev"),	N_("Next"),	N_("Vol +"),
	N_("Seek -5s"),	N_("Seek +5s"),	N_("Vol -"),
	N_("Shuffle"),	N_("Repeat"),	N_("Playlist"),
	N_("+100")
};
static gchar *ir_playlist[10] =
{
	N_("0"), N_("1"), N_("2"), N_("3"), N_("4"),
	N_("5"), N_("6"), N_("7"), N_("8"), N_("9")
};

static gchar *irbutton_to_edit;
static gint ir_was_enabled;

static void irconf_ok_cb(GtkWidget * w, gpointer data)
{
	ircfg.device = g_strdup(gtk_entry_get_text(GTK_ENTRY(dev_entry)));
	ircfg.codelen = atoi(gtk_entry_get_text(GTK_ENTRY(codelen_entry)));
	if(ircfg.codelen > IR_MAX_CODE_LEN)
	    ircfg.codelen = IR_MAX_CODE_LEN;
	if(ircfg.codelen < 0)
	    ircfg.codelen = 0;
	/* Re-initialize remote using new settings */
	ir_close_port();
	ir_open_port(ircfg.device);
	irapp_save_config();
	gtk_widget_destroy(irconf_mainwin);
}

static void irconf_cancel_cb(GtkWidget * w, gpointer data)
{
	irapp_read_config();
	gtk_widget_destroy(irconf_mainwin);
}

static gint irconf_codeentry_routine(gpointer data)
{
	unsigned char *code;
	char *text;

	code = ir_poll_code();
	if (code)
	{
		text = ir_code_to_text(code);
		gtk_entry_set_text(GTK_ENTRY(ircode_entry), text);
	}
	if (keepConfGoing)
		return TRUE;
	else
		return FALSE;
}

static void irconf_control_ok_cb(GtkWidget * w, gpointer data)
{
	gint i;

	keepConfGoing = FALSE;
	ir_close_port();
	if (ir_was_enabled)
		irapp_init_port(ircfg.device);
	irconf_is_going = FALSE;
	if (!strcmp(irbutton_to_edit, ir_control[0]))
		ircfg.button_play = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[1]))
		ircfg.button_stop = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[2]))
		ircfg.button_pause = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[3]))
		ircfg.button_prev = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[4]))
		ircfg.button_next = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[5]))
		ircfg.button_volup = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[6]))
		ircfg.button_seekb = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[7]))
		ircfg.button_seekf = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[8]))
		ircfg.button_voldown = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[9]))
		ircfg.button_shuffle = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[10]))
		ircfg.button_repeat = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[11]))
		ircfg.button_playlist = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else if (!strcmp(irbutton_to_edit, ir_control[12]))
		ircfg.button_plus100 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));
	else
		for (i = 0; i < 10; i++)
			if (!strcmp(irbutton_to_edit, ir_playlist[i]))
				ircfg.button[i] = g_strdup(gtk_entry_get_text(GTK_ENTRY(ircode_entry)));

	gtk_widget_destroy(irconf_controlwin);
}

static void irconf_control_cancel_cb(GtkWidget * w, gpointer data)
{
	keepConfGoing = FALSE;
	ir_close_port();
	if (ir_was_enabled)
		irapp_init_port(ircfg.device);
	irconf_is_going = FALSE;
	gtk_widget_destroy(irconf_controlwin);
}

static void irconf_control_cb(GtkWidget * w, gchar * data)
{
	GtkWidget *vbox, *frame, *table, *button, *box;
	gint i;

	if (!irconf_controlwin && !irconf_playlistwin)
	{
		char *tmp;

		keepConfGoing = TRUE;
		irbutton_to_edit = data;
		irconf_controlwin = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_signal_connect(GTK_OBJECT(irconf_controlwin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &irconf_controlwin);
		tmp=g_strdup_printf(_("`%s' Button Setup"),_(data));
		gtk_window_set_title(GTK_WINDOW(irconf_controlwin), tmp);
		g_free(tmp);
		gtk_window_set_policy(GTK_WINDOW(irconf_controlwin), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(irconf_controlwin), GTK_WIN_POS_MOUSE);
		gtk_container_border_width(GTK_CONTAINER(irconf_controlwin), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(irconf_controlwin), vbox);

		frame = gtk_frame_new(_("Enter code or use remote"));
		gtk_container_border_width(GTK_CONTAINER(frame), 5);
		gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

		table = gtk_table_new(1, 1, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 2);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);

		ircode_entry = gtk_entry_new();
		gtk_table_attach_defaults(GTK_TABLE(table), ircode_entry, 0, 1, 0, 1);
		if (!strcmp(data, ir_control[0]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_play);
		else if (!strcmp(data, ir_control[1]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_stop);
		else if (!strcmp(data, ir_control[2]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_pause);
		else if (!strcmp(data, ir_control[3]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_prev);
		else if (!strcmp(data, ir_control[4]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_next);
		else if (!strcmp(data, ir_control[5]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_volup);
		else if (!strcmp(data, ir_control[6]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_seekb);
		else if (!strcmp(data, ir_control[7]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_seekf);
		else if (!strcmp(data, ir_control[8]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_voldown);
		else if (!strcmp(data, ir_control[9]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_shuffle);
		else if (!strcmp(data, ir_control[10]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_repeat);
		else if (!strcmp(data, ir_control[11]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_playlist);
		else if (!strcmp(data, ir_control[12]))
			gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button_plus100);
		else
			for (i = 0; i < 10; i++)
				if (!strcmp(data, ir_playlist[i]))
					gtk_entry_set_text(GTK_ENTRY(ircode_entry), ircfg.button[i]);
		gtk_widget_show(ircode_entry);

		box = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(box), 5);
		gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

		button = gtk_button_new_with_label(_("OK"));
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_control_ok_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_control_cancel_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_show(button);

		gtk_widget_show(box);
		gtk_widget_show(table);
		gtk_widget_show(frame);
		gtk_widget_show(vbox);
		gtk_widget_show(irconf_controlwin);

		if ((ir_was_enabled = ir_get_portfd()))
			ir_close_port();
		irapp_init_port(g_strdup(gtk_entry_get_text(GTK_ENTRY(dev_entry))));
		irconf_is_going = TRUE;
		gtk_timeout_add(10, irconf_codeentry_routine, NULL);
	}
}

static void spin_change_cb(GtkWidget * widget, GtkSpinButton * spin)
{
	gtk_entry_set_text(GTK_ENTRY(playlist_entry), ircfg.playlist[gtk_spin_button_get_value_as_int(spin)]);
}

static void pl_entry_change_cb(GtkWidget * widget, GtkSpinButton * spin)
{
	ircfg.playlist[gtk_spin_button_get_value_as_int(spin)] = g_strdup(gtk_entry_get_text(GTK_ENTRY(playlist_entry)));
}

void ir_configure(void)
{
	GtkWidget *vbox, *notebook, *box, *frame, *table, *vbox2, *label,
	         *button, *label_codelen;
	GtkAdjustment *adj;
	gint i;

	irapp_read_config();
	if (!irconf_mainwin)
	{

		irconf_mainwin = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_signal_connect(GTK_OBJECT(irconf_mainwin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &irconf_mainwin);
		gtk_window_set_title(GTK_WINDOW(irconf_mainwin), _("XMMS IRman Configuration"));
		gtk_window_set_policy(GTK_WINDOW(irconf_mainwin), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(irconf_mainwin), GTK_WIN_POS_MOUSE);
		gtk_container_border_width(GTK_CONTAINER(irconf_mainwin), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(irconf_mainwin), vbox);

		notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

		box = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(box), 5);

		frame = gtk_frame_new(_("Device:"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);

		table = gtk_table_new(2, 1, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 5);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);

		label = gtk_label_new(_("Device: "));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
		gtk_widget_show(label);

		dev_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(dev_entry), ircfg.device);
		gtk_table_attach_defaults(GTK_TABLE(table), dev_entry, 1, 2, 0, 1);
		gtk_widget_show(dev_entry);

                label_codelen = gtk_label_new(_("IR code length: "));
		gtk_misc_set_alignment(GTK_MISC(label_codelen), 1.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label_codelen, 0, 1, 1, 2);
		gtk_widget_show(label_codelen);

		codelen_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(codelen_entry), g_strdup_printf("%d", ircfg.codelen));
		gtk_table_attach_defaults(GTK_TABLE(table), codelen_entry, 1, 2, 1, 2);
		gtk_widget_show(codelen_entry);

		gtk_widget_show(table);
		gtk_widget_show(frame);

		frame = gtk_frame_new(_("Controls:"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);

		vbox2 = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), vbox2);

		table = gtk_table_new(5, 3, TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_table_set_row_spacings(GTK_TABLE(table), 5);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);
		gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

		for (i = 0; i < 13; i++)
		{
			button = gtk_button_new_with_label(_(ir_control[i]));
			gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_control_cb), ir_control[i]);
			gtk_table_attach_defaults(GTK_TABLE(table), button, i % 3, (i % 3) + 1, i / 3, (i / 3) + 1);
			gtk_widget_show(button);
		}
		gtk_widget_show(table);
		gtk_widget_show(frame);

		table = gtk_table_new(2, 5, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_table_set_row_spacings(GTK_TABLE(table), 0);
		gtk_table_set_col_spacings(GTK_TABLE(table), 0);
		gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

		for (i = 0; i < 10; i++)
		{
			button = gtk_button_new_with_label(_(ir_playlist[i]));
			gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_control_cb), ir_playlist[i]);
			gtk_table_attach_defaults(GTK_TABLE(table), button, i % 5, (i % 5) + 1, i / 5, (i / 5) + 1);
			gtk_widget_show(button);
		}
		gtk_widget_show(table);
		gtk_widget_show(frame);

		frame = gtk_frame_new(_("Playlists:"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);

		table = gtk_table_new(2, 1, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 5);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);

		adj = (GtkAdjustment *) gtk_adjustment_new(0, 0, 99, 1, 5, 0);
		playlist_spin = gtk_spin_button_new(adj, 0, 0);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(playlist_spin), TRUE);
		gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(playlist_spin), GTK_UPDATE_IF_VALID);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(playlist_spin), FALSE);
		gtk_table_attach_defaults(GTK_TABLE(table), playlist_spin, 0, 1, 0, 1);
		gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(spin_change_cb), (gpointer) playlist_spin);
		gtk_widget_show(playlist_spin);

		playlist_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(playlist_entry), ircfg.playlist[0]);
		gtk_signal_connect(GTK_OBJECT(playlist_entry), "changed", GTK_SIGNAL_FUNC(pl_entry_change_cb), (gpointer) playlist_spin);
		gtk_table_attach_defaults(GTK_TABLE(table), playlist_entry, 1, 2, 0, 1);
		gtk_widget_show(playlist_entry);

		gtk_widget_show(table);
		gtk_widget_show(frame);
		gtk_widget_show(vbox2);
		gtk_widget_show(box);

		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, gtk_label_new(_("General")));
		gtk_widget_show(notebook);

		box = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(box), 5);
		gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

		button = gtk_button_new_with_label(_("OK"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_ok_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(irconf_cancel_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_show(button);
		gtk_widget_show(box);
		gtk_widget_show(vbox);
		gtk_widget_show(irconf_mainwin);
	}
}
