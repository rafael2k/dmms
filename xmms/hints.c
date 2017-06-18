/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen
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
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include "xmms.h"

/* flags for the window layer */
typedef enum
{
	WIN_LAYER_DESKTOP = 0,
	WIN_LAYER_BELOW = 2,
	WIN_LAYER_NORMAL = 4,
	WIN_LAYER_ONTOP = 6,
	WIN_LAYER_DOCK = 8,
	WIN_LAYER_ABOVE_DOCK = 10
}
WinLayer;

#define WIN_STATE_STICKY		(1 << 0)

#define WIN_HINTS_SKIP_WINLIST		(1 << 1) /* not in win list */
#define WIN_HINTS_SKIP_TASKBAR		(1 << 2) /* not on taskbar */

#define _NET_WM_STATE_REMOVE   0
#define _NET_WM_STATE_ADD      1
#define _NET_WM_STATE_TOGGLE   2

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8


static void (*set_always_func)(GtkWidget *, gboolean);
static void (*set_sticky_func)(GtkWidget *, gboolean);
static void (*set_skip_taskbar_func)(GtkWidget *);
static void (*move_resize_func)(GtkWidget *, int, int, gboolean);

void hint_set_skip_winlist(GtkWidget *window)
{
	if (set_skip_taskbar_func)
		set_skip_taskbar_func(window);
}

void hint_set_always(gboolean always)
{
	if (set_always_func)
	{
		set_always_func(mainwin, always);
		set_always_func(equalizerwin, always);
		set_always_func(playlistwin, always);
	}
}

gboolean hint_always_on_top_available(void)
{
	return !!set_always_func;
}

void hint_set_sticky(gboolean sticky)
{
	if (set_sticky_func)
	{
		set_sticky_func(mainwin, sticky);
		set_sticky_func(equalizerwin, sticky);
		set_sticky_func(playlistwin, sticky);
	}
}

gboolean hint_move_resize_available(void)
{
	return 0;
}

void hint_move_resize(GtkWidget *window, int x, int y, gboolean move)
{
	move_resize_func(window, x, y, move);
}

static void gnome_wm_set_skip_taskbar(GtkWidget *widget)
{
	long data[1];
	Atom xa_win_hints = gdk_atom_intern("_WIN_HINTS", FALSE);

	data[0] = WIN_HINTS_SKIP_TASKBAR;
	XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(widget->window),
			xa_win_hints, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *) data, 1);
}

static void gnome_wm_set_window_always(GtkWidget * window, gboolean always)
{
	XEvent xev;
	long layer = WIN_LAYER_ONTOP;
	Atom xa_win_layer = gdk_atom_intern("_WIN_LAYER", FALSE);

	if (always == FALSE)
		layer = WIN_LAYER_NORMAL;

	if (GTK_WIDGET_MAPPED(window))
	{
		xev.type = ClientMessage;
		xev.xclient.type = ClientMessage;
		xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
		xev.xclient.message_type = xa_win_layer;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = layer;

		XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
			   SubstructureNotifyMask, &xev);
	}
	else
	{
		long data[1];

		data[0] = layer;
		XChangeProperty(GDK_DISPLAY(),
				GDK_WINDOW_XWINDOW(window->window),
				xa_win_layer, XA_CARDINAL, 32, PropModeReplace,
				(unsigned char *) data,	1);
	}
}

static void gnome_wm_set_window_sticky(GtkWidget * window, gboolean sticky)
{
	XEvent xev;
	long state = 0;
	Atom xa_win_state = gdk_atom_intern("_WIN_STATE", FALSE);

	if (sticky)
		state = WIN_STATE_STICKY;

	if (GTK_WIDGET_MAPPED(window))
	{
		xev.type = ClientMessage;
		xev.xclient.type = ClientMessage;
		xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
		xev.xclient.message_type = xa_win_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = WIN_STATE_STICKY;
		xev.xclient.data.l[1] = state;

		XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
			   SubstructureNotifyMask, &xev);
	}
	else
	{
		long data[2];

		data[0] = state;
		XChangeProperty(GDK_DISPLAY(),
				GDK_WINDOW_XWINDOW(window->window),
				xa_win_state, XA_CARDINAL, 32, PropModeReplace,
				(unsigned char *) data, 1);
	}
}

