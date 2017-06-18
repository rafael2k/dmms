/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2006  Haavard Kvaalen
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
#include "xmms.h"
#include <time.h>
#include "libxmms/util.h"
#include <sys/stat.h>
#include <unistd.h>

GList *playlist = NULL;
GList *shuffle_list = NULL;
GList *queued_list = NULL;
static gboolean playlist_get_info_scan_active = FALSE;
gboolean playlist_get_info_going = FALSE;
static pthread_t playlist_get_info_thread;
pthread_mutex_t playlist_mutex = PTHREAD_MUTEX_INITIALIZER;
static PlaylistEntry *playlist_position;

extern PlayList_List *playlistwin_list;
extern Vis *mainwin_vis;
extern SVis *mainwin_svis;

static gboolean playlist_get_info_entry(PlaylistEntry *entry);
static int playlist_sort_str_by_path_cmpfunc(gconstpointer a, gconstpointer b);
static guint playlist_load_ins(char * filename, long pos);
static void playlist_load_ins_file(char *filename, char *playlist_name, long pos, char *title, int len);
static int __get_playlist_length(void);
static void playlist_generate_shuffle_list(void);
static void __playlist_generate_shuffle_list(void);
static GList *playlist_shuffle_list(GList *list);

struct devino {
	dev_t dev;
	ino_t ino;
};

static GList *find_playlist_position_list(void)
{
	/* Caller should hold playlist-mutex */
	if (!playlist_position)
	{
		if (cfg.shuffle)
			return shuffle_list;
		return playlist;
	}
	if (cfg.shuffle)
		return g_list_find(shuffle_list, playlist_position);
	return g_list_find(playlist, playlist_position);
}

static void play_queued(void)
{
	/* Caller should hold playlist-mutex */
	GList *temp = queued_list;
	
	playlist_position = queued_list->data;
	queued_list = g_list_remove_link(queued_list, queued_list);
	g_list_free_1(temp);
}

void playlist_clear(void)
{
	GList *node;
	PlaylistEntry *entry;
	
	if (get_input_playing())
		input_stop();

	PL_LOCK();
	if (playlist)
	{
		node = playlist;
		while (node)
		{
			entry = node->data;
			if (entry->filename)
				g_free(entry->filename);
			if (entry->title)
				g_free(entry->title);
			g_free(entry);
			node = node->next;
		}
		g_list_free(playlist);
		playlist = NULL;
		playlist_position = NULL;
	}

	if (queued_list)
	{
		g_list_free(queued_list);
		queued_list = NULL;
	}

	PL_UNLOCK();
	playlist_generate_shuffle_list();
	playlistwin_update_list();
}

void playlist_delete_node(GList *node, gboolean *set_info_text, gboolean *restart_playing)
{
	/* Caller should hold playlist mutex */
	PlaylistEntry *entry = node->data;
	GList *playing_song = NULL;
	GList *queued_song = NULL;

	/*
	 * We call g_list_find manually here because
	 * we don't want an item in the shuffle_list
	 */
	if (playlist_position)
		playing_song = g_list_find(playlist, playlist_position);

	/*
	 * Remove the song from the queue first if it is there, so
	 * that that we avoid trying to play it again
	 */
	if ((queued_song = g_list_find(queued_list, entry)) != NULL)
	{
		queued_list = g_list_remove_link(queued_list, queued_song);
		g_list_free_1(queued_song);
	}

	if (playing_song == node)
	{
		*set_info_text = TRUE;
		if (get_input_playing())
		{
			PL_UNLOCK();
			input_stop();
			PL_LOCK();
			*restart_playing = TRUE;
		}
		playing_song = find_playlist_position_list();
		if (queued_list)
			play_queued();
		else if (g_list_next(playing_song))
			playlist_position = playing_song->next->data;
		else if (g_list_previous(playing_song))
			playlist_position = playing_song->prev->data;
		else
			playlist_position = NULL;
		/* Make sure the entry did not disappear under us */
		if (g_list_index(get_playlist(), entry) == -1)
			return;
	}
	else if (playing_song &&
		 g_list_position(playlist, playing_song) >
		 g_list_position(playlist, node))
		*set_info_text = TRUE;

	if (entry->filename)
		g_free(entry->filename);
	if (entry->title)
		g_free(entry->title);
	shuffle_list = g_list_remove(shuffle_list, entry);
	playlist = g_list_remove_link(playlist, node);
	g_free(entry);
	g_list_free_1(node);
}

void playlist_delete_index(glong index)
{
	gboolean restart_playing = FALSE, set_info_text = FALSE;
	GList *node;
	
	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return;
	}

	node = g_list_nth(playlist, index);
	if(!node)
	{
		PL_UNLOCK();
		return;
	}

	playlist_delete_node(node, &set_info_text, &restart_playing);

	PL_UNLOCK();

	playlistwin_update_list();
	if (restart_playing)
	{
		if (playlist_position)
			playlist_play();
		else
			mainwin_clear_song_info();
	}
	else if (set_info_text)
		mainwin_set_info_text();
}

void playlist_delete_filenames(GList *filenames)
{
	GList *node, *fnode;
	gboolean set_info_text = FALSE, restart_playing = FALSE;

	PL_LOCK();
	for (fnode = filenames; fnode; fnode = g_list_next(fnode))
	{
		node = playlist;
		while (node)
		{
			GList *next = g_list_next(node);
			PlaylistEntry *entry = node->data;
			if (!strcmp(entry->filename, fnode->data))
				playlist_delete_node(node, &set_info_text,
						     &restart_playing);
			node = next;
		}
	}

	PL_UNLOCK();

	playlistwin_update_list();
	if (restart_playing)
	{
		if (playlist_position)
			playlist_play();
		else
			mainwin_clear_song_info();
	}
	else if (set_info_text)
		mainwin_set_info_text();	
}

void playlist_delete(gboolean crop)
{
	gboolean restart_playing = FALSE, set_info_text = FALSE;
	GList *node, *next;
	PlaylistEntry *entry;

	PL_LOCK();

	node = playlist;

	while (node)
	{
		entry = node->data;
		next = g_list_next(node);
		if ((entry->selected && !crop) || (!entry->selected && crop))
			playlist_delete_node(node, &set_info_text, &restart_playing);
		node = next;
	}
	PL_UNLOCK();
	
	playlistwin_update_list();
	if (restart_playing)
	{
		if (playlist_position)
			playlist_play();
		else
			mainwin_clear_song_info();
	}
	else if (set_info_text)
		mainwin_set_info_text();	
}

