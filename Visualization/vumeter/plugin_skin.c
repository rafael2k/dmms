#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "config.h"
#include "plugin_main.h"
#include "plugin_skin.h"
#include "plugin_helper.h"

/****************************************************************************
                            Variables
*****************************************************************************/
extern GArray	*plugin_skin_list;
extern GArray	*plugin_skin_data;

/******************************************************************
 * Functions to see if global & user directories contain skins
 ******************************************************************/
int vumeter_scan_dir(char *dir,int pathnum)
{
	char		*tmpdir1,
			*tmpdir2;
	DIR 		*directory;
	struct dirent 	*dir_ent;
	struct stat 	statbuf;
	int		skin_cnt=0;
	plugin_sl_el	tmp;

	DEBUG( printf("VUMETER: Scanning directory: %s\n",dir););

	tmp.pathnum = pathnum;

	// Try to open directory
	if( ( directory  = opendir(dir) ) == NULL)
	{
	   DEBUG( printf("VUMETER:   * Unable to open directory\n"););
	   return(0);
	}

	// Process directories; skip '.' and '..'
	DEBUG( printf("VUMETER:   * Found the following skins:\n"););

	while( (dir_ent=readdir(directory)) )
	if( strcmp(dir_ent->d_name,"..")!=0 && strcmp(dir_ent->d_name,".")!=0 )
	{
		if( ( tmpdir1 = malloc( strlen(dir) + strlen(dir_ent->d_name) + 4) ) == NULL)
		{
			DEBUG( printf("VUMETER: Failed to allocate memory (2)!\n"););
			return(0);
		}
		sprintf(tmpdir1,"%s/%s",dir,dir_ent->d_name);

		if( ( tmpdir2 = malloc( strlen(tmpdir1) + 12) ) == NULL)
		{
			free(tmpdir1);
			DEBUG( printf("VUMETER: Failed to allocate memory (3)!\n"););
			return(0);
		}
		sprintf(tmpdir2,"%s/skin.cfg",tmpdir1);

		// Current entry is most likely skin, if entry is directory and
		// it contains regular file called 'skin.cfg' which is atleast
		// 10 bytes or larger
		if(!lstat(tmpdir1,&statbuf) && S_ISDIR(statbuf.st_mode))
		if(!lstat(tmpdir2,&statbuf) && S_ISREG(statbuf.st_mode))
		if(statbuf.st_size>10)
		{
			DEBUG( printf("VUMETER:     - %s\n",dir_ent->d_name););

			strncpy(tmp.dirname, dir_ent->d_name, 256);
			g_array_append_val(plugin_skin_list,tmp);
			skin_cnt++;
		}

		free(tmpdir1);
		free(tmpdir2);
	}

	closedir(directory);

	return(skin_cnt);
}

int vumeter_scan_skin_dirs(void)
{
	char 	*directory;
	int	skins_found=0;

	if(plugin_skin_list != NULL)
		g_array_free(plugin_skin_list,TRUE);

	plugin_skin_list = g_array_new(FALSE,FALSE,sizeof(plugin_sl_el));

	// Scan global skin directory
	directory = g_strconcat(SKINDIR,"/VU_Meter_skins",NULL);
	skins_found+=vumeter_scan_dir(directory,0);
	g_free(directory);

	// Scan users skin directory
	directory = g_strconcat(g_get_home_dir(),"/.xmms/VU_Meter_skins",NULL);
	skins_found+=vumeter_scan_dir(directory,1);
	g_free(directory);

	return(skins_found);
}

/******************************************************************
 * Functions to initialize structures
 ******************************************************************/

void vumeter_deinit_skin(vumeter_skin *skin)
{
	gint		l1;
	vumeter_module *module;

	// Do not deinit empty skins.. bad things happens :)
	if(skin->pathnum==-1)
		return;

	// Free memory
	skin->pathnum=-1;

	if(skin->img_titlebar_on != NULL)
		vumeter_image_free( skin->img_titlebar_on );

	if(skin->img_titlebar_off != NULL)
		vumeter_image_free( skin->img_titlebar_off );

	if(skin->img_background != NULL)
		vumeter_image_free( skin->img_background );

	for(l1=0; l1<skin->modules->len; l1++)
	{
		module = &g_array_index(skin->modules,vumeter_module,l1);

		if(module->on_img != NULL)
		vumeter_image_free( module->on_img );

		if(module->off_img != NULL)
		vumeter_image_free( module->off_img );
	}

	if(skin->modules != NULL)
		g_array_free( skin->modules,TRUE );
}