static gboolean gnome_wm_found(void)
{
	Atom r_type, support_check;
	int r_format, p;
	unsigned long count, bytes_remain;
	unsigned char *prop = NULL, *prop2 = NULL;
	gboolean ret = FALSE;

	gdk_error_trap_push();

	support_check = gdk_atom_intern("_WIN_SUPPORTING_WM_CHECK", FALSE);
	
	p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), support_check,
			       0, 1, False, XA_CARDINAL, &r_type, &r_format,
			       &count, &bytes_remain, &prop);

	if (p == Success && prop && r_type == XA_CARDINAL &&
	    r_format == 32 && count == 1)
	{
		Window n = *(long *) prop;

		p = XGetWindowProperty(GDK_DISPLAY(), n, support_check, 0, 1,
				       False, XA_CARDINAL, &r_type, &r_format,
				       &count, &bytes_remain, &prop2);

		if (p == Success && prop2 && r_type == XA_CARDINAL &&
		    r_format == 32 && count == 1)
			ret = TRUE;
	}

	if (prop)
		XFree(prop);
	if (prop2)
		XFree(prop2);
	if (gdk_error_trap_pop())
		return FALSE;
	return ret;
}

static gboolean net_wm_found(void)
{
	Atom r_type, support_check;
	int r_format,  p;
	unsigned long count, bytes_remain;
	unsigned char *prop = NULL, *prop2 = NULL;
	gboolean ret = FALSE;

	gdk_error_trap_push();
	support_check = gdk_atom_intern("_NET_SUPPORTING_WM_CHECK", FALSE);

	p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), support_check,
			       0, 1, False, XA_WINDOW, &r_type, &r_format,
			       &count, &bytes_remain, &prop);

	if (p == Success && prop && r_type == XA_WINDOW &&
	    r_format == 32 && count == 1)
	{
		Window n = *(Window *) prop;

		p = XGetWindowProperty(GDK_DISPLAY(), n, support_check, 0, 1,
				       False, XA_WINDOW, &r_type, &r_format,
				       &count, &bytes_remain, &prop2);
	
		if (p == Success && prop2 && *prop2 == *prop &&
		    r_type == XA_WINDOW && r_format == 32 && count == 1)
			ret = TRUE;
	}

	if (prop)
		XFree(prop);
	if (prop2)
		XFree(prop2);
	if (gdk_error_trap_pop())
		return FALSE;
	return ret;
}

static void net_wm_set_property(GtkWidget * window, char *atom, gboolean state)
{
	XEvent xev;
	int set = _NET_WM_STATE_ADD;
	Atom type, property;

	if (state == FALSE)
		set = _NET_WM_STATE_REMOVE;

	type = gdk_atom_intern("_NET_WM_STATE", FALSE);
	property = gdk_atom_intern(atom, FALSE);

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
	xev.xclient.message_type = type;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = set;
	xev.xclient.data.l[1] = property;
	xev.xclient.data.l[2] = 0;
	
	XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		   SubstructureNotifyMask, &xev);
}

static void net_wm_set_desktop(GtkWidget * window, gboolean all)
{
	XEvent xev;
	guint32 current_desktop = 0;

	if (!all)
	{
		int r_format,  p;
		unsigned long count, bytes_remain;
		unsigned char* prop;
		Atom r_type;
		Atom current = gdk_atom_intern("_NET_WM_DESKTOP", FALSE);

		p = XGetWindowProperty(GDK_DISPLAY(),
				       GDK_WINDOW_XWINDOW(window->window), current,
				       0, 1, False, XA_CARDINAL, &r_type, &r_format,
				       &count, &bytes_remain, &prop);

		if (p == Success && prop && r_type == XA_CARDINAL &&
		    r_format == 32 && count == 1)
		{
			current_desktop = *(long*)prop;
			XFree(prop);
		}
		if (current_desktop < 0xfffffffe)
			/*
			 * We don't want to move the window if
			 * it isn't sticky
			 */
			return;

		current = gdk_atom_intern("_NET_CURRENT_DESKTOP", FALSE);

		p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), current,
				       0, 1, False, XA_CARDINAL, &r_type, &r_format,
				       &count, &bytes_remain, &prop);

		if (p == Success && prop && r_type == XA_CARDINAL &&
		    r_format == 32 && count == 1)
		{
			current_desktop = *(long*)prop;
			XFree(prop);
		}
	}
	else
		current_desktop = 0xffffffff;

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
	xev.xclient.message_type = gdk_atom_intern("_NET_WM_DESKTOP", FALSE);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = current_desktop;
	
	XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		   SubstructureNotifyMask, &xev);
}

