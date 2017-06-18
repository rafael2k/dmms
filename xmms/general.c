#include "xmms.h"

#ifndef fixed
#define fixed short
#endif

struct GeneralPluginData *gp_data;

GList *get_general_list(void)
{
	return gp_data->general_list;
}

GList *get_general_enabled_list(void)
{
	return gp_data->enabled_list;
}

void general_about(int i)
{
	GList *node = g_list_nth(gp_data->general_list, i);

	if (node && node->data && ((GeneralPlugin *) node->data)->about)
		((GeneralPlugin *) node->data)->about();
}

void general_configure(int i)
{
	GList *node = g_list_nth(gp_data->general_list, i);

	if (node && node->data && ((GeneralPlugin *) node->data)->configure)
		((GeneralPlugin *) node->data)->configure();
}

void enable_general_plugin(int i, gboolean enable)
{
	GList *node = g_list_nth(gp_data->general_list, i);
	GeneralPlugin *gp;

	if (!node || !(node->data))
		return;
	gp = (GeneralPlugin *) node->data;

	if (enable && !g_list_find(gp_data->enabled_list, gp))
	{
		gp_data->enabled_list = g_list_append(gp_data->enabled_list, gp);
		if (gp->init)
			gp->init();
	}
	else if (!enable && g_list_find(gp_data->enabled_list, gp))
	{
		gp_data->enabled_list = g_list_remove(gp_data->enabled_list, gp);
		if (gp->cleanup)
			gp->cleanup();
	}
}

gboolean general_enabled(int i)
{
	return (g_list_find(gp_data->enabled_list, (GeneralPlugin *) g_list_nth(gp_data->general_list, i)->data) ? TRUE : FALSE);
}

gchar *general_stringify_enabled_list(void)
{
	gchar *enalist = NULL, *temp, *temp2;
	GList *node = gp_data->enabled_list;

	if (g_list_length(node))
	{
		enalist = g_strdup(g_basename(((GeneralPlugin *) node->data)->filename));
		node = node->next;
		while (node)
		{
			temp = enalist;
			temp2 = g_strdup(g_basename(((GeneralPlugin *) node->data)->filename));
			enalist = g_strconcat(temp, ",", temp2, NULL);
			g_free(temp);
			g_free(temp2);
			node = node->next;
		}
	}
	return enalist;
}

void general_enable_from_stringified_list(gchar * list)
{
	gchar **plugins, *base;
	GList *node;
	gint i;
	GeneralPlugin *gp;

	if (!list || !strcmp(list, ""))
		return;
	plugins = g_strsplit(list, ",", 0);
	for(i = 0; plugins[i]; i++)
	{
		node = gp_data->general_list;
		while (node)
		{
			base = g_basename(((GeneralPlugin *) node->data)->filename);
			if (!strcmp(plugins[i], base))
			{
				gp = node->data;
				gp_data->enabled_list = g_list_append(gp_data->enabled_list, gp);
				if(gp->init)
					gp->init();
			}
			node = node->next;
		}
	}
	g_strfreev(plugins);
}
