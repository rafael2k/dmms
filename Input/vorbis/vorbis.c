/*
 * Copyright (C) Tony Arcieri <bascule@inferno.tusculum.edu>
 * Copyright (C) 2001-2002  Haavard Kvaalen <havardk@xmms.org>
 *
 * ReplayGain processing Copyright (C) 2002 Gian-Carlo Pascutto <gcp@sjeng.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

/*
 * 2002-01-11 ReplayGain processing added by Gian-Carlo Pascutto <gcp@sjeng.org>
 */

/*
 * Note that this uses vorbisfile, which is not (currently)
 * thread-safe.
 */

#include "config.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "xmms/plugin.h"
#include "libxmms/util.h"
#include "libxmms/configfile.h"
#include "libxmms/titlestring.h"
#include "libxmms/charset.h"
#include <xmms/i18n.h>

#include "vorbis.h"
#include "http.h"

extern vorbis_config_t vorbis_cfg;

static int vorbis_check_file(char *filename);
static void vorbis_play(char *filename);
static void vorbis_stop(void);
static void vorbis_pause(short p);
static void vorbis_seek(int time);
static int vorbis_time(void);
static void vorbis_get_song_info(char *filename, char **title, int *length);
static gchar *vorbis_generate_title(OggVorbis_File *vorbisfile, char *fn);
static void vorbis_aboutbox(void);
static void vorbis_init(void);
static long vorbis_process_replaygain(float **pcm, int samples, int ch, char *pcmout, float rg_scale);
static gboolean vorbis_update_replaygain(float *scale);


static size_t ovcb_read(void *ptr, size_t size, size_t nmemb, void *datasource);
static int ovcb_seek(void *datasource, int64_t offset, int whence);
static int ovcb_close(void *datasource);
static long ovcb_tell(void *datasource);

ov_callbacks vorbis_callbacks = 
{
	ovcb_read,
	ovcb_seek,
	ovcb_close,
	ovcb_tell
};

InputPlugin vorbis_ip =
{
	NULL,
	NULL,
	NULL,               /* description */
	vorbis_init,        /* init */
	vorbis_aboutbox,    /* aboutbox */
	vorbis_configure,   /* configure */
	vorbis_check_file,  /* is_our_file */
	NULL,
	vorbis_play,
	vorbis_stop,
	vorbis_pause,
	vorbis_seek,
	NULL,               /* set eq */
	vorbis_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	vorbis_get_song_info,
	vorbis_file_info_box,       /* file info box, tag editing */
	NULL
};

static OggVorbis_File vf;

static pthread_t tid;
int vorbis_playing;
static int output_opened;
static int dopause;
static int vorbis_eos;
static int vorbis_is_streaming;
static int vorbis_bytes_streamed;
static volatile int seekneeded = -1;
static int samplerate, channels;
pthread_mutex_t vf_mutex = PTHREAD_MUTEX_INITIALIZER;
static gboolean output_error;
	

InputPlugin *get_iplugin_info(void)
{
	vorbis_ip.description = g_strdup_printf(_("Ogg Vorbis Player %s"), VERSION);
	return &vorbis_ip;
}

static int vorbis_check_file(char *filename)
{
	FILE *stream;
	OggVorbis_File vfile; /* avoid thread interaction */
	char *ext;

	/* is this our http resource? */
	if (strncasecmp(filename, "http://", 7) == 0) {
		ext = strrchr(filename, '.');
		if (ext) {
			if (!strncasecmp(ext, ".ogg", 4)) {
				return TRUE;
			}
		}
		return FALSE;
	}
   
	if ((stream = fopen(filename, "r")) == NULL)
		return FALSE;
   
	/*
	 * The open function performs full stream detection and machine
	 * initialization.  If it returns zero, the stream *is* Vorbis and
	 * we're fully ready to decode.
	 */

	/* libvorbisfile isn't thread safe... */
	memset(&vfile, 0, sizeof(vfile));
	pthread_mutex_lock(&vf_mutex);
	if (ov_open(stream, &vfile, NULL, 0) < 0)
	{
		pthread_mutex_unlock(&vf_mutex);
		fclose(stream);
		return FALSE;
	}

	ov_clear(&vfile); /* once the ov_open succeeds, the stream belongs to
			     vorbisfile.a.  ov_clear will fclose it */
	pthread_mutex_unlock(&vf_mutex);
	return TRUE;
}

