
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
#ifndef _JOY_H_
#define _JOY_H_

#include "config.h"

/* System general includes */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

/* XMMS-required includes (glib, threads) */
#include <glib.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include "xmms/plugin.h"
#include "libxmms/xmmsctrl.h"
#include "libxmms/configfile.h"

typedef enum joy_cmd_en
{
	JC_PLAYPAUSE, JC_STOP, JC_NEXT, JC_PREV, JC_FWD5, JC_BWD5, JC_VOLUP,
	JC_VOLDWN, JC_FWD, JC_RWD, JC_SHUFFLE, JC_REPEAT, JC_ALT, JC_NONE
}
joy_cmd;

typedef struct joy_config_st
{
	int sens;
	gchar *device_1, *device_2;
	int up, down, left, right;
	int alt_up, alt_down, alt_left, alt_right;
	int num_buttons;
	int *button_cmd;
}
joy_config;

extern joy_config joy_cfg;


void joy_about(void);
void joy_configure(void);
void joyapp_read_config(void);
void joyapp_save_config(void);
void joyapp_read_buttoncmd(void);
void joyapp_save_buttoncmd(void);

#endif
