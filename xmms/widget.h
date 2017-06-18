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
#ifndef WIDGET_H
#define	WIDGET_H

typedef struct _Widget
{
	GdkPixmap *parent;
	GdkGC *gc;
	gint x, y, width, height, visible;
	void (*button_press_cb) (GtkWidget *, GdkEventButton *, gpointer);
	void (*button_release_cb) (GtkWidget *, GdkEventButton *, gpointer);
	void (*motion_cb) (GtkWidget *, GdkEventMotion *, gpointer);
	void (*draw) (struct _Widget *);
	gboolean redraw;
	pthread_mutex_t mutex;
}
Widget;

int inside_widget(gint x, gint y, void *w);
void show_widget(void *w);
void hide_widget(void *w);
void resize_widget(void *w, gint width, gint height);
void move_widget(void *w, gint x, gint y);
void draw_widget(void *w);
void add_widget(GList ** list, void *v);
void handle_press_cb(GList * wlist, GtkWidget * widget, GdkEventButton * event);
void handle_release_cb(GList * wlist, GtkWidget * widget, GdkEventButton * event);
void handle_motion_cb(GList * wlist, GtkWidget * widget, GdkEventMotion * event);
void draw_widget_list(GList * wlist, gboolean * redraw, gboolean force);
void widget_list_change_pixmap(GList * wlist, GdkPixmap * pixmap);
void clear_widget_list_redraw(GList * wlist);
void lock_widget(void *w);
void unlock_widget(void *w);
void lock_widget_list(GList * wlist);
void unlock_widget_list(GList * wlist);

#endif
