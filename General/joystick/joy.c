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
#include <stdio.h>
#include "xmms/i18n.h"
#include "joy.h"

/* Important stuff to know */
static gboolean keep_going = FALSE;

/* The thread handle */
static pthread_t joyapp_thread;

static int joy_fd1 = -1, joy_fd2 = -1;
static unsigned char joy_axes, joy_buttons;

/* Declarations for calls that we need to mention in the plugin struct */
static void init(void);
static void cleanup(void);
static void *xmms_joyapp_routine(void *arg);


GeneralPlugin joy_gp =
{
	NULL,			/* handle */
	NULL,			/* filename */
	-1,			/* xmms_session */
	NULL,			/* Description */
	init,
	joy_about,
	joy_configure,
	cleanup
};

joy_config joy_cfg =
{
	10000,
	NULL, NULL,
	JC_VOLUP, JC_VOLDWN, JC_PREV, JC_NEXT,
	JC_FWD5, JC_BWD5, JC_RWD, JC_FWD,
	0, NULL
};

/* ---------------------------------------------------------------------- */
GeneralPlugin *get_gplugin_info(void)
{
	joy_gp.description =
		g_strdup_printf(_("Joystick Control %s"), VERSION);
	return &joy_gp;
}

/* ---------------------------------------------------------------------- */
void joyapp_read_config(void)
{
	ConfigFile *cfile;

	joy_cfg.device_1 = g_strdup("/dev/js0");
	joy_cfg.device_2 = g_strdup("/dev/js1");

	cfile = xmms_cfg_open_default_file();

	xmms_cfg_read_string(cfile, "joystick", "device1", &joy_cfg.device_1);
	xmms_cfg_read_string(cfile, "joystick", "device2", &joy_cfg.device_2);
	xmms_cfg_read_int(cfile, "joystick", "sensitivity", &joy_cfg.sens);
	xmms_cfg_read_int(cfile, "joystick", "up", &joy_cfg.up);
	xmms_cfg_read_int(cfile, "joystick", "down", &joy_cfg.down);
	xmms_cfg_read_int(cfile, "joystick", "left", &joy_cfg.left);
	xmms_cfg_read_int(cfile, "joystick", "right", &joy_cfg.right);
	xmms_cfg_read_int(cfile, "joystick", "alt_up", &joy_cfg.alt_up);
	xmms_cfg_read_int(cfile, "joystick", "alt_down", &joy_cfg.alt_down);
	xmms_cfg_read_int(cfile, "joystick", "alt_left", &joy_cfg.alt_left);
	xmms_cfg_read_int(cfile, "joystick", "alt_right", &joy_cfg.alt_right);

	xmms_cfg_free(cfile);
}

/* ---------------------------------------------------------------------- */
void joyapp_read_buttoncmd(void)
{
	ConfigFile *cfile;
	gchar *button;
	int i;

	cfile = xmms_cfg_open_default_file();

	for (i = 0; i < joy_cfg.num_buttons; i++)
	{
		joy_cfg.button_cmd[i] = 13;
		button = g_strdup_printf("button%d", i+1);
		xmms_cfg_read_int (cfile, "joystick", button, &joy_cfg.button_cmd[i]);
		g_free(button);
	}
	xmms_cfg_free (cfile);
}

/* ---------------------------------------------------------------------- */
void joyapp_save_buttoncmd(void)
{
	ConfigFile *cfile;
	gchar *button;
	int i;
    
	cfile = xmms_cfg_open_default_file();
		
	for (i = 0; i < joy_cfg.num_buttons; i++)
	{
		button = g_strdup_printf("button%d", i+1);
		xmms_cfg_write_int (cfile, "joystick", button, joy_cfg.button_cmd[i]);
		g_free(button);
	}
	
	xmms_cfg_write_default_file(cfile);
	xmms_cfg_free(cfile);
}	

