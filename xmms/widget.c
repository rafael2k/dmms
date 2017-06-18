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

int inside_widget(gint x, gint y, void *p)
{
	Widget *w = (Widget *) p;

	if (x >= w->x && x < w->x + w->width && y >= w->y && y < w->y + w->height && w->visible)
		return 1;
	return 0;
}

void show_widget(void *w)
{
	((Widget *) w)->visible = 1;
	draw_widget(w);
}

void hide_widget(void *w)
{
	((Widget *) w)->visible = 0;
}

void resize_widget(void *w, gint width, gint height)
{
	((Widget *) w)->width = width;
	((Widget *) w)->height = height;
	draw_widget(w);
}

void move_widget(void *w, gint x, gint y)
{
	((Widget *) w)->x = x;
	((Widget *) w)->y = y;
	draw_widget(w);
}

void draw_widget(void *p)
{
	Widget *w = (Widget *) p;
	lock_widget(w);
	w->redraw = TRUE;
	unlock_widget(w);

}

void add_widget(GList ** list, void *w)
{
	(*list) = g_list_append(*list, w);
	pthread_mutex_init(&((Widget *)w)->mutex,NULL);
}

void handle_press_cb(GList * wlist, GtkWidget * widget, GdkEventButton * event)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		if (((Widget *) wl->data)->button_press_cb)
			((Widget *) wl->data)->button_press_cb(widget, event, (Widget *) wl->data);
		wl = wl->next;
	}
}

void handle_release_cb(GList * wlist, GtkWidget * widget, GdkEventButton * event)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		if (((Widget *) wl->data)->button_release_cb)
			((Widget *) wl->data)->button_release_cb(widget, event, (Widget *) wl->data);
		wl = wl->next;
	}
}

void handle_motion_cb(GList * wlist, GtkWidget * widget, GdkEventMotion * event)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		if (((Widget *) wl->data)->motion_cb)
			((Widget *) wl->data)->motion_cb(widget, event, (Widget *) wl->data);
		wl = wl->next;
	}
}

void draw_widget_list(GList * wlist, gboolean * redraw, gboolean force)
{
	/*
	 * The widget list should be locked before calling this
	 */

	GList *wl;
	Widget *w;

	*redraw = FALSE;
	wl = wlist;
	while (wl)
	{
		w = (Widget *) wl->data;
		if ((w->redraw || force) && w->visible && w->draw)
		{
			w->draw(w);
			/*w->redraw=FALSE; */
			*redraw = TRUE;
		}
		wl = wl->next;
	}
}

void widget_list_change_pixmap(GList * wlist, GdkPixmap * pixmap)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		((Widget *) wl->data)->parent = pixmap;
		wl = wl->next;
	}
}

void clear_widget_list_redraw(GList * wlist)
{
	/*
	 * The widget list should be locked before calling this
	 */
	
	GList *wl;
	Widget *w;

	wl = wlist;
	while (wl)
	{
		w = (Widget *) wl->data;
		w->redraw = FALSE;
		wl = wl->next;
	}
}

void lock_widget(void *w)
{
	pthread_mutex_lock(&((Widget *) w)->mutex);
}

void unlock_widget(void *w)
{
	pthread_mutex_unlock(&((Widget *) w)->mutex);
}

void lock_widget_list(GList * wlist)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		lock_widget(wl->data);
		wl = wl->next;
	}
}

void unlock_widget_list(GList * wlist)
{
	GList *wl;

	wl = wlist;
	while (wl)
	{
		unlock_widget(wl->data);
		wl = wl->next;
	}
}
