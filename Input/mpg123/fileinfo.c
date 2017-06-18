/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <libxmms/xentry.h>
#include <gdk/gdkkeysyms.h>
#include "mpg123.h"

static GtkWidget *window = NULL;
static GtkWidget *filename_entry, *id3_frame;
static GtkWidget *title_entry, *artist_entry, *album_entry, *year_entry;
static GtkWidget *tracknum_entry, *comment_entry, *genre_combo;
static GtkWidget *mpeg_level, *mpeg_bitrate, *mpeg_samplerate, *mpeg_flags;
static GtkWidget *mpeg_fileinfo;

static GList *genre_list;
struct genre_item {
	const char *name;
	int id;
};
static int current_genre;
static char *current_filename;

extern char *mpg123_filename;
extern int mpg123_bitrate, mpg123_frequency, mpg123_layer, mpg123_lsf, mpg123_mode;
extern gboolean mpg123_stereo, mpg123_mpeg25;

#define MAX_STR_LEN 100

static void label_set_text(GtkWidget * label, char *str, ...)
G_GNUC_PRINTF(2, 3);

static void set_entry_tag(GtkEntry * entry, char * tag, int length)
{
	char *text = g_strchomp(g_strndup(tag, length));
	gtk_entry_set_text(entry, text);
	g_free(text);
}

static void get_entry_tag(GtkEntry * entry, char * tag, int length)
{
	strncpy(tag, gtk_entry_get_text(entry), length);
}

static int genre_find_index(GList *genre_list, int id)
{
	int idx = 0;
	while (genre_list)
	{
		struct genre_item *item = genre_list->data;
		if (item->id == id)
			break;
		idx++;
		genre_list = genre_list->next;
	}
	return idx;
}

static int genre_comp_func(gconstpointer a, gconstpointer b)
{
	const struct genre_item *ga = a, *gb = b;
	return strcasecmp(ga->name, gb->name);
}

static void save_cb(GtkWidget * w, gpointer data)
{
	int fd;
	struct id3v1tag_t tag;
	char *msg = NULL;

	if (!strncasecmp(current_filename, "http://", 7))
		return;

	if ((fd = open(current_filename, O_RDWR)) != -1)
	{
		int tracknum;

		lseek(fd, -128, SEEK_END);
		read(fd, &tag, sizeof (struct id3v1tag_t));

		if (!strncmp(tag.tag, "TAG", 3))
			lseek(fd, -128, SEEK_END);
		else
			lseek(fd, 0, SEEK_END);
		tag.tag[0] = 'T';
		tag.tag[1] = 'A';
		tag.tag[2] = 'G';
		get_entry_tag(GTK_ENTRY(title_entry), tag.title, 30);
		get_entry_tag(GTK_ENTRY(artist_entry), tag.artist, 30);
		get_entry_tag(GTK_ENTRY(album_entry), tag.album, 30);
		get_entry_tag(GTK_ENTRY(year_entry), tag.year, 4);
		tracknum = atoi(gtk_entry_get_text(GTK_ENTRY(tracknum_entry)));
		if (tracknum > 0)
		{
			get_entry_tag(GTK_ENTRY(comment_entry),
				      tag.u.v1_1.comment, 28);
			tag.u.v1_1.__zero = 0;
			tag.u.v1_1.track_number = MIN(tracknum, 255);
		}
		else
			get_entry_tag(GTK_ENTRY(comment_entry),
				      tag.u.v1_0.comment, 30);
		tag.genre = current_genre;
		if (write(fd, &tag, sizeof (tag)) != sizeof (tag))
			msg = g_strdup_printf(_("%s\nUnable to write to file: %s"),
					      _("Couldn't write tag!"),
					      strerror(errno));
		close(fd);
	}
	else
		msg = g_strdup_printf(_("%s\nUnable to open file: %s"),
				      _("Couldn't write tag!"),
				      strerror(errno));
	if (msg)
	{
		GtkWidget *mwin = xmms_show_message(_("File Info"), msg,
						    _("OK"), FALSE, NULL, NULL);
		gtk_window_set_transient_for(GTK_WINDOW(mwin),
					     GTK_WINDOW(window));
		g_free(msg);
	}
	else
		gtk_widget_destroy(window);
}

