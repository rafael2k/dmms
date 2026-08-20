#ifndef __PLUGIN_MAIN_H
#define __PLUGIN_MAIN_H

#define MAX_INSTANCES	10

#ifdef DEBUG_OUTPUT
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif

typedef struct {
	gint		xpos,
			ypos,
			skin_num,
			width,
			height,
			slot_num;

	GtkWidget	*win;
	GdkPixmap	*pixmap;
	GdkGC		*pen;

	guchar		*bg_buf;	/* background + titlebar, RGB raster */
	guchar		*frame_buf;	/* bg_buf + LED overlays for current frame */

} vumeter_window;

GtkWidget *vumeter_create_window(gint, gint);
void vumeter_change_window_skin(gint,gint,gchar *);
void vumeter_redraw_window(vumeter_window *);

#endif