static void vorbis_jump_to_time(long time)
{
	pthread_mutex_lock(&vf_mutex);

	/*
	 * We need to guard against seeking to the end, or things
	 * don't work right.  Instead, just seek to one second prior
	 * to this
	 */
	if (time == ov_time_total(&vf, -1))
		time--;

	vorbis_ip.output->flush(time * 1000);
	ov_time_seek(&vf, time);
	
	pthread_mutex_unlock(&vf_mutex);
}

#ifdef WORDS_BIGENDIAN
#define XMMS_BIG_ENDIAN TRUE
#define GET_BYTE1(val) ((val) >> 8)
#define GET_BYTE2(val) ((val) & 0xff)
#else
#define XMMS_BIG_ENDIAN FALSE
#define GET_BYTE1(val) ((val) & 0xff)
#define GET_BYTE2(val) ((val) >> 8)
#endif

static void do_seek(void)
{
	if (seekneeded != -1 && !vorbis_is_streaming) {
		vorbis_jump_to_time(seekneeded);
		seekneeded = -1;
		vorbis_eos = FALSE;
	}
}

static int vorbis_process_data(int last_section, gboolean use_rg, float rg_scale)
{
	char pcmout[4096];
	const int bep = XMMS_BIG_ENDIAN;
	int bytes;
	float **pcm;

	/*
	 * A vorbis physical bitstream may consist of many logical
	 * sections (information for each of which may be fetched from
	 * the vf structure).  This value is filled in by ov_read to
	 * alert us what section we're currently decoding in case we
	 * need to change playback settings at a section boundary
	 */
	int current_section;

	pthread_mutex_lock(&vf_mutex);
	if (use_rg)
	{
		bytes = ov_read_float(&vf, &pcm, sizeof(pcmout)/2/channels,
				      &current_section);
		if (bytes > 0)
			bytes = vorbis_process_replaygain(pcm, bytes, channels,
			                                  pcmout, rg_scale);
	}
	else
		bytes = ov_read(&vf, pcmout, sizeof(pcmout), bep, 2, 1,
				&current_section);

	switch (bytes)
	{
		case 0:
			/* EOF */
			pthread_mutex_unlock(&vf_mutex);
			vorbis_ip.output->buffer_free();
			vorbis_ip.output->buffer_free();
			vorbis_eos = TRUE;
			return last_section;

		case OV_HOLE:
		case OV_EBADLINK:
			/*
			 * error in the stream.  Not a problem, just
			 * reporting it in case we (the app) cares.
			 * In this case, we don't.
			 */
			pthread_mutex_unlock(&vf_mutex);
			return last_section;
	}

	if (current_section != last_section)
	{
		/*
		 * The info struct is different in each section.  vf
		 * holds them all for the given bitstream.  This
		 * requests the current one
		 */
		vorbis_info* vi = ov_info(&vf, -1);

		if (vi->channels > 2)
		{
			vorbis_eos = TRUE;
			pthread_mutex_unlock(&vf_mutex);
			return current_section;
		}
		

		if (vi->rate != samplerate || vi->channels != channels)
		{
			samplerate = vi->rate;
			channels = vi->channels;
			vorbis_ip.output->buffer_free();
			vorbis_ip.output->buffer_free();
			vorbis_ip.output->close_audio();
			if (!vorbis_ip.output->
			    open_audio(FMT_S16_NE, vi->rate,
				       vi->channels))
			{
				output_error = TRUE;
				vorbis_eos = TRUE;
				pthread_mutex_unlock(&vf_mutex);
				return current_section;
			}
			vorbis_ip.output->flush(ov_time_tell(&vf) * 1000);
		}
	}
	pthread_mutex_unlock(&vf_mutex);

	vorbis_ip.add_vis_pcm(vorbis_ip.output->written_time(), 
			      FMT_S16_NE, channels, bytes, pcmout);
     
	while (vorbis_ip.output->buffer_free() < bytes)
	{
		xmms_usleep(20000);
		if (!vorbis_playing)
			return current_section;
		if (seekneeded != -1)
			do_seek();
	}
	vorbis_ip.output->write_audio(pcmout, bytes);

	return current_section;
}

