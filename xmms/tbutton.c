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

void tbutton_draw(Widget * w)
{
	TButton *button = (TButton *) w;
	GdkPixmap *obj;

	obj = button->tb_widget.parent;

	if (button->tb_pressed && button->tb_inside)
	{
		if (button->tb_selected)
		{
			skin_draw_pixmap(obj, button->tb_widget.gc,
					 button->tb_skin_index,
					 button->tb_psx, button->tb_psy,
					 button->tb_widget.x, button->tb_widget.y,
					 button->tb_widget.width,
					 button->tb_widget.height);
		}
		else
		{
			skin_draw_pixmap(obj, button->tb_widget.gc,
					 button->tb_skin_index,
					 button->tb_pux, button->tb_puy,
					 button->tb_widget.x, button->tb_widget.y,
					 button->tb_widget.width,
					 button->tb_widget.height);
		}
	}
	else
	{
		if (button->tb_selected)
		{
			skin_draw_pixmap(obj, button->tb_widget.gc,
					 button->tb_skin_index,
					 button->tb_nsx, button->tb_nsy,
					 button->tb_widget.x, button->tb_widget.y,
					 button->tb_widget.width,
					 button->tb_widget.height);
		}
		else
		{
			skin_draw_pixmap(obj, button->tb_widget.gc,
					 button->tb_skin_index,
					 button->tb_nux, button->tb_nuy,
					 button->tb_widget.x, button->tb_widget.y,
					 button->tb_widget.width,
					 button->tb_widget.height);

		}
	}
}

void tbutton_button_press_cb(GtkWidget * widget, GdkEventButton * event, TButton * button)
{
	if (event->button != 1)
		return;
	if (inside_widget(event->x, event->y, &button->tb_widget))
	{
		button->tb_pressed = 1;
		button->tb_inside = 1;
		draw_widget(button);
	}
}

void tbutton_button_release_cb(GtkWidget * widget, GdkEventButton * event, TButton * button)
{
	if (event->button != 1)
		return;
	if (button->tb_inside && button->tb_pressed)
	{
		button->tb_inside = 0;
		button->tb_selected = !button->tb_selected;
		draw_widget(button);
		if (button->tb_push_cb)
			button->tb_push_cb(button->tb_selected);
	}
	if (button->tb_pressed)
		button->tb_pressed = 0;
}

void tbutton_motion_cb(GtkWidget * widget, GdkEventMotion * event, TButton * button)
{
	int inside;

	if (!button->tb_pressed)
		return;
	inside = inside_widget(event->x, event->y, &button->tb_widget);
	if (inside != button->tb_inside)
	{
		button->tb_inside = inside;
		draw_widget(button);
	}
}

TButton *create_tbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h,
			gint nux, gint nuy, gint pux, gint puy, gint nsx, gint nsy, gint psx, gint psy,
			void (*cb) (gboolean), SkinIndex si)
{
	TButton *b;

	b = (TButton *) g_malloc0(sizeof (TButton));
	b->tb_widget.parent = parent;
	b->tb_widget.gc = gc;
	b->tb_widget.x = x;
	b->tb_widget.y = y;
	b->tb_widget.width = w;
	b->tb_widget.height = h;
	b->tb_widget.visible = 1;
	b->tb_widget.button_press_cb = GTK_SIGNAL_FUNC(tbutton_button_press_cb);
	b->tb_widget.button_release_cb = GTK_SIGNAL_FUNC(tbutton_button_release_cb);
	b->tb_widget.motion_cb = GTK_SIGNAL_FUNC(tbutton_motion_cb);
	b->tb_widget.draw = tbutton_draw;
	b->tb_nux = nux;
	b->tb_nuy = nuy;
	b->tb_pux = pux;
	b->tb_puy = puy;
	b->tb_nsx = nsx;
	b->tb_nsy = nsy;
	b->tb_psx = psx;
	b->tb_psy = psy;
	b->tb_push_cb = cb;
	b->tb_skin_index = si;
	add_widget(wlist, b);
	return b;
}

void tbutton_set_toggled(TButton * tb, gboolean toggled)
{
	tb->tb_selected = toggled;
	draw_widget(tb);
}

void free_tbutton(TButton * b)
{
	g_free(b);
}
