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

void sbutton_button_press_cb(GtkWidget * widget, GdkEventButton * event, SButton * button)
{
	if (event->button != 1)
		return;
	if (inside_widget(event->x, event->y, &button->sb_widget))
	{
		button->sb_pressed = 1;
		button->sb_inside = 1;
	}
}

void sbutton_button_release_cb(GtkWidget * widget, GdkEventButton * event, SButton * button)
{
	if (event->button != 1)
		return;
	if (button->sb_inside && button->sb_pressed)
	{
		button->sb_inside = 0;
		if (button->sb_push_cb)
			button->sb_push_cb();
	}
	if (button->sb_pressed)
		button->sb_pressed = 0;
}

void sbutton_motion_cb(GtkWidget * widget, GdkEventMotion * event, SButton * button)
{
	int inside;

	if (!button->sb_pressed)
		return;
	inside = inside_widget(event->x, event->y, &button->sb_widget);
	if (inside != button->sb_inside)
	{
		button->sb_inside = inside;
	}
}

SButton *create_sbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, void (*cb) (void))
{
	SButton *b;

	b = (SButton *) g_malloc0(sizeof (SButton));
	b->sb_widget.parent = parent;
	b->sb_widget.gc = gc;
	b->sb_widget.x = x;
	b->sb_widget.y = y;
	b->sb_widget.width = w;
	b->sb_widget.height = h;
	b->sb_widget.visible = 1;
	b->sb_widget.button_press_cb = GTK_SIGNAL_FUNC(sbutton_button_press_cb);
	b->sb_widget.button_release_cb = GTK_SIGNAL_FUNC(sbutton_button_release_cb);
	b->sb_widget.motion_cb = GTK_SIGNAL_FUNC(sbutton_motion_cb);
	b->sb_push_cb = cb;
	add_widget(wlist, b);
	return b;
}

void free_sbutton(SButton * b)
{
	g_free(b);
}
