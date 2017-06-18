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

struct rgb_quad
{
	guchar rgbBlue;
	guchar rgbGreen;
	guchar rgbRed;
	guchar rgbReserved;
};

struct bitfieldmask {
	guint32 r, g, b;
};

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

static GdkGC *bmp_gc = NULL;

static int read_le_short(FILE * file, guint16 * ret)
{
	guint16 tmp;

	if (fread(&tmp, sizeof (guint16), 1, file) != 1)
		return 0;

	*ret = GINT16_FROM_LE(tmp);
	return 1;
}

static int read_le_long(FILE * file, guint32 * ret)
{
	guint32 tmp;

	if (fread(&tmp, sizeof (guint32), 1, file) != 1)
		return 0;

	*ret = GUINT32_FROM_LE(tmp);
	return 1;
}

static int find_first_set(guint32 v)
{
	int i;
	if (v == 0)
		return -1;
	for (i = 0; !(v & 1); i++)
		v >>= 1;
	return i;
}

static int find_last_set(guint32 v, int s)
{
	int i;
	v >>= s;
	for (i = 0; (v & 1) && i + s < 32; i++)
		v >>= 1;
	return i;
}

static void get_shift(guint32 v, int *r, int *l)
{
	int w;
	*r = find_first_set(v);
	*l = 0;
	w = find_last_set(v, *r);
	/* Limit to 8 bits */
	if (w > 8)
		*r += w - 8;
	else
		*l = 8 - w;
}

static void read_1b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette);
static void read_4b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette);
static void read_4b_rle(guint8 *input, guint32 compr_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette);
static void read_8b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette);
static void read_8b_rle(guint8 *input, guint32 compr_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette);
static void read_16b_rgb(guint8 *input, int input_size, guint8 *output,
			 guint32 w, guint32 h, struct bitfieldmask *mask);
static void read_24b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h);
static void read_32b_rgb(guint8 *input, int input_size, guint8 *output,
			 guint32 w, guint32 h, struct bitfieldmask *mask);


