/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen <havardk@xmms.org>
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

#include "xmms/i18n.h"
#include "cdaudio.h"
#include "libxmms/titlestring.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define GET_TB(b) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))
#define SET_TB(b) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE)

struct driveconfig {
	GtkWidget *device, *directory;
	GtkWidget *mixer_oss, *mixer_drive;
	GtkWidget *remove_button;
	GtkWidget *dae;
};

static GList *drives;

static GtkWidget *cdda_configure_win;
static GtkWidget *cdi_name, *cdi_name_override;
static GtkWidget *cdi_use_cddb, *cdi_cddb_server /*, *cdi_use_cdin, *cdi_cdin_server */;

void cdda_cddb_show_server_dialog(GtkWidget *w, gpointer data);
void cdda_cddb_show_network_window(GtkWidget *w, gpointer data);
void cdda_cddb_set_server(char *new_server);

static GtkWidget* configurewin_add_drive(struct driveinfo *drive, void *nb);

static void cdda_configurewin_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigFile *cfgfile;
	struct driveinfo *drive;
	GList *node;
	int olddrives, ndrives, i;

	olddrives = g_list_length(cdda_cfg.drives);
	for (node = cdda_cfg.drives; node; node = node->next)
	{
		drive = node->data;
		g_free(drive->device);
		g_free(drive->directory);
		g_free(drive);
	}
	g_list_free(cdda_cfg.drives);
	cdda_cfg.drives = NULL;

	for (node = drives; node; node = node->next)
	{
		struct driveconfig *config = node->data;
		char *tmp;

		drive = g_malloc0(sizeof (*drive));
		drive->device = g_strdup(gtk_entry_get_text(GTK_ENTRY(config->device)));

		tmp = gtk_entry_get_text(GTK_ENTRY(config->directory));
		if (strlen(tmp) < 2 || tmp[strlen(tmp) - 1] == '/')
			drive->directory = g_strdup(tmp);
		else
			drive->directory = g_strconcat(tmp, "/", NULL);

		if (GET_TB(config->mixer_oss))
			drive->mixer = CDDA_MIXER_OSS;
		else if (GET_TB(config->mixer_drive))
			drive->mixer = CDDA_MIXER_DRIVE;
		else
			drive->mixer = CDDA_MIXER_NONE;
		if (GET_TB(config->dae))
			drive->dae = CDDA_READ_DAE;
		else
			drive->dae = CDDA_READ_ANALOG;
#ifdef HAVE_OSS
		drive->oss_mixer = SOUND_MIXER_CD;
#endif

		cdda_cfg.drives = g_list_append(cdda_cfg.drives, drive);
	}

	cdda_cfg.title_override = GET_TB(cdi_name_override);
	g_free(cdda_cfg.name_format);
	cdda_cfg.name_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_name)));

	cdda_cfg.use_cddb = GET_TB(cdi_use_cddb);
	cdda_cddb_set_server(gtk_entry_get_text(GTK_ENTRY(cdi_cddb_server)));

#if 0
	cdda_cfg.use_cdin = GET_TB(cdi_use_cdin);
	if (strcmp(cdda_cfg.cdin_server, gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server))))
	{
		g_free(cdda_cfg.cdin_server);
		cdda_cfg.cdin_server = g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server)));
	}
#endif

	cfgfile = xmms_cfg_open_default_file();

	drive = cdda_cfg.drives->data;
	xmms_cfg_write_string(cfgfile, "CDDA", "device", drive->device);
	xmms_cfg_write_string(cfgfile, "CDDA", "directory", drive->directory);
  	xmms_cfg_write_int(cfgfile, "CDDA", "mixer", drive->mixer);
	xmms_cfg_write_int(cfgfile, "CDDA", "readmode", drive->dae);