static void __playlist_ins_with_info(char *filename, long pos, char* title, int len)
{
	PlaylistEntry *entry;

	entry = g_malloc0(sizeof (PlaylistEntry));
	entry->filename = g_strdup(filename);
	if (title)
		entry->title = g_strdup(title);
	entry->length = len;

	PL_LOCK();
	playlist = g_list_insert(playlist, entry, pos);
	PL_UNLOCK();

	playlist_get_info_scan_active = TRUE;
}

static void __playlist_ins(char * filename, long pos)
{
	__playlist_ins_with_info(filename, pos, NULL, -1);
}

static gboolean is_playlist_name(char *pathname)
{
	char *ext;

	ext = strrchr (pathname, '.');
	if (ext && ((!strcasecmp(ext, ".m3u") || !strcasecmp(ext, ".pls"))))
		return TRUE;

	return FALSE;
}


void playlist_ins(char * filename, long pos)
{
	gboolean ispl = FALSE;

	ispl = is_playlist_name (filename);

	if (!ispl && !input_check_file(filename))
	{
		/*
		 * Some files (typically produced by some cgi-scripts)
		 * don't have the correct extension.  Try to recognize
		 * these files by looking at their content.  We only
		 * check for http entries since it does not make sense
		 * to have file entries in a playlist fetched from the
		 * net.
		 */
		struct stat stat_buf;
	    
		/*
		 * Some strange people put fifo's with the .mp3 extension,
		 * so we need to make sure it's a real file.  This is not
		 * supposed to be for security, else we'd open then do
		 * fstat()
		 */
		if (!stat(filename, &stat_buf) && S_ISREG(stat_buf.st_mode))
		{
			char buf[64], *p;
			FILE *file;
			int r;

			if ((file = fopen(filename, "r")) != NULL)
			{
				r = fread(buf, 1, sizeof(buf), file);
				fclose(file);

				for (p = buf; r-- > 0 && (*p == '\r' || *p == '\n'); p++)
					;
				if (r > 5 && !strncasecmp(p, "http:", 5))
					ispl = TRUE;
			}
		}

	}

	if (ispl)
		playlist_load_ins(filename, pos);
	else
		if (input_check_file(filename))
		{
			__playlist_ins(filename, pos);
			playlist_generate_shuffle_list();
			playlistwin_update_list();
		}
}

static guint devino_hash(gconstpointer key)
{
	const struct devino *d = key;
	return d->ino;
}

static int devino_compare(gconstpointer a, gconstpointer b)
{
	const struct devino *da = a, *db = b;
	return da->dev == db->dev && da->ino == db->ino;
}

static gboolean devino_destroy(gpointer key, gpointer value, gpointer data)
{
	g_free(key);
	return TRUE;
}

static GList* playlist_dir_find_files(char *path, gboolean background, GHashTable *htab)
{
	DIR *dir;
	struct dirent *dirent;
	struct stat statbuf;
	char *temp;
	GList *list = NULL, *ilist;
	struct devino *devino;

	if (stat(path, &statbuf) < 0)
		return NULL;

	if (!S_ISDIR(statbuf.st_mode))
		return NULL;

	devino = g_malloc(sizeof (*devino));
	devino->dev = statbuf.st_dev;
	devino->ino = statbuf.st_ino;

	if (g_hash_table_lookup(htab, devino))
	{
		g_free(devino);
		return NULL;
	}

	g_hash_table_insert(htab, devino, GINT_TO_POINTER(1));

	if (path[strlen(path) - 1] != '/')
		temp = g_strconcat(path, "/", NULL);
	else
		temp = g_strdup(path);

	if ((ilist = input_scan_dir(temp)) != NULL)
	{
		GList *node;
		node = ilist;
		while (node)
		{
			char *name = g_strconcat(temp, node->data, NULL);
			list = g_list_prepend(list, name);
			g_free(node->data);
			node = g_list_next(node);
		}
		g_list_free(ilist);
		g_free(temp);
		return list;
	}

	if ((dir = opendir(path)) == NULL)
	{
		g_free(temp);
		return 0;
	}

	while ((dirent = readdir(dir)) != NULL)
	{
		char *filename;

		if (dirent->d_name[0] == '.')
			continue;

		filename = g_strconcat(temp, dirent->d_name, NULL);

		if (stat(filename, &statbuf) < 0)
		{
			g_free(filename);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode))
		{
			GList *sub;
			sub = playlist_dir_find_files(filename,
						      background, htab);
			g_free(filename);
			list = g_list_concat(list, sub);
		}
		else if (input_check_file(filename))
			list = g_list_prepend(list, filename);
		else
			g_free(filename);

		while (background && gtk_events_pending())
			gtk_main_iteration();
	}
	closedir(dir);
	g_free(temp);

	return list;
}

guint playlist_ins_dir(char *path, long pos, gboolean background)
{
	guint entries = 0;
	GList *list, *node;
	GHashTable *htab;

	htab = g_hash_table_new(devino_hash, devino_compare);

	list = playlist_dir_find_files(path, background, htab);
	list = g_list_sort(list, playlist_sort_str_by_path_cmpfunc);

	g_hash_table_foreach_remove(htab, devino_destroy, NULL);
	
	node = list;
	while (node)
	{
		__playlist_ins(node->data, pos);
		entries++;
		if (pos >= 0)
			pos++;
		g_free(node->data);
		node = g_list_next(node);
	}
	g_list_free(list);

	playlist_generate_shuffle_list();
	playlistwin_update_list();
	return entries;
}