void vumeter_init_skin(vumeter_skin *skin)
{
	skin->width=275;
	skin->height=116;
	skin->pathnum=0;

	skin->img_titlebar_on=NULL;
	skin->img_titlebar_off=NULL;
	skin->img_background=NULL;

	skin->modules = g_array_new(FALSE,FALSE,sizeof(vumeter_module));

	memset(skin->skin_name,0,255);
}

void vumeter_copy_skin(vumeter_skin *skin_dst,vumeter_skin *skin_src)
{
	skin_dst->width			= skin_src->width;
	skin_dst->height		= skin_src->height;
	skin_dst->pathnum		= skin_src->pathnum;

	skin_dst->img_titlebar_on	= skin_src->img_titlebar_on;
	skin_dst->img_titlebar_off	= skin_src->img_titlebar_off;
	skin_dst->img_background	= skin_src->img_background;

	skin_dst->modules		= skin_src->modules;
	memcpy(skin_dst->skin_name,skin_src->skin_name,255);
}

void vumeter_reset_module(vumeter_module *module)
{
	module->type=0;
	module->enabled=0;
	module->channel=0;
	module->layer=1;
	module->position[0]=0;
	module->position[1]=0;
	module->db_min=-300.0;
	module->db_max=0.0;

	module->on_img=NULL;
	module->off_img=NULL;
}

/******************************************************************
 * Functions to load skin
 ******************************************************************/
int vumeter_ls_helper1(gchar *key,gchar *val,vumeter_skin *skin,char *dir)
{
	gchar	**tmp1,
		*img_loc;
	int	tmp2,tmp3,tmp4,tmp5;

	if(strcasecmp(key,"skin_size")==0)
	{

		tmp1=g_strsplit(val,",",2);
		if(tmp1[0]!=NULL && tmp1[1]!=NULL)
		{
			g_strstrip(tmp1[0]); tmp2 = atoi(tmp1[0]);
			if(tmp2<10 || tmp2>800) tmp2 = 275;
			skin->width = tmp2;

			g_strstrip(tmp1[1]); tmp3 = atoi(tmp1[1]);
			if(tmp3<10 || tmp3>800) tmp3 = 116;
			skin->height = tmp3;
		}
		g_strfreev(tmp1);

	} else if(strcasecmp(key,"exit_button_pos")==0) {

		tmp1=g_strsplit(val,",",4);
		if(tmp1[0]!=NULL && tmp1[1]!=NULL &&
		   tmp1[2]!=NULL && tmp1[3]!=NULL )
		{
			g_strstrip(tmp1[0]); tmp2 = atoi(tmp1[0]);
			g_strstrip(tmp1[1]); tmp3 = atoi(tmp1[1]);
			g_strstrip(tmp1[2]); tmp4 = atoi(tmp1[2]);
			g_strstrip(tmp1[3]); tmp5 = atoi(tmp1[3]);

			// Sanity check
			if(tmp2<0 || tmp2>800) tmp2=0;
			if(tmp3<0 || tmp3>800) tmp3=0;
			if(tmp4<0 || tmp4>800 || tmp4<tmp2) tmp4=tmp2;
			if(tmp5<0 || tmp5>800 || tmp5<tmp3) tmp5=tmp3;

			skin->exit_button_pos[0][0]=tmp2;
			skin->exit_button_pos[0][1]=tmp3;
			skin->exit_button_pos[1][0]=tmp4;
			skin->exit_button_pos[1][1]=tmp5;
		}
		g_strfreev(tmp1);

	} else if(strcasecmp(key,"conf_button_pos")==0) {

		tmp1=g_strsplit(val,",",4);
		if(tmp1[0]!=NULL && tmp1[1]!=NULL &&
		   tmp1[2]!=NULL && tmp1[3]!=NULL )
		{
			g_strstrip(tmp1[0]); tmp2 = atoi(tmp1[0]);
			g_strstrip(tmp1[1]); tmp3 = atoi(tmp1[1]);
			g_strstrip(tmp1[2]); tmp4 = atoi(tmp1[2]);
			g_strstrip(tmp1[3]); tmp5 = atoi(tmp1[3]);

			// Sanity check
			if(tmp2<0 || tmp2>800) tmp2=0;
			if(tmp3<0 || tmp3>800) tmp3=0;
			if(tmp4<0 || tmp4>800 || tmp4<tmp2) tmp4=tmp2;
			if(tmp5<0 || tmp5>800 || tmp5<tmp3) tmp5=tmp3;

			skin->conf_button_pos[0][0]=tmp2;
			skin->conf_button_pos[0][1]=tmp3;
			skin->conf_button_pos[1][0]=tmp4;
			skin->conf_button_pos[1][1]=tmp5;
		}
		g_strfreev(tmp1);

	} else if(strcasecmp(key,"background_img")==0) {

		img_loc = g_strconcat(dir,"/",val,NULL);
		skin->img_background = vumeter_image_load(img_loc);
		if(skin->img_background==NULL)
		{
			printf("VUMETER: Unable to open file: %s\n",img_loc);
		}
		g_free(img_loc);

	} else if(strcasecmp(key,"titlebar_on_img")==0) {

		img_loc = g_strconcat(dir,"/",val,NULL);
		skin->img_titlebar_on = vumeter_image_load(img_loc);
		if(skin->img_titlebar_on==NULL)
		{
			printf("VUMETER: Unable to open file: %s\n",img_loc);
		}
		g_free(img_loc);

	} else if(strcasecmp(key,"titlebar_off_img")==0) {

		img_loc = g_strconcat(dir,"/",val,NULL);
		skin->img_titlebar_off = vumeter_image_load(img_loc);
		if(skin->img_titlebar_off==NULL)
		{
			printf("VUMETER: Unable to open file: %s\n",img_loc);
		}
		g_free(img_loc);

	} else {
		DEBUG( printf("VUMETER: Helper1.. unknown value: %s = %s\n",key,val) );
	}

	return(1);
}