/*  	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_oss_mixer", cdda_cfg.use_oss_mixer); */
	
	for (node = cdda_cfg.drives->next, i = 1; node; node = node->next, i++)
	{
		char label[20];
		drive = node->data;

		sprintf(label, "device%d", i);
		xmms_cfg_write_string(cfgfile, "CDDA", label, drive->device);
		
		sprintf(label, "directory%d", i);
		xmms_cfg_write_string(cfgfile, "CDDA", label, drive->directory);

		sprintf(label, "mixer%d", i);
		xmms_cfg_write_int(cfgfile, "CDDA", label, drive->mixer);

		sprintf(label, "readmode%d", i);
		xmms_cfg_write_int(cfgfile, "CDDA", label, drive->dae);
	}

	ndrives = g_list_length(cdda_cfg.drives);

	for (i = ndrives; i < olddrives; i++)
		/* FIXME: Clear old entries */;

	xmms_cfg_write_int(cfgfile, "CDDA", "num_drives", ndrives);

	xmms_cfg_write_boolean(cfgfile, "CDDA", "title_override", cdda_cfg.title_override);
	xmms_cfg_write_string(cfgfile, "CDDA", "name_format", cdda_cfg.name_format);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_cddb", cdda_cfg.use_cddb);
	xmms_cfg_write_string(cfgfile, "CDDA", "cddb_server", cdda_cfg.cddb_server);
	xmms_cfg_write_int(cfgfile, "CDDA", "cddb_protocol_level", cdda_cfg.cddb_protocol_level);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_cdin", cdda_cfg.use_cdin);
	xmms_cfg_write_string(cfgfile, "CDDA", "cdin_server", cdda_cfg.cdin_server);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);
}

static void configurewin_close(GtkButton *w, gpointer data)
{
	GList *node;

	for (node = drives; node; node = node->next)
		g_free(node->data);
	g_list_free(drives);
	drives = NULL;

	gtk_widget_destroy(cdda_configure_win);
}

static void toggle_set_sensitive_cb(GtkToggleButton * w, gpointer data)
{
	gboolean set = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(data), set);
}

static void configurewin_add_page(GtkButton *w, gpointer data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(data);
	GtkWidget *box = configurewin_add_drive(NULL, nb);
	char *label = g_strdup_printf(_("Drive %d"), g_list_length(drives));
	
	gtk_widget_show_all(box);
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), box,
				 gtk_label_new(label));
	g_free(label);
}

static void redo_nb_labels(GtkNotebook *nb)
{
	int i;
	GtkWidget *child;
	
	for (i = 0; (child = gtk_notebook_get_nth_page(nb, i)) != NULL; i++)
	{
		char *label = g_strdup_printf(_("Drive %d"), i + 1);

		gtk_notebook_set_tab_label_text(nb, child, label);
		g_free(label);
	}
}
	

static void configurewin_remove_page(GtkButton *w, gpointer data)
{
	GList *node;
	GtkNotebook *nb = GTK_NOTEBOOK(data);
	gtk_notebook_remove_page(nb, gtk_notebook_get_current_page(nb));
	for (node = drives; node; node = node->next)
	{
		struct driveconfig *drive = node->data;

		if (GTK_WIDGET(w) == drive->remove_button)
		{
			if (node->next)
				redo_nb_labels(nb);
			drives = g_list_remove(drives, drive);
			g_free(drive);
			break;
		}
	}
	if (g_list_length(drives) == 1)
	{
		struct driveconfig *drive = drives->data;
		gtk_widget_set_sensitive(drive->remove_button, FALSE);
	}
}