static void *vorbis_play_loop(void *arg)
{
	char *filename = (char *)arg;
	gchar *title = NULL;
	double time;
	long timercount = 0;
	vorbis_info *vi;

	int last_section = -1;

	FILE *stream = NULL;
	void *datasource = NULL;

	gboolean use_rg;
	float rg_scale;

	memset(&vf, 0, sizeof(vf));

	if (strncasecmp("http://", filename, 7) != 0) {
		/* file is a real file */
		if ((stream = fopen(filename, "r")) == NULL) {
			vorbis_eos = TRUE;
			goto play_cleanup;
		}
		datasource = (void *)stream;
	} else {
		/* file is a stream */
		vorbis_is_streaming = 1;
		vorbis_http_open(filename);
		datasource = "NULL";
	}

	/*
	 * The open function performs full stream detection and
	 * machine initialization.  None of the rest of ov_xx() works
	 * without it
	 */

	pthread_mutex_lock(&vf_mutex);
	if (ov_open_callbacks(datasource, &vf, NULL, 0, vorbis_callbacks) < 0)
	{
		vorbis_callbacks.close_func(datasource);
		pthread_mutex_unlock(&vf_mutex);
		vorbis_eos = TRUE;
		goto play_cleanup;
	}
	vi = ov_info(&vf, -1);

	if (vorbis_is_streaming) 
		time = -1;
	else
		time = ov_time_total(&vf, -1) * 1000;

	if (vi->channels > 2) {
		vorbis_eos = TRUE;
		pthread_mutex_unlock(&vf_mutex);
		goto play_cleanup;
	}

	samplerate = vi->rate;
	channels = vi->channels;
   
	title = vorbis_generate_title(&vf, filename);
	use_rg = vorbis_update_replaygain(&rg_scale);

	vorbis_ip.set_info(title, time, ov_bitrate(&vf, -1), samplerate, channels);
	if (!vorbis_ip.output->open_audio(FMT_S16_NE, vi->rate, vi->channels))
	{
		output_error = TRUE;
		pthread_mutex_unlock(&vf_mutex);
		goto play_cleanup;
	}
	pthread_mutex_unlock(&vf_mutex);
	output_opened = TRUE;
	if (dopause) {
		vorbis_pause(TRUE);
		dopause = FALSE;
	}

	seekneeded = -1;
   
	/*
	 * Note that chaining changes things here; A vorbis file may
	 * be a mix of different channels, bitrates and sample rates.
	 * You can fetch the information for any section of the file
	 * using the ov_ interface.
	 */

	while (vorbis_playing)
	{
		int current_section;

		if (seekneeded != -1)
			do_seek();

		if (vorbis_eos)
		{
			xmms_usleep(20000);
			continue;
		}
			
		current_section = vorbis_process_data(last_section,
						      use_rg, rg_scale);

		if (current_section != last_section)
		{
			/*
			 * set total play time, bitrate, rate, and channels of
			 * current section
			 */
			if (title)
				g_free(title);
			pthread_mutex_lock(&vf_mutex);
			title = vorbis_generate_title(&vf, filename);
			use_rg = vorbis_update_replaygain(&rg_scale);

			if (vorbis_is_streaming)
				time = -1;
			else
				time = ov_time_total(&vf, -1) * 1000;

			vorbis_ip.set_info(title, time,
					   ov_bitrate(&vf, current_section), 
					   samplerate, channels);
			pthread_mutex_unlock(&vf_mutex);
			timercount = vorbis_ip.output->output_time();

			last_section = current_section;
		}

		if (!(vi->bitrate_upper == vi->bitrate_lower == vi->bitrate_nominal) &&
		     (vorbis_ip.output->output_time() > timercount + 1000 ||
		      vorbis_ip.output->output_time() < timercount))
		{
			/*
			 * simple hack to avoid updating too
			 * often
			 */
			long br;
			
			pthread_mutex_lock(&vf_mutex);
			br = ov_bitrate_instant(&vf);
			pthread_mutex_unlock(&vf_mutex);
			if (br > 0)
				vorbis_ip.set_info(title, time, br,
						   samplerate, channels);
			timercount = vorbis_ip.output->output_time();
		}
	}
	output_opened = FALSE;
	if (!output_error)
		vorbis_ip.output->close_audio();
	/* fall through intentional */

 play_cleanup:
	if (title)
		g_free(title);
	g_free(filename);

	/*
	 * ov_clear closes the stream if its open.  Safe to call on an
	 * uninitialized structure as long as we've zeroed it
	 */
	pthread_mutex_lock(&vf_mutex);
	ov_clear(&vf);
	pthread_mutex_unlock(&vf_mutex);
	vorbis_is_streaming = 0;
	return NULL;
}