guint playlist_ins_url_string(char * string, long pos)
{
	char *temp;
	struct stat statbuf;
	int i = 1, entries = 0;

	while (*string)
	{
		temp = strchr(string, '\n');
		if (temp)
		{
			if (*(temp - 1) == '\r')
				*(temp - 1) = '\0';
			*temp = '\0';
		}
		if (!strncasecmp(string, "file:", 5)) {
			if(!xmms_urldecode_path(string))
				return 0;
		}

		stat(string, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
			i = playlist_ins_dir(string, pos, FALSE);
		else
		{
			if (is_playlist_name (string))
				i = playlist_load_ins(string, pos);
			else
			{
				playlist_ins(string, pos);
				i = 1;
			}
		}
		entries += i;
		if (pos >= 0)
			pos += i;
		if (!temp)
			break;
		string = temp + 1;
	}
	playlist_generate_shuffle_list();
	playlistwin_update_list();

	return entries;
}

void playlist_play(void)
{
	char *filename = NULL;

	if (get_playlist_length() == 0)
		return;

	/* If the user wants a skin randomly selected on play */
	if (cfg.random_skin_on_play)
	{
		/* Get the current skin list */
		scan_skins();
		/* If there are entries */
		if (g_list_length(skinlist)) {
			/* Get a random value to select the skin to use */
			int randval = (gint)(random() / (RAND_MAX + 1.0) * (g_list_length(skinlist) + 1));
			/* If the random value is 0, use the default skin */
			/* Otherwise subtract 1 from the random value and */
			/* select the skin */
			if (randval == 0)
				load_skin(NULL);
			else
			{
				struct SkinNode *skin;
				skin = g_list_nth(skinlist, randval - 1)->data;
				load_skin(skin->path);
			}
		}
		/* Call scan_skins() again to make sure skin selector */
		/* is up to date */
		scan_skins();
	}

	if (get_input_playing())
		input_stop();

	vis_clear_data(mainwin_vis);
	vis_clear_data(playlistwin_vis);
	svis_clear_data(mainwin_svis);
	mainwin_disable_seekbar();

	PL_LOCK();
	if (playlist)
	{
		if (!playlist_position)
		{
			if (cfg.shuffle)
				playlist_position = shuffle_list->data;
			else
				playlist_position = playlist->data;
		}
		filename = playlist_position->filename;
	}
	PL_UNLOCK();

	if (!filename)
		return;
	
	input_play(filename);

	if (input_get_time() != -1)
	{
		equalizerwin_load_auto_preset(filename);
		input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
			     cfg.equalizer_bands);
	}
	playlist_check_pos_current();
}

void playlist_set_info(char *title, int length, int rate, int freq, int nch)
{
	PL_LOCK();
	if (playlist_position)
	{
		g_free(playlist_position->title);
		playlist_position->title = g_strdup(title);
		playlist_position->length = length;
	}
	PL_UNLOCK();
	mainwin_set_song_info(rate, freq, nch);
}

void playlist_check_pos_current(void)
{
	int pos, row, bottom;

	PL_LOCK();
	if (!playlist || !playlist_position || !playlistwin_list)
	{
		PL_UNLOCK();
		return;
	}

	pos = g_list_index(playlist, playlist_position);

	if (playlistwin_item_visible(pos))
	{
		PL_UNLOCK();
		return;
	}

	bottom = MAX(0, __get_playlist_length() -
		     playlistwin_list->pl_num_visible);
	row = CLAMP(pos - playlistwin_list->pl_num_visible / 2, 0, bottom);
	PL_UNLOCK();
	playlistwin_set_toprow(row);
}

void playlist_next(void)
{
	GList *plist_pos_list;
	gboolean restart_playing = FALSE;

	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return;
	}

	plist_pos_list = find_playlist_position_list();

	if (!cfg.repeat && !g_list_next(plist_pos_list) && !queued_list)
	{
		PL_UNLOCK();
		return;
	}
	
	if (get_input_playing())
	{
		/* We need to stop before changing playlist_position */
		PL_UNLOCK();
		input_stop();
		PL_LOCK();
		restart_playing = TRUE;
	}

	plist_pos_list = find_playlist_position_list();
	if (queued_list)
		play_queued();
	else if (g_list_next(plist_pos_list))
		playlist_position = plist_pos_list->next->data;
	else if (cfg.repeat)
	{
		playlist_position = NULL;
		__playlist_generate_shuffle_list();
		if (cfg.shuffle)
			playlist_position = shuffle_list->data;
		else
			playlist_position = playlist->data;
	}
	PL_UNLOCK();
	playlist_check_pos_current();

	if (restart_playing)
		playlist_play();
	else
	{
		mainwin_set_info_text();
		playlistwin_update_list();
	}
}

void playlist_prev(void)
{
	GList *plist_pos_list;
	gboolean restart_playing = FALSE;
	
	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return;
	}

	plist_pos_list = find_playlist_position_list();

	if (!cfg.repeat && !g_list_previous(plist_pos_list))
	{
		PL_UNLOCK();
		return;
	}

	if (get_input_playing())
	{
		/* We need to stop before changing playlist_position */
		PL_UNLOCK();
		input_stop();
		PL_LOCK();
		restart_playing = TRUE;
	}
	
	plist_pos_list = find_playlist_position_list();
	if (g_list_previous(plist_pos_list))
		playlist_position = plist_pos_list->prev->data;
	else if (cfg.repeat)
	{
		GList *node;
		playlist_position = NULL;
		__playlist_generate_shuffle_list();
		if (cfg.shuffle)
			node = g_list_last(shuffle_list);
		else
			node = g_list_last(playlist);
		if (node)
			playlist_position = node->data;
	}
	PL_UNLOCK();
	playlist_check_pos_current();

	if (restart_playing)
		playlist_play();
	else
	{
		mainwin_set_info_text();
		playlistwin_update_list();
	}
}

static void playlist_queue_toggle(void *data)
{
	GList *temp;

	temp = g_list_find(queued_list, data);
	if (temp)
	{
		queued_list = g_list_remove_link(queued_list, temp);
		g_list_free_1(temp);
	}
	else
		queued_list = g_list_append(queued_list, data);
}

gint get_playlist_queue_length(void)
{
	gint qlength;

	PL_LOCK();
	qlength = g_list_length(queued_list);
	PL_UNLOCK();

	return qlength;
}

void playlist_queue_selected(void)
{
	GList *node;

	PL_LOCK();
	for (node = get_playlist(); node; node = node->next)
	{
		PlaylistEntry *entry = node->data;

		if (!entry->selected)
			continue;

		playlist_queue_toggle(entry);
	}
	PL_UNLOCK();

	playlistwin_update_list();
}

void playlist_queue_position(int pos)
{
	void *entry;

	PL_LOCK();
	entry = g_list_nth_data(playlist, pos);
	playlist_queue_toggle(entry);
	PL_UNLOCK();

	playlistwin_update_list();
}

