/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
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
#include "xmms.h"

#ifdef HPUX
# include <dl.h>
#else
# include <dlfcn.h>
#endif

#ifdef HPUX
# define SHARED_LIB_EXT ".sl"
#else
# define SHARED_LIB_EXT ".so"
#endif

#ifndef RTLD_NOW
# define RTLD_NOW 0
#endif


char *plugin_dir_list[] =
{
	PLUGINSUBS,
	NULL
};

extern struct InputPluginData *ip_data;
extern struct OutputPluginData *op_data;
extern struct EffectPluginData *ep_data;
extern struct GeneralPluginData *gp_data;
extern struct VisPluginData *vp_data;

void scan_plugins(char *dirname);
void add_plugin(char * filename);

static int d_iplist_compare(const void *a, const void *b)
{
	return strcmp(((char *) a), ((char *) b));
}

static int inputlist_compare_func(const void *a, const void *b)
{
	const InputPlugin *ap = a, *bp = b;
	return strcasecmp(ap->description, bp->description);
}

static int outputlist_compare_func(const void *a, const void *b)
{
	const OutputPlugin *ap = a, *bp = b;
	return strcasecmp(ap->description, bp->description);
}

static int effectlist_compare_func(const void *a, const void *b)
{
	const EffectPlugin *ap = a, *bp = b;
	return strcasecmp(ap->description, bp->description);
}

static int generallist_compare_func(const void *a, const void *b)
{
	const GeneralPlugin *ap = a, *bp = b;
	return strcasecmp(ap->description, bp->description);
}

static int vislist_compare_func(const void *a, const void *b)
{
	const VisPlugin *ap = a, *bp = b;
	return strcasecmp(ap->description, bp->description);
}

void init_plugins(void)
{
	char *dir, *temp, *temp2;
	GList *node, *disabled_iplugin_names = NULL;
	int dirsel = 0;

	if (cfg.disabled_iplugins)
	{
		temp = cfg.disabled_iplugins;
		while ((temp2 = strchr(temp, ',')) != NULL)
		{
			(*temp2) = '\0';
			disabled_iplugin_names = g_list_append(disabled_iplugin_names, g_strdup(temp));
			temp = temp2 + 1;
		}
		disabled_iplugin_names = g_list_append(disabled_iplugin_names, g_strdup(temp));
		g_free(cfg.disabled_iplugins);
		cfg.disabled_iplugins = NULL;
	}

	ip_data = g_malloc0(sizeof (struct InputPluginData));
	op_data = g_malloc0(sizeof (struct OutputPluginData));
	ep_data = g_malloc0(sizeof (struct EffectPluginData));
	gp_data = g_malloc0(sizeof (struct GeneralPluginData));
	vp_data = g_malloc0(sizeof (struct VisPluginData));


#ifndef DISABLE_USER_PLUGIN_DIR
	dir = g_strconcat(g_get_home_dir(), "/.xmms/Plugins", NULL);
	scan_plugins(dir);
	g_free(dir);

	/*
	 * Having directories below ~/.xmms/Plugins is depreciated and
	 * might be removed at some point.
	 */

	/*
	 * This is in a separate loop so if the user puts them in the
	 * wrong dir we'll still get them in the right order (home dir
	 * first)                                                - Zinx
	 */
	while (plugin_dir_list[dirsel])
	{
		dir = g_strconcat(g_get_home_dir(), "/.xmms/Plugins/",
				  plugin_dir_list[dirsel++], NULL);
		scan_plugins(dir);
		g_free(dir);
	}
	dirsel = 0;
#endif

	while (plugin_dir_list[dirsel])
	{
		dir = g_strconcat(PLUGIN_DIR, "/",
				  plugin_dir_list[dirsel++], NULL);
		scan_plugins(dir);
		g_free(dir);
	}

	op_data->output_list = g_list_sort(op_data->output_list, outputlist_compare_func);
	if (!op_data->current_output_plugin && g_list_length(op_data->output_list))
		op_data->current_output_plugin = op_data->output_list->data;
	ip_data->input_list = g_list_sort(ip_data->input_list, inputlist_compare_func);
	ep_data->effect_list = g_list_sort(ep_data->effect_list, effectlist_compare_func);
	ep_data->enabled_list = NULL;
	gp_data->general_list = g_list_sort(gp_data->general_list, generallist_compare_func);
	gp_data->enabled_list = NULL;
	vp_data->vis_list = g_list_sort(vp_data->vis_list, vislist_compare_func);
	vp_data->enabled_list = NULL;
	general_enable_from_stringified_list(cfg.enabled_gplugins);
	vis_enable_from_stringified_list(cfg.enabled_vplugins);
	effect_enable_from_stringified_list(cfg.enabled_eplugins);
	if (cfg.enabled_gplugins)
	{
		g_free(cfg.enabled_gplugins);
		cfg.enabled_gplugins = NULL;
	}
	if (cfg.enabled_vplugins)
	{
		g_free(cfg.enabled_vplugins);
		cfg.enabled_vplugins = NULL;
	}
	if (cfg.enabled_eplugins)
	{
		g_free(cfg.enabled_eplugins);
		cfg.enabled_eplugins = NULL;
	}

	node = op_data->output_list;
	while (node)
	{
		OutputPlugin *op = node->data;
		/*
		 * Only test basename to avoid problems when changing
		 * prefix.  We will only see one plugin with the same
		 * basename, so this is usually what the user want.
		 */
		if (!strcmp(g_basename(cfg.outputplugin),
			    g_basename(op->filename)))
			op_data->current_output_plugin = op;
		if (op->init)
			op->init();
		node = node->next;
	}

	node = ip_data->input_list;
	while (node)
	{
		InputPlugin *ip = node->data;
		temp = g_basename(ip->filename);
		if (g_list_find_custom(disabled_iplugin_names, temp, d_iplist_compare))
			disabled_iplugins = g_list_append(disabled_iplugins, ip);
		if (ip->init)
			ip->init();
		node = node->next;
	}

	node = disabled_iplugin_names;
	while (node)
	{
		g_free(node->data);
		node = node->next;
	}
	g_list_free(disabled_iplugin_names);

}

