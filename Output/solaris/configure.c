#include "Sun.h"
#include <strings.h>
#include <math.h>
#include <xmms/i18n.h>

static GtkWidget *configure_win;

/* Audio device tab widgets */
static GtkWidget *audio_device_option;
static GtkWidget *audio_device_always_audiodev;

static GtkWidget *port_chk_speaker, *port_chk_headphones, *port_chk_lineout;

/* buffer size tab widgets */
static GtkWidget *buffer_pre_spin, *buffer_size_spin;

static gchar *audio_device;

/* configure_write(); Write configuration to file */
static void configure_write()
{
	ConfigFile *cfgfile;

	cfgfile = xmms_cfg_open_default_file();
	
	xmms_cfg_write_string(cfgfile, "Sun", "audio_device", sun_cfg.audio_device);
	xmms_cfg_write_int(cfgfile, "Sun", "always_use_audiodev", sun_cfg.always_audiodev );
	xmms_cfg_write_int(cfgfile, "Sun", "buffer_size", sun_cfg.buffer_size);
	xmms_cfg_write_int(cfgfile, "Sun", "prebuffer", sun_cfg.prebuffer);
	xmms_cfg_write_int(cfgfile, "Sun", "speaker", (sun_cfg.channel_flags & AUDIO_SPEAKER) != 0);
	xmms_cfg_write_int(cfgfile, "Sun", "line_out", (sun_cfg.channel_flags & AUDIO_LINE_OUT) != 0);
	xmms_cfg_write_int(cfgfile, "Sun", "headphone", (sun_cfg.channel_flags & AUDIO_HEADPHONE) != 0);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);
}

/* Handler for "APPLY"/"OK" buttons */
static void configure_win_apply_cb(GtkWidget *w, gpointer data)
{
	sun_cfg.audio_device = audio_device;
	sun_cfg.buffer_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_size_spin));
	sun_cfg.prebuffer = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_pre_spin));
	sun_cfg.channel_flags = ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(port_chk_lineout)) ? AUDIO_LINE_OUT : 0) |
				 (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(port_chk_speaker)) ? AUDIO_SPEAKER : 0) |
				 (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(port_chk_headphones)) ? AUDIO_HEADPHONE : 0));
	sun_cfg.always_audiodev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_device_always_audiodev));
	configure_write();

	abuffer_set_dev();
}

static void configure_win_audio_dev_cb(GtkWidget *widget, gchar *device)
{
	audio_device = device;
}

static gchar * device_name(gchar *file_name)
{
	int fd;
	gchar *name = NULL;
	gchar *ctl_name;

	/* Use the ctl device to prevent a lock on any device being
	   used by another process; also for CD output */
	ctl_name = g_strconcat(file_name, "ctl", NULL);
  
	fd = open(ctl_name, O_RDONLY);
	g_free(ctl_name);
	if (fd != -1)
	{
		audio_device_t device;
		if (ioctl(fd, AUDIO_GETDEV, &device) >= 0)
			name = g_strdup(device.name);

		close (fd);
	}

	return name;
}

static gint scan_devices(gchar *type, GtkWidget *option_menu,
			 GtkSignalFunc sigfunc, gchar *default_name)
{
	GtkWidget *menu, *item;
	gchar *name;
	gchar *default_device;

	gint index;
	gint menu_index = 0, default_index = -1;

	menu = gtk_menu_new ();

	default_device = getenv("AUDIODEV");
	if (! default_device)
		default_device = "/dev/audio";

	/* Check first the default audio device. */
	name = device_name (default_device);
	if (name)
	{
		gchar *tem = g_strdup_printf(_("Default - %s"), name);
		item = gtk_menu_item_new_with_label (tem);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   sigfunc, default_device);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU (menu), item);
		
		if (default_name && strcmp (default_name, default_device) == 0)
			default_index = menu_index;
		menu_index ++;
	}

	/* Scan up to ten devices. */
	for (index = 0; index < 10; index ++)
	{
		gchar *devname = g_strdup_printf("/dev/sound/%d", index);

		name = device_name(devname);
		if (name)
		{
			gchar *tem = g_strdup_printf(_("Soundcard #%d - %s"), index, name);
			item = gtk_menu_item_new_with_label (tem);
			gtk_signal_connect(GTK_OBJECT(item), "activate",
					   sigfunc, devname);
			gtk_widget_show(item);
			gtk_menu_append(GTK_MENU (menu), item);

			if (default_name && strcmp(default_name, devname) == 0)
				default_index = menu_index;
			menu_index ++;
		}
		else
			g_free(devname);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
	return default_index;
}