int vumeter_ls_helper2(gchar *key,gchar *val,vumeter_module *module,char *dir)
{
	gchar		**tmp1,
			tmp5[3]={0},
			*img_loc;

	gint		tmp2,
			tmp3;
	gfloat		tmp4;
	guint32		r,g,b;

	if(strcasecmp(key,"type")==0)
	{
		if(strcasecmp(val,"image")==0) {
			module->type=2;
		} else if(strcasecmp(val,"analogvu")==0) {
			module->type=1;
		}
	} else if(strcasecmp(key,"enabled")==0) {

		tmp2 = atoi(val);
		if(tmp2!=1)	module->enabled=0;
		else		module->enabled=1;

	} else if(strcasecmp(key,"channel")==0) {

		tmp2 = atoi(val);
		if(tmp2>=0 && tmp2<=2)	module->channel=tmp2;
		else			module->channel=0;

	} else if(strcasecmp(key,"layer")==0) {

		tmp2 = atoi(val);
		if(tmp2>=1 && tmp2<=5)	module->layer=tmp2;
		else			module->layer=1;

	} else if(strcasecmp(key,"position")==0) {

		tmp1=g_strsplit(val,",",2);
		if(tmp1[0]!=NULL && tmp1[1]!=NULL)
		{
			// X-position
			g_strstrip(tmp1[0]); tmp2 = atoi(tmp1[0]);
			module->position[0]=tmp2;

			// Y-position
			g_strstrip(tmp1[1]); tmp3 = atoi(tmp1[1]);
			module->position[1]=tmp3;
		}
		g_strfreev(tmp1);

	} else if(strcasecmp(key,"on_img")==0) {

		img_loc = g_strconcat(dir,"/",val,NULL);
		module->on_img = vumeter_image_load(img_loc);
		if(module->on_img==NULL)
		{
			printf("VUMETER: Unable to open file: %s\n",img_loc);
		}
		g_free(img_loc);

	} else if(strcasecmp(key,"off_img")==0) {

		img_loc = g_strconcat(dir,"/",val,NULL);
		module->off_img = vumeter_image_load(img_loc);
		if(module->off_img==NULL)
		{
			printf("VUMETER: Unable to open file: %s\n",img_loc);
		}
		g_free(img_loc);

	} else if(strcasecmp(key,"radius")==0) {

		tmp2 = atoi(val);
		if(tmp2<1 || tmp2>100) tmp2=1;
		module->radius = tmp2;

	} else if(strcasecmp(key,"width")==0) {

		tmp2 = atoi(val);
		if(tmp2<1 || tmp2>10) tmp2=1;
		module->width = tmp2;

	} else if(strcasecmp(key,"color")==0) {

		if(strlen(val)!=7 || val[0]!='#')
		{
			module->color = 0x00FFFFFF;
			return(0);
		}

		// Red
		tmp5[0]=val[1]; tmp5[1]=val[2];
		r = strtol(tmp5,NULL,16);

		// Green
		tmp5[0]=val[3]; tmp5[1]=val[4];
		g = strtol(tmp5,NULL,16);

		// Blue
		tmp5[0]=val[5]; tmp5[1]=val[6];
		b = strtol(tmp5,NULL,16);

		module->color = (r<<16) | (g<<8) | b;

	} else if(strcasecmp(key,"min_angle")==0) {

		tmp4 = atof(val);
		if(tmp4<-180.0 || tmp4>180.0) tmp4=-180.0;
		module->angle_min = tmp4;

	} else if(strcasecmp(key,"max_angle")==0) {

		tmp4 = atof(val);
		if(tmp4<-180.0 || tmp4>180.0) tmp4=180.0;
		module->angle_max = tmp4;

	} else if(strcasecmp(key,"db_min")==0) {

		tmp4 = atof(val);
		if(tmp4<-300 || tmp4>0)	tmp4=-300.0;
		module->db_min = tmp4;

	} else if(strcasecmp(key,"db_max")==0) {

		tmp4 = atof(val);
		if(tmp4<-300 || tmp4>0)	tmp4=0;
		module->db_max = tmp4;

	} else {
		DEBUG( printf("VUMETER: Helper2.. unknown value: %s = %s\n",key,val) );
	}

	return(1);
}