static void configurewin_check_drive(GtkButton *w, gpointer data)
{
	struct driveconfig *drive = data;
	GtkWidget *window, *vbox, *label, *bbox, *closeb;
	char *device, *directory;
	int fd, dae_track = -1;
	GString *str = g_string_new("");
	struct stat stbuf;

	device = gtk_entry_get_text(GTK_ENTRY(drive->device));
	directory = gtk_entry_get_text(GTK_ENTRY(drive->directory));

	if ((fd = open(device, CDOPENFLAGS)) < 0)
		g_string_sprintfa(str, _("Failed to open device %s\n"
					 "Error: %s\n\n"),
				  device, strerror(errno));
	else
	{
		cdda_disc_toc_t toc;
		if (!cdda_get_toc(&toc, device))
			g_string_append(str,
					_("Failed to read \"Table of Contents\""
					  "\nMaybe no disc in the drive?\n\n"));
		else
		{
			int i, data = 0;
			g_string_sprintfa(str, _("Device %s OK.\n"
						 "Disc has %d tracks"), device,
					  toc.last_track - toc.first_track + 1);
			for (i = toc.first_track; i <= toc.last_track; i++)
				if (toc.track[i].flags.data_track)
					data++;
				else if (dae_track < 0)
					dae_track = i;
			if (data > 0)
				g_string_sprintfa(str, _(" (%d data tracks)"),
						  data);
			g_string_sprintfa(str, _("\nTotal length: %d:%.2d\n"),
					  toc.leadout.minute,
					  toc.leadout.second);
#ifdef CDDA_HAS_READAUDIO
			if (dae_track == -1)
				g_string_sprintfa(str,
						  _("Digital audio extraction "
						    "not tested as the disc has "
						    "no audio tracks\n"));
			else
			{
				int start, end, fr;
				char buffer[CD_FRAMESIZE_RAW];
				start = LBA(toc.track[dae_track]);

				if (dae_track == toc.last_track)
					end = LBA(toc.leadout);
				else
					end = LBA(toc.track[dae_track + 1]);
				fr = read_audio_data(fd, start + (end - start) / 2,
						     1, buffer);
				if (fr > 0)
					g_string_sprintfa(str,
						_("Digital audio extraction "
						  "test: OK\n\n"));
				else
					g_string_sprintfa(str,
						_("Digital audio extraction "
						  "test failed: %s\n\n"),
						strerror(-fr));
			}
#else
			g_string_sprintfa(str, "\n");
#endif
		}
		close(fd);
	}

	if (stat(directory, &stbuf) < 0)
	{
		g_string_sprintfa(str, _("Failed to check directory %s\n"
					 "Error: %s"),
				  directory, strerror(errno));
	}
	else
	{
		if (!S_ISDIR(stbuf.st_mode))
			g_string_sprintfa(str,
				_("Error: %s exists, but is not a directory"),
				directory);
		else
		{
			if (!access(directory, R_OK))
				g_string_sprintfa(str, _("Directory %s OK."),
						  directory);
			else
				g_string_sprintfa(str,
						  _("Directory %s exists, but "
						    "you do not have permission "
						    "to access it."),
						  directory);
		}
				
	}
			
	window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_transient_for(GTK_WINDOW(window),
				     GTK_WINDOW(cdda_configure_win));
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	label = gtk_label_new(str->str);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	closeb = gtk_button_new_with_label("Close");
	GTK_WIDGET_SET_FLAGS(closeb, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(closeb), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
	gtk_box_pack_start(GTK_BOX(bbox), closeb, TRUE, TRUE, 0);
	gtk_widget_grab_default(closeb);

	g_string_free(str, TRUE);

	gtk_widget_show_all(window);
}

static GtkWidget* configurewin_add_drive(struct driveinfo *drive, void *nb)
{
	GtkWidget *vbox, *bbox, *dev_frame, *dev_table, *dev_label;
	GtkWidget *dev_dir_label, *check_btn;
	GtkWidget *volume_frame, *volume_box, *volume_none;
	GtkWidget *readmode_frame, *readmode_box, *readmode_analog;
	struct driveconfig *d = g_malloc(sizeof (struct driveconfig));

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	
	dev_frame = gtk_frame_new(_("Device:"));
	gtk_box_pack_start(GTK_BOX(vbox), dev_frame, FALSE, FALSE, 0);
	dev_table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dev_table), 5);
	gtk_container_add(GTK_CONTAINER(dev_frame), dev_table);
	gtk_table_set_row_spacings(GTK_TABLE(dev_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(dev_table), 5);

	dev_label = gtk_label_new(_("Device:"));
	gtk_misc_set_alignment(GTK_MISC(dev_label), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(dev_table), dev_label, 0, 1, 0, 1,
			 GTK_FILL, 0, 0, 0);

	d->device = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(dev_table), d->device, 1, 2, 0, 1,
			 GTK_FILL | GTK_EXPAND, 0, 0 , 0);

	dev_dir_label = gtk_label_new(_("Directory:"));
	gtk_misc_set_alignment(GTK_MISC(dev_dir_label), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(dev_table), dev_dir_label, 0, 1, 1, 2,
			 GTK_FILL, 0, 0, 0);

	d->directory = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(dev_table), d->directory, 1, 2, 1, 2,
			 GTK_FILL | GTK_EXPAND, 0, 0, 0);

	readmode_frame = gtk_frame_new(_("Play mode:"));
	gtk_box_pack_start(GTK_BOX(vbox), readmode_frame, FALSE, FALSE, 0);

	readmode_box = gtk_vbox_new(5, FALSE);
	gtk_container_add(GTK_CONTAINER(readmode_frame), readmode_box);

	readmode_analog = gtk_radio_button_new_with_label(NULL, _("Analog"));
	gtk_box_pack_start(GTK_BOX(readmode_box), readmode_analog, FALSE, FALSE, 0);
	
	d->dae = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(readmode_analog), _("Digital audio extraction"));
	gtk_box_pack_start(GTK_BOX(readmode_box), d->dae, FALSE, FALSE, 0);