GdkPixmap *read_bmp(gchar * filename)
{
	FILE *file;
	gchar type[2];
	guint32 size, offset, headSize, w, h, comp, imgsize;
	guint16 bitcount;
	guint8 *data, *buffer;
	struct stat statbuf;

	struct rgb_quad rgb_quads[256];
	struct bitfieldmask mask = {0};
	GdkPixmap *ret = NULL;

	if (stat(filename, &statbuf) == -1)
		return NULL;
	size = statbuf.st_size;

	file = fopen(filename, "rb");
	if (!file)
		return NULL;

	if (fread(type, 1, 2, file) != 2)
		goto failure;
	if (strncmp(type, "BM", 2))
	{
		g_warning("read_bmp(): Error in BMP file: wrong type");
		goto failure;
	}
	fseek(file, 8, SEEK_CUR);
	if (!read_le_long(file, &offset) ||
	    !read_le_long(file, &headSize))
		goto failure;
	if (headSize == 12) /* BITMAPCOREINFO */
	{
		guint16 tmpw, tmph, dummy;
		
		if (!read_le_short(file, &tmpw) ||
		    !read_le_short(file, &tmph) ||
		    !read_le_short(file, &dummy) ||
		    !read_le_short(file, &bitcount))
			goto failure;
		w = tmpw;
		h = tmph;
		imgsize = size - offset;
		comp = BI_RGB;
	}
	else if (headSize == 40) /* BITMAPINFO */
	{
		guint16 tmp;
		
		if (!read_le_long(file, &w) ||
		    !read_le_long(file, &h) ||
		    !read_le_short(file, &tmp) ||
		    !read_le_short(file, &bitcount) ||
		    !read_le_long(file, &comp) ||
		    !read_le_long(file, &imgsize))
			goto failure;
		imgsize = size - offset;

		fseek(file, 16, SEEK_CUR);
	}
	else
	{
		g_warning("read_bmp(): Error in BMP file: Invalid header size: %d (%s)",
			  headSize, filename);
		goto failure;
	}
	if ((bitcount == 16 || bitcount == 32) && comp == BI_BITFIELDS)
	{
		if (offset - headSize - 14 >= 12)
		{
			if (!read_le_long(file, &mask.r) ||
			    !read_le_long(file, &mask.g) ||
			    !read_le_long(file, &mask.b))
				goto failure;
		}
	}
	else if (bitcount != 24 && bitcount != 16 && bitcount != 32)
	{
		gint ncols, i;

		ncols = offset - headSize - 14;
		if (headSize == 12)
		{
			ncols = MIN(ncols / 3, 256);
			for (i = 0; i < ncols; i++)
				if (fread(&rgb_quads[i], 3, 1, file) != 1)
					goto failure;
		}
		else
		{
			ncols = MIN(ncols / 4, 256);
			if (fread(rgb_quads, 4, ncols, file) != ncols)
				goto failure;
		}
	}
	fseek(file, offset, SEEK_SET);
	buffer = g_malloc(imgsize);
	fread(buffer, imgsize, 1, file);
	data = g_malloc0((w * 3 * h) + 3);	/* +3 is just for safety */

	if (bitcount == 1)
		read_1b_rgb(buffer, imgsize, data, w, h, rgb_quads);
	else if (bitcount == 4)
	{
		if (comp == BI_RLE4)
			read_4b_rle(buffer, imgsize, data, w, h, rgb_quads);
		else if (comp == BI_RGB)
			read_4b_rgb(buffer, imgsize, data, w, h, rgb_quads);
		else
			g_warning("read_bmp(): Invalid compression (%d)", comp);
	}
	else if (bitcount == 8)
	{
		if (comp == BI_RLE8)
			read_8b_rle(buffer, imgsize, data, w, h, rgb_quads);
		else if (comp == BI_RGB)
			read_8b_rgb(buffer, imgsize, data, w, h, rgb_quads);
		else
			g_warning("read_bmp(): Invalid compression (%d)", comp);
	}
	else if (bitcount == 16)
		read_16b_rgb(buffer, imgsize, data, w, h, &mask);
	else if (bitcount == 24)
		read_24b_rgb(buffer, imgsize, data, w, h);
	else if (bitcount == 32)
		read_32b_rgb(buffer, imgsize, data, w, h, &mask);
	else
		g_warning("read_bmp(): Unsupported bitdepth: %d", bitcount);

	ret = gdk_pixmap_new(mainwin->window, w, h, gdk_rgb_get_visual()->depth);

	if (!bmp_gc)
		bmp_gc = gdk_gc_new(mainwin->window);

	gdk_draw_rgb_image(ret, bmp_gc, 0, 0, w, h, GDK_RGB_DITHER_MAX, data, w * 3);

	g_free(data);
	g_free(buffer);

 failure:
	fclose(file);
	return ret;
}


static void read_1b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	gint padding = (4 - ((w + 7) / 8) % 4) & 3;
	gint x, y, i;
	
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < input_end;)
		{
			guint8 byte = *(input++);
			for (i = 0; i < 8 && x < w; i++, x++)
			{
				*output_ptr++ = palette[(byte >> (7 - i)) & 1].rgbRed;
				*output_ptr++ = palette[(byte >> (7 - i)) & 1].rgbGreen;
				*output_ptr++ = palette[(byte >> (7 - i)) & 1].rgbBlue;
			}
		}
		input += padding;
		output_ptr -= w * 6;        /* Back up two scanlines */
	}
}

