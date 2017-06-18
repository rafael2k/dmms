/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2006  Haavard Kvaalen <havardk@xmms.org>
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
#include <gdk/gdkprivate.h>
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <ctype.h>
#ifdef HAVE_FTS_H
#include <fts.h>
#endif

static GQuark quark_popup_data;


/*
 * find_file_recursively() by Jörg Schuler Wed, 17 Feb 1999 23:50:52
 * +0100 Placed under GPL version 2 or (at your option) any later
 * version
 */
/*
 * find <file> in directory <dirname> or subdirectories.  return
 * pointer to complete filename which has to be freed by calling
 * "g_free()" after use. Returns NULL if file could not be found.
 */
gchar *find_file_recursively(const char *dirname, const char *file)
{
	DIR *dir;
	struct dirent *dirent;
	char *result, *found;
	struct stat buf;

	if ((dir = opendir(dirname)) == NULL)
		return NULL;

	while ((dirent = readdir(dir)) != NULL)
	{
		/* Don't scan "." and ".." */
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, ".."))
			continue;
		/* We need this in order to find out if file is directory */
		found = g_strconcat(dirname, "/", dirent->d_name, NULL);
		if (stat(found, &buf) != 0)
		{
			/* Error occured */
			g_free(found);
			closedir(dir);
			return NULL;
		}
		if (!S_ISDIR(buf.st_mode))
		{
			/* Normal file -- maybe just what we are looking for? */
			if (!strcasecmp(dirent->d_name, file))
			{
				if (strlen(found) > FILENAME_MAX)
				{
					/* No good! File + path too long */
					g_free(found);
					closedir(dir);
					return NULL;
				}
				else
				{
					closedir(dir);
					return found;
				}
			}
		}
		g_free(found);
	}
	rewinddir(dir);
	while ((dirent = readdir(dir)) != NULL)
	{
		/* Don't scan "." and ".." */
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, ".."))
			continue;
		/* We need this in order to find out if file is directory */
		found = g_strconcat(dirname, "/", dirent->d_name, NULL);
		if (stat(found, &buf) != 0)
		{
			/* Error occured */
			g_free(found);
			closedir(dir);
			return NULL;
		}
		if (S_ISDIR(buf.st_mode))
		{
			/* Found directory -- descend into it */
			result = find_file_recursively(found, file);
			if (result != NULL)
			{	/* Found file there -- exit */
				g_free(found);
				closedir(dir);
				return result;
			}
		}
		g_free(found);
	}
	closedir(dir);
	return NULL;
}

void del_directory(const char *dirname)
{
#ifdef HAVE_FTS_H
	char * const argv[2] = { (char *) dirname, NULL };
	FTS *fts;
	FTSENT *p;

	fts = fts_open(argv, FTS_PHYSICAL, (int (*)())NULL);
	while ((p = fts_read(fts)) != NULL) {
		switch (p->fts_info) {
		case FTS_D:
			break;
		case FTS_DNR:
		case FTS_ERR:
			break;
		case FTS_DP:
			rmdir(p->fts_accpath);
			break;
		default:
			unlink(p->fts_accpath);
			break;
		}
	}
	fts_close(fts);
#else /* !HAVE_FTS_H */
	DIR *dir;
	struct dirent *dirent;
	char *file;

	if ((dir = opendir(dirname)) != NULL)
	{
		while ((dirent = readdir(dir)) != NULL)
		{
			if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
			{
				file = g_strdup_printf("%s/%s", dirname, dirent->d_name);
				if (unlink(file) == -1)
					if (errno == EISDIR)
						del_directory(file);
				g_free(file);
			}
		}
		closedir(dir);
	}
	rmdir(dirname);
#endif /* !HAVE_FTS_H */
}