#ifndef CDDA_HAS_READAUDIO
	gtk_widget_set_sensitive(readmode_frame, FALSE);
#endif	

	/*
	 * Volume config
	 */

	volume_frame = gtk_frame_new(_("Volume control:"));
	gtk_box_pack_start(GTK_BOX(vbox), volume_frame, FALSE, FALSE, 0);

	volume_box = gtk_vbox_new(5, FALSE);
	gtk_container_add(GTK_CONTAINER(volume_frame), volume_box);

	volume_none = gtk_radio_button_new_with_label(NULL, _("No mixer"));
	gtk_box_pack_start(GTK_BOX(volume_box), volume_none, FALSE, FALSE, 0);

	d->mixer_drive = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(volume_none), _("CD-ROM drive"));
	gtk_box_pack_start(GTK_BOX(volume_box), d->mixer_drive, FALSE, FALSE, 0);

	d->mixer_oss = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(volume_none), _("OSS mixer"));
	gtk_box_pack_start(GTK_BOX(volume_box), d->mixer_oss, FALSE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(readmode_analog), "toggled",
			   toggle_set_sensitive_cb, volume_frame);
#ifndef HAVE_OSS
	gtk_widget_set_sensitive(d->mixer_oss, FALSE);
#endif
	if (drive)
	{
		gtk_entry_set_text(GTK_ENTRY(d->device), drive->device);
		gtk_entry_set_text(GTK_ENTRY(d->directory), drive->directory);
		if (drive->mixer == CDDA_MIXER_DRIVE)
			SET_TB(d->mixer_drive);
		else if (drive->mixer == CDDA_MIXER_OSS)
			SET_TB(d->mixer_oss);
		if (drive->dae == CDDA_READ_DAE)
			SET_TB(d->dae);
	}

	bbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);

	check_btn = gtk_button_new_with_label(_("Check drive..."));
	GTK_WIDGET_SET_FLAGS(check_btn, GTK_CAN_DEFAULT);
	gtk_box_pack_start_defaults(GTK_BOX(bbox), check_btn);
	gtk_signal_connect(GTK_OBJECT(check_btn), "clicked",
			   GTK_SIGNAL_FUNC(configurewin_check_drive), d);

	d->remove_button = gtk_button_new_with_label(_("Remove drive"));
	GTK_WIDGET_SET_FLAGS(d->remove_button, GTK_CAN_DEFAULT);
	gtk_box_pack_start_defaults(GTK_BOX(bbox), d->remove_button);
	gtk_signal_connect(GTK_OBJECT(d->remove_button), "clicked",
			   GTK_SIGNAL_FUNC(configurewin_remove_page), nb);


	if (drives == NULL)
		gtk_widget_set_sensitive(d->remove_button, FALSE);
	else
	{
		struct driveconfig *tmp = drives->data;
		gtk_widget_set_sensitive(tmp->remove_button, TRUE);
	}

	drives = g_list_append(drives, d);

	return vbox;
}

