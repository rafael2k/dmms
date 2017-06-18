/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
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
#ifndef PLAYLIST_SLIDER_H
#define PLAYLIST_SLIDER_H

typedef struct
{
	Widget ps_widget;
	PlayList_List *ps_list;
	gboolean ps_is_draging;
	gint ps_drag_y, ps_prev_y, ps_prev_height;
	GdkImage *ps_back_image;
	int ps_skin_id;
}
PlaylistSlider;

PlaylistSlider *create_playlistslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint h, PlayList_List * list);

#endif
