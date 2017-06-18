/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
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

GtkWidget *skinwin, *skinwin_list, *skinwin_close;
GList *skinlist = NULL;

gint skinwin_delete_event(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_widget_hide(skinwin);
	return (TRUE);
}

void change_skin_event(GtkWidget * widget, gint row, gint column, GdkEventButton * event)
{
	if (row == 0)
		load_skin(NULL);
	else
		load_skin(((struct SkinNode *) g_list_nth(skinlist, row - 1)->data)->path);

}

static void enable_random_skin_event(GtkWidget * widget, gpointer data)
{
	cfg.random_skin_on_play = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

void create_skin_window(void)
{
	char *titles[1];
	GtkWidget *vbox, *hbox, *main_hbox, *separator, *scrolled_win, *checkbox;

	skinwin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(skinwin), _("Skin selector"));
	gtk_window_set_transient_for(GTK_WINDOW(skinwin), GTK_WINDOW(mainwin));
	gtk_signal_connect(GTK_OBJECT(skinwin), "delete_event", GTK_SIGNAL_FUNC(skinwin_delete_event), NULL);
	gtk_container_border_width(GTK_CONTAINER(skinwin), 10);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(skinwin), vbox);

	titles[0] = _("Skins");
	skinwin_list = gtk_clist_new_with_titles(1, titles);
	gtk_clist_column_titles_passive(GTK_CLIST(skinwin_list));
	gtk_clist_set_selection_mode(GTK_CLIST(skinwin_list), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(skinwin_list), "select_row", GTK_SIGNAL_FUNC(change_skin_event), NULL);
	gtk_widget_set_usize(skinwin_list, 250, 200);
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), skinwin_list);
	gtk_container_border_width(GTK_CONTAINER(scrolled_win), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, TRUE, 0);

	main_hbox = gtk_hbox_new(FALSE,5);
	gtk_box_set_spacing(GTK_BOX(main_hbox),5);
	gtk_box_pack_start(GTK_BOX(vbox), main_hbox, FALSE, FALSE, 0);

	checkbox = gtk_check_button_new_with_label(_("Select random skin on play"));
	gtk_box_pack_start(GTK_BOX(main_hbox), checkbox, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), cfg.random_skin_on_play);
	gtk_signal_connect(GTK_OBJECT(checkbox), "toggled", GTK_SIGNAL_FUNC(enable_random_skin_event), NULL);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 5);
	gtk_box_pack_start(GTK_BOX(main_hbox), hbox, TRUE, TRUE, 0);
	skinwin_close = gtk_button_new_with_label(_("Close"));
	GTK_WIDGET_SET_FLAGS(skinwin_close, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(skinwin_close), "clicked", GTK_SIGNAL_FUNC(skinwin_delete_event), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), skinwin_close, FALSE, FALSE, 0);
	gtk_widget_grab_default(skinwin_close);
}

static void add_skin(gchar * skin)
{
	struct SkinNode *node = (struct SkinNode *) g_malloc(sizeof (struct SkinNode));
	gchar *tmp;

	node->path = skin;

	tmp = strrchr(skin, '/');
	if (!tmp)
		tmp = skin;
	node->name = (char *) g_malloc(strlen(tmp + 1) + 1);
	strcpy(node->name, tmp + 1);
	tmp = strrchr(node->name, '.');
	if (tmp)
	{
		if (!strcasecmp(tmp, ".zip"))
			*tmp = '\0';
		else if(!strcasecmp(tmp, ".wsz"))
			*tmp = '\0';
		else if (!strcasecmp(tmp, ".tgz"))
			*tmp = '\0';
		else if (!strcasecmp(tmp, ".gz"))
			*tmp = '\0';
		else if (!strcasecmp(tmp, ".bz2"))
			*tmp = '\0';
		else if (!strcasecmp(tmp, ".tar"))
			*tmp = '\0';
		tmp = strrchr(node->name, '.');
		if (tmp)
		{		/* maybe we still have to remove a leftover ".tar" */
			if (!strcasecmp(tmp, ".tar"))
				*tmp = '\0';
		}
	}
	skinlist = g_list_prepend(skinlist, node);
}

