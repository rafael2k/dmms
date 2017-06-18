#include "xmms.h"


typedef struct
{
	GtkWidget *w;
	gint offset_x, offset_y;
} DockedWindow;

void dock_get_widget_pos(GtkWidget *w, gint *x,gint *y)
{
	*x = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(w), "window_x"));
	*y = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(w), "window_y"));
}

static gint docked_list_compare(DockedWindow *a, DockedWindow *b)
{
	if (a->w == b->w)
		return 0;
	return 1;
}

static void snap_edge(gint *x, gint *y, gint w, gint h, gint bx, gint by, gint bw, gint bh)
{
	gint sd = cfg.snap_distance;

	if ((*x + w > bx - sd) && (*x + w < bx + sd) &&
	    (*y > by - h - sd) && (*y < by + bh + sd))
	{
		*x = bx - w;
		if((*y > by - sd) && (*y < by + sd))
			*y = by;
		if((*y + h > by + bh - sd) && (*y + h < by + bh + sd))
			*y = by + bh - h;
	}
	if((*x > bx + bw - sd) && (*x < bx + bw + sd) &&
	   (*y > by - h - sd) && (*y < by + bh + sd))
	{
		*x = bx + bw;
		if((*y > by - sd) && (*y < by + sd))
			*y = by;
		if((*y + h > by + bh - sd) && (*y + h < by + bh +sd))
			*y = by + bh - h;
	}
}

static void snap(gint *x,gint *y,gint w, gint h, gint bx, gint by, gint bw, gint bh)
{
	snap_edge(x,y,w,h,bx,by,bw,bh);
	snap_edge(y,x,h,w,by,bx,bh,bw);
}

static void calc_snap_offset(GList *dlist, GList *wlist, gint x, gint y,gint *off_x, gint *off_y)
{
	gint nx,ny,nw,nh,sx,sy,sw,sh;
	GtkWidget *w;
	GList *dnode,*wnode;
	DockedWindow temp,*dw;
  

	*off_x = 0;
	*off_y = 0;

	if (!cfg.snap_windows)
		return;

	/*
	 * FIXME: Why not break out of the loop when we find someting
	 * to snap to?
	 */
	for (dnode = dlist; dnode; dnode = g_list_next(dnode))
	{
		dw = dnode->data;
		gdk_window_get_size(dw->w->window, &nw, &nh);


		nx = dw->offset_x + *off_x + x;
		ny = dw->offset_y + *off_y + y;

		/* Snap to screen edges */
		if (abs(nx) < cfg.snap_distance)
			*off_x -= nx;
		if (abs(ny) < cfg.snap_distance)
			*off_y -= ny;
		if (abs(nx + nw - gdk_screen_width()) < cfg.snap_distance)
			*off_x -= nx + nw - gdk_screen_width();
		if (abs(ny + nh - gdk_screen_height()) < cfg.snap_distance)
			*off_y -= ny + nh - gdk_screen_height();

		/* Snap to other windows */
		for (wnode = wlist; wnode; wnode = g_list_next(wnode))
		{
			temp.w = wnode->data;
			if (g_list_find_custom(dlist, &temp, (GCompareFunc)docked_list_compare))
				/* These windows are already docked */
				continue;

			w = GTK_WIDGET(wnode->data);
			dock_get_widget_pos(w, &sx, &sy);
			gdk_window_get_size(w->window, &sw, &sh);
			
			nx = dw->offset_x + *off_x + x;
			ny = dw->offset_y + *off_y + y;

			snap(&nx, &ny, nw, nh, sx, sy, sw, sh);

			*off_x += nx - (dw->offset_x + *off_x + x);
			*off_y += ny - (dw->offset_y + *off_y + y);
		}
	}
}


static gboolean is_docked(gint a_x, gint a_y, gint a_w, gint a_h,
			  gint b_x, gint b_y, gint b_w, gint b_h)
{
	if(((a_x == b_x + b_w) || (a_x + a_w == b_x)) &&
	   (b_y + b_h >= a_y) && (b_y <= a_y + a_h))
		return TRUE;

	if(((a_y == b_y + b_h) || (a_y + a_h == b_y)) &&
	   (b_x >= a_x - b_w) && (b_x <= a_x + a_w))
		return TRUE;

	return FALSE;
}     

/*
 * Builds a list of all windows that are docked to the window "w".
 * Recursively adds all windows that are docked to the windows that are
 * docked to "w" and so on...
 * FIXME: init_off_?  ?
 */

