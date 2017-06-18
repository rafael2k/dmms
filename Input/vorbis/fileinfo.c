/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
 *  Copyright (C) 2001, Jorn Baayen <jorn@nl.linux.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "libxmms/util.h"
#include "libxmms/charset.h"
#include <xmms/i18n.h>

#include "vorbis.h"
#include "vcedit.h"

static struct vte_struct {
	FILE *in;
	gchar *filename;
} vte;

static void fail(gchar *error);
static void save_cb(GtkWidget * w, gpointer data);
static void remove_cb(GtkWidget * w, gpointer data);
static gint init_files(vcedit_state *state);
static gint close_files(vcedit_state *state);

extern pthread_mutex_t vf_mutex;
static GtkWidget *window = NULL;
static GList *genre_list = NULL;

static GtkWidget *title_entry, *album_entry, *performer_entry;
static GtkWidget *tracknumber_entry, *date_entry;
static GtkWidget *genre_combo, *user_comment_entry;
#ifdef ALL_VORBIS_TAGS
static GtkWidget *description_entry, *version_entry, *isrc_entry;
static GtkWidget *copyright_entry, *organization_entry, *location_entry;
#endif
static GtkWidget *rg_track_entry, *rg_album_entry, *rg_track_peak_entry, *rg_album_peak_entry;
static GtkWidget *rg_track_label, *rg_album_label, *rg_track_peak_label, *rg_album_peak_label;
static GtkWidget *rg_show_button;

static GtkWidget *bitrate_label, *avgbitrate_label, *rate_label;
static GtkWidget *channel_label, *length_label, *filesize_label;
static GtkWidget *replaygain_label, *audiophilegain_label, *peak_label;
static GtkWidget *filename_entry, *tag_frame, *vendor_label;


/* From mpg123.c, as no standardized Ogg Vorbis genres exists. */
static const gchar *vorbis_genres[] =
{
	N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
	N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
	N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
	N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
	N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
	N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
	N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
	N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
	N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
	N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("AlternRock"),
	N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
	N_("Meditative"), N_("Instrumental Pop"),
	N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
	N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
	N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
	N_("Southern Rock"), N_("Comedy"), N_("Cult"),
	N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
	N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
	N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
	N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
	N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
	N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
	N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
	N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
	N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
	N_("Gothic Rock"), N_("Progressive Rock"),
	N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
	N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
	N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
	N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
	N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
	N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
	N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
	N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
	N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
	N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
	N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
	N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
	N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
	N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
	N_("Contemporary Christian"), N_("Christian Rock"),
	N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
	N_("Anime"), N_("JPop"), N_("Synthpop")
};

static gchar* get_comment(vorbis_comment *vc, gchar *label)
{
	gchar *tag;
	if (vc && (tag = vorbis_comment_query(vc, label, 0)) != NULL)
		return xmms_charset_from_utf8(tag);
	else
		return g_strdup("");
}

static char** get_comment_list(vorbis_comment *vc)
{
	int i;
	char **strv;

	strv = g_new0(char*, vc->comments + 1);
	for (i = 0; i < vc->comments; i++)
	{
		strv[i] = g_strdup(vc->user_comments[i]);
	}

	return strv;
}

static char** add_tag(char **list, char *label, char *tag)
{
	char **ptr = list, *reallabel = g_strconcat(label, "=", NULL);

	g_strstrip(tag);
	if (strlen(tag) == 0)
		tag = NULL;
	/*
	 * There can be several tags with the same label.  We clear
	 * them all.
	 */
	while (*ptr != NULL)
	{
		if (!g_strncasecmp(reallabel, *ptr, strlen(reallabel)))
		{
			g_free(*ptr);
			if (tag != NULL)
			{
				tag = xmms_charset_to_utf8(tag);
				*ptr = g_strconcat(reallabel, tag, NULL);
				g_free(tag);
				tag = NULL;
				ptr++;
			}
			else
			{
				char **str;
				for (str = ptr; *str; str++)
					*str = *(str + 1);
			}
			
		}
		else
			ptr++;
	}
	if (tag)
	{
		int i = 0;
		for (ptr = list; *ptr; ptr++)
			i++;
		list = g_renew(char*, list, i + 2);
		tag = xmms_charset_to_utf8(tag);
		list[i] = g_strconcat(reallabel, tag, NULL);
		list[i + 1] = NULL;
		g_free(tag);
	}
	g_free(reallabel);

	return list;
}

