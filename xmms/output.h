
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
#ifndef OUTPUT_H
#define OUTPUT_H

struct OutputPluginData
{
	GList *output_list;
	OutputPlugin *current_output_plugin;
};

GList *get_output_list(void);
OutputPlugin *get_current_output_plugin(void);
void set_current_output_plugin(int i);
void output_about(int i);
void output_configure(int i);
void output_get_volume(int *l, int *r);
void output_set_volume(int l, int r);

#endif
