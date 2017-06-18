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

/* Important stuff to know */
static gboolean keepGoing = FALSE;

/* The thread handle */
static pthread_t irapp_thread;

/* Declarations for calls that we need to mention in the plugin struct */
static void init(void);
static void cleanup(void);
void irapp_read_config(void);
void irapp_save_config(void);
void irapp_init_port(gchar * ir_port);

/* Config struct for ir device and buttons */
irConfig ircfg;

static void *xmms_irapp_routine(void *arg);

GeneralPlugin ir_gp =
{
	NULL,			/* handle */
	NULL,			/* filename */
	-1,			/* xmms_session */
	NULL,			/* Description */
	init,
	ir_about,
	ir_configure,
	cleanup
};

GeneralPlugin *get_gplugin_info(void)
{
	ir_gp.description = g_strdup_printf(_("IRman Control %s"), VERSION);
	return &ir_gp;
}

/* Call irapp's thread */
static void init(void)
{
	irapp_read_config();
	irapp_init_port(ircfg.device);
	keepGoing = TRUE;
	pthread_create(&irapp_thread, NULL, xmms_irapp_routine, NULL);
}

/* Tell irapp its lifetime is up */
static void cleanup(void)
{
	keepGoing = FALSE;
	pthread_join(irapp_thread, NULL);
}

/* Read xmms config file and load irman specific information */
void irapp_read_config(void)
{
	ConfigFile *cfgfile;
	gchar *filename, buttontext[20];
	gint i;

	ircfg.device = g_strdup("/dev/ttyS1");
	ircfg.codelen = 6;
	for (i = 0; i < 10; i++)
	{
		ircfg.button[i] = g_strdup("");
		ircfg.playlist[i] = g_strdup("");
	}
	for (i = 10; i < 100; i++)
		ircfg.playlist[i] = g_strdup("");

	ircfg.button_play = g_strdup("");
	ircfg.button_stop = g_strdup("");
	ircfg.button_next = g_strdup("");
	ircfg.button_prev = g_strdup("");
	ircfg.button_pause = g_strdup("");
	ircfg.button_seekf = g_strdup("");
	ircfg.button_seekb = g_strdup("");
	ircfg.button_volup = g_strdup("");
	ircfg.button_voldown = g_strdup("");
	ircfg.button_shuffle = g_strdup("");
	ircfg.button_repeat = g_strdup("");
	ircfg.button_playlist = g_strdup("");
	ircfg.button_plus100 = g_strdup("");

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfgfile = xmms_cfg_open_file(filename);
	if (cfgfile)
	{
		xmms_cfg_read_string(cfgfile, "irman", "device", &ircfg.device);
		xmms_cfg_read_int(cfgfile, "irman", "codelen", &ircfg.codelen);
		for (i = 0; i < 10; i++)
		{
			sprintf(buttontext, "button%d", i);
			xmms_cfg_read_string(cfgfile, "irman", buttontext, &ircfg.button[i]);
			sprintf(buttontext, "playlist%d", i);
			xmms_cfg_read_string(cfgfile, "irman", buttontext, &ircfg.playlist[i]);
		}
		for (i = 10; i < 100; i++)
		{
			sprintf(buttontext, "playlist%d", i);
			xmms_cfg_read_string(cfgfile, "irman", buttontext, &ircfg.playlist[i]);
		}
		xmms_cfg_read_string(cfgfile, "irman", "button_play", &ircfg.button_play);
		xmms_cfg_read_string(cfgfile, "irman", "button_stop", &ircfg.button_stop);
		xmms_cfg_read_string(cfgfile, "irman", "button_next", &ircfg.button_next);
		xmms_cfg_read_string(cfgfile, "irman", "button_prev", &ircfg.button_prev);
		xmms_cfg_read_string(cfgfile, "irman", "button_pause", &ircfg.button_pause);
		xmms_cfg_read_string(cfgfile, "irman", "button_seekf", &ircfg.button_seekf);
		xmms_cfg_read_string(cfgfile, "irman", "button_seekb", &ircfg.button_seekb);
		xmms_cfg_read_string(cfgfile, "irman", "button_volup", &ircfg.button_volup);
		xmms_cfg_read_string(cfgfile, "irman", "button_voldown", &ircfg.button_voldown);
		xmms_cfg_read_string(cfgfile, "irman", "button_shuffle", &ircfg.button_shuffle);
		xmms_cfg_read_string(cfgfile, "irman", "button_repeat", &ircfg.button_repeat);
		xmms_cfg_read_string(cfgfile, "irman", "button_playlist", &ircfg.button_playlist);
		xmms_cfg_read_string(cfgfile, "irman", "button_plus100", &ircfg.button_plus100);
		xmms_cfg_free(cfgfile);
	}
	g_free(filename);
}

