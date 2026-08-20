#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "libxmms/util.h"
#include "plugin_dialogs.h"
#include "plugin_main.h"
#include "plugin_skin.h"

extern vumeter_window	plugin_win[MAX_INSTANCES];

extern GArray		*plugin_skin_data,
			*plugin_skin_list;

extern gint		num_of_windows,
			num_of_samples,
			target_fps,
			data_source,
			decay_pct,
			plugin_initialized,

			devmode_enabled;

extern float		devmode_left_value,
			devmode_right_value;

gint			tmp_num_of_samples,
			tmp_decay_pct,
			tmp_target_fps;

static gboolean		ignore_slist_select = FALSE;

GtkObject		*conf_hadj1,		// FPS
			*conf_hadj2,		// Averaging
			*conf_hadj3,		// Decay

			*dev_hadj1,		// Left channel
			*dev_hadj2;		// Right channel

GtkWidget		*aboutWin=NULL,
			*configWin=NULL,

			*conf_entry_1,
			*conf_entry_2,
			*conf_entry_3,

			*dev_entry_1,
			*dev_entry_2,

			*tmp_rb1,
			*tmp_rb2;

GtkWidget		*clist_1=NULL,		// Plugin windows
			*clist_2=NULL;		// Available skins

/********************************************************************
 * Function to update window list to config window
 ********************************************************************/
void vumeter_wlist_select_first(void)
{
	if(GTK_CLIST(clist_1)->rows > 0)
		gtk_clist_select_row(GTK_CLIST(clist_1),0,0);
}

void vumeter_update_window_list(void)
{
	gint	i,row;
	gchar	tmp[10];
	gchar	*rowtext[1];

	if(configWin==NULL || clist_1==NULL)
		return;

	rowtext[0]=tmp;

	gtk_clist_freeze(GTK_CLIST(clist_1));
	gtk_clist_clear(GTK_CLIST(clist_1));

	for(i=0; i<MAX_INSTANCES; i++)
	if(plugin_win[i].win!=NULL)
	{
		snprintf(tmp,10,"%d",i+1);
		row = gtk_clist_append(GTK_CLIST(clist_1),rowtext);
		gtk_clist_set_row_data(GTK_CLIST(clist_1), row, GINT_TO_POINTER(i));
	}

	gtk_clist_thaw(GTK_CLIST(clist_1));
}

void vumeter_update_skin_list(void)
{
	guint		i;
	plugin_sl_el	*t_ptr;
	gchar		*t_txt;
	gchar		name_buf[257];
	gchar		*rowtext[2];

	if(configWin==NULL || clist_2==NULL)
		return;

	gtk_clist_freeze(GTK_CLIST(clist_2));
	gtk_clist_clear(GTK_CLIST(clist_2));

	for(i=0; i<plugin_skin_list->len; i++)
	{
		t_ptr = &g_array_index(plugin_skin_list,plugin_sl_el,i);

		if(t_ptr->pathnum == 1)	t_txt="local";
		else			t_txt="global";

		strncpy(name_buf,t_ptr->dirname,256);
		name_buf[256]=0;

		rowtext[0]=t_txt;
		rowtext[1]=name_buf;

		gtk_clist_append(GTK_CLIST(clist_2),rowtext);
	}

	gtk_clist_thaw(GTK_CLIST(clist_2));
}

/********************************************************************
 * Functions to handle clist row selection
 ********************************************************************/
static void vumeter_select_skin_for_window(gint slot_num)
{
	guint		i;
	vumeter_skin	*skin;
	plugin_sl_el	*t_ptr;

	if(slot_num<0)
	{
		ignore_slist_select = TRUE;
		gtk_clist_unselect_all(GTK_CLIST(clist_2));
		ignore_slist_select = FALSE;
		return;
	}

	skin = &g_array_index(plugin_skin_data, vumeter_skin, plugin_win[slot_num].skin_num-1);

	for(i=0; i<plugin_skin_list->len; i++)
	{
		t_ptr = &g_array_index(plugin_skin_list, plugin_sl_el, i);
		if(t_ptr->pathnum == skin->pathnum && strcmp(t_ptr->dirname,skin->skin_name)==0)
		{
			ignore_slist_select = TRUE;
			gtk_clist_select_row(GTK_CLIST(clist_2), i, 0);
			ignore_slist_select = FALSE;
			return;
		}
	}

	ignore_slist_select = TRUE;
	gtk_clist_unselect_all(GTK_CLIST(clist_2));
	ignore_slist_select = FALSE;
}