static void add_list(vorbis_comment *vc, char **comments)
{
	while (*comments)
		vorbis_comment_add(vc, *comments++);
}


static void fail(char *error)
{
	char *errorstring;
	errorstring = g_strdup_printf(_("An error occurred:\n%s"), error);
	
	xmms_show_message(_("Error!"), errorstring, _("OK"), FALSE, NULL, NULL);

	g_free(errorstring);
	return;
}


static void save_cb(GtkWidget * w, gpointer data)
{
	gchar *track_name, *performer, *album_name, *date, *track_number;
	gchar *genre, *user_comment;
#ifdef ALL_VORBIS_TAGS
	gchar *description, *version, *isrc, *copyright, *organization;
	gchar *location;
#endif
	gchar *rg_track_gain, *rg_album_gain, *rg_track_peak, *rg_album_peak;
	char **comment_list;
	vcedit_state *state;
	vorbis_comment *comment;

	if (!g_strncasecmp(vte.filename, "http://", 7))
		return;
	
	state = vcedit_new_state();

	pthread_mutex_lock(&vf_mutex);
	if(init_files(state) < 0)
	{
		fail(_("Failed to modify tag"));
		goto close;
	}

	comment = vcedit_comments(state);

	comment_list = get_comment_list(comment);
	
	vorbis_comment_clear(comment);
	
	track_name = gtk_entry_get_text(GTK_ENTRY(title_entry));
	performer = gtk_entry_get_text(GTK_ENTRY(performer_entry));
	album_name = gtk_entry_get_text(GTK_ENTRY(album_entry));
	track_number = gtk_entry_get_text(GTK_ENTRY(tracknumber_entry));
	genre = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry));
	date = gtk_entry_get_text(GTK_ENTRY(date_entry));
	user_comment = gtk_entry_get_text(GTK_ENTRY(user_comment_entry));
#ifdef ALL_VORBIS_TAGS
	location = gtk_entry_get_text(GTK_ENTRY(location_entry));
	description = gtk_entry_get_text(GTK_ENTRY(description_entry));
	version = gtk_entry_get_text(GTK_ENTRY(version_entry));
	isrc = gtk_entry_get_text(GTK_ENTRY(isrc_entry));
	organization = gtk_entry_get_text(GTK_ENTRY(organization_entry));
	copyright = gtk_entry_get_text(GTK_ENTRY(copyright_entry));
#endif
	rg_track_gain = gtk_entry_get_text(GTK_ENTRY(rg_track_entry));
	rg_album_gain = gtk_entry_get_text(GTK_ENTRY(rg_album_entry));
	rg_track_peak = gtk_entry_get_text(GTK_ENTRY(rg_track_peak_entry));
	rg_album_peak = gtk_entry_get_text(GTK_ENTRY(rg_album_peak_entry));

	comment_list = add_tag(comment_list, "title", track_name);
	comment_list = add_tag(comment_list, "artist", performer);
	comment_list = add_tag(comment_list, "album", album_name);
	comment_list = add_tag(comment_list, "tracknumber", track_number);
	comment_list = add_tag(comment_list, "genre", genre);
	comment_list = add_tag(comment_list, "date", date);
	comment_list = add_tag(comment_list, "comment", user_comment);

#ifdef ALL_VORBIS_TAGS
	comment_list = add_tag(comment_list, "location", location);
	comment_list = add_tag(comment_list, "description", description);
	comment_list = add_tag(comment_list, "version", version);
	comment_list = add_tag(comment_list, "isrc", isrc);
	comment_list = add_tag(comment_list, "organization", organization);
	comment_list = add_tag(comment_list, "copyright", copyright);