/* Save current settings to the xmms config file */
void irapp_save_config(void)
{
	ConfigFile *cfgfile;
	gchar *filename, buttontext[20];
	gint i;

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfgfile = xmms_cfg_open_file(filename);
	if (!cfgfile)
		cfgfile = xmms_cfg_new();
	xmms_cfg_write_string(cfgfile, "irman", "device", ircfg.device);
	xmms_cfg_write_int(cfgfile, "irman", "codelen", ircfg.codelen);
	for (i = 0; i < 10; i++)
	{
		sprintf(buttontext, "button%d", i);
		xmms_cfg_write_string(cfgfile, "irman", buttontext, ircfg.button[i]);
		sprintf(buttontext, "playlist%d", i);
		xmms_cfg_write_string(cfgfile, "irman", buttontext, ircfg.playlist[i]);
	}
	for (i = 10; i < 100; i++)
	{
		sprintf(buttontext, "playlist%d", i);
		xmms_cfg_write_string(cfgfile, "irman", buttontext, ircfg.playlist[i]);
	}
	xmms_cfg_write_string(cfgfile, "irman", "button_play", ircfg.button_play);
	xmms_cfg_write_string(cfgfile, "irman", "button_stop", ircfg.button_stop);
	xmms_cfg_write_string(cfgfile, "irman", "button_next", ircfg.button_next);
	xmms_cfg_write_string(cfgfile, "irman", "button_prev", ircfg.button_prev);
	xmms_cfg_write_string(cfgfile, "irman", "button_pause", ircfg.button_pause);
	xmms_cfg_write_string(cfgfile, "irman", "button_seekf", ircfg.button_seekf);
	xmms_cfg_write_string(cfgfile, "irman", "button_seekb", ircfg.button_seekb);
	xmms_cfg_write_string(cfgfile, "irman", "button_volup", ircfg.button_volup);
	xmms_cfg_write_string(cfgfile, "irman", "button_voldown", ircfg.button_voldown);
	xmms_cfg_write_string(cfgfile, "irman", "button_shuffle", ircfg.button_shuffle);
	xmms_cfg_write_string(cfgfile, "irman", "button_repeat", ircfg.button_repeat);
	xmms_cfg_write_string(cfgfile, "irman", "button_playlist", ircfg.button_playlist);
	xmms_cfg_write_string(cfgfile, "irman", "button_plus100", ircfg.button_plus100);

	xmms_cfg_write_file(cfgfile, filename);
	xmms_cfg_free(cfgfile);
	g_free(filename);
}

/* A modified port initialization - seems to work everytime */
void irapp_init_port(gchar * ir_port)
{
	gint i;

	for (i = 0; i < 2; i++)
	{
		if (ir_open_port(ir_port) < 0)
			fprintf(stderr, _("unable to open port `%s' (%s)\n"), ir_port, strerror(errno));
		else
		{
			ir_write_char('I');
			ir_usleep(IR_HANDSHAKE_GAP);
			ir_write_char('R');
			ir_set_enabled(1);
			ir_clear_buffer(); /* Take the 'OK' */
		}
	}
}