static void vumeter_handle_wlist_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	gint slot_num = GPOINTER_TO_INT( gtk_clist_get_row_data(clist, row) );
	vumeter_select_skin_for_window(slot_num);
}

static void vumeter_handle_wlist_unselect(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	if(clist->selection == NULL)
		vumeter_select_skin_for_window(-1);
}

static void vumeter_handle_slist_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	gint		slot_num,
			path_num;
	plugin_sl_el	*t_ptr;

	if(ignore_slist_select)
		return;

	if(GTK_CLIST(clist_1)->selection == NULL)
		return;

	slot_num = GPOINTER_TO_INT( gtk_clist_get_row_data(GTK_CLIST(clist_1),
				GPOINTER_TO_INT(GTK_CLIST(clist_1)->selection->data)) );

	t_ptr = &g_array_index(plugin_skin_list, plugin_sl_el, row);
	path_num = t_ptr->pathnum;

	vumeter_change_window_skin(slot_num, path_num, t_ptr->dirname);
}

/********************************************************************
 * Function to handle buttons
 ********************************************************************/
static void vumeter_handle_button_1(GtkWidget *widget, gpointer ptr)
{
	int		i,snum;

	// Can't create more instances :/
	if(num_of_windows>=MAX_INSTANCES)
	{
		xmms_show_message("VU meter", "Can't create more instances", "OK", TRUE, NULL, NULL);
		return;
	}

	// Create new window
	for(i=0; i<MAX_INSTANCES; i++)
	if(plugin_win[i].win==NULL)
	{
		snum = vumeter_load_skin(	g_array_index(plugin_skin_list, plugin_sl_el, 0).pathnum,
						g_array_index(plugin_skin_list, plugin_sl_el, 0).dirname);

		if(vumeter_create_window(i,snum)==NULL)
		{
			printf("VUMETER: Critical error while creating windows!\n");
			return;
		}

		num_of_windows++;
		return;
	}
}

static void vumeter_handle_button_2(GtkWidget *widget, gpointer ptr)
{
	GtkCList	*clist = GTK_CLIST(clist_1);
	gint		value=-1;

	// Do nothing if no rows have been selected
	if( clist->selection == NULL )
		return;

	value = GPOINTER_TO_INT( gtk_clist_get_row_data(clist, GPOINTER_TO_INT(clist->selection->data)) );

	if(value>=0)
	{
		if(num_of_windows==1)
		{
			xmms_show_message("VU meter",
					"This is the last window, please disable\nplugin instead, if you wish to close it",
					"OK", TRUE, NULL, NULL);
			return;
		}

		gtk_object_destroy( GTK_OBJECT(plugin_win[value].win) );
	}
}

// Refresh skin list
static void vumeter_handle_button_3(GtkWidget *widget, gpointer ptr)
{
	// Rescan skin directories
	vumeter_scan_skin_dirs();

	// Repopulate skin list
	vumeter_update_skin_list();

	// Reselect the current skin.. if any
	if(GTK_CLIST(clist_1)->selection != NULL)
	{
		gint slot_num = GPOINTER_TO_INT( gtk_clist_get_row_data(GTK_CLIST(clist_1),
					GPOINTER_TO_INT(GTK_CLIST(clist_1)->selection->data)) );
		vumeter_select_skin_for_window(slot_num);
	}
}

// Apply changes
static void vumeter_handle_button_4(GtkWidget *widget, gpointer ptr)
{
	target_fps	= tmp_target_fps;
	num_of_samples	= tmp_num_of_samples;
	decay_pct	= tmp_decay_pct;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp_rb1))==TRUE)	data_source=1;
	else									data_source=2;
}