static void label_set_text(GtkWidget * label, char *str, ...)
{
	va_list args;
	char tempstr[MAX_STR_LEN];

	va_start(args, str);
	g_vsnprintf(tempstr, MAX_STR_LEN, str, args);
	va_end(args);

	gtk_label_set_text(GTK_LABEL(label), tempstr);
}

static void remove_id3_cb(GtkWidget * w, gpointer data)
{
	int fd, len;
	struct id3v1tag_t tag;
	char *msg = NULL;

	if (!strncasecmp(current_filename, "http://", 7))
		return;

	if ((fd = open(current_filename, O_RDWR)) != -1)
	{
		len = lseek(fd, -128, SEEK_END);
		read(fd, &tag, sizeof (struct id3v1tag_t));

		if (!strncmp(tag.tag, "TAG", 3))
		{
			if (ftruncate(fd, len))
				msg = g_strdup_printf(
					_("%s\n"
					  "Unable to truncate file: %s"),
					_("Couldn't remove tag!"),
					strerror(errno));
		}
		else
			msg = strdup(_("No tag to remove!"));
		close(fd);
	}
	else
		msg = g_strdup_printf(_("%s\nUnable to open file: %s"),
				      _("Couldn't remove tag!"),
				      strerror(errno));
	if (msg)
	{
		GtkWidget *mwin = xmms_show_message(_("File Info"), msg,
						    _("OK"), FALSE, NULL, NULL);
		gtk_window_set_transient_for(GTK_WINDOW(mwin),
					     GTK_WINDOW(window));
		g_free(msg);
	}
	else
		gtk_widget_destroy(window);
}

static void set_mpeg_level_label(gboolean mpeg25, int lsf, int layer)
{
	if (mpeg25)
		label_set_text(mpeg_level, "MPEG 2.5, layer %d", layer);
	else
		label_set_text(mpeg_level, "MPEG %d, layer %d", lsf + 1, layer);
}

static char* channel_mode_name(int mode)
{
	static const char *channel_mode[] =
		{N_("Stereo"), N_("Joint stereo"),
		 N_("Dual channel"), N_("Single channel")};
	if (mode < 0 || mode > 3)
		return "";
	return gettext(channel_mode[mode]);
}

static void file_info_http(char *filename)
{
	gtk_widget_set_sensitive(id3_frame, FALSE);
	if (mpg123_filename && !strcmp(filename, mpg123_filename) &&
	    mpg123_bitrate != 0)
	{
		set_mpeg_level_label(mpg123_mpeg25, mpg123_lsf, mpg123_layer);
		label_set_text(mpeg_bitrate, _("Bitrate: %d kb/s"),
			       mpg123_bitrate);
		label_set_text(mpeg_samplerate, _("Samplerate: %d Hz"),
			       mpg123_frequency);
		label_set_text(mpeg_flags, "%s",
			       channel_mode_name(mpg123_mode));
	}
}

static void genre_selected(GtkList *list, GtkWidget *w, gpointer data)
{
	void * p;
	p = gtk_object_get_data(GTK_OBJECT(w), "genre_id");
	if (p != NULL)
		current_genre = GPOINTER_TO_INT(p);
	else
		current_genre = 0;
}

static void genre_set_popdown(GtkWidget *combo, GList *genres)
{
	struct genre_item *item;
	GtkWidget *w;

	while (genres)
	{
		item = genres->data;
		w = gtk_list_item_new_with_label(item->name);
		gtk_object_set_data(GTK_OBJECT(w), "genre_id",
				    GINT_TO_POINTER(item->id));
		gtk_widget_show(w);
		gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), w);
		genres = genres->next;
	}
}

