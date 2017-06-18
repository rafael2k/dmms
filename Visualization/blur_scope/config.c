#include "config.h"

#include <gtk/gtk.h>

#include "libxmms/configfile.h"
#include "blur_scope.h"
#include "xmms/i18n.h"

static GtkWidget *configure_win = NULL;
static GtkWidget *vbox, *options_frame, *options_vbox;
static GtkWidget *options_colorpicker;
static GtkWidget *bbox, *ok, *cancel;

static void configure_ok(GtkWidget *w, gpointer data)
{
	ConfigFile *cfg;	
	gchar *filename;
	gdouble color[3]; 
	
	
	
	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfg = xmms_cfg_open_file(filename);
	if (!cfg)
		cfg = xmms_cfg_new();
	gtk_color_selection_get_color(GTK_COLOR_SELECTION(options_colorpicker), color);
	bscope_cfg.color = ((guint32)(255.0*color[0])<<16) |
		              ((guint32)(255.0*color[1])<<8) |
		              ((guint32)(255.0*color[2])); 
	xmms_cfg_write_int(cfg, "BlurScope", "color", bscope_cfg.color);
	xmms_cfg_write_file(cfg, filename);
	xmms_cfg_free(cfg);
	g_free(filename);
	generate_cmap();
	gtk_widget_destroy(configure_win);
}

static void configure_cancel(GtkWidget *w, gpointer data)
{
	bscope_cfg.color = GPOINTER_TO_UINT(data);
	generate_cmap();
	gtk_widget_destroy(configure_win);
}

static void color_changed(GtkWidget *w, gpointer data)
{
	gdouble color[3]; 
	gtk_color_selection_get_color(GTK_COLOR_SELECTION(options_colorpicker), color);
	bscope_cfg.color = ((guint32)(255.0*color[0])<<16) |
		              ((guint32)(255.0*color[1])<<8) |
		              ((guint32)(255.0*color[2]));
	generate_cmap();
}

void bscope_configure (void)
{
	gdouble color[3];
	if(configure_win)
		return;

	bscope_read_config();
	color[0]=((gdouble)(bscope_cfg.color /0x10000))/256;
	color[1]=((gdouble)((bscope_cfg.color %0x10000)/0x100))/256;
	color[2]=((gdouble)(bscope_cfg.color %0x100))/256;
	
	configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);
	gtk_window_set_title(GTK_WINDOW(configure_win), _("Color Entry"));
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &configure_win);

	vbox = gtk_vbox_new(FALSE, 5);

	options_frame = gtk_frame_new(_("Options:"));
	gtk_container_set_border_width(GTK_CONTAINER(options_frame), 5);

	options_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(options_vbox), 5);

	options_colorpicker = gtk_color_selection_new();
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(options_colorpicker), color);
	gtk_signal_connect(GTK_OBJECT(options_colorpicker), "color_changed", GTK_SIGNAL_FUNC(color_changed), NULL);

	gtk_box_pack_start(GTK_BOX(options_vbox), options_colorpicker, FALSE, FALSE, 0);
        gtk_widget_show(options_colorpicker);
	
	
	gtk_container_add(GTK_CONTAINER(options_frame), options_vbox);
	gtk_widget_show(options_vbox);

	gtk_box_pack_start(GTK_BOX(vbox), options_frame, TRUE, TRUE, 0);
	gtk_widget_show(options_frame);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(configure_ok), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_show(ok);
	

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
			   GTK_SIGNAL_FUNC(configure_cancel),
			   GUINT_TO_POINTER(bscope_cfg.color));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
	gtk_widget_show(cancel);
	gtk_widget_show(bbox);
	
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);
	gtk_widget_show(vbox);
	gtk_widget_show(configure_win);
	gtk_widget_grab_default(ok);
}






