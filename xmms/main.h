/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
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
#ifndef MAIN_H
#define MAIN_H

typedef enum
{
	TIMER_ELAPSED, TIMER_REMAINING
}
TimerMode;

typedef struct
{
	gint player_x, player_y, equalizer_x, equalizer_y;
	gint playlist_x, playlist_y, playlist_width, playlist_height;
	guint snap_distance;
	gboolean use_realtime, shuffle, repeat, doublesize, autoscroll;
	gboolean analyzer_peaks, equalizer_autoload, easy_move, equalizer_active;
	gboolean playlist_visible, equalizer_visible, player_visible;
	gboolean player_shaded, playlist_shaded, equalizer_shaded;
	gboolean allow_multiple_instances, always_show_cb;
	gboolean convert_underscore, convert_twenty;
	gboolean show_numbers_in_pl, snap_windows, save_window_position;
	gboolean dim_titlebar, get_info_on_load;
	gboolean get_info_on_demand, eq_doublesize_linked;
	gboolean sort_jump_to_file, use_eplugins, always_on_top, sticky;
	gboolean no_playlist_advance, smooth_title_scroll, use_pl_metadata;
	gboolean use_backslash_as_dir_delimiter;
	gboolean random_skin_on_play, use_fontsets;
	gboolean mainwin_use_xfont, timer_minutes_only;
	gfloat equalizer_preamp, equalizer_bands[10];
	gchar *skin, *outputplugin, *filesel_path, *playlist_path;
	gchar *playlist_font, *mainwin_font;
	gchar *disabled_iplugins, *enabled_gplugins, *enabled_vplugins, *enabled_eplugins;
	gchar *eqpreset_default_file, *eqpreset_extension;
	GList *url_history;
	gint timer_mode, vis_type, analyzer_mode, analyzer_type, scope_mode;
	gint vu_mode, vis_refresh;
	gint analyzer_falloff, peaks_falloff;
	gint playlist_position;
	gint pause_between_songs_time;
	gboolean pause_between_songs, show_wm_decorations;
	gint mouse_change;
	gboolean playlist_transparent;
        gchar *gentitle_format;
}
Config;

extern Config cfg;

extern GtkWidget *mainwin;
extern GdkGC *mainwin_gc;
extern gboolean mainwin_moving;
extern GList *disabled_iplugins;
extern GtkWidget *equalizerwin;
extern GtkWidget *playlistwin;
extern GtkItemFactory *mainwin_vis_menu, *mainwin_general_menu, *mainwin_options_menu;
extern GList *dock_window_list;
extern gboolean pposition_broken;

void save_config(void);

void draw_main_window(gboolean);
void mainwin_quit_cb(void);
void set_timer_mode(TimerMode mode);
void mainwin_lock_info_text(gchar * text);
void mainwin_release_info_text(void);
void mainwin_play_pushed(void);
void mainwin_stop_pushed(void);
void mainwin_eject_pushed(void);

void mainwin_set_back_pixmap(void);

void mainwin_adjust_volume_motion(gint v);
void mainwin_adjust_volume_release(void);
void mainwin_adjust_balance_motion(gint b);
void mainwin_adjust_balance_release(void);
void mainwin_set_volume_slider(gint percent);
void mainwin_set_balance_slider(gint percent);

void mainwin_vis_set_type(VisType mode);

void mainwin_set_info_text(void);
void mainwin_set_song_info(gint rate, gint freq, gint nch);
void mainwin_get_song_info(int *rate, int *freq, int *nch);
void mainwin_clear_song_info(void);

void mainwin_set_always_on_top(gboolean always);
void mainwin_set_volume_diff(gint diff);
void mainwin_set_balance_diff(gint diff);

void mainwin_show(gboolean);
void mainwin_real_show(void);
void mainwin_real_hide(void);
void mainwin_move(gint x, gint y);
void mainwin_shuffle_pushed(gboolean toggled);
void mainwin_repeat_pushed(gboolean toggled);
void mainwin_advance_pushed(gboolean toggled);
void mainwin_disable_seekbar(void);
void mainwin_set_shape_mask(void);
void mainwin_recreate(void);
void mainwin_set_shade_menu_cb(gboolean shaded);
void mainwin_set_shade(gboolean shaded);
void mainwin_shade_toggle(void);
void mainwin_queue_manager(void);

#define PLAYER_HEIGHT ((cfg.player_shaded ? 14 : 116) * (cfg.doublesize + 1))
#define PLAYER_WIDTH (275 * (cfg.doublesize + 1))

#endif