// Reset values
static void vumeter_handle_button_5(GtkWidget *widget, gpointer ptr)
{
	char tmp[30];

	tmp_target_fps		= target_fps;
	snprintf(tmp,30,"%d",target_fps);
	gtk_entry_set_text(GTK_ENTRY(conf_entry_1),tmp);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(conf_hadj1),(gdouble)target_fps);

	tmp_num_of_samples	= num_of_samples;
	snprintf(tmp,30,"%d",num_of_samples);
	gtk_entry_set_text(GTK_ENTRY(conf_entry_2),tmp);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(conf_hadj2),(gdouble)num_of_samples);

	tmp_decay_pct		= decay_pct;
	snprintf(tmp,30,"%d",decay_pct);
	gtk_entry_set_text(GTK_ENTRY(conf_entry_3),tmp);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(conf_hadj3),(gdouble)decay_pct);

	if(data_source==1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_rb1),TRUE);
	if(data_source==2)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_rb2),TRUE);
}


/********************************************************************
 * Functions to handle hscale adjustments
 ********************************************************************/
static void vumeter_handle_hscale_adj(GtkAdjustment *adj,gpointer data)
{
	int	value1;
	float	value2;
	char	tmp1[30],
		tmp2[30];

	if( adj->value >= 0.0)	value1 = (int)(adj->value+0.5);
	else			value1 = (int)(adj->value-0.5);

	value2 = adj->value;

	snprintf(tmp1,30,"%d",value1);
	snprintf(tmp2,30,"%.01f",value2);

	if(data == conf_hadj1)
	{
		gtk_entry_set_text(GTK_ENTRY(conf_entry_1),tmp1);
		tmp_target_fps = value1;
	} else if( data == conf_hadj2 ) {
		gtk_entry_set_text(GTK_ENTRY(conf_entry_2),tmp1);
		tmp_num_of_samples = value1;
	} else if( data == conf_hadj3 ) {
		gtk_entry_set_text(GTK_ENTRY(conf_entry_3),tmp1);
		tmp_decay_pct = value1;
	} else if( data == dev_hadj1 ) {
		gtk_entry_set_text(GTK_ENTRY(dev_entry_1),tmp2);
		devmode_left_value = value2;
	} else if( data == dev_hadj2 ) {
		gtk_entry_set_text(GTK_ENTRY(dev_entry_2),tmp2);
		devmode_right_value = value2;
	}
}

static void vumeter_handle_checkbox(GtkToggleButton *btn,gpointer data)
{
	if(btn->active==TRUE)
	{
		devmode_enabled=1;
	} else {
		devmode_enabled=0;
	}
}

/********************************************************************
 * Error dialog functions
 ********************************************************************/
void vumeter_error_dialog(char *message)
{
	xmms_show_message("VU meter error", message, "OK", TRUE, NULL, NULL);
}

/********************************************************************
 * About dialog functions
 ********************************************************************/
void vumeter_about(void)
{
	if(aboutWin!=NULL) return;

	aboutWin = xmms_show_message( "About Analog VU meter",
		"Analog VU meter " VERSION "\n"
		"Original Audacious plugin by mcfish\n"
		"http://vumeterplugin.sourceforge.net/\n\n"
		"Ported to GTK+1.2 / xmms for dmms by\n"
		"Rafael \"Dina\" Diniz",
		"Close", FALSE, NULL, NULL);

	gtk_signal_connect( GTK_OBJECT(aboutWin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed),
	                    &aboutWin);
}

/********************************************************************
 * Configure dialog functions
 ********************************************************************/
