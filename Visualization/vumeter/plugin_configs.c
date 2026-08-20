#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "libxmms/configfile.h"
#include "plugin_main.h"
#include "plugin_helper.h"
#include "plugin_skin.h"

/**************************************************************************n
                            External variables
*****************************************************************************/
extern GArray		*plugin_skin_data,
			*plugin_skin_list;

extern gint		num_of_windows,
			num_of_samples,
			decay_pct,
			data_source,
			target_fps,
			plugin_initialized;

/****************************************************************************
                            Save configuration
*****************************************************************************/
void vumeter_save_configuration(vumeter_window *winlist)
{
	ConfigFile	*db;
	gint		i,l1,snum;
	gchar		*b64_str;
	char		tmp1[200],
			tmp2[200];

	// ??
	if(plugin_initialized!=1)
	{
		DEBUG( printf("VUMETER: Why am I here?!\n"); );
		DEBUG( printf("VUMETER: Refusing to save configuration, plugin has not been initialized!\n"); );
		return;
	}

	// Get window positions
	for(i=0; i<MAX_INSTANCES; i++)
	if(winlist[i].win!=NULL)
	{
		gdk_window_get_root_origin( winlist[i].win->window , &winlist[i].xpos, &winlist[i].ypos );
	}

	db = xmms_cfg_open_default_file();
	if(!db)
	{
		DEBUG( printf("VUMETER: Unable to save configuration\n"); );
		return;
	}

	DEBUG( printf("VUMETER: Saving configuration\n"); );

	xmms_cfg_write_int(db, "analog_vumeter", "num_of_windows", num_of_windows);
	DEBUG( printf("VUMETER:   'num_of_windows'=%d\n",num_of_windows); );

	xmms_cfg_write_int(db, "analog_vumeter", "num_of_samples", num_of_samples);
	DEBUG( printf("VUMETER:   'num_of_samples'=%d\n",num_of_samples); );

	xmms_cfg_write_int(db, "analog_vumeter", "data_source", data_source);
	DEBUG( printf("VUMETER:   'data_source'=%d\n",data_source); );

	xmms_cfg_write_int(db, "analog_vumeter", "target_fps", target_fps);
	DEBUG( printf("VUMETER:   'target_fps'=%d\n",target_fps); );

	xmms_cfg_write_int(db, "analog_vumeter", "decay_pct", decay_pct);
	DEBUG( printf("VUMETER:   'decay_pct'=%d\n",decay_pct); );

	for(i=0,l1=0; i<MAX_INSTANCES; i++)
	if(winlist[i].win!=NULL)
	{
		DEBUG( printf("VUMETER:   Win %d pos: %d , %d\n",l1,winlist[i].xpos,winlist[i].ypos); );
		snum = winlist[i].skin_num-1;

		b64_str = vumeter_base64_encode( (guchar *)&g_array_index(plugin_skin_data, vumeter_skin, snum).skin_name,
					   strlen(g_array_index(plugin_skin_data, vumeter_skin, snum).skin_name) );

		snprintf(tmp1,200,"window_%d",l1);
		snprintf(tmp2,200,"%d,%d,%d,%s",winlist[i].xpos,winlist[i].ypos,
						g_array_index(plugin_skin_data, vumeter_skin, snum).pathnum,
						b64_str);
		xmms_cfg_write_string(db, "analog_vumeter", tmp1, tmp2);
		g_free(b64_str);
		l1++;
	}

	xmms_cfg_write_default_file(db);
	xmms_cfg_free(db);
}

