
/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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

static gint svis_redraw_delays[] = {1, 2, 4, 8};
/* FIXME: Are the svis_scope_colors correct? */
static guint8 svis_scope_colors[] = {20, 19, 18, 19, 20};
static guint8 svis_vu_normal_colors[] = {17, 17, 17, 12, 12, 12, 2, 2};

#define DRAW_DS_PIXEL(ptr,value) \
	*(ptr) = (value); \
	*((ptr) + 1) = (value); \
	*((ptr) + 76) = (value); \
	*((ptr) + 77) = (value);

#define SVIS_HEIGHT 5
#define SVIS_WIDTH 38

void svis_timeout_func(SVis * svis, guchar * data)
{
	static GTimer *timer = NULL;
	gulong micros = 9999999;
	gboolean falloff = FALSE;
	gint i;

	if (!timer)
	{
		timer = g_timer_new();
		g_timer_start(timer);
	}
	else
	{
		g_timer_elapsed(timer, &micros);
		if (micros > 14000)
			g_timer_reset(timer);

	}

	if (cfg.vis_type == INPUT_VIS_ANALYZER)
	{
		if (micros > 14000)
			falloff = TRUE;

		for (i = 0; i < 2; i++)
		{
			if (falloff || data)
			{
				if (data && data[i] > svis->vs_data[i])
					svis->vs_data[i] = data[i];
				else if (falloff)
				{
					if (svis->vs_data[i] >= 2)
						svis->vs_data[i] -= 2;
					else
						svis->vs_data[i] = 0;
				}
			}

		}
	}
	else if (data)
	{
		for (i = 0; i < 75; i++)
			svis->vs_data[i] = data[i];
	}

	if (micros > 14000)
	{
		if (!svis->vs_refresh_delay)
		{
			svis_draw((Widget *) svis);
			svis->vs_refresh_delay = svis_redraw_delays[cfg.vis_refresh];

		}
		svis->vs_refresh_delay--;
	}
}

