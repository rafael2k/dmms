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
#ifndef PLAYLIST_H
#define PLAYLIST_H

typedef struct
{
        gchar *filename;
        gchar *title;
        gint length;
        gboolean selected;
}
PlaylistEntry;

void playlist_clear(void);
void playlist_delete(gboolean crop);
/*  void playlist_add(gchar * filename); */
#define playlist_add(filename) playlist_ins(filename, -1)
void playlist_ins(gchar * filename, glong pos);
/*  void playlist_add_dir(gchar * dir); */
#define playlist_add_dir(directory) playlist_ins_dir(directory, -1, TRUE)
guint playlist_ins_dir(char *dir, long pos, gboolean background);
/*  void playlist_add_url_string(gchar * string); */
#define playlist_add_url_string(string) playlist_ins_url_string(string, -1)
guint playlist_ins_url_string(gchar * string, glong pos);
void playlist_play(void);
void playlist_set_info(gchar * title, gint length, gint rate, gint freq, gint nch);
void playlist_check_pos_current(void);
void playlist_next(void);
void playlist_prev(void);
void playlist_queue_selected(void);
void playlist_queue_position(gint pos);
gboolean playlist_is_position_queued(int pos);
gboolean playlist_is_position_selected(int pos);
gint get_playlist_queue_length(void);
void playlist_clear_queue(void);
void playlist_queue_remove(int pos);
void playlist_queue_move(int oldpos, int newpos);
int playlist_get_queue_position(PlaylistEntry *entry);
int playlist_get_playqueue_position_from_playlist_position(int pos);
int playlist_get_playlist_position_from_playqueue_position(int pos);
void playlist_eof_reached(void);
void playlist_set_position(gint pos);
gint get_playlist_length(void);
gint get_playlist_position(void);
gint __get_playlist_position(void);
gchar *playlist_get_info_text(void);
int playlist_get_current_length(void);
gboolean playlist_save(char *filename, gboolean is_pls);
gboolean playlist_load(gchar * filename);
GList *get_playlist(void);
GList *get_queue(void);
void playlist_start_get_info_thread(void);
void playlist_stop_get_info_thread();
void playlist_start_get_info_scan(void);
void playlist_sort_by_title(void);
void playlist_sort_by_filename(void);
void playlist_sort_by_path(void);
void playlist_sort_by_date(void);
void playlist_sort_selected_by_title(void);
void playlist_sort_selected_by_filename(void);
void playlist_sort_selected_by_path(void);
void playlist_sort_selected_by_date(void);
void playlist_reverse(void);
void playlist_random(void);
void playlist_randomize_selected(void);
void playlist_remove_dead_files(void);
void playlist_fileinfo_current(void);
void playlist_fileinfo(gint pos);
void playlist_delete_index(glong index);
void playlist_delete_filenames(GList *filenames);
gchar* playlist_get_filename(gint pos);
gchar* playlist_get_songtitle(gint pos);
gint playlist_get_songtime(gint pos);
GList * playlist_get_selected(void);
GList * playlist_get_selected_list(void);
int playlist_get_num_selected(void);
void playlist_get_total_time(gulong *total_time, gulong *selection_time, gboolean *total_more, gboolean *selection_more);
void playlist_select_all(gboolean set);
void playlist_select_range(int min, int max, gboolean sel);
void playlist_select_invert_all(void);
gboolean playlist_select_invert(int num);
gboolean playlist_read_info_selection(void);
void playlist_read_info(int pos);
void playlist_set_shuffle(gboolean shuffle);


#define PL_LOCK()    pthread_mutex_lock(&playlist_mutex)
#define PL_UNLOCK()  pthread_mutex_unlock(&playlist_mutex)

extern pthread_mutex_t playlist_mutex;

#endif
