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

void hslider_set_position(HSlider * hs, gint pos)
{
	if (pos == hs->hs_position || hs->hs_pressed)
		return;
	hs->hs_position = pos;
	if (hs->hs_frame_cb)
		hs->hs_frame = hs->hs_frame_cb(hs->hs_position);
	draw_widget(hs);
}

gint hslider_get_position(HSlider * hs)
{
	return hs->hs_position;
}

void hslider_draw(Widget * w)
{
	HSlider *hs = (HSlider *) w;
	GdkPixmap *obj;

	obj = hs->hs_widget.parent;

	skin_draw_pixmap(obj, hs->hs_widget.gc, hs->hs_skin_index,
			 hs->hs_frame_offset, hs->hs_frame * hs->hs_frame_height,
			 hs->hs_widget.x, hs->hs_widget.y, hs->hs_widget.width,
			 hs->hs_widget.height);
	if (hs->hs_pressed)
		skin_draw_pixmap(obj, hs->hs_widget.gc,
				 hs->hs_skin_index, hs->hs_knob_px,
				 hs->hs_knob_py, hs->hs_widget.x + hs->hs_position,
				 hs->hs_widget.y +
				 ((hs->hs_widget.height - hs->hs_knob_height) / 2),
				 hs->hs_knob_width, hs->hs_knob_height);
	else
		skin_draw_pixmap(obj, hs->hs_widget.gc, hs->hs_skin_index,
				 hs->hs_knob_nx, hs->hs_knob_ny,
				 hs->hs_widget.x + hs->hs_position,
				 hs->hs_widget.y +
				 ((hs->hs_widget.height - hs->hs_knob_height) / 2),
				 hs->hs_knob_width, hs->hs_knob_height);
}

void hslider_button_press_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	HSlider *hs = (HSlider *) data;
	gint x;

	if (event->button != 1)
		return;
	if (inside_widget(event->x, event->y, &hs->hs_widget))
	{
		x = event->x - hs->hs_widget.x;
		hs->hs_pressed = TRUE;
		if (x >= hs->hs_position && x < hs->hs_position + hs->hs_knob_width)
			hs->hs_pressed_x = x - hs->hs_position;
		else
		{
			hs->hs_position = x - (hs->hs_knob_width / 2);
			hs->hs_pressed_x = hs->hs_knob_width / 2;
			if (hs->hs_position < hs->hs_min)
				hs->hs_position = hs->hs_min;
			if (hs->hs_position > hs->hs_max)
				hs->hs_position = hs->hs_max;
			if (hs->hs_frame_cb)
				hs->hs_frame = hs->hs_frame_cb(hs->hs_position);

		}
		if (hs->hs_motion_cb)
			hs->hs_motion_cb(hs->hs_position);
		draw_widget(hs);

	}
}

void hslider_motion_cb(GtkWidget * w, GdkEventMotion * event, gpointer data)
{
	HSlider *hs = (HSlider *) data;
	gint x;

	if (hs->hs_pressed)
	{
		if (!hs->hs_widget.visible)
		{
			hs->hs_pressed = FALSE;
			return;
		}
		x = event->x - hs->hs_widget.x;
		hs->hs_position = x - hs->hs_pressed_x;
		if (hs->hs_position < hs->hs_min)
			hs->hs_position = hs->hs_min;
		if (hs->hs_position > hs->hs_max)
			hs->hs_position = hs->hs_max;
		if (hs->hs_frame_cb)
			hs->hs_frame = hs->hs_frame_cb(hs->hs_position);
		if (hs->hs_motion_cb)
			hs->hs_motion_cb(hs->hs_position);
		draw_widget(hs);
	}
}

void hslider_button_release_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	HSlider *hs = (HSlider *) data;

	if (hs->hs_pressed)
	{
		hs->hs_pressed = FALSE;
		if (hs->hs_release_cb)
			hs->hs_release_cb(hs->hs_position);
		draw_widget(hs);
	}
}

HSlider *create_hslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x,
			gint y, gint w, gint h,	gint knx, gint kny, gint kpx,
			gint kpy, gint kw, gint kh, gint fh, gint fo, gint min,
			gint max, gint(*fcb) (gint), void (*mcb) (gint),
			void (*rcb) (gint), SkinIndex si)
{
	HSlider *hs;

	hs = (HSlider *) g_malloc0(sizeof (HSlider));
	hs->hs_widget.parent = parent;
	hs->hs_widget.gc = gc;
	hs->hs_widget.x = x;
	hs->hs_widget.y = y;
	hs->hs_widget.width = w;
	hs->hs_widget.height = h;
	hs->hs_widget.visible = 1;
	hs->hs_widget.button_press_cb = GTK_SIGNAL_FUNC(hslider_button_press_cb);
	hs->hs_widget.button_release_cb = GTK_SIGNAL_FUNC(hslider_button_release_cb);
	hs->hs_widget.motion_cb = GTK_SIGNAL_FUNC(hslider_motion_cb);
	hs->hs_widget.draw = hslider_draw;
	hs->hs_knob_nx = knx;
	hs->hs_knob_ny = kny;
	hs->hs_knob_px = kpx;
	hs->hs_knob_py = kpy;
	hs->hs_knob_width = kw;
	hs->hs_knob_height = kh;
	hs->hs_frame_height = fh;
	hs->hs_frame_offset = fo;
	hs->hs_min = min;
	hs->hs_position = min;
	hs->hs_max = max;
	hs->hs_frame_cb = fcb;
	hs->hs_motion_cb = mcb;
	hs->hs_release_cb = rcb;
	if (hs->hs_frame_cb)
		hs->hs_frame = hs->hs_frame_cb(0);
	hs->hs_skin_index = si;
	add_widget(wlist, hs);

	return hs;
}
