/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
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
#ifndef PLAYLIST_LIST_H
#define PLAYLIST_LIST_H

typedef struct
{
	Widget pl_widget;
	gint pl_fheight, pl_prev_selected, pl_prev_min, pl_prev_max;
	gint pl_first, pl_num_visible, pl_drag_pos;
	gboolean pl_dragging, pl_auto_drag_down, pl_auto_drag_up;
	gint pl_auto_drag_up_tag, pl_auto_drag_down_tag;
}
PlayList_List;

PlayList_List *create_playlist_list(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h);
void playlist_list_move_up(PlayList_List *pl);
void playlist_list_move_down(PlayList_List *pl);
int playlist_list_get_playlist_position(PlayList_List *pl, int x, int y);
void playlist_list_set_font(char *font);

#endif
