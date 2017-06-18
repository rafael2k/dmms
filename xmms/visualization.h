/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
                             Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
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
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

struct VisPluginData
{
	GList *vis_list;
	GList *enabled_list;
	gboolean playback_started;	
};

GList *get_vis_list(void);
GList *get_vis_enabled_list(void);
void enable_vis_plugin(int i, gboolean enable);
void vis_disable_plugin(VisPlugin *vp);
void vis_about(int i);
void vis_configure(int i);
void vis_playback_start(void);
void vis_playback_stop(void);
gboolean vis_enabled(int i);
gchar *vis_stringify_enabled_list(void);
void vis_enable_from_stringified_list(gchar * list);
void vis_send_data(gint16 pcm_data[2][512], int nch, int lenght);

#endif