static void file_info_box_keypress_cb(GtkWidget *w, GdkEventKey *event, gpointer data)
{
	if (event && event->keyval == GDK_Escape)
		gtk_widget_destroy(w);
}

void mpg123_file_info_box(char *filename)
{
	int i;
	struct id3v1tag_t tag;
	FILE *fh;
	char *tmp, *title;
	const char *emphasis[4];
	const char *bool_label[2];

	emphasis[0] = _("None");
	emphasis[1] = _("50/15 ms");
	emphasis[2] = "";
	emphasis[3] = _("CCITT J.17");
	bool_label[0] = _("No");
	bool_label[1] = _("Yes");

	if (!window)
	{
		GtkWidget *vbox, *hbox, *left_vbox, *table;
		GtkWidget *mpeg_frame, *mpeg_box;
		GtkWidget *label, *filename_hbox;
		GtkWidget *bbox, *save, *remove_id3, *cancel;
		
		window = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
		gtk_signal_connect(GTK_OBJECT(window), "destroy",
				   gtk_widget_destroyed, &window);
		gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
				   file_info_box_keypress_cb, NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(window), vbox);

		filename_hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), filename_hbox,
				   FALSE, TRUE, 0);

		label = gtk_label_new(_("Filename:"));
		gtk_box_pack_start(GTK_BOX(filename_hbox), label,
				   FALSE, TRUE, 0);
		filename_entry = xmms_entry_new();
		gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(filename_hbox),
				   filename_entry, TRUE, TRUE, 0);
		
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
		
		left_vbox = gtk_vbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);

		id3_frame = gtk_frame_new(_("ID3 Tag:"));
		gtk_box_pack_start(GTK_BOX(left_vbox), id3_frame,
				   FALSE, FALSE, 0);

		table = gtk_table_new(5, 5, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(id3_frame), table);

		label = gtk_label_new(_("Title:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
				 GTK_FILL, GTK_FILL, 5, 5);

		title_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), title_entry, 1, 4, 0, 1,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Artist:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
				 GTK_FILL, GTK_FILL, 5, 5);

		artist_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), artist_entry, 1, 4, 1, 2,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Album:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
				 GTK_FILL, GTK_FILL, 5, 5);

		album_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), album_entry, 1, 4, 2, 3,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Comment:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
				 GTK_FILL, GTK_FILL, 5, 5);

		comment_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), comment_entry, 1, 4, 3, 4,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Year:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
				 GTK_FILL, GTK_FILL, 5, 5);

		year_entry = gtk_entry_new_with_max_length(4);
		gtk_widget_set_usize(year_entry, 40, -1);
		gtk_table_attach(GTK_TABLE(table), year_entry, 1, 2, 4, 5,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Track number:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5,
				 GTK_FILL, GTK_FILL, 5, 5);

		tracknum_entry = gtk_entry_new_with_max_length(3);
		gtk_widget_set_usize(tracknum_entry, 40, -1);
		gtk_table_attach(GTK_TABLE(table), tracknum_entry, 3, 4, 4, 5,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Genre:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
				 GTK_FILL, GTK_FILL, 5, 5);

		genre_combo = gtk_combo_new();
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(genre_combo)->entry),
				       FALSE);
		if (!genre_list)
		{
			struct genre_item *item;

			for (i = 0; i < GENRE_MAX; i++)
			{
				item = g_malloc(sizeof (*item));
				item->name = gettext(mpg123_id3_genres[i]);
				item->id = i;
				genre_list = g_list_prepend(genre_list, item);
			}
			item = g_malloc(sizeof (*item));
			item->name = "";
			item->id = 0xff;
			genre_list = g_list_prepend(genre_list, item);
			genre_list = g_list_sort(genre_list, genre_comp_func);
		}
		genre_set_popdown(genre_combo, genre_list);
		gtk_signal_connect(GTK_OBJECT(GTK_COMBO(genre_combo)->list),
				   "select-child", genre_selected, NULL);

		gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 4, 5, 6,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox),
					  GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
		gtk_box_pack_start(GTK_BOX(left_vbox), bbox, FALSE, FALSE, 0);

		save = gtk_button_new_with_label(_("Save"));
		gtk_signal_connect(GTK_OBJECT(save), "clicked", save_cb, NULL);
		GTK_WIDGET_SET_FLAGS(save, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), save, TRUE, TRUE, 0);
		gtk_widget_grab_default(save);

		remove_id3 = gtk_button_new_with_label(_("Remove ID3"));
		gtk_signal_connect(GTK_OBJECT(remove_id3), "clicked",
				   remove_id3_cb, NULL);
		GTK_WIDGET_SET_FLAGS(remove_id3, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), remove_id3, TRUE, TRUE, 0);

		cancel = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
					  gtk_widget_destroy, GTK_OBJECT(window));
		GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

		mpeg_frame = gtk_frame_new(_("MPEG Info:"));
		gtk_box_pack_start(GTK_BOX(hbox), mpeg_frame, FALSE, FALSE, 0);

		mpeg_box = gtk_vbox_new(FALSE, 5);
		gtk_container_add(GTK_CONTAINER(mpeg_frame), mpeg_box);
		gtk_container_set_border_width(GTK_CONTAINER(mpeg_box), 10);
		gtk_box_set_spacing(GTK_BOX(mpeg_box), 0);

		mpeg_level = gtk_label_new("");
		gtk_widget_set_usize(mpeg_level, 120, -2);
		gtk_misc_set_alignment(GTK_MISC(mpeg_level), 0, 0);
		gtk_box_pack_start(GTK_BOX(mpeg_box), mpeg_level, FALSE, FALSE, 0);

		mpeg_bitrate = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(mpeg_bitrate), 0, 0);
		gtk_label_set_justify(GTK_LABEL(mpeg_bitrate),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(mpeg_box),
				   mpeg_bitrate, FALSE, FALSE, 0);

		mpeg_samplerate = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(mpeg_samplerate), 0, 0);
		gtk_box_pack_start(GTK_BOX(mpeg_box), mpeg_samplerate,
				   FALSE, FALSE, 0);

		mpeg_flags = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(mpeg_flags), 0, 0);
		gtk_label_set_justify(GTK_LABEL(mpeg_flags), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(mpeg_box), mpeg_flags,
				   FALSE, FALSE, 0);

		mpeg_fileinfo = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(mpeg_fileinfo), 0, 0);
		gtk_label_set_justify(GTK_LABEL(mpeg_fileinfo),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(mpeg_box), mpeg_fileinfo,
				   FALSE, FALSE, 0);

		gtk_widget_show_all(window);
	}

	if (current_filename)
		g_free(current_filename);
	current_filename = g_strdup(filename);

	title = g_strdup_printf(_("File Info - %s"), g_basename(filename));
	gtk_window_set_title(GTK_WINDOW(window), title);
	g_free(title);

	gtk_entry_set_text(GTK_ENTRY(filename_entry), filename);
	gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

	title = g_strdup(g_basename(filename));
	if ((tmp = strrchr(title, '.')) != NULL)
		*tmp = '\0';
	gtk_entry_set_text(GTK_ENTRY(title_entry), title);
	g_free(title);

	gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
	gtk_entry_set_text(GTK_ENTRY(album_entry), "");
	gtk_entry_set_text(GTK_ENTRY(year_entry), "");
	gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
	gtk_entry_set_text(GTK_ENTRY(comment_entry), "");
	gtk_list_select_item(GTK_LIST(GTK_COMBO(genre_combo)->list),
			     genre_find_index(genre_list, 0xff));
	gtk_label_set_text(GTK_LABEL(mpeg_level), "MPEG ?, layer ?");
	gtk_label_set_text(GTK_LABEL(mpeg_bitrate), "");
	gtk_label_set_text(GTK_LABEL(mpeg_samplerate), "");
	gtk_label_set_text(GTK_LABEL(mpeg_flags), "");
	gtk_label_set_text(GTK_LABEL(mpeg_fileinfo), "");

	if (!strncasecmp(filename, "http://", 7))
	{
		file_info_http(filename);
		return;
	}

	gtk_widget_set_sensitive(id3_frame, TRUE);

	if ((fh = fopen(current_filename, "rb")) != NULL)
	{
		struct frame frm;
		gboolean id3_found = FALSE;
		guint8 *buf;
		double tpf;
		int pos;
		xing_header_t xing_header;
		guint32 num_frames;

		fseek(fh, -sizeof (tag), SEEK_END);
		if (fread(&tag, 1, sizeof (tag), fh) == sizeof (tag))
		{
			if (!strncmp(tag.tag, "TAG", 3))
			{
				id3_found = TRUE;
				set_entry_tag(GTK_ENTRY(title_entry),
					      tag.title, 30);
				set_entry_tag(GTK_ENTRY(artist_entry),
					      tag.artist, 30);
				set_entry_tag(GTK_ENTRY(album_entry),
					      tag.album, 30);
				set_entry_tag(GTK_ENTRY(year_entry),
					      tag.year, 4);
				/* Check for v1.1 tags */
				if (tag.u.v1_1.__zero == 0 && tag.u.v1_1.track_number > 0)
				{
					char *temp = g_strdup_printf("%d", tag.u.v1_1.track_number);
					set_entry_tag(GTK_ENTRY(comment_entry),
						      tag.u.v1_1.comment, 28);
					gtk_entry_set_text(GTK_ENTRY(tracknum_entry), temp);
					g_free(temp);
				}
				else
				{
					set_entry_tag(GTK_ENTRY(comment_entry),
						      tag.u.v1_0.comment, 30);
					gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
				}
				
				gtk_list_select_item(GTK_LIST(GTK_COMBO(genre_combo)->list), genre_find_index(genre_list, tag.genre));
			}
		}
		rewind(fh);
		
		if (!mpg123_get_first_frame(fh, &frm, &buf))
		{
			fclose(fh);
			return;
		}

		tpf = mpg123_compute_tpf(&frm);
		set_mpeg_level_label(frm.mpeg25, frm.lsf, frm.lay);
		pos = ftell(fh);
		fseek(fh, 0, SEEK_END);
		if (mpg123_get_xing_header(&xing_header, buf))
		{
			num_frames = xing_header.frames;
			label_set_text(mpeg_bitrate,
				       _("Bitrate: Variable,\n"
					 "avg. bitrate: %d kb/s"),
				       (int)((xing_header.bytes * 8) /
					     (tpf * xing_header.frames * 1000)));
		}
		else
		{
			num_frames = ((ftell(fh) - pos - (id3_found ? 128 : 0)) /
				      mpg123_compute_bpf(&frm)) + 1;
			label_set_text(mpeg_bitrate,
				       _("Bitrate: %d kb/s"),
				       tabsel_123[frm.lsf][frm.lay - 1][frm.bitrate_index]);
		}
		label_set_text(mpeg_samplerate, _("Samplerate: %d Hz"),
			       mpg123_freqs[frm.sampling_frequency]);
		label_set_text(mpeg_flags, _("%s\nError protection: %s\n"
					     "Copyright: %s\nOriginal: %s\n"
					     "Emphasis: %s"),
			       channel_mode_name(frm.mode),
			       bool_label[frm.error_protection],
			       bool_label[frm.copyright], bool_label[frm.original],
			       emphasis[frm.emphasis]);
		label_set_text(mpeg_fileinfo, _("%d frames\nFilesize: %lu B"),
			       num_frames, ftell(fh));
		g_free(buf);
		fclose(fh);
	}
}