/* Our main thread */
static void *xmms_irapp_routine(void *arg)
{
	unsigned char *code;
	char *text;
	gint playlist_time, playlist_pos, output_time, vl, vr, i;
	gint ir_numpress = -1;
	gint ir_hundreds = 0;
	gboolean ir_playlist_mode = FALSE;
	GList *ir_playlist;
	GTimer *timer1, *timer2;

#define S_PAUSE	0.2
#define L_PAUSE 0.4
#define PL_PAUSE 2.0

	timer1 = g_timer_new();
	timer2 = g_timer_new();
	g_timer_start(timer1);

	while (keepGoing)
	{
		if (!irconf_is_going)
		{
			code = ir_poll_code();
			if (code)
			{
				text = ir_code_to_text(code);
				if (!strcmp(text, ircfg.button_play) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_play(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}

				else if (!strcmp(text, ircfg.button_stop) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_stop(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_pause) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_pause(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_shuffle) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_toggle_shuffle(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_repeat) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_toggle_repeat(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_playlist) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					ir_playlist_mode = !ir_playlist_mode;
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_next) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_playlist_next(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_prev) && g_timer_elapsed(timer1, NULL) > L_PAUSE)
				{
					xmms_remote_playlist_prev(ir_gp.xmms_session);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_seekf) && g_timer_elapsed(timer1, NULL) > S_PAUSE / 2)
				{
					output_time = xmms_remote_get_output_time(ir_gp.xmms_session);
					playlist_pos = xmms_remote_get_playlist_pos(ir_gp.xmms_session);
					playlist_time = xmms_remote_get_playlist_time(ir_gp.xmms_session, playlist_pos);
					if (playlist_time - output_time < 5000)
						output_time = playlist_time - 5000;
					xmms_remote_jump_to_time(ir_gp.xmms_session, output_time + 5000);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_seekb) && g_timer_elapsed(timer1, NULL) > S_PAUSE / 2)
				{
					output_time = xmms_remote_get_output_time(ir_gp.xmms_session);
					if (output_time < 5000)
						output_time = 5000;
					xmms_remote_jump_to_time(ir_gp.xmms_session, output_time - 5000);
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_stop(timer2);
					ir_hundreds = 0;
				}
				else if (!strcmp(text, ircfg.button_volup) && g_timer_elapsed(timer1, NULL) > S_PAUSE)
				{
					xmms_remote_get_volume(ir_gp.xmms_session, &vl, &vr);
					if (vl > 95)
						vl = 95;
					if (vr > 95)
						vr = 95;
					xmms_remote_set_volume(ir_gp.xmms_session, vl + 5, vr + 5);
					g_timer_reset(timer1);
				}
				else if (!strcmp(text, ircfg.button_voldown) && g_timer_elapsed(timer1, NULL) > S_PAUSE)
				{
					xmms_remote_get_volume(ir_gp.xmms_session, &vl, &vr);
					if (vl < 5)
						vl = 5;
					if (vr < 5)
						vr = 5;
					xmms_remote_set_volume(ir_gp.xmms_session, vl - 5, vr - 5);
					g_timer_reset(timer1);
				}
				else if (!strcmp(text, ircfg.button_plus100) && g_timer_elapsed(timer1, NULL) > S_PAUSE)
				{
					ir_hundreds += 1;
					g_timer_reset(timer1);
					g_timer_reset(timer2);
					g_timer_start(timer2);
				}
				else
				{
					for (i = 0; i < 10; i++)
					{
						if (!strcmp(text, ircfg.button[i]) && g_timer_elapsed(timer1, NULL) > S_PAUSE)
						{
							g_timer_reset(timer2);
							if (ir_numpress >= 0)
							{
								g_timer_stop(timer2);
								if (ir_playlist_mode)
								{
									if (strcmp(ircfg.playlist[(10 * ir_numpress) + i], ""))
									{
										ir_playlist = NULL;
										ir_playlist = g_list_append(ir_playlist, ircfg.playlist[(10 * ir_numpress) + i]);
										xmms_remote_play_files(ir_gp.xmms_session, ir_playlist);
										g_list_free(ir_playlist);
									}
									ir_playlist_mode = FALSE;
								}
								else
								{
									ir_numpress = (100 * ir_hundreds) + (10 * ir_numpress) + i;
									if (ir_numpress == 0)
										xmms_remote_set_playlist_pos(ir_gp.xmms_session,
													     xmms_remote_get_playlist_length(ir_gp.xmms_session) - 1);
									else
										xmms_remote_set_playlist_pos(ir_gp.xmms_session, ir_numpress - 1);
								}
								ir_numpress = -1;
								ir_hundreds = 0;
							}
							else
							{
								g_timer_start(timer2);
								ir_numpress = i;
							}
							g_timer_reset(timer1);
						}
					}
				}
			}
			if (g_timer_elapsed(timer2, NULL) > PL_PAUSE)
			{
				if (ir_numpress >= 0 || ir_hundreds > 0)
				{
					if (ir_playlist_mode)
					{
						if (strcmp(ircfg.playlist[ir_numpress], ""))
						{
							ir_playlist = NULL;
							ir_playlist = g_list_append(ir_playlist, ircfg.playlist[ir_numpress]);
							xmms_remote_play_files(ir_gp.xmms_session, ir_playlist);
							g_list_free(ir_playlist);
						}
					}
					else if (ir_numpress <= 0)
						if (ir_hundreds > 0)
							xmms_remote_set_playlist_pos(ir_gp.xmms_session, (100 * ir_hundreds) - 1);
						else
							xmms_remote_set_playlist_pos(ir_gp.xmms_session,
										     xmms_remote_get_playlist_length(ir_gp.xmms_session) - 1);
					else
						xmms_remote_set_playlist_pos(ir_gp.xmms_session, (100 * ir_hundreds) + ir_numpress - 1);
				}
				ir_numpress = -1;
				ir_hundreds = 0;
				ir_playlist_mode = FALSE;
				g_timer_reset(timer2);
				g_timer_stop(timer2);
			}
		}
		ir_usleep(20000L);
	}
	g_timer_destroy(timer1);
	ir_close_port();
	pthread_exit(NULL);
}
