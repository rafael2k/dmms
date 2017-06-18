/******************************************************************************
 *
 * Solaris audio plugin for XMMS
 * 
 * This file contains most of the audio routines for talking to the sound
 * device. 
 *
 *****************************************************************************/


#include "Sun.h"
#include "config.h"
#include <pthread.h>
#include <stropts.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
#include <strings.h>
#include <sys/modctl.h>
#include <glib.h>
#include <libxmms/util.h>
#include <pthread.h>


/*****************************************************************************/

/* This is the maximum data we're willing to send to the audio device and
   when we should refill it. */
#define SUN_AUDIO_CACHE 16384

/*****************************************************************************/

static char *buffer_ptr = NULL;
static char *buffer_end = NULL;
static char *buffer_read = NULL;
static char *buffer_write = NULL;
static int buffer_len = 0;
static pthread_mutex_t buflock = PTHREAD_MUTEX_INITIALIZER;

static gint time_offset = 0;
static gint byte_offset = 0;

static gboolean paused = FALSE;
static gboolean playing = FALSE;
static gboolean aopen = FALSE;
static gboolean prebuffer, remove_prebuffer;
static int prebuffer_size;
static volatile gint flush = -1;

static gint audiofd = -2;
static audio_info_t adinfo;
static pthread_t buffer_thread;

static uint_t audio_scount = 0;

static struct {
    gboolean is_signed;
    gboolean is_bigendian;
    uint_t sample_rate;
    uint_t channels;
    uint_t precision;
    gint bps;
    gint output_precision;
} stream_attrs;


static AFormat input_format;

/*****************************************************************************/

void abuffer_shutdown(void);

/*****************************************************************************/

static char *get_audiodev(void)
{
	if (sun_cfg.always_audiodev)
	{
		char * audiodev;
		audiodev = getenv("AUDIODEV");
		if (audiodev != NULL)
			return audiodev;
	}
	return (sun_cfg.audio_device);
}

/* The hack in the following function relieves us of using audiotool/audiocontrol */
/* in order to switch to internal CD port when playing CD Audio. 	*/

int ctlfd = -1; /* Global file descriptor for /dev/audioctl usage */
		
int init_ctlfd(void)
{

	char *ctlname;
	int port, monitor;
	audio_info_t info;
	audio_device_t device;
	
	/* Get audio device name */
	ctlname = g_strconcat(get_audiodev(), "ctl", NULL);
	
	/* Open audio control device, audio device may be in use already */
    	if ((ctlfd = open(ctlname, O_RDONLY, 0)) < 0)
	{
		g_free(ctlname);
		return -1;
	}

	g_free(ctlname);
	
	/* Get audio device characteristics */
	if (ioctl(ctlfd, AUDIO_GETDEV, &device) < 0)
	{
		close(ctlfd);
		ctlfd = -1;
		return -1;
	}
	
	/* We can apply the trick only for sound cards using Sun driver */	
	if (strcmp(device.name, "SUNW,CS4231") &&
	    strcmp(device.name, "SUNW,sb16") &&
	    strcmp(device.name, "SUNW,sbpro"))
		return 0; /* Sun driver is not used, but it's okay */

	/* Use an internal CD port if the device has one, or use line-in port */
	if (ioctl(ctlfd, AUDIO_GETINFO, &info) < 0)
	{
        	close(ctlfd);
        	ctlfd = -1;
        	return -1;
	}
	if (info.record.avail_ports & AUDIO_INTERNAL_CD_IN) 
		port = AUDIO_INTERNAL_CD_IN;
	else
		port = AUDIO_LINE_IN;
	monitor = info.monitor_gain;
	
	/* Initialize info structure */
	AUDIO_INITINFO(&info);	
	
	/* Set port */
	info.record.port = port;
	
	/* Set monitor volume so that CD sound is audible. */
	/* My tests shows that 3 is a minimum value (MZ). */
	if (monitor < 3)
		info.monitor_gain = AUDIO_MAX_GAIN / 2;
	
	/* Apply changes to audio control device */
	if (ioctl(ctlfd, AUDIO_SETINFO, &info) < 0)
		perror(get_audiodev());
	
	return 0;
}