GdkImage *create_dblsize_image(GdkImage * img)
{
	GdkImage *dblimg;
	register guint x, y;

	/*
	 * This needs to be optimized
	 */

	dblimg = gdk_image_new(GDK_IMAGE_NORMAL, gdk_visual_get_best_with_depth(img->depth), img->width << 1, img->height << 1);
	if (dblimg->bpp == 1)
	{
		char *srcptr, *ptr, *ptr2;

		srcptr = GDK_IMAGE_XIMAGE(img)->data;
		ptr = GDK_IMAGE_XIMAGE(dblimg)->data;
		ptr2 = ptr + dblimg->bpl;

		for (y = 0; y < img->height; y++)
		{
			for (x = 0; x < img->width; x++)
			{
				gint8 pix = *srcptr++;
				*ptr++ = pix;
				*ptr++ = pix;
				*ptr2++ = pix;
				*ptr2++ = pix;
			}
			srcptr += img->bpl - img->width;
			ptr += (dblimg->bpl << 1) - dblimg->width;
			ptr2 += (dblimg->bpl << 1) - dblimg->width;
		}
	}
	if (dblimg->bpp == 2)
	{
		guint16 *srcptr, *ptr, *ptr2;

		srcptr = (guint16 *) GDK_IMAGE_XIMAGE(img)->data;
		ptr = (guint16 *) GDK_IMAGE_XIMAGE(dblimg)->data;
		ptr2 = ptr + (dblimg->bpl >> 1);

		for (y = 0; y < img->height; y++)
		{
			for (x = 0; x < img->width; x++)
			{
				guint16 pix = *srcptr++;
				*ptr++ = pix;
				*ptr++ = pix;
				*ptr2++ = pix;
				*ptr2++ = pix;
			}
			srcptr += (img->bpl >> 1) - img->width;
			ptr += (dblimg->bpl) - dblimg->width;
			ptr2 += (dblimg->bpl) - dblimg->width;
		}
	}
	if (dblimg->bpp == 3)
	{
		char *srcptr, *ptr, *ptr2;

		srcptr = GDK_IMAGE_XIMAGE(img)->data;
		ptr = GDK_IMAGE_XIMAGE(dblimg)->data;
		ptr2 = ptr + dblimg->bpl;

		for (y = 0; y < img->height; y++)
		{
			for (x = 0; x < img->width; x++)
			{
				char pix1 = *srcptr++;
				char pix2 = *srcptr++;
				char pix3 = *srcptr++;
				*ptr++ = pix1;
				*ptr++ = pix2;
				*ptr++ = pix3;
				*ptr++ = pix1;
				*ptr++ = pix2;
				*ptr++ = pix3;
				*ptr2++ = pix1;
				*ptr2++ = pix2;
				*ptr2++ = pix3;
				*ptr2++ = pix1;
				*ptr2++ = pix2;
				*ptr2++ = pix3;

			}
			srcptr += img->bpl - (img->width * 3);
			ptr += (dblimg->bpl << 1) - (dblimg->width * 3);
			ptr2 += (dblimg->bpl << 1) - (dblimg->width * 3);
		}
	}
	if (dblimg->bpp == 4)
	{
		register guint32 *srcptr, *ptr, *ptr2, pix;

		srcptr = (guint32 *) GDK_IMAGE_XIMAGE(img)->data;
		ptr = (guint32 *) GDK_IMAGE_XIMAGE(dblimg)->data;
		ptr2 = ptr + (dblimg->bpl >> 2);

		for (y = 0; y < img->height; y++)
		{
			for (x = 0; x < img->width; x++)
			{
				pix = *srcptr++;
				*ptr++ = pix;
				*ptr++ = pix;
				*ptr2++ = pix;
				*ptr2++ = pix;
			}
			srcptr += (img->bpl >> 2) - img->width;
			ptr += (dblimg->bpl >> 1) - dblimg->width;
			ptr2 += (dblimg->bpl >> 1) - dblimg->width;
		}
	}
	return dblimg;
}

