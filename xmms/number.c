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
#include "xmms.h"

void number_set_number(Number * nu, int number)
{
	if (number == nu->nu_number)
		return;
	nu->nu_number = number;
	draw_widget(nu);
}

void number_draw(Widget * w)
{
	Number *nu = (Number *) w;
	GdkPixmap *obj;

	obj = nu->nu_widget.parent;

	if (nu->nu_number <= 11)
		skin_draw_pixmap(obj, nu->nu_widget.gc, nu->nu_skin_index,
				 nu->nu_number * 9, 0,
				 nu->nu_widget.x, nu->nu_widget.y, 9, 13);
	else
		skin_draw_pixmap(obj, nu->nu_widget.gc, nu->nu_skin_index,
				 90, 0, nu->nu_widget.x, nu->nu_widget.y, 9, 13);
}

Number *create_number(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, SkinIndex si)
{
	Number *nu;

	nu = (Number *) g_malloc0(sizeof (Number));
	nu->nu_widget.parent = parent;
	nu->nu_widget.gc = gc;
	nu->nu_widget.x = x;
	nu->nu_widget.y = y;
	nu->nu_widget.width = 9;
	nu->nu_widget.height = 13;
	nu->nu_widget.visible = 1;
	nu->nu_widget.draw = number_draw;
	nu->nu_number = 10;
	nu->nu_skin_index = si;

	add_widget(wlist, nu);
	return nu;
}