static void audio_yield(void)
{
	audio_info_t info;

	ioctl(audiofd, AUDIO_GETINFO, &info);

	while (playing && (flush == -1) && ((info.play.eof + 10) < audio_scount))
	{
	        xmms_usleep(10000);
	        ioctl(audiofd, AUDIO_GETINFO, &info);
	}
}

static void audio_play(char *bufferP, int length)
{
	int written;

	while (playing && (flush == -1) && (length > 0))
	{
		written = write(audiofd, bufferP, length);
		if (written > 0)
		{
			length -= written;
			bufferP += written;
			if (write(audiofd, NULL, 0) == 0)
			{
				audio_scount++;
				audio_yield();
			}
		}
		else
		{
		        xmms_usleep(10000);
		}
	}
}

gint abuffer_get_written_time(void)
{
	if (!playing)
		return 0;
    
	return ((double)byte_offset * 1000.0) / stream_attrs.bps;
}

gint abuffer_get_output_time(void)
{
	if (!aopen || !playing)
		return 0;
    
	ioctl(audiofd, AUDIO_GETINFO, &adinfo);

	return time_offset + ((double)adinfo.play.samples * 1000) / adinfo.play.sample_rate;
}

gint abuffer_used(void)
{
	int retval;
	pthread_mutex_lock(&buflock);
	if(buffer_write >= buffer_read)
		retval = buffer_write - buffer_read;
	else
		retval = buffer_len - (buffer_read - buffer_write);
	pthread_mutex_unlock(&buflock);
	return retval;
}

/*
 * Doesn't free the buffer. Just returns the number of bytes
 * in the buffer available to write.
 */
gint abuffer_free(void)
{
	int retval;

	/*
	 * Two calls to this function without a write in between,
	 * means we are at the end of the stream so stop prebuffering.
	 */
	if (remove_prebuffer && prebuffer)
	{
		prebuffer = FALSE;
		remove_prebuffer = FALSE;
	}
	if (prebuffer)
		remove_prebuffer = TRUE;
	/* Find the amount of buffer free (circular buffer) */
	pthread_mutex_lock(&buflock);
	if(buffer_write >= buffer_read)
		retval = buffer_len - (buffer_write - buffer_read) - 1;
	else
		retval = buffer_read - buffer_write - 1;
	pthread_mutex_unlock(&buflock);
	return retval;
}

/***************************************************************************/
/* This copies data into the buffer, dependant on the endian-ness and sign */
/***************************************************************************/
void dsp_memcpy(void *dst, void *src, int len)
{
	/**********************************************************************
	 * Whether or not we have to do translations rather depends on the
	 * architecture; use WORDS_BIGENDIAN to define this for us.  We want
	 * this bit as optimal as is humanely possible as it is called quite
	 * often; try to catch the common situation first, and less common at
	 * the end.
	 ***********************************************************************
	 * In order to keep the flow as fast as possibly, there are few 'else'
	 * statements; instead, we use a 'return' call to get out of here.
	 **********************************************************************/

#if WORDS_BIGENDIAN
	if (stream_attrs.is_bigendian && stream_attrs.is_signed)
#else /* WORDS_BIGENDIAN */
	if (!stream_attrs.is_bigendian && stream_attrs.is_signed)
#endif /* WORDS_BIGENDIAN */
	{
		/* this is the default; simply copy the data over */
		memcpy(dst, src, len);
		return;
	}

/* We have either to make an endian transformation or a sign transformation
   or possibly both; which one(s)?  */

#if WORDS_BIGENDIAN
	if (!stream_attrs.is_bigendian)
#else /* WORDS_BIGENDIAN */
	if (stream_attrs.is_bigendian)
#endif /* WORDS_BIGENDIAN */
	{
		/*
		 * Endian tranformation required
		 * Note that since the endianness for 8 bit samples is set to native,
		 * we can assume we have a 16 bit sample What we cannot assume is the
		 * sign of the data 
		 */
		guint16 *csrc = src, *csend = (guint16 *) ((char *) src + len);
		guint16 *cdst = dst;
		while (csrc < csend)
		{
			*cdst = GUINT16_SWAP_LE_BE(*csrc);
			if (!stream_attrs.is_signed)
				*cdst = (*cdst) ^ (1 << 15);
			csrc++;
			cdst++;
		}
		return;
	}
	/* Native endianess */
	/* (!stream_attrs.is_signed) */
	if (stream_attrs.precision == 16)
	{
		guint16 *csrc = src, *csend = (guint16 *) ((char *) src + len);
		gint16 *cdst = dst;

		while (csrc < csend)
		{
			*cdst++ = (*csrc) ^ (1 << 15);
			csrc++;
		}
	}
	else
	{
		guint8 *csrc = src, *csend = (guint8 *) ((char *) src + len);
		gint8 *cdst = dst;

		while (csrc < csend)
		{
			*cdst++ = (*csrc) ^  (1 << 7);
			csrc++;
		}
	}
}
    
