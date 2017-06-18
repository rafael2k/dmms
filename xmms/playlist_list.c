/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
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

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#include <X11/Xatom.h>

static GdkFont *playlist_list_font = NULL;

static int playlist_list_auto_drag_down_func(gpointer data)
{
	PlayList_List *pl = data;

	if (pl->pl_auto_drag_down)
	{
		playlist_list_move_down(pl);
		pl->pl_first++;
		playlistwin_update_list();
		return TRUE;

	}
	return FALSE;
}

static int playlist_list_auto_drag_up_func(gpointer data)
{
	PlayList_List *pl = data;

	if (pl->pl_auto_drag_up)
	{
		playlist_list_move_up(pl);
		pl->pl_first--;
		playlistwin_update_list();
		return TRUE;

	}
	return FALSE;
}

void playlist_list_move_up(PlayList_List *pl)
{
	GList *list;

	PL_LOCK();
	if ((list = get_playlist()) == NULL)
	{
		PL_UNLOCK();
		return;
	}
	if (((PlaylistEntry *) list->data)->selected)
	{
		/* We are at the top */
		PL_UNLOCK();
		return;
	}
	while (list)
	{
		if (((PlaylistEntry *) list->data)->selected)
			glist_moveup(list);
		list = g_list_next(list);
	}
	PL_UNLOCK();
	if (pl->pl_prev_selected != -1)
		pl->pl_prev_selected--;
	if (pl->pl_prev_min != -1)
		pl->pl_prev_min--;
	if (pl->pl_prev_max != -1)
		pl->pl_prev_max--;
}

void playlist_list_move_down(PlayList_List *pl)
{
	GList *list;

	PL_LOCK();
	if ((list = g_list_last(get_playlist())) == NULL)
	{
		PL_UNLOCK();
		return;
	}
	if (((PlaylistEntry *) list->data)->selected)
	{
		/* We are at the bottom */
		PL_UNLOCK();
		return;
	}
	while (list)
	{
		if (((PlaylistEntry *) list->data)->selected)
			glist_movedown(list);
		list = g_list_previous(list);
	}
	PL_UNLOCK();
	if (pl->pl_prev_selected != -1)
		pl->pl_prev_selected++;
	if (pl->pl_prev_min != -1)
		pl->pl_prev_min++;
	if (pl->pl_prev_max != -1)
		pl->pl_prev_max++;
}

void playlist_list_button_press_cb(GtkWidget * widget, GdkEventButton * event, PlayList_List * pl)
{
	if (event->button == 1 && pl->pl_fheight &&
	    inside_widget(event->x, event->y, &pl->pl_widget))
	{
		int nr, y;

		y = event->y - pl->pl_widget.y;
		nr = (y / pl->pl_fheight) + pl->pl_first;
		if (nr >= get_playlist_length())
			nr = get_playlist_length() - 1;
		if (!(event->state & GDK_CONTROL_MASK))
			playlist_select_all(FALSE);
		
		if (event->state & GDK_SHIFT_MASK && pl->pl_prev_selected != -1)
		{
			playlist_select_range(pl->pl_prev_selected, nr, TRUE);
			pl->pl_prev_min = pl->pl_prev_selected;
			pl->pl_prev_max = nr;
			pl->pl_drag_pos = nr - pl->pl_first;
		}
		else
		{
			if (playlist_select_invert(nr))
			{
				if (event->state & GDK_CONTROL_MASK)
				{
					if (pl->pl_prev_min == -1)
					{
						pl->pl_prev_min =
							pl->pl_prev_selected;
						pl->pl_prev_max =
							pl->pl_prev_selected;
					}
					if (nr < pl->pl_prev_min)
						pl->pl_prev_min = nr;
					else if (nr > pl->pl_prev_max)
						pl->pl_prev_max = nr;
				}
				else
					pl->pl_prev_min = -1;
				pl->pl_prev_selected = nr;
				pl->pl_drag_pos = nr - pl->pl_first;
			}
		}
		if (event->type == GDK_2BUTTON_PRESS)
		{
			/*
			 * Ungrab the pointer to prevent us from
			 * hanging on to it during the sometimes slow
			 * playlist_play().
			 */
			gdk_pointer_ungrab(GDK_CURRENT_TIME);
			gdk_flush();
			playlist_set_position(nr);
			if (!get_input_playing())
				playlist_play();
		}
		pl->pl_dragging = TRUE;
		playlistwin_update_list();
	}
}

