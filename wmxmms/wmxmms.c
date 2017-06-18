/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xmms/i18n.h"
#include "config.h"

#include "libxmms/xmmsctrl.h"
#include "xmms-dock-master.xpm"

#include "getopt.h"

typedef struct
{
	int x, y, width, height, pressed_x, pressed_y, normal_x, normal_y;
	gboolean focus, pressed;
	void (*callback) (void);
}
Button;

void action_play(void);
void action_pause(void);
void action_eject(void);
void action_prev(void);
void action_next(void);
void action_stop(void);

Button buttons[] =
{
	{21, 28, 9, 11, 20, 64, 0, 64, FALSE, FALSE, action_play},   /* PLAY */
	{33, 28, 9, 11, 30, 64, 10, 64, FALSE, FALSE, action_pause}, /* PAUSE */
	{45, 28, 9, 11, 20, 75, 0, 75, FALSE, FALSE, action_eject},  /* EJECT */
	{21, 42, 9, 11, 20, 86, 0, 86, FALSE, FALSE, action_prev},   /* PREV */
	{33, 42, 9, 11, 30, 86, 10, 86, FALSE, FALSE, action_next},  /* NEXT */
	{45, 42, 9, 11, 30, 75, 10, 75, FALSE, FALSE, action_stop},  /* STOP */
};

#define NUM_BUTTONS 6

GList *button_list;

#define VOLSLIDER_X		11
#define VOLSLIDER_Y		12
#define	VOLSLIDER_WIDTH		6
#define	VOLSLIDER_HEIGHT	40

#define SEEKSLIDER_X		21
#define SEEKSLIDER_Y		16
#define SEEKSLIDER_WIDTH	30
#define SEEKSLIDER_HEIGHT	7
#define SEEKSLIDER_KNOB_WIDTH	3
#define SEEKSLIDER_MAX		(SEEKSLIDER_WIDTH - SEEKSLIDER_KNOB_WIDTH)

gboolean volslider_dragging = FALSE;
int volslider_pos = 0;
gboolean seekslider_visible = FALSE, seekslider_dragging = FALSE;
int seekslider_pos = -1, seekslider_drag_offset = 0;
int timeout_tag = 0;

void init(void);

GtkWidget *window, *icon_win;
GdkPixmap *pixmap, *launch_pixmap;
GdkBitmap *mask, *launch_mask;
GdkGC *dock_gc;
GtkTooltips *tooltips = NULL;

int xmms_session = 0;
char *xmms_cmd = "xmms";
gboolean xmms_running = FALSE;

gboolean has_geometry = FALSE, single_click = FALSE, song_title = FALSE;
char *icon_name = NULL;
int win_x, win_y;

GtkTargetEntry drop_types[] =
{
	{"text/plain", 0, 1}
};

/* Does anyone know a better way? */

extern Window gdk_leader_window;

void action_play(void)
{
	xmms_remote_play(xmms_session);
}

void action_pause(void)
{
	xmms_remote_pause(xmms_session);
}

void action_eject(void)
{
	xmms_remote_eject(xmms_session);
}

void action_prev(void)
{
	xmms_remote_playlist_prev(xmms_session);
}

void action_next(void)
{
	xmms_remote_playlist_next(xmms_session);
}

void action_stop(void)
{
	xmms_remote_stop(xmms_session);
}

gboolean inside_region(int mx, int my, int x, int y, int w, int h)
{
	if ((mx >= x && mx < x + w) && (my >= y && my < y + h))
		return TRUE;
	return FALSE;
}

void real_draw_button(GdkWindow *w, Button *button)
{

	if (button->pressed)
		gdk_draw_pixmap(w, dock_gc, pixmap,
				button->pressed_x, button->pressed_y,
				button->x, button->y,
				button->width, button->height);
	else
		gdk_draw_pixmap(w, dock_gc, pixmap,
				button->normal_x, button->normal_y,
				button->x, button->y,
				button->width, button->height);
}

