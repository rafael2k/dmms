/* Sun.c; data to be exported for the plugin to be used by XMMS */

#include "Sun.h"
#include <glib.h>
#include "config.h"
#include "xmms/i18n.h"

static OutputPlugin op =
{
	NULL,
	NULL,
	NULL,
	abuffer_init,
	aboutSunAudio,
	abuffer_configure,
	abuffer_get_volume,
	abuffer_set_volume,
	abuffer_open,
	abuffer_write,
	abuffer_close,
	abuffer_flush,
	abuffer_pause,
	abuffer_free,
	abuffer_used,
	abuffer_get_output_time,
	abuffer_get_written_time,
};


OutputPlugin * get_oplugin_info(void)
{
	op.description = g_strdup_printf(_("Solaris audio plugin %s"), VERSION);
	return &op;
}
