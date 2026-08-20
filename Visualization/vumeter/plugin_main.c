#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "xmms/plugin.h"
#include "xmms/dock.h"

#include "plugin_icon.xpm"

#include "config.h"

#include "plugin_main.h"
#include "plugin_dialogs.h"
#include "plugin_skin.h"
#include "plugin_helper.h"
#include "plugin_worker.h"

/****************************************************************************
                        External variables / functions
*****************************************************************************/
extern GList	*dock_window_list;

/**************************************************************************n
                            Local variables
*****************************************************************************/
GdkPixmap 	*pluginIcon	= NULL;
GdkBitmap	*pluginIconMask	= NULL;

pthread_t	vumeter_thread1;
gboolean	vumeter_thread_running = FALSE;

gint		num_of_windows		= -1,
		num_of_samples		= 3,
		data_source		= 1,
		target_fps		= 25,
		decay_pct		= 10,
		plugin_initialized	= 0,
		worker_can_quit		= 1,
		worker_state		= 0,

		devmode_enabled		= 0;

float 		devmode_left_value	= -100,
		devmode_right_value	= -100;

gint16		shared_data[2][512];

// Windows
vumeter_window	plugin_win[MAX_INSTANCES];

// List of skins that directory scanner found (name + dirnum)
GArray		*plugin_skin_list	= NULL;

// Loaded skins (settings, image pointers, etc..)
GArray		*plugin_skin_data	= NULL;

extern float	rms_values[3],
		peak_values[3];

/****************************************************************************
                            Function definitions
*****************************************************************************/
void      win_click_event   (GtkWidget *, GdkEventButton *, gpointer);
void      win_release_event (GtkWidget *, GdkEventButton *, gpointer);
void      win_motion_event  (GtkWidget *, GdkEventMotion *, gpointer);
void      win_focus_event   (GtkWidget *, GdkEventFocus *, gpointer);
void	  win_close_event   (GtkObject *, gpointer);
gboolean  expose_cb         (GtkWidget *, GdkEventExpose *, gpointer);

void      vumeter_render(gint16 data[2][512]);
void      vumeter_play(void);
void      vumeter_pause(void);
void      vumeter_init(void);
void      vumeter_cleanup(void);

void      vumeter_save_configuration(vumeter_window *);
void      vumeter_load_configuration(vumeter_window *);

gint vumeter_error_timer( gpointer );

VisPlugin *get_vplugin_info(void);

/****************************************************************************
                            Plugin definition
*****************************************************************************/
VisPlugin vumeter_vp =
{
	.description = "Analog VU meter " VERSION,

        .num_pcm_chs_wanted = 2,
	.num_freq_chs_wanted = 0,

	.about = vumeter_about,
	.configure = vumeter_config,

        .init = vumeter_init,
	.cleanup = vumeter_cleanup,
	.playback_start = vumeter_play,
	.playback_stop = vumeter_pause,
	.render_pcm = vumeter_render
};

VisPlugin *get_vplugin_info(void) { return &vumeter_vp; }

/****************************************************************************
                            Render function
*****************************************************************************/
void vumeter_render(gint16 data[2][512])
{
	if(worker_state!=0) return;
	memcpy(shared_data,data,sizeof(gint16)*2*512);
	worker_state=1;
}

/****************************************************************************
                        Play / Pause functions
*****************************************************************************/
void vumeter_play(void)
{
}

void vumeter_pause(void)
{
}

/****************************************************************************
                Static background/titlebar raster (per window)
*****************************************************************************/
void vumeter_window_init(gint inum,gboolean focused)
{
	vumeter_skin   	*skin;
	vumeter_image	*titleimg;
	gint	 	snum=plugin_win[inum].skin_num-1,
			tbar_height=0,
			rowstride;

	skin = &g_array_index(plugin_skin_data, vumeter_skin, snum);

	titleimg = focused ? skin->img_titlebar_on : skin->img_titlebar_off;
	if(titleimg == NULL)
		titleimg = skin->img_titlebar_off;

	rowstride = plugin_win[inum].width * 3;
	memset(plugin_win[inum].bg_buf, 0, rowstride * plugin_win[inum].height);

	if(titleimg != NULL)
	{
		tbar_height = titleimg->height;
		vumeter_image_composite(	plugin_win[inum].bg_buf, plugin_win[inum].width, plugin_win[inum].height,
						rowstride, titleimg, 0, 0);
	}

	// Draw background image below the titlebar
	if(skin->img_background != NULL)
	{
		vumeter_image_composite(	plugin_win[inum].bg_buf, plugin_win[inum].width, plugin_win[inum].height,
						rowstride, skin->img_background, 0, tbar_height);
	}
}