static void read_4b_rle(guint8 *input, guint32 compr_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette)
{
	gboolean at_end = 0;
	gint j;
	guint16 x = 0, y = 0;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	guint8 *input_ptr = input;
	guint8 *input_end = input + compr_size;
	guint8 *output_end = output + w * h * 3;
	guint8 col, col1 = 0, col2 = 0, num, byte;


	while (!at_end && input_ptr < input_end)
	{
		byte = *(input_ptr++);
		if (byte)
		{
			/* "Encoded mode" */
			num = byte;
			byte = *(input_ptr++);
			col1 = byte & 0xF;
			col2 = (byte >> 4);
			for (j = 0; j < num; j++)
			{
				col = (j & 1) ? col1 : col2;

				if (x >= w)
					break;

				*output_ptr++ = palette[col].rgbRed;
				*output_ptr++ = palette[col].rgbGreen;
				*output_ptr++ = palette[col].rgbBlue;
				x++;
				if (output_ptr > output_end)
					output_ptr = output_end;
			}
		}
		else
		{
			byte = *(input_ptr++);
			switch (byte)
			{
				case 0:
					/* End of line */
					x = 0;
					y++;
					output_ptr = output + ((h - y - 1) * w * 3) + (x * 3);
					if (output_ptr > output_end)
						output_ptr = output_end;
					break;
				case 1:
					/* End of bitmap */
					at_end = 1;
					break;
				case 2:
					/* Delta */
					x += *(input_ptr++);
					y += *(input_ptr++);
					output_ptr = output + ((h - y - 1) * w * 3) + (x * 3);
					if (output_ptr > output_end)
						output_ptr = output_end;
					break;
				default:
					/*
					 * "Absolute mode"
					 *  non RLE encoded
					 */
					num = byte;
					for (j = 0; j < num; j++)
					{
						if ((j & 1) == 0)
						{
							byte = *(input_ptr++);
							col1 = byte & 0xF;
							col2 = (byte >> 4);
						}
						col = (j & 1) ? col1 : col2;

						if (x >= w)
						{
							input_ptr += (num - j) / 2;
							break;
						}

						*output_ptr++ = palette[col].rgbRed;
						*output_ptr++ = palette[col].rgbGreen;
						*output_ptr++ = palette[col].rgbBlue;

						x++;

						if (output_ptr > output_end)
							output_ptr = output_end;
					}

					/* Skip padding */
					if ((num & 3) == 1 || (num & 3) == 2)
						input_ptr++;
					break;
			}
		}
	}
}

static void read_4b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	gint padding = ((((w + 7) / 8) * 8) - w) / 2;
	guint x, y;
	
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < input_end; x+=2)
		{
			guint8 byte = *(input++);

			*output_ptr++ = palette[byte >> 4].rgbRed;
			*output_ptr++ = palette[byte >> 4].rgbGreen;
			*output_ptr++ = palette[byte >> 4].rgbBlue;

			if (x + 1 == w)
				break;
			
			*output_ptr++ = palette[byte & 0xF].rgbRed;
			*output_ptr++ = palette[byte & 0xF].rgbGreen;
			*output_ptr++ = palette[byte & 0xF].rgbBlue;
		}
		input += padding;
		output_ptr -= w * 6;        /* Back up two scanlines */
	}
}


static void read_8b_rle(guint8 *input, guint32 compr_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette)
{
	gboolean at_end = 0;
	gint j;
	guint16 x = 0, y = 0;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	guint8 *input_ptr = input;
	guint8 *input_end = input + compr_size;
	guint8 *output_end = output + w * h * 3;
	guint8 num, byte;

	while (!at_end && input_ptr < input_end)
	{
		byte = *(input_ptr++);
		if (byte)
		{
			/* "Encoded mode" */
			num = byte;
			byte = *(input_ptr++);
			for (j = 0; j < num; j++)
			{
				if (x >= w)
					break;
				
				*output_ptr++ = palette[byte].rgbRed;
				*output_ptr++ = palette[byte].rgbGreen;
				*output_ptr++ = palette[byte].rgbBlue;
				x++;
				if (output_ptr > output_end)
					output_ptr = output_end;

			}

		}
		else
		{
			byte = *(input_ptr++);
			switch (byte)
			{
				/* End of line */
				case 0:
					x = 0;
					y++;
					output_ptr = output + ((h - y - 1) * w * 3) + (x * 3);
					if (output_ptr > output_end)
						output_ptr = output_end;
					break;
				case 1:
					/* End of bitmap */
					at_end = 1;
					break;
				case 2:
					/* Delta */
					x += *(input_ptr++);
					y += *(input_ptr++);
					output_ptr = output + ((h - y - 1) * w * 3) + (x * 3);
					if (output_ptr > output_end)
						output_ptr = output_end;
					break;
				default:
					/*
					 * "Absolute mode"
					 *  non RLE encoded
					 */
					num = byte;
					for (j = 0; j < num; j++)
					{
						byte = *(input_ptr++);
						
						if (x >= w)
						{
							input_ptr += num - j;
							break;
						}
						
						*output_ptr++ = palette[byte].rgbRed;
						*output_ptr++ = palette[byte].rgbGreen;
						*output_ptr++ = palette[byte].rgbBlue;
						x++;
						
						if (output_ptr > output_end)
							output_ptr = output_end;

					}

					/* Skip padding */
					if (num & 1)
						input_ptr++;
					break;
			}
		}
	}
}