#endif
	comment_list = add_tag(comment_list, "replaygain_track_gain", rg_track_gain);
	comment_list = add_tag(comment_list, "replaygain_album_gain", rg_album_gain);
	comment_list = add_tag(comment_list, "replaygain_track_peak", rg_track_peak);
	comment_list = add_tag(comment_list, "replaygain_album_peak", rg_album_peak);

	add_list(comment, comment_list);
	g_strfreev(comment_list);

	if (close_files(state) < 0) 
		fail(_("Failed to modify tag"));
	
close:
	vcedit_clear(state);
	pthread_mutex_unlock(&vf_mutex);
	gtk_widget_destroy(window);
}

static void remove_cb(GtkWidget * w, gpointer data)
{
	vcedit_state *state;
	vorbis_comment *comment;

	if (!g_strncasecmp(vte.filename, "http://", 7))
		return;

	state = vcedit_new_state();

	pthread_mutex_lock(&vf_mutex);
	if (init_files(state) < 0)
	{
		fail(_("Failed to modify tag"));
		goto close;
	}

	comment = vcedit_comments(state);

	vorbis_comment_clear(comment);

	if (close_files(state) < 0) 
		fail(_("Failed to modify tag"));

close:
	vcedit_clear(state);
	pthread_mutex_unlock(&vf_mutex);
	gtk_widget_destroy(window);
}

static void rg_show_cb(GtkWidget * w, gpointer data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rg_show_button)))
	{
		gtk_widget_show(rg_track_label);
		gtk_widget_show(rg_track_entry);
		gtk_widget_show(rg_album_label);
		gtk_widget_show(rg_album_entry);
		gtk_widget_show(rg_track_peak_label);
		gtk_widget_show(rg_track_peak_entry);
		gtk_widget_show(rg_album_peak_label);
		gtk_widget_show(rg_album_peak_entry);
	}
	else
	{
		gtk_widget_hide(rg_track_label);
		gtk_widget_hide(rg_track_entry);
		gtk_widget_hide(rg_album_label);
		gtk_widget_hide(rg_album_entry);
		gtk_widget_hide(rg_track_peak_label);
		gtk_widget_hide(rg_track_peak_entry);
		gtk_widget_hide(rg_album_peak_label);
		gtk_widget_hide(rg_album_peak_entry);
	}
}

static gint init_files(vcedit_state *state)
{
	if ((vte.in = fopen(vte.filename, "rb")) == NULL)
		return -1;

	if (vcedit_open(state, vte.in) < 0)
	{
		fclose(vte.in);
		return -1;
	}
	return 0;
}

static gint close_files(vcedit_state *state)
{
	int retval = 0, ofh;
	char *tmpfn;
	FILE* out;
	
	tmpfn = g_strdup_printf("%s.XXXXXX", vte.filename);

	if ((ofh = mkstemp(tmpfn)) < 0)
	{
		g_free(tmpfn);
		fclose(vte.in);
		return -1;
	}

	if ((out = fdopen(ofh, "wb")) == NULL)
	{
		close(ofh);
		remove(tmpfn);
		g_free(tmpfn);
		fclose(vte.in);
		return -1;
	}

	if (vcedit_write(state, out) < 0)
	{
		g_warning("vcedit_write: %s", state->lasterror);
		retval = -1;
	}

	fclose(vte.in);

	if (fclose(out) != 0)
		retval = -1;

	if (retval < 0 || rename(tmpfn, vte.filename) < 0)
	{
		remove(tmpfn);
		retval = -1;
	}

	g_free(tmpfn);
	return retval;
}


static void label_set_text(GtkWidget * label, char *str, ...)
{
	va_list args;
	gchar *tempstr;

	va_start(args, str);
	tempstr = g_strdup_vprintf(str, args);
	va_end(args);

	gtk_label_set_text(GTK_LABEL(label), tempstr);
	g_free(tempstr);
}

static void keypress_cb(GtkWidget * w, GdkEventKey * event, gpointer data)
{
	if (event && event->keyval == GDK_Escape)
		gtk_widget_destroy(w);
}

/***********************************************************************/

