
/*  Joystick plugin for xmms by Tim Ferguson (timf@dgs.monash.edu.au
 *                                  http://www.dgs.monash.edu.au/~timf/) ...
 *  14/12/2000 - patched to allow 5 or more buttons to be used (Justin Wake <justin@globalsoft.com.au>) 
 *  XMMS is Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
 */
#include "xmms/i18n.h"
#include "joy.h"

static GtkWidget *joyconf_mainwin;
static GtkWidget *dev_entry1, *dev_entry2, *sens_entry, **joy_menus;

#define num_menu_txt		14
static char *menu_txt[] =
{
	N_("Play/Pause"), N_("Stop"), N_("Next Track"), N_("Prev Track"),
	N_("Fwd 5 tracks"), N_("Back 5 tracks"), N_("Volume Up"),
	N_("Volume Down"), N_("Forward 5s"), N_("Rewind 5s"), N_("Shuffle"),
	N_("Repeat"), N_("Alternate"), N_("Nothing")
};

/* ---------------------------------------------------------------------- */
static void set_joy_config(void)
{
	int i;
	joy_cfg.sens = atoi(gtk_entry_get_text(GTK_ENTRY(sens_entry)));
	joy_cfg.up = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[0]))), "cmd");
	joy_cfg.down = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[1]))), "cmd");
	joy_cfg.left = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[2]))), "cmd");
	joy_cfg.right = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[3]))), "cmd");
	joy_cfg.alt_up = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[4]))), "cmd");
	joy_cfg.alt_down = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[5]))), "cmd");
	joy_cfg.alt_left = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[6]))), "cmd");
	joy_cfg.alt_right = (gint) gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[7]))), "cmd");

	for (i = 0; i < joy_cfg.num_buttons; i++) {
		joy_cfg.button_cmd[i] = (gint)gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(joy_menus[8+i]))), "cmd");
	}
}

/* ---------------------------------------------------------------------- */
static void joyconf_ok_cb(GtkWidget * w, gpointer data)
{
	set_joy_config();
	joy_cfg.device_1 = g_strdup(gtk_entry_get_text(GTK_ENTRY(dev_entry1)));
	joy_cfg.device_2 = g_strdup(gtk_entry_get_text(GTK_ENTRY(dev_entry2)));
	joyapp_save_config();
	g_free(joy_menus);
	gtk_widget_destroy(joyconf_mainwin);
}

/* ---------------------------------------------------------------------- */
static void joyconf_cancel_cb(GtkWidget * w, gpointer data)
{
	joyapp_read_config();
	joyapp_read_buttoncmd();
	g_free(joy_menus);
	gtk_widget_destroy(joyconf_mainwin);
}

/* ---------------------------------------------------------------------- */
static void joyconf_apply_cb(GtkWidget * w, gpointer data)
{
	set_joy_config();
}