static char *read_string(const char *filename, const char *section, const char *key, gboolean comment)
{
	FILE *file;
	char *buffer, *ret_buffer = NULL;
	int found_section = 0, off = 0, len = 0;
	struct stat statbuf;

	if (!filename)
		return NULL;

	if ((file = fopen(filename, "r")) == NULL)
		return NULL;

	if (stat(filename, &statbuf) < 0)
	{
		fclose(file);
		return NULL;
	}
		
	buffer = g_malloc(statbuf.st_size);
	fread(buffer, 1, statbuf.st_size, file);
	while (!ret_buffer && off < statbuf.st_size)
	{
		while (off < statbuf.st_size &&
		       (buffer[off] == '\r' || buffer[off] == '\n' ||
			buffer[off] == ' ' || buffer[off] == '\t'))
			off++;
		if (off >= statbuf.st_size)
			break;
		if (buffer[off] == '[')
		{
			int slen = strlen(section);
			off++;
			found_section = 0;
			if (off + slen + 1 < statbuf.st_size &&
			    !strncasecmp(section, &buffer[off], slen))
			{
				off += slen;
				if (buffer[off] == ']')
				{
					off++;
					found_section = 1;
				}
			}
		}
		else if (found_section && off + strlen(key) < statbuf.st_size &&
			 !strncasecmp(key, &buffer[off], strlen(key)))
		{
			off += strlen(key);
			while (off < statbuf.st_size &&
			       (buffer[off] == ' ' || buffer[off] == '\t'))
				off++;
			if (off >= statbuf.st_size)
				break;
			if (buffer[off] == '=')
			{
				off++;
				while (off < statbuf.st_size &&
				       (buffer[off] == ' ' ||
					buffer[off] == '\t'))
					off++;
				if (off >= statbuf.st_size)
					break;
				len = 0;
				while (off + len < statbuf.st_size &&
				       buffer[off + len] != '\r' &&
				       buffer[off + len] != '\n' &&
				       (!comment ||
					buffer[off + len] != ';'))
					len++;
				ret_buffer = g_strndup(&buffer[off], len);
				off += len;
			}
		}
		while (off < statbuf.st_size &&
		       buffer[off] != '\r' && buffer[off] != '\n')
			off++;
	}

	g_free(buffer);
	fclose(file);
	return ret_buffer;
}

char *read_ini_string(const char *filename, const char *section, const char *key)
{
	return read_string(filename, section, key, TRUE);
}

char *read_ini_string_no_comment(const char *filename, const char *section, const char *key)
{
	return read_string(filename, section, key, FALSE);
}

GArray *string_to_garray(const gchar * str)
{
	GArray *array;
	gint temp;
	const gchar *ptr = str;
	gchar *endptr;

	array = g_array_new(FALSE, TRUE, sizeof (gint));
	for (;;)
	{
		temp = strtol(ptr, &endptr, 10);
		if (ptr == endptr)
			break;
		g_array_append_val(array, temp);
		ptr = endptr;
		while (!isdigit(*ptr) && (*ptr) != '\0')
			ptr++;
		if (*ptr == '\0')
			break;
	}
	return (array);
}

GArray *read_ini_array(const gchar * filename, const gchar * section, const gchar * key)
{
	gchar *temp;
	GArray *a;

	if ((temp = read_ini_string(filename, section, key)) == NULL)
		return NULL;
	a = string_to_garray(temp);
	g_free(temp);
	return a;
}

void glist_movedown(GList * list)
{
	gpointer temp;

	if (g_list_next(list))
	{
		temp = list->data;
		list->data = list->next->data;
		list->next->data = temp;
	}
}

void glist_moveup(GList * list)
{
	gpointer temp;

	if (g_list_previous(list))
	{
		temp = list->data;
		list->data = list->prev->data;
		list->prev->data = temp;
	}
}

struct MenuPos
{
	gint x;
	gint y;
};

static void util_menu_position(GtkMenu *menu, gint *x, gint *y, gpointer data)
{
	GtkRequisition requisition;
	gint screen_width;
	gint screen_height;
	struct MenuPos *pos = data;

	gtk_widget_size_request(GTK_WIDGET(menu), &requisition);
      
	screen_width = gdk_screen_width();
	screen_height = gdk_screen_height();
	  
	*x = CLAMP(pos->x - 2, 0, MAX(0, screen_width - requisition.width));
	*y = CLAMP(pos->y - 2, 0, MAX(0, screen_height - requisition.height));
}

static void util_menu_delete_popup_data(GtkObject *object,
					GtkItemFactory *ifactory)
{
	gtk_signal_disconnect_by_func(object,
				      GTK_SIGNAL_FUNC(util_menu_delete_popup_data),
				      ifactory);
	gtk_object_remove_data_by_id(GTK_OBJECT(ifactory), quark_popup_data);
}


/*
 * util_item_factory_popup[_with_data]() is a replacement for
 * gtk_item_factory_popup[_with_data]().
 *
 * The difference is that the menu is always popped up whithin the
 * screen.  This means it does not neccesarily pop up at (x,y).
 */