/* ---------------------------------------------------------------------- */
void joyapp_save_config(void)
{
	ConfigFile *cfile;

	cfile = xmms_cfg_open_default_file();

	xmms_cfg_write_string(cfile, "joystick", "device1", joy_cfg.device_1);
	xmms_cfg_write_string(cfile, "joystick", "device2", joy_cfg.device_2);
	xmms_cfg_write_int(cfile, "joystick", "sensitivity", joy_cfg.sens);
	xmms_cfg_write_int(cfile, "joystick", "up", joy_cfg.up);
	xmms_cfg_write_int(cfile, "joystick", "down", joy_cfg.down);
	xmms_cfg_write_int(cfile, "joystick", "left", joy_cfg.left);
	xmms_cfg_write_int(cfile, "joystick", "right", joy_cfg.right);
	xmms_cfg_write_int(cfile, "joystick", "alt_up", joy_cfg.alt_up);
	xmms_cfg_write_int(cfile, "joystick", "alt_down", joy_cfg.alt_down);
	xmms_cfg_write_int(cfile, "joystick", "alt_left", joy_cfg.alt_left);
	xmms_cfg_write_int(cfile, "joystick", "alt_right", joy_cfg.alt_right);
	xmms_cfg_write_default_file(cfile);
	xmms_cfg_free(cfile);
	
	joyapp_save_buttoncmd();
}

/* ---------------------------------------------------------------------- */
static void init(void)
{
	joyapp_read_config();

	if ((joy_fd1 = open(joy_cfg.device_1, O_RDONLY)) < 0)
	{
		perror(_("Joystick Control"));
		return;
	}

	joy_fd2 = open(joy_cfg.device_2, O_RDONLY);

	ioctl(joy_fd1, JSIOCGAXES, &joy_axes);
	ioctl(joy_fd1, JSIOCGBUTTONS, &joy_buttons);

	joy_cfg.num_buttons = joy_buttons;
	joy_cfg.button_cmd = g_malloc(joy_buttons * sizeof(int));
    
	joyapp_read_buttoncmd();
    
	keep_going = TRUE;
	pthread_create(&joyapp_thread, NULL, xmms_joyapp_routine, NULL);
}

/* ---------------------------------------------------------------------- */
static void cleanup(void)
{
	if (keep_going)
	{
		keep_going = FALSE;
		pthread_join(joyapp_thread, NULL);
	}
	if (joy_fd1 > 0)
		close(joy_fd1);
	if (joy_fd2 > 0)
		close(joy_fd2);
}

