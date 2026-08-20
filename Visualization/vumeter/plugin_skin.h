#ifndef __PLUGIN_SKIN_H
#define __PLUGIN_SKIN_H

#include "plugin_image.h"

typedef struct {
	char	pathnum,
		dirname[256];
} plugin_sl_el;


typedef struct {
	char		skin_name[255];

	gint		width,
			height,
			pathnum,
			exit_button_pos[2][2],
			conf_button_pos[2][2];

	GArray		*modules;

	vumeter_image	*img_background,
			*img_titlebar_on,
			*img_titlebar_off;

} vumeter_skin;


typedef struct {
	gint		type,
			enabled,
			channel,
			layer,
			width,
			radius,
			position[2];

	guint32		color; /* packed 0x00RRGGBB */

	gfloat		db_min,
			db_max,
			angle_min,
			angle_max,
			angle_range;

	vumeter_image	*on_img,
			*off_img;
} vumeter_module;

int vumeter_scan_skin_dirs(void);
int vumeter_load_skin(int, char *);
void vumeter_deinit_skin(vumeter_skin *);

#endif