void util_item_factory_popup_with_data(GtkItemFactory * ifactory,
				       gpointer data, GtkDestroyNotify destroy,
				       guint x, guint y,
				       guint mouse_button, guint32 time)
{
	static GQuark quark_user_menu_pos = 0;
	struct MenuPos *pos;

	if (!quark_user_menu_pos)
		quark_user_menu_pos =
			g_quark_from_static_string("user_menu_pos");

	if (!quark_popup_data)
		quark_popup_data =
			g_quark_from_static_string("GtkItemFactory-popup-data");

	pos = gtk_object_get_data_by_id(GTK_OBJECT(ifactory),
					quark_user_menu_pos);
	if (!pos)
	{
		pos = g_malloc0(sizeof (struct MenuPos));

		gtk_object_set_data_by_id_full(GTK_OBJECT(ifactory->widget),
					       quark_user_menu_pos, pos, g_free);
	}
	pos->x = x;
	pos->y = y;

	if (data != NULL)
	{
		gtk_object_set_data_by_id_full(GTK_OBJECT (ifactory),
					       quark_popup_data,
					       data, destroy);
		gtk_signal_connect(GTK_OBJECT(ifactory->widget),
				   "selection-done",
				   GTK_SIGNAL_FUNC(util_menu_delete_popup_data),
				   ifactory);
	}

	gtk_menu_popup(GTK_MENU(ifactory->widget), NULL, NULL,
		       (GtkMenuPositionFunc) util_menu_position,
		       pos, mouse_button, time);
}

void util_item_factory_popup(GtkItemFactory * ifactory, guint x, guint y,
			     guint mouse_button, guint32 time)
{
	util_item_factory_popup_with_data(ifactory, NULL, NULL, x, y,
					  mouse_button, time);
}

static gint util_find_compare_func(gconstpointer a, gconstpointer b)
{
	return strcasecmp(a, b);
}

static void util_add_url_callback(GtkWidget *w, GtkWidget *entry)
{
	gchar *text;
	GList *node;

	text = gtk_entry_get_text(GTK_ENTRY(entry));
	if(!g_list_find_custom(cfg.url_history, text, util_find_compare_func))
	{
		cfg.url_history = g_list_prepend(cfg.url_history, g_strdup(text));
		while (g_list_length(cfg.url_history) > 30)
		{
			node = g_list_last(cfg.url_history);
			g_free(node->data);
			cfg.url_history = g_list_remove_link(cfg.url_history, node);
		}
	}
}

GtkWidget* util_create_add_url_window(gchar *caption, GtkSignalFunc ok_func, GtkSignalFunc enqueue_func)
{
	GtkWidget *win, *vbox, *bbox, *ok, *enqueue, *cancel, *combo;
	
	win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(win), caption);
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_MOUSE);
	gtk_window_set_default_size(GTK_WINDOW(win), 400, -1);
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);
	
	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(win), vbox);

	combo = gtk_combo_new();
	if(cfg.url_history)
		gtk_combo_set_popdown_strings(GTK_COMBO(combo), cfg.url_history);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "activate", util_add_url_callback, GTK_COMBO(combo)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "activate", ok_func, GTK_COMBO(combo)->entry);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);
	gtk_window_set_focus(GTK_WINDOW(win), GTK_COMBO(combo)->entry);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "");
	gtk_combo_set_use_arrows_always(GTK_COMBO(combo), TRUE);
	gtk_widget_show(combo);
	
	bbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
	gtk_widget_show(bbox);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	
	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", util_add_url_callback, GTK_COMBO(combo)->entry);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", ok_func, GTK_COMBO(combo)->entry);
	
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(ok);
	gtk_box_pack_start(GTK_BOX(bbox), ok, FALSE, FALSE, 0);
	gtk_widget_show(ok);

	if (enqueue_func)
	{
		/* I18N: "Enqueue" here means "Add to playlist" */
		enqueue = gtk_button_new_with_label(_("Enqueue"));
		gtk_signal_connect(GTK_OBJECT(enqueue), "clicked", util_add_url_callback, GTK_COMBO(combo)->entry);
		gtk_signal_connect(GTK_OBJECT(enqueue), "clicked", enqueue_func, GTK_COMBO(combo)->entry);
		GTK_WIDGET_SET_FLAGS(enqueue, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), enqueue, FALSE, FALSE, 0);
		gtk_widget_show(enqueue);
	}
	
	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, FALSE, FALSE, 0);
	gtk_widget_show(cancel);
	
	gtk_widget_show(vbox);
	return win;
}

