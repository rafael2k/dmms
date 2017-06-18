/*  IRman plugin for xmms by Charles Sielski (stray@teklabs.net) ..
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
#include "ir.h"

static GtkWidget *ir_about_win;

void ir_about(void)
{
	GtkWidget *vbox, *frame, *label, *box, *button, *textbox;

	if (ir_about_win)
		return;
	ir_about_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(ir_about_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &ir_about_win);
	gtk_window_set_title(GTK_WINDOW(ir_about_win), _("About"));
	gtk_window_set_policy(GTK_WINDOW(ir_about_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(ir_about_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(ir_about_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(ir_about_win), vbox);

	frame = gtk_frame_new(_("XMMS IRman Plugin:"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	textbox = gtk_vbox_new(FALSE, 10);	
	gtk_container_border_width(GTK_CONTAINER(textbox), 10);
	gtk_container_add(GTK_CONTAINER(frame), textbox);

	label = gtk_label_new(_("Created by Charles Sielski <stray@teklabs.net>\n"
				"Control XMMS with your TV / VCR / Stereo remote \n"
				"IRman page - http://www.evation.com/irman/"));

	gtk_box_pack_start_defaults(GTK_BOX(textbox), label);

	box = gtk_hbutton_box_new();
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(box), 5);
	gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

	button = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(ir_about_win));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);

	gtk_widget_show(button);
	gtk_widget_show(box);
	gtk_widget_show(frame);
	gtk_widget_show(textbox);
	gtk_widget_show(label);
	gtk_widget_show(vbox);
	gtk_widget_show(ir_about_win);
}