static GList *get_docked_list(GList *dlist, GList *wlist, GtkWidget *w, gint init_off_x, gint init_off_y)
{
	GList *node;
	DockedWindow *dwin, temp;
	gint w_x, w_y, w_width, w_height;
	gint t_x, t_y, t_width, t_height;
  

	dock_get_widget_pos(w, &w_x, &w_y);
	gdk_window_get_size(w->window,&w_width,&w_height);
	if (!dlist)
	{
		dwin = g_malloc0(sizeof(DockedWindow));
		dwin->w = w;
		dlist = g_list_append(dlist,dwin);
	}
	
	for (node = wlist; node; node = g_list_next(node))
	{
		temp.w = node->data;
		if (g_list_find_custom(dlist, &temp, (GCompareFunc)docked_list_compare))
			continue;

		dock_get_widget_pos(GTK_WIDGET(node->data), &t_x, &t_y);
		gdk_window_get_size(GTK_WIDGET(node->data)->window, &t_width, &t_height);
		if (is_docked(w_x, w_y, w_width, w_height, t_x, t_y, t_width, t_height))
		{
			dwin = g_malloc(sizeof(DockedWindow));
			dwin->w = node->data;
			
			dwin->offset_x = t_x - w_x + init_off_x;
			dwin->offset_y = t_y - w_y + init_off_y;

			dlist = g_list_append(dlist, dwin);

			dlist = get_docked_list(dlist, wlist, dwin->w, dwin->offset_x, dwin->offset_y);
		}
	}
	return dlist;
}

static void free_docked_list(GList *dlist)
{
	GList *node;

	for(node = dlist; node; node = g_list_next(node))
		g_free(node->data);
	g_list_free(dlist);
}

static void docked_list_move(GList *list, gint x, gint y)
{
	GList *node;
	DockedWindow *dw;

	for (node = list; node; node = g_list_next(node))
	{
		dw = node->data;
		dock_set_uposition(dw->w, x + dw->offset_x, y + dw->offset_y);
		gdk_flush();
	}
}

static GList *shade_move_list(GList *list, GtkWidget *widget, gint offset)
{
	gint x, y, w, h;
	GList *node;
	DockedWindow *dw;

	dock_get_widget_pos(widget, &x, &y);
	gdk_window_get_size(widget->window, &w, &h);
 
  
	for (node = list; node;)
	{
		gint dx, dy, dwidth, dheight;

		dw = node->data;
		dock_get_widget_pos(dw->w, &dx, &dy);
		gdk_window_get_size(dw->w->window, &dwidth,&dheight);
		if (is_docked(x, y, w, h, dx, dy, dwidth, dheight) &&
		    ((dx + dwidth) > x && dx < (x + w)))
		{
			list = g_list_remove_link(list, node);
			g_list_free_1(node);

			node = list = shade_move_list(list, dw->w, offset);
		}
		else
			node = g_list_next(node);
	}
	dock_set_uposition(widget, x, y + offset);
	return list;
}

/*
 * Builds a list of the windows in the list of DockedWindows "winlist"
 * that are docked to the top or bottom of the window, and recursively
 * adds all windows that are docked to the top or bottom of that window,
 * and so on...
 * Note: The data in "winlist" is not copied.
 */
static GList *find_shade_list(GtkWidget *widget, GList *winlist, GList *shade_list)
{
	gint x,y,w,h;
	gint dx, dy, dwidth, dheight;
	GList *node;
	
	dock_get_widget_pos(widget, &x, &y);
	gdk_window_get_size(widget->window, &w, &h);
	for(node = winlist; node; node = g_list_next(node))
	{
		DockedWindow *dw = node->data;
		if (g_list_find_custom(shade_list, dw, (GCompareFunc)docked_list_compare))
			continue;
		dock_get_widget_pos(dw->w, &dx, &dy);
		gdk_window_get_size(dw->w->window, &dwidth, &dheight);

		/* FIXME. Is the is_docked() necessary? */
		if (is_docked(x, y, w, h, dx, dy, dwidth, dheight) &&
		    ((dx + dwidth) > x && dx < (x + w)))
		{
			shade_list = g_list_append(shade_list, dw);
			shade_list = find_shade_list(dw->w, winlist, shade_list);
		}
	}
	return shade_list;
}

