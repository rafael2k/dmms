/*
 *  Copyright (C) 2001  Haavard Kvaalen
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

#include "OSS.h"

#include <math.h>

struct buffer {
	void *buffer;
	int size;
} format_buffer, stereo_buffer;


static void* oss_get_convert_buffer(struct buffer *buffer, size_t size)
{
	if (size > 0 && size <= buffer->size)
		return buffer->buffer;

	buffer->size = size;
	buffer->buffer = g_realloc(buffer->buffer, size);
	return buffer->buffer;
}

void oss_free_convert_buffer(void)
{
	oss_get_convert_buffer(&format_buffer, 0);
	oss_get_convert_buffer(&stereo_buffer, 0);
}


static int convert_swap_endian(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr);

	return i;
}

static int convert_swap_sign_and_endian_to_native(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr) ^ 1 << 15;

	return i;
}

static int convert_swap_sign_and_endian_to_alien(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr ^ 1 << 15);

	return i;
}

static int convert_swap_sign16(void **data, int length)
{
	gint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr ^= 1 << 15;

	return i;
}

static int convert_swap_sign8(void **data, int length)
{
	gint8 *ptr = *data;
	int i;
	for (i = 0; i < length; i++)
		*ptr++ ^= 1 << 7;

	return i;
}

static int convert_to_8_native_endian(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = *input++ >> 8;

	return i;
}

static int convert_to_8_native_endian_swap_sign(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = (*input++ >> 8) ^ (1 << 7);

	return i;
}


static int convert_to_8_alien_endian(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = *input++ & 0xff;

	return i;
}

static int convert_to_8_alien_endian_swap_sign(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = (*input++ & 0xff) ^ (1 << 7);

	return i;
}

static int convert_to_16_native_endian(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = oss_get_convert_buffer(&format_buffer, length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++ << 8;

	return i * 2;
}

static int convert_to_16_native_endian_swap_sign(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = oss_get_convert_buffer(&format_buffer, length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = (*input++ << 8) ^ (1 << 15);

	return i * 2;
}


static int convert_to_16_alien_endian(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = oss_get_convert_buffer(&format_buffer, length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++;

	return i * 2;
}

static int convert_to_16_alien_endian_swap_sign(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = oss_get_convert_buffer(&format_buffer, length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++ ^ (1 << 7);

	return i * 2;
}

int (*oss_get_convert_func(int output, int input))(void **, int)
{
	if (output == input)
		return NULL;

	if ((output == AFMT_U16_BE && input == AFMT_U16_LE) ||
	    (output == AFMT_U16_LE && input == AFMT_U16_BE) ||
	    (output == AFMT_S16_BE && input == AFMT_S16_LE) ||
	    (output == AFMT_S16_LE && input == AFMT_S16_BE))
		return convert_swap_endian;

	if ((output == AFMT_U16_BE && input == AFMT_S16_BE) ||
	    (output == AFMT_U16_LE && input == AFMT_S16_LE) ||
	    (output == AFMT_S16_BE && input == AFMT_U16_BE) ||
	    (output == AFMT_S16_LE && input == AFMT_U16_LE))
		return convert_swap_sign16;

	if ((IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_S16_LE) ||
	      (output == AFMT_S16_BE && input == AFMT_U16_LE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_S16_BE) ||
	      (output == AFMT_S16_LE && input == AFMT_U16_BE))))
		return convert_swap_sign_and_endian_to_native;
		
	if ((!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_S16_LE) ||
	      (output == AFMT_S16_BE && input == AFMT_U16_LE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_S16_BE) ||
	      (output == AFMT_S16_LE && input == AFMT_U16_BE))))
		return convert_swap_sign_and_endian_to_alien;

	if ((IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_U16_BE) ||
	      (output == AFMT_S8 && input == AFMT_S16_BE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_U16_LE) ||
	      (output == AFMT_S8 && input == AFMT_S16_LE))))
		return convert_to_8_native_endian;

	if ((IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_S16_BE) ||
	      (output == AFMT_S8 && input == AFMT_U16_BE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_S16_LE) ||
	      (output == AFMT_S8 && input == AFMT_U16_LE))))
		return convert_to_8_native_endian_swap_sign;

	if ((!IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_U16_BE) ||
	      (output == AFMT_S8 && input == AFMT_S16_BE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_U16_LE) ||
	      (output == AFMT_S8 && input == AFMT_S16_LE))))
		return convert_to_8_alien_endian;

	if ((!IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_S16_BE) ||
	      (output == AFMT_S8 && input == AFMT_U16_BE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == AFMT_U8 && input == AFMT_S16_LE) ||
	      (output == AFMT_S8 && input == AFMT_U16_LE))))
		return convert_to_8_alien_endian_swap_sign;

	if ((output == AFMT_U8 && input == AFMT_S8) ||
	    (output == AFMT_S8 && input == AFMT_U8))
		return convert_swap_sign8;

	if ((IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_U8) ||
	      (output == AFMT_S16_BE && input == AFMT_S8))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_U8) ||
	      (output == AFMT_S16_LE && input == AFMT_S8))))
		return convert_to_16_native_endian;

	if ((IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_S8) ||
	      (output == AFMT_S16_BE && input == AFMT_U8))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_S8) ||
	      (output == AFMT_S16_LE && input == AFMT_U8))))
		return convert_to_16_native_endian_swap_sign;

	if ((!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_U8) ||
	      (output == AFMT_S16_BE && input == AFMT_S8))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_U8) ||
	      (output == AFMT_S16_LE && input == AFMT_S8))))
		return convert_to_16_alien_endian;

	if ((!IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_BE && input == AFMT_S8) ||
	      (output == AFMT_S16_BE && input == AFMT_U8))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == AFMT_U16_LE && input == AFMT_S8) ||
	      (output == AFMT_S16_LE && input == AFMT_U8))))
		return convert_to_16_alien_endian_swap_sign;

	g_warning("Translation needed, but not available.\n"
		  "Input: %d; Output %d.", input, output);
	return NULL;
}

static int convert_mono_to_stereo(void **data, int length, int fmt)
{
	int i;
	void *outbuf = oss_get_convert_buffer(&stereo_buffer, length * 2);

	if (fmt == AFMT_U8 || fmt ==  AFMT_S8)
	{
		guint8 *output = outbuf, *input = *data;
		for (i = 0; i < length; i++)
		{
			*output++ = *input;
			*output++ = *input;
			input++;
		}
	}
	else 
	{
		guint16 *output = outbuf, *input = *data;
		for (i = 0; i < length / 2; i++)
		{
			*output++ = *input;
			*output++ = *input;
			input++;
		}
	}
	*data = outbuf;

	return length * 2;
}

static int convert_stereo_to_mono(void **data, int length, int fmt)
{
	int i;

	switch (fmt)
	{
		case AFMT_U8:
		{
			guint8 *output = *data, *input = *data;
			for (i = 0; i < length / 2; i++)
			{
				guint16 tmp;
				tmp = *input++;
				tmp += *input++;
				*output++ = tmp / 2;
			}
		}
		break;
		case AFMT_S8:
		{
			gint8 *output = *data, *input = *data;
			for (i = 0; i < length / 2; i++)
			{
				gint16 tmp;
				tmp = *input++;
				tmp += *input++;
				*output++ = tmp / 2;
			}
		}
		break;
		case AFMT_U16_LE:
		{
			guint16 *output = *data, *input = *data;
			for (i = 0; i < length / 4; i++)
			{
				guint32 tmp;
				guint16 stmp;
				tmp = GUINT16_FROM_LE(*input);
				input++;
				tmp += GUINT16_FROM_LE(*input);
				input++;
				stmp = tmp / 2;
				*output++ = GUINT16_TO_LE(stmp);
			}
		}
		break;
		case AFMT_U16_BE:
		{
			guint16 *output = *data, *input = *data;
			for (i = 0; i < length / 4; i++)
			{
				guint32 tmp;
				guint16 stmp;
				tmp = GUINT16_FROM_BE(*input);
				input++;
				tmp += GUINT16_FROM_BE(*input);
				input++;
				stmp = tmp / 2;
				*output++ = GUINT16_TO_BE(stmp);
			}
		}
		break;
		case AFMT_S16_LE:
		{
			gint16 *output = *data, *input = *data;
			for (i = 0; i < length / 4; i++)
			{
				gint32 tmp;
				gint16 stmp;
				tmp = GINT16_FROM_LE(*input);
				input++;
				tmp += GINT16_FROM_LE(*input);
				input++;
				stmp = tmp / 2;
				*output++ = GINT16_TO_LE(stmp);
			}
		}
		break;
		case AFMT_S16_BE:
		{
			gint16 *output = *data, *input = *data;
			for (i = 0; i < length / 4; i++)
			{
				gint32 tmp;
				gint16 stmp;
				tmp = GINT16_FROM_BE(*input);
				input++;
				tmp += GINT16_FROM_BE(*input);
				input++;
				stmp = tmp / 2;
				*output++ = GINT16_TO_BE(stmp);
			}
		}
		break;
		default:
			g_error("unknown format");
	}

	return length / 2;
}

int (*oss_get_stereo_convert_func(int output, int input))(void **, int, int)
{
	if (output == input)
		return NULL;

	if (input == 1 && output == 2)
		return convert_mono_to_stereo;
	if (input == 2 && output == 1)
		return convert_stereo_to_mono;

	g_warning("Input has %d channels, soundcard uses %d channels\n"
		  "No conversion is available", input, output);
	return NULL;
}

#define EQ_BANDS 10

typedef struct {
	gfloat b0, b1, b2, a1, a2;
	gfloat x1, x2, y1, y2;
} EqBiquad;

static gfloat eq_band_freqs[EQ_BANDS] =
{60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};
static EqBiquad eq_filters[EQ_BANDS][2];
static gint eq_active = FALSE;
static gint eq_rate = 0;
static gfloat eq_gains[EQ_BANDS];

void oss_set_eq(int on, float preamp, float *bands)
{
	int i;

	eq_active = on;
	for (i = 0; i < EQ_BANDS; i++)
		eq_gains[i] = bands[i] + preamp;
	eq_rate = 0;
}

static void eq_recompute(gint rate)
{
	int i, ch;

	for (i = 0; i < EQ_BANDS; i++)
	{
		gfloat w0 = 2.0 * M_PI * eq_band_freqs[i] / rate;
		gfloat alpha = sin(w0) / 2.0;
		gfloat A = pow(10.0, eq_gains[i] / 40.0);
		gfloat a0 = 1.0 + alpha / A;

		for (ch = 0; ch < 2; ch++)
		{
			EqBiquad *q = &eq_filters[i][ch];
			q->b0 = (1.0 + alpha * A) / a0;
			q->b1 = (-2.0 * cos(w0)) / a0;
			q->b2 = (1.0 - alpha * A) / a0;
			q->a1 = (-2.0 * cos(w0)) / a0;
			q->a2 = (1.0 - alpha / A) / a0;
			q->x1 = q->x2 = q->y1 = q->y2 = 0.0;
		}
	}
	eq_rate = rate;
}

void oss_apply_equalizer(void *data, int length, int fmt, int nch, int rate)
{
	int i, b, ch;
	gint16 *d = data;
	int nsamples;
	gint32 off;

	if (!eq_active || nch < 1 || nch > 2)
		return;

	switch (fmt)
	{
		case FMT_S16_NE:
#ifdef WORDS_BIGENDIAN
		case FMT_S16_BE:
#else
		case FMT_S16_LE:
#endif
			off = 0;
			break;
		case FMT_U16_NE:
#ifdef WORDS_BIGENDIAN
		case FMT_U16_BE:
#else
		case FMT_U16_LE:
#endif
			off = 1 << 15;
			break;
		default:
			return;
	}

	if (eq_rate != rate)
		eq_recompute(rate);

	nsamples = length / sizeof(gint16);
	for (i = 0; i < nsamples; i++)
	{
		gfloat x;
		ch = (nch == 2) ? (i & 1) : 0;
		x = d[i] - off;
		for (b = 0; b < EQ_BANDS; b++)
		{
			EqBiquad *q = &eq_filters[b][ch];
			gfloat y = q->b0 * x + q->b1 * q->x1 + q->b2 * q->x2
				- q->a1 * q->y1 - q->a2 * q->y2;
			q->x2 = q->x1;
			q->x1 = x;
			q->y2 = q->y1;
			q->y1 = y;
			x = y;
		}
		x += off;
		if (x > 32767.0)
			x = 32767.0;
		else if (x < -32768.0)
			x = -32768.0;
		d[i] = (gint16) x;
	}
}

void oss_apply_pan(void *data, int length, int fmt, int nch)
{
	int i;

	if (nch != 2)
		return;

	switch (fmt)
	{
		case AFMT_S16_LE:
		case AFMT_S16_BE:
		case AFMT_U16_LE:
		case AFMT_U16_BE:
		{
			gint16 *d = data;
			for (i = 0; i < length / 2; i += 2)
			{
				gint16 l = d[i], r = d[i + 1];
				d[i] = (gint16) ((l * oss_pan_l) / 100);
				d[i + 1] = (gint16) ((r * oss_pan_r) / 100);
			}
		}
		break;
		case AFMT_S8:
		case AFMT_U8:
		{
			gint8 *d = data;
			for (i = 0; i < length; i += 2)
			{
				d[i] = (gint8) ((d[i] * oss_pan_l) / 100);
				d[i + 1] = (gint8) ((d[i + 1] * oss_pan_r) / 100);
			}
		}
		break;
	}
}