/****************************************************************************
                       Per-frame composite + redraw
*****************************************************************************/
void vumeter_redraw_window(vumeter_window *ptr)
{
	vumeter_skin   	*skin;
	vumeter_module 	*module;
	gint		snum,l1,l2,
			rowstride,
			t_x,t_y;
	float		tmp_val=0.0,
			angle=0.0,
			rad_tmp = (M_PI/180.0);

	if(ptr==NULL || ptr->win==NULL)	return;

	snum 		= ptr->skin_num-1;
	skin 		= &g_array_index(plugin_skin_data, vumeter_skin, snum);
	rowstride	= ptr->width * 3;

	// Start from the static background+titlebar
	memcpy(ptr->frame_buf, ptr->bg_buf, rowstride * ptr->height);

	// Composite image (LED) layers, in layer order
	for(l1=1; l1<=5; l1++)
	for(l2=0; l2<skin->modules->len; l2++)
	{
		module = &g_array_index(skin->modules,vumeter_module,l2);
		if(module->enabled==0) continue;
		if(module->layer!=l1) continue;
		if(module->type!=2) continue;
		if(module->channel<0 || module->channel>2) continue;

		if(data_source==1)	tmp_val = rms_values[module->channel];
		else 			tmp_val = peak_values[module->channel];

		if(tmp_val >= module->db_min && tmp_val<= module->db_max)
			vumeter_image_composite(ptr->frame_buf, ptr->width, ptr->height, rowstride,
						 module->on_img, module->position[0], module->position[1]);
		else
			vumeter_image_composite(ptr->frame_buf, ptr->width, ptr->height, rowstride,
						 module->off_img, module->position[0], module->position[1]);
	}

	// Blit composited raster to the backing pixmap
	gdk_draw_rgb_image(	ptr->pixmap, ptr->pen, 0, 0, ptr->width, ptr->height,
				GDK_RGB_DITHER_NORMAL, ptr->frame_buf, rowstride);

	// Draw the analog needles on top, in the same layer order
	for(l1=1; l1<=5; l1++)
	for(l2=0; l2<skin->modules->len; l2++)
	{
		module = &g_array_index(skin->modules,vumeter_module,l2);
		if(module->enabled==0) continue;
		if(module->layer!=l1) continue;
		if(module->type!=1) continue;
		if(module->channel<0 || module->channel>2) continue;

		if(data_source==1)	tmp_val = rms_values[module->channel];
		else 			tmp_val = peak_values[module->channel];

		if ( tmp_val <= module->db_min )	angle=module->angle_min;
		else if ( tmp_val >= module->db_max )	angle=module->angle_max;
		else {
			angle = log10(fabs(tmp_val-1.0))/log10(fabs(module->db_min)+1.0);
			angle = module->angle_range*(1.0-angle)+module->angle_min;
		}

		t_x = module->position[0] + floor( (float)module->radius * cos ( ( 90.0 + angle ) * rad_tmp ) );
		t_y = module->position[1] + floor( (float)module->radius * sin ( ( 90.0 + angle ) * rad_tmp ) );

		gdk_gc_set_line_attributes( ptr->pen, module->width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER );
		gdk_rgb_gc_set_foreground( ptr->pen, module->color );
		gdk_draw_line(	ptr->pixmap, ptr->pen,
				module->position[0], module->position[1], t_x, t_y );
	}

	// pixmap to window
	gdk_draw_pixmap(	ptr->win->window, ptr->pen, ptr->pixmap,
				0,0,0,0, ptr->width,ptr->height);
}

