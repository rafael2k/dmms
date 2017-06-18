/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999       Galex Yen
 *  Copyright (C) 1999-2001  Haavard Kvaalen
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include "xmms/i18n.h"
#include "esdout.h"
#include "libxmms/configfile.h"
#include <gtk/gtk.h>

static GtkWidget *configure_win;
static GtkWidget *server_use_remote, *server_oss_mixer, *server_host_entry;
static GtkWidget *server_port_entry, *buffer_size_spin, *buffer_pre_spin;


static void configure_win_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigFile *cfgfile;

	esd_cfg.use_remote = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(server_use_remote));
	esd_cfg.use_oss_mixer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(server_oss_mixer));
	if (esd_cfg.server)
		g_free(esd_cfg.server);
	esd_cfg.server =
		g_strdup(gtk_entry_get_text(GTK_ENTRY(server_host_entry)));
	esd_cfg.port = atoi(gtk_entry_get_text(GTK_ENTRY(server_port_entry)));
	esd_cfg.buffer_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_size_spin));
	esd_cfg.prebuffer = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_pre_spin));

	cfgfile = xmms_cfg_open_default_file();

	xmms_cfg_write_boolean(cfgfile, "ESD", "use_remote",
			       esd_cfg.use_remote);
	xmms_cfg_write_boolean(cfgfile, "ESD", "use_oss_mixer",
			       esd_cfg.use_oss_mixer);
	xmms_cfg_write_string(cfgfile, "ESD", "remote_host", esd_cfg.server);
	xmms_cfg_write_int(cfgfile, "ESD", "remote_port", esd_cfg.port);
	xmms_cfg_write_int(cfgfile, "ESD", "buffer_size", esd_cfg.buffer_size);
	xmms_cfg_write_int(cfgfile, "ESD", "prebuffer", esd_cfg.prebuffer);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);

	gtk_widget_destroy(configure_win);
}

static void use_remote_cb(GtkWidget * w, gpointer data)
{
	gboolean use_remote;
	GtkWidget *box = data;

	use_remote = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(server_use_remote));

	gtk_widget_set_sensitive(box, use_remote);
}

void esdout_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *server_frame, *server_vbox, *server_hbox, *server_btn_hbox;
	GtkWidget *server_host_label, *server_port_label;
	GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
	GtkWidget *buffer_size_box, *buffer_size_label;
	GtkObject *buffer_size_adj, *buffer_pre_adj;
	GtkWidget *buffer_pre_box, *buffer_pre_label;
	GtkWidget *bbox, *ok, *cancel;
	char *temp;

	if (configure_win)
	{
		gdk_window_raise(configure_win->window);
		return;
	}

	configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win),
			     _("ESD Plugin configuration"));
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	server_frame = gtk_frame_new(_("Host:"));
	gtk_container_set_border_width(GTK_CONTAINER(server_frame), 5);

	server_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(server_vbox), 5);
	gtk_container_add(GTK_CONTAINER(server_frame), server_vbox);

	server_btn_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(server_vbox),
			   server_btn_hbox, FALSE, FALSE, 0);

	server_use_remote =
		gtk_check_button_new_with_label(_("Use remote host"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(server_use_remote),
				     esd_cfg.use_remote);
	gtk_box_pack_start(GTK_BOX(server_btn_hbox), server_use_remote,
			   FALSE, FALSE, 0);

	server_oss_mixer =
		gtk_check_button_new_with_label(_("Volume controls OSS mixer"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(server_oss_mixer),
				     esd_cfg.use_oss_mixer);
	gtk_box_pack_start(GTK_BOX(server_btn_hbox),
			   server_oss_mixer, TRUE, TRUE, 0);
#ifndef HAVE_OSS
	gtk_widget_set_sensitive(server_oss_mixer, FALSE);
#endif
	server_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(server_hbox, esd_cfg.use_remote);
	gtk_box_pack_start(GTK_BOX(server_vbox), server_hbox, FALSE, FALSE, 0);

	server_host_label = gtk_label_new(_("Host:"));
	gtk_box_pack_start(GTK_BOX(server_hbox), server_host_label,
			   FALSE, FALSE, 0);

	server_host_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(server_host_entry), esd_cfg.server);
	gtk_box_pack_start(GTK_BOX(server_hbox), server_host_entry,
			   TRUE, TRUE, 0);

	server_port_label = gtk_label_new(_("Port:"));
	gtk_box_pack_start(GTK_BOX(server_hbox), server_port_label,
			   FALSE, FALSE, 0);

	server_port_entry = gtk_entry_new();
	gtk_widget_set_usize(server_port_entry, 50, -1);
	temp = g_strdup_printf("%d", esd_cfg.port);
	gtk_entry_set_text(GTK_ENTRY(server_port_entry), temp);
	g_free(temp);
	gtk_box_pack_start(GTK_BOX(server_hbox), server_port_entry,
			   FALSE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(server_use_remote), "clicked",
			   use_remote_cb, server_hbox);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), server_frame,
				 gtk_label_new(_("Server")));

	buffer_frame = gtk_frame_new(_("Buffering:"));
	gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);

	buffer_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);

	buffer_table = gtk_table_new(2, 1, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
	gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);

	buffer_size_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table),
				  buffer_size_box, 0, 1, 0, 1);
	buffer_size_label = gtk_label_new(_("Buffer size (ms):"));
	gtk_box_pack_start(GTK_BOX(buffer_size_box),
			   buffer_size_label, FALSE, FALSE, 0);
	buffer_size_adj = gtk_adjustment_new(esd_cfg.buffer_size,
					     200, 10000, 100, 100, 100);
	buffer_size_spin =
		gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj), 8, 0);
	gtk_widget_set_usize(buffer_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_size_box),
			   buffer_size_spin, FALSE, FALSE, 0);

	buffer_pre_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table),
				  buffer_pre_box, 1, 2, 0, 1);
	buffer_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
	gtk_box_pack_start(GTK_BOX(buffer_pre_box),
			   buffer_pre_label, FALSE, FALSE, 0);
	buffer_pre_adj = gtk_adjustment_new(esd_cfg.prebuffer, 0, 90, 1, 1, 1);
	buffer_pre_spin =
		gtk_spin_button_new(GTK_ADJUSTMENT(buffer_pre_adj), 1, 0);
	gtk_widget_set_usize(buffer_pre_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_pre_box),
			   buffer_pre_spin, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 buffer_frame, gtk_label_new(_("Buffering")));

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(configure_win);
}