void draw_button(Button *button)
{
	real_draw_button(window->window, button);
	real_draw_button(icon_win->window, button);
}

void draw_buttons(GList *list)
{
	for (; list; list = g_list_next(list))
		draw_button(list->data);
}

void real_draw_volslider(GdkWindow *w)
{
	gdk_draw_pixmap(w, dock_gc, pixmap, 48, 65, VOLSLIDER_X, VOLSLIDER_Y,
			VOLSLIDER_WIDTH, VOLSLIDER_HEIGHT);
	gdk_draw_pixmap(w, dock_gc, pixmap, 42,
			65 + VOLSLIDER_HEIGHT - volslider_pos,
			VOLSLIDER_X,
			VOLSLIDER_Y + VOLSLIDER_HEIGHT - volslider_pos,
			VOLSLIDER_WIDTH, volslider_pos);
}

void draw_volslider(void)
{
	real_draw_volslider(window->window);
	real_draw_volslider(icon_win->window);
}

void real_draw_seekslider(GdkWindow *w)
{
	int slider_x;

	if (seekslider_visible)
	{
		gdk_draw_pixmap(w, dock_gc, pixmap, 2, 114, 19, 12, 35, 14);
		if (seekslider_pos < SEEKSLIDER_MAX / 3)
			slider_x = 44;
		else if (seekslider_pos < (SEEKSLIDER_MAX * 2) / 3)
			slider_x = 47;
		else
			slider_x = 50;
		gdk_draw_pixmap(w, dock_gc, pixmap, slider_x, 112,
				SEEKSLIDER_X + seekslider_pos, SEEKSLIDER_Y, 3,
				SEEKSLIDER_HEIGHT);
	}
	else
		gdk_draw_pixmap(w, dock_gc, pixmap, 2, 100, 19, 12, 35, 14);
}

void draw_seekslider(void)
{
	real_draw_seekslider(window->window);
	real_draw_seekslider(icon_win->window);
}

void redraw_window(void)
{
	if (xmms_running)
	{
		gdk_draw_pixmap(window->window, dock_gc, pixmap,
				0, 0, 0, 0, 64, 64);
		gdk_draw_pixmap(icon_win->window, dock_gc, pixmap,
				0, 0, 0, 0, 64, 64);
		draw_buttons(button_list);
		draw_volslider();
		draw_seekslider();
	}
	else
	{
		gdk_draw_pixmap(window->window, dock_gc, launch_pixmap,
				0, 0, 0, 0, 64, 64);
		gdk_draw_pixmap(icon_win->window, dock_gc, launch_pixmap,
				0, 0, 0, 0, 64, 64);
	}
}

void expose_cb(GtkWidget *w, GdkEventExpose *event, gpointer data)
{
	redraw_window();
}

