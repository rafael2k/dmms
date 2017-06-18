#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include "xmms/plugin.h"
#include <mikmod.h>

extern InputPlugin mikmod_ip;

enum {
	SAMPLE_FREQ_44 = 0,
	SAMPLE_FREQ_22,
	SAMPLE_FREQ_11,
};

typedef struct
{
	int mixing_freq;
	int volumefadeout;
	int surround;
	int force8bit;
	int hidden_patterns;
	int force_mono;
	int interpolation;
	int filename_titles;
	int def_pansep;
}
MIKMODConfig;

extern MIKMODConfig mikmod_cfg;

extern MDRIVER drv_xmms;


