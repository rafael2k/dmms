/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
#include "OSS.h"
#include <errno.h>

gint oss_pan_l = -1, oss_pan_r = -1;

static char* get_mixer_device(void)
{
	char *name;
	
	if (oss_cfg.use_alt_mixer_device && oss_cfg.alt_mixer_device)
		name = g_strdup(oss_cfg.alt_mixer_device);
	else if (oss_cfg.mixer_device > 0)
		name = g_strdup_printf("%s%d", DEV_MIXER, oss_cfg.mixer_device);
	else
		name = g_strdup(DEV_MIXER);

	return name;
}

void oss_get_volume(int *l, int *r)
{
	int fd, v, cmd, devs;
	gchar *devname;

	if (oss_pan_l >= 0)
	{
		*l = oss_pan_l;
		*r = oss_pan_r;
		return;
	}

	devname = get_mixer_device();
	fd = open(devname, O_RDONLY);
	g_free(devname);

	/*
	 * We dont show any errors if this fails, as this is called
	 * rather often
	 */
	if (fd != -1)
	{
		ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
		if ((devs & SOUND_MASK_PCM) && (oss_cfg.use_master==0))
			cmd = SOUND_MIXER_READ_PCM;
		else if ((devs & SOUND_MASK_VOLUME) && (oss_cfg.use_master==1))
			cmd = SOUND_MIXER_READ_VOLUME;
		else
		{
			close(fd);
			return;
		}
		ioctl(fd, cmd, &v);
		*r = (v & 0xFF00) >> 8;
		*l = (v & 0x00FF);
		oss_pan_l = *l;
		oss_pan_r = *r;
		close(fd);
	}
}

void oss_set_volume(int l, int r)
{
	int fd, v, cmd, devs;
	gchar *devname;

	oss_pan_l = l;
	oss_pan_r = r;
	if (l != r)
	{
		/*
		 * Some mixers (OSSv4/vmix) only honor one byte of the
		 * legacy two-channel value and apply it to both channels,
		 * so a balance setting would mute the audio.  The actual
		 * balance is applied in software (see oss_apply_pan), the
		 * mixer just gets the louder of the two.
		 */
		if (r > l)
			l = r;
		else
			r = l;
	}

	devname = get_mixer_device();
	fd = open(devname, O_RDONLY);

	if (fd != -1)
	{
		ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
		if ((devs & SOUND_MASK_PCM) && (oss_cfg.use_master==0))
			cmd = SOUND_MIXER_WRITE_PCM;
		else if ((devs & SOUND_MASK_VOLUME) && (oss_cfg.use_master==1))
			cmd = SOUND_MIXER_WRITE_VOLUME;
		else
		{
			close(fd);
			return;
		}
		v = (r << 8) | l;
		ioctl(fd, cmd, &v);
		close(fd);
	}
	else
		g_warning("oss_set_volume(): Failed to open mixer device (%s): %s",
			  devname, strerror(errno));
	g_free(devname);
}