/****************************************************************************
                      Function to create one plugin window
*****************************************************************************/
GtkWidget *vumeter_create_window(gint inum, gint skin_num)
{
	gint		width=g_array_index(plugin_skin_data, vumeter_skin, skin_num-1).width,
			height=g_array_index(plugin_skin_data, vumeter_skin, skin_num-1).height;
	GtkWidget	*newWin=NULL;

	// Create new window, set title, and disable resize
	newWin = gtk_window_new  ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title	 ( GTK_WINDOW(newWin), "Analog VU meter");
	gtk_window_set_policy	 ( GTK_WINDOW(newWin), FALSE, FALSE, FALSE );

	// Realize widget
	gtk_widget_realize(newWin);

	// Disable decorations
	gdk_window_set_decorations( newWin->window, 0 );

	// Add focus, move and button events to window
	gtk_widget_add_events	( GTK_WIDGET(newWin), GDK_BUTTON_PRESS_MASK |
	                           GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK | GDK_FOCUS_CHANGE_MASK);

	// Add signal handlers

	gtk_signal_connect(	GTK_OBJECT(newWin), "destroy",
				GTK_SIGNAL_FUNC(win_close_event), &plugin_win[inum] );

	gtk_signal_connect(	GTK_OBJECT(newWin), "button_press_event",
				GTK_SIGNAL_FUNC(win_click_event), &plugin_win[inum] );

	gtk_signal_connect(	GTK_OBJECT(newWin), "button_release_event",
				GTK_SIGNAL_FUNC(win_release_event), &plugin_win[inum] );

	gtk_signal_connect(	GTK_OBJECT(newWin), "motion_notify_event",
				GTK_SIGNAL_FUNC(win_motion_event), &plugin_win[inum] );

	gtk_signal_connect(	GTK_OBJECT(newWin), "focus_in_event",
				GTK_SIGNAL_FUNC(win_focus_event), &plugin_win[inum] );
	gtk_signal_connect(	GTK_OBJECT(newWin), "focus_out_event",
				GTK_SIGNAL_FUNC(win_focus_event), &plugin_win[inum] );

	gtk_signal_connect(	GTK_OBJECT (newWin), "expose_event",
				GTK_SIGNAL_FUNC (expose_cb), &plugin_win[inum]);

	// Set widget size and add icon to window
	if(pluginIcon==NULL)
	{
		pluginIcon = gdk_pixmap_create_from_xpm_d(	newWin->window, &pluginIconMask,
								&newWin->style->bg[GTK_STATE_NORMAL], plugin_icon_xpm);
	}
	gdk_window_set_icon( newWin->window, NULL, pluginIcon, pluginIconMask );
	gtk_widget_set_usize ( GTK_WIDGET(newWin) , width, height);

	// Move window to it's position
	if(plugin_win[inum].xpos>0 && plugin_win[inum].ypos>0)
	{
		gdk_window_move ( newWin->window , plugin_win[inum].xpos, plugin_win[inum].ypos);
	}

	plugin_win[inum].win      = newWin;

	// Fill structure
	plugin_win[inum].skin_num = skin_num;
	plugin_win[inum].width    = width;
	plugin_win[inum].height   = height;
	plugin_win[inum].slot_num = inum;

	// Create backing pixmap + GC
	plugin_win[inum].pixmap	= gdk_pixmap_new( newWin->window,width,height,-1 );
	plugin_win[inum].pen    = gdk_gc_new ( newWin->window );

	// Allocate raster buffers
	plugin_win[inum].bg_buf    = g_malloc0( width*height*3 );
	plugin_win[inum].frame_buf = g_malloc0( width*height*3 );

	// Draw initial contents *before* the window is mapped, so there is
	// no visible flash of an undrawn (gray) window on first show
	vumeter_window_init(inum,FALSE);
	vumeter_redraw_window(&plugin_win[inum]);

	// Show widget, and all it's sub widgets
	gtk_widget_show_all ( GTK_WIDGET(newWin) );

	// Join the dock (so the window drags/snaps together with the rest of xmms)
	dock_window_list = dock_add_window(dock_window_list, newWin);

	// Update config window
	vumeter_update_window_list();

	// Return pointer to window
	DEBUG( printf("VUMETER: window created (title: %s , size: %d x %d , slot: %d)\n","Analog VU meter",width,height,inum); );
	return(newWin);
}


