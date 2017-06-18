/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen
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
#include "libxmms/configfile.h"

GtkWidget *equalizerwin;

static GtkWidget *equalizerwin_load_window = NULL;
static GtkWidget *equalizerwin_load_auto_window = NULL;
static GtkWidget *equalizerwin_save_window = NULL, *equalizerwin_save_entry;
static GtkWidget *equalizerwin_save_auto_window = NULL, *equalizerwin_save_auto_entry;
static GtkWidget *equalizerwin_delete_window = NULL;
static GtkWidget *equalizerwin_delete_auto_window = NULL;
static GtkWidget *equalizerwin_configure_window = NULL;

static GtkWidget *eqconfwin_options_eqdf_entry, *eqconfwin_options_eqef_entry;

GdkPixmap *equalizerwin_bg, *equalizerwin_bg_dblsize;
GdkGC *equalizerwin_gc;

GList *equalizerwin_wlist = NULL;

static GtkAccelGroup *equalizerwin_accel;

static TButton *equalizerwin_on, *equalizerwin_auto;
extern TButton *mainwin_eq;
static PButton *equalizerwin_presets, *equalizerwin_shade;
PButton *equalizerwin_close;
static EqGraph *equalizerwin_graph;
static EqSlider *equalizerwin_preamp, *equalizerwin_bands[10];
static HSlider *equalizerwin_volume, *equalizerwin_balance;

static GtkItemFactory *equalizerwin_presets_menu;

gboolean equalizerwin_focus = FALSE;

typedef struct
{
	gchar *name;
	gfloat preamp, bands[10];
}
EqualizerPreset;

static GList *equalizer_presets = NULL, *equalizer_auto_presets = NULL;

void equalizerwin_presets_menu_cb(gpointer cb_data, guint action, GtkWidget * w);
GtkWidget * equalizerwin_create_conf_window(void);

enum
{
	EQUALIZER_PRESETS_LOAD_PRESET, EQUALIZER_PRESETS_LOAD_AUTOPRESET,
	EQUALIZER_PRESETS_LOAD_DEFAULT,	EQUALIZER_PRESETS_LOAD_ZERO,
	EQUALIZER_PRESETS_LOAD_FROM_FILE, EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE,
	EQUALIZER_PRESETS_IMPORT_WINAMPFILE,
	EQUALIZER_PRESETS_SAVE_PRESET, EQUALIZER_PRESETS_SAVE_AUTOPRESET,
	EQUALIZER_PRESETS_SAVE_DEFAULT,	EQUALIZER_PRESETS_SAVE_TO_FILE,
	EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE, EQUALIZER_PRESETS_DELETE_PRESET,
	EQUALIZER_PRESETS_DELETE_AUTOPRESET, EQUALIZER_PRESETS_CONFIGURE
};

