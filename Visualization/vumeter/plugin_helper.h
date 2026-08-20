#ifndef __PLUGIN_HELPER_H
#define __PLUGIN_HELPER_H

char *trim(char *,char *);
void reset_win_structure(vumeter_window *);

/* GLib 1.2 has no g_base64_encode/g_base64_decode (GLib 2.x only) */
gchar *vumeter_base64_encode(const guchar *data, gint len);
guchar *vumeter_base64_decode(const gchar *data, gint *out_len);

#endif