static void scan_skindir(char *path)
{
	DIR *dir;
	struct dirent *dirent;
	struct stat statbuf;
	char *file, *tmp;

	if ((dir = opendir(path)) != NULL)
	{
		while ((dirent = readdir(dir)) != NULL)
		{
			if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
			{
				file = g_strdup_printf("%s/%s", path, dirent->d_name);
				if (!stat(file, &statbuf))
				{
					if (S_ISDIR(statbuf.st_mode))
						add_skin(file);
					else if (S_ISREG(statbuf.st_mode))
					{
						tmp = strrchr(file, '.');
						if (tmp)
						{
							if (!strcasecmp(tmp, ".zip") ||
							    !strcasecmp(tmp, ".wsz") ||
							    !strcasecmp(tmp, ".tgz") ||
							    !strcasecmp(tmp, ".gz") ||
							    !strcasecmp(tmp, ".bz2"))
								add_skin(file);
							else
								g_free(file);
						}
						else
							g_free(file);
					}
					else
						g_free(file);
				}
				else
					g_free(file);
			}
		}
		closedir(dir);
	}
}

static gint skinlist_compare_func(gconstpointer a, gconstpointer b)
{
	return strcasecmp(((struct SkinNode *) a)->name, ((struct SkinNode *) b)->name);
}

static void skin_free_func(gpointer data, gpointer user_data)
{
	g_free(((struct SkinNode *) data)->name);
	g_free(((struct SkinNode *) data)->path);
	g_free(data);
}

void scan_skins(void)
{
	int i;
	GList *entry;
	char *none, *str, *skinsdir;
	gchar **list;

	none = _("(none)");
	if (skinlist)
	{
		g_list_foreach(skinlist, skin_free_func, NULL);
		g_list_free(skinlist);
	}
	skinlist = NULL;
	str = g_strconcat(g_get_home_dir(), "/.xmms/Skins", NULL);
	scan_skindir(str);
	g_free(str);
	str = g_strconcat(DATA_DIR, "/Skins", NULL);
	scan_skindir(str);
	g_free(str);
	skinlist = g_list_sort(skinlist, skinlist_compare_func);
	skinsdir = getenv("SKINSDIR");
	if (skinsdir)
	{
		list = g_strsplit(skinsdir, ":", 0);
		i = 0;
		while (list[i])
			scan_skindir(list[i++]);
	}

	gtk_clist_freeze(GTK_CLIST(skinwin_list));
	gtk_clist_clear(GTK_CLIST(skinwin_list));
	gtk_clist_append(GTK_CLIST(skinwin_list), &none);
	if (!skin->path)
		gtk_clist_select_row(GTK_CLIST(skinwin_list), 0, 0);

	for (i = 0; i < g_list_length(skinlist); i++)
	{
		entry = g_list_nth(skinlist, i);
		gtk_clist_append(GTK_CLIST(skinwin_list), (gchar **) & ((struct SkinNode *) entry->data)->name);
		if (skin->path)
			if (!strcmp(((struct SkinNode *) entry->data)->path, skin->path))
				gtk_clist_select_row(GTK_CLIST(skinwin_list), i + 1, 0);
	}
	gtk_clist_thaw(GTK_CLIST(skinwin_list));
}

void show_skin_window(void)
{
	scan_skins();
	gtk_window_set_position(GTK_WINDOW(skinwin), GTK_WIN_POS_MOUSE);
	gtk_widget_show_all(skinwin);
	gtk_widget_grab_focus(skinwin_list);
	if (GTK_CLIST(skinwin_list)->selection)
	{
		gtk_clist_moveto(GTK_CLIST(skinwin_list), GPOINTER_TO_INT(GTK_CLIST(skinwin_list)->selection->data), 0, 0.5, 0.0);
		GTK_CLIST(skinwin_list)->focus_row = GPOINTER_TO_INT(GTK_CLIST(skinwin_list)->selection->data);
	}
}
