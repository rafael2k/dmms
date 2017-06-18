/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2006  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2006  Haavard Kvaalen
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

struct EffectPluginData *ep_data;
static pthread_mutex_t emutex = PTHREAD_MUTEX_INITIALIZER;

static int effect_do_mod_samples(gpointer *data, int length, AFormat fmt, int srate, int nch)
{
	GList *l;

	pthread_mutex_lock(&emutex);
	l = ep_data->enabled_list;

	while (l)
	{
		if (l->data)
		{
			EffectPlugin *ep = l->data;
			if (ep->mod_samples)
				length = ep->mod_samples(data, length, fmt, srate, nch);
		}
		l = g_list_next(l);
	}
	pthread_mutex_unlock(&emutex);
	
	return length;
}

static void effect_do_query_format(AFormat *fmt, int *rate, int *nch)
{
	GList *l;

	pthread_mutex_lock(&emutex);
	l = ep_data->enabled_list;

	while (l)
	{
		if (l->data)
		{
			EffectPlugin *ep = l->data;
			if (ep->query_format)
				ep->query_format(fmt, rate, nch);
		}
		l = g_list_next(l);
	}	
	pthread_mutex_unlock(&emutex);
}

static EffectPlugin pseudo_effect_plugin =
{
	NULL,
	NULL,
	"XMMS Multiple Effects Support",
	NULL, 
	NULL,
	NULL,
	NULL,
	effect_do_mod_samples,
	effect_do_query_format
};
	
/* get_current_effect_plugin() and effects_enabled() are still to be used by 
 * output plugins as they were when we only supported one effects plugin at
 * a time. We now had a pseudo-effects-plugin that chains all the enabled
 * plugins. -- Jakdaw */

EffectPlugin *get_current_effect_plugin(void)
{
	return &pseudo_effect_plugin;
}

int effects_enabled(void)
{
	return TRUE;
}

void effect_about(int i)
{
	GList *node = g_list_nth(ep_data->effect_list, i);
	if (node)
	{
		EffectPlugin *effect = node->data;
		if (effect && effect->about)
			effect->about();
	}
}

void effect_configure(int i)
{
	GList *node = g_list_nth(ep_data->effect_list, i);
	if (node)
	{
		EffectPlugin *effect = node->data;
		if (effect && effect->configure)
			effect->configure();
	}
}


void enable_effect_plugin(int i, gboolean enable)
{
	GList *node = g_list_nth(ep_data->effect_list, i);
	EffectPlugin *ep;
	
	if (!node || !(node->data))
		return;
	ep = node->data;
	
	pthread_mutex_lock(&emutex);
	if (enable && !g_list_find(ep_data->enabled_list, ep))
	{
		ep_data->enabled_list = g_list_append(ep_data->enabled_list, ep);
		if (ep->init)
			ep->init();
	}
	else if (!enable && g_list_find(ep_data->enabled_list, ep))
	{
		ep_data->enabled_list = g_list_remove(ep_data->enabled_list, ep);
		if (ep->cleanup)
			ep->cleanup();
	}
	pthread_mutex_unlock(&emutex);
}

GList *get_effect_list(void)
{
	return ep_data->effect_list;
}

gboolean effect_enabled(int i)
{
	GList *pl = g_list_nth(ep_data->effect_list, i);
	if (g_list_find(ep_data->enabled_list, pl->data))
		return TRUE;
	return FALSE;
}

gchar *effect_stringify_enabled_list(void)
{
	char *enalist = NULL, *temp;
	GList *node = ep_data->enabled_list;

	if (g_list_length(node))
	{
		EffectPlugin *ep = node->data;
		enalist = g_strdup(g_basename(ep->filename));
		node = node->next;
		while (node)
		{
			temp = enalist;
			ep = node->data;
			enalist = g_strconcat(temp, ",", g_basename(ep->filename), NULL);
			g_free(temp);
			node = node->next;
		}
	}
	return enalist;
}

void effect_enable_from_stringified_list(char * list)
{
	char **plugins, *base;
	GList *node;
	int i;

	if (!list || list[0] == '\0')
		return;
	plugins = g_strsplit(list, ",", 0);
	for (i = 0; plugins[i]; i++)
	{
		node = ep_data->effect_list;
		while (node)
		{
			EffectPlugin *ep = node->data;
			base = g_basename(ep->filename);
			if (!strcmp(plugins[i], base))
			{
				ep_data->enabled_list = g_list_append(ep_data->enabled_list, ep);
				if(ep->init)
					ep->init();
			}
			node = node->next;
		}
	}
	g_strfreev(plugins);
}
