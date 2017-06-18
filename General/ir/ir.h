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

#ifndef _IR_H_
#define _IR_H_
#include "config.h"

/* System general includes */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <errno.h>

/* XMMS-required includes (glib, threads) */
#include <glib.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include "xmms/plugin.h"
#include "../../libxmms/xmmsctrl.h"
#include "../../libxmms/configfile.h"

#include "irman.h"

typedef struct
{
	gchar *device;
	gint codelen;
	gchar *button_play, *button_stop, *button_next, *button_prev;
	gchar *button_pause, *button_seekf, *button_seekb;
	gchar *button_volup, *button_voldown, *button_plus100;
	gchar *button_shuffle, *button_repeat, *button_playlist;
	gchar *button[10], *playlist[100];
}
irConfig;

extern irConfig ircfg;
extern gboolean irconf_is_going;


void ir_about(void);
void ir_configure(void);

void irapp_read_config(void);
void irapp_save_config(void);
void irapp_init_port(gchar * ir_port);

#endif /* _IR_H_ */