GtkItemFactoryEntry equalizerwin_presets_menu_entries[] =
{
	{N_("/Load"), NULL, NULL, 0, "<Branch>"},
	{N_("/Load/Preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_PRESET, "<Item>"},
	{N_("/Load/Auto-load preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_AUTOPRESET, "<Item>"},
	{N_("/Load/Default"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_DEFAULT, "<Item>"},
	{N_("/Load/-"), NULL, NULL, 0, "<Separator>"},
	{N_("/Load/Zero"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_ZERO, "<Item>"},
	{N_("/Load/-"), NULL, NULL, 0, "<Separator>"},
	{N_("/Load/From file"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_FROM_FILE, "<Item>"},
	{N_("/Load/From WinAMP EQF file"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE, "<Item>"},
	{N_("/Import"), NULL, NULL, 0, "<Branch>"},
	{N_("/Import/WinAMP Presets"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_IMPORT_WINAMPFILE, "<Item>"},
	{N_("/Save"), NULL, NULL, 0, "<Branch>"},
	{N_("/Save/Preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_SAVE_PRESET, "<Item>"},
	{N_("/Save/Auto-load preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_SAVE_AUTOPRESET, "<Item>"},
	{N_("/Save/Default"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_SAVE_DEFAULT, "<Item>"},
	{N_("/Save/-"), NULL, NULL, 0, "<Separator>"},
	{N_("/Save/To file"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_SAVE_TO_FILE, "<Item>"},
	{N_("/Save/To WinAMP EQF file"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE, "<Item>"},
	{N_("/Delete"), NULL, NULL, 0, "<Branch>"},
	{N_("/Delete/Preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_DELETE_PRESET, "<Item>"},
	{N_("/Delete/Auto-load preset"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_DELETE_AUTOPRESET, "<Item>"},
	{N_("/Configure Equalizer"), NULL, equalizerwin_presets_menu_cb, EQUALIZER_PRESETS_CONFIGURE, "<Item>"},
};

static gint equalizerwin_presets_menu_entries_num = 
	sizeof(equalizerwin_presets_menu_entries) / 
	sizeof(equalizerwin_presets_menu_entries[0]);

void equalizerwin_set_shape_mask(void)
{
	if (cfg.show_wm_decorations)
		return;

	gtk_widget_shape_combine_mask(equalizerwin, skin_get_mask(SKIN_MASK_EQ, EQUALIZER_DOUBLESIZE, cfg.equalizer_shaded), 0, 0);
}

void equalizerwin_set_doublesize(gboolean ds)
{
	gint height;
	
	if(cfg.equalizer_shaded)
		height = 14;
	else
		height = 116;

	equalizerwin_set_shape_mask();

	if (ds)
	{
		dock_resize(dock_window_list, equalizerwin, 550, height * 2);
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg_dblsize, 0);
	}
	else
	{
		dock_resize(dock_window_list, equalizerwin, 275, height);
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg, 0);
	}
	draw_equalizer_window(TRUE);
}

void equalizerwin_set_shade_menu_cb(gboolean shaded)
{
	cfg.equalizer_shaded = shaded;

	equalizerwin_set_shape_mask();
	
	if (shaded)
	{
		dock_shade(dock_window_list, equalizerwin, 14 * (EQUALIZER_DOUBLESIZE + 1));
		pbutton_set_button_data(equalizerwin_shade, -1, 3, -1, 47);
		pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQ_EX);
		pbutton_set_button_data(equalizerwin_close, 11, 38, 11, 47);
		pbutton_set_skin_index(equalizerwin_close, SKIN_EQ_EX);
		show_widget(equalizerwin_volume);
		show_widget(equalizerwin_balance);
	}
	else
	{
		dock_shade(dock_window_list, equalizerwin, 116 * (EQUALIZER_DOUBLESIZE + 1));
		pbutton_set_button_data(equalizerwin_shade, -1, 137, -1, 38);
		pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQMAIN);
		pbutton_set_button_data(equalizerwin_close, 0, 116, 0, 125);
		pbutton_set_skin_index(equalizerwin_close, SKIN_EQMAIN);
		hide_widget(equalizerwin_volume);
		hide_widget(equalizerwin_balance);
	}
	
	draw_equalizer_window(TRUE);
}

void equalizerwin_shade_toggle(void)
{
	equalizerwin_set_shade(!cfg.equalizer_shaded);
}

void equalizerwin_set_shade(gboolean shaded)
{
	GtkWidget *widget;
        widget = gtk_item_factory_get_widget(mainwin_options_menu,
					     "/Equalizer WindowShade Mode");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), shaded);
}

void equalizerwin_raise(void)
{
	if (cfg.equalizer_visible)
		gdk_window_raise(equalizerwin->window);
}

void equalizerwin_eq_changed(void)
{
	int i;

	cfg.equalizer_preamp = eqslider_get_position(equalizerwin_preamp);
	for (i = 0; i < 10; i++)
		cfg.equalizer_bands[i] = eqslider_get_position(equalizerwin_bands[i]);
	input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp, cfg.equalizer_bands);
	draw_widget(equalizerwin_graph);
}

void equalizerwin_on_pushed(gboolean toggled)
{
	cfg.equalizer_active = toggled;
	equalizerwin_eq_changed();
}

void equalizerwin_presets_pushed(void)
{
	GdkModifierType modmask;
	gint x, y;

	gdk_window_get_pointer(NULL, &x, &y, &modmask);
	util_item_factory_popup(equalizerwin_presets_menu, x, y, 1, GDK_CURRENT_TIME);
}

void equalizerwin_auto_pushed(gboolean toggled)
{
	cfg.equalizer_autoload = toggled;
}

void draw_equalizer_window(gboolean force)
{
	GdkImage *img, *img2;
	GList *wl;
	Widget *w;
	gboolean redraw;

	lock_widget_list(equalizerwin_wlist);
	if (force)
	{
		skin_draw_pixmap(equalizerwin_bg, equalizerwin_gc, SKIN_EQMAIN,
				 0, 0, 0, 0, 275, 116);
		if (equalizerwin_focus || !cfg.dim_titlebar)
		{
			if (!cfg.equalizer_shaded)
				skin_draw_pixmap(equalizerwin_bg, equalizerwin_gc,
						 SKIN_EQMAIN, 0, 134, 0, 0, 275, 14);
			else
				skin_draw_pixmap(equalizerwin_bg, equalizerwin_gc,
						 SKIN_EQ_EX, 0, 0, 0, 0, 275, 14);
		}
		else
		{
			if(!cfg.equalizer_shaded)
				skin_draw_pixmap(equalizerwin_bg, equalizerwin_gc,
						 SKIN_EQMAIN, 0, 149, 0, 0, 275, 14);
			else
				skin_draw_pixmap(equalizerwin_bg, equalizerwin_gc,
						 SKIN_EQ_EX, 0, 15, 0, 0, 275, 14);

		}
		draw_widget_list(equalizerwin_wlist, &redraw, TRUE);
	}
	else
		draw_widget_list(equalizerwin_wlist, &redraw, FALSE);

	if (force || redraw)
	{
		if (cfg.doublesize && cfg.eq_doublesize_linked)
		{
			if (force)
			{
				img = gdk_image_get(equalizerwin_bg, 0, 0, 275, 116);
				img2 = create_dblsize_image(img);
				gdk_draw_image(equalizerwin_bg_dblsize, equalizerwin_gc, img2, 0, 0, 0, 0, 550, 232);
				gdk_image_destroy(img2);
				gdk_image_destroy(img);
			}
			else
			{
				wl = equalizerwin_wlist;
				while (wl)
				{
					w = (Widget *) wl->data;
					if (w->redraw && w->visible)
					{
						img = gdk_image_get(equalizerwin_bg, w->x, w->y, w->width, w->height);
						img2 = create_dblsize_image(img);
						gdk_draw_image(equalizerwin_bg_dblsize, equalizerwin_gc, img2, 0, 0, w->x << 1, w->y << 1, w->width << 1, w->height << 1);
						gdk_image_destroy(img2);
						gdk_image_destroy(img);
						w->redraw = FALSE;
					}
					wl = wl->next;
				}
			}
		}
		else
			clear_widget_list_redraw(equalizerwin_wlist);
		gdk_window_clear(equalizerwin->window);
		gdk_flush();
	}
	unlock_widget_list(equalizerwin_wlist);
}

static gboolean inside_sensitive_widgets(gint x, gint y)
{
	return (inside_widget(x, y, equalizerwin_on) ||
		inside_widget(x, y, equalizerwin_auto) ||
		inside_widget(x, y, equalizerwin_presets) ||
		inside_widget(x, y, equalizerwin_close) ||
		inside_widget(x, y, equalizerwin_shade) ||
		inside_widget(x, y, equalizerwin_preamp) ||
		inside_widget(x, y, equalizerwin_bands[0]) ||
		inside_widget(x, y, equalizerwin_bands[1]) ||
		inside_widget(x, y, equalizerwin_bands[2]) ||
		inside_widget(x, y, equalizerwin_bands[3]) ||
		inside_widget(x, y, equalizerwin_bands[4]) ||
		inside_widget(x, y, equalizerwin_bands[5]) ||
		inside_widget(x, y, equalizerwin_bands[6]) ||
		inside_widget(x, y, equalizerwin_bands[7]) ||
		inside_widget(x, y, equalizerwin_bands[8]) ||
		inside_widget(x, y, equalizerwin_bands[9]) ||
		inside_widget(x, y, equalizerwin_volume) ||
		inside_widget(x, y, equalizerwin_balance));
}

void equalizerwin_press(GtkWidget * widget, GdkEventButton * event, gpointer callback_data)
{
	gint mx, my;
	gboolean grab = TRUE;

	mx = event->x;
	my = event->y;
	if (cfg.doublesize && cfg.eq_doublesize_linked)
	{
		event->x /= 2;
		event->y /= 2;
	}

	if (event->button == 1 && event->type == GDK_BUTTON_PRESS &&
	    ((cfg.easy_move || cfg.equalizer_shaded || event->y < 14) &&
	     !inside_sensitive_widgets(event->x, event->y)))
	{
		if (0 && hint_move_resize_available())
		{
			hint_move_resize(equalizerwin, event->x_root,
					 event->y_root, TRUE);
			grab = FALSE;
		}
		else
		{
			equalizerwin_raise();
			dock_move_press(dock_window_list, equalizerwin, event, FALSE);
		}
	}
	else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS && event->y < 14)
	{
		equalizerwin_set_shade(!cfg.equalizer_shaded);
		if(dock_is_moving(equalizerwin))
			dock_move_release(equalizerwin);
	}
	else if (event->button == 3 &&
		 !(inside_widget(event->x, event->y, equalizerwin_on) ||
		   inside_widget(event->x, event->y, equalizerwin_auto)))
	{
		/*
		 * Pop up the main menu a few pixels down to avoid
		 * anything to be selected initially.
		 */
		util_item_factory_popup(mainwin_general_menu, event->x_root, event->y_root + 2, 3, event->time);
		grab = FALSE;
	}
	else
	{
		handle_press_cb(equalizerwin_wlist, widget, event);
		draw_equalizer_window(FALSE);
	}
	if (grab)
		gdk_pointer_grab(equalizerwin->window, FALSE,
				 GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 GDK_NONE, GDK_NONE, GDK_CURRENT_TIME);
}

void equalizerwin_motion(GtkWidget * widget, GdkEventMotion * event, gpointer callback_data)
{
	XEvent ev;

	if (cfg.doublesize && cfg.eq_doublesize_linked)
	{
		event->x /= 2;
		event->y /= 2;
	}
	if (dock_is_moving(equalizerwin))
	{
		dock_move_motion(equalizerwin, event);
	}
	else
	{
		handle_motion_cb(equalizerwin_wlist, widget, event);
		draw_main_window(FALSE);
	}
	gdk_flush();
	while (XCheckMaskEvent(GDK_DISPLAY(), ButtonMotionMask, &ev)) ;

}

void equalizerwin_release(GtkWidget * widget, GdkEventButton * event, gpointer callback_data)
{
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	gdk_flush();
	if (dock_is_moving(equalizerwin))
	{
		dock_move_release(equalizerwin);
	}
	else
	{
		handle_release_cb(equalizerwin_wlist, widget, event);
		draw_equalizer_window(FALSE);
	}
}

void equalizerwin_focus_in(GtkWidget * widget, GdkEvent * event, gpointer callback_data)
{
	equalizerwin_close->pb_allow_draw = TRUE;
	equalizerwin_shade->pb_allow_draw = TRUE;
	equalizerwin_focus = TRUE;
	draw_equalizer_window(TRUE);
}

void equalizerwin_focus_out(GtkWidget * widget, GdkEventButton * event, gpointer callback_data)
{
	equalizerwin_close->pb_allow_draw = FALSE;
	equalizerwin_shade->pb_allow_draw = FALSE;
	equalizerwin_focus = FALSE;
	draw_equalizer_window(TRUE);
}

#define EQ_SLIDER_DOWN(slider)							\
do {										\
	eqslider_set_position(slider, eqslider_get_position(slider) - 1.6f);	\
	equalizerwin_eq_changed();						\
} while (0)
	
#define EQ_SLIDER_UP(slider)							\
do {										\
	eqslider_set_position(slider, eqslider_get_position(slider) + 1.6f);	\
	equalizerwin_eq_changed();						\
} while (0)

gboolean equalizerwin_keypress(GtkWidget * w, GdkEventKey * event, gpointer data)
{
	switch (event->keyval)
	{
		case GDK_Left:
		case GDK_KP_Left:
			if (cfg.equalizer_shaded)
				mainwin_set_balance_diff(-4);
			else
				gtk_widget_event(mainwin, (GdkEvent*) event);
			break;
		case GDK_Right:
		case GDK_KP_Right:
			if (cfg.equalizer_shaded)
				mainwin_set_balance_diff(4);
			else
				gtk_widget_event(mainwin, (GdkEvent*) event);
			break;
		case GDK_quoteleft:
			EQ_SLIDER_UP(equalizerwin_preamp);
			break;
		case GDK_Tab:
			EQ_SLIDER_DOWN(equalizerwin_preamp);
			break;
		case GDK_1:
			EQ_SLIDER_UP(equalizerwin_bands[0]);
			break;
		case GDK_q:
			EQ_SLIDER_DOWN(equalizerwin_bands[0]);
			break;
		case GDK_2:
			EQ_SLIDER_UP(equalizerwin_bands[1]);
			break;
		case GDK_w:
			EQ_SLIDER_DOWN(equalizerwin_bands[1]);
			break;
		case GDK_3:
			EQ_SLIDER_UP(equalizerwin_bands[2]);
			break;
		case GDK_e:
			EQ_SLIDER_DOWN(equalizerwin_bands[2]);
			break;
		case GDK_4:
			EQ_SLIDER_UP(equalizerwin_bands[3]);
			break;
		case GDK_r:
			EQ_SLIDER_DOWN(equalizerwin_bands[3]);
			break;
		case GDK_5:
			EQ_SLIDER_UP(equalizerwin_bands[4]);
			break;
		case GDK_t:
			EQ_SLIDER_DOWN(equalizerwin_bands[4]);
			break;
		case GDK_6:
			EQ_SLIDER_UP(equalizerwin_bands[5]);
			break;
		case GDK_y:
			EQ_SLIDER_DOWN(equalizerwin_bands[5]);
			break;
		case GDK_7:
			EQ_SLIDER_UP(equalizerwin_bands[6]);
			break;
		case GDK_u:
			EQ_SLIDER_DOWN(equalizerwin_bands[6]);
			break;
		case GDK_8:
			EQ_SLIDER_UP(equalizerwin_bands[7]);
			break;
		case GDK_i:
			EQ_SLIDER_DOWN(equalizerwin_bands[7]);
			break;
		case GDK_9:
			EQ_SLIDER_UP(equalizerwin_bands[8]);
			break;
		case GDK_o:
			EQ_SLIDER_DOWN(equalizerwin_bands[8]);
			break;
		case GDK_0:
			EQ_SLIDER_UP(equalizerwin_bands[9]);
			break;
		case GDK_p:
			EQ_SLIDER_DOWN(equalizerwin_bands[9]);
			break;
		default:
			if (!gtk_accel_group_activate(equalizerwin_accel,
						      event->keyval,
						      event->state))
				gtk_widget_event(mainwin, (GdkEvent*) event);
			break;
	}

	return TRUE;
}

static gboolean equalizerwin_configure(GtkWidget * window, GdkEventConfigure *event, gpointer data)
{
	if (!GTK_WIDGET_VISIBLE(window))
		return FALSE;

	if (cfg.show_wm_decorations)
		gdk_window_get_root_origin(window->window,
					   &cfg.equalizer_x, &cfg.equalizer_y);
	else
		gdk_window_get_deskrelative_origin(window->window,
						   &cfg.equalizer_x,
						   &cfg.equalizer_y);
	return FALSE;
}

void equalizerwin_set_back_pixmap(void)
{
	if (cfg.doublesize && cfg.eq_doublesize_linked)
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg_dblsize, 0);
	else
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg, 0);
	gdk_window_clear(equalizerwin->window);
}

gint equalizerwin_client_event(GtkWidget *w, GdkEventClient *event, gpointer data)
{
	static GdkAtom atom_rcfiles = GDK_NONE;

	if (!atom_rcfiles)
		atom_rcfiles = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);

	if(event->message_type == atom_rcfiles)
	{
		mainwin_set_back_pixmap();
		equalizerwin_set_back_pixmap();
		playlistwin_set_back_pixmap();
		return TRUE;
	}
	return FALSE;
}

void equalizerwin_close_cb(void)
{
	equalizerwin_show(FALSE);
}

int equalizerwin_delete(GtkWidget * w, gpointer data)
{
	equalizerwin_show(FALSE);
	return TRUE;
}

static GList *equalizerwin_read_presets(gchar * fname)
{
	gchar *filename, *name;
	ConfigFile *cfgfile;
	GList *list = NULL;
	gint i, p = 0;
	EqualizerPreset *preset;

	filename = g_strdup_printf("%s/.xmms/%s", g_get_home_dir(), fname);
	if ((cfgfile = xmms_cfg_open_file(filename)) == NULL)
	{
		g_free(filename);
		return NULL;
	}
	g_free(filename);

	for (;;)
	{
		gchar section[21];
		
		sprintf(section, "Preset%d", p++);
		if (xmms_cfg_read_string(cfgfile, "Presets", section, &name))
		{
			preset = g_malloc(sizeof (EqualizerPreset));
			preset->name = name;
			xmms_cfg_read_float(cfgfile, name, "Preamp",
					    &preset->preamp);
			for (i = 0; i < 10; i++)
			{
				gchar band[7];
				sprintf(band, "Band%d", i);
				xmms_cfg_read_float(cfgfile, name, band,
						    &preset->bands[i]);
			}
			list = g_list_prepend(list, preset);
		}
		else
			break;
	}
	list = g_list_reverse(list);
	xmms_cfg_free(cfgfile);
	return list;
}

gint equalizerwin_volume_frame_cb(gint pos)
{
	if(equalizerwin_volume)
	{
		if (pos < 32)
			equalizerwin_volume->hs_knob_nx = equalizerwin_volume->hs_knob_px = 1;
		else if (pos < 63)
			equalizerwin_volume->hs_knob_nx = equalizerwin_volume->hs_knob_px = 4;
		else
			equalizerwin_volume->hs_knob_nx = equalizerwin_volume->hs_knob_px = 7;
	}
	return 1;
}

void equalizerwin_volume_motion_cb(gint pos)
{
	gint v = (gint) rint(pos * 100 / 94.0);
	mainwin_adjust_volume_motion(v);
	mainwin_set_volume_slider(v);
}

void equalizerwin_volume_release_cb(gint pos)
{
	mainwin_adjust_volume_release();
}

gint equalizerwin_balance_frame_cb(gint pos)
{
	if(equalizerwin_balance)
	{
		if(pos < 13)
			equalizerwin_balance->hs_knob_nx = equalizerwin_balance->hs_knob_px = 11;
		else if (pos < 26)
			equalizerwin_balance->hs_knob_nx = equalizerwin_balance->hs_knob_px = 14;
		else
			equalizerwin_balance->hs_knob_nx = equalizerwin_balance->hs_knob_px = 17;
	}
			
	return 1;
}

void equalizerwin_balance_motion_cb(gint pos)
{
	gint b;
	pos = MIN(pos,38); /* The skin uses a even number of pixels
			      for the balance-slider *sigh* */
	b = (gint) rint((pos - 19) * 100 / 19.0);
	mainwin_adjust_balance_motion(b);
	mainwin_set_balance_slider(b);
}

void equalizerwin_balance_release_cb(gint pos)
{
	mainwin_adjust_balance_release();
}

void equalizerwin_set_balance_slider(gint percent)
{
	hslider_set_position(equalizerwin_balance, (gint) rint((percent*19/100.0)+19));
}

void equalizerwin_set_volume_slider(gint percent)
{
	hslider_set_position(equalizerwin_volume, (gint) rint(percent*94/100.0));
}

static void equalizerwin_create_widgets(void)
{
	int i;

	equalizerwin_on = create_tbutton(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 14, 18, 25, 12, 10, 119, 128, 119, 69, 119, 187, 119, equalizerwin_on_pushed, SKIN_EQMAIN);
	tbutton_set_toggled(equalizerwin_on, cfg.equalizer_active);
	equalizerwin_auto = create_tbutton(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 39, 18, 33, 12, 35, 119, 153, 119, 94, 119, 212, 119, equalizerwin_auto_pushed, SKIN_EQMAIN);
	tbutton_set_toggled(equalizerwin_auto, cfg.equalizer_autoload);
	equalizerwin_presets = create_pbutton(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 217, 18, 44, 12, 224, 164, 224, 176, equalizerwin_presets_pushed, SKIN_EQMAIN);
	equalizerwin_close = create_pbutton(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 264, 3, 9, 9, 0, 116, 0, 125, equalizerwin_close_cb, SKIN_EQMAIN);
	equalizerwin_close->pb_allow_draw = FALSE;

	equalizerwin_shade = create_pbutton_ex(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 254, 3, 9, 9, 254, 137, 1, 38, equalizerwin_shade_toggle, SKIN_EQMAIN, SKIN_EQ_EX);
	equalizerwin_shade->pb_allow_draw = FALSE;

	equalizerwin_graph = create_eqgraph(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 86, 17);
	equalizerwin_preamp = create_eqslider(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 21, 38);
	eqslider_set_position(equalizerwin_preamp, cfg.equalizer_preamp);
	for (i = 0; i < 10; i++)
	{
		equalizerwin_bands[i] = create_eqslider(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 78 + (i * 18), 38);
		eqslider_set_position(equalizerwin_bands[i], cfg.equalizer_bands[i]);
	}

	equalizerwin_volume = create_hslider(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 61, 4, 97, 8, 1, 30, 1, 30, 3, 7, 4, 61, 0, 94, equalizerwin_volume_frame_cb, equalizerwin_volume_motion_cb, equalizerwin_volume_release_cb, SKIN_EQ_EX);
	equalizerwin_balance = create_hslider(&equalizerwin_wlist, equalizerwin_bg, equalizerwin_gc, 164, 4, 42, 8, 11, 30, 11, 30, 3, 7, 4, 164, 0, 39, equalizerwin_balance_frame_cb, equalizerwin_balance_motion_cb, equalizerwin_balance_release_cb, SKIN_EQ_EX);

	if (!cfg.equalizer_shaded)
	{
		hide_widget(equalizerwin_volume);
		hide_widget(equalizerwin_balance);
	}
	else
	{
		pbutton_set_button_data(equalizerwin_shade, -1, 3, -1, 47);
		pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQ_EX);
		pbutton_set_button_data(equalizerwin_close, 11, 38, 11, 47);
		pbutton_set_skin_index(equalizerwin_close, SKIN_EQ_EX);
	}


}


static void equalizerwin_create_gtk(void)
{
	equalizerwin = gtk_window_new(GTK_WINDOW_DIALOG);
	dock_add_window(dock_window_list, equalizerwin);
	gtk_widget_set_app_paintable(equalizerwin, TRUE);
	gtk_window_set_policy(GTK_WINDOW(equalizerwin), FALSE, FALSE, TRUE);
	gtk_window_set_title(GTK_WINDOW(equalizerwin), _("XMMS Equalizer"));
	gtk_window_set_wmclass(GTK_WINDOW(equalizerwin), "XMMS_Equalizer", "xmms");
	gtk_window_set_transient_for(GTK_WINDOW(equalizerwin), GTK_WINDOW(mainwin));
	if (cfg.equalizer_x != -1 && cfg.save_window_position)
		dock_set_uposition(equalizerwin, cfg.equalizer_x, cfg.equalizer_y);
	if (cfg.doublesize && cfg.eq_doublesize_linked)
		gtk_widget_set_usize(equalizerwin, 550, (cfg.equalizer_shaded ? 28 : 232));
	else
		gtk_widget_set_usize(equalizerwin, 275, (cfg.equalizer_shaded ? 14 : 116));

	gtk_widget_set_events(equalizerwin, GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	gtk_widget_realize(equalizerwin);
	hint_set_skip_winlist(equalizerwin);
	util_set_cursor(equalizerwin);
	if (!cfg.show_wm_decorations)
		gdk_window_set_decorations(equalizerwin->window, 0);

	gtk_window_add_accel_group(GTK_WINDOW(equalizerwin), equalizerwin_accel);

	equalizerwin_set_back_pixmap();
	if (cfg.doublesize && cfg.eq_doublesize_linked)
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg_dblsize, 0);
	else
		gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg, 0);

	gtk_signal_connect(GTK_OBJECT(equalizerwin), "delete_event",
			   GTK_SIGNAL_FUNC(equalizerwin_delete), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "button_press_event",
			   GTK_SIGNAL_FUNC(equalizerwin_press), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "button_release_event",
			   GTK_SIGNAL_FUNC(equalizerwin_release), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "motion_notify_event",
			   GTK_SIGNAL_FUNC(equalizerwin_motion), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "focus_in_event",
			   GTK_SIGNAL_FUNC(equalizerwin_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "focus_out_event",
			   GTK_SIGNAL_FUNC(equalizerwin_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "configure_event",
			   GTK_SIGNAL_FUNC(equalizerwin_configure), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "client_event",
			   GTK_SIGNAL_FUNC(equalizerwin_client_event), NULL);
	gtk_signal_connect(GTK_OBJECT(equalizerwin), "key-press-event",
			   GTK_SIGNAL_FUNC(equalizerwin_keypress), NULL);
}

void equalizerwin_create(void)
{
	equalizerwin_accel = gtk_accel_group_new();
	equalizerwin_presets_menu = gtk_item_factory_new(GTK_TYPE_MENU,
							 "<Main>",
							 equalizerwin_accel);
	gtk_item_factory_set_translate_func(equalizerwin_presets_menu,
					    util_menu_translate, NULL, NULL);
	gtk_item_factory_create_items(equalizerwin_presets_menu, equalizerwin_presets_menu_entries_num, equalizerwin_presets_menu_entries, NULL);
	equalizer_presets = equalizerwin_read_presets("eq.preset");
	equalizer_auto_presets = equalizerwin_read_presets("eq.auto_preset");

	equalizerwin_bg = gdk_pixmap_new(NULL, 275, 116, gdk_rgb_get_visual()->depth);
	equalizerwin_bg_dblsize = gdk_pixmap_new(NULL, 550, 232, gdk_rgb_get_visual()->depth);
	equalizerwin_create_gtk();
	equalizerwin_gc = gdk_gc_new(equalizerwin->window);
	equalizerwin_create_widgets();
}

void equalizerwin_recreate(void)
{
	dock_window_list = g_list_remove(dock_window_list, equalizerwin);
	gtk_widget_destroy(equalizerwin);
	equalizerwin_create_gtk();

	equalizerwin_set_shape_mask();
}

void equalizerwin_show(gboolean show)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mainwin_general_menu, "/Graphical EQ")), show);
}

void equalizerwin_real_show(void)
{
	/*
	 * This function should only be called from the
	 * main menu signal handler
	 */
	if (!pposition_broken && cfg.equalizer_x != -1 &&
	    cfg.save_window_position && cfg.show_wm_decorations)
		dock_set_uposition(equalizerwin, cfg.equalizer_x, cfg.equalizer_y);
	gtk_widget_show(equalizerwin);
	if (pposition_broken && cfg.equalizer_x != -1 && cfg.save_window_position)
		dock_set_uposition(equalizerwin, cfg.equalizer_x, cfg.equalizer_y);
	if (cfg.doublesize && cfg.eq_doublesize_linked)
		gtk_widget_set_usize(equalizerwin, 550, (cfg.equalizer_shaded ? 28 : 232));
	else
		gtk_widget_set_usize(equalizerwin, 275, (cfg.equalizer_shaded ? 14 : 116));
	gdk_flush();
	draw_equalizer_window(TRUE);
	cfg.equalizer_visible = TRUE;
	tbutton_set_toggled(mainwin_eq, TRUE);
	hint_set_always(cfg.always_on_top);
	hint_set_sticky(cfg.sticky);
	hint_set_skip_winlist(equalizerwin);
}

void equalizerwin_real_hide(void)
{
	/*
	 * This function should only be called from the
	 * main menu signal handler
	 */
	gtk_widget_hide(equalizerwin);
	cfg.equalizer_visible = FALSE;
	tbutton_set_toggled(mainwin_eq, FALSE);
}

static EqualizerPreset *equalizerwin_find_preset(GList * list, gchar * name)
{
	GList *node = list;
	EqualizerPreset *preset;

	while (node)
	{
		preset = node->data;
		if (!strcasecmp(preset->name, name))
			return preset;
		node = g_list_next(node);
	}
	return NULL;
}

static void equalizerwin_write_preset_file(GList * list, gchar * fname)
{
	gchar *filename, *tmp;
	gint i, p;
	EqualizerPreset *preset;
	ConfigFile *cfgfile;
	GList *node;

	cfgfile = xmms_cfg_new();
	p = 0;
	node = list;
	while (node)
	{
		preset = node->data;
		tmp = g_strdup_printf("Preset%d", p++);
		xmms_cfg_write_string(cfgfile, "Presets", tmp, preset->name);
		g_free(tmp);
		xmms_cfg_write_float(cfgfile, preset->name, "Preamp", preset->preamp);
		for (i = 0; i < 10; i++)
		{
			tmp = g_strdup_printf("Band%d\n", i);
			xmms_cfg_write_float(cfgfile, preset->name, tmp, preset->bands[i]);
			g_free(tmp);
		}
		node = g_list_next(node);
	}
	filename = g_strdup_printf("%s/.xmms/%s", g_get_home_dir(), fname);
	xmms_cfg_write_file(cfgfile, filename);
	xmms_cfg_free(cfgfile);
	g_free(filename);
}

static gboolean equalizerwin_load_preset(GList * list, gchar * name)
{
	EqualizerPreset *preset;
	gint i;

	if ((preset = equalizerwin_find_preset(list, name)) != NULL)
	{
		eqslider_set_position(equalizerwin_preamp, preset->preamp);
		for (i = 0; i < 10; i++)
			eqslider_set_position(equalizerwin_bands[i], preset->bands[i]);
		equalizerwin_eq_changed();
		return TRUE;
	}
	return FALSE;
}

static GList *equalizerwin_save_preset(GList * list, gchar * name, gchar * fname)
{
	gint i;
	EqualizerPreset *preset;

	if (!(preset = equalizerwin_find_preset(list, name)))
	{
		preset = g_malloc(sizeof (EqualizerPreset));
		preset->name = g_strdup(name);
		list = g_list_append(list, preset);
	}

	preset->preamp = eqslider_get_position(equalizerwin_preamp);
	for (i = 0; i < 10; i++)
		preset->bands[i] = eqslider_get_position(equalizerwin_bands[i]);

	equalizerwin_write_preset_file(list, fname);

	return list;
}

static GList *equalizerwin_delete_preset(GList * list, gchar * name, gchar * fname)
{
	EqualizerPreset *preset;
	GList *node;

	if ((preset = equalizerwin_find_preset(list, name)) && (node = g_list_find(list, preset)))
	{
		list = g_list_remove_link(list, node);
		g_free(preset->name);
		g_free(preset);
		g_list_free_1(node);

		equalizerwin_write_preset_file(list, fname);
	}

	return list;
}


static GList *equalizerwin_import_winamp_eqf(FILE *file)
{
	gchar header[31];
	gchar tmp[257];
	gchar bands[11];
	gint i=0;
	GList *list = NULL;
	EqualizerPreset *preset;

	fread(header, 1, 31, file);
	if (!strncmp(header, "Winamp EQ library file v1.1", 27))
	{
		while (fread(tmp, 1, 257, file)) {
			preset = g_malloc(sizeof (EqualizerPreset));

			fread(bands, 1, 11, file);

			preset->name = g_strdup(tmp);
			preset->preamp =  20.0 - ((bands[10] * 40.0) / 64);

			for (i = 0; i < 10; i++)
				preset->bands[i] = 20.0 - ((bands[i] * 40.0) / 64);

			list = g_list_prepend(list, preset);
		}
        }

	list = g_list_reverse(list);
	return list;

}

static void equalizerwin_read_winamp_eqf(FILE *file)
{
	gchar header[31];
	guchar bands[11];
	gint i;
	
	fread(header, 1, 31, file);
	if (!strncmp(header, "Winamp EQ library file v1.1", 27))
	{
		if (fseek(file, 257, SEEK_CUR) == -1)	/* Skip name */
			return;
		if (fread(bands, 1, 11, file) != 11)
			return;
		eqslider_set_position(equalizerwin_preamp, 20.0 - ((bands[10] * 40.0) / 63.0));
		for (i = 0; i < 10; i++)
			eqslider_set_position(equalizerwin_bands[i], 20.0 - ((bands[i] * 40.0) / 64.0));
	}
	equalizerwin_eq_changed();
}

static void equalizerwin_read_xmms_preset(ConfigFile *cfgfile)
{
	gfloat val;
	gint i;
	
	if (xmms_cfg_read_float(cfgfile, "Equalizer preset", "Preamp", &val))
		eqslider_set_position(equalizerwin_preamp, val);
	for (i = 0; i < 10; i++)
	{
		gchar tmp[7];
		sprintf(tmp, "Band%d", i);
		if (xmms_cfg_read_float(cfgfile, "Equalizer preset", tmp, &val))
			eqslider_set_position(equalizerwin_bands[i], val);
	}
	equalizerwin_eq_changed();
}

static void equalizerwin_save_ok(GtkWidget * widget, gpointer data)
{
	gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_entry));
	if (strlen(text) != 0)
		equalizer_presets = equalizerwin_save_preset(equalizer_presets, text, "eq.preset");
	gtk_widget_destroy(equalizerwin_save_window);
}

static void equalizerwin_save_select(GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer data)
{
	gchar *text;

	gtk_clist_get_text(clist, row, 0, &text);

	gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_entry), text);
	if (event && event->type == GDK_2BUTTON_PRESS)
		equalizerwin_save_ok(NULL, NULL);

}

