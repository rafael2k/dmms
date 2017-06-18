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
#ifndef PLAYLISTWIN_H
#define PLAYLISTWIN_H

#define PLAYLIST_HEIGHT (cfg.playlist_shaded ? 14 : cfg.playlist_height)

void playlistwin_update_list(void);
gboolean playlistwin_item_visible(int index);
gint playlistwin_get_toprow(void);
void playlistwin_set_toprow(gint top);
void playlistwin_set_shade_menu_cb(gboolean shaded);
void playlistwin_raise(void);
void playlistwin_create(void);
void playlistwin_recreate(void);
void draw_playlist_window(gboolean force);
void playlistwin_hide_timer(void);
void playlistwin_set_time(gint time, gint length, TimerMode mode);
void playlistwin_show(gboolean show);
void playlistwin_real_show(void);
void playlistwin_real_hide(void);
void playlistwin_set_back_pixmap(void);
void playlistwin_scroll(int num);
void playlistwin_vis_disable(void);
void playlistwin_vis_enable(void);

extern Vis *playlistwin_vis;

#endif