/* ---------------------------------------------------------------------- */
void joy_configure(void)
{
	static gint pack_pos[4] =
	{GTK_SIDE_TOP, GTK_SIDE_BOTTOM, GTK_SIDE_LEFT, GTK_SIDE_RIGHT},
	     hist_val[4];
	GtkWidget *vbox, *vbox2, *hbox, *box, *box2, *frame, *table, *label, *button, *item;
	GtkWidget *dir_pack, *blist;
	int i, j;
	char buf[20];

	joyapp_read_config();

	if (!joyconf_mainwin)
	{
		joyconf_mainwin = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_signal_connect(GTK_OBJECT(joyconf_mainwin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &joyconf_mainwin);
		gtk_window_set_title(GTK_WINDOW(joyconf_mainwin), _("XMMS Joystick Configuration"));
		gtk_window_set_policy(GTK_WINDOW(joyconf_mainwin), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(joyconf_mainwin), GTK_WIN_POS_MOUSE);
		gtk_container_border_width(GTK_CONTAINER(joyconf_mainwin), 10);

		/* -------------------------------------------------- */
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(joyconf_mainwin), hbox);
		
		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(hbox), vbox);

        vbox2 = gtk_vbox_new(FALSE, 10);
        gtk_container_add(GTK_CONTAINER(hbox), vbox2);
                
		box = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(box), 5);
		gtk_box_pack_start(GTK_BOX(vbox), box, TRUE, TRUE, 0);

        box2 = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(box2), 5);
        gtk_box_pack_start(GTK_BOX(vbox2), box2, TRUE, TRUE, 0);
        
		/* -------------------------------------------------- */
		frame = gtk_frame_new(_("Devices:"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);

		table = gtk_table_new(3, 2, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 5);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);

		/* ------------------------------ */
		label = gtk_label_new(_("Joystick 1:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
		gtk_widget_show(label);

		dev_entry1 = gtk_entry_new();
		gtk_widget_set_usize(dev_entry1, 100, -1);
		gtk_entry_set_text(GTK_ENTRY(dev_entry1), joy_cfg.device_1);
		gtk_table_attach_defaults(GTK_TABLE(table), dev_entry1, 1, 2, 0, 1);
		gtk_widget_show(dev_entry1);

		/* ------------------------------ */
		label = gtk_label_new(_("Joystick 2:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
		gtk_widget_show(label);

		dev_entry2 = gtk_entry_new();
		gtk_widget_set_usize(dev_entry2, 100, -1);
		gtk_entry_set_text(GTK_ENTRY(dev_entry2), joy_cfg.device_2);
		gtk_table_attach_defaults(GTK_TABLE(table), dev_entry2, 1, 2, 1, 2);
		gtk_widget_show(dev_entry2);

		/* ------------------------------ */
		label = gtk_label_new(_("Sensitivity (10-32767):"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
		gtk_widget_show(label);

		sens_entry = gtk_entry_new();
		gtk_widget_set_usize(sens_entry, 100, -1);
		sprintf(buf, "%d", joy_cfg.sens);
		gtk_entry_set_text(GTK_ENTRY(sens_entry), buf);
		gtk_table_attach_defaults(GTK_TABLE(table), sens_entry, 1, 2, 2, 3);
		gtk_widget_show(sens_entry);

		/* ------------------------------ */
		gtk_widget_show(table);
		gtk_widget_show(frame);

		/* -------------------------------------------------- */
		joy_menus = g_malloc(sizeof(GtkWidget *) * (8 + joy_cfg.num_buttons));
		for (j = 0; j < (8 + joy_cfg.num_buttons); j++)
		{
			joy_menus[j] = gtk_menu_new();
			for (i = 0; i < num_menu_txt; i++)
			{
				item = gtk_menu_item_new_with_label(_(menu_txt[i]));
				gtk_object_set_data(GTK_OBJECT(item), "cmd", (gpointer) i);
				gtk_widget_show(item);
				gtk_menu_append(GTK_MENU(joy_menus[j]), item);
			}
		}

		/* -------------------------------------------------- */
		frame = gtk_frame_new(_("Directionals:"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		dir_pack = gtk_packer_new();
		gtk_container_set_border_width(GTK_CONTAINER(dir_pack), 5);
		gtk_container_add(GTK_CONTAINER(frame), dir_pack);
		hist_val[0] = joy_cfg.up;
		hist_val[1] = joy_cfg.down;
		hist_val[2] = joy_cfg.left;
		hist_val[3] = joy_cfg.right;

		for (i = 0; i < 4; i++)
		{
			blist = gtk_option_menu_new();
			gtk_widget_set_usize(blist, 120, -1);
			gtk_packer_add(GTK_PACKER(dir_pack), blist, pack_pos[i], GTK_ANCHOR_CENTER, 0, 0, 5, 5, 0, 0);
			gtk_option_menu_remove_menu(GTK_OPTION_MENU(blist));
			gtk_option_menu_set_menu(GTK_OPTION_MENU(blist), joy_menus[i]);
			gtk_option_menu_set_history(GTK_OPTION_MENU(blist), hist_val[i]);
			gtk_widget_show(blist);
		}

		gtk_widget_show(dir_pack);
		gtk_widget_show(frame);

		/* -------------------------------------------------- */
		frame = gtk_frame_new(_("Directionals (alternate):"));
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		dir_pack = gtk_packer_new();
		gtk_container_set_border_width(GTK_CONTAINER(dir_pack), 5);
		gtk_container_add(GTK_CONTAINER(frame), dir_pack);
		hist_val[0] = joy_cfg.alt_up;
		hist_val[1] = joy_cfg.alt_down;
		hist_val[2] = joy_cfg.alt_left;
		hist_val[3] = joy_cfg.alt_right;

		for (i = 0; i < 4; i++)
		{
			blist = gtk_option_menu_new();
			gtk_widget_set_usize(blist, 120, -1);
			gtk_packer_add(GTK_PACKER(dir_pack), blist, pack_pos[i], GTK_ANCHOR_CENTER, 0, 0, 5, 5, 0, 0);
			gtk_option_menu_remove_menu(GTK_OPTION_MENU(blist));
			gtk_option_menu_set_menu(GTK_OPTION_MENU(blist), joy_menus[4 + i]);
			gtk_option_menu_set_history(GTK_OPTION_MENU(blist), hist_val[i]);
			gtk_widget_show(blist);
		}

		gtk_widget_show(dir_pack);
		gtk_widget_show(frame);

		/* -------------------------------------------------- */
		frame = gtk_frame_new(_("Buttons:"));
		gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);

		table = gtk_table_new(joy_cfg.num_buttons, 2, TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 5);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);

		for (i = 0; i < joy_cfg.num_buttons; i++)
		{
			char *tmpstr = g_strdup_printf(_("Button %d:"), i + 1);
			label = gtk_label_new(tmpstr);
			g_free(tmpstr);
			gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
			gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, i, i + 1);
			gtk_widget_show(label);

			blist = gtk_option_menu_new();
			gtk_widget_set_usize(blist, 120, -1);
			gtk_table_attach_defaults(GTK_TABLE(table), blist, 1, 2, i, i + 1);
			gtk_option_menu_remove_menu(GTK_OPTION_MENU(blist));
			gtk_option_menu_set_menu(GTK_OPTION_MENU(blist), joy_menus[8 + i]);
			gtk_option_menu_set_history(GTK_OPTION_MENU(blist), joy_cfg.button_cmd[i]);
			gtk_widget_show(blist);
		}

		gtk_widget_show(table);
		gtk_widget_show(frame);

		/* -------------------------------------------------- */
		gtk_widget_show(box);

		/* -------------------------------------------------- */
		box = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(box), 5);
		gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

		button = gtk_button_new_with_label(_("OK"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(joyconf_ok_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(joyconf_cancel_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("Apply"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(joyconf_apply_cb), NULL);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_widget_show(button);

		gtk_widget_show(box);
		gtk_widget_show(box2);
		/* -------------------------------------------------- */
		gtk_widget_show(vbox);
		gtk_widget_show(vbox2);
		gtk_widget_show(hbox);
		gtk_widget_show(joyconf_mainwin);
	}
}