void vumeter_change_window_skin(gint inum,gint path_num,gchar *skin_name)
{
	gint		snum=0,
			found,l1,l2;
	vumeter_skin   	*skin;

	// Are given parameters valid?
	if(inum<0 || inum>=MAX_INSTANCES) return;
	if(path_num<0 || path_num>1) return;
	if(strlen(skin_name)==0) return;
	if(plugin_win[inum].win==NULL) return;

	// Try to load specified skin
	snum = vumeter_load_skin( path_num, skin_name);
	if( snum == 0 ) return;

	skin = &g_array_index(plugin_skin_data, vumeter_skin, snum-1);

	// Resize window, if needed
	if(skin->width != plugin_win[inum].width || skin->height != plugin_win[inum].height)
	{
		DEBUG( printf("VUMETER: window resized (slot: %d  %dx%d -> %dx%d)\n",inum,
				plugin_win[inum].width,plugin_win[inum].height,skin->width,skin->height); );
		gtk_widget_set_usize ( GTK_WIDGET(plugin_win[inum].win) , skin->width, skin->height);
	}

	// Change window parameters
	plugin_win[inum].skin_num = snum;
	plugin_win[inum].width    = skin->width;
	plugin_win[inum].height   = skin->height;

	// Resize backing pixmap + raster buffers
	gdk_pixmap_unref(plugin_win[inum].pixmap);
	plugin_win[inum].pixmap = gdk_pixmap_new( plugin_win[inum].win->window,skin->width,skin->height,-1 );

	g_free(plugin_win[inum].bg_buf);
	g_free(plugin_win[inum].frame_buf);
	plugin_win[inum].bg_buf    = g_malloc0( skin->width*skin->height*3 );
	plugin_win[inum].frame_buf = g_malloc0( skin->width*skin->height*3 );

	// Redraw
	vumeter_window_init(inum, GTK_WIDGET_HAS_FOCUS(plugin_win[inum].win));
	vumeter_redraw_window(&plugin_win[inum]);

	// Free unused skins (if any)
	for(l1=0; l1<plugin_skin_data->len; l1++)
	{
		skin = &g_array_index(plugin_skin_data, vumeter_skin, l1);
		if(skin->pathnum==-1) continue;

		found = 0;
		for(l2=0; l2<MAX_INSTANCES; l2++)
		if( plugin_win[l2].skin_num == (l1+1) )
		{
			found = 1; break;
		}

		if(found==0)
		{
			DEBUG( printf("VUMETER: Skin (snum: %d , pnum: %d) not in use, freeing memory.\n",
					l1,skin->pathnum););
			vumeter_deinit_skin( &g_array_index(plugin_skin_data, vumeter_skin, l1) );
		}
	}
}

/****************************************************************************
                        Initialization and cleanup
*****************************************************************************/
void vumeter_init(void)
{
	int 	i;

	// Init
	devmode_enabled = 0;
	devmode_left_value = -100.0;
	devmode_right_value = -100.0;
	worker_can_quit = 0;
	worker_state    = 0;

	for(i=0; i<MAX_INSTANCES; i++)
		reset_win_structure(&plugin_win[i]);

	// Look for available skins
	if(vumeter_scan_skin_dirs()==0)
	{
		vumeter_error_dialog("VUMETER: No skins found! Please check you installation.\n");
		gtk_timeout_add(10,vumeter_error_timer,NULL);
		return;
	}

	// Load configuration
	vumeter_load_configuration(plugin_win);

	// Create worker thread
	if( pthread_create(&vumeter_thread1,NULL,vumeter_worker,NULL) != 0 )
	{
		vumeter_error_dialog("VUMETER: Unable to create worker thread :...(\n");
		gtk_timeout_add(10,vumeter_error_timer,NULL);
		return;
	}
	vumeter_thread_running = TRUE;

	// Create requested number of windows, and load skin configuration
	for(i=0; i<num_of_windows; i++)
	{
		// Create window
		if(plugin_win[i].win==NULL)
		{
			if(vumeter_create_window(i,plugin_win[i].skin_num)==NULL)
			{
				printf("VUMETER: Critical error while creating windows!\n");
				gtk_timeout_add(10,vumeter_error_timer,NULL);
				return;
			}
		}
	}

	plugin_initialized = 1;
}

void vumeter_cleanup(void)
{
	gint i;

	DEBUG( printf("VUMETER: Cleanup...\n"); );

	worker_can_quit = 1;

	// Save configuration
	vumeter_save_configuration(plugin_win);

	// Wait for worker thread to quit
	if(vumeter_thread_running)
	{
		pthread_join(vumeter_thread1,NULL);
		vumeter_thread_running = FALSE;
	}

	// Close windows
	for(i=0; i<MAX_INSTANCES; i++)
	if(plugin_win[i].win!=NULL)
		gtk_object_destroy( GTK_OBJECT(plugin_win[i].win) );

	// Free memory
	if(plugin_skin_data != NULL)
	{
		for(i=0; i<plugin_skin_data->len; i++)
			vumeter_deinit_skin( &g_array_index(plugin_skin_data, vumeter_skin, i) );

		g_array_free(plugin_skin_data,TRUE);
	}

	if(pluginIcon!=NULL)
	{
		gdk_pixmap_unref( pluginIcon );
		if(pluginIconMask!=NULL)
			gdk_bitmap_unref( pluginIconMask );
	}

	if(plugin_skin_list != NULL)
		g_array_free(plugin_skin_list,TRUE);

	// Reset values
	pluginIcon		= NULL;
	pluginIconMask		= NULL;
	plugin_skin_data	= NULL;
	plugin_skin_list	= NULL;
	plugin_initialized	= 0;
}