gboolean playlist_is_position_queued(int pos)
{
	PlaylistEntry *entry;
	GList *temp;

	PL_LOCK();
	entry = g_list_nth_data(playlist, pos);
	temp = g_list_find(queued_list, entry);
	PL_UNLOCK();

	return temp != NULL;
}

gboolean playlist_is_position_selected(int pos)
{
	PlaylistEntry *entry;
	gboolean selected;

	PL_LOCK();
	entry = g_list_nth_data(playlist, pos);
	selected = entry && entry->selected;
	PL_UNLOCK();

	return selected;
}

void playlist_clear_queue(void)
{
	PL_LOCK();
	g_list_free(queued_list);
	queued_list = NULL;
	PL_UNLOCK();

	playlistwin_update_list();
}

void playlist_queue_remove(int pos)
{
	void *entry;

	PL_LOCK();
	entry = g_list_nth_data(playlist, pos);
	queued_list = g_list_remove(queued_list, entry);
	PL_UNLOCK();

	playlistwin_update_list();
}

void playlist_queue_move(int oldpos, int newpos)
{
	GList *tmp;

	PL_LOCK();
	tmp = g_list_nth(queued_list, oldpos);
	queued_list = g_list_remove_link (queued_list, tmp);
	queued_list = g_list_insert(queued_list, tmp->data, newpos);
	g_list_free_1 (tmp);
	PL_UNLOCK();
	
	playlistwin_update_list();
}

int playlist_get_queue_position(PlaylistEntry *entry)
{
	return g_list_index(queued_list, entry);
}

int playlist_get_playqueue_position_from_playlist_position(int pos)
{
	return playlist_get_queue_position(g_list_nth_data(playlist, pos));
}

int playlist_get_playlist_position_from_playqueue_position(int pos)
{
	return g_list_index(playlist, g_list_nth_data(queued_list, pos));
}

void playlist_set_position(int pos)
{
	GList *node;
	gboolean restart_playing = FALSE;
	
	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return;
	}

	node = g_list_nth(playlist, pos);
	if (!node)
	{
		PL_UNLOCK();
		return;
	}
	
	if (get_input_playing())
	{
		/* We need to stop before changing playlist_position */
		PL_UNLOCK();
		input_stop();
		PL_LOCK();
		restart_playing = TRUE;
	}

	playlist_position = node->data;
	PL_UNLOCK();
	playlist_check_pos_current();

	if (restart_playing)
		playlist_play();
	else
	{
		mainwin_set_info_text();
		playlistwin_update_list();
	}

	/*
	 * Regenerate the shuffle list when the user set a position
	 * manually
	 */
	playlist_generate_shuffle_list();
}

void playlist_eof_reached(void)
{
	GList *plist_pos_list;

	input_stop();

	PL_LOCK();
	plist_pos_list = find_playlist_position_list();

	if (cfg.no_playlist_advance)
	{
		PL_UNLOCK();
		mainwin_clear_song_info();
		if (cfg.repeat)
			playlist_play();
		return;
	}
	if (queued_list)
		play_queued();
	else if (!g_list_next(plist_pos_list))
	{
		if (cfg.shuffle)
		{
			playlist_position = NULL;
			__playlist_generate_shuffle_list();
		}
		else
			playlist_position = playlist->data;
		if (!cfg.repeat)
		{
			PL_UNLOCK();
			mainwin_clear_song_info();
			mainwin_set_info_text();
			return;
		}
	}
	else
		playlist_position = plist_pos_list->next->data;
	PL_UNLOCK();
	playlist_check_pos_current();
	playlist_play();
	mainwin_set_info_text();
	playlistwin_update_list();
}

int get_playlist_length(void)
{
	int retval;

	PL_LOCK();
	retval = __get_playlist_length();
	PL_UNLOCK();
	
	return retval;
}

static int __get_playlist_length(void)
{
	/* Caller should hold playlist_mutex */
	if (playlist)
		return(g_list_length(playlist));
	return 0;
}

char *playlist_get_info_text(void)
{
	char *text, *title, *tmp, *numbers, *length;

	PL_LOCK();
	if (!playlist_position)
	{
		PL_UNLOCK();
		return NULL;
	}

	if (playlist_position->title)
		title = playlist_position->title;
	else
		title = g_basename(playlist_position->filename);

	/*
	 * If the user don't want numbers in the playlist, don't
	 * display them in other parts of XMMS
	 */

	if (cfg.show_numbers_in_pl)
		numbers = g_strdup_printf("%d. ", __get_playlist_position() + 1);
	else
		numbers = g_strdup("");

	if (playlist_position->length != -1)
		length = g_strdup_printf(" (%d:%-2.2d)",
					 playlist_position->length / 60000,
					 (playlist_position->length / 1000) % 60);
	else
		length = g_strdup("");

	text = g_strdup_printf("%s%s%s", numbers, title, length);
	g_free(numbers);
	g_free(length);

	PL_UNLOCK();

	if (cfg.convert_underscore)
		while ((tmp = strchr(text, '_')) != NULL)
			*tmp = ' ';
	if (cfg.convert_twenty)
		while ((tmp = strstr(text, "%20")) != NULL)
		{
			char *tmp2;
			tmp2 = tmp + 3;
			*(tmp++) = ' ';
			while (*tmp2)
				*(tmp++) = *(tmp2++);
			*tmp = '\0';
		}
	
	return text;
}

int playlist_get_current_length(void)
{
	int retval = 0;

	PL_LOCK();
	if (playlist && playlist_position)
		retval = playlist_position->length;
	PL_UNLOCK();
	return retval;
}

gboolean playlist_save(char *filename, gboolean is_pls)
{
	GList *node;
	FILE *file;

	if ((file = fopen(filename, "w")) == NULL)
		return FALSE;

	if (is_pls)
	{
		is_pls = TRUE;
		fprintf(file, "[playlist]\n");
		fprintf(file, "NumberOfEntries=%d\n", get_playlist_length());
	}
	else if (cfg.use_pl_metadata)
		fprintf(file, "#EXTM3U\n");

	PL_LOCK();
	node = playlist;
	while (node)
	{
		PlaylistEntry *entry = node->data;
		if (is_pls)
			fprintf(file, "File%d=%s\n",
				g_list_position(playlist, node) + 1,
				entry->filename);
		else
		{
			if (entry->title && cfg.use_pl_metadata)
			{
				int seconds;
				
				if (entry->length > 0)
					seconds = (entry->length) / 1000;
				else
					seconds = -1;

				fprintf(file, "#EXTINF:%d,%s\n",
					seconds, entry->title);
			}
			fprintf(file, "%s\n", entry->filename);
		}
		node = g_list_next(node);
	}
	PL_UNLOCK();
	return (fclose(file) == 0);
}