gint abuffer_write_sub(void *ptr, gint length)
{
	/* Amount to end of buffer */
	int bdiff = buffer_end - buffer_write;

	/* Amount of space needed or max space avail if we don't have space */
	int amt = MIN(length, abuffer_free());

	/* Update the byte offset */
	byte_offset += amt;

	/* Fill the buffer */
	if (bdiff < amt)
	{
		dsp_memcpy(buffer_write, ptr, bdiff);
		dsp_memcpy(buffer_ptr, (char *) ptr + bdiff, amt - bdiff);
		pthread_mutex_lock(&buflock);
		buffer_write = buffer_ptr + amt - bdiff;
		pthread_mutex_unlock(&buflock);
	}
	else
	{
		dsp_memcpy(buffer_write, ptr, amt);
		pthread_mutex_lock(&buflock);
		buffer_write += amt;
		pthread_mutex_unlock(&buflock);
	}

	/* Return the amount actually written */
	return amt;
}

void abuffer_write(void *ptr, gint length)
{
	char *iptr; 
	gint amt;
	void *tmp_buf;
	gint tmp_buf_size;
	gboolean need_free = FALSE;

	AFormat new_format;
	gint new_frequency, new_channels;
	EffectPlugin *ep;

	remove_prebuffer = FALSE;
	
	new_format = input_format;
	new_frequency = stream_attrs.sample_rate;
	new_channels = stream_attrs.channels;
	
	ep = get_current_effect_plugin();
	if (effects_enabled() && ep && ep->query_format)
	{
		ep->query_format(&new_format,&new_frequency,&new_channels);
	}
	
	if (effects_enabled() && ep && ep->mod_samples)
		length = ep->mod_samples(&ptr,length, input_format, stream_attrs.sample_rate , stream_attrs.channels);

	if (new_format != input_format || new_frequency !=stream_attrs.sample_rate  || new_channels != stream_attrs.channels)
	{
		/* FIXME: This is in effect a flush.  Need to recalulate buffer_offset and time_offset */
/*  		time_offset += (gint) ((output_bytes * 1000) / stream_attrs.bps); */
/*  		output_bytes = 0; */
/*  		oss_setup_format(new_format, new_frequency, new_channels); */
/*  		stream_attrs.sample_rate = new_frequency; */
/*  		stream_attrs.channels = new_channels; */
/*  		close(fd); */
/*  		fd = open(device_name,O_WRONLY); */
/*  		oss_set_audio_params(); */
		/* FIXME: This is leaking memory */
		abuffer_open(new_format, new_frequency, new_channels);
		
	}


	/* Check if we're downsizing/upsizing the stream */
	if (stream_attrs.precision != stream_attrs.output_precision)
	{
		gint i;
		/* Flag we have allocated memory for the end */
		need_free = TRUE;
		if (stream_attrs.output_precision == 16)
		{
			/* scaling 8 => 16 */
			gint8 *src = (gint8*) ptr;
			gint16 *target;
			tmp_buf_size = 2 * length;
			tmp_buf = g_malloc(tmp_buf_size);
			target = (gint16*)tmp_buf;
			for (i = 0; i < length; i++)
			{
				target[i] = (gint16)(src[i] ^ 128) * 256;
			}
		}
		else
		{
			/* scaling 16 => 8 */
			gint16 *src = (gint16*) ptr;
			gint8 *target;
			/* note that length should be even, so no rounding req'd */
			tmp_buf_size = length / 2;
			tmp_buf = g_malloc(tmp_buf_size);
			target = (gint8*) tmp_buf;
			for (i = 0; i < length; i++)
			{
				target[i] = ((gint8) (src[i] / 256)) ^ 128;
			}
		}
	}
	else
	{
		tmp_buf = ptr;
		tmp_buf_size = length;
	}
	iptr = tmp_buf;

	/* Loop till all the data is consumed */
	while (tmp_buf_size > 0)
	{
		/* Write out the first section */
		amt = abuffer_write_sub(iptr, tmp_buf_size);
		iptr += amt;
		tmp_buf_size -= amt;
	}
	if (need_free)
		g_free(tmp_buf);
}