static void equalizerwin_load_ok(GtkWidget * widget, gpointer data)
{
	gchar *text;
	GtkCList *clist = GTK_CLIST(data);

	if (clist && clist->selection)
	{
		gtk_clist_get_text(clist, GPOINTER_TO_INT(clist->selection->data), 0, &text);
		equalizerwin_load_preset(equalizer_presets, text);
	}
	gtk_widget_destroy(equalizerwin_load_window);
}

static void equalizerwin_load_select(GtkCList * widget, gint row, gint column, GdkEventButton * event, gpointer data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		equalizerwin_load_ok(NULL, widget);
}

static void equalizerwin_delete_delete(GtkWidget * widget, gpointer data)
{
	gchar *text;
	GList *list, *next;
	GtkCList *clist = GTK_CLIST(data);

	g_return_if_fail(clist != NULL);

	list = clist->selection;
	gtk_clist_freeze(clist);
	while (list)
	{
		next = g_list_next(list);
		gtk_clist_get_text(clist, GPOINTER_TO_INT(list->data), 0, &text);
		equalizer_presets = equalizerwin_delete_preset(equalizer_presets,
							       text, "eq.preset");
		gtk_clist_remove(clist, GPOINTER_TO_INT(list->data));
		list = next;
	}
	gtk_clist_thaw(clist);
}