gboolean playlist_load(char * filename)
{
	return playlist_load_ins(filename, -1);
}

static void playlist_load_ins_file(char *filename, char *playlist_name,
				   long pos, char *title, int len)
{
	char *temp, *path;

	if (cfg.use_backslash_as_dir_delimiter)
	{
		while ((temp = strchr(filename, '\\')) != NULL)
		      *temp = '/';
	}

	if (filename[0] != '/' && !strstr(filename, "://"))
	{
		path = g_strdup(playlist_name);
		temp = strrchr(path, '/');
		if (temp)
			*temp = '\0';
		else
		{
			__playlist_ins_with_info(filename, pos, title, len);
			return;
		}
		temp = g_strdup_printf("%s/%s", path, filename);
		__playlist_ins_with_info(temp, pos, title, len);
		g_free(temp);
		g_free(path);
	}
	else
		__playlist_ins_with_info(filename, pos, title, len);
}

static void parse_extm3u_info(char *info, char **title, int *length)
{
	char *str;
	
	*title = NULL;
	*length = -1;

	if (!info)
		return;

	g_return_if_fail(strlen(info) >= 8);
	
	*length = atoi(info + 8);
	if (*length <= 0)
		*length = -1;
	else
		*length *= 1000;
	if ((str = strchr(info, ',')) != NULL)
	{
		*title = g_strdup(str + 1);
		g_strstrip(*title);
		if (strlen(*title) < 1)
		{
			g_free(*title);
			*title = NULL;
		}
	}
}

static guint playlist_load_ins(char * filename, long pos)
{
	FILE *file;
	char *line, *ext;
	guint entries = 0;
	int linelen = 1024;
	gboolean extm3u = FALSE;
	char *ext_info = NULL, *ext_title = NULL;
	int ext_len = -1;

	ext = strrchr(filename, '.');
	if (ext && !strcasecmp(ext, ".pls"))
	{
		int noe, i;
		
		line = read_ini_string(filename, "playlist", "NumberOfEntries");
		if (line == NULL)
			return 0;

		noe = atoi(line);
		g_free(line);

		for (i = 1; i <= noe; i++)
		{
			char key[15];
			g_snprintf(key, 15, "File%d", i);
			line = read_ini_string_no_comment(filename,
							  "playlist", key);
			g_snprintf(key, 15, "Title%d", i);
			if (cfg.use_pl_metadata)
				ext_title = read_ini_string_no_comment(
						filename, "playlist", key);
			else
				ext_title = NULL;
			if (line != NULL)
			{
				playlist_load_ins_file(line, filename, pos,
						       ext_title, -1);
				entries++;
				if (pos >= 0)
					pos++;
				g_free(line);
			}
			g_free(ext_title);
		}
		playlist_generate_shuffle_list();
		playlistwin_update_list();
		return entries;
	}

	/*
	 * Seems like an m3u.  Maybe we should do some sanity checking
	 * here?  If someone accidentally selects something else, we
	 * will try to read it.
	 */

	if ((file = fopen(filename, "r")) == NULL)
		return 0;

	line = g_malloc(linelen);
	while (fgets(line, linelen, file))
	{
		while (strlen(line) == linelen - 1 &&
		       line[strlen(line) - 1] != '\n')
		{
			linelen += 1024;
			line = g_realloc(line, linelen);
			fgets(&line[strlen(line)], 1024, file);
		}
		while (line[strlen(line) - 1] == '\r' ||
		       line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
		
		if (!strncmp(line, "#EXTM3U", 8))
		{
			extm3u = TRUE;
			continue;
		}
		
		if (extm3u && !strncmp(line, "#EXTINF:", 8))
		{
			if (ext_info)
				g_free(ext_info);
			ext_info = g_strdup(line);
			continue;
		}
		
		if (line[0] == '#')
		{
			if (ext_info)
			{
				g_free(ext_info);
				ext_info = NULL;
			}
			continue;
		}
		
		if (extm3u)
		{
			if (cfg.use_pl_metadata)
				parse_extm3u_info(ext_info, &ext_title, &ext_len);
			g_free(ext_info);
			ext_info = NULL;
		}
		
		playlist_load_ins_file(line, filename, pos, ext_title, ext_len);
		
		g_free(ext_title);
		ext_title = NULL;
		ext_len = -1;
		
		entries++;
		if (pos >= 0)
			pos++;
	}
	fclose(file);
	g_free(line);
	playlist_generate_shuffle_list();
	playlistwin_update_list();

	return entries;
}

GList *get_playlist(void)
{
	/* Caller should hold playlist_mutex */
	return playlist;
}

GList *get_queue(void)
{
	/* Caller should hold playlist_mutex */
	return queued_list;
} 

int __get_playlist_position(void)
{
	/* Caller should hold playlist_mutex */
	if (playlist && playlist_position)
		return g_list_index(playlist, playlist_position);
	return 0;
}

int get_playlist_position(void)
{
	int retval;

	PL_LOCK();
	retval = __get_playlist_position();
	PL_UNLOCK();

	return retval;
}

char * playlist_get_filename(int pos)
{
	char *ret;
	PlaylistEntry *entry;
	GList *node;
	
	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return NULL;
	}
	node = g_list_nth(playlist, pos);
	if (!node)
	{
		PL_UNLOCK();
		return NULL;
	}
	entry = node->data;
	
	ret = g_strdup(entry->filename);
	PL_UNLOCK();

	return ret;
}

char * playlist_get_songtitle(int pos)
{
	char *title = NULL, *filename;
	PlaylistEntry *entry;
	GList *node;

	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return NULL;
	}
	node = g_list_nth(playlist, pos);
	if (!node)
	{
		PL_UNLOCK();
		return NULL;
	}
	entry = node->data;

	filename = g_strdup(entry->filename);
	
	if (entry->title == NULL && entry->length == -1)
	{
		if (playlist_get_info_entry(entry))
			title = g_strdup(entry->title);

		PL_UNLOCK();
	}
	else
	{
		title = g_strdup(entry->title);
		PL_UNLOCK();
	}

	if (title == NULL)
		title = g_strdup(g_basename(filename));

	g_free(filename);

	return title;
}