void abuffer_close(void)
{
	/* Stop the playing */
	playing = FALSE;

	/* Rejoin the thread */
	pthread_join(buffer_thread, NULL);
}

void abuffer_flush(gint ftime)
{
	flush = ftime;

	/* Wait till  */
	while (flush != -1)
		xmms_usleep(10000);
}

void abuffer_pause(short p)
{
	if (p)
		paused = TRUE;
	else
		paused = FALSE;
}

gint abuffer_startup(void)
{
	/* Open audio device if it's not already open */
	if (audiofd < 0)
	{
		if ((audiofd = open(get_audiodev(), O_WRONLY )) == -1)
		{
			perror("xmms Solaris Output plugin");
			return 1;
		}
	}
	else
	{
		fprintf(stderr, "xmms: Warning: Attempted to reopen audio device.\n");
	}

	/* Initialise audio device */
	abuffer_set_audio_params();	

	/* Get the audio info */
	ioctl(audiofd, AUDIO_GETINFO, &adinfo);

	/* We should be playing */
	playing = TRUE;

	/* NOTE: What is flush for? */
	flush = -1;

	/* We aren't paused */
	paused = FALSE;

	/* We're open */
	aopen = TRUE;

	prebuffer = TRUE;
	remove_prebuffer = FALSE;

	/* We're at the beginning */
	time_offset = 0;
	byte_offset = 0;

	audio_scount = 0;
	return 0;
}

void abuffer_shutdown(void)
{
	if (audiofd >= 0)
	{
		/* Flush out the queue */
		if (abuffer_used() > 0)
			ioctl(audiofd, I_FLUSH, FLUSHW);

		/* Close the stream */
		close(audiofd);
		audiofd = -2;
	}

	/* We're closed */
	aopen = FALSE;

	/* Free the buffer */
	g_free(buffer_ptr);
	buffer_ptr = buffer_end = buffer_read = buffer_write = NULL;
}

void * abuffer_loop(void *arg)
{
	gint length, bdiff = paused;

	while (playing)
	{
		if (abuffer_used() > prebuffer_size)
			prebuffer = FALSE;

		/* If pause has changed status, go ahead and do it */
		if (adinfo.play.pause ^ paused)
		{
			audio_info_t info;

			/* Make the structure harmless */
			AUDIO_INITINFO(&info);

			/* Set the pause */
			info.play.pause = paused;

			/* Set the option */
			ioctl(audiofd, AUDIO_SETINFO, &info);

			/* Readback the new data */
			ioctl(audiofd, AUDIO_GETINFO, &adinfo);
		}

		/* If we're not paused and have data, send it */
		if (abuffer_used() > 0 && !paused && !prebuffer)
		{
			/* we have something in the buffer and we're not paused */
			length = MIN(SUN_AUDIO_CACHE, abuffer_used()); 
			bdiff = buffer_end - buffer_read;

			/* Write out the buffer */
			if (bdiff < length)
			{
				audio_play(buffer_read, bdiff);
				audio_play(buffer_ptr, length - bdiff);
				pthread_mutex_lock(&buflock);
				buffer_read = buffer_ptr + length - bdiff;
				pthread_mutex_unlock(&buflock);
			}
			else
			{
				audio_play(buffer_read, length);
				pthread_mutex_lock(&buflock);
				buffer_read += length;
				pthread_mutex_unlock(&buflock);
			}
		}
		else if (flush != -1)
		{
			audio_info_t info;

			AUDIO_INITINFO(&info);
			info.play.pause = 1;
			ioctl(audiofd, AUDIO_SETINFO, &info);
            
			/* Flush out the queue */
                        /* Yes this is voodoo magic. On my box, this works.
                           But, if I remove the calls to xmms_usleep(), the
                           I_FLUSH ioctl puts the audio into a screwy state
                           about a quarter of the time. */
			xmms_usleep(10000);
			ioctl(audiofd, I_FLUSH, FLUSHW);
			xmms_usleep(10000);

			/* Make the structure harmless */
			AUDIO_INITINFO(&info);

			/* Make sure we clear the statistics */
			info.play.samples = 0;
			info.play.pause = 0;
			info.play.eof = 0;
			info.play.error = 0;
			ioctl(audiofd, AUDIO_SETINFO, &info);
			ioctl(audiofd, AUDIO_GETINFO, &adinfo);

			audio_scount = 0;

			/* Force the time and byte offsets */
			time_offset = flush;
			byte_offset = ((double)flush * stream_attrs.bps / 1000);
            
			/* Reset the buffers */
			buffer_write = buffer_read = buffer_ptr;
            
			/* Finish the flush */
			flush = -1;
		}
		else
		{
			xmms_usleep(10000);
		}
	}

	/* Shutdown the buffer */
	abuffer_shutdown();

	return(NULL);
}