void cdda_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *dev_vbox, *dev_notebook, *add_drive, *add_bbox;
	GtkWidget *cdi_vbox;
	GtkWidget *cdi_cddb_frame, *cdi_cddb_vbox, *cdi_cddb_hbox;
	GtkWidget *cdi_cddb_server_hbox, *cdi_cddb_server_label;
	GtkWidget *cdi_cddb_server_list, *cdi_cddb_debug_win;
#if 0
	GtkWidget *cdi_cdin_frame, *cdi_cdin_vbox;
	GtkWidget *cdi_cdin_server_hbox, *cdi_cdin_server_label;
#endif
	GtkWidget *cdi_name_frame, *cdi_name_vbox, *cdi_name_hbox;
	GtkWidget *cdi_name_label, *cdi_desc;
	GtkWidget *cdi_name_enable_vbox;
	GtkWidget *bbox, *ok, *cancel;

	GList *node;
	int i = 1;

	if (cdda_configure_win)
		return;
	
	cdda_configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(cdda_configure_win), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &cdda_configure_win);
	gtk_window_set_title(GTK_WINDOW(cdda_configure_win),
			     _("CD Audio Player Configuration"));
	gtk_window_set_policy(GTK_WINDOW(cdda_configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(cdda_configure_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(cdda_configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdda_configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/*
	 * Device config
	 */
	dev_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

	dev_notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(dev_notebook), TRUE);
	gtk_box_pack_start(GTK_BOX(dev_vbox), dev_notebook, FALSE, FALSE, 0);

	for (node = cdda_cfg.drives; node; node = node->next)
	{
		struct driveinfo *drive = node->data;
		char *label = g_strdup_printf(_("Drive %d"), i++);
		GtkWidget *w;

		w = configurewin_add_drive(drive, dev_notebook);
		gtk_notebook_append_page(GTK_NOTEBOOK(dev_notebook), w,
					 gtk_label_new(label));
		g_free(label);

	}

	add_bbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(dev_vbox), add_bbox, FALSE, FALSE, 0);
	add_drive = gtk_button_new_with_label(_("Add drive"));
	gtk_signal_connect(GTK_OBJECT(add_drive), "clicked",
			   GTK_SIGNAL_FUNC(configurewin_add_page), dev_notebook);
	GTK_WIDGET_SET_FLAGS(add_drive, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(add_bbox), add_drive, FALSE, FALSE, 0);

	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox,
				 gtk_label_new(_("Device")));

	/*
	 * CD Info config
	 */
	cdi_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(cdi_vbox), 5);


	/* CDDB */
	cdi_cddb_frame = gtk_frame_new(_("CDDB:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cddb_frame, FALSE, FALSE, 0);

	cdi_cddb_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cddb_vbox), 5);
	gtk_container_add(GTK_CONTAINER(cdi_cddb_frame), cdi_cddb_vbox);

	cdi_cddb_hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cddb_hbox), 0);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox),
			   cdi_cddb_hbox, FALSE, FALSE, 0);
	cdi_use_cddb = gtk_check_button_new_with_label(_("Use CDDB"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cddb),
				     cdda_cfg.use_cddb);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_hbox), cdi_use_cddb, FALSE, FALSE, 0);
	cdi_cddb_server_list = gtk_button_new_with_label(_("Get server list"));
	gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox), cdi_cddb_server_list, FALSE, FALSE, 0);
	cdi_cddb_debug_win = gtk_button_new_with_label(_("Show network window"));
	gtk_signal_connect(GTK_OBJECT(cdi_cddb_debug_win), "clicked",
			   GTK_SIGNAL_FUNC(cdda_cddb_show_network_window), NULL);
	gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox),
			 cdi_cddb_debug_win, FALSE, FALSE, 0);

	cdi_cddb_server_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox),
			   cdi_cddb_server_hbox, FALSE, FALSE, 0);

	cdi_cddb_server_label = gtk_label_new(_("CDDB server:"));
	gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox),
			   cdi_cddb_server_label, FALSE, FALSE, 0);

	cdi_cddb_server = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_cddb_server), cdda_cfg.cddb_server);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox), cdi_cddb_server, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(cdi_cddb_server_list), "clicked",
			   GTK_SIGNAL_FUNC(cdda_cddb_show_server_dialog),
			   cdi_cddb_server);

