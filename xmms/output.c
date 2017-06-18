/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
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
#include "xmms.h"

struct OutputPluginData *op_data;

OutputPlugin *get_current_output_plugin(void)
{
	return op_data->current_output_plugin;
}

void set_current_output_plugin(int i)
{
	GList *node = g_list_nth(op_data->output_list, i);
	if (node)
		op_data->current_output_plugin = node->data;
	else
		op_data->current_output_plugin = NULL;
}

GList *get_output_list(void)
{
	return op_data->output_list;
}

void output_about(int i)
{
	OutputPlugin *out = (OutputPlugin *) g_list_nth(op_data->output_list, i)->data;
	if (out && out->about)
		out->about();
}

void output_configure(int i)
{
	OutputPlugin *out = (OutputPlugin *) g_list_nth(op_data->output_list, i)->data;
	if (out && out->configure)
		out->configure();
}

void output_get_volume(int *l, int *r)
{
	if (op_data->current_output_plugin && op_data->current_output_plugin->get_volume)
		op_data->current_output_plugin->get_volume(l, r);
	else
		(*l) = (*r) = -1;
}

void output_set_volume(int l, int r)
{
	if (op_data->current_output_plugin && op_data->current_output_plugin->set_volume)
		op_data->current_output_plugin->set_volume(l, r);
}
