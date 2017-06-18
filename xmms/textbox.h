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
#ifndef TEXTBOX_H
#define	TEXTBOX_H

#define	TEXTBOX_SCROLL_TIMEOUT	200
#define TEXTBOX_SCROLL_SMOOTH_TIMEOUT 30

typedef struct
{
	Widget tb_widget;
	GdkPixmap *tb_pixmap;
	gchar *tb_text, *tb_pixmap_text;
	gint tb_pixmap_width;
	gint tb_offset;
	gboolean tb_scroll_allowed, tb_scroll_enabled;
	gboolean tb_is_scrollable, tb_is_dragging;
	gint tb_timeout_tag, tb_drag_x, tb_drag_off;
	gint tb_nominal_y, tb_nominal_height;
	int tb_skin_id;
	SkinIndex tb_skin_index;
	GdkFont *tb_font;
}
TextBox;

void textbox_set_text(TextBox * tb, gchar * text);
void textbox_set_scroll(TextBox * tb, gboolean s);
TextBox *create_textbox(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gboolean allow_scroll, SkinIndex si);
void textbox_set_xfont(TextBox * tb, gboolean use_xfont, gchar *fontname);
void free_textbox(TextBox * tb);

#endif