#if 0
	/*
	 * CDindex
	 */
	cdi_cdin_frame = gtk_frame_new(_("CD Index:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cdin_frame, FALSE, FALSE, 0);

	cdi_cdin_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cdin_vbox), 5);
	gtk_container_add(GTK_CONTAINER(cdi_cdin_frame), cdi_cdin_vbox);

	cdi_use_cdin = gtk_check_button_new_with_label(_("Use CD Index"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cdin), cdda_cfg.use_cdin);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_use_cdin, FALSE, FALSE, 0);

	cdi_cdin_server_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_cdin_server_hbox, FALSE, FALSE, 0);

	cdi_cdin_server_label = gtk_label_new(_("CD Index server:"));
	gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox), cdi_cdin_server_label,
			   FALSE, FALSE, 0);

	cdi_cdin_server = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_cdin_server), cdda_cfg.cdin_server);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox), cdi_cdin_server, TRUE, TRUE, 0);
#ifndef WITH_CDINDEX
	gtk_widget_set_sensitive(cdi_cdin_frame, FALSE);
#endif
#endif

	/*
	 * Track names
	 */
	cdi_name_frame = gtk_frame_new(_("Track names:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_name_frame, FALSE, FALSE, 0);

	cdi_name_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdi_name_frame), cdi_name_vbox);
	gtk_container_border_width(GTK_CONTAINER(cdi_name_vbox), 5);
	cdi_name_override = gtk_check_button_new_with_label(_("Override generic titles"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_name_override),
				     cdda_cfg.title_override);
	gtk_box_pack_start(GTK_BOX(cdi_name_vbox), cdi_name_override, FALSE, FALSE, 0);

	cdi_name_enable_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdi_name_vbox), cdi_name_enable_vbox);
	gtk_widget_set_sensitive(cdi_name_enable_vbox, cdda_cfg.title_override);
	gtk_signal_connect(GTK_OBJECT(cdi_name_override), "toggled",
			   toggle_set_sensitive_cb, cdi_name_enable_vbox);

	cdi_name_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_name_hbox, FALSE, FALSE, 0);
	cdi_name_label = gtk_label_new(_("Name format:"));
	gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name_label, FALSE, FALSE, 0);
	cdi_name = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_name), cdda_cfg.name_format);
	gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name, TRUE, TRUE, 0);

	cdi_desc = xmms_titlestring_descriptions("patn", 2);
	gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_desc, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cdi_vbox,
				 gtk_label_new(_("CD Info")));

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(cdda_configurewin_ok_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(configurewin_close), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
			   GTK_SIGNAL_FUNC(configurewin_close), NULL);
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(cdda_configure_win);
}
