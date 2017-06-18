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
#include "libxmms/util.h"
#include "joy.h"

void joy_about(void)
{
	static GtkWidget *aboutbox;

	if (aboutbox != NULL)
		return;
	
	aboutbox = xmms_show_message(
		_("About Joystick Driver"),
		_("Joystick Control Plugin\n\n"
		  "Created by Tim Ferguson <timf@dgs.monash.edu.au>.\n"
		  "http://www.dgs.monash.edu.au/~timf/\n\n"
		  "5+ button support by Justin Wake "
		  "<justin@globalsoft.com.au>\n\n"
		  "Control XMMS with one or two joysticks.\n"),
		_("OK"), FALSE, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &aboutbox);
}