void abuffer_set_audio_params(void)
{
	audio_info_t info;

	if (audiofd >= 0)
	{
		/* Make the audio info structure harmless */
		AUDIO_INITINFO(&info);

		/* Fill out our attributes */
		info.play.sample_rate = stream_attrs.sample_rate;
		info.play.channels = stream_attrs.channels;
		info.play.precision = stream_attrs.output_precision;
		info.play.encoding = AUDIO_ENCODING_LINEAR;
		if (sun_cfg.channel_flags)
			info.play.port = sun_cfg.channel_flags;

		/* Make sure we're not muted */
		info.output_muted = 0;
	
		/* Set input audio data encoding for CD */
		info.record.sample_rate = 44100;
		info.record.channels = 2;
		info.record.precision = 16;
		info.record.encoding = AUDIO_ENCODING_LINEAR;
	
		/* Set the info */
		if (ioctl(audiofd, AUDIO_SETINFO, &info) == -1)
		{
			perror ("xmms: could not set audio params");
		}
		else
		{
			ioctl(audiofd, AUDIO_GETINFO, &adinfo);
		}
	}
}

/* This function sets the parameters for endian-ness and signed status */
/* note that for precision=8 we set endianness to be native for speed in */
/* dsp_memcpy */
gint abuffer_open(AFormat fmt, gint rate, gint nch)
{
	int audiofd;
	struct audio_device aud_dev;
	char *ctl_dev;
	input_format = fmt;

	switch (fmt)
	{
		case FMT_U8:
		case FMT_S8:
			stream_attrs.precision = 8;
			break;
		case FMT_U16_LE:
		case FMT_U16_BE:
		case FMT_U16_NE:
		case FMT_S16_LE:
		case FMT_S16_BE:
		case FMT_S16_NE:
			stream_attrs.precision = 16;
			break;
	}

	switch (fmt)
	{
		case FMT_S8:
		case FMT_S16_LE:
		case FMT_S16_BE:
		case FMT_S16_NE:
			stream_attrs.is_signed = TRUE;
			break;
		case FMT_U8:
		case FMT_U16_LE:
		case FMT_U16_BE:
		case FMT_U16_NE:
			stream_attrs.is_signed = FALSE;
			break;
	}

	switch (fmt)
	{
		case FMT_S8:
		case FMT_U8:
		case FMT_S16_NE:
		case FMT_U16_NE:
#if WORDS_BIGENDIAN
			stream_attrs.is_bigendian = TRUE;
#else
			stream_attrs.is_bigendian = FALSE;
#endif
			break;
		case FMT_S16_LE:
		case FMT_U16_LE:
			stream_attrs.is_bigendian = FALSE;
			break;
		case FMT_U16_BE:
		case FMT_S16_BE:
			stream_attrs.is_bigendian = TRUE;
			break;
	}

	/* Check the card's capabilities */
	ctl_dev = g_strconcat(get_audiodev(), "ctl", NULL);
	/* FIXME: Check if open fails */
	audiofd = open(ctl_dev, O_WRONLY);
	g_free(ctl_dev);
	ioctl(audiofd, AUDIO_GETDEV, &aud_dev);
	close(audiofd);
    
	if (!strcmp(aud_dev.name, "SUNW,CS4231"))
	{
		/* CS4231 card (SS[45], Ultra's) */
		/* 16 bit sound only */
		stream_attrs.output_precision=16;
		stream_attrs.is_signed = TRUE;
	}
	else if (!strcmp(aud_dev.name, "SUNW,dbri"))
	{
		/* DBRI card (SS10's and 20's) */
		/* 16 bit sound only           */
		stream_attrs.output_precision=16;
		stream_attrs.is_signed = TRUE;
	}
	else if (!strcmp(aud_dev.name, "SUNW,audioens"))
	{
		/* Ensoniq1371/1373 and Creative Labs 5880 audio controller */
		stream_attrs.output_precision=stream_attrs.precision;
		stream_attrs.is_signed = TRUE;
		sun_cfg.channel_flags = 0;
	}
	else if (!strcmp(aud_dev.name, "SUNW,sb16") || !strcmp(aud_dev.name, "TOOLS,sbpci"))
	{
		/* Sound blaster (or compatible, inc AWE32/64) */
		/* 8 or 16 bit sound */
		/* Also SB PCI card using driver from http://www.tools.de/solaris/sbpci/ */
		stream_attrs.output_precision = stream_attrs.precision;
	}
	else if (!strcmp(aud_dev.name, "SUNW,sbpro"))
	{
		/* Sound blaster pro (or compatible) */
		/* 8 bit sound only, and only up to 22,050Hz in stereo */
		/* Currently, the 22kHz limit is not worked around */
		stream_attrs.output_precision = 8;
	}
	else if (!strcmp(aud_dev.name, "SUNW,audiots"))
	{
		/* AUDIOTS card (Sun Blade 100's) */
		/* 16 bit sound only           */
		stream_attrs.output_precision=16;
		stream_attrs.is_signed = TRUE;
	}
	else if (!strcmp(aud_dev.name, "USB Audio"))
	{
		/* USB Audio (usb_ac) */
		/* 8 or 16 bit sound */
		stream_attrs.output_precision = stream_attrs.precision;
	}
	else if (!strcmp(aud_dev.name, "SUNW,audio810"))
	{
		/* AMD8111 AC'97 audio (SunOS 5.10) */
		stream_attrs.output_precision = stream_attrs.precision;
		stream_attrs.is_signed = TRUE;
		sun_cfg.channel_flags = 0;
	}       
	else if (!strcmp(aud_dev.name, "SUNW,oss"))
	{
		/* OSS emulating /dev/audio */
		stream_attrs.output_precision=16;
		stream_attrs.is_signed = TRUE;
	}
	else
	{
		g_warning("solaris output: Unknown sound card type: %s", aud_dev.name);
		g_warning("solaris output: Assuming capable of the bitstream");
		stream_attrs.output_precision = stream_attrs.precision;
	}
	
	stream_attrs.bps = rate * nch;
	if (stream_attrs.precision == 16)
		stream_attrs.bps *= 2;

	stream_attrs.channels = nch;
	stream_attrs.sample_rate = rate;

	/* Define the length for the buffer */
	buffer_len = (sun_cfg.buffer_size * stream_attrs.bps) / 1000;
	prebuffer_size = (buffer_len * sun_cfg.prebuffer) / 100;
	if (buffer_len - prebuffer_size < 4096)
		prebuffer_size = buffer_len - 4096;

	buffer_ptr = g_malloc0(buffer_len);
	buffer_write = buffer_ptr;
	buffer_read = buffer_ptr;
	buffer_end = buffer_ptr + buffer_len;

	/* Open the audio device */
	if (abuffer_startup())
	{
		/* Shutdown the buffer */
		abuffer_shutdown();
		return(0);
	}

	pthread_create(&buffer_thread, NULL, abuffer_loop, NULL);

	while (audiofd == -2)
		xmms_usleep(10000);

	if (audiofd == -1)
	{
		pthread_join(buffer_thread, NULL);
		return 0;
	}
	return 1;
}