void abuffer_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *port_frame, *port_vbox;
	GtkWidget *audio_vbox, *audio_device_frame, *audio_device_box;
	GtkObject *buffer_size_adj, *buffer_pre_adj;
	GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
	GtkWidget *buffer_size_box, *buffer_size_label;
	GtkWidget *buffer_pre_box, *buffer_pre_label;
	GtkWidget *bbox, *ok, *cancel, *apply;

	int default_index = -1;

	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(configure_win), _("Configure Solaris driver"));
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/********************/
	/* Audio device tab */
	/********************/
	audio_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(audio_vbox), 5);

	audio_device_frame = gtk_frame_new("Audio device:");
	gtk_box_pack_start(GTK_BOX(audio_vbox), audio_device_frame,
			    FALSE, FALSE, 0);

	audio_device_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(audio_device_box), 5);
	gtk_container_add(GTK_CONTAINER(audio_device_frame),
			  audio_device_box);

	audio_device_option = gtk_option_menu_new ();
	gtk_box_pack_start(GTK_BOX(audio_device_box), audio_device_option,
			    TRUE, TRUE, 0);
	default_index = scan_devices("Audio devices:", audio_device_option,
				     configure_win_audio_dev_cb,
				     sun_cfg.audio_device);
	audio_device = sun_cfg.audio_device;
	if (default_index >= 0)
		gtk_option_menu_set_history(GTK_OPTION_MENU(audio_device_option),
					    default_index);

	audio_device_always_audiodev = gtk_check_button_new_with_label(_("Always use AUDIODEV environment variable"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_device_always_audiodev),
				     sun_cfg.always_audiodev == 1);
	gtk_box_pack_start(GTK_BOX(audio_device_box), audio_device_always_audiodev,
			    TRUE, TRUE, 0);

	port_frame = gtk_frame_new(_("Output ports:"));
	
	gtk_box_pack_start(GTK_BOX(audio_vbox), port_frame,
			   FALSE, FALSE, 0);

	port_vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(port_frame), port_vbox);

	gtk_container_set_border_width(GTK_CONTAINER(port_vbox), 5);

	port_chk_lineout = gtk_check_button_new_with_label(_("Line out"));
	port_chk_headphones = gtk_check_button_new_with_label(_("Headphones"));
	port_chk_speaker = gtk_check_button_new_with_label(_("Internal speaker"));
	gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_lineout, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_headphones, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_speaker, FALSE, FALSE, 0);

	/*
	 * Set buttons to correct state
	 * First, get current state of buttons in case they've been changed by
	 * eg audiocontrol
	 */
	if (abuffer_isopen())
		abuffer_update_dev();

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(port_chk_lineout),
				    !!(sun_cfg.channel_flags & AUDIO_LINE_OUT));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(port_chk_headphones), 
				    !!(sun_cfg.channel_flags & AUDIO_HEADPHONE));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(port_chk_speaker),
				    !!(sun_cfg.channel_flags & AUDIO_SPEAKER));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 audio_vbox, gtk_label_new(_("Devices")));

	buffer_frame = gtk_frame_new(_("Buffering:"));
	gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);

	buffer_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);

	buffer_table = gtk_table_new(1, 2, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
	gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);

	buffer_size_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_size_box, 0, 1, 0, 1);
	buffer_size_label = gtk_label_new(_("Buffer size (ms):"));
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_label, FALSE, FALSE, 0);
	buffer_size_adj = gtk_adjustment_new(sun_cfg.buffer_size, 100, 10000, 100, 100, 100);
	buffer_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj), 8, 0);
	gtk_widget_set_usize(buffer_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_spin, FALSE, FALSE, 0);
    
	buffer_pre_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_pre_box, 1, 2, 0, 1);
	buffer_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_label, FALSE, FALSE, 0);
	buffer_pre_adj = gtk_adjustment_new(sun_cfg.prebuffer, 0, 90, 1, 1, 1);
	buffer_pre_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_pre_adj), 1, 0);
	gtk_widget_set_usize(buffer_pre_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_spin, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buffer_frame, gtk_label_new(_("Buffering")));

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
    
	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(configure_win_apply_cb), NULL);
	gtk_signal_connect_object(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	apply = gtk_button_new_with_label(_("Apply"));
	gtk_signal_connect(GTK_OBJECT(apply), "clicked", GTK_SIGNAL_FUNC(configure_win_apply_cb), NULL);
	GTK_WIDGET_SET_FLAGS(apply, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), apply, TRUE, TRUE, 0);
	
	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
	
	gtk_widget_show_all(configure_win);
}