static int int_compare_func(gconstpointer a, gconstpointer b)
{
	if (GPOINTER_TO_INT(a) < GPOINTER_TO_INT(b))
		return -1;
	if (GPOINTER_TO_INT(a) > GPOINTER_TO_INT(b))
		return 1;
	else
		return 0;
}

static void filebrowser_changed(GtkFileSelection * filesel)
{
	GList *list, *node;
	char *filename = gtk_file_selection_get_filename(filesel);

	if ((list = input_scan_dir(filename)) != NULL)
	{
		/*
		 * We enter a directory that has been "hijacked" by an
		 * input-plugin. This is used by the CDDA plugin
		 */
		char *current = "./", *parent = "../";

		gtk_clist_clear(GTK_CLIST(filesel->dir_list));
		gtk_clist_append(GTK_CLIST(filesel->dir_list), &current);
		gtk_clist_append(GTK_CLIST(filesel->dir_list), &parent);
		
		gtk_clist_freeze(GTK_CLIST(filesel->file_list));
		gtk_clist_clear(GTK_CLIST(filesel->file_list));
		node = list;
		while (node)
		{
			gtk_clist_append(GTK_CLIST(filesel->file_list),
					 (gchar **) & node->data);
			g_free(node->data);
			node = g_list_next(node);
		}
		gtk_clist_thaw(GTK_CLIST(filesel->file_list));
		g_list_free(list);
	}
}

static int filebrowser_idle_changed(gpointer data)
{
	GDK_THREADS_ENTER();
	filebrowser_changed(GTK_FILE_SELECTION(data));
	GDK_THREADS_LEAVE();

	return FALSE;
}

static void filebrowser_entry_changed(GtkEditable *entry, gpointer data)
{
	char *text = gtk_entry_get_text(GTK_ENTRY(entry));

	if (!text || !(*text))
		filebrowser_changed(GTK_FILE_SELECTION(data));
}

static void filebrowser_dir_select(GtkCList *clist, int row, int col, GdkEventButton *event, gpointer data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		gtk_idle_add(filebrowser_idle_changed, data);
}

gboolean util_filebrowser_is_dir(GtkFileSelection * filesel)
{
	char *text;
	struct stat buf;
	gboolean retv = FALSE;
	
	text = g_strdup(gtk_file_selection_get_filename(filesel));
	if (strlen(text) == 0)
	{
		/*
		 * This is a weird special case.  If the "current" dir
		 * don't exist, gtk_file_selection_get_filename()
		 * return an empty string, even if the user typed in a
		 * dir.
		 */
		g_free(text);
		text = g_strdup(gtk_entry_get_text(GTK_ENTRY(filesel->selection_entry)));
	}

	if ((stat(text, &buf) == 0 && S_ISDIR(buf.st_mode)) || strlen(text) == 0)
	{
		/* Selected directory */
		int len = strlen(text);
		if (len > 3 && !strcmp(text + len - 4, "/../"))
		{
			if (len == 4)
				/* At the root already */
				*(text + len - 3) = '\0';
			else
			{
				char *ptr;
				*(text + len - 4) = '\0';
				ptr = strrchr(text, '/');
				*(ptr + 1) = '\0';
			}
		}
		else if (len > 2 && !strcmp(text + len - 3, "/./"))
			*(text + len - 2) = '\0';
		gtk_file_selection_set_filename(filesel, text);
		retv = TRUE;
	}
	g_free(text);
	return retv;
}

