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
 *  w
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"

#include <gtk/gtk.h>
#include <string.h>
#include "xmms/plugin.h"
#include "libxmms/util.h"
#include "libxmms/configfile.h"
#include "xmms_logo.xpm"
#include "blur_scope.h"
#include "xmms/i18n.h"

static GtkWidget *window = NULL,*area;
static GdkPixmap *bg_pixmap = NULL;
static gboolean config_read = FALSE;

static void bscope_init(void);
static void bscope_cleanup(void);
static void bscope_playback_stop(void);
static void bscope_render_pcm(gint16 data[2][512]);

BlurScopeConfig bscope_cfg;

VisPlugin bscope_vp =
{
	NULL,
	NULL,
	0, /* XMMS Session ID, filled in by XMMS */
	NULL, /* description */
	1, /* Number of PCM channels wanted */
	0, /* Number of freq channels wanted */
	bscope_init, /* init */
	bscope_cleanup, /* cleanup */
	NULL, /* about */
	bscope_configure, /* configure */
	NULL, /* disable_plugin */
	NULL, /* playback_start */
	bscope_playback_stop, /* playback_stop */
	bscope_render_pcm, /* render_pcm */
	NULL  /* render_freq */
};

VisPlugin *get_vplugin_info(void)
{
	bscope_vp.description = g_strdup_printf(_("Blur Scope %s"), VERSION);
	return &bscope_vp;
}

#define WIDTH 256 
#define HEIGHT 128
#define min(x,y) ((x)<(y)?(x):(y))
#define BPL	((WIDTH + 2))

static guchar rgb_buf[(WIDTH + 2) * (HEIGHT + 2)];
static GdkRgbCmap *cmap = NULL; 
	
static void inline draw_pixel_8(guchar *buffer,gint x, gint y, guchar c)
{
	buffer[((y + 1) * BPL) + (x + 1)] = c;
}


void bscope_read_config(void)
{
	ConfigFile *cfg;
	gchar *filename;

	if(!config_read)
	{
		bscope_cfg.color = 0xFF3F7F;
		filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
		cfg = xmms_cfg_open_file(filename);
		
		if (cfg)
		{
			xmms_cfg_read_int(cfg, "BlurScope", "color", &bscope_cfg.color);
			xmms_cfg_free(cfg);
		}
		g_free(filename);
		config_read = TRUE;
	}
}


#ifndef I386_ASSEM
void bscope_blur_8(guchar *ptr,gint w, gint h, gint bpl)
{
	register guint i,sum;
	register guchar *iptr;
	
	iptr = ptr + bpl + 1;
	i = bpl * h;
	while(i--)
	{
		sum = (iptr[-bpl] + iptr[-1] + iptr[1] + iptr[bpl]) >> 2;
		if(sum > 2)
			sum -= 2;
		*(iptr++) = sum;
	}
	
	
}
#else
extern void bscope_blur_8(guchar *ptr,gint w, gint h, gint bpl);
#endif

void generate_cmap(void)
{
	guint32 colors[256],i,red,blue,green;
	if(window)
	{
		red = (guint32)(bscope_cfg.color / 0x10000);
		green = (guint32)((bscope_cfg.color % 0x10000)/0x100);
		blue = (guint32)(bscope_cfg.color % 0x100);
		for(i = 255; i > 0; i--)
		{
			colors[i] = (((guint32)(i*red/256) << 16) | ((guint32)(i*green/256) << 8) | ((guint32)(i*blue/256)));
		}
		colors[0]=0;
		if(cmap)
		{
			gdk_rgb_cmap_free(cmap);
		}
		cmap = gdk_rgb_cmap_new(colors,256);
	}
}

static void bscope_destroy_cb(GtkWidget *w,gpointer data)
{
	bscope_vp.disable_plugin(&bscope_vp);
}

static void bscope_init(void)
{
	if(window)
		return;
	bscope_read_config();

	window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(window),_("Blur scope"));
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
	gtk_widget_realize(window);
	bg_pixmap = gdk_pixmap_create_from_xpm_d(window->window,NULL,NULL,bscope_xmms_logo_xpm);
	gdk_window_set_back_pixmap(window->window,bg_pixmap,0);
	gtk_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(bscope_destroy_cb),NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &window);
	gtk_widget_set_usize(window, WIDTH, HEIGHT);
	
	area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window),area);
	gtk_widget_realize(area);
	gdk_window_set_back_pixmap(area->window,bg_pixmap,0);
	generate_cmap();
	memset(rgb_buf,0,(WIDTH + 2) * (HEIGHT + 2));
		
	gtk_widget_show(area);
	gtk_widget_show(window);
	gdk_window_clear(window->window);
	gdk_window_clear(area->window);
}

static void bscope_cleanup(void)
{
	if(window)
		gtk_widget_destroy(window);
	if(bg_pixmap)
	{
		gdk_pixmap_unref(bg_pixmap);
		bg_pixmap = NULL;
	}
	if(cmap)
	{
		gdk_rgb_cmap_free(cmap);
		cmap = NULL;
	}
}

static void bscope_playback_stop(void)
{
	if(GTK_WIDGET_REALIZED(area))
		gdk_window_clear(area->window);
}

static inline void draw_vert_line(guchar *buffer, gint x, gint y1, gint y2)
{
	int y;
	if(y1 < y2)
	{
		for(y = y1; y <= y2; y++)
			draw_pixel_8(buffer,x,y,0xFF);
	}
	else if(y2 < y1)
	{
		for(y = y2; y <= y1; y++)
			draw_pixel_8(buffer,x,y,0xFF);
	}
	else
		draw_pixel_8(buffer,x,y1,0xFF);
}

static void bscope_render_pcm(gint16 data[2][512])
{
	gint i,y, prev_y;
	
	if(!window)
		return;
	bscope_blur_8(rgb_buf, WIDTH, HEIGHT, BPL);
	prev_y = y = (HEIGHT / 2) + (data[0][0] >> 9);
	for(i = 0; i < WIDTH; i++)
	{
		y = (HEIGHT / 2) + (data[0][i >> 1] >> 9);
		if(y < 0)
			y = 0;
		if(y >= HEIGHT)
			y = HEIGHT - 1;
		draw_vert_line(rgb_buf,i,prev_y,y);
		prev_y = y;
	}
				
	GDK_THREADS_ENTER();
	gdk_draw_indexed_image(area->window,area->style->white_gc,0,0,WIDTH,HEIGHT,GDK_RGB_DITHER_NONE,rgb_buf + BPL + 1,(WIDTH + 2),cmap);
	GDK_THREADS_LEAVE();
	return;			
}