void vorbis_file_info_box(char *fn)
{
	char *track_name, *performer, *album_name, *date, *track_number;
	char *genre, *user_comment, *tmp;
	char *description, *version, *isrc, *copyright, *organization;
	char *location, *vendor = "N/A";
	char *rg_track_gain, *rg_album_gain, *rg_track_peak, *rg_album_peak;

	int time, minutes, seconds, bitrate, avgbitrate, rate, channels;
	int filesize, i;

	OggVorbis_File vf;
	vorbis_info *vi;
	vorbis_comment *comment = NULL;
	FILE *fh;
	gboolean clear_vf = FALSE;

	g_free(vte.filename);
	vte.filename = g_strdup(fn);
	
	if (!window)
	{
		GtkWidget *info_frame, *info_box;
		GtkWidget *hbox, *label, *filename_hbox, *vbox, *left_vbox;
		GtkWidget *table, *bbox, *cancel_button;
		GtkWidget *save_button, *remove_button;

		window = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
		gtk_signal_connect(GTK_OBJECT(window), "destroy", 
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &window);
		gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
				   keypress_cb, NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(window), vbox);

		filename_hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), filename_hbox, FALSE,
				   TRUE, 0);
		
		label = gtk_label_new(_("Filename:"));
		gtk_box_pack_start(GTK_BOX(filename_hbox), label, FALSE,
				   TRUE, 0);
		filename_entry = gtk_entry_new();
		gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(filename_hbox), filename_entry,
				   TRUE, TRUE, 0);

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
		
		left_vbox = gtk_vbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);

		tag_frame = gtk_frame_new(_("Ogg Vorbis Tag:"));
		gtk_box_pack_start(GTK_BOX(left_vbox), tag_frame, FALSE,
				   FALSE, 0);

		table = gtk_table_new(5, 5, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(tag_frame), table);
		
		label = gtk_label_new(_("Title:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
				 GTK_FILL, GTK_FILL, 5, 5);
		
		title_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), title_entry, 1, 4, 0, 1,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Artist:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
				 GTK_FILL, GTK_FILL, 5, 5);
		
		performer_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), performer_entry, 1, 4, 1, 2,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Album:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
				 GTK_FILL, GTK_FILL, 5, 5);
		
		album_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), album_entry, 1, 4, 2, 3,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Comment:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
				 GTK_FILL, GTK_FILL, 5, 5);

		user_comment_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), user_comment_entry, 1, 4, 3,
				 4, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Date:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
				 GTK_FILL, GTK_FILL, 5, 5);

		date_entry = gtk_entry_new();
		gtk_widget_set_usize(date_entry, 60, -1);
		gtk_table_attach(GTK_TABLE(table), date_entry, 1, 2, 4, 5,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
		
		label = gtk_label_new(_("Track number:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5,
				 GTK_FILL, GTK_FILL, 5, 5);
		
		tracknumber_entry = gtk_entry_new_with_max_length(4);
		gtk_widget_set_usize(tracknumber_entry, 20, -1);
		gtk_table_attach(GTK_TABLE(table), tracknumber_entry, 3, 4, 4,
				 5, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Genre:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
				 GTK_FILL, GTK_FILL, 5, 5);
		
		genre_combo = gtk_combo_new();
		if (!genre_list)
		{
			for (i = 0; i < sizeof(vorbis_genres)/sizeof(*vorbis_genres) ; i++)
				genre_list = g_list_prepend(genre_list, _(vorbis_genres[i]));
			genre_list = g_list_sort(genre_list, (GCompareFunc)g_strcasecmp);
		}
		gtk_combo_set_popdown_strings(GTK_COMBO(genre_combo),
					      genre_list);
		gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 4, 5, 6,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

#ifdef ALL_VORBIS_TAGS
		label = gtk_label_new(_("Description:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7,
				 GTK_FILL, GTK_FILL, 5, 5);

		description_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), description_entry, 1, 4, 6,
				 7, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Location:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8,
				 GTK_FILL, GTK_FILL, 5, 5);

		location_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), location_entry, 1, 4, 7, 8,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Version:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9,
				 GTK_FILL, GTK_FILL, 5, 5);

		version_entry = gtk_entry_new();
		gtk_widget_set_usize(version_entry, 60, -1);
		gtk_table_attach(GTK_TABLE(table), version_entry, 1, 2, 8, 9,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("ISRC number:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 8, 9,
				 GTK_FILL, GTK_FILL, 5, 5);

		isrc_entry = gtk_entry_new();
		gtk_widget_set_usize(isrc_entry, 20, -1);
		gtk_table_attach(GTK_TABLE(table), isrc_entry, 3, 4, 8, 9,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Organization:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 9, 10,
				 GTK_FILL, GTK_FILL, 5, 5);

		organization_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), organization_entry, 1, 4, 9,
				 10, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Copyright:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 10, 11,
				 GTK_FILL, GTK_FILL, 5, 5);

		copyright_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), copyright_entry, 1, 4, 10,
				 11, GTK_FILL |	GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