int playlist_get_songtime(int pos)
{
	int retval = -1;
	PlaylistEntry *entry;
	GList *node;
	
	PL_LOCK();
	if (!playlist)
	{
		PL_UNLOCK();
		return -1;
	}
	node = g_list_nth(playlist, pos);
	if (!node)
	{
		PL_UNLOCK();
		return -1;
	}
	entry = node->data;

	if (entry->title == NULL && entry->length == -1)
	{
		if (playlist_get_info_entry(entry))
			retval = entry->length;

		PL_UNLOCK();
	}
	else
	{
		retval = entry->length;
		PL_UNLOCK();
	}

	return retval;
}

static int playlist_sort_by_title_cmpfunc(PlaylistEntry * a, PlaylistEntry * b)
{
	char *a_title, *b_title;

	if (a->title)
		a_title = a->title;
	else
	{
		if (strrchr(a->filename, '/'))
			a_title = strrchr(a->filename, '/') + 1;
		else
			a_title = a->filename;
	}

	if (b->title)
		b_title = b->title;
	else
	{
		if (strrchr(a->filename, '/'))
			b_title = strrchr(b->filename, '/') + 1;
		else
			b_title = b->filename;

	}
	return strcasecmp(a_title, b_title);
}

void playlist_sort_by_title(void)
{
	PL_LOCK();
	playlist = g_list_sort(playlist, (GCompareFunc) playlist_sort_by_title_cmpfunc);
	PL_UNLOCK();
}

static int playlist_sort_by_filename_cmpfunc(PlaylistEntry * a, PlaylistEntry * b)
{
	char *a_filename, *b_filename;

	if (strrchr(a->filename, '/'))
		a_filename = strrchr(a->filename, '/') + 1;
	else
		a_filename = a->filename;

	if (strrchr(b->filename,'/'))
		b_filename = strrchr(b->filename, '/') + 1;
	else
		b_filename = b->filename;

	return strcasecmp(a_filename, b_filename);
}

void playlist_sort_by_filename(void)
{
	PL_LOCK();
	playlist = g_list_sort(playlist, (GCompareFunc) playlist_sort_by_filename_cmpfunc);
	PL_UNLOCK();
}

static int playlist_sort_str_by_path_cmpfunc(gconstpointer pa, gconstpointer pb)
{
	const char *a = pa, *b = pb;
	char *posa, *posb;

	posa = strrchr(a, '/');
	posb = strrchr(b, '/');

	/*
	 * Sort directories before files
	 */
	if (posa && posb && (posa - a != posb - b))
	{
		int len, ret = 0;
		if (posa - a > posb - b && a[posb - b] == '/')
		{
			len = posb - b;
			ret = -1;
		}
		else if (posb - b > posa - a && b[posa - a] == '/')
		{
			len = posa - a;
			ret = 1;
		}
		if (ret && !strncasecmp(a, b, len))
			return ret;
	}
	return strcasecmp(a, b);
}

static int playlist_sort_by_path_cmpfunc(PlaylistEntry * a, PlaylistEntry * b)
{
	return playlist_sort_str_by_path_cmpfunc(a->filename, b->filename);
}

void playlist_sort_by_path(void)
{
	PL_LOCK();
	playlist = g_list_sort(playlist, (GCompareFunc) playlist_sort_by_path_cmpfunc);
	PL_UNLOCK();
}

static int playlist_sort_by_date_cmpfunc(PlaylistEntry * a, PlaylistEntry * b)
{
	struct stat buf;
	time_t modtime;
	
	if (!lstat(a->filename, &buf))
	{
		modtime = buf.st_mtime;
		if (!lstat(b->filename, &buf))
		{
			if (buf.st_mtime == modtime)
				return 0;
			else
				return (buf.st_mtime-modtime) > 0 ? -1 : 1;
		}
		else
			return -1;
	}
	else if(!lstat(b->filename, &buf))
		return 1;
	else
		return playlist_sort_by_filename_cmpfunc(a,b);
}

void playlist_sort_by_date(void)
{
	PL_LOCK();
	playlist = g_list_sort(playlist, (GCompareFunc) playlist_sort_by_date_cmpfunc);
	PL_UNLOCK();
}

static GList* playlist_sort_selected(GList *list, GCompareFunc cmpfunc)
{
	GList *list1, *list2;
	GList *temp_list = NULL;
	GList *index_list = NULL;

	/*
	 * We take all the selected entries out of the playlist,
	 * sort them, and then put them back in again.
	 */

	list1 = g_list_last(list);

	while (list1)
	{
		list2 = g_list_previous(list1);
		if (((PlaylistEntry *) list1->data)->selected)
		{
			gpointer idx;
			idx = GINT_TO_POINTER(g_list_position(list, list1));
			index_list = g_list_prepend(index_list, idx);
			list = g_list_remove_link(list, list1);
			temp_list = g_list_concat(list1, temp_list);
		}
		list1 = list2;
	}
	
	if (cmpfunc)
		temp_list = g_list_sort(temp_list, cmpfunc);
	else
		temp_list = playlist_shuffle_list(temp_list);
		
	list1 = temp_list;
	list2 = index_list;

	while (list2)
	{
		if (!list1)
		{
			g_log(NULL, G_LOG_LEVEL_CRITICAL,
			      "%s: Error during list sorting. "
			      "Possibly dropped some playlist-entries.",
			      PACKAGE);
			break;
		}
		list = g_list_insert(list, list1->data, GPOINTER_TO_INT(list2->data));
		list2 = g_list_next(list2);
		list1 = g_list_next(list1);
	}
	g_list_free(index_list);
	g_list_free(temp_list);

	return list;
}


void playlist_sort_selected_by_title(void)
{
	PL_LOCK();	
	playlist = playlist_sort_selected(playlist, (GCompareFunc) playlist_sort_by_title_cmpfunc);
	PL_UNLOCK();
}

void playlist_sort_selected_by_filename(void)
{
	PL_LOCK();
	playlist = playlist_sort_selected(playlist, (GCompareFunc) playlist_sort_by_filename_cmpfunc);
	PL_UNLOCK();
}