static void equalizerwin_save_auto_ok(GtkWidget * widget, gpointer data)
{
	gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_auto_entry));
	if (strlen(text) != 0)
		equalizer_auto_presets = equalizerwin_save_preset(equalizer_auto_presets, text, "eq.auto_preset");
	gtk_widget_destroy(equalizerwin_save_auto_window);
}

static void equalizerwin_save_auto_select(GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer data)
{
	gchar *text;

	gtk_clist_get_text(clist, row, 0, &text);

	gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry), text);
	if (event && event->type == GDK_2BUTTON_PRESS)
		equalizerwin_save_auto_ok(NULL, NULL);

}

static void equalizerwin_load_auto_ok(GtkWidget * widget, gpointer data)
{
	gchar *text;
	GtkCList *clist = GTK_CLIST(data);

	if (clist && clist->selection)
	{
		gtk_clist_get_text(clist, GPOINTER_TO_INT(clist->selection->data), 0, &text);
		equalizerwin_load_preset(equalizer_auto_presets, text);
	}
	gtk_widget_destroy(equalizerwin_load_auto_window);
}

static void equalizerwin_load_auto_select(GtkWidget * widget, gint row, gint column, GdkEventButton * event, gpointer data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		equalizerwin_load_auto_ok(NULL, widget);
}