int vumeter_load_skin(int pathnum, char *name)
{
	int		l1;
	gchar		**tmp4;
	char		*directory,
			*directory2,
			tmp1[512],
			inside_module;
	FILE    	*fd;
	vumeter_skin	skin;
	vumeter_module	module;

	// Build array, if it doesn't already exist
	if(plugin_skin_data == NULL)
	{
		plugin_skin_data = g_array_new(FALSE,FALSE,sizeof(vumeter_skin));
	}

	// Is skin already loaded?
	for(l1=0; l1<plugin_skin_data->len; l1++)
	if( 	g_array_index(plugin_skin_data, vumeter_skin, l1).pathnum == pathnum &&
		strcmp(g_array_index(plugin_skin_data, vumeter_skin, l1).skin_name, name) == 0 )
	{
		DEBUG( printf("VUMETER: Skin (%d,%s) already loaded..\n",pathnum,name) );
		return(l1+1);
	}

	DEBUG( printf("VUMETER: Loading skin (%d,%s)...\n",pathnum,name) );

	// Build path to skin directory
	if(pathnum==0)
	{
		directory = g_strconcat(SKINDIR,"/VU_Meter_skins/",name,"/skin.cfg",NULL);
		directory2 = g_strconcat(SKINDIR,"/VU_Meter_skins/",name,NULL);
	} else if(pathnum==1) {
		directory = g_strconcat(g_get_home_dir(),"/.xmms/VU_Meter_skins/",name,"/skin.cfg",NULL);
		directory2 = g_strconcat(g_get_home_dir(),"/.xmms/VU_Meter_skins/",name,NULL);
	} else {
		return(0);
	}

	// Try to open skin.cfg
	fd = fopen(directory,"rb");
	if(fd == NULL)
	{
		g_free(directory);
		g_free(directory2);
		return(0);
	}

	// Reset skin settings
	vumeter_init_skin(&skin);
	skin.pathnum = pathnum;
	strncpy(skin.skin_name,name,255);

	// Read lines from file
	inside_module=0;
	while( fgets(tmp1,512,fd) != NULL)
	{
		// Trim line, and skip comments
		g_strstrip(tmp1);
		if(tmp1[0]=='#' || tmp1[0]==0) continue;

		if( strcasecmp(tmp1,"<module>") == 0)
		{
			if(!inside_module)
			{
				vumeter_reset_module(&module);
				inside_module=1;
				continue;
			}

		} else if( strcasecmp(tmp1,"</module>") == 0) {
			if(inside_module)
			{
				// Last minute checks for values
				if(module.angle_min > module.angle_max)
					module.angle_min = module.angle_max;

				if(module.db_min > module.db_max)
					module.db_min = module.db_max;

				// Precalculate value(s)
				module.angle_min+=180.0;
				module.angle_max+=180.0;
				module.angle_range = module.angle_max-module.angle_min;

				// Add module to array
				g_array_append_val(skin.modules,module);
				inside_module=0;
				continue;
			}
		}

		// Split line to  key, value  pair
		tmp4=g_strsplit(tmp1,"=",2);
		if(tmp4[0]==NULL || tmp4[1]==NULL)
		{
			g_strfreev(tmp4);
			continue;
		}
		g_strstrip(tmp4[0]);
		g_strstrip(tmp4[1]);

		// Handle key
		if(inside_module)
		{
			vumeter_ls_helper2(tmp4[0],tmp4[1],&module,directory2);
		} else {
			vumeter_ls_helper1(tmp4[0],tmp4[1],&skin,directory2);
		}

		g_strfreev(tmp4);
	}

	// Free memory & close file
	fclose(fd);
	g_free(directory);
	g_free(directory2);

	// Free slots to put new skin into?
	for(l1=0; l1<plugin_skin_data->len; l1++)
	if( g_array_index(plugin_skin_data, vumeter_skin, l1).pathnum == -1 )
	{
		vumeter_copy_skin( &g_array_index(plugin_skin_data, vumeter_skin, l1),&skin);
		return(l1+1);
	}

	// Create new position for skin
	g_array_append_val(plugin_skin_data,skin);
	return(plugin_skin_data->len);
}