void playlist_sort_selected_by_path(void)
{
	PL_LOCK();
	playlist = playlist_sort_selected(playlist, (GCompareFunc) playlist_sort_by_path_cmpfunc);
	PL_UNLOCK();
}

void playlist_sort_selected_by_date(void)
{
	PL_LOCK();
	playlist = playlist_sort_selected(playlist, (GCompareFunc) playlist_sort_by_date_cmpfunc);
	PL_UNLOCK();
}

void playlist_reverse(void)
{
	PL_LOCK();
	playlist = g_list_reverse(playlist);
	PL_UNLOCK();
}

static GList *playlist_shuffle_list(GList *list)
{
	/* Caller should hold playlist mutex */
	/*
	 * Note that this doesn't make a copy of the original list.
	 * The pointer to the original list is not valid after this
	 * fuction is run.
	 */
	int len = g_list_length(list);
	int i, j;
	GList *node, **ptrs;

	if (!len)
		return NULL;

	ptrs = g_new(GList *, len);

	for (node = list, i = 0; i < len; node = g_list_next(node), i++)
		ptrs[i] = node;

	j = (int)(random() / (RAND_MAX + 1.0) * len); 
	list = ptrs[j];
	ptrs[j]->next = NULL;
	ptrs[j] = ptrs[0];

	for (i = 1; i < len; i++)
	{
		j = (int)(random() / (RAND_MAX + 1.0) * (len - i));
		list->prev = ptrs[i + j];
		ptrs[i + j]->next = list;
		list = ptrs[i + j];
		ptrs[i + j] = ptrs[i];
	}
	list->prev = NULL;

	g_free(ptrs);

	return list;
}

void playlist_random(void)
{
	PL_LOCK();

	playlist = playlist_shuffle_list(playlist);

	PL_UNLOCK();
}

void playlist_randomize_selected(void)
{
	PL_LOCK();
	playlist = playlist_sort_selected(playlist, NULL);
	PL_UNLOCK();
}

GList * playlist_get_selected(void)
{
	GList *node, *list = NULL;
	int i = 0;

	PL_LOCK();
	for (node = get_playlist(); node != 0; node = g_list_next(node), i++)
	{
		PlaylistEntry *entry = node->data;
		if (entry->selected)
			list = g_list_prepend(list, GINT_TO_POINTER(i));
	}
	PL_UNLOCK();
	return g_list_reverse(list);
}

int playlist_get_num_selected(void)
{
	GList *node;
	int num = 0;

	PL_LOCK();
	for (node = get_playlist(); node != 0; node = g_list_next(node))
	{
		PlaylistEntry *entry = node->data;
		if (entry->selected)
			num++;
	}
	PL_UNLOCK();
	return num;
}
	

static void playlist_generate_shuffle_list(void)
{
	PL_LOCK();
	__playlist_generate_shuffle_list();
	PL_UNLOCK();
}

static void __playlist_generate_shuffle_list(void)
{
	/* Caller should hold playlist mutex */

	GList *node;
	int numsongs;

	if (shuffle_list)
	{
		g_list_free(shuffle_list);
		shuffle_list = NULL;
	}

	if (!cfg.shuffle || !playlist)
		return;
	
	shuffle_list = playlist_shuffle_list(g_list_copy(playlist));
	numsongs = g_list_length(shuffle_list);

	if (playlist_position)
	{
		int i = g_list_index(shuffle_list, playlist_position);
		node = g_list_nth(shuffle_list, i);
		shuffle_list = g_list_remove_link(shuffle_list, node);
		shuffle_list = g_list_prepend(shuffle_list, node->data);
	}
}

void playlist_fileinfo(int pos)
{
	char *path = NULL;
	GList *node;
	
	PL_LOCK();
	if ((node = g_list_nth(get_playlist(), pos)) != NULL)
	{
		PlaylistEntry *entry = node->data;
		path = g_strdup(entry->filename);
	}
	PL_UNLOCK();
	if (path)
	{
		input_file_info_box(path);
		g_free(path);
	}
}

void playlist_fileinfo_current(void)
{
	char *path = NULL;
	PL_LOCK();
	
	if (get_playlist() && playlist_position)
		path = g_strdup(playlist_position->filename);
	PL_UNLOCK();
	if (path)
	{
		input_file_info_box(path);
		g_free(path);
	}
}

static gboolean playlist_get_info_entry(PlaylistEntry *entry)
{
	/*
	 * Caller need to hold playlist mutex.
	 * Note that this function temporarily drops the playlist mutex.
	 * If it returns false, the entry might no longer be valid.
	 */
	char *temp_filename, *temp_title;
	int temp_length;

	temp_filename = g_strdup(entry->filename);
	temp_title = NULL;
	temp_length = -1;

	/* We don't want to lock the playlist while reading info */
	PL_UNLOCK();
	input_get_song_info(temp_filename, &temp_title, &temp_length);
	PL_LOCK();
	g_free(temp_filename);

	if (!temp_title && temp_length == -1)
		return FALSE;

	/* Make sure entry is still around */
	if (g_list_index(get_playlist(), entry) == -1)
		return FALSE;

	/* entry is still around */
	entry->title = temp_title;
	entry->length = temp_length;

	return TRUE;
}

static void *playlist_get_info_func(void *arg)
{
	GList *node;
	gboolean update_playlistwin = FALSE, update_mainwin = FALSE;
	PlaylistEntry *entry;

	while (playlist_get_info_going)
	{
		if (cfg.get_info_on_load && playlist_get_info_scan_active)
		{
			PL_LOCK();
			for (node = get_playlist(); node; node = g_list_next(node))
			{
				entry = node->data;
				if (entry->title || entry->length != -1)
					continue;
				if (!playlist_get_info_entry(entry))
				{
					if (g_list_index(get_playlist(), entry) == -1)
						/* Entry disapeared while we
						   looked it up.  Restart. */
						node = get_playlist();
				}
				else if (entry->title || entry->length != -1)
				{
					update_playlistwin = TRUE;
					if (entry == playlist_position)
						update_mainwin = TRUE;
					break;
				}
			}
			PL_UNLOCK();

			if (!node)
				playlist_get_info_scan_active = FALSE;
		}
		else if (!cfg.get_info_on_load && cfg.get_info_on_demand &&
			 cfg.playlist_visible && !cfg.playlist_shaded)
		{
			gboolean found = FALSE;

			PL_LOCK();
			if (!get_playlist())
			{
				PL_UNLOCK();
				xmms_usleep(1000000);
				continue;
			}
			
			for (node = g_list_nth(get_playlist(), playlistwin_get_toprow());
			     node && playlistwin_item_visible(g_list_position(get_playlist(), node));
			     node = g_list_next(node))
			{
				entry = node->data;
				if (entry->title || entry->length != -1)
					continue;

				if (!playlist_get_info_entry(entry))
				{
					if (g_list_index(get_playlist(), entry) == -1)
						/* Entry disapeared while we
						   looked it up.  Restart. */
						node = g_list_nth(get_playlist(), playlistwin_get_toprow());
				}
				else if (entry->title || entry->length != -1)
				{
					update_playlistwin = TRUE;
					if (entry == playlist_position)
						update_mainwin = TRUE;
					found = TRUE;
					break;
				}
			}
			PL_UNLOCK();
			if (!found)
			{
				xmms_usleep(500000);
				continue;
			}
		}
		else
			xmms_usleep(500000);

		if (update_playlistwin)
		{
			playlistwin_update_list();
			update_playlistwin = FALSE;
		}
		if (update_mainwin)
		{
			mainwin_set_info_text();
			update_mainwin = FALSE;
		}
	}
	pthread_exit(NULL);
}