static void filebrowser_add_files(GtkFileSelection * filesel)
{
	GList *sel_list = NULL, *node;
	char *text, *ptr;

	if (cfg.filesel_path)
		g_free(cfg.filesel_path);

	/*
	 * There got to be some clean way to do this too
	 */
	gtk_label_get(GTK_LABEL(GTK_BIN(filesel->history_pulldown)->child), &ptr);
	/* This will give an extra slash if the current dir is the root. */
	cfg.filesel_path = g_strconcat(ptr, "/", NULL);

	node = GTK_CLIST(filesel->file_list)->selection;
	while (node)
	{
		sel_list = g_list_prepend(sel_list, node->data);
		node = g_list_next(node);
	}
	sel_list = g_list_sort(sel_list, int_compare_func);

	node = sel_list;
	
	if (node)
	{
		do {
			char *tmp;
			gtk_clist_get_text(GTK_CLIST(filesel->file_list),
					   GPOINTER_TO_INT(node->data), 0, &text);
			tmp = g_strconcat(cfg.filesel_path, text, NULL);
			playlist_add(tmp);
			g_free(tmp);
		} while ((node = g_list_next(node)) != NULL);
	}
	else
	{
		/*
		 * No files selected, but the user may have
		 * typed a filename.
		 */
		text = gtk_file_selection_get_filename(filesel);
		if (text[strlen(text) - 1] != '/')
			playlist_add(text);
		gtk_file_selection_set_filename(filesel, "");
	}
	g_list_free(sel_list);
	playlistwin_update_list();
}

static void filebrowser_add(GtkWidget * w, GtkWidget * filesel)
{
	if (util_filebrowser_is_dir(GTK_FILE_SELECTION(filesel)))
		return;
	gtk_widget_hide(filesel);
	filebrowser_add_files(GTK_FILE_SELECTION(filesel));
	gtk_widget_destroy(filesel);
}

static void filebrowser_play(GtkWidget * w, GtkWidget * filesel)
{
	if (util_filebrowser_is_dir(GTK_FILE_SELECTION(filesel)))
		return;
	playlist_clear();
	gtk_widget_hide(filesel);
	filebrowser_add_files(GTK_FILE_SELECTION(filesel));
	gtk_widget_destroy(filesel);
	playlist_play();
}

static void filebrowser_add_selected_files(GtkWidget * w, gpointer data)
{
	GtkFileSelection *filesel = GTK_FILE_SELECTION(data);
	
	filebrowser_add_files(filesel);
	gtk_clist_unselect_all(GTK_CLIST(filesel->file_list));

	/*HACK*/
	gtk_entry_set_text(GTK_ENTRY(filesel->selection_entry), "");
}

static void filebrowser_add_all_files(GtkWidget * w, gpointer data)
{
	GtkFileSelection *filesel = GTK_FILE_SELECTION(data);

	gtk_clist_freeze(GTK_CLIST(filesel->file_list));
	gtk_clist_select_all(GTK_CLIST(filesel->file_list));
	filebrowser_add_files(filesel);
	/*
	 * We want gtk_clist_undo_selection() but it seems to be buggy
	 * in GTK+ 1.2.6
	 */
	/*  gtk_clist_undo_selection(GTK_CLIST(GTK_FILE_SELECTION(filesel)->file_list)); */
	gtk_clist_unselect_all(GTK_CLIST(filesel->file_list));
	gtk_clist_thaw(GTK_CLIST(filesel->file_list));

	gtk_entry_set_text(GTK_ENTRY(filesel->selection_entry), "");
}