static void dock_window_resize(GtkWidget *widget, int new_w, int new_h, int w, int h)
{
	gdk_window_set_hints(widget->window, 0, 0, MIN(w, new_w), MIN(h, new_h),
			     MAX(w, new_w), MAX (h, new_h),
			     GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
	gdk_window_resize(widget->window, new_w, new_h);
	gdk_window_set_hints(widget->window, 0, 0, new_w, new_h, new_w, new_h,
			     GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
}

void dock_shade(GList *window_list, GtkWidget *widget, gint new_h)
{
	gint x, y, w, h, off_y, orig_off_y;
	GList *node, *docked_list, *slist;
	DockedWindow *dw;

	dock_get_widget_pos(widget, &x, &y);
	gdk_window_get_size(widget->window, &w, &h);

	if (cfg.show_wm_decorations)
	{
		dock_window_resize(widget, w, new_h, w, h);
		return;
	}

	docked_list = get_docked_list(NULL, window_list, widget, 0, 0);
	slist = find_shade_list(widget, docked_list, NULL);

	off_y = new_h - h;
	do
	{
		orig_off_y = off_y;
		for (node = slist; node; node = g_list_next(node))
		{
			gint dx, dy, dwidth, dheight;
			
			dw = node->data;
			if (dw->w == widget)
				continue;
			dock_get_widget_pos(dw->w, &dx, &dy);
			gdk_window_get_size(dw->w->window, &dwidth, &dheight);
			if ((dy >= y) &&
			    ((dy + off_y + dheight) > gdk_screen_height()))
				off_y -= (dy + off_y + dheight) - gdk_screen_height();
			else if ((dy >= y) &&
				 ((dy + dheight) == gdk_screen_height()))
				off_y = 0;

			if (((dy >= y) && ((dy + off_y) < 0)))
				off_y -= dy + off_y;
			if ((dy < y) && ((dy + (off_y - (new_h - h))) < 0))
				off_y -= dy + (off_y - (new_h - h));
		}
	} while (orig_off_y != off_y);
	if (slist)
	{
		GList *mlist = g_list_copy(slist);

		/* Remove this widget from the list */
		for (node = mlist; node; node = g_list_next(node))
		{
			dw = node->data;
			if (dw->w == widget)
			{
				mlist = g_list_remove_link(mlist, node);
				g_list_free_1(node);
				break;
			}
		}
		for (node = mlist; node;)
		{
			GList *temp;
			gint dx, dy, dwidth, dheight;
	  
			dw = node->data;

			dock_get_widget_pos(dw->w, &dx, &dy);
			gdk_window_get_size(dw->w->window, &dwidth,&dheight);
			/*
			 * Find windows that are directly docked to this window,
			 * move it, and any windows docked to that window again
			 */
			if (is_docked(x, y, w, h, dx, dy, dwidth, dheight) &&
			    ((dx + dwidth) > x && dx < (x + w)))
			{
				mlist = g_list_remove_link(mlist, node);
				g_list_free_1(node);
				if (dy > y)
					temp = shade_move_list(mlist, dw->w, off_y);
				else if(off_y - (new_h - h) != 0)
					temp = shade_move_list(mlist, dw->w, off_y - (new_h - h));
				else
					temp = mlist;
				node = mlist = temp;
			}
			else
				node = g_list_next(node);
		}
		g_list_free(mlist);
	}
	g_list_free(slist);
	free_docked_list(docked_list);
	dock_set_uposition(widget, x, y + off_y - (new_h - h));
	dock_window_resize(widget, w, new_h, w, h);
}

static GList *resize_move_list(GList *list, GtkWidget *widget, gint offset_x, gint offset_y)
{
	gint x,y,w,h;
	GList *node;
	DockedWindow *dw;

	dock_get_widget_pos(widget,&x,&y);
	gdk_window_get_size(widget->window, &w,&h);
 
  
	for(node = list; node;)
	{
		gint dx, dy, dwidth, dheight;
		dw = node->data;
		dock_get_widget_pos(dw->w, &dx, &dy);
		gdk_window_get_size(dw->w->window, &dwidth,&dheight);
		if(is_docked(x,y,w,h,dx,dy,dwidth,dheight))
		{
			
			list = g_list_remove_link(list, node);
			g_list_free_1(node);			
			node = list = resize_move_list(list, dw->w, offset_x, offset_y);
		}
		else
			node = g_list_next(node);
	}
	dock_set_uposition(widget, x + offset_x, y + offset_y);
	return list;
}

static GList *resize_calc_offset(GList *list, GtkWidget *widget, gint offset_x, gint offset_y, gint *goffset_x, gint *goffset_y)
{
	gint x, y, w, h;
	GList *node;
	DockedWindow *dw;

	dock_get_widget_pos(widget, &x, &y);
	gdk_window_get_size(widget->window, &w, &h);
 
  
	for(node = list; node;)
	{
		gint dx, dy, dwidth, dheight;
		dw = node->data;
		dock_get_widget_pos(dw->w, &dx, &dy);
		gdk_window_get_size(dw->w->window, &dwidth,&dheight);
		if (is_docked(x, y, w, h, dx, dy, dwidth, dheight))
		{
			if (dx + offset_x + dwidth > gdk_screen_width())
			{
				offset_x -= dx + offset_x + dwidth - gdk_screen_width();
				(*goffset_x) -= dx + offset_x + dwidth - gdk_screen_width();
			}
			if (dy + offset_y + dheight > gdk_screen_height())
			{
				offset_y -= dy + offset_y + dheight - gdk_screen_height();
				(*goffset_y) -= dy + offset_y + dheight - gdk_screen_height();
			}
			list = g_list_remove_link(list, node);
			g_list_free_1(node);			
			node = list = resize_calc_offset(list, dw->w, offset_x, offset_y, goffset_x, goffset_y);
		}
		else
			node = g_list_next(node);
	}
	return list;
}

void dock_resize(GList *window_list, GtkWidget *widget, gint new_w, gint new_h)
{
	gint x, y, w, h;
	gint dx, dy, dwidth, dheight;
	gint off_x, off_y;
	GList *list, *dlist = NULL, *tlist = NULL, *mlist = NULL, *node;
	DockedWindow *dw;
	
	dock_get_widget_pos(widget, &x, &y);
	gdk_window_get_size(widget->window, &w, &h);
	if (cfg.show_wm_decorations)
	{
		dock_window_resize(widget, new_w, new_h, w, h);
		return;
	}

	list = get_docked_list(NULL, window_list,widget,0,0);

	off_x = 0;
	off_y = 0;

	for (node = list; node; node = g_list_next(node))
	{
		dw = node->data;
		if(dw->w != widget)
		{
			dock_get_widget_pos(dw->w, &dx, &dy);
			gdk_window_get_size(dw->w->window,&dwidth,&dheight);
			if(is_docked(x, y, w, h, dx, dy, dwidth, dheight))
				dlist = g_list_append(dlist, dw);
			else
				mlist = g_list_append(mlist, dw);
		}
	}
	tlist = g_list_copy(mlist);
	for (node = dlist; node; node = g_list_next(node))
	{
		gint doff_x, doff_y;
		dw = node->data;
		dock_get_widget_pos(dw->w, &dx, &dy);
		gdk_window_get_size(dw->w->window,&dwidth,&dheight);
		if (dx - x - w == 0)
			doff_x = (x + off_x + new_w) - dx;
		else
			doff_x = (x + off_x + (dx - x)) - dx;
				  
		if (dy - y - h == 0)
			doff_y = (y + off_y + new_h) - dy;
		else
			doff_y = (y + off_y + (dy - y)) - dy;
		
		if (dx + doff_x + dwidth > gdk_screen_width())
		{
			off_x -= dx + doff_x + dwidth - gdk_screen_width();
			doff_x -= dx + doff_x + dwidth - gdk_screen_width();
		}
		if (dy + doff_y + dheight > gdk_screen_height())
		{
			off_y -= dy + doff_y + dheight - gdk_screen_height();
			doff_y -= dy + doff_y + dheight - gdk_screen_height();
		}		
		tlist = resize_calc_offset(tlist, dw->w, doff_x, doff_y, &off_x, &off_y);
	}
	if((x + off_x + new_w) > gdk_screen_width())
		off_x -= x + off_x + new_w - gdk_screen_width();
	if((y + off_y + new_h) > gdk_screen_height())
		off_y -= y + off_y + new_h - gdk_screen_height();
		
	g_list_free(tlist);
	for(node = dlist; node; node = g_list_next(node))
	{
		gint doff_x, doff_y;
		dw = node->data;
		dock_get_widget_pos(dw->w, &dx, &dy);
		if(dx - x - w == 0)
			doff_x = (x + off_x + new_w) - dx;
		else
			doff_x = (x + off_x + (dx - x)) - dx;
		
		if(dy - y - h == 0)
			doff_y = (y + off_y + new_h) - dy;
		else
			doff_y = (y + off_y + (dy - y)) - dy;
		mlist = resize_move_list(mlist, dw->w, doff_x, doff_y);
		dock_set_uposition(dw->w, dx + doff_x, dy + doff_y);
	}
		   
	
	dock_set_uposition(widget, x + off_x, y + off_y);
	dock_window_resize(widget, new_w, new_h, w, h);
}
	
void dock_move_press(GList *window_list, GtkWidget *w, GdkEventButton *event, gboolean move_list)
{
	gint mx,my;
	DockedWindow *dwin;

	if (cfg.show_wm_decorations)
		return;

	gdk_window_raise(w->window);
	gdk_window_get_pointer(w->window, &mx, &my, NULL);
	gtk_object_set_data(GTK_OBJECT(w), "move_offset_x", GINT_TO_POINTER(mx));
	gtk_object_set_data(GTK_OBJECT(w), "move_offset_y", GINT_TO_POINTER(my));
	if (move_list)
		gtk_object_set_data(GTK_OBJECT(w),"docked_list", get_docked_list(NULL,window_list,w,0,0));
	else
	{
		dwin = g_malloc0(sizeof(DockedWindow));
		dwin->w = w;
		gtk_object_set_data(GTK_OBJECT(w),"docked_list", g_list_append(NULL, dwin));
	}
	gtk_object_set_data(GTK_OBJECT(w), "window_list", window_list);
	gtk_object_set_data(GTK_OBJECT(w), "is_moving", GINT_TO_POINTER(1));
}

void dock_move_motion(GtkWidget *w, GdkEventMotion *event)
{
	gint offset_x, offset_y, win_x, win_y, x, y, mx, my;
	GList *dlist;
	GList *window_list;
	XEvent ev;

	gdk_flush();
	while (XCheckTypedEvent(GDK_DISPLAY(), MotionNotify, &ev));

	if (!gtk_object_get_data(GTK_OBJECT(w), "is_moving"))
		return;
	
	offset_x = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(w), "move_offset_x"));
	offset_y = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(w), "move_offset_y"));
	dlist = gtk_object_get_data(GTK_OBJECT(w), "docked_list");
	window_list = gtk_object_get_data(GTK_OBJECT(w), "window_list");
		
	dock_get_widget_pos(w, &win_x, &win_y);

	gdk_window_get_pointer(NULL, &mx, &my, NULL);

	x = mx - offset_x;
	y = my - offset_y;

	calc_snap_offset(dlist, window_list, x, y, &offset_x, &offset_y);
	x += offset_x;
	y += offset_y;

	docked_list_move(dlist, x, y);
}