static void vorbis_play(char *filename)
{
	vorbis_playing = 1;
	vorbis_bytes_streamed = 0;
	vorbis_eos = 0;
	output_opened = FALSE;
	output_error = FALSE;
	dopause = FALSE;
	pthread_create(&tid, NULL, vorbis_play_loop, g_strdup(filename));
}
 
static void vorbis_stop(void)
{
	if (vorbis_playing)
	{
		vorbis_playing = 0;
		pthread_join(tid, NULL);
	}
}

static void vorbis_pause(short p)
{
	if (output_opened)
		vorbis_ip.output->pause(p);
	else
		dopause = p;
}

static int vorbis_time(void)
{
	if (output_error)
		return -2;
	if (vorbis_eos && !vorbis_ip.output->buffer_playing())
		return -1;
	return vorbis_ip.output->output_time();
}

static void vorbis_seek(int time)
{
	if (vorbis_is_streaming) return;

	seekneeded = time;

	while (seekneeded != -1)
		xmms_usleep(20000);
}

static void vorbis_get_song_info(char *filename, char **title, int *length)
{
	FILE *stream;
	OggVorbis_File vf; /* avoid thread interaction */
	
	if (strncasecmp(filename, "http://", 7)) {
		if ((stream = fopen(filename, "r")) == NULL)
			return;
   
		/*
		 * The open function performs full stream detection and
		 * machine initialization.  If it returns zero, the stream
		 * *is* Vorbis and we're fully ready to decode.
		 */
		pthread_mutex_lock(&vf_mutex);
		if (ov_open(stream, &vf, NULL, 0) < 0)
		{
			pthread_mutex_unlock(&vf_mutex);
			fclose(stream);
			return;
		}
	
		/* Retrieve the length */
		*length = ov_time_total(&vf, -1) * 1000;
	
		*title = NULL;
		*title = vorbis_generate_title(&vf, filename);
  	 
		/*
		 * once the ov_open succeeds, the stream belongs to
		 * vorbisfile.a.  ov_clear will fclose it
		 */
		ov_clear(&vf);
		pthread_mutex_unlock(&vf_mutex);
	} else {
		/* streaming song info */
		*length = -1;
		*title = (char *)vorbis_http_get_title(filename);
	}
}

static gchar *extname(const char *filename)
{
	gchar *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

/* Make sure you've locked vf_mutex */
static gboolean vorbis_update_replaygain(float *scale)
{
	vorbis_comment *comment;
	char *rg_gain = NULL, *rg_peak_str = NULL;
	float rg_peak;

	if (!vorbis_cfg.use_replaygain && !vorbis_cfg.use_anticlip)
		return FALSE;
	if ((comment = ov_comment(&vf, -1)) == NULL)
		return FALSE;

	*scale = 1.0;

	if (vorbis_cfg.use_replaygain)
	{
		if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_ALBUM)
		{
			rg_gain = vorbis_comment_query(comment, "replaygain_album_gain", 0);
			if (!rg_gain)
				/* Obsolete name */
				rg_gain = vorbis_comment_query(comment, "rg_audiophile", 0);
		}

		if (!rg_gain)
			rg_gain = vorbis_comment_query(comment, "replaygain_track_gain", 0);
		if (!rg_gain)
			/* Obsolete name */
			rg_gain = vorbis_comment_query(comment, "rg_radio", 0);
		
		/* FIXME: Make sure this string is the correct format first? */
		if (rg_gain)
			*scale = pow(10., atof(rg_gain)/20);
	}
	
	if (vorbis_cfg.use_anticlip)
	{
		if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_ALBUM)
			rg_peak_str =
				vorbis_comment_query(comment,
						     "replaygain_album_peak", 0);

		if (!rg_peak_str)
			rg_peak_str =
				vorbis_comment_query(comment,
						     "replaygain_track_peak", 0);
		if (!rg_peak_str)
			/* Obsolete name */
			rg_peak_str = vorbis_comment_query(comment, "rg_peak", 0);

		if (rg_peak_str)
			rg_peak = atof(rg_peak_str);
		else
			rg_peak = 1;

		if (*scale * rg_peak > 1.0)
			*scale = 1.0 / rg_peak;
	}
	
	if (*scale != 1.0 || (vorbis_cfg.use_booster && rg_gain)) {
		if (*scale > 15.0)
			*scale = 15.0;

		return TRUE;
	}

	return FALSE;
}

