/*  xmms - graphically mp3 player..
 *  Copyright (C) 1998-1999  Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
#ifndef SUN_H
#define SUN_H

#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/audioio.h>

#include <fcntl.h>
#include <thread.h>

#include <unistd.h>
#include <stropts.h>

#include <stdlib.h>
#include <stdio.h>

#include "xmms/plugin.h"
#include "libxmms/configfile.h"

extern OutputPlugin op;

typedef struct
{
	gchar *audio_device;
	gboolean always_audiodev;
	gint channel_flags;
	gint buffer_size;
	gint prebuffer;
} SunConfig;

extern SunConfig sun_cfg;

void abuffer_init(void);
void aboutSunAudio(void);
void abuffer_configure(void);

void abuffer_get_volume(int *l,int *r);
void abuffer_set_volume(int l,int r);

int abuffer_used(void);
int abuffer_free(void);
void abuffer_write(void *ptr,int length);
void abuffer_close(void);
void abuffer_flush(int time);
void abuffer_pause(short p);
int abuffer_open(AFormat fmt,int rate,int nch);
int abuffer_get_output_time(void);
int abuffer_get_written_time(void);
void abuffer_set_audio_params(void);
int abuffer_isopen(void);
int abuffer_update_dev(void);
int abuffer_set_dev(void);
			
#endif /* SUN_H */
