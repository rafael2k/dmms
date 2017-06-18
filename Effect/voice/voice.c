/* A voice removal plugin 
   by Anders Carlsson <andersca@gnu.org> */

#include "voice.h"
#include "xmms/i18n.h"

static int mod_samples(gpointer * d, gint length, AFormat afmt, gint srate, gint nch);

EffectPlugin voice_ep =
{
	NULL,
	NULL,
	NULL, /* Description */
	NULL,
	NULL,
	voice_about,
	NULL,
	mod_samples,
};

EffectPlugin *get_eplugin_info(void)
{
	voice_ep.description = g_strdup_printf(_("Voice removal plugin %s"), VERSION);
	return &voice_ep;
}

static int mod_samples(gpointer * d, gint length, AFormat afmt, gint srate, gint nch)
{
	int x;
	int left, right;
	gint16 *dataptr = (gint16 *)*d;

	if (!(afmt == FMT_S16_NE || (afmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) || (afmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN)) || nch != 2)
		return length;

	
	for (x = 0; x < length; x += 4)
	{
		left = dataptr[1] - dataptr[0];
		right = dataptr[0] - dataptr[1];
		if (left < -32768)
			left = -32768;
		if (left > 32767)
			left = 32767;
		if (right < -32768)
			right = -32768;
		if (right > 32767)
			right = 32767;
		dataptr[0] = left;
		dataptr[1] = right;
		dataptr += 2;
	}

	return length;
}
