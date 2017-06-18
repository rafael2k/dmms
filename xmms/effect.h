#ifndef EFFECT_H
#define EFFECT_H

struct EffectPluginData
{
	GList *effect_list;
	GList *enabled_list;
	/* FIXME: Needed? */
	gboolean playing;
	gboolean paused;
};

GList *get_effect_list(void);
void effect_about(int i);
void effect_configure(int i);
void enable_effect_plugin(int i, gboolean enable);
gboolean effect_enabled(int i);
gchar *effect_stringify_enabled_list(void);
void effect_enable_from_stringified_list(gchar * list);


#endif