/* ---------------------------------------------------------------------- */
static void *xmms_joyapp_routine(void *arg)
{
	gint vl, vr, output_time, playlist_pos, playlist_time, playlist_length;
	struct js_event js;
	struct timeval tv;
	fd_set set;
	int max_fd, js_alt = 0;
	joy_cmd js_command;

	while (keep_going)
	{
		max_fd = joy_fd1 + 1;
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		FD_ZERO(&set);
		FD_SET(joy_fd1, &set);
		if (joy_fd2 > 0)
		{
			FD_SET(joy_fd2, &set);
			max_fd = joy_fd2 + 1;
		}
		js_command = JC_NONE;

		if (select(max_fd, &set, NULL, NULL, &tv))
		{
			if (FD_ISSET(joy_fd1, &set))
			{
				if (read(joy_fd1, &js, sizeof (struct js_event)) != sizeof (struct js_event))
					perror(_("\nJoystick Control: error reading"));

				switch (js.type & ~JS_EVENT_INIT)
				{
					case JS_EVENT_BUTTON:
						if (js.number <= joy_cfg.num_buttons) {js_command = joy_cfg.button_cmd[js.number];}
						break;
					case JS_EVENT_AXIS:
						if (js.number == 0)
						{
							if (js.value > joy_cfg.sens)
							{
								if (js_alt)
									js_command = joy_cfg.alt_right;
								else
									js_command = joy_cfg.right;
							}
							else if (js.value < -joy_cfg.sens)
							{
								if (js_alt)
									js_command = joy_cfg.alt_left;
								else
									js_command = joy_cfg.left;
							}
						}
						else if (js.number == 1)
						{
							if (js.value > joy_cfg.sens)
							{
								if (js_alt)
									js_command = joy_cfg.alt_down;
								else
									js_command = joy_cfg.down;
							}
							else if (js.value < -joy_cfg.sens)
							{
								if (js_alt)
									js_command = joy_cfg.alt_up;
								else
									js_command = joy_cfg.up;
							}
						}
						break;
				}
			}
			if (joy_fd2 > 0)
			{
				if (FD_ISSET(joy_fd2, &set))
				{
					if (read(joy_fd2, &js, sizeof (struct js_event)) != sizeof (struct js_event))
					        	 perror(_("\nJoystick Control: error reading"));

					switch (js.type & ~JS_EVENT_INIT)
					{
						case JS_EVENT_BUTTON:
							if (js.number == 0)
								js_command = joy_cfg.button_cmd[2];
							else if (js.number == 1)
								js_command = joy_cfg.button_cmd[3];
							break;
						case JS_EVENT_AXIS:
							if (js.number == 0)
							{
								if (js.value > joy_cfg.sens)
									js_command = joy_cfg.alt_right;
								else if (js.value < -joy_cfg.sens)
									js_command = joy_cfg.alt_left;
							}
							else if (js.number == 1)
							{
								if (js.value > joy_cfg.sens)
									js_command = joy_cfg.alt_down;
								else if (js.value < -joy_cfg.sens)
									js_command = joy_cfg.alt_up;
							}
							break;
					}
				}
			}
			if (js_command != JC_ALT && js.value == 0)
				js_command = JC_NONE;

			switch (js_command)
			{
				case JC_PLAYPAUSE:
					if (xmms_remote_is_playing(joy_gp.xmms_session))
						xmms_remote_pause(joy_gp.xmms_session);
					else
						xmms_remote_play(joy_gp.xmms_session);
					break;
				case JC_STOP:
					xmms_remote_stop(joy_gp.xmms_session);
					break;
				case JC_NEXT:
					xmms_remote_playlist_next(joy_gp.xmms_session);
					break;
				case JC_PREV:
					xmms_remote_playlist_prev(joy_gp.xmms_session);
					break;
				case JC_FWD5:
					playlist_pos = xmms_remote_get_playlist_pos(joy_gp.xmms_session);
					playlist_length = xmms_remote_get_playlist_length(joy_gp.xmms_session);
					if (playlist_length - playlist_pos < 5)
						playlist_pos = playlist_length - 5;
					xmms_remote_set_playlist_pos(joy_gp.xmms_session, playlist_pos + 5);
					break;
				case JC_BWD5:
					playlist_pos = xmms_remote_get_playlist_pos(joy_gp.xmms_session);
					if (playlist_pos < 5)
						playlist_pos = 5;
					xmms_remote_set_playlist_pos(joy_gp.xmms_session, playlist_pos - 5);
					break;
				case JC_VOLUP:
					xmms_remote_get_volume(joy_gp.xmms_session, &vl, &vr);
					if (vl > 95)
						vl = 95;
					if (vr > 95)
						vr = 95;
					xmms_remote_set_volume(joy_gp.xmms_session, vl + 5, vr + 5);
					break;
				case JC_VOLDWN:
					xmms_remote_get_volume(joy_gp.xmms_session, &vl, &vr);
					if (vl < 5)
						vl = 5;
					if (vr < 5)
						vr = 5;
					xmms_remote_set_volume(joy_gp.xmms_session, vl - 5, vr - 5);
					break;
				case JC_FWD:
					output_time = xmms_remote_get_output_time(joy_gp.xmms_session);
					playlist_pos = xmms_remote_get_playlist_pos(joy_gp.xmms_session);
					playlist_time = xmms_remote_get_playlist_time(joy_gp.xmms_session, playlist_pos);
					if (playlist_time - output_time < 5000)
						output_time = playlist_time - 5000;
					xmms_remote_jump_to_time(joy_gp.xmms_session, output_time + 5000);
					break;
				case JC_RWD:
					output_time = xmms_remote_get_output_time(joy_gp.xmms_session);
					if (output_time < 5000)
						output_time = 5000;
					xmms_remote_jump_to_time(joy_gp.xmms_session, output_time - 5000);
					break;
				case JC_SHUFFLE:
					xmms_remote_toggle_shuffle(joy_gp.xmms_session);
					break;
				case JC_REPEAT:
					xmms_remote_toggle_repeat(joy_gp.xmms_session);
					break;
				case JC_ALT:
					js_alt = js.value;
					break;
			        case JC_NONE:
				        break;
			}
		}
	}

	pthread_exit(NULL);
}


