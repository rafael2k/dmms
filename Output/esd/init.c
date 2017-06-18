/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999       Galex Yen
 *  Copyright (C) 1999-2004  Haavard Kvaalen
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

#include "esdout.h"
#include "libxmms/configfile.h"

ESDConfig esd_cfg;
esd_info_t *all_info;
esd_player_info_t *player_info;

void esdout_init(void)
{
	ConfigFile *cfgfile;
	char *env;
	int l = 100, r = 100;

	memset(&esd_cfg, 0, sizeof (ESDConfig));
	esd_cfg.port = ESD_DEFAULT_PORT;
	esd_cfg.buffer_size = 3000;
	esd_cfg.prebuffer = 25;

	cfgfile = xmms_cfg_open_default_file();

	if ((env = getenv("ESPEAKER")) != NULL)
	{
		char *temp;
		esd_cfg.use_remote = TRUE;
		esd_cfg.server = g_strdup(env);
		temp = strchr(esd_cfg.server,':');
		if (temp != NULL)
		{
			*temp = '\0';
			esd_cfg.port = atoi(temp+1);
			if (esd_cfg.port == 0)
				esd_cfg.port = ESD_DEFAULT_PORT;
		}
	}
	else
	{
		xmms_cfg_read_boolean(cfgfile, "ESD", "use_remote",
				      &esd_cfg.use_remote);
		xmms_cfg_read_string(cfgfile, "ESD", "remote_host",
				     &esd_cfg.server);
		xmms_cfg_read_int(cfgfile, "ESD", "remote_port", &esd_cfg.port);
	}
	xmms_cfg_read_boolean(cfgfile, "ESD", "use_oss_mixer",
			      &esd_cfg.use_oss_mixer);
	xmms_cfg_read_int(cfgfile, "ESD", "buffer_size", &esd_cfg.buffer_size);
	xmms_cfg_read_int(cfgfile, "ESD", "prebuffer", &esd_cfg.prebuffer);
	xmms_cfg_read_int(cfgfile, "ESD", "volume_l", &l);
	xmms_cfg_read_int(cfgfile, "ESD", "volume_r", &r);
	esdout_mixer_init_vol(l, r);
	
	xmms_cfg_free(cfgfile);

	if (!esd_cfg.server)
		esd_cfg.server = g_strdup("localhost");
}