static void* open_dynamic_lib(char *filename)
{
#ifdef HPUX
	/*
	 * Use shl_load family of functions on HP-UX. HP-UX does not
	 * support dlopen on 32-bit PA-RISC executables
	 */
	return shl_load(filename, BIND_DEFERRED, 0); 
#else
	return dlopen(filename, RTLD_NOW);
#endif
}

static void close_dynamic_lib(void* handle)
{
#ifdef HPUX
	shl_t h = handle;
	shl_unload(h);
#else
	dlclose(handle);
#endif
}

static void* find_dynamic_symbol(void *handle, char *symbol)
{
#ifdef HPUX
	void *gpi;
	shl_t h = handle;
	if ((shl_findsym(&h, symbol, TYPE_PROCEDURE, &gpi)) != 0)
		gpi = NULL;
	return gpi;
#else
#ifdef SYMBOL_PREFIX
	char *sym = g_strconcat(SYMBOL_PREFIX, symbol, NULL);
	void *symh = dlsym(handle, sym);
	g_free(sym);
	return symh;
#else
	return dlsym(handle, symbol);
#endif /* SYMBOL_PREFIX */
#endif /* HPUX */
}

static void dynamic_lib_error(void)
{
#ifdef HPUX
	perror("Error loading plugin!"); 
#else
	fprintf(stderr, "%s\n", dlerror());
#endif
}

static int plugin_check_duplicate(char *filename)
{
	GList *l;
	char *base_filename = g_basename(filename);
	/*
	 * erg.. gotta check 'em all, surely there's a better way
	 *                                                 - Zinx
	 */

	for (l = ip_data->input_list; l; l = l->next)
		if (!strcmp(base_filename,
			    g_basename(((InputPlugin*)l->data)->filename)))
			return 1;

	for (l = op_data->output_list; l; l = l->next)
		if (!strcmp(base_filename,
			    g_basename(((OutputPlugin*)l->data)->filename)))
			return 1;

	for (l = ep_data->effect_list; l; l = l->next)
		if (!strcmp(base_filename,
			    g_basename(((EffectPlugin*)l->data)->filename)))
			return 1;

	for (l = gp_data->general_list; l; l = l->next)
		if (!strcmp(base_filename,
			    g_basename(((GeneralPlugin*)l->data)->filename)))
			return 1;

	for (l = vp_data->vis_list; l; l = l->next)
		if (!strcmp(base_filename,
			    g_basename(((VisPlugin*)l->data)->filename)))
			return 1;

	return 0;
}