static void equalizerwin_delete_auto_delete(GtkWidget * widget, gpointer data)
{
	gchar *text;
	GList *list, *next;
	GtkCList *clist = GTK_CLIST(data);

	g_return_if_fail(clist != NULL);

	list = clist->selection;
	gtk_clist_freeze(clist);
	while (list)
	{
		next = g_list_next(list);
		gtk_clist_get_text(clist, GPOINTER_TO_INT(list->data), 0, &text);
		equalizer_auto_presets = equalizerwin_delete_preset(equalizer_auto_presets, text, "eq.auto_preset");
		gtk_clist_remove(clist, GPOINTER_TO_INT(list->data));
		list = next;
	}
	gtk_clist_thaw(clist);
}

static void equalizerwin_load_filesel_ok(GtkWidget * w, GtkFileSelection * filesel)
{
	gchar *filename;
	ConfigFile *cfgfile;

	if (util_filebrowser_is_dir(filesel))
		return;

	filename = gtk_file_selection_get_filename(filesel);

	if ((cfgfile = xmms_cfg_open_file(filename)) != NULL)
	{
		equalizerwin_read_xmms_preset(cfgfile);
	        xmms_cfg_free(cfgfile);
	}
	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static void equalizerwin_import_winamp_filesel_ok(GtkWidget * w, GtkFileSelection * filesel)
{
	gchar *filename;
	FILE *file;

	if (util_filebrowser_is_dir(filesel))
		return;

	filename = gtk_file_selection_get_filename(filesel);

	if ((file = fopen(filename, "rb")) != NULL)
	{
		equalizer_presets = g_list_concat(equalizer_presets,
				    equalizerwin_import_winamp_eqf(file));
		equalizerwin_write_preset_file(equalizer_presets, "eq.preset");
	}

	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static void equalizerwin_load_winamp_filesel_ok(GtkWidget * w, GtkFileSelection * filesel)
{
	gchar *filename;
	FILE *file;

	if (util_filebrowser_is_dir(filesel))
		return;

	filename = gtk_file_selection_get_filename(filesel);

	if ((file = fopen(filename, "rb")) != NULL)
		equalizerwin_read_winamp_eqf(file);

	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static void equalizerwin_save_filesel_ok(GtkWidget * w, GtkFileSelection * filesel)
{
	gchar *filename;
	ConfigFile *cfgfile;
	gint i;

	if (util_filebrowser_is_dir(filesel))
		return;

	filename = gtk_file_selection_get_filename(filesel);

	cfgfile = xmms_cfg_new();
	xmms_cfg_write_float(cfgfile, "Equalizer preset", "Preamp", eqslider_get_position(equalizerwin_preamp));
	for (i = 0; i < 10; i++)
	{
		gchar tmp[7];
		sprintf(tmp, "Band%d", i);
		xmms_cfg_write_float(cfgfile, "Equalizer preset", tmp,
				     eqslider_get_position(equalizerwin_bands[i]));
	}
	xmms_cfg_write_file(cfgfile, filename);
	xmms_cfg_free(cfgfile);
	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static void equalizerwin_save_winamp_filesel_ok(GtkWidget * w, GtkFileSelection * filesel)
{
	gchar *filename, name[257];
	FILE *file;
	gint i;
	guchar bands[11];

	if (util_filebrowser_is_dir(filesel))
		return;

	filename = gtk_file_selection_get_filename(filesel);

	if ((file = fopen(filename, "wb")) != NULL)
	{
		fwrite("Winamp EQ library file v1.1\x1a!--", 1, 31, file);
		memset(name, 0, 257);
		strcpy(name, "Entry1");
		fwrite(name, 1, 257, file);
		for (i = 0; i < 10; i++)
			bands[i] = 63 - (((eqslider_get_position(equalizerwin_bands[i]) + 20) * 63) / 40);
		bands[10] = 63 - (((eqslider_get_position(equalizerwin_preamp) + 20) * 63) / 40);
		fwrite(bands, 1, 11, file);
		fclose(file);
	}

	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static gint equalizerwin_list_sort_func(GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	return strcasecmp(GTK_CELL_TEXT(row1->cell[clist->sort_column])->text, GTK_CELL_TEXT(row2->cell[clist->sort_column])->text);
}

static GtkWidget *equalizerwin_create_list_window(GList * preset_list,
						  gchar * title,
						  GtkWidget ** window,
						  GtkSelectionMode sel_mode,
						  GtkWidget ** entry,
						  gchar * btn1_caption,
						  gchar * btn2_caption,
						  GtkSignalFunc btn1_func,
						  GtkSignalFunc select_row_func)
{
	GtkWidget *vbox, *scrolled_window, *bbox, *btn1, *btn2, *clist;
	char *preset_text[1];
	GList *node;

	*window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(*window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), window);
	gtk_window_set_transient_for(GTK_WINDOW(*window), GTK_WINDOW(equalizerwin));
	gtk_window_set_position(GTK_WINDOW(*window), GTK_WIN_POS_MOUSE);
	gtk_window_set_title(GTK_WINDOW(*window), title);
	
	gtk_widget_set_usize(*window, 350, 300);
	gtk_container_set_border_width(GTK_CONTAINER(*window), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(*window), vbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	preset_text[0] = _("Presets");
	clist = gtk_clist_new_with_titles(1, preset_text);
	if (select_row_func)
		gtk_signal_connect(GTK_OBJECT(clist), "select_row",
				   GTK_SIGNAL_FUNC(select_row_func), NULL);
	gtk_clist_column_titles_passive(GTK_CLIST(clist));
	gtk_clist_set_selection_mode(GTK_CLIST(clist), sel_mode);

	node = preset_list;
	while (node)
	{
		gtk_clist_append(GTK_CLIST(clist),
				 &((EqualizerPreset *) node->data)->name);
		node = node->next;
	}
	gtk_clist_set_compare_func(GTK_CLIST(clist), equalizerwin_list_sort_func);
	gtk_clist_sort(GTK_CLIST(clist));

	gtk_container_add(GTK_CONTAINER(scrolled_window), clist);
	gtk_widget_show(clist);
	gtk_widget_show(scrolled_window);

	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	if (entry)
	{
		*entry = gtk_entry_new();
		gtk_signal_connect(GTK_OBJECT(*entry), "activate",
				   GTK_SIGNAL_FUNC(btn1_func), NULL);
		gtk_box_pack_start(GTK_BOX(vbox), *entry, FALSE, FALSE, 0);
		gtk_widget_show(*entry);
	}

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);

	btn1 = gtk_button_new_with_label(btn1_caption);
	gtk_signal_connect(GTK_OBJECT(btn1), "clicked",
			   GTK_SIGNAL_FUNC(btn1_func), clist);
	GTK_WIDGET_SET_FLAGS(btn1, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), btn1, TRUE, TRUE, 0);
	gtk_widget_show(btn1);

	btn2 = gtk_button_new_with_label(btn2_caption);
	gtk_signal_connect_object(GTK_OBJECT(btn2), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(*window));
	GTK_WIDGET_SET_FLAGS(btn2, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), btn2, TRUE, TRUE, 0);
	gtk_widget_show(btn2);

	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
	gtk_widget_show(bbox);

	gtk_widget_grab_default(btn1);
	gtk_widget_show(vbox);
	gtk_widget_show(*window);
	return *window;
}

void equalizerwin_presets_menu_cb(gpointer cb_data, guint action, GtkWidget * w)
{
	switch (action)
	{
		case EQUALIZER_PRESETS_LOAD_PRESET:
			if (!equalizerwin_load_window)
				equalizerwin_create_list_window(equalizer_presets, _("Load preset"), &equalizerwin_load_window, GTK_SELECTION_SINGLE, NULL, _("OK"), _("Cancel"), equalizerwin_load_ok, equalizerwin_load_select);
			else
				gdk_window_raise(equalizerwin_load_window->window);
			break;
		case EQUALIZER_PRESETS_LOAD_AUTOPRESET:
			if (!equalizerwin_load_auto_window)
				equalizerwin_create_list_window(equalizer_auto_presets, _("Load auto-preset"), &equalizerwin_load_auto_window, GTK_SELECTION_SINGLE, NULL, _("OK"), _("Cancel"), equalizerwin_load_auto_ok, equalizerwin_load_auto_select);
			else
				gdk_window_raise(equalizerwin_load_auto_window->window);
			break;
		case EQUALIZER_PRESETS_LOAD_DEFAULT:
			equalizerwin_load_preset(equalizer_presets, "Default");
			break;
		case EQUALIZER_PRESETS_LOAD_ZERO:
		{
			gint i;

			eqslider_set_position(equalizerwin_preamp, 0);
			for (i = 0; i < 10; i++)
				eqslider_set_position(equalizerwin_bands[i], 0);
			equalizerwin_eq_changed();
			break;
		}
		case EQUALIZER_PRESETS_LOAD_FROM_FILE:
		{
			static GtkWidget *load_filesel;
			if (load_filesel != NULL)
				break;
			
			load_filesel = gtk_file_selection_new(_("Load equalizer preset"));
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(equalizerwin_load_filesel_ok), load_filesel);
			gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(load_filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(load_filesel));
			gtk_signal_connect(GTK_OBJECT(load_filesel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &load_filesel);
			gtk_widget_show(load_filesel);
			
			break;
		}
		case EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE:
		{
			static GtkWidget *load_winamp_filesel;

			if (load_winamp_filesel != NULL)
				break;

			load_winamp_filesel = gtk_file_selection_new(_("Load equalizer preset"));
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_winamp_filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(equalizerwin_load_winamp_filesel_ok), load_winamp_filesel);
			gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(load_winamp_filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(load_winamp_filesel));
			gtk_signal_connect(GTK_OBJECT(load_winamp_filesel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &load_winamp_filesel);
			gtk_widget_show(load_winamp_filesel);
			
			break;
		}
		case EQUALIZER_PRESETS_IMPORT_WINAMPFILE:
		{
			static GtkWidget *import_winamp_filesel;

			if (import_winamp_filesel != NULL)
				break;

			import_winamp_filesel = gtk_file_selection_new(_("Import equalizer presets"));
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(import_winamp_filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(equalizerwin_import_winamp_filesel_ok), import_winamp_filesel);
			gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(import_winamp_filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(import_winamp_filesel));
			gtk_signal_connect(GTK_OBJECT(import_winamp_filesel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &import_winamp_filesel);
			gtk_widget_show(import_winamp_filesel);
			
			break;
		}
	
		case EQUALIZER_PRESETS_SAVE_PRESET:
			if (!equalizerwin_save_window)
				equalizerwin_create_list_window(equalizer_presets, _("Save preset"), &equalizerwin_save_window, GTK_SELECTION_SINGLE, &equalizerwin_save_entry, _("OK"), _("Cancel"), equalizerwin_save_ok, equalizerwin_save_select);
			else
				gdk_window_raise(equalizerwin_save_window->window);
			break;
		case EQUALIZER_PRESETS_SAVE_AUTOPRESET:
		{
			gchar *name;

			if (!equalizerwin_save_auto_window)
				equalizerwin_create_list_window(equalizer_auto_presets, _("Save auto-preset"), &equalizerwin_save_auto_window, GTK_SELECTION_SINGLE, &equalizerwin_save_auto_entry, _("OK"), _("Cancel"), equalizerwin_save_auto_ok, equalizerwin_save_auto_select);
			else
				gdk_window_raise(equalizerwin_save_auto_window->window);
			if ((name = playlist_get_filename(get_playlist_position())) != NULL)
			{
				gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry), g_basename(name));
				g_free(name);
			}
			break;
		}
		case EQUALIZER_PRESETS_SAVE_DEFAULT:
			equalizer_presets = equalizerwin_save_preset(equalizer_presets, "Default", "eq.preset");
			break;
		case EQUALIZER_PRESETS_SAVE_TO_FILE:
		{
			static GtkWidget *equalizerwin_save_filesel;
			gchar *songname;

			if (equalizerwin_save_filesel != NULL)
				break;

			equalizerwin_save_filesel = gtk_file_selection_new(_("Save equalizer preset"));

			if ((songname = playlist_get_filename(get_playlist_position())) != NULL)
			{
				gchar *eqname = g_strdup_printf("%s.%s", songname, cfg.eqpreset_extension);
				g_free(songname);
				gtk_file_selection_set_filename(GTK_FILE_SELECTION(equalizerwin_save_filesel), eqname);
				g_free(eqname);
			}
			
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(equalizerwin_save_filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(equalizerwin_save_filesel_ok), equalizerwin_save_filesel);
			gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(equalizerwin_save_filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(equalizerwin_save_filesel));
			gtk_signal_connect(GTK_OBJECT(equalizerwin_save_filesel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &equalizerwin_save_filesel);
			gtk_widget_show(equalizerwin_save_filesel);

			break;
		}
		case EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE: {
			static GtkWidget *save_winamp_filesel;
			
			if (save_winamp_filesel != NULL)
				break;

			save_winamp_filesel = gtk_file_selection_new(_("Save equalizer preset"));
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(save_winamp_filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(equalizerwin_save_winamp_filesel_ok), save_winamp_filesel);
			gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(save_winamp_filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(save_winamp_filesel));
			gtk_signal_connect(GTK_OBJECT(save_winamp_filesel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &save_winamp_filesel);
			gtk_widget_show(save_winamp_filesel);
			
			break;
		}
		case EQUALIZER_PRESETS_DELETE_PRESET:
			if (!equalizerwin_delete_window)
				equalizerwin_create_list_window(equalizer_presets, _("Delete preset"), &equalizerwin_delete_window, GTK_SELECTION_EXTENDED, NULL, _("Delete"), _("Close"), equalizerwin_delete_delete, NULL);
			break;
		case EQUALIZER_PRESETS_DELETE_AUTOPRESET:
			if (!equalizerwin_delete_auto_window)
				equalizerwin_create_list_window(equalizer_auto_presets, _("Delete auto-preset"), &equalizerwin_delete_auto_window, GTK_SELECTION_EXTENDED, NULL, _("Delete"), _("Close"), equalizerwin_delete_auto_delete, NULL);
			break;
	        case EQUALIZER_PRESETS_CONFIGURE:
			if (!equalizerwin_configure_window)
			{
				equalizerwin_configure_window =
					equalizerwin_create_conf_window();
				gtk_signal_connect(GTK_OBJECT(equalizerwin_configure_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &equalizerwin_configure_window);
			}

			break;
	}
}

void equalizerwin_load_auto_preset(gchar * filename)
{
	gchar *presetfilename, *directory;
	ConfigFile *cfgfile;
	
	if (!cfg.equalizer_autoload)
		return;

	g_return_if_fail(filename != NULL);

	presetfilename = g_strdup_printf("%s.%s", filename, cfg.eqpreset_extension);

	/*
	 * First try to find a per file preset file
	 */
	if (strlen(cfg.eqpreset_extension) > 0 &&
	    (cfgfile = xmms_cfg_open_file(presetfilename)) != NULL)
	{
		g_free(presetfilename);
		equalizerwin_read_xmms_preset(cfgfile);
		xmms_cfg_free(cfgfile);
		return;
	}

	directory = g_dirname(filename);
	g_free(presetfilename);
	presetfilename = g_strdup_printf("%s/%s", directory, cfg.eqpreset_default_file);
	g_free(directory);

	/*
	 * Try to find a per directory preset file
	 */
	if (strlen(cfg.eqpreset_default_file) > 0 &&
	    (cfgfile = xmms_cfg_open_file(presetfilename)) != NULL)
	{
		equalizerwin_read_xmms_preset(cfgfile);
		xmms_cfg_free(cfgfile);
	}
	/*
	 * Fall back to the oldstyle auto presets
	 */
	else if (!equalizerwin_load_preset(equalizer_auto_presets, g_basename(filename)))
		equalizerwin_load_preset(equalizer_presets, "Default");
	g_free(presetfilename);
}
	
void equalizerwin_set_preamp(gfloat preamp)
{
	eqslider_set_position(equalizerwin_preamp, preamp);
	equalizerwin_eq_changed();
}

void equalizerwin_set_band(gint band, gfloat value)
{
	g_return_if_fail(band >= 0 && band < 10);
	eqslider_set_position(equalizerwin_bands[band], value);
}

gfloat equalizerwin_get_preamp(void)
{
	return eqslider_get_position(equalizerwin_preamp);
}

gfloat equalizerwin_get_band(gint band)
{
	g_return_val_if_fail(band >= 0 && band < 10, 0);
	return eqslider_get_position(equalizerwin_bands[band]);
}

static void equalizerwin_conf_apply_changes(void)
{
	gchar *start;
	
	g_free(cfg.eqpreset_default_file);
	g_free(cfg.eqpreset_extension);

	cfg.eqpreset_default_file = gtk_editable_get_chars(GTK_EDITABLE(eqconfwin_options_eqdf_entry), 0, -1);
	cfg.eqpreset_extension = gtk_editable_get_chars(GTK_EDITABLE(eqconfwin_options_eqef_entry), 0, -1);

	g_strstrip(cfg.eqpreset_default_file);
	start = cfg.eqpreset_default_file;
	/* Strip leading '.' */
	while (*start == '.')
		start++;
	if (start != cfg.eqpreset_default_file)
		g_memmove(cfg.eqpreset_default_file, start, strlen(start) + 1);

	g_strstrip(cfg.eqpreset_extension);
	start = cfg.eqpreset_extension;
	while (*start == '.')
		start++;
	if (start != cfg.eqpreset_extension)
		g_memmove(cfg.eqpreset_extension, start, strlen(start) + 1);
}

static void equalizerwin_conf_ok_cb(GtkWidget * w, gpointer data)
{
	equalizerwin_conf_apply_changes();
	gtk_widget_destroy(equalizerwin_configure_window);
}

static void equalizerwin_conf_apply_cb(GtkWidget * w, gpointer data)
{
	equalizerwin_conf_apply_changes();
}

GtkWidget * equalizerwin_create_conf_window(void)
{
	GtkWidget *window, *notebook;
	GtkWidget *options_eqdf_box;
	GtkWidget *options_eqe_box;
	GtkWidget *options_eqdf;
	GtkWidget *options_eqe;
	GtkWidget *options_frame, *options_vbox;
	GtkWidget *options_table;
	GtkWidget *vbox, *hbox, *instructions;
	GtkWidget *ok, *cancel, *apply;

	window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(window), _("Configure Equalizer"));
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(mainwin));
	gtk_container_border_width(GTK_CONTAINER(window), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	options_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(options_vbox), 5);
	options_frame = gtk_frame_new(_("Options"));
	gtk_box_pack_start(GTK_BOX(options_vbox), options_frame, FALSE, FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(options_frame), 0);
	options_table = gtk_table_new(1, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(options_table), 10);
	gtk_container_add(GTK_CONTAINER(options_frame), options_table);
	gtk_container_border_width(GTK_CONTAINER(options_table), 5);

	options_eqdf_box = gtk_hbox_new(FALSE, 5);
	options_eqdf = gtk_label_new(_("Directory preset file:"));
	gtk_box_pack_start(GTK_BOX(options_eqdf_box), options_eqdf, FALSE, FALSE, 0);
	eqconfwin_options_eqdf_entry = gtk_entry_new_with_max_length(40);
	gtk_widget_set_usize(eqconfwin_options_eqdf_entry, 115, -1);
	gtk_box_pack_start(GTK_BOX(options_eqdf_box), eqconfwin_options_eqdf_entry, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(options_table), options_eqdf_box, 0, 1, 0, 1);
	
	options_eqe_box = gtk_hbox_new(FALSE, 5);
	options_eqe = gtk_label_new(_("File preset extension:"));
	gtk_box_pack_start(GTK_BOX(options_eqe_box), options_eqe, FALSE, FALSE, 0);
	eqconfwin_options_eqef_entry = gtk_entry_new_with_max_length(20);
	gtk_widget_set_usize(eqconfwin_options_eqef_entry, 55, -1);
	gtk_box_pack_start(GTK_BOX(options_eqe_box), eqconfwin_options_eqef_entry, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(options_table), options_eqe_box, 1, 2, 0, 1);
	instructions =
		gtk_label_new(_("If \"Auto\" is enabled on the equalizer, xmms "
				"will try to load equalizer presets like this:\n"
				"1: Look for a preset file in the directory of the "
				"file we are about to play.\n"
				"2: Look for a directory preset file in the same "
				"directory.\n"
				"3: Look for a preset saved with the "
				"\"auto-load\" feature\n"
				"4: Finally, try to load the \"default\" preset"));
	gtk_label_set_justify(GTK_LABEL(instructions), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start_defaults(GTK_BOX(options_vbox), instructions);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), options_vbox, gtk_label_new(_("Options")));

        /* 
         * OK, Cancel & Apply 
         */

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 5);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(equalizerwin_conf_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(hbox), ok, TRUE, TRUE, 0);
	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(hbox), cancel, TRUE, TRUE, 0);
	apply = gtk_button_new_with_label(_("Apply"));
	gtk_signal_connect(GTK_OBJECT(apply), "clicked", GTK_SIGNAL_FUNC(equalizerwin_conf_apply_cb), NULL);
	GTK_WIDGET_SET_FLAGS(apply, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(hbox), apply, TRUE, TRUE, 0);

	gtk_entry_set_text(GTK_ENTRY(eqconfwin_options_eqdf_entry), cfg.eqpreset_default_file);
	gtk_entry_set_text(GTK_ENTRY(eqconfwin_options_eqef_entry), cfg.eqpreset_extension);

	gtk_widget_show_all(window);
	gtk_widget_grab_default(ok);

	return window;
}