/* This was in mixer.c for the OSS driver, but since we _have_
 * to have the audio device open for writing, it has to be here.
 * The alternative is to have fd defined in Sun.h and used as
 * an extern, but that is messier and probably more trouble than
 * it's worth
 */

/*
 * xmms expects 0<=volume<=100; Solaris audio is 0<=vol<=255
 */

/*
 * l is returned with the left channel volume,
 * r is returned with the right channel volume
 */

/* get volume doesn't require write on audio device                    */
/* However, if we have audio device open, might as well save an open() */
/* This is called fairly regularly by XMMS (for visual volume slider,  */
/* I assume) so is worth optimising.  As a result, this is now         */
/* optimised for speed, not readability.                               */

void abuffer_get_volume(int *left, int *right)
{
	audio_info_t info;

	/* Since we do not know if CD is playing or not, we need ctlfd 
	   open all the time. */
	if (ctlfd < 0)
		init_ctlfd(); 	

	if (ctlfd == -1)
		return;

	if (ioctl (ctlfd, AUDIO_GETINFO, &info) >= 0)
	{
		/* Try to figure how to get values. */
		int gain = ((info.play.gain - AUDIO_MIN_GAIN) * 100) / (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN);
		int balance = ((info.play.balance - AUDIO_MID_BALANCE) * 100) /
			(AUDIO_RIGHT_BALANCE - AUDIO_LEFT_BALANCE);

		if (balance == 0)
		{
			*left = gain;
			*right = gain;
		}
		else if (balance > 0)
		{
			*right = gain;
			*left = (gain * (100 - balance)) / 100;
		}
		else
		{
			*left = gain;
			*right = (gain * (100 + balance)) / 100;
		}
	}
}