void button_press_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GList *node;
	Button *btn;
	int pos;
	char *cmd;

	if (xmms_running)
	{
		if (event->button == 2)
		{
			if(xmms_remote_is_pl_win(xmms_session))
				xmms_remote_pl_win_toggle(xmms_session, 0);
			else
				xmms_remote_pl_win_toggle(xmms_session, 1);
		}
		else if (event->button == 3)
		{
			if(xmms_remote_is_main_win(xmms_session))
				xmms_remote_main_win_toggle(xmms_session, 0);
			else
				xmms_remote_main_win_toggle(xmms_session, 1);
		}
		else if (event->button == 4 || event->button == 5)
		{
			if (event->button == 4)
				volslider_pos += 3;
			else
				volslider_pos -= 3;
			if (volslider_pos < 0)
				volslider_pos = 0;
			if (volslider_pos > VOLSLIDER_HEIGHT)
				volslider_pos = VOLSLIDER_HEIGHT;
			xmms_remote_set_main_volume(xmms_session, (volslider_pos * 100) / VOLSLIDER_HEIGHT);
			draw_volslider();
		}
	}

	if (event->button != 1)
		return;
	if (xmms_running)
	{
		for (node = button_list; node; node = g_list_next(node))
		{
			btn = node->data;
			if (inside_region(event->x, event->y, btn->x, btn->y, btn->width, btn->height))
			{
				btn->focus = TRUE;
				btn->pressed = TRUE;
				draw_button(btn);
			}
		}
		if (inside_region(event->x, event->y, VOLSLIDER_X, VOLSLIDER_Y, VOLSLIDER_WIDTH, VOLSLIDER_HEIGHT))
		{
			volslider_pos = VOLSLIDER_HEIGHT - (event->y - VOLSLIDER_Y);
			xmms_remote_set_main_volume(xmms_session, (volslider_pos * 100) / VOLSLIDER_HEIGHT);
			draw_volslider();
			volslider_dragging = TRUE;
		}
		if (inside_region(event->x, event->y, SEEKSLIDER_X, SEEKSLIDER_Y, SEEKSLIDER_WIDTH, SEEKSLIDER_HEIGHT) && seekslider_visible)
		{
			pos = event->x - SEEKSLIDER_X;

			if (pos >= seekslider_pos &&
			    pos < seekslider_pos + SEEKSLIDER_KNOB_WIDTH)
				seekslider_drag_offset = pos - seekslider_pos;
			else
			{
				seekslider_drag_offset = 1;
				seekslider_pos = pos - seekslider_drag_offset;
				if (seekslider_pos < 0)
					seekslider_pos = 0;
				if (seekslider_pos > SEEKSLIDER_MAX)
					seekslider_pos = SEEKSLIDER_MAX;
			}
			draw_seekslider();
			seekslider_dragging = TRUE;
		}
	}
	else if ((!single_click && event->type == GDK_2BUTTON_PRESS) ||
		 (single_click && event->type == GDK_BUTTON_PRESS))
	{
		cmd = g_strconcat(xmms_cmd, " &", NULL);
		system(cmd);
		g_free(cmd);
	}
}

void button_release_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GList *node;
	Button *btn;
	int len;

	if (event->button != 1)
		return;

	for (node = button_list; node; node = g_list_next(node))
	{
		btn = node->data;
		if (btn->pressed)
		{
			btn->focus = FALSE;
			btn->pressed = FALSE;
			draw_button(btn);
			if (btn->callback)
				btn->callback();
		}
	}
	volslider_dragging = FALSE;
	if (seekslider_dragging)
	{
		len = xmms_remote_get_playlist_time(xmms_session, xmms_remote_get_playlist_pos(xmms_session));
		xmms_remote_jump_to_time(xmms_session, (seekslider_pos * len) / SEEKSLIDER_MAX);
		seekslider_dragging = FALSE;
	}

}

void motion_notify_cb(GtkWidget *w, GdkEventMotion *event, gpointer data)
{
	GList *node;
	Button *btn;
	gboolean inside;

	for (node = button_list; node; node = g_list_next(node))
	{
		btn = node->data;
		if (btn->focus)
		{
			inside = inside_region(event->x, event->y,
					       btn->x, btn->y,
					       btn->width, btn->height);
			if ((inside && !btn->pressed) ||
			    (!inside && btn->pressed))
			{
				btn->pressed = inside;
				draw_button(btn);
			}
		}
	}
	if (volslider_dragging)
	{
		volslider_pos = VOLSLIDER_HEIGHT - (event->y - VOLSLIDER_Y);
		if (volslider_pos < 0)
			volslider_pos = 0;
		if (volslider_pos > VOLSLIDER_HEIGHT)
			volslider_pos = VOLSLIDER_HEIGHT;
		xmms_remote_set_main_volume(xmms_session, (volslider_pos * 100) / VOLSLIDER_HEIGHT);
		draw_volslider();
	}
	if (seekslider_dragging)
	{
		seekslider_pos =
			event->x - SEEKSLIDER_X - seekslider_drag_offset;
		if (seekslider_pos < 0)
			seekslider_pos = 0;
		if (seekslider_pos > SEEKSLIDER_MAX)
			seekslider_pos = SEEKSLIDER_MAX;
		draw_seekslider();
	}

}