int playlist_list_get_playlist_position(PlayList_List *pl, int x, int y)
{
	int iy, length;
	
	if (!inside_widget(x, y, pl) || !pl->pl_fheight)
		return -1;

	if ((length = get_playlist_length()) == 0)
		return -1;
	iy = y - pl->pl_widget.y;

	return(MIN((iy / pl->pl_fheight) + pl->pl_first, length - 1));
}

void playlist_list_motion_cb(GtkWidget * widget, GdkEventMotion * event, PlayList_List * pl)
{
	gint nr, y, off, i;

	if (pl->pl_dragging)
	{
		y = event->y - pl->pl_widget.y;
		nr = (y / pl->pl_fheight);
		if (nr < 0)
		{
			nr = 0;
			if (!pl->pl_auto_drag_up)
			{
				pl->pl_auto_drag_up = TRUE;
				pl->pl_auto_drag_up_tag = gtk_timeout_add(100, playlist_list_auto_drag_up_func, pl);
			}
		}
		else if (pl->pl_auto_drag_up)
			pl->pl_auto_drag_up = FALSE;

		if (nr >= pl->pl_num_visible)
		{
			nr = pl->pl_num_visible - 1;
			if (!pl->pl_auto_drag_down)
			{
				pl->pl_auto_drag_down = TRUE;
				pl->pl_auto_drag_down_tag = gtk_timeout_add(100, playlist_list_auto_drag_down_func, pl);
			}
		}
		else if (pl->pl_auto_drag_down)
			pl->pl_auto_drag_down = FALSE;

		off = nr - pl->pl_drag_pos;
		if (off)
		{
			for (i = 0; i < abs(off); i++)
			{
				if (off < 0)
					playlist_list_move_up(pl);
				else
					playlist_list_move_down(pl);

			}
			playlistwin_update_list();
		}
		pl->pl_drag_pos = nr;
	}
}

void playlist_list_button_release_cb(GtkWidget * widget, GdkEventButton * event, PlayList_List * pl)
{
	pl->pl_dragging = FALSE;
	pl->pl_auto_drag_down = FALSE;
	pl->pl_auto_drag_up = FALSE;
}

#ifdef HAVE_WCHAR_H

static GdkWChar * find_in_wstr(GdkWChar *haystack, char * needle)
{
	/* This will only work if needle is 7bit ASCII characters only */
	GdkWChar *tmp = haystack;
	int i = 0;

	if (haystack == NULL)
		return NULL;

	if (needle == NULL || *needle == '\0')
		return haystack;

	for (; *tmp != L'\0'; tmp++)
	{
		if (*tmp == needle[i])
		{
			if (needle[i + 1] == '\0')
				return (tmp - i);
			i++;
		}
		else if (i > 0)
		{
			tmp -= i;
			i = 0;
		}
	}
	return NULL;
}


