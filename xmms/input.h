
/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
#ifndef INPUT_H
#define INPUT_H

struct InputPluginData
{
	GList *input_list;
	InputPlugin *current_input_plugin;
	gboolean playing;
	gboolean paused;
};

GList *get_input_list(void);
InputPlugin *get_current_input_plugin(void);
void set_current_input_plugin(InputPlugin * ip);
InputVisType input_get_vis_type();
gboolean input_check_file(gchar * filename);
void input_play(char *filename);
void input_stop(void);
void input_pause(void);
int input_get_time(void);
void input_set_eq(int on, float preamp, float *bands);
void input_seek(int time);
void input_get_song_info(gchar * filename, gchar ** title, gint * length);
gboolean get_input_playing(void);
gboolean get_input_paused(void);
guchar *input_get_vis(gint time);
void input_update_vis_plugin(gint time);
gchar *input_get_info_text(void);
void input_about(gint index);
void input_configure(gint index);
void input_add_vis(int time, unsigned char *s, InputVisType type);
void input_add_vis_pcm(int time, AFormat fmt, int nch, int length, void *ptr);
InputVisType input_get_vis_type();
void input_update_vis(gint time);

void input_set_info_text(gchar * text);
GList *input_scan_dir(gchar * dir);
void input_get_volume(int *l, int *r);
void input_set_volume(int l, int r);
void input_file_info_box(gchar * filename);

#endif
