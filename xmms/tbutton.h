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
#ifndef TBUTTON_H
#define TBUTTON_H

typedef struct
{
	Widget tb_widget;
	gint tb_nux, tb_nuy, tb_pux, tb_puy, tb_nsx, tb_nsy, tb_psx, tb_psy;
	gint tb_pressed, tb_inside, tb_selected;
	void (*tb_push_cb) (gboolean);
	SkinIndex tb_skin_index;
}
TButton;

TButton *create_tbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h,
			gint nux, gint nuy, gint pux, gint puy, gint nsx, gint nsy, gint psx, gint psy,
			void (*cb) (gboolean), SkinIndex si);
void tbutton_set_toggled(TButton * tb, gboolean toggled);
void free_tbutton(TButton * b);

#endif