static long vorbis_process_replaygain(float **pcm, int samples, int ch, char *pcmout, float rg_scale)
{
	int i, j;
	/* ReplayGain processing */
	for (i = 0; i < samples; i++)
		for (j = 0; j < ch; j++)
		{
			float sample = pcm[j][i] * rg_scale;
			int value;
				
			if (vorbis_cfg.use_booster)
			{
				sample *= 2;
				
				/* hard 6dB limiting */
				if (sample < -0.5)
					sample = tanh((sample + 0.5) / 0.5) * 0.5 - 0.5;
				else if (sample > 0.5)
					sample = tanh((sample - 0.5) / 0.5) * 0.5 + 0.5;
			}
				
			value = sample * 32767;
			if (value > 32767)
				value = 32767;
			else if (value < -32767)
				value = -32767;
			
			*pcmout++ = GET_BYTE1(value);
			*pcmout++ = GET_BYTE2(value);
		}

	return 2 * ch * samples;
}			

static char *vorbis_generate_title(OggVorbis_File *vorbisfile, char *fn)
{
	/* Caller should hold vf_mutex */
	char *displaytitle = NULL, *tmp, *path, *temp;
	vorbis_comment *comment;
	TitleInput *input;

	XMMS_NEW_TITLEINPUT(input);
	path = g_strdup(fn);
	temp = strrchr(path, '/');
	if (temp)
		*temp = '\0';
	input->file_name = g_basename(fn);
	input->file_ext = extname(fn);
	input->file_path = g_strdup_printf("%s/", path);

	if ((comment = ov_comment(vorbisfile, -1)) != NULL)
	{
		input->track_name = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "title", 0));
		input->performer = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "artist", 0));
		input->album_name = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "album", 0));
		if ((tmp = vorbis_comment_query(comment, "tracknumber", 0)) != NULL)
			input->track_number = atoi(tmp);
		input->date = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "date", 0));
		if ((tmp = vorbis_comment_query(comment, "year", 0)) != NULL)
			input->year = atoi(tmp);
		input->genre = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "genre", 0));
		/* "comment" = user comment, not "" */
		input->comment = xmms_charset_from_utf8(
			vorbis_comment_query(comment, "comment", 0));
	}

	displaytitle = xmms_get_titlestring(vorbis_cfg.tag_override ?
					    vorbis_cfg.tag_format :
					    xmms_get_gentitle_format(), input);
	g_free(input->track_name);
	g_free(input->performer);
	g_free(input->album_name);
	g_free(input->date);
	g_free(input->genre);
	g_free(input->comment);
	g_free(input);
	g_free(path);

	if (!displaytitle)
	{
		if (!vorbis_is_streaming)
		{
			char *tmp;
			displaytitle = g_strdup(g_basename(fn));
			if ((tmp = strrchr(displaytitle, '.')) != NULL)
				*tmp = '\0';
		}
		else
			displaytitle = vorbis_http_get_title(fn);
	}

	return displaytitle;
}