static void read_8b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h, struct rgb_quad *palette)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	gint padding = (((w + 3) / 4) * 4) - w;
	guint8 byte;
	guint x, y;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < input_end; x++)
		{
			byte = *(input++);
			*output_ptr++ = palette[byte].rgbRed;
			*output_ptr++ = palette[byte].rgbGreen;
			*output_ptr++ = palette[byte].rgbBlue;
		}
		output_ptr -= w * 6;
		input += padding;
	}
}


static void read_16b_rgb(guint8 *input, int input_size, guint8 *output, guint32 w, guint32 h, struct bitfieldmask *mask)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	int padding = (4 - ((w * 2) % 4)) & 3;
	guint16 rgb16;
	guint x, y;
	int rsr, rsl, gsr, gsl, bsr, bsl;

	if (mask->r == 0)
		mask->r = 0x7C00;
	if (mask->g == 0)
		mask->g = 0x03E0;
	if (mask->b == 0)
		mask->b = 0x001F;
	get_shift(mask->r, &rsr, &rsl);
	get_shift(mask->g, &gsr, &gsl);
	get_shift(mask->b, &bsr, &bsl);

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < (input_end - 1); x++)
		{
			rgb16 = ((*(input + 1)) << 8) | (*input);
			input += 2;
			*output_ptr++ = ((rgb16 & mask->r) >> rsr) << rsl;
			*output_ptr++ = ((rgb16 & mask->g) >> gsr) << gsl;
			*output_ptr++ = ((rgb16 & mask->b) >> bsr) << bsl;
		}
		output_ptr -= w * 6; /* Back up two scanlines */
		input += padding;
	}
}

static void read_24b_rgb(guint8 *input, gint input_size, guint8 *output, guint32 w, guint32 h)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	gint padding = (4 - ((w * 3) % 4)) & 3;
	guint8 r, g, b;
	guint x, y;
	
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < (input_end - 2); x++)
		{
			b = *(input++);
			g = *(input++);
			r = *(input++);
			*output_ptr++ = r;
			*output_ptr++ = g;
			*output_ptr++ = b;
		}
		output_ptr -= w * 6;
		input += padding;
	}
}

static void read_32b_rgb(guint8 *input, int input_size, guint8 *output, guint32 w, guint32 h, struct bitfieldmask *mask)
{
	guint8 *input_end = input + input_size;
	guint8 *output_ptr = output + ((h - 1) * w * 3);
	guint x, y;
	int rsr, rsl, gsr, gsl, bsr, bsl;

	if (mask->r == 0)
		mask->r = 0x00FF0000;
	if (mask->g == 0)
		mask->g = 0x0000FF00;
	if (mask->b == 0)
		mask->b = 0x000000FF;
	get_shift(mask->r, &rsr, &rsl);
	get_shift(mask->g, &gsr, &gsl);
	get_shift(mask->b, &bsr, &bsl);

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w && input < (input_end - 3); x++)
		{
			guint32 rgb = *(input + 3) << 24 | *(input + 2) << 16 |
				*(input + 1) << 8 | *input;
			input += 4;
			*output_ptr++ = ((rgb & mask->r) >> rsr) << rsl;
			*output_ptr++ = ((rgb & mask->g) >> gsr) << gsl;
			*output_ptr++ = ((rgb & mask->b) >> bsr) << bsl;
		}
		output_ptr -= w * 6;
	}
}