void dock_move_release(GtkWidget *w)
{
	GList *dlist;
	gtk_object_remove_data(GTK_OBJECT(w), "is_moving");
	gtk_object_remove_data(GTK_OBJECT(w), "move_offset_x");
	gtk_object_remove_data(GTK_OBJECT(w), "move_offset_y");
	if ((dlist = gtk_object_get_data(GTK_OBJECT(w), "docked_list")) != NULL)
		free_docked_list(dlist);
	gtk_object_remove_data(GTK_OBJECT(w), "docked_list");
	gtk_object_remove_data(GTK_OBJECT(w), "window_list");
}

gboolean dock_is_moving(GtkWidget *w)
{
	if(gtk_object_get_data(GTK_OBJECT(w),"is_moving"))
		return TRUE;
	return FALSE;
}

static gboolean configure_event(GtkWidget *w, GdkEventConfigure *event, gpointer data)
{
	gint x, y;

	if (!GTK_WIDGET_VISIBLE(w))
		return FALSE;
	
	gdk_window_get_deskrelative_origin(w->window, &x, &y);
	gtk_object_set_data(GTK_OBJECT(w), "window_x", GINT_TO_POINTER(x));
	gtk_object_set_data(GTK_OBJECT(w), "window_y", GINT_TO_POINTER(y));

	return FALSE;
}
	
GList *dock_add_window(GList *list, GtkWidget *w)
{
	list = g_list_append(list, w);
	gtk_signal_connect(GTK_OBJECT(w), "configure_event", GTK_SIGNAL_FUNC(configure_event), NULL);
	return list;
}
	
void dock_set_uposition(GtkWidget *w, gint x, gint y)
{
	gtk_widget_set_uposition(w, x, y);
	gtk_object_set_data(GTK_OBJECT(w), "window_x", GINT_TO_POINTER(x));
	gtk_object_set_data(GTK_OBJECT(w), "window_y", GINT_TO_POINTER(y));
}