void playlist_list_draw_string_wc(PlayList_List *pl, GdkFont *font, gint line, gint width, gchar *text)
{
	GdkWChar *wtext;
	int len, newlen;
	/*
	 * Convert the string to a wide character string to avoid
	 * destroying multibyte strings, when converting underscores,
	 * "%20" etc.
	 */
	/*
	 * Allocate some extra space, we might extend it by one
	 * character below
	 */
	wtext = g_malloc((strlen(text) + 3) * sizeof(GdkWChar));
	len = gdk_mbstowcs(wtext, text, strlen(text) + 1);
	if (len == -1)
	{
		/* Conversion failed */
		for (len = 0; text[len] != '\0'; len++)
			wtext[len] = text[len];
	}
	wtext[len] = L'\0';
	if (cfg.convert_underscore)
	{
		int i;
		for (i = 0; i < len; i++)
			if (wtext[i] == L'_')
				wtext[i] = L' ';
	}

	if (cfg.convert_twenty && len > 2)
	{
		GdkWChar *wtmp;
		while ((wtmp = find_in_wstr(wtext, "%20")) != NULL)
		{
			GdkWChar *wtmp2 = wtmp + 3;
			*(wtmp++) = L' ';
			while (*wtmp2)
				*(wtmp++) = *(wtmp2++);
			*wtmp = L'\0';
			len -= 2;
		}
	}
		
	newlen = len + 2;

	while (gdk_text_width_wc(font, wtext, len) > width && len > 4)
	{
		/*
		 * First check if the string gets short enough by
		 * extending it with one character and then convert
		 * three characters to dots.  If it's still too long,
		 * remove charaters, one by one.
		 */
		len = newlen--;
		wtext[len - 3] = L'.';
		wtext[len - 2] = L'.';
		wtext[len - 1] = L'.';
		wtext[len] = L'\0';
	}
	
	gdk_draw_text_wc(pl->pl_widget.parent, font, pl->pl_widget.gc,
			 pl->pl_widget.x,
			 pl->pl_widget.y + line * pl->pl_fheight + font->ascent,
			 wtext, len);
	g_free(wtext);
}

#else /* !HAVE_WCHAR_H */

#define playlist_list_draw_string_wc playlist_list_draw_string

#endif /* HAVE_WCHAR_H */

void playlist_list_draw_string(PlayList_List *pl, GdkFont *font, gint line, gint width, gchar *text)
{
	int len;
	char *tmp;

	if (cfg.convert_underscore)
		while ((tmp = strchr(text, '_')) != NULL)
			*tmp = ' ';
	if (cfg.convert_twenty)
		while ((tmp = strstr(text, "%20")) != NULL)
		{
			char *tmp2 = tmp + 3;
			*(tmp++) = ' ';
			while (*tmp2)
				*(tmp++) = *(tmp2++);
			*tmp = '\0';
		}
	len = strlen(text);
	while (gdk_text_width(font, text, len) > width && len > 4)
	{
		len--;
		text[len - 3] = '.';
		text[len - 2] = '.';
		text[len - 1] = '.';
		text[len] = '\0';
	}
	
	gdk_draw_text(pl->pl_widget.parent, font, pl->pl_widget.gc, pl->pl_widget.x, pl->pl_widget.y + line * pl->pl_fheight + font->ascent, text, len);
}