/*
 * Set volume (gain)
 */
void abuffer_set_volume(int left, int right)
{
	audio_info_t info;

	/* Since we do not know if CD is playing or not, we need ctlfd 
	   open all the time. */
	if (ctlfd < 0)
		init_ctlfd(); 	

	if (ctlfd == -1)
		return;
		
	AUDIO_INITINFO (&info);

        /* Calculate gain and volume. */
	info.play.gain = (MAX(left, right) * (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) / 100;

	if (left + right == 0)
		info.play.balance = AUDIO_MID_BALANCE;
	else
		info.play.balance = AUDIO_LEFT_BALANCE + (right * AUDIO_RIGHT_BALANCE) / (left + right);

        /* It seems that the sound card (at least CS4321 on Ultra10) truncates
           the last three bits of the volume.  Round it so that it does not keep
           drifting away.

           Ideally, this should be somehow automatically determined. */

	/* FIXME:  Read it back instead? */
        if ((info.play.gain & 0xf) >= 4)
		info.play.gain += 4;

        info.record.gain =  info.play.gain;
        info.record.balance = info.play.balance;

        ioctl (ctlfd, AUDIO_SETINFO, &info);
}

/*
 * Simple function to check if audio device is open 
 * Returns 1 if open, 0 otherwise.
 */
int abuffer_isopen(void)
{
	if (audiofd < 0)
		return 0;

	return 1;
}

/*
 * abuffer_update_dev()
 * Function to update the open devices in case they have changed (eg, someone
 * has used audiocontrol.  This makes it a bit friendlier to other apps.
 * returns 0 on success, 1 on failure.
 */

int abuffer_update_dev(void)
{
	audio_info_t ainfo;

	if (audiofd < 0)
		return 1;

	if (ioctl(audiofd, AUDIO_GETINFO, &ainfo) == -1)
		return 1;
    
	sun_cfg.channel_flags = ainfo.play.port;

	return 0;
}

/*
 * abuffer_set_dev()
 * Function to set the output devices
 * returns 0 on success, 1 if device not open
 */
int abuffer_set_dev(void)
{
	audio_info_t ainfo;
	if (audiofd < 0)
		return 1;

	AUDIO_INITINFO(&ainfo);
	ainfo.play.port = sun_cfg.channel_flags;
	ioctl(audiofd, AUDIO_SETINFO, &ainfo);
	return 0;
}