void destroy_cb(GtkWidget *w, gpointer data)
{
	gtk_exit(0);
}

static void update_tooltip(void)
{
	static int pl_pos = -1;
	static char *filename;
	int new_pos;
	
	if (!tooltips)
		return;
	
	new_pos = xmms_remote_get_playlist_pos(xmms_session);

	if (new_pos == 0)
	{
		/*
		 * Need to do some extra checking, as we get 0 also on
		 * a empty playlist
		 */
		char *current = xmms_remote_get_playlist_file(xmms_session, 0);
		if (!filename && current)
		{
			filename = current;
			new_pos = -1;
		}
		else if (filename && !current)
		{
			g_free(filename);
			filename = NULL;
			new_pos = -1;
		}
		else if (filename && current && strcmp(filename, current))
		{
			g_free(filename);
			filename = current;
			new_pos = -1;
		}
	}

	if (pl_pos != new_pos)
	{
		char *tip = NULL;
		char *title =
			xmms_remote_get_playlist_title(xmms_session, new_pos);
		if (title)
		{
			tip = g_strdup_printf("%d. %s", new_pos+1, title);
			g_free(title);
		}
		gtk_tooltips_set_tip(tooltips, window, tip, NULL);
		g_free(tip);
		pl_pos = new_pos;
	}
}

int timeout_func(gpointer data)
{
	int new_pos, pos;
	gboolean playing, running;

	running = xmms_remote_is_running(xmms_session);

	if (running)
	{
		if (!xmms_running)
		{
			gtk_widget_shape_combine_mask(icon_win, mask, 0, 0);
			gtk_widget_shape_combine_mask(window, mask, 0, 0);
			xmms_running = running;
			redraw_window();
		}
		if (!volslider_dragging)
		{
			int vl, vr;
			xmms_remote_get_volume(xmms_session, &vl, &vr);

			new_pos = ((vl > vr ? vl : vr) * 40) / 100;

			if (new_pos < 0)
				new_pos = 0;
			if (new_pos > VOLSLIDER_HEIGHT)
				new_pos = VOLSLIDER_HEIGHT;

			if (volslider_pos != new_pos)
			{
				volslider_pos = new_pos;
				draw_volslider();
			}
		}

		update_tooltip();

		playing = xmms_remote_is_playing(xmms_session);
		if (!playing && seekslider_visible)
		{
			seekslider_visible = FALSE;
			seekslider_dragging = FALSE;
			seekslider_pos = -1;
			draw_seekslider();
		}
		else if (playing)
		{
			int len, p = xmms_remote_get_playlist_pos(xmms_session);
			len = xmms_remote_get_playlist_time(xmms_session, p);
			if (len == -1)
			{
				seekslider_visible = FALSE;
				seekslider_dragging = FALSE;
				seekslider_pos = -1;
				draw_seekslider();
			}
			else if (!seekslider_dragging)
			{
				seekslider_visible = TRUE;
				pos = xmms_remote_get_output_time(xmms_session);
				if (len != 0)
					new_pos = (pos * SEEKSLIDER_MAX) / len;
				else
					new_pos = 0;
				if (new_pos < 0)
					new_pos = 0;
				if (new_pos > SEEKSLIDER_MAX)
					new_pos = SEEKSLIDER_MAX;
				if (seekslider_pos != new_pos)
				{
					seekslider_pos = new_pos;
					draw_seekslider();
				}
			}
		}
	}
	else
	{
		if (xmms_running)
		{
			if (tooltips != NULL)
				gtk_tooltips_set_tip(tooltips, window, NULL, NULL);
			gtk_widget_shape_combine_mask(icon_win, launch_mask, 0, 0);
			gtk_widget_shape_combine_mask(window, launch_mask, 0, 0);
			xmms_running = FALSE;
			redraw_window();
		}
	}

	return TRUE;
}

