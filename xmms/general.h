#ifndef GENERIC_H
#define GENERIC_H

struct GeneralPluginData
{
	GList *general_list;
	GList *enabled_list;
};

GList *get_general_list(void);
GList *get_general_enabled_list(void);
void enable_general_plugin(int i, gboolean enable);
void general_about(int i);
void general_configure(int i);
gboolean general_enabled(int i);
gchar *general_stringify_enabled_list(void);
void general_enable_from_stringified_list(gchar * list);

#endif