void svis_draw(Widget * w)
{
	SVis *svis = (SVis *) w;
	gint x, y, h;
	guchar svis_color[24][3];
	guchar rgb_data[SVIS_WIDTH * 2 * SVIS_HEIGHT * 2], *ptr, c;
	guint32 colors[24];
	GdkRgbCmap *cmap;

	GDK_THREADS_ENTER();

	get_skin_viscolor(svis_color);
	for (y = 0; y < 24; y++)
	{
		colors[y] = svis_color[y][0] << 16 | svis_color[y][1] << 8 | svis_color[y][2];
	}
	cmap = gdk_rgb_cmap_new(colors, 24);

	if (!cfg.doublesize)
	{
		memset(rgb_data, 0, SVIS_WIDTH * SVIS_HEIGHT);
		if (cfg.vis_type == VIS_ANALYZER)
		{
			switch (cfg.vu_mode)
			{
				case VU_NORMAL:
					for (y = 0; y < 2; y++)
					{
						ptr = rgb_data + ((y * 3) * 38);
						h = (svis->vs_data[y] * 7) / 37;
						for (x = 0; x < h; x++, ptr += 5)
						{
							c = svis_vu_normal_colors[x];
							*(ptr) = c;
							*(ptr + 1) = c;
							*(ptr + 2) = c;
							*(ptr + 38) = c;
							*(ptr + 39) = c;
							*(ptr + 40) = c;
						}
					}
					break;
				case VU_SMOOTH:
					for (y = 0; y < 2; y++)
					{
						ptr = rgb_data + ((y * 3) * SVIS_WIDTH);
						for (x = 0; x < svis->vs_data[y]; x++, ptr++)
						{
							c = 17 - ((x * 15) / 37);
							*(ptr) = c;
							*(ptr + 38) = c;
						}
					}
					break;
			}
		}
		else if (cfg.vis_type == VIS_SCOPE)
		{
			for (x = 0; x < 38; x++)
			{
				h = svis->vs_data[x << 1] / 3;
				ptr = rgb_data + ((4 - h) * 38) + x;
				*ptr = svis_scope_colors[h];
			}
		}

		gdk_draw_indexed_image(mainwin->window, mainwin_gc, svis->vs_widget.x, svis->vs_widget.y, svis->vs_widget.width, svis->vs_widget.height, GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data, 38, cmap);
	}
	else /* doublesize */
	{
		memset(rgb_data, 0, SVIS_WIDTH * 2 * SVIS_HEIGHT * 2);
		if (cfg.vis_type == VIS_ANALYZER)
		{
			switch (cfg.vu_mode)
			{
				case VU_NORMAL:
					for (y = 0; y < 2; y++)
					{
						ptr = rgb_data + ((y * 3) * 152);
						h = (svis->vs_data[y] * 8) / 37;
						for (x = 0; x < h; x++, ptr += 10)
						{
							c = svis_vu_normal_colors[x];
							DRAW_DS_PIXEL(ptr, c);
							DRAW_DS_PIXEL(ptr + 2, c);
							DRAW_DS_PIXEL(ptr + 4, c);
							DRAW_DS_PIXEL(ptr + 152, c);
							DRAW_DS_PIXEL(ptr + 154, c);
							DRAW_DS_PIXEL(ptr + 156, c);
						}
					}
					break;
				case VU_SMOOTH:
					for (y = 0; y < 2; y++)
					{
						ptr = rgb_data + ((y * 3) * 152);
						for (x = 0; x < svis->vs_data[y]; x++, ptr += 2)
						{
							c = 17 - ((x * 15) / 37);
							DRAW_DS_PIXEL(ptr, c);
							DRAW_DS_PIXEL(ptr + 152, c);
						}
					}
					break;
			}
		}
		else if (cfg.vis_type == VIS_SCOPE)
		{
			for (x = 0; x < 38; x++)
			{
				h = svis->vs_data[x << 1] / 3;
				ptr = rgb_data + ((4 - h) * 152) + (x << 1);
				*ptr = svis_scope_colors[h];
				*(ptr + 1) = svis_scope_colors[h];
				*(ptr + 76) = svis_scope_colors[h];
				*(ptr + 77) = svis_scope_colors[h];
			}
		}

		gdk_draw_indexed_image(mainwin->window, mainwin_gc, svis->vs_widget.x << 1, svis->vs_widget.y << 1, svis->vs_widget.width << 1, svis->vs_widget.height << 1, GDK_RGB_DITHER_NONE, (guchar *) rgb_data, 76, cmap);
	}
	gdk_rgb_cmap_free(cmap);
	GDK_THREADS_LEAVE();
}

void svis_clear_data(SVis * svis)
{
	gint i;

	for (i = 0; i < 75; i++)
	{
		svis->vs_data[i] = (cfg.vis_type == VIS_SCOPE) ? 6 : 0;
	}
}

void svis_clear(SVis * svis)
{
	if (!cfg.doublesize)
		gdk_window_clear_area(mainwin->window, svis->vs_widget.x, svis->vs_widget.y, svis->vs_widget.width, svis->vs_widget.height);
	else
		gdk_window_clear_area(mainwin->window, svis->vs_widget.x << 1, svis->vs_widget.y << 1, svis->vs_widget.width << 1, svis->vs_widget.height << 1);
}

SVis *create_svis(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y)
{
	SVis *svis;

	svis = (SVis *) g_malloc0(sizeof (SVis));
	svis->vs_widget.parent = parent;
	svis->vs_widget.gc = gc;
	svis->vs_widget.x = x;
	svis->vs_widget.y = y;
	svis->vs_widget.width = SVIS_WIDTH;
	svis->vs_widget.height = SVIS_HEIGHT;
	svis->vs_widget.visible = 1;

	add_widget(wlist, svis);
	return svis;
}