void vumeter_config(void)
{
	char		tmp_txt[30];
	GtkWidget	*notebook,
			*tmp_label,
			*tmp_frame1,
			*tmp_frame2,
			*tmp_table,
			*tmp_vbox,
			*tmp_hbox,
			*tmp_swin_1,
			*tmp_swin_2,
			*tmp_button,
			*tmp_hbar,
			*tmp_cbox;

	GSList 		*btn_group;

	gchar		*titles_1[1] = { "Slot" };
	gchar		*titles_2[2] = { "Loc", "Name" };

	// Does config window already exist?
	if(configWin!=NULL) return;

	// Causes segfault otherwise :/
	if(plugin_initialized==0) return;

	// Set temp values
	tmp_num_of_samples = num_of_samples;
	tmp_target_fps	   = target_fps;
	tmp_decay_pct      = decay_pct;

	// -- Main window
	configWin = gtk_window_new (GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title(GTK_WINDOW(configWin), "Analog VU meter Configuration");
	gtk_window_set_policy ( GTK_WINDOW(configWin), TRUE, TRUE, FALSE );
	gtk_container_set_border_width(GTK_CONTAINER(configWin),5);
        gtk_widget_set_usize (configWin, 600 , 300);

	gtk_signal_connect(GTK_OBJECT(configWin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configWin);

	// -- Notebook
	notebook = gtk_notebook_new();
	gtk_container_add (GTK_CONTAINER (configWin), notebook);

	// -----------------------------------------------------
	// --  Page 1
	// -----------------------------------------------------
	tmp_label  = gtk_label_new ("Window management");

	tmp_vbox   = gtk_vbox_new(FALSE,0);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tmp_vbox, tmp_label);

	tmp_swin_1 = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_set_border_width(GTK_CONTAINER(tmp_swin_1),5);
	tmp_frame1 = gtk_frame_new ("Plugin windows");
        gtk_container_add(GTK_CONTAINER(tmp_frame1), GTK_WIDGET(tmp_swin_1));

	tmp_swin_2 = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_set_border_width(GTK_CONTAINER(tmp_swin_2),5);
	tmp_frame2 = gtk_frame_new ("Available skins");
        gtk_container_add(GTK_CONTAINER(tmp_frame2), GTK_WIDGET(tmp_swin_2));

	tmp_hbox   = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_frame1, FALSE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_frame2, TRUE, TRUE, 4);

	gtk_box_pack_start (GTK_BOX (tmp_vbox), tmp_hbox, TRUE, TRUE, 4);

	tmp_hbox   = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start (GTK_BOX (tmp_vbox), tmp_hbox, FALSE, TRUE, 4);

	tmp_button = gtk_button_new_with_label("Add window");
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_button, FALSE, TRUE, 4);
	gtk_signal_connect(GTK_OBJECT(tmp_button), "clicked", GTK_SIGNAL_FUNC(vumeter_handle_button_1), NULL);

	tmp_button = gtk_button_new_with_label("Close window");
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_button, FALSE, TRUE, 4);
	gtk_signal_connect(GTK_OBJECT(tmp_button), "clicked", GTK_SIGNAL_FUNC(vumeter_handle_button_2), NULL);

	gtk_box_pack_start (GTK_BOX (tmp_hbox), gtk_vbox_new(FALSE,0), TRUE, TRUE, 4);

	tmp_button = gtk_button_new_with_label("Refresh skin list");
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_button, FALSE, TRUE, 4);
	gtk_signal_connect(GTK_OBJECT(tmp_button), "clicked", GTK_SIGNAL_FUNC(vumeter_handle_button_3), NULL);

	// Show all Plugin windows
	clist_1 = gtk_clist_new_with_titles(1, titles_1);
	gtk_clist_column_titles_passive(GTK_CLIST(clist_1));
	gtk_clist_set_selection_mode(GTK_CLIST(clist_1), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(clist_1), "select_row", GTK_SIGNAL_FUNC(vumeter_handle_wlist_select), NULL);
	gtk_signal_connect(GTK_OBJECT(clist_1), "unselect_row", GTK_SIGNAL_FUNC(vumeter_handle_wlist_unselect), NULL);
	gtk_container_add(GTK_CONTAINER(tmp_swin_1), GTK_WIDGET(clist_1));

	vumeter_update_window_list();

	// Show available skins
	clist_2 = gtk_clist_new_with_titles(2, titles_2);
	gtk_clist_column_titles_passive(GTK_CLIST(clist_2));
	gtk_clist_set_column_width(GTK_CLIST(clist_2), 0, 50);
	gtk_clist_set_selection_mode(GTK_CLIST(clist_2), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(clist_2), "select_row", GTK_SIGNAL_FUNC(vumeter_handle_slist_select), NULL);
	gtk_container_add(GTK_CONTAINER(tmp_swin_2), GTK_WIDGET(clist_2));

	vumeter_update_skin_list();

	// Select first window from list
	vumeter_wlist_select_first();

	// -----------------------------------------------------
	// --  Page 2
	// -----------------------------------------------------
	tmp_label = gtk_label_new ("Preferences");
	tmp_table = gtk_table_new (5,3,FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tmp_table, tmp_label);

	tmp_label  = gtk_label_new ("FPS");
	gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	conf_hadj1  = gtk_adjustment_new(target_fps,25,50,1,5,0);
	tmp_hbar   = gtk_hscale_new(GTK_ADJUSTMENT(conf_hadj1));
	gtk_scale_set_digits(GTK_SCALE(tmp_hbar),0);
	gtk_scale_set_draw_value(GTK_SCALE(tmp_hbar),FALSE);

	conf_entry_1= gtk_entry_new();
	gtk_widget_set_usize(conf_entry_1,30,-1);
	gtk_entry_set_editable   (GTK_ENTRY(conf_entry_1),FALSE);
	snprintf(tmp_txt,30,"%d",target_fps);
	gtk_entry_set_text(GTK_ENTRY(conf_entry_1),tmp_txt);

		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label  ,0,1,0,1,            GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbar   ,1,2,0,1, GTK_EXPAND|GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),conf_entry_1,2,3,0,1,                   0,0, 10,10);

	gtk_signal_connect(GTK_OBJECT(conf_hadj1),"value_changed",GTK_SIGNAL_FUNC(vumeter_handle_hscale_adj),conf_hadj1);

	//
	tmp_label  = gtk_label_new ("Averaging");
	  gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	conf_hadj2  = gtk_adjustment_new(num_of_samples,1,10,1,2,0);
	tmp_hbar   = gtk_hscale_new(GTK_ADJUSTMENT(conf_hadj2));
	  gtk_scale_set_digits(GTK_SCALE(tmp_hbar),0);
	  gtk_scale_set_draw_value(GTK_SCALE(tmp_hbar),FALSE);

	conf_entry_2= gtk_entry_new();
	  gtk_widget_set_usize(conf_entry_2,30,-1);
	  gtk_entry_set_editable   (GTK_ENTRY(conf_entry_2),FALSE);
	  snprintf(tmp_txt,30,"%d",num_of_samples);
	  gtk_entry_set_text(GTK_ENTRY(conf_entry_2),tmp_txt);

		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label   ,0,1,1,2,            GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbar    ,1,2,1,2, GTK_EXPAND|GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),conf_entry_2,2,3,1,2,                   0,0, 10,10);

	gtk_signal_connect(GTK_OBJECT(conf_hadj2),"value_changed",GTK_SIGNAL_FUNC(vumeter_handle_hscale_adj),conf_hadj2);

	//
	tmp_label  = gtk_label_new ("Decay PCT");
	  gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	conf_hadj3  = gtk_adjustment_new(decay_pct,1,90,1,10,0);
	tmp_hbar   = gtk_hscale_new(GTK_ADJUSTMENT(conf_hadj3));
	  gtk_scale_set_digits(GTK_SCALE(tmp_hbar),0);
	  gtk_scale_set_draw_value(GTK_SCALE(tmp_hbar),FALSE);

	conf_entry_3= gtk_entry_new();
	  gtk_widget_set_usize(conf_entry_3,30,-1);
	  gtk_entry_set_editable   (GTK_ENTRY(conf_entry_3),FALSE);
	  snprintf(tmp_txt,30,"%d",decay_pct);
	  gtk_entry_set_text(GTK_ENTRY(conf_entry_3),tmp_txt);

		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label   ,0,1,2,3,            GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbar    ,1,2,2,3, GTK_EXPAND|GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),conf_entry_3,2,3,2,3,                   0,0, 10,10);

	gtk_signal_connect(GTK_OBJECT(conf_hadj3),"value_changed",GTK_SIGNAL_FUNC(vumeter_handle_hscale_adj),conf_hadj3);

	//
	tmp_hbox   = gtk_hbox_new(FALSE,0);
	tmp_label  = gtk_label_new ("Use");
	  gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	tmp_rb1    = gtk_radio_button_new_with_label (NULL, "RMS-values");
	btn_group  = gtk_radio_button_group (GTK_RADIO_BUTTON (tmp_rb1));
	tmp_rb2    = gtk_radio_button_new_with_label (btn_group, "Peak-values");

	if(data_source==1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_rb1),TRUE);
	if(data_source==2)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_rb2),TRUE);

	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_rb1, FALSE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_rb2, FALSE, TRUE, 4);

	gtk_table_attach(GTK_TABLE(tmp_table),tmp_label  ,0,1,3,4,            GTK_FILL,0, 10,10);
	gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbox   ,1,3,3,4,            GTK_FILL,0, 10,10);


	//
	tmp_hbox   = gtk_hbox_new(FALSE,0);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbox  ,0,2,4,5,            GTK_FILL,0, 10,10);
	tmp_button = gtk_button_new_with_label("Apply changes");
		gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_button, FALSE, TRUE, 4);
		gtk_signal_connect(GTK_OBJECT(tmp_button), "clicked",
				GTK_SIGNAL_FUNC(vumeter_handle_button_4), NULL);
	tmp_button = gtk_button_new_with_label("Reset values");
		gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_button, FALSE, TRUE, 4);
		gtk_signal_connect(GTK_OBJECT(tmp_button), "clicked",
				GTK_SIGNAL_FUNC(vumeter_handle_button_5), NULL);


	// -----------------------------------------------------
	// --  Page 3
	// -----------------------------------------------------
	tmp_label = gtk_label_new ("Devel tools");
	tmp_table = gtk_table_new (4,3,FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tmp_table, tmp_label);

	//
	tmp_label  = gtk_label_new ("Enabled");
	gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label   ,0,1,0,1,            GTK_FILL,0, 10,10);

	tmp_cbox   = gtk_check_button_new();
	tmp_label  = gtk_label_new ("");
	tmp_hbox   = gtk_hbox_new(FALSE,0);

		gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_cbox, FALSE, TRUE, 4);
		gtk_box_pack_start (GTK_BOX (tmp_hbox), tmp_label, FALSE, TRUE, 4);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbox    ,1,3,0,1,            GTK_FILL,0, 10,10);

	if(devmode_enabled==1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_cbox),TRUE);

	//
	tmp_label  = gtk_label_new ("Left");
	gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	dev_hadj1  = gtk_adjustment_new(devmode_left_value,-100,0,1,5,0);
	tmp_hbar   = gtk_hscale_new(GTK_ADJUSTMENT(dev_hadj1));
	gtk_scale_set_digits(GTK_SCALE(tmp_hbar),0);
	gtk_scale_set_draw_value(GTK_SCALE(tmp_hbar),FALSE);

	dev_entry_1= gtk_entry_new();
	gtk_widget_set_usize(dev_entry_1,40,-1);
	gtk_entry_set_editable   (GTK_ENTRY(dev_entry_1),FALSE);
	snprintf(tmp_txt,30,"%.01f",devmode_left_value);
	gtk_entry_set_text(GTK_ENTRY(dev_entry_1),tmp_txt);

		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label   ,0,1,1,2,            GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbar    ,1,2,1,2, GTK_EXPAND|GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),dev_entry_1 ,2,3,1,2,                   0,0, 10,10);

	gtk_signal_connect(GTK_OBJECT(tmp_cbox),"toggled",GTK_SIGNAL_FUNC(vumeter_handle_checkbox),NULL);
	gtk_signal_connect(GTK_OBJECT(dev_hadj1),"value_changed",GTK_SIGNAL_FUNC(vumeter_handle_hscale_adj),dev_hadj1);

	//
	tmp_label  = gtk_label_new ("Right");
	gtk_misc_set_alignment(GTK_MISC(tmp_label),1.0,1.0);

	dev_hadj2  = gtk_adjustment_new(devmode_right_value,-100,0,1,5,0);
	tmp_hbar   = gtk_hscale_new(GTK_ADJUSTMENT(dev_hadj2));
	gtk_scale_set_digits(GTK_SCALE(tmp_hbar),0);
	gtk_scale_set_draw_value(GTK_SCALE(tmp_hbar),FALSE);

	dev_entry_2= gtk_entry_new();
	gtk_widget_set_usize(dev_entry_2,40,-1);
	gtk_entry_set_editable   (GTK_ENTRY(dev_entry_2),FALSE);
	snprintf(tmp_txt,30,"%.01f",devmode_right_value);
	gtk_entry_set_text(GTK_ENTRY(dev_entry_2),tmp_txt);

		gtk_table_attach(GTK_TABLE(tmp_table),tmp_label   ,0,1,2,3,            GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),tmp_hbar    ,1,2,2,3, GTK_EXPAND|GTK_FILL,0, 10,10);
		gtk_table_attach(GTK_TABLE(tmp_table),dev_entry_2 ,2,3,2,3,                   0,0, 10,10);

	gtk_signal_connect(GTK_OBJECT(dev_hadj2),"value_changed",GTK_SIGNAL_FUNC(vumeter_handle_hscale_adj),dev_hadj2);

	// -----------------------------------------------------
	// --  Show all widgets
	// -----------------------------------------------------
	gtk_widget_show_all (configWin);
}
