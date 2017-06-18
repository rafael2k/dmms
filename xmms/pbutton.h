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
#ifndef PBUTTON_H
#define PBUTTON_H

typedef struct
{
	Widget pb_widget;
	gint pb_nx, pb_ny, pb_px, pb_py;
	gboolean pb_pressed, pb_inside, pb_allow_draw;
	void (*pb_push_cb) (void);
	SkinIndex pb_skin_index1, pb_skin_index2;
}
PButton;

PButton *create_pbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, void (*cb) (void), SkinIndex si);
PButton *create_pbutton_ex(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, void (*cb) (void), SkinIndex si1, SkinIndex si2);
void free_pbutton(PButton * b);
void pbutton_set_skin_index(PButton *b, SkinIndex si);
void pbutton_set_skin_index1(PButton *b, SkinIndex si);
void pbutton_set_skin_index2(PButton *b, SkinIndex si);
void pbutton_set_button_data(PButton *b, gint nx, gint ny, gint px, gint py);

#endif