#endif

		rg_show_button = gtk_check_button_new_with_label(_("ReplayGain Settings:"));
		gtk_signal_connect(GTK_OBJECT(rg_show_button), "toggled",
				   GTK_SIGNAL_FUNC(rg_show_cb), NULL);
		gtk_table_attach(GTK_TABLE(table), rg_show_button, 0, 2, 11, 12,
				 GTK_FILL, GTK_FILL, 5, 5);


		rg_track_label = gtk_label_new(_("Track gain:"));
		gtk_misc_set_alignment(GTK_MISC(rg_track_label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), rg_track_label, 2, 3, 11, 12,
				 GTK_FILL, GTK_FILL, 5, 5);

		rg_track_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), rg_track_entry, 3, 4, 11,
				 12, GTK_FILL |	GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		rg_track_peak_label = gtk_label_new(_("Track peak:"));
		gtk_misc_set_alignment(GTK_MISC(rg_track_peak_label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), rg_track_peak_label, 2, 3, 12, 13,
				 GTK_FILL, GTK_FILL, 5, 5);

		rg_track_peak_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), rg_track_peak_entry, 3, 4, 12,
				 13, GTK_FILL |	GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);


		rg_album_label = gtk_label_new(_("Album gain:"));
		gtk_misc_set_alignment(GTK_MISC(rg_album_label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), rg_album_label, 2, 3, 13, 14,
				 GTK_FILL, GTK_FILL, 5, 5);

		rg_album_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), rg_album_entry, 3, 4, 13,
				 14, GTK_FILL |	GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		rg_album_peak_label = gtk_label_new(_("Album peak:"));
		gtk_misc_set_alignment(GTK_MISC(rg_album_peak_label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), rg_album_peak_label, 2, 3, 14, 15,
				 GTK_FILL, GTK_FILL, 5, 5);

		rg_album_peak_entry = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(table), rg_album_peak_entry, 3, 4, 14,
				 15, GTK_FILL |	GTK_EXPAND | GTK_SHRINK,
				 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);


		bbox = gtk_hbutton_box_new(); 
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox),
					  GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
		gtk_box_pack_start(GTK_BOX(left_vbox), bbox, FALSE, FALSE, 0);
		
		save_button = gtk_button_new_with_label(_("Save"));
		gtk_signal_connect(GTK_OBJECT(save_button), "clicked", 
				   GTK_SIGNAL_FUNC(save_cb), NULL);
		GTK_WIDGET_SET_FLAGS(save_button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), save_button, TRUE, TRUE, 0);
		gtk_widget_grab_default(save_button);

		remove_button = gtk_button_new_with_label(_("Remove Tag"));
		gtk_signal_connect_object(GTK_OBJECT(remove_button),
					  "clicked", 
					  GTK_SIGNAL_FUNC(remove_cb), NULL);
		GTK_WIDGET_SET_FLAGS(remove_button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox),remove_button, TRUE, TRUE, 0);

		cancel_button = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(cancel_button),
					  "clicked", 
					  GTK_SIGNAL_FUNC(gtk_widget_destroy),
					  GTK_OBJECT(window));
		GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox),cancel_button, TRUE, TRUE, 0);

		info_frame = gtk_frame_new(_("Ogg Vorbis Info:"));
		gtk_box_pack_start(GTK_BOX(hbox), info_frame, FALSE, FALSE, 0);

		info_box = gtk_vbox_new(FALSE, 5);
		gtk_container_add(GTK_CONTAINER(info_frame), info_box);
		gtk_container_set_border_width(GTK_CONTAINER(info_box), 10);
		gtk_box_set_spacing(GTK_BOX(info_box), 0);

		bitrate_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(bitrate_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(bitrate_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), bitrate_label, FALSE,
				   FALSE, 0);

		avgbitrate_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(avgbitrate_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(avgbitrate_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), avgbitrate_label, FALSE,
				   FALSE, 0);
		
		rate_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(rate_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(rate_label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), rate_label, FALSE,
				   FALSE, 0);

		channel_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(channel_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(channel_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), channel_label, FALSE,
				   FALSE, 0);

		length_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(length_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(length_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), length_label, FALSE,
				   FALSE, 0);

		filesize_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(filesize_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(filesize_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), filesize_label, FALSE,
				   FALSE, 0);
	
		vendor_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(vendor_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(vendor_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), vendor_label, FALSE,
				   FALSE, 0);

		replaygain_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(replaygain_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(replaygain_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), replaygain_label, FALSE,
				   FALSE, 0);

		audiophilegain_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(audiophilegain_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(audiophilegain_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), audiophilegain_label, FALSE,
				   FALSE, 0);

		peak_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(peak_label), 0, 0);
		gtk_label_set_justify(GTK_LABEL(peak_label),
				      GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(info_box), peak_label, FALSE,
				   FALSE, 0);
		
		gtk_widget_show_all(window);
	} else
		gdk_window_raise(window->window);

	if (!g_strncasecmp(vte.filename, "http://", 7))
		gtk_widget_set_sensitive(tag_frame, FALSE);
	else
		gtk_widget_set_sensitive(tag_frame, TRUE);		

	gtk_label_set_text(GTK_LABEL(bitrate_label), "");
	gtk_label_set_text(GTK_LABEL(avgbitrate_label), "");
	gtk_label_set_text(GTK_LABEL(rate_label), "");
	gtk_label_set_text(GTK_LABEL(channel_label), "");
	gtk_label_set_text(GTK_LABEL(length_label), "");
	gtk_label_set_text(GTK_LABEL(filesize_label), "");
	gtk_label_set_text(GTK_LABEL(vendor_label), "");

	if ((fh = fopen(vte.filename, "r")) != NULL)
	{
		pthread_mutex_lock(&vf_mutex);

		if (ov_open(fh, &vf, NULL, 0) == 0)
		{
			comment = ov_comment(&vf, -1);
			if (comment && comment->vendor)
				vendor = comment->vendor;

			if ((vi = ov_info(&vf, 0)) != NULL)
			{
				bitrate = vi->bitrate_nominal/1000;
				avgbitrate = ov_bitrate(&vf, -1);
				if (avgbitrate == OV_EINVAL ||
				    avgbitrate == OV_FALSE)
					avgbitrate = 0;
				rate = vi->rate;
				channels = vi->channels;
				clear_vf = TRUE;
			}
			else
			{
				bitrate = 0;
				avgbitrate = 0;
				rate = 0;
				channels = 0;
			}
	
			time = ov_time_total(&vf, -1);
			minutes = time / 60;
			seconds = time % 60;
			fseek(fh, 0, SEEK_END);
			filesize = ftell(fh);

			label_set_text(bitrate_label, _("Nominal bitrate: %d kbps"), bitrate);
			label_set_text(avgbitrate_label, _("Average bitrate: %.1f kbps"), ((float) avgbitrate) / 1000);
			label_set_text(rate_label, _("Samplerate: %d Hz"), rate);
			label_set_text(channel_label, _("Channels: %d"), channels);
			label_set_text(length_label, _("Length: %d:%.2d"), minutes, seconds);
			label_set_text(filesize_label, _("File size: %d B"), filesize);
			label_set_text(vendor_label, _("Vendor: %s"), vendor);
		}
		else
			fclose(fh);
	}


	track_name = get_comment(comment, "title");
	performer = get_comment(comment, "artist");
	album_name = get_comment(comment, "album");
	track_number = get_comment(comment, "tracknumber");
	genre = get_comment(comment, "genre");
	date = get_comment(comment, "date");
	user_comment = get_comment(comment, "comment");
	location = get_comment(comment, "location");
	description = get_comment(comment, "description");
	version = get_comment(comment, "version");
	isrc = get_comment(comment, "isrc");
	organization = get_comment(comment, "organization");
	copyright = get_comment(comment, "copyright");

	rg_track_gain = get_comment(comment, "replaygain_track_gain");
	if (*rg_track_gain == '\0')
	{
		g_free(rg_track_gain);
		rg_track_gain = get_comment(comment, "rg_radio"); /* Old */
	}
	rg_album_gain = get_comment(comment, "replaygain_album_gain");
	if (*rg_album_gain == '\0')
	{
		g_free(rg_album_gain);
		rg_album_gain = get_comment(comment, "rg_audiophile"); /* Old */
	}
	rg_track_peak = get_comment(comment, "replaygain_track_peak");
	if (*rg_track_peak == '\0')
	{
		g_free(rg_track_peak);
		rg_track_peak = get_comment(comment, "rg_peak"); /* Old */
	}
	rg_album_peak = get_comment(comment, "replaygain_album_peak"); /* Old had no album peak */

	/* ov_clear closes the file */
	if (clear_vf)
		ov_clear(&vf);
	pthread_mutex_unlock(&vf_mutex);

	/* Fill it all in .. */
	gtk_entry_set_text(GTK_ENTRY(title_entry), track_name);
	gtk_entry_set_text(GTK_ENTRY(performer_entry), performer);
	gtk_entry_set_text(GTK_ENTRY(album_entry), album_name);
	gtk_entry_set_text(GTK_ENTRY(user_comment_entry), user_comment);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry), genre);
	gtk_entry_set_text(GTK_ENTRY(tracknumber_entry), track_number);
	gtk_entry_set_text(GTK_ENTRY(date_entry), date);
