#include "Sun.h"
#include "libxmms/configfile.h"

SunConfig sun_cfg;

static void set_chan_mode(ConfigFile *cfg, int flag, char* name, int def_val);

void abuffer_init(void)
{
	ConfigFile *cfgfile;
    
	memset(&sun_cfg, 0, sizeof(SunConfig));
    
	sun_cfg.audio_device = getenv("AUDIODEV");
	if (! sun_cfg.audio_device)
		sun_cfg.audio_device = "/dev/audio";
	/* if we have UTAUDIODEV, we're probably on a Sunray so we _do_ use AUDIODEV */
	if (getenv("UTAUDIODEV") == NULL)
		sun_cfg.always_audiodev = FALSE;
	else
		sun_cfg.always_audiodev = TRUE;

	sun_cfg.buffer_size=500;
	sun_cfg.prebuffer=25;
	sun_cfg.channel_flags = 0;
    
	cfgfile = xmms_cfg_open_default_file();
	xmms_cfg_read_string(cfgfile,"Sun","audio_device",
			     &sun_cfg.audio_device);
        xmms_cfg_read_int(cfgfile,"Sun","always_use_audiodev",&sun_cfg.always_audiodev);
        xmms_cfg_read_int(cfgfile,"Sun","buffer_size",&sun_cfg.buffer_size);
        xmms_cfg_read_int(cfgfile,"Sun","prebuffer",&sun_cfg.prebuffer);
        set_chan_mode(cfgfile, AUDIO_SPEAKER, "speaker", 0);
        set_chan_mode(cfgfile, AUDIO_LINE_OUT, "line_out", 0);
        set_chan_mode(cfgfile, AUDIO_HEADPHONE, "headphone", 0);
        xmms_cfg_free(cfgfile);
}

static void set_chan_mode(ConfigFile *cfg, int flag, char* name, int def_val)
{
	xmms_cfg_read_int(cfg,"Sun", name, &def_val);
	if (def_val)
		sun_cfg.channel_flags |= flag;
	else
		sun_cfg.channel_flags &= ~flag;
}