static void vorbis_aboutbox()
{
	static GtkWidget *about_window;

	if (about_window)
		gdk_window_raise(about_window->window);

	about_window = xmms_show_message(
		_("About Ogg Vorbis Plugin"),
		/*
		 * I18N: UTF-8 Translation: "Haavard Kvaalen" ->
		 * "H\303\245vard Kv\303\245len"
		 */
		_("Ogg Vorbis Plugin by the Xiph.org Foundation\n\n"
		  "Original code by\n"
		  "Tony Arcieri <bascule@inferno.tusculum.edu>\n"
		  "Contributions from\n"
		  "Chris Montgomery <monty@xiph.org>\n"
		  "Peter Alm <peter@xmms.org>\n"
		  "Michael Smith <msmith@labyrinth.edu.au>\n"
		  "Jack Moffitt <jack@icecast.org>\n"
		  "Jorn Baayen <jorn@nl.linux.org>\n"
		  "Haavard Kvaalen <havardk@xmms.org>\n"
		  "Gian-Carlo Pascutto <gcp@sjeng.org>\n\n"
		  "Visit the Xiph.org Foundation at http://www.xiph.org/\n"),
		_("OK"), FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(about_window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &about_window);
}

static void vorbis_init(void)
{
	ConfigFile *cfg;

	memset(&vorbis_cfg, 0, sizeof(vorbis_config_t));
	vorbis_cfg.http_buffer_size = 128;
	vorbis_cfg.http_prebuffer = 25;
	vorbis_cfg.proxy_port = 8080;
	vorbis_cfg.proxy_use_auth = FALSE;
	vorbis_cfg.proxy_user = NULL;
	vorbis_cfg.proxy_pass = NULL;
	vorbis_cfg.tag_override = FALSE;
	vorbis_cfg.tag_format = NULL;
	vorbis_cfg.use_anticlip = FALSE;
	vorbis_cfg.use_replaygain = FALSE;
	vorbis_cfg.replaygain_mode = REPLAYGAIN_MODE_TRACK;
	vorbis_cfg.use_booster = FALSE;

	cfg = xmms_cfg_open_default_file();
	xmms_cfg_read_int(cfg, "vorbis", "http_buffer_size",
			  &vorbis_cfg.http_buffer_size);
	xmms_cfg_read_int(cfg, "vorbis", "http_prebuffer",
			  &vorbis_cfg.http_prebuffer);
	xmms_cfg_read_boolean(cfg, "vorbis", "save_http_stream",
			      &vorbis_cfg.save_http_stream);
	if (!xmms_cfg_read_string(cfg, "vorbis", "save_http_path",
				  &vorbis_cfg.save_http_path))
		vorbis_cfg.save_http_path = g_strdup(g_get_home_dir());

	xmms_cfg_read_boolean(cfg, "vorbis", "use_proxy", &vorbis_cfg.use_proxy);
	if (!xmms_cfg_read_string(cfg, "vorbis", "proxy_host",
				  &vorbis_cfg.proxy_host))
		vorbis_cfg.proxy_host = g_strdup("localhost");
	xmms_cfg_read_int(cfg, "vorbis", "proxy_port", &vorbis_cfg.proxy_port);
	xmms_cfg_read_boolean(cfg, "vorbis", "proxy_use_auth",
			      &vorbis_cfg.proxy_use_auth);
	xmms_cfg_read_string(cfg, "vorbis", "proxy_user",
			     &vorbis_cfg.proxy_user);
	xmms_cfg_read_string(cfg, "vorbis", "proxy_pass",
			     &vorbis_cfg.proxy_pass);
	xmms_cfg_read_boolean(cfg, "vorbis", "tag_override",
			      &vorbis_cfg.tag_override);
	if (!xmms_cfg_read_string(cfg, "vorbis", "tag_format",
				  &vorbis_cfg.tag_format))
		vorbis_cfg.tag_format = g_strdup("%p - %t");
	xmms_cfg_read_boolean(cfg, "vorbis", "use_anticlip", &vorbis_cfg.use_anticlip);
	xmms_cfg_read_boolean(cfg, "vorbis", "use_replaygain", &vorbis_cfg.use_replaygain);
	xmms_cfg_read_int(cfg, "vorbis", "replaygain_mode", &vorbis_cfg.replaygain_mode);
	xmms_cfg_read_boolean(cfg, "vorbis", "use_booster", &vorbis_cfg.use_booster);
	xmms_cfg_free(cfg);
}

static size_t ovcb_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	size_t tmp;


	if (vorbis_is_streaming) {
		/* this is a stream */
		tmp = vorbis_http_read(ptr, size * nmemb);
		vorbis_bytes_streamed += tmp;
		return tmp;
	}

	return fread(ptr, size, nmemb, (FILE *)datasource);
}

static int ovcb_seek(void *datasource, int64_t offset, int whence)
{
	if (vorbis_is_streaming) {
		/* this is a stream */
		/* streams aren't seekable */
		return -1;
	}

	return fseek((FILE *)datasource, offset, whence);
}

static int ovcb_close(void *datasource)
{
	if (vorbis_is_streaming) {
		/* this is a stream */
		vorbis_http_close();
		return 0;
	}

	return fclose((FILE *)datasource);
}

static long ovcb_tell(void *datasource)
{
	if (vorbis_is_streaming) {
		/* this is a stream */
		/* return bytes read */
		return vorbis_bytes_streamed;
	}

	return ftell((FILE *)datasource);
}
