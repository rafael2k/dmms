#ifndef __PLUGIN_IMAGE_H
#define __PLUGIN_IMAGE_H

/* Minimal RGBA image loader/compositor, used in place of GdkPixbuf
 * (which does not exist for GTK+1.2). Decoding is done with libpng
 * directly; compositing is plain software alpha blending onto an
 * RGB buffer that is later blitted with gdk_draw_rgb_image(). */

typedef struct
{
	gint	width,
		height;

	guchar	*pixels; /* RGBA, row-major, 4 bytes/pixel */
} vumeter_image;

vumeter_image *vumeter_image_load(const char *filename);
void vumeter_image_free(vumeter_image *img);

void vumeter_image_composite(	guchar *dst_rgb, gint dst_w, gint dst_h, gint dst_rowstride,
				vumeter_image *src, gint x, gint y);

#endif