/****************************************************************************
                            Load configuration
*****************************************************************************/
void vumeter_load_configuration(vumeter_window *winlist)
{
	ConfigFile *db;
	gchar	**tmp3;
	gint 	l1,l2,
		t_pnum,
		skin_num;
	gchar	*b64_str;
	gint	b64_len=0;
	char	tmp1[200],
		t_sname[250],
		*tmp2;

	db = xmms_cfg_open_default_file();
	if(!db)
	{
		DEBUG( printf("VUMETER: Unable to load configuration\n"); );
		return;
	}

	// Load settings
	DEBUG( printf("VUMETER: Loading configuration...\n"); );

	xmms_cfg_read_int(db, "analog_vumeter", "num_of_samples", &num_of_samples);
	if(num_of_samples<1 || num_of_samples>10)
		num_of_samples=1;

	xmms_cfg_read_int(db, "analog_vumeter", "target_fps", &target_fps);
	if(target_fps<25 || target_fps>50)
		target_fps=25;

	xmms_cfg_read_int(db, "analog_vumeter", "data_source", &data_source);
	if(data_source!=1 && data_source!=2)
		data_source=1;

	xmms_cfg_read_int(db, "analog_vumeter", "decay_pct", &decay_pct);
	if(decay_pct<1 || decay_pct>90)
		decay_pct=30;

	xmms_cfg_read_int(db, "analog_vumeter", "num_of_windows", &num_of_windows);
	if(num_of_windows <= 0 || num_of_windows > MAX_INSTANCES)
	{
		DEBUG( printf("VUMETER:   Value of 'num_of_windows' out of range (%d)\n",num_of_windows); );

		// Value out of range... set defaults for now
		num_of_windows=1;
	}

	DEBUG( printf("VUMETER:   Trying to read settings for %d window(s) \n",num_of_windows); );

	// Read window positions and skins
	for(l1=0; l1<num_of_windows; l1++)
	{
		// Set defaults
		t_pnum=-1;
		skin_num=-1;
		winlist[l1].xpos=0;
		winlist[l1].ypos=0;

		// Read config string for window l1
		snprintf(tmp1,200,"window_%d",l1);
		if ( xmms_cfg_read_string(db, "analog_vumeter", tmp1, &tmp2) )
		{
			g_strstrip(tmp2);

			// Does the config string make any sense?
			if(strlen(tmp2)>2 && strlen(tmp2)<200)
			{
				DEBUG( printf("VUMETER:     Found window configs for slot %d \n",l1); );

				// String should be split in 4 parts (xpos, ypos, pathnum, skin_name)
				tmp3=g_strsplit(tmp2,",",4);
				if(tmp3[0]!=NULL && tmp3[1]!=NULL && tmp3[2]!=NULL && tmp3[3]!=NULL)
				{
					l2 = atoi(tmp3[0]);
					if(l2<0) l2=0;
					winlist[l1].xpos=l2;

					l2 = atoi(tmp3[1]);
					if(l2<0) l2=0;
					winlist[l1].ypos=l2;

					l2 = atoi(tmp3[2]);
					if(l2<0) l2=0;
					t_pnum=l2;

					b64_str = (gchar *)vumeter_base64_decode(tmp3[3],&b64_len);
					strncpy(t_sname,b64_str,249);
					t_sname[249]=0;
					g_free(b64_str);

					DEBUG( printf("VUMETER:     - Pos (%dx%d) ; pnum: %d ; skin: %s \n",
						winlist[l1].xpos,winlist[l1].ypos,t_pnum,t_sname); );
				}
				g_strfreev(tmp3);
			}

			g_free(tmp2);
		}

		// Try to load last skin
		if(t_pnum!=-1)
		{
			skin_num = vumeter_load_skin(t_pnum,t_sname);
		}

		// Load fallback skin
		if(t_pnum==-1 || skin_num==0)
		{
			DEBUG( printf("VUMETER:     Loading fallback skin, for win %d \n",l1); );
			skin_num=vumeter_load_skin(	g_array_index(plugin_skin_list, plugin_sl_el, 0).pathnum,
							g_array_index(plugin_skin_list, plugin_sl_el, 0).dirname);
		}

		winlist[l1].skin_num = skin_num;
	}

	xmms_cfg_free(db);
}