#ifdef ALL_VORBIS_TAGS
	gtk_entry_set_text(GTK_ENTRY(version_entry), version);
	gtk_entry_set_text(GTK_ENTRY(description_entry), description);
	gtk_entry_set_text(GTK_ENTRY(organization_entry), organization);
	gtk_entry_set_text(GTK_ENTRY(copyright_entry), copyright);
	gtk_entry_set_text(GTK_ENTRY(isrc_entry), isrc);
	gtk_entry_set_text(GTK_ENTRY(location_entry), location);
#endif
	gtk_entry_set_text(GTK_ENTRY(filename_entry), vte.filename);
	gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

	gtk_entry_set_text(GTK_ENTRY(rg_track_entry), rg_track_gain);
	gtk_entry_set_text(GTK_ENTRY(rg_album_entry), rg_album_gain);
	gtk_entry_set_text(GTK_ENTRY(rg_track_peak_entry), rg_track_peak);
	gtk_editable_set_position(GTK_EDITABLE(rg_track_peak_entry), -1);
	gtk_entry_set_text(GTK_ENTRY(rg_album_peak_entry), rg_album_peak);
	gtk_editable_set_position(GTK_EDITABLE(rg_album_peak_entry), -1);

	if (*rg_track_gain == '\0' && *rg_album_gain == '\0' && *rg_track_peak == '\0' && *rg_album_peak == '\0')
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_show_button), FALSE);
		rg_show_cb(rg_show_button, NULL);
	}
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_show_button), TRUE);

	tmp = g_strdup_printf(_("File Info - %s"), g_basename(vte.filename));
	gtk_window_set_title(GTK_WINDOW(window), tmp);
	g_free(tmp);

	/* Cleanup .. */

	g_free(track_name);
	g_free(performer);
	g_free(album_name);
	g_free(track_number);
	g_free(genre);
	g_free(date);
	g_free(user_comment);
	g_free(location);
	g_free(description);
	g_free(version);
	g_free(isrc);
	g_free(organization);
	g_free(copyright);
	g_free(rg_track_gain);
	g_free(rg_album_gain);
	g_free(rg_track_peak);
	g_free(rg_album_peak);
}