GtkWidget * util_create_filebrowser(gboolean play_button)
{
	GtkWidget *filebrowser, *bbox, *add_selected, *add_all, *label, *button;
	GtkFileSelection *fb;
	GtkSignalFunc sf;
	char *title;

	if (play_button)
		title = _("Play files");
	else
		title = _("Load files");

	filebrowser = gtk_file_selection_new(title);
	fb = GTK_FILE_SELECTION(filebrowser);
		
	gtk_clist_set_selection_mode(GTK_CLIST(fb->file_list),
				     GTK_SELECTION_EXTENDED);
	gtk_signal_connect(GTK_OBJECT(fb->selection_entry), "changed",
			   filebrowser_entry_changed, filebrowser);
	gtk_signal_connect(GTK_OBJECT(fb->dir_list), "select_row",
			   filebrowser_dir_select, filebrowser);
	if (play_button)
		sf = filebrowser_play;
	else
		sf = filebrowser_add;
	gtk_signal_connect(GTK_OBJECT(fb->ok_button),
			   "clicked", sf, filebrowser);
	gtk_signal_connect_object(GTK_OBJECT(fb->cancel_button), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(filebrowser));

	if (cfg.filesel_path)
		gtk_file_selection_set_filename(fb, cfg.filesel_path);
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 0);
	gtk_box_pack_end(GTK_BOX(fb->action_area), bbox, TRUE, TRUE, 0);
	add_selected  = gtk_button_new_with_label(_("Add selected files"));
	gtk_box_pack_start(GTK_BOX(bbox), add_selected, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(add_selected), "clicked",
			   filebrowser_add_selected_files, filebrowser);
	add_all = gtk_button_new_with_label(_("Add all files in directory"));
	gtk_box_pack_start(GTK_BOX(bbox), add_all, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(add_all), "clicked",
			   filebrowser_add_all_files, filebrowser);
	gtk_widget_show_all(bbox);

	/*
	 * Change the Cancel buttons caption to Close.
	 */
	
	label = gtk_label_new(_("Close"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_container_remove(GTK_CONTAINER(fb->cancel_button),
			     GTK_BIN(fb->cancel_button)->child);
	gtk_container_add(GTK_CONTAINER(fb->cancel_button), label);
	gtk_widget_show(label);

	/*
	 * Change the OK buttons caption to Add or Play.
	 */

	if (play_button)
		label = gtk_label_new(_("Play"));
	else
		label = gtk_label_new(_("Add"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_container_remove(GTK_CONTAINER(fb->ok_button),
			     GTK_BIN(fb->ok_button)->child);
	gtk_container_add(GTK_CONTAINER(fb->ok_button), label);
	gtk_widget_show(label);

	if (play_button)
	{
		GtkArg arg;

		/*
		 * The amount of fiddling we do with the filesel
		 * internals are starting to get ridicolous.  Maybe we
		 * should copy the entire filesel code instead.
		 */
		arg.name = "GtkWidget::parent";
		gtk_object_arg_get(GTK_OBJECT(fb->ok_button), &arg, NULL);
		button = gtk_button_new_with_label(_("Add"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   filebrowser_add, filebrowser);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(arg.d.object_data),
				   button, TRUE, TRUE, 0);
		gtk_box_reorder_child(GTK_BOX(arg.d.object_data), button, 1);
		gtk_widget_show(button);
	}

	gtk_widget_show(filebrowser);
	return filebrowser;
}

GdkFont *util_font_load(char *name)
{
	GdkFont *font;

	/* First try the prefered way, then just try to get some font */

	if (!cfg.use_fontsets)
	{	
		if ((font = gdk_font_load(name)) == NULL)
			font = gdk_fontset_load(name);
	}
	else
	{
		if ((font = gdk_fontset_load(name)) == NULL)
			font = gdk_font_load(name);
	}

	if (!font)
	{
		g_warning("Failed to open font: \"%s\".", name);
		font = gdk_font_load("fixed");
	}
	
	return font;
}

#ifdef ENABLE_NLS
char* util_menu_translate(const char *path, void *func_data)
{
	char *translation = gettext(path);

	if (!translation || *translation != '/')
	{
		g_warning("Bad translation for menupath: %s", path);
		translation = (char*) path;
	}

	return translation;
}
#endif

void util_set_cursor(GtkWidget *window)
{
	static GdkCursor *cursor;

	if (!window)
	{
		if (cursor)
		{
			gdk_cursor_destroy(cursor);
			cursor = NULL;
		}
		return;
	}
	if (!cursor)
		cursor = gdk_cursor_new(GDK_LEFT_PTR);

	gdk_window_set_cursor(window->window, cursor);
}

void util_dump_menu_rc(void)
{
	char *filename = g_strconcat(g_get_home_dir(), "/.xmms/menurc", NULL);
	gtk_item_factory_dump_rc(filename, NULL, FALSE);
	g_free(filename);
}

void util_read_menu_rc(void)
{
	char *filename = g_strconcat(g_get_home_dir(), "/.xmms/menurc", NULL);
	gtk_item_factory_parse_rc(filename);
	g_free(filename);
}

void util_dialog_keypress_cb(GtkWidget *w, GdkEventKey *event, gpointer data)
{
	if (event && event->keyval == GDK_Escape)
		gtk_widget_destroy(w);
}