static void net_wm_set_window_always(GtkWidget * window, gboolean always)
{
	net_wm_set_property(window, "_NET_WM_STATE_STAYS_ON_TOP", always);
}

static void net_wm_set_window_above(GtkWidget * window, gboolean always)
{
	net_wm_set_property(window, "_NET_WM_STATE_ABOVE", always);
}

static void net_wm_set_skip_taskbar(GtkWidget * window)
{
	net_wm_set_property(window, "_NET_WM_STATE_SKIP_TASKBAR", TRUE);
}

static void net_wm_move_resize(GtkWidget *window, int x, int y, gboolean move)
{
	XEvent xev;
	int dir;
	Atom type;

	if (move)
		dir = _NET_WM_MOVERESIZE_MOVE;
	else
		dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;

	gdk_pointer_ungrab(GDK_CURRENT_TIME);

	type = gdk_atom_intern("_NET_WM_MOVERESIZE", FALSE);

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
	xev.xclient.message_type = type;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = x;
	xev.xclient.data.l[1] = y;
	xev.xclient.data.l[2] = dir;
	xev.xclient.data.l[3] = 1;    /* button */
	
	
	XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		   SubstructureNotifyMask, &xev);
}
			       
static gboolean find_atom(Atom *atoms, int n, const char *name)
{
	Atom a = gdk_atom_intern(name, FALSE);
	int i;

	for (i = 0; i < n; i++)
		if (a == atoms[i])
			return TRUE;
	return FALSE;
}

static gboolean get_supported_atoms(Atom **atoms, unsigned long *natoms, const char *name)
{
	Atom supported = gdk_atom_intern(name, FALSE), r_type;
	unsigned long bremain;
	int r_format, p;

	*atoms = NULL;
	gdk_error_trap_push();
	p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), supported,
			       0, 1000, False, XA_ATOM, &r_type, &r_format,
			       natoms, &bremain, (unsigned char **) atoms);
	if (gdk_error_trap_pop() || p != Success || r_type != XA_ATOM ||
	    *natoms == 0 || *atoms == NULL)
		return FALSE;

	return TRUE;
}

static void net_wm_check_features(void)
{
	Atom *atoms;
	unsigned long n_atoms;

	if (!get_supported_atoms(&atoms, &n_atoms, "_NET_SUPPORTED"))
		return;

	if (find_atom(atoms, n_atoms, "_NET_WM_STATE"))
	{
		if (!set_always_func &&
		    find_atom(atoms, n_atoms, "_NET_WM_STATE_ABOVE"))
			set_always_func = net_wm_set_window_above;
		if (!set_always_func &&
		    find_atom(atoms, n_atoms, "_NET_WM_STATE_STAYS_ON_TOP"))
			set_always_func = net_wm_set_window_always;
		if (!set_sticky_func &&
		    find_atom(atoms, n_atoms, "_NET_WM_DESKTOP"))
			set_sticky_func = net_wm_set_desktop;
		if (!set_skip_taskbar_func &&
		    find_atom(atoms, n_atoms, "_NET_WM_STATE_SKIP_TASKBAR"))
			set_skip_taskbar_func = net_wm_set_skip_taskbar;
	}

	if (find_atom(atoms, n_atoms, "_NET_WM_MOVERESIZE"))
		move_resize_func = net_wm_move_resize;

	XFree(atoms);
}

static void gnome_wm_check_features(void)
{
	Atom *atoms;
	unsigned long n_atoms;

	if (!get_supported_atoms(&atoms, &n_atoms, "_WIN_PROTOCOLS"))
		return;

	if (!set_always_func &&
	    find_atom(atoms, n_atoms, "_WIN_LAYER"))
		set_always_func = gnome_wm_set_window_always;
	if (!set_sticky_func &&
	    find_atom(atoms, n_atoms, "_WIN_STATE"))
		set_sticky_func = gnome_wm_set_window_sticky;
	if (!set_skip_taskbar_func &&
	    find_atom(atoms, n_atoms, "_WIN_HINTS"))
		set_skip_taskbar_func = gnome_wm_set_skip_taskbar;

	XFree(atoms);
}

void check_wm_hints(void)
{
	if (net_wm_found())
		net_wm_check_features();
	if (gnome_wm_found())
		gnome_wm_check_features();
}