void drag_data_received(GtkWidget *widget, GdkDragContext *context,
			int x, int y, GtkSelectionData *selection_data,
			guint info, guint time)
{
	if (selection_data->data)
	{
		char *url = selection_data->data;
		xmms_remote_playlist_clear(xmms_session);
		xmms_remote_playlist_add_url_string(xmms_session, url);
		xmms_remote_play(xmms_session);
	}
}

void init(void)
{
	GdkColor bg_color;
	GdkWindow *leader;
	XWMHints hints;
	int i, w, h;
	GdkGC *mask_gc;

	for (i = 0; i < NUM_BUTTONS; i++)
		button_list = g_list_append(button_list, &buttons[i]);

	if (song_title)
	{
		tooltips = gtk_tooltips_new();
		gtk_tooltips_set_delay(tooltips, 1000);
	}

	window = gtk_window_new(GTK_WINDOW_DIALOG);
	if (has_geometry)
		gtk_widget_set_uposition(window, win_x, win_y);
	gtk_widget_set_usize(window, 64, 64);
	gtk_widget_set_app_paintable(window, TRUE);
	gtk_widget_set_events(window,
			      GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK | GDK_EXPOSURE_MASK);
	gtk_signal_connect(GTK_OBJECT(window), "expose_event",
			   GTK_SIGNAL_FUNC(expose_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "button_press_event",
			   GTK_SIGNAL_FUNC(button_press_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "button_release_event",
			   GTK_SIGNAL_FUNC(button_release_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "motion_notify_event",
			   GTK_SIGNAL_FUNC(motion_notify_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   GTK_SIGNAL_FUNC(destroy_cb), NULL);
	gtk_drag_dest_set(window, GTK_DEST_DEFAULT_ALL, drop_types, 1,
			  GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(window), "drag_data_received",
			   GTK_SIGNAL_FUNC(drag_data_received), NULL);

	gtk_widget_realize(window);
	bg_color.red = 0;
	bg_color.green = 0;
	bg_color.blue = 0;
	gdk_colormap_alloc_color(gdk_colormap_get_system(),
				 &bg_color, FALSE, TRUE);
	gdk_window_set_background(window->window, &bg_color);
	gdk_window_clear(window->window);
	hints.initial_state = WithdrawnState;
	hints.flags = StateHint;
	XSetWMHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(window->window), &hints);

	icon_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_widget_set_app_paintable(icon_win, TRUE);
	gtk_widget_set_uposition(icon_win, 0, 0);
	gtk_widget_set_usize(icon_win, 64, 64);
	gtk_widget_set_events(icon_win,
			      GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK | GDK_EXPOSURE_MASK);
	gtk_signal_connect(GTK_OBJECT(icon_win), "expose_event",
			   GTK_SIGNAL_FUNC(expose_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(icon_win), "button_press_event",
			   GTK_SIGNAL_FUNC(button_press_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(icon_win), "button_release_event",
			   GTK_SIGNAL_FUNC(button_release_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(icon_win), "motion_notify_event",
			   GTK_SIGNAL_FUNC(motion_notify_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(icon_win), "destroy",
			   GTK_SIGNAL_FUNC(destroy_cb), NULL);
	gtk_drag_dest_set(icon_win, GTK_DEST_DEFAULT_ALL, drop_types, 1,
			  GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(icon_win), "drag_data_received",
			   GTK_SIGNAL_FUNC(drag_data_received), NULL);
	gtk_widget_realize(icon_win);
	bg_color.red = 0;
	bg_color.green = 0;
	bg_color.blue = 0;
	gdk_colormap_alloc_color(gdk_colormap_get_system(),
				 &bg_color, FALSE, TRUE);
	gdk_window_set_background(icon_win->window, &bg_color);
	gdk_window_clear(icon_win->window);
	dock_gc = gdk_gc_new(icon_win->window);

	launch_pixmap = gdk_pixmap_new(window->window, 64, 64,
				       -1);

	launch_mask = gdk_pixmap_new(window->window, 64, 64, 1);
	mask_gc = gdk_gc_new(launch_mask);
	bg_color.pixel = 0;
	gdk_gc_set_foreground(mask_gc, &bg_color);
	gdk_draw_rectangle(launch_mask, mask_gc, TRUE, 0, 0, -1, -1);

	if (!icon_name)
		icon_name = g_strdup_printf("%s/wmxmms.xpm", DATA_DIR);
	pixmap = gdk_pixmap_create_from_xpm(window->window, &mask,
					    NULL, icon_name);
	if (!pixmap)
	{
		printf(_("ERROR: Couldn't find %s\n"), icon_name);
		g_free(icon_name);
		gtk_exit(1);
	}
	g_free(icon_name);
	gdk_window_get_size(pixmap, &w, &h);
	if (w > 64)
		w = 64;
	if (h > 64)
		h = 64;
	gdk_draw_pixmap(launch_pixmap, dock_gc, pixmap,
			0, 0, 32 - (w / 2), 32 - (h / 2), w, h);
	gdk_draw_pixmap(launch_mask, mask_gc, mask,
			0, 0, 32 - (w / 2), 32 - (h / 2), w, h);
	gdk_gc_unref(mask_gc);
	gdk_pixmap_unref(pixmap);
	gdk_bitmap_unref(mask);

	gtk_widget_shape_combine_mask(window, launch_mask, 0, 0);
	gtk_widget_shape_combine_mask(icon_win, launch_mask, 0, 0);

	pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					      &mask, NULL, xmms_dock_master_xpm);

	leader = gdk_window_foreign_new(gdk_leader_window);
	gdk_window_set_icon(leader, icon_win->window, NULL, NULL);
	gdk_window_reparent(icon_win->window, leader, 0, 0);

	gdk_window_unref(leader);
	gtk_widget_show(icon_win);
	gtk_widget_show(window);
	timeout_tag = gtk_timeout_add(100, timeout_func, NULL);

}

void display_usage(char *cmd)
{
	printf(_(
		"Usage: %s [options]\n\n"
		"Options:\n"
		"--------\n\n"
		"-h, --help		Display this text and exit.\n"
		"-g, --geometry		Set the geometry (for example +20+20)\n"
		"-s, --session		Set the xmms session to use (Default: 0)\n"
		"-c, --command		Command to launch xmms (Default: xmms)\n"
		"-i, --icon		Set the icon to use when xmms is not running\n"
		"-n, --single		Only a single click is needed to start xmms\n"
		"-t, --title		Display song title when mouse is in window\n"
		"-v, --version		Display version information and exit\n\n"),
	       cmd);
}

static struct option lopt[] =
{
	{"help", 0, NULL, 'h'},
	{"geometry", 1, NULL, 'g'},
	{"session", 1, NULL, 's'},
	{"command", 1, NULL, 'c'},
	{"icon", 1, NULL, 'i'},
	{"single", 0, NULL, 'n'},
	{"title", 0, NULL, 't'},
	{"version", 0, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
	int c;

	gtk_set_locale();
#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	gtk_init(&argc, &argv);

	while ((c = getopt_long(argc, argv, "hg:s:c:i:ntv", lopt, NULL)) != -1)
	{
		switch (c)
		{
			case 'h':
				display_usage(argv[0]);
				gtk_exit(0);
				break;
			case 'g':
				XParseGeometry(optarg, &win_x, &win_y,
					       NULL, NULL);
				has_geometry = TRUE;
				break;
			case 's':
				xmms_session = atoi(optarg);
				break;
			case 'c':
				xmms_cmd = g_strdup(optarg);
				break;
			case 'i':
				icon_name = g_strdup(optarg);
				break;
			case 'n':
				single_click = TRUE;
				break;
			case 't':
				song_title = TRUE;
				break;
			case 'v':
				printf("wmxmms %s\n", VERSION);
				gtk_exit(0);
				break;
		}
	}

	init();
	gtk_main();
	return 0;
}