void add_plugin(char * filename)
{
	void *h;
	void *(*gpi) (void);

	if (plugin_check_duplicate(filename))
		return;
	
	if ((h = open_dynamic_lib(filename)) == NULL)
	{
		dynamic_lib_error();
		return;
	}

	if ((gpi = find_dynamic_symbol(h, "get_iplugin_info")) != NULL)
	{
		InputPlugin *p = gpi();
		p->handle = h;
		p->filename = g_strdup(filename);
		p->get_vis_type = input_get_vis_type;
		p->add_vis_pcm = input_add_vis_pcm;
		p->set_info = playlist_set_info;
		p->set_info_text = input_set_info_text;

		ip_data->input_list = g_list_prepend(ip_data->input_list, p);
	}
	else if ((gpi = find_dynamic_symbol(h, "get_oplugin_info")) != NULL)
	{
		OutputPlugin *p = gpi();
		p->handle = h;
		p->filename = g_strdup(filename);
		op_data->output_list = g_list_prepend(op_data->output_list, p);
	}
	else if ((gpi = find_dynamic_symbol(h, "get_eplugin_info")) != NULL)
	{
		EffectPlugin *p = gpi();
		p->handle = h;
		p->filename = g_strdup(filename);
		ep_data->effect_list = g_list_prepend(ep_data->effect_list, p);
	}
	else if ((gpi = find_dynamic_symbol(h, "get_gplugin_info")) != NULL)
	{
		GeneralPlugin *p = gpi();
		p->handle = h;
		p->filename = g_strdup(filename);
		p->xmms_session = ctrlsocket_get_session_id();
		gp_data->general_list = g_list_prepend(gp_data->general_list, p);
	}
	else if ((gpi = find_dynamic_symbol(h, "get_vplugin_info")) != NULL)
	{
		VisPlugin *p = gpi();
		p->handle = h;
		p->filename = g_strdup(filename);
		p->xmms_session = ctrlsocket_get_session_id();
		p->disable_plugin = vis_disable_plugin;
		vp_data->vis_list = g_list_prepend(vp_data->vis_list, p);
	}
	else
		close_dynamic_lib(h);
}

void scan_plugins(char *dirname)
{
	char *filename, *ext;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;

	dir = opendir(dirname);
	if (!dir)
		return;

	while ((ent = readdir(dir)) != NULL)
	{
		filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
		    (ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, SHARED_LIB_EXT))
				add_plugin(filename);
		g_free(filename);
	}
	closedir(dir);
}

void cleanup_plugins(void)
{
	GList *node, *next;

	if (get_input_playing())
		input_stop();

	g_list_free(disabled_iplugins);
	node = ip_data->input_list;
	while (node)
	{
		InputPlugin *ip = node->data;
		if (ip->cleanup)
		{
			ip->cleanup();
			GDK_THREADS_LEAVE();
			while(g_main_iteration(FALSE));
			GDK_THREADS_ENTER();
		}
		g_free(ip->filename);
		close_dynamic_lib(ip->handle);
		node = node->next;
	}
	g_list_free(ip_data->input_list);
	g_free(ip_data);

	node = op_data->output_list;
	while (node)
	{
		OutputPlugin *op = node->data;
		g_free(op->filename);
		close_dynamic_lib(op->handle);
		node = node->next;
	}
	g_list_free(op_data->output_list);
	g_free(op_data);

	node = ep_data->effect_list;
	while (node)
	{
		EffectPlugin *ep = node->data;
		if (ep->cleanup)
		{
			ep->cleanup();
			GDK_THREADS_LEAVE();
			while(g_main_iteration(FALSE));
			GDK_THREADS_ENTER();
		}
		g_free(ep->filename);
		close_dynamic_lib(ep->handle);
		node = node->next;
	}
	g_list_free(ep_data->effect_list);
	g_free(ep_data);

	node = get_general_enabled_list();
	while (node)
	{
		GeneralPlugin *gp = node->data;
		next = node->next;
		enable_general_plugin(g_list_index(gp_data->general_list, gp), FALSE);
		node = next;
	}
	g_list_free(gp_data->enabled_list);

	GDK_THREADS_LEAVE();
	while(g_main_iteration(FALSE));
	GDK_THREADS_ENTER();
	
	node = gp_data->general_list;
	while (node)
	{
		GeneralPlugin *gp = node->data;
		g_free(gp->filename);
		close_dynamic_lib(gp->handle);
		node = node->next;
	}
	g_list_free(gp_data->general_list);
	g_free(gp_data);

	node = get_vis_enabled_list();
	while (node)
	{
		VisPlugin *vp = node->data;
		next = node->next;
		enable_vis_plugin(g_list_index(vp_data->vis_list, vp), FALSE);
		node = next;
	}
	g_list_free(vp_data->enabled_list);
	
	GDK_THREADS_LEAVE();
	while(g_main_iteration(FALSE));
	GDK_THREADS_ENTER();
	
	node = vp_data->vis_list;
	while (node)
	{
		VisPlugin *vp = node->data;
		g_free(vp->filename);
		close_dynamic_lib(vp->handle);
		node = node->next;
	}
	g_list_free(vp_data->vis_list);
	g_free(vp_data);
}