/****************************************************************************
                          Timer callbacks
*****************************************************************************/
gint vumeter_error_timer( gpointer data )
{
	DEBUG( printf("VUMETER: Entered disable function!\n") );
	vumeter_vp.disable_plugin(&vumeter_vp);
	return(0);
}

/****************************************************************************
                          Area expose events
*****************************************************************************/
gboolean expose_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	vumeter_window *ptr = (vumeter_window *)data;

	if(data==NULL || ptr==NULL)	return(FALSE);

	vumeter_redraw_window(ptr);

	return(TRUE);
}

/****************************************************************************
                            Mouse events
*****************************************************************************/
void win_close_event (GtkObject *aObject, gpointer aCallbackData)
{
	vumeter_window *ptr = (vumeter_window *)aCallbackData;
	gint inum = ptr->slot_num;

	// Free memory
	dock_window_list = g_list_remove(dock_window_list, ptr->win);

	gdk_pixmap_unref(plugin_win[inum].pixmap);
	gdk_gc_unref(plugin_win[inum].pen);
	g_free(plugin_win[inum].bg_buf);
	g_free(plugin_win[inum].frame_buf);

	// Reset structure
	reset_win_structure(&plugin_win[inum]);

	// ...
	num_of_windows--;
	vumeter_update_window_list();

	DEBUG( printf("VUMETER: window closed (slot: %d)\n",inum); );
}

void win_click_event(GtkWidget *aWidget, GdkEventButton *aEvent, gpointer aCallbackData)
{

	vumeter_window *ptr = (vumeter_window *)aCallbackData;
	vumeter_skin   *skin;
	gint		snum,tbar_height;

	// We only care about left mouse button
	if( aEvent->button != 1 ) return;
	//
	snum 		= ptr->skin_num-1;
	skin 		= &g_array_index(plugin_skin_data, vumeter_skin, snum);
	tbar_height	= (skin->img_titlebar_on!=NULL) ? skin->img_titlebar_on->height : 0;

	// Button has been pressed
	if( aEvent->type == GDK_BUTTON_PRESS )
	{
		if(aEvent->x >= skin->exit_button_pos[0][0] && aEvent->y >= skin->exit_button_pos[0][1] &&
		   aEvent->x <= skin->exit_button_pos[1][0] && aEvent->y <= skin->exit_button_pos[1][1])
		{
			DEBUG( printf("VUMETER: Click on windows (id: %d ) exit button!\n",ptr->slot_num) );
			if(num_of_windows==1)
			{
				gtk_timeout_add(10,vumeter_error_timer,NULL);
			}
			gtk_object_destroy( GTK_OBJECT(ptr->win) );
			return;
		}

		if(aEvent->x >= skin->conf_button_pos[0][0] && aEvent->y >= skin->conf_button_pos[0][1] &&
		   aEvent->x <= skin->conf_button_pos[1][0] && aEvent->y <= skin->conf_button_pos[1][1])
		{
			vumeter_config();
			return;
		}

		if(aEvent->y >=0 && aEvent->y < tbar_height )
		{
			dock_move_press(dock_window_list, aWidget, aEvent, TRUE);
		}
	}

}

void win_release_event(GtkWidget *aWidget, GdkEventButton *aEvent, gpointer aCallbackData)
{
	if(dock_is_moving(aWidget))
		dock_move_release(aWidget);
}

void win_motion_event(GtkWidget *aWidget, GdkEventMotion *aEvent, gpointer aCallbackData)
{
	if(dock_is_moving(aWidget))
		dock_move_motion(aWidget, aEvent);
}

void win_focus_event(GtkWidget *aWidget, GdkEventFocus *aEvent, gpointer aCallbackData)
{
	vumeter_window *ptr = (vumeter_window *)aCallbackData;

	// If (true) then window got focus :)
	vumeter_window_init(ptr->slot_num, aEvent->in ? TRUE : FALSE);
	vumeter_redraw_window(ptr);
}