void playlist_list_draw(Widget * w)
{
	PlayList_List *pl = (PlayList_List *) w;
	GList *list;
	GdkGC *gc;
	GdkPixmap *obj;
	int width, height;
	char *text, *title;
	int i, tw, max_first;

	gc = pl->pl_widget.gc;
	width = pl->pl_widget.width;
	height = pl->pl_widget.height;

	obj = pl->pl_widget.parent;

	gdk_gc_set_foreground(gc, get_skin_color(SKIN_PLEDIT_NORMALBG));
	gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x, pl->pl_widget.y,
			   width, height);

	if (playlist_list_font == NULL)
	{
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "Couldn't open playlist font");
		return;
	}
	

	PL_LOCK();
	list = get_playlist();
	pl->pl_fheight = playlist_list_font->ascent + playlist_list_font->descent + 1;
	pl->pl_num_visible = height / pl->pl_fheight;

	max_first = g_list_length(list) - pl->pl_num_visible;
	if (max_first < 0)
		max_first = 0;
	if (pl->pl_first >= max_first)
		pl->pl_first = max_first;
	if (pl->pl_first < 0)
		pl->pl_first = 0;
	for (i = 0; i < pl->pl_first; i++)
		list = g_list_next(list);

	for (i = pl->pl_first;
	     list && i < pl->pl_first + pl->pl_num_visible;
	     list = list->next,	i++)
	{
		char qstr[20] = "", length[40] = "";
		int pos;
		
		PlaylistEntry *entry = (PlaylistEntry *) list->data;
		if (entry->selected)
		{
			gdk_gc_set_foreground(gc, get_skin_color(SKIN_PLEDIT_SELECTEDBG));
			gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x,
					   pl->pl_widget.y +
					   ((i - pl->pl_first) * pl->pl_fheight),
					   width, pl->pl_fheight);
		}
		if (i == __get_playlist_position())
			gdk_gc_set_foreground(gc, get_skin_color(SKIN_PLEDIT_CURRENT));
		else
			gdk_gc_set_foreground(gc, get_skin_color(SKIN_PLEDIT_NORMAL));

		if (entry->title)
			title = entry->title;
		else
			title = g_basename(entry->filename);

		pos = playlist_get_queue_position(entry);
		
		if (pos != -1)
			sprintf(qstr, "|%d|%s", pos + 1,
				entry->length != -1 ? " " : "");

		if (entry->length != -1)
			sprintf(length, "%d:%-2.2d", entry->length / 60000,
				(entry->length / 1000) % 60);

		if (pos != -1 || entry->length != -1)
		{
			int x, y;
			char tail[60];

			sprintf(tail, "%s%s", qstr, length);
			x = pl->pl_widget.x + width -
				gdk_text_width(playlist_list_font,
					       tail, strlen(tail)) - 2;
			y = pl->pl_widget.y +
				(i - pl->pl_first) * pl->pl_fheight +
				playlist_list_font->ascent;
			gdk_draw_text(obj, playlist_list_font, gc, x, y,
				      tail, strlen(tail));
			tw = width - gdk_text_width(playlist_list_font,
						    tail, strlen(tail)) - 5;
		}
		else
			tw = width;
		if (cfg.show_numbers_in_pl)
			text = g_strdup_printf("%d. %s", i + 1, title);
		else
			text = g_strdup_printf("%s", title);

		if (cfg.use_fontsets)
			playlist_list_draw_string_wc(pl, playlist_list_font,
						     i - pl->pl_first, tw, text);
		else
			playlist_list_draw_string(pl, playlist_list_font,
						  i - pl->pl_first, tw, text);
		g_free(text);
	}
	PL_UNLOCK();
}

PlayList_List *create_playlist_list(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h)
{
	PlayList_List *pl;

	pl = (PlayList_List *) g_malloc0(sizeof (PlayList_List));
	pl->pl_widget.parent = parent;
	pl->pl_widget.gc = gc;
	pl->pl_widget.x = x;
	pl->pl_widget.y = y;
	pl->pl_widget.width = w;
	pl->pl_widget.height = h;
	pl->pl_widget.visible = TRUE;
	pl->pl_widget.button_press_cb = GTK_SIGNAL_FUNC(playlist_list_button_press_cb);
	pl->pl_widget.button_release_cb = GTK_SIGNAL_FUNC(playlist_list_button_release_cb);
	pl->pl_widget.motion_cb = GTK_SIGNAL_FUNC(playlist_list_motion_cb);
	pl->pl_widget.draw = playlist_list_draw;
	pl->pl_prev_selected = -1;
	pl->pl_prev_min = -1;
	pl->pl_prev_max = -1;
	add_widget(wlist, pl);
	return pl;
}

void playlist_list_set_font(char *font)
{
	if (playlist_list_font)
		gdk_font_unref(playlist_list_font);

	playlist_list_font = util_font_load(font);
}
