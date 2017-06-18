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

void eqslider_set_position(EqSlider * es, gfloat pos)
{
	es->es_position = 25 - (int) ((pos * 25.0) / 20.0);
	if (es->es_position < 0)
		es->es_position = 0;
	if (es->es_position > 50)
		es->es_position = 50;
	if (es->es_position >= 24 && es->es_position <= 26)
		es->es_position = 25;
	draw_widget(es);
}

gfloat eqslider_get_position(EqSlider * es)
{
	return 20.0 - (((gfloat) es->es_position * 20.0) / 25.0);
}

void eqslider_draw(Widget * w)
{
	EqSlider *es = (EqSlider *) w;
	GdkPixmap *obj;
	SkinIndex src;
	gint frame;

	src = SKIN_EQMAIN;
	obj = es->es_widget.parent;

	frame = 27 - ((es->es_position * 27) / 50);
	if (frame < 14)
		skin_draw_pixmap(obj, es->es_widget.gc, src, (frame * 15) + 13, 164, es->es_widget.x, es->es_widget.y, es->es_widget.width, es->es_widget.height);
	else
		skin_draw_pixmap(obj, es->es_widget.gc, src, ((frame - 14) * 15) + 13, 229, es->es_widget.x, es->es_widget.y, es->es_widget.width, es->es_widget.height);
	if (es->es_isdragging)
		skin_draw_pixmap(obj, es->es_widget.gc, src, 0, 176, es->es_widget.x + 1, es->es_widget.y + es->es_position, 11, 11);
	else
		skin_draw_pixmap(obj, es->es_widget.gc, src, 0, 164, es->es_widget.x + 1, es->es_widget.y + es->es_position, 11, 11);
}

void eqslider_set_mainwin_text(EqSlider * es)
{
	gint band = 0;
	const gchar *bandname[11] = {N_("PREAMP"), N_("60HZ"), N_("170HZ"),
				     N_("310HZ"), N_("600HZ"), N_("1KHZ"),
				     N_("3KHZ"), N_("6KHZ"), N_("12KHZ"),
				     N_("14KHZ"), N_("16KHZ")};
	gchar *tmp;

	if (es->es_widget.x > 21)
		band = ((es->es_widget.x - 78) / 18) + 1;

	tmp = g_strdup_printf("EQ: %s: %+.1f DB", _(bandname[band]), eqslider_get_position(es));
	mainwin_lock_info_text(tmp);
	g_free(tmp);
}

void eqslider_button_press_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	EqSlider *es = (EqSlider *) data;
	gint y;

	if (inside_widget(event->x, event->y, &es->es_widget))
	{
		if (event->button == 1)
		{
			y = event->y - es->es_widget.y;
			es->es_isdragging = TRUE;
			if (y >= es->es_position && y < es->es_position + 11)
				es->es_drag_y = y - es->es_position;
			else
			{
				es->es_position = y - 5;
				es->es_drag_y = 5;
				if (es->es_position < 0)
					es->es_position = 0;
				if (es->es_position > 50)
					es->es_position = 50;
				if (es->es_position >= 24 && es->es_position <= 26)
					es->es_position = 25;
				equalizerwin_eq_changed();
			}
			eqslider_set_mainwin_text(es);
			draw_widget(es);
		}
		if (event->button == 4)
		{
			es->es_position -= 2;
			if (es->es_position < 0)
				es->es_position = 0;
			equalizerwin_eq_changed();
			draw_widget(es);
		}
		if (event->button == 5)
		{
			es->es_position += 2;
			if (es->es_position > 50)
				es->es_position = 50;
			equalizerwin_eq_changed();
			draw_widget(es);
		}
	}
}

void eqslider_motion_cb(GtkWidget * w, GdkEventMotion * event, gpointer data)
{
	EqSlider *es = (EqSlider *) data;
	gint y;

	y = event->y - es->es_widget.y;
	if (es->es_isdragging)
	{
		es->es_position = y - es->es_drag_y;
		if (es->es_position < 0)
			es->es_position = 0;
		if (es->es_position > 50)
			es->es_position = 50;
		if (es->es_position >= 24 && es->es_position <= 26)
			es->es_position = 25;
		equalizerwin_eq_changed();
		eqslider_set_mainwin_text(es);
		draw_widget(es);
	}
}

void eqslider_button_release_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	EqSlider *es = (EqSlider *) data;

	if (es->es_isdragging)
	{
		es->es_isdragging = FALSE;
		mainwin_release_info_text();
		draw_widget(es);
	}
}

EqSlider *create_eqslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y)
{
	EqSlider *es;

	es = (EqSlider *) g_malloc0(sizeof (EqSlider));
	es->es_widget.parent = parent;
	es->es_widget.gc = gc;
	es->es_widget.x = x;
	es->es_widget.y = y;
	es->es_widget.width = 14;
	es->es_widget.height = 63;
	es->es_widget.visible = TRUE;
	es->es_widget.button_press_cb = GTK_SIGNAL_FUNC(eqslider_button_press_cb);
	es->es_widget.button_release_cb = GTK_SIGNAL_FUNC(eqslider_button_release_cb);
	es->es_widget.motion_cb = GTK_SIGNAL_FUNC(eqslider_motion_cb);
	es->es_widget.draw = eqslider_draw;
	add_widget(wlist, es);
	return es;
}
