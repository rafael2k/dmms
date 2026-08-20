#include <glib.h>
#include <stdio.h>
#include <png.h>

#include "plugin_image.h"

vumeter_image *vumeter_image_load(const char *filename)
{
	FILE		*fp;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_uint_32	width, height;
	int		bit_depth, color_type;
	vumeter_image	*img;
	png_bytep	*row_pointers;
	png_uint_32	y;

	fp = fopen(filename, "rb");
	if(fp == NULL)
		return(NULL);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		fclose(fp);
		return(NULL);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return(NULL);
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return(NULL);
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	/* Normalize every source format to 8-bit RGBA */
	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	if(bit_depth == 16)
		png_set_strip_16(png_ptr);

	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_PALETTE ||
	   color_type == PNG_COLOR_TYPE_GRAY)
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

	png_read_update_info(png_ptr, info_ptr);

	img = g_new0(vumeter_image, 1);
	img->width  = width;
	img->height = height;
	img->pixels = g_malloc(width * height * 4);

	row_pointers = g_new(png_bytep, height);
	for(y = 0; y < height; y++)
		row_pointers[y] = img->pixels + (y * width * 4);

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, NULL);

	g_free(row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);

	return(img);
}

void vumeter_image_free(vumeter_image *img)
{
	if(img == NULL)
		return;

	g_free(img->pixels);
	g_free(img);
}

void vumeter_image_composite(	guchar *dst_rgb, gint dst_w, gint dst_h, gint dst_rowstride,
				vumeter_image *src, gint x, gint y)
{
	gint	sx, sy, dx, dy;
	guchar	*srow, *drow, *spix, *dpix;
	guchar	sa;

	if(src == NULL)
		return;

	for(sy = 0; sy < src->height; sy++)
	{
		dy = y + sy;
		if(dy < 0 || dy >= dst_h)
			continue;

		srow = src->pixels + (sy * src->width * 4);
		drow = dst_rgb + (dy * dst_rowstride);

		for(sx = 0; sx < src->width; sx++)
		{
			dx = x + sx;
			if(dx < 0 || dx >= dst_w)
				continue;

			spix = srow + (sx * 4);
			sa   = spix[3];

			if(sa == 0)
				continue;

			dpix = drow + (dx * 3);

			if(sa == 255)
			{
				dpix[0] = spix[0];
				dpix[1] = spix[1];
				dpix[2] = spix[2];
			} else {
				dpix[0] = ((spix[0] * sa) + (dpix[0] * (255 - sa))) / 255;
				dpix[1] = ((spix[1] * sa) + (dpix[1] * (255 - sa))) / 255;
				dpix[2] = ((spix[2] * sa) + (dpix[2] * (255 - sa))) / 255;
			}
		}
	}
}