void playlist_start_get_info_thread(void)
{
	playlist_get_info_going = TRUE;
	pthread_create(&playlist_get_info_thread, NULL,
		       playlist_get_info_func, NULL);
}

void playlist_stop_get_info_thread(void)
{
	playlist_get_info_going = FALSE;
	pthread_join(playlist_get_info_thread, NULL);
}

void playlist_start_get_info_scan(void)
{
	playlist_get_info_scan_active = TRUE;
}

void playlist_remove_dead_files(void)
{
	/* FIXME? Does virtual directories work well? */
	GList *node, *next_node, *temp = NULL;
	PlaylistEntry *entry;
	gboolean list_changed = FALSE;
	
	PL_LOCK();
	node = playlist;
	while (node)
	{
		/* A dead file is a file that is not readable. */
		next_node = g_list_next(node);
		entry = node->data;

		if (entry && entry->filename &&
		    !strstr(entry->filename, "://") && /* Don't kill URL's */
		    ((temp = input_scan_dir(entry->filename)) == NULL) &&
		    access(entry->filename, R_OK))
		{
			gboolean set_info_text = FALSE, restart_playing = FALSE;
			if (entry == playlist_position && get_input_playing())
			{
				/* Don't remove the currently playing song */
				node = next_node;
				continue;
			}

			playlist_delete_node(node, &set_info_text, &restart_playing);
			if (set_info_text)
				list_changed = TRUE;
		}
		else if (temp)
		{
			g_list_free(temp);
			temp = NULL;
		}
		node = next_node;
	}
	PL_UNLOCK();

	if (list_changed)
	{
		playlist_generate_shuffle_list();
		playlistwin_update_list();
	}
}

void playlist_get_total_time(gulong *total_time, gulong *selection_time, gboolean *total_more, gboolean *selection_more)
{
	GList *list;
	PlaylistEntry *entry;
	
	*total_time = 0;
	*selection_time = 0;
	*total_more = FALSE;
	*selection_more = FALSE;
	
	PL_LOCK();
	list = get_playlist();
	while (list)
	{
		entry = list->data;
		if (entry->length != -1)
			*total_time += entry->length / 1000;
		else
			*total_more = TRUE;
		if (entry->selected)
		{
			if (entry->length != -1)
				*selection_time += entry->length / 1000;
			else
				*selection_more = TRUE;
		}
		list = g_list_next(list);
	}
	PL_UNLOCK();
}

void playlist_select_all(gboolean set)
{
	GList *list;
	
	PL_LOCK();
	list = get_playlist();
	while (list)
	{
		PlaylistEntry *entry = list->data;
		entry->selected = set;
		list = list->next;
	}
	PL_UNLOCK();
}

void playlist_select_invert_all(void)
{
	GList *list;
	
	PL_LOCK();
	list = get_playlist();
	while (list)
	{
		PlaylistEntry *entry = list->data;
		entry->selected = !entry->selected;
		list = list->next;
	}
	PL_UNLOCK();
}

gboolean playlist_select_invert(int num)
{
	GList *list;
	gboolean retv = FALSE;

	PL_LOCK();
	if ((list = g_list_nth(get_playlist(), num)) != NULL)
	{
		PlaylistEntry *entry = list->data;
		entry->selected = !entry->selected;
		retv = TRUE;
	}
	PL_UNLOCK();

	return retv;
}
	

void playlist_select_range(int min, int max, gboolean sel)
{
	GList *list;
	int i;

	if (min > max)
	{
		int tmp = min;
		min = max;
		max = tmp;
	}

	PL_LOCK();
	list = g_list_nth(get_playlist(), min);
	for (i = min; i <= max && list; i++)
	{
		PlaylistEntry *entry = list->data;
		entry->selected = sel;
		list = list->next;
	}
	PL_UNLOCK();
}

gboolean playlist_read_info_selection(void)
{
	GList *node;
	gboolean retval = FALSE;

	PL_LOCK();
	for (node = get_playlist(); node; node = node->next)
	{
		PlaylistEntry *entry = node->data;
		if (entry->selected)
		{
			retval = TRUE;
			if (entry->title)
				g_free(entry->title);
			entry->title = NULL;
			entry->length = -1;
			if (!playlist_get_info_entry(entry))
			{
				if (g_list_index(get_playlist(), entry) == -1)
					/* Entry disapeared while we
					   looked it up.  Restart. */
					node = get_playlist();
			}
		}
	}
	PL_UNLOCK();
	playlistwin_update_list();
	return retval;
}

void playlist_read_info(int pos)
{
	GList *node;

	PL_LOCK();
	if ((node = g_list_nth(get_playlist(), pos)) != NULL)
	{
		PlaylistEntry *entry = node->data;

		if (entry->title)
			g_free(entry->title);
		entry->title = NULL;
		entry->length = -1;
		playlist_get_info_entry(entry);
	}
	PL_UNLOCK();
	playlistwin_update_list();
}

void playlist_set_shuffle(gboolean shuffle)
{
	PL_LOCK();
	cfg.shuffle = shuffle;
	__playlist_generate_shuffle_list();
	PL_UNLOCK();
}
