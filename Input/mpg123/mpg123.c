#include "mpg123.h"
#include "id3_header.h"
#include "libxmms/configfile.h"
#include "libxmms/titlestring.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define CPU_HAS_MMX() (cpu_fflags & 0x800000)
#define CPU_HAS_3DNOW() (cpu_efflags & 0x80000000)

static const long outscale = 32768;

PlayerInfo *mpg123_info = NULL;
static pthread_t decode_thread;

static gboolean audio_error = FALSE, output_opened = FALSE, dopause = FALSE;
gint mpg123_bitrate, mpg123_frequency, mpg123_length, mpg123_layer, mpg123_lsf;
gchar *mpg123_title = NULL, *mpg123_filename = NULL;
static int disp_bitrate, skip_frames = 0;
static int cpu_fflags, cpu_efflags;
gboolean mpg123_stereo, mpg123_mpeg25;
int mpg123_mode;

const char *mpg123_id3_genres[GENRE_MAX] =
{
	N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
	N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
	N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
	N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
	N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
	N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
	N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
	N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
	N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
	N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("AlternRock"),
	N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
	N_("Meditative"), N_("Instrumental Pop"),
	N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
	N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
	N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
	N_("Southern Rock"), N_("Comedy"), N_("Cult"),
	N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
	N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
	N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
	N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
	N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
	N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
	N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
	N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
	N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
	N_("Gothic Rock"), N_("Progressive Rock"),
	N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
	N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
	N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
	N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
	N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
	N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
	N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
	N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
	N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
	N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
	N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
	N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
	N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
	N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
	N_("Contemporary Christian"), N_("Christian Rock"),
	N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
	N_("Anime"), N_("JPop"), N_("Synthpop")
};

double mpg123_compute_tpf(struct frame *fr)
{
	const int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = bs[fr->lay];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

static void set_synth_functions(struct frame *fr)
{
	typedef int (*func) (real *, int, unsigned char *, int *);
	typedef int (*func_mono) (real *, unsigned char *, int *);
	typedef void (*func_dct36)(real *, real *, real *, real *, real *);

	int ds = fr->down_sample;
	int p8 = 0;

	static func funcs[][3] =
	{
		{mpg123_synth_1to1,
		 mpg123_synth_2to1,
		 mpg123_synth_4to1},
		{mpg123_synth_1to1_8bit,
		 mpg123_synth_2to1_8bit,
		 mpg123_synth_4to1_8bit},
#ifdef USE_SIMD
		{mpg123_synth_1to1_mmx,
		 mpg123_synth_2to1,
		 mpg123_synth_4to1},
		{mpg123_synth_1to1_3dnow,
		 mpg123_synth_2to1,
		 mpg123_synth_4to1}
#endif
	};

	static func_mono funcs_mono[2][4] =
	{
		{mpg123_synth_1to1_mono,
		 mpg123_synth_2to1_mono,
		 mpg123_synth_4to1_mono},
		{mpg123_synth_1to1_8bit_mono,
		 mpg123_synth_2to1_8bit_mono,
		 mpg123_synth_4to1_8bit_mono}
	};

#ifdef USE_SIMD
	static func_dct36 funcs_dct36[2] = {mpg123_dct36, dct36_3dnow};
#endif

	if (mpg123_cfg.resolution == 8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[p8][ds];
	fr->synth_type = SYNTH_FPU;

#ifdef USE_SIMD
	fr->dct36 = funcs_dct36[0];
   
	if (CPU_HAS_3DNOW() && !p8 &&
	    (mpg123_cfg.default_synth == SYNTH_3DNOW || 
	     mpg123_cfg.default_synth == SYNTH_AUTO))
	{
		fr->synth = funcs[3][ds]; /* 3DNow! optimized synth_1to1() */
		fr->dct36 = funcs_dct36[1]; /* 3DNow! optimized dct36() */
		fr->synth_type = SYNTH_3DNOW;
	}
	else if (CPU_HAS_MMX() && !p8 &&
		 (mpg123_cfg.default_synth == SYNTH_MMX ||
		  mpg123_cfg.default_synth == SYNTH_AUTO))
	{
		fr->synth = funcs[2][ds]; /* MMX optimized synth_1to1() */
		fr->synth_type = SYNTH_MMX;
	}
#endif
	if (p8)
	{
		mpg123_make_conv16to8_table();
	}
}

static void init(void)
{
	ConfigFile *cfg;

	mpg123_make_decode_tables(outscale);

	mpg123_cfg.resolution = 16;
	mpg123_cfg.channels = 2;
	mpg123_cfg.downsample = 0;
	mpg123_cfg.http_buffer_size = 128;
	mpg123_cfg.http_prebuffer = 25;
	mpg123_cfg.proxy_port = 8080;
	mpg123_cfg.proxy_use_auth = FALSE;
	mpg123_cfg.proxy_user = NULL;
	mpg123_cfg.proxy_pass = NULL;
	mpg123_cfg.cast_title_streaming = TRUE;
	mpg123_cfg.use_udp_channel = FALSE;
	mpg123_cfg.title_override = FALSE;
	mpg123_cfg.disable_id3v2 = FALSE;
	mpg123_cfg.detect_by = DETECT_EXTENSION;
	mpg123_cfg.default_synth = SYNTH_AUTO;

	cfg = xmms_cfg_open_default_file();

	xmms_cfg_read_int(cfg, "MPG123", "resolution", &mpg123_cfg.resolution);
	xmms_cfg_read_int(cfg, "MPG123", "channels", &mpg123_cfg.channels);
	xmms_cfg_read_int(cfg, "MPG123", "downsample", &mpg123_cfg.downsample);
	xmms_cfg_read_int(cfg, "MPG123", "http_buffer_size", &mpg123_cfg.http_buffer_size);
	xmms_cfg_read_int(cfg, "MPG123", "http_prebuffer", &mpg123_cfg.http_prebuffer);
	xmms_cfg_read_boolean(cfg, "MPG123", "save_http_stream", &mpg123_cfg.save_http_stream);
	if (!xmms_cfg_read_string(cfg, "MPG123", "save_http_path", &mpg123_cfg.save_http_path))
		mpg123_cfg.save_http_path = g_strdup(g_get_home_dir());
	xmms_cfg_read_boolean(cfg, "MPG123", "cast_title_streaming", &mpg123_cfg.cast_title_streaming);
	xmms_cfg_read_boolean(cfg, "MPG123", "use_udp_channel", &mpg123_cfg.use_udp_channel);

	xmms_cfg_read_boolean(cfg, "MPG123", "use_proxy", &mpg123_cfg.use_proxy);
	if (!xmms_cfg_read_string(cfg, "MPG123", "proxy_host", &mpg123_cfg.proxy_host))
		mpg123_cfg.proxy_host = g_strdup("localhost");
	xmms_cfg_read_int(cfg, "MPG123", "proxy_port", &mpg123_cfg.proxy_port);
	xmms_cfg_read_boolean(cfg, "MPG123", "proxy_use_auth", &mpg123_cfg.proxy_use_auth);
	xmms_cfg_read_string(cfg, "MPG123", "proxy_user", &mpg123_cfg.proxy_user);
	xmms_cfg_read_string(cfg, "MPG123", "proxy_pass", &mpg123_cfg.proxy_pass);

	xmms_cfg_read_boolean(cfg, "MPG123", "title_override", &mpg123_cfg.title_override);
	xmms_cfg_read_boolean(cfg, "MPG123", "disable_id3v2", &mpg123_cfg.disable_id3v2);
	if (!xmms_cfg_read_string(cfg, "MPG123", "id3_format", &mpg123_cfg.id3_format))
		mpg123_cfg.id3_format = g_strdup("%p - %t");
	xmms_cfg_read_int(cfg, "MPG123", "detect_by", &mpg123_cfg.detect_by);
	xmms_cfg_read_int(cfg, "MPG123", "default_synth", &mpg123_cfg.default_synth);

	xmms_cfg_free(cfg);

	if (mpg123_cfg.resolution != 16 && mpg123_cfg.resolution != 8)
		mpg123_cfg.resolution = 16;
	mpg123_cfg.channels = CLAMP(mpg123_cfg.channels, 0, 2);
	mpg123_cfg.downsample = CLAMP(mpg123_cfg.downsample, 0, 2);
	mpg123_getcpuflags(&cpu_fflags, &cpu_efflags);
}

/* needed for is_our_file() */
static int read_n_bytes(FILE * file, guint8 * buf, int n)
{

	if (fread(buf, 1, n, file) != n)
	{
		return FALSE;
	}
	return TRUE;
}

static guint32 convert_to_long(guint8 * buf)
{

	return (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
}

static guint16 read_wav_id(char *filename)
{
	FILE *file;
	guint8 buf[4], head[4];
	long seek = 0;

	if (!(file = fopen(filename, "rb")))
		return 0;

	if (!(read_n_bytes(file, buf, 4)))
		goto out;
	if (strncmp(buf, "RIFF", 4))
		goto out;
	if (fseek(file, 4, SEEK_CUR) != 0)
		goto out;
	if (!(read_n_bytes(file, buf, 4)))
		goto out;
	if (strncmp(buf, "WAVE", 4))
		goto out;
	do
	{
		/*
		 * We'll be looking for the fmt-chunk which comes
		 * before the data-chunk
		 */

		/*
		 * A chunk consists of an header identifier (4 bytes),
		 * the length of the chunk (4 bytes), and the
		 * chunkdata itself, padded to be an even number of
		 * bytes.  We'll skip all chunks until we find the
		 * "data"-one which could contain mpeg-data
		 */
		if (seek != 0)
			if (fseek(file, seek, SEEK_CUR) != 0)
				goto out;
		if (!(read_n_bytes(file, head, 4)))
			goto out;
		if (!(read_n_bytes(file, buf, 4)))
			goto out;
		seek = convert_to_long(buf);
		/* Has to be even (padding) */
		seek += (seek % 2);
		if (seek >= 2 && !strncmp(head, "fmt ", 4))
		{
			guint16 wavid;
			if (!(read_n_bytes(file, buf, 2)))
				goto out;
			wavid = buf[0] | buf[1] << 8;
			seek -= 2;
			/*
			 * we could go on looking for other things,
			 * but all we wanted was the wavid
			 */
			fclose(file);
			return wavid;
		}
	} while (strncmp(head, "data", 4));
 out:
	fclose(file);
	return 0;
}

#define DET_BUF_SIZE 1024

static gboolean mpg123_detect_by_content(char *filename)
{
	FILE *file;
	struct frame fr;
	gboolean ret;

	if ((file = fopen(filename, "rb")) == NULL)
		return FALSE;

	ret = mpg123_get_first_frame(file, &fr, NULL);
	fclose(file);

	return ret;
}

static int is_our_file(char *filename)
{
	char *ext;
	guint16 wavid;

	if (!strncasecmp(filename, "http://", 7))
	{			/* We assume all http:// (except those ending in .ogg) are mpeg -- why do we do that? */
		ext = strrchr(filename, '.');
		if (ext) 
		{
			if (!strncasecmp(ext, ".ogg", 4)) 
				return FALSE;
			if (!strncasecmp(ext, ".rm", 3) || 
			    !strncasecmp(ext, ".ra", 3)  ||
			    !strncasecmp(ext, ".rpm", 4)  ||
			    !strncasecmp(ext, ".fla", 4)  ||
			    !strncasecmp(ext, ".flac", 5)  ||
			    !strncasecmp(ext, ".ram", 4))
				return FALSE;
		}
		return TRUE;
	}
	if(mpg123_cfg.detect_by == DETECT_CONTENT)
		return (mpg123_detect_by_content(filename));

	ext = strrchr(filename, '.');
	if (ext)
	{
		if (!strncasecmp(ext, ".mp2", 4) || !strncasecmp(ext, ".mp3", 4))
		{
			return TRUE;
		}
		if (!strncasecmp(ext, ".wav", 4))
		{
			wavid = read_wav_id(filename);
			if (wavid == 85 || wavid == 80)
			{	/* Microsoft says 80, files say 85... */
				return TRUE;
			}
		}
	}

	if(mpg123_cfg.detect_by == DETECT_BOTH)
		return (mpg123_detect_by_content(filename));
	return FALSE;
}

static void play_frame(struct frame *fr)
{
	if (fr->error_protection)
	{
		bsi.wordpointer += 2;
		/*  mpg123_getbits(16); */	/* skip crc */
	}
	if(!fr->do_layer(fr))
	{
		skip_frames = 2;
		mpg123_info->output_audio = FALSE;
	}
	else
	{
		if(!skip_frames)
			mpg123_info->output_audio = TRUE;
		else
			skip_frames--;
	}
}

const char *mpg123_get_id3_genre(unsigned char genre_code)
{
	if (genre_code < GENRE_MAX)
		return gettext(mpg123_id3_genres[genre_code]);

	return "";
}

/*
 * Function extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static gchar *extname(const char *filename)
{
	gchar *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

/*
 * Function id3v1_to_id3v2 (v1, v2)
 *
 *    Convert ID3v1 tag `v1' to ID3v2 tag `v2'.
 *
 */
struct id3v2tag_t* mpg123_id3v1_to_id3v2(struct id3v1tag_t *v1)
{
	char *year;
	struct id3v2tag_t *v2 = g_malloc0(sizeof (struct id3v2tag_t));

	v2->title = g_strstrip(g_strndup(v1->title, 30));
	v2->artist = g_strstrip(g_strndup(v1->artist, 30));
	v2->album = g_strstrip(g_strndup(v1->album, 30));
	v2->comment = g_strstrip(g_strndup(v1->u.v1_0.comment, 30));
	v2->genre = g_strstrip(g_strdup(mpg123_get_id3_genre(v1->genre)));
	
	year = g_strndup(v1->year, 4);
	v2->year = atoi(year);
	g_free(year);

	/* Check for v1.1 tags. */
	if (v1->u.v1_1.__zero == 0)
		v2->track_number = v1->u.v1_1.track_number;
	else
		v2->track_number = 0;

	return v2;
}

static char* mpg123_getstr(char* str)
{
	if (str && strlen(str) > 0)
		return str;
	return NULL;
}

/*
 * Function mpg123_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
char *mpg123_format_song_title(struct id3v2tag_t *tag, char *filename)
{
	char *ret = NULL, *path, *temp;
	TitleInput *input;

	XMMS_NEW_TITLEINPUT(input);

	if (tag)
	{
		input->performer = mpg123_getstr(tag->artist);
		input->album_name = mpg123_getstr(tag->album);
		input->track_name = mpg123_getstr(tag->title);
		input->year = tag->year;
		input->track_number = tag->track_number;
		input->genre = mpg123_getstr(tag->genre);
		input->comment = mpg123_getstr(tag->comment);
	}
	path = g_strdup(filename);
	temp = strrchr(path, '/');
	if (temp)
		*temp = '\0';
	input->file_name = g_basename(filename);
	input->file_path = g_strdup_printf("%s/", path);
	input->file_ext = extname(filename);
	ret = xmms_get_titlestring(mpg123_cfg.title_override ?
				   mpg123_cfg.id3_format :
				   xmms_get_gentitle_format(), input);
	g_free(input);
	g_free(path);

	if (!ret)
	{
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (extname(ret) != NULL)
			*(extname(ret) - 1) = '\0';	/* removes period */
	}

	return ret;
}

static int id3v2_get_num(struct id3_tag *id3d, int id)
{
	int r = 0, num;
	struct id3_frame *id3frm = id3_get_frame(id3d, id, 1);
	if (id3frm && (num = id3_get_text_number(id3frm)) > 0)
		r = num;
	return r;
}

static char* id3v2_get_text(struct id3_tag *id3d, int id)
{
	char *c;
	struct id3_frame *id3frm = id3_get_frame(id3d, id, 1);
	if (!id3frm)
		return NULL;
	switch (id)
	{
		case ID3_COMM:
			c = id3_get_comment(id3frm);
			break;
		case ID3_TCON:
			c = id3_get_content(id3frm);
			break;
		default:
			c = id3_get_text(id3frm);
			break;
	}
	return c;
}

/*
 * Function mpg123_get_id3v2 (id3d, tag)
 *
 *    Get desired contents from the indicated id3tag and store it in
 *    `tag'. 
 *
 */
struct id3v2tag_t* mpg123_id3v2_get(struct id3_tag *id3d)
{
	struct id3v2tag_t *tag = g_malloc0(sizeof (struct id3v2tag_t));

	tag->title = id3v2_get_text(id3d, ID3_TIT2);
	tag->artist = id3v2_get_text(id3d, ID3_TPE1);
	if (!tag->artist)
		tag->artist = id3v2_get_text(id3d, ID3_TPE2);
	tag->album = id3v2_get_text(id3d, ID3_TALB);
	tag->year = id3v2_get_num(id3d, ID3_TYER);
	tag->track_number = id3v2_get_num(id3d, ID3_TRCK);
	tag->comment = id3v2_get_text(id3d, ID3_COMM);
	tag->genre = id3v2_get_text(id3d, ID3_TCON);

	return tag;
}

void mpg123_id3v2_destroy(struct id3v2tag_t* tag)
{
	g_free(tag->title);
	g_free(tag->artist);
	g_free(tag->album);
	g_free(tag->comment);
	g_free(tag->genre);

	g_free(tag);
}


/*
 * Function get_song_title (fd, filename)
 *
 *    Get song title of file.  File position of `fd' will be
 *    clobbered.  `fd' may be NULL, in which case `filename' is opened
 *    separately.  The returned song title must be subsequently freed
 *    using g_free().
 *
 */
static gchar *get_song_title(FILE * fd, char *filename)
{
	FILE *file = fd;
	char *ret = NULL;

	if (file || (file = fopen(filename, "rb")) != 0)
	{
		struct id3_tag *id3 = NULL;
		struct id3v1tag_t id3v1tag;
		struct id3v2tag_t *id3tag;

		/*
		 * Try reading ID3v2 tag.
		 */
		if (!mpg123_cfg.disable_id3v2)
		{
			fseek(file, 0, SEEK_SET);
			id3 = id3_open_fp(file, 0);
			if (id3)
			{
				id3tag = mpg123_id3v2_get(id3);
				ret = mpg123_format_song_title(id3tag, filename);
				mpg123_id3v2_destroy(id3tag);
				id3_close(id3);
			}
		}

		/*
		 * Try reading ID3v1 tag.
		 */
		if (!id3 && (fseek(file, -1 * sizeof (id3v1tag), SEEK_END) == 0) &&
		    (fread(&id3v1tag, 1, sizeof (id3v1tag), file) == sizeof (id3v1tag)) &&
		    (strncmp(id3v1tag.tag, "TAG", 3) == 0))
		{
			id3tag = mpg123_id3v1_to_id3v2(&id3v1tag);
			ret = mpg123_format_song_title(id3tag, filename);
			mpg123_id3v2_destroy(id3tag);
		}

		if (!fd)
			/*
			 * File was opened in this function.
			 */
			fclose(file);
	}

	if (ret == NULL)
		/*
		 * Unable to get ID3 tag.
		 */
		ret = mpg123_format_song_title(NULL, filename);

	return ret;
}

static long get_song_length(FILE *file)
{
	int len;
	char tmp[4];

	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, -128, SEEK_END);
	fread(tmp, 1, 3, file);
	if (!strncmp(tmp, "TAG", 3))
		len -= 128;
	return len;
}

static int head_read(FILE *f, guint32 *head)
{
	guint8 tmp[4];
	if (fread(tmp, 1, 4, f) != 4)
		return FALSE;
	*head = tmp[0] << 24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3];
	return TRUE;
}	

static int head_shift(FILE *f, guint32 *head)
{
	guint8 tmp[1];
	*head <<= 8;
	if (fread(tmp, 1, 1, f) != 1)
		return FALSE;
	*head |= tmp[0];
	return TRUE;
}	

/*
 * Check if we are at the beginning of a id3v2 tag and skip it if we
 * are.  Returns FALSE on error.
 */

static int skip_id3v2(FILE *f, guint32 head)
{
	guint32 id3v2size;
	guint8 tmp[6];
	
	if (!((head & 0xffffff00) == (('I' << 24) | ('D' << 16) | ('3' << 8))))
		return TRUE;

	if (fread(tmp, 1, 6, f) != 6)
		return FALSE;
		
	id3v2size = ID3_GET_SIZE28(tmp[2], tmp[3], tmp[4], tmp[5]);

	if (tmp[1] & 0x10)
		/* Footer */
		id3v2size += 10;
	/* Let this fseek fail silently and continue parsing the file */
	fseek(f, id3v2size, SEEK_CUR);
	return TRUE;
}


/*
 * Locate the first frame in a file and return a 'struct frame'.  If
 * 'buffer' is non null, there will be allocated memory and the first
 * frame will be read into it.
 *
 * Returns TRUE on success and zero on failure.
 */
gboolean mpg123_get_first_frame(FILE *fh, struct frame *frm, guint8 **buffer)
{
	guint32 h, h2;
	/*
	 * Mask of things that we don't expect to change between
	 * frames.  This includes: MPEG version, layer, sample rate
	 * and channel mode.
	 */
	guint32 mask = 0xFFFE0CC0;
	int skip = 0;

	rewind(fh);
	if (!head_read(fh, &h))
		return FALSE;
	for (;;)
	{
		int offset;
		struct frame tmp;
		
		while (!mpg123_head_check(h) ||
		       !mpg123_decode_header(frm, h))
		{
			if (!skip_id3v2(fh, h))
				return FALSE;
			if (!head_shift(fh, &h))
				return FALSE;
			if (skip++ > MAX_SKIP_LENGTH)
				return FALSE;
		}

		offset = frm->framesize;
		if (fseek(fh, offset, SEEK_CUR) || !head_read(fh, &h2))
			return FALSE;
		
		if (fseek(fh, -(offset + 4), SEEK_CUR))
			return FALSE;

		if (mpg123_head_check(h2) &&
		    mpg123_decode_header(&tmp, h2) &&
		    (h & mask) == (h2 & mask))
		{
			if (fseek(fh, -4, SEEK_CUR))
				return FALSE;
			if (buffer)
			{
				*buffer = g_malloc(offset + 4);
				if (fread(*buffer, 1, offset + 4, fh) != offset + 4 || 
				    fseek(fh, -(offset + 4), SEEK_CUR))
				{
					g_free(*buffer);
					return FALSE;
				}
			}

			return TRUE;
		}

		if (!head_shift(fh, &h))
			return FALSE;
		skip++;
	}
}

static guint get_song_time(FILE * file)
{
	guint8 *buf;
	struct frame frm;
	xing_header_t xing_header;
	double tpf, bpf;
	guint32 len;

	if (!file)
		return -1;

	if (!mpg123_get_first_frame(file, &frm, &buf))
		return 0;

	tpf = mpg123_compute_tpf(&frm);
	if (mpg123_get_xing_header(&xing_header, buf))
	{
		g_free(buf);
		return (tpf * xing_header.frames * 1000);
	}

	g_free(buf);
	bpf = mpg123_compute_bpf(&frm);
	len = get_song_length(file);
	return ((guint)(len / bpf) * tpf * 1000);
}

static void get_song_info(char *filename, char **title_real, int *len_real)
{
	FILE *file;

	(*len_real) = -1;
	(*title_real) = NULL;

	/*
	 * TODO: Getting song info from http streams.
	 */
	if (strncasecmp(filename, "http://", 7))
	{
		if ((file = fopen(filename, "rb")) != NULL)
		{
			(*len_real) = get_song_time(file);
			(*title_real) = get_song_title(file, filename);
			fclose(file);
		}
	}
}

static int open_output(struct frame fr)
{
	int r;
	AFormat fmt = mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8;
	int freq = mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample;
	int channels = mpg123_cfg.channels == 2 ? fr.stereo : 1;
	r = mpg123_ip.output->open_audio(fmt, freq, channels);

	if (r && dopause)
	{
		mpg123_ip.output->pause(TRUE);
		dopause = FALSE;
	}

	return r;
}


static int mpg123_seek(struct frame *fr, xing_header_t *xh, gboolean vbr, int time)
{
	int jumped = -1;
	
	if (xh)
	{
		int percent = ((double) time * 100.0) /
			(mpg123_info->num_frames * mpg123_info->tpf);
		int byte = mpg123_seek_point(xh, percent);
		jumped = mpg123_stream_jump_to_byte(fr, byte);
	}
	else if (vbr && mpg123_length > 0)
	{
		int byte = ((guint64)time * 1000 * mpg123_info->filesize) /
			mpg123_length;
		jumped = mpg123_stream_jump_to_byte(fr, byte);
	}
	else
	{
		int frame = time / mpg123_info->tpf;
		jumped = mpg123_stream_jump_to_frame(fr, frame);
	}

	return jumped;
}


static void *decode_loop(void *arg)
{
	struct frame fr = {0}, temp_fr;
	gboolean have_xing_header = FALSE, vbr = FALSE;
	int disp_count = 0, temp_time;
	char *filename = arg;
	xing_header_t xing_header;

	/* This is used by fileinfo on http streams */
	mpg123_bitrate = 0;

	mpg123_pcm_sample = g_malloc0(32768);
	mpg123_pcm_point = 0;
	mpg123_filename = filename;

	mpg123_read_frame_init();

	mpg123_open_stream(filename, -1);
	if (mpg123_info->eof || !mpg123_read_frame(&fr))
		mpg123_info->eof = TRUE;
	if (!mpg123_info->eof && mpg123_info->going)
	{
		if (mpg123_cfg.channels == 2)
			fr.single = -1;
		else
			fr.single = 3;

		fr.down_sample = mpg123_cfg.downsample;
		fr.down_sample_sblimit = SBLIMIT >> mpg123_cfg.downsample;
		set_synth_functions(&fr);
		mpg123_init_layer3(fr.down_sample_sblimit);

		mpg123_info->tpf = mpg123_compute_tpf(&fr);
		if (strncasecmp(filename, "http://", 7))
		{
			if (mpg123_stream_check_for_xing_header(&fr, &xing_header))
			{
				mpg123_info->num_frames = xing_header.frames;
				have_xing_header = TRUE;
				if (xing_header.bytes == 0)
				{
					if (mpg123_info->filesize > 0)
						xing_header.bytes = mpg123_info->filesize;
					else
						have_xing_header = FALSE;
				}
				mpg123_read_frame(&fr);
			}
		}

		for (;;)
		{
			memcpy(&temp_fr, &fr, sizeof(struct frame));
			if (!mpg123_read_frame(&temp_fr))
			{
				mpg123_info->eof = TRUE;
				break;
			}
			if (fr.lay != temp_fr.lay ||
			    fr.sampling_frequency != temp_fr.sampling_frequency ||
	        	    fr.stereo != temp_fr.stereo || fr.lsf != temp_fr.lsf)
				memcpy(&fr,&temp_fr,sizeof(struct frame));
			else
				break;
		}

		if (!have_xing_header && strncasecmp(filename, "http://", 7))
			mpg123_info->num_frames = mpg123_calc_numframes(&fr);

		memcpy(&fr, &temp_fr, sizeof(struct frame));
		mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
		disp_bitrate = mpg123_bitrate;
		mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
		mpg123_stereo = fr.stereo;
		mpg123_layer = fr.lay;
		mpg123_lsf = fr.lsf;
		mpg123_mpeg25 = fr.mpeg25;
		mpg123_mode = fr.mode;

		if (strncasecmp(filename, "http://", 7))
		{
			mpg123_length =
				mpg123_info->num_frames * mpg123_info->tpf * 1000;
			if (!mpg123_title)
				mpg123_title = get_song_title(NULL,filename);
		}
		else
		{
			if (!mpg123_title)
				mpg123_title = mpg123_http_get_title(filename);
			mpg123_length = -1;
		}
		mpg123_ip.set_info(mpg123_title, mpg123_length,
				   mpg123_bitrate * 1000,
				   mpg123_freqs[fr.sampling_frequency],
				   fr.stereo);
		output_opened = TRUE;
		if (!open_output(fr))
		{
			audio_error = TRUE;
			mpg123_info->eof = TRUE;
		}
		else
			play_frame(&fr);
	}

	mpg123_info->first_frame = FALSE;
	while (mpg123_info->going)
	{
		if (mpg123_info->jump_to_time != -1)
		{
			void *xp = NULL;
			if (have_xing_header)
				xp = &xing_header;
			if (mpg123_seek(&fr, xp, vbr,
					mpg123_info->jump_to_time) > -1)
			{
				mpg123_ip.output->flush(mpg123_info->jump_to_time * 1000);
				mpg123_info->eof = FALSE;
			}
			mpg123_info->jump_to_time = -1;
		}
		if (!mpg123_info->eof)
		{
			if (mpg123_read_frame(&fr) != 0)
			{
				if(fr.lay != mpg123_layer || fr.lsf != mpg123_lsf)
				{
					memcpy(&temp_fr, &fr, sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.lay == temp_fr.lay && fr.lsf == temp_fr.lsf)
						{
							mpg123_layer = fr.lay;
							mpg123_lsf = fr.lsf;
							memcpy(&fr,&temp_fr,sizeof(struct frame));
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
						
					}
				}
				if(mpg123_freqs[fr.sampling_frequency] != mpg123_frequency || mpg123_stereo != fr.stereo)
				{
					memcpy(&temp_fr,&fr,sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.sampling_frequency == temp_fr.sampling_frequency && temp_fr.stereo == fr.stereo)
						{
							mpg123_ip.output->buffer_free();
							mpg123_ip.output->buffer_free();
							while(mpg123_ip.output->buffer_playing() && mpg123_info->going && mpg123_info->jump_to_time == -1)
								xmms_usleep(20000);
							if(!mpg123_info->going)
								break;
							temp_time = mpg123_ip.output->output_time();
							mpg123_ip.output->close_audio();
							mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
							mpg123_stereo = fr.stereo;
							if (!mpg123_ip.output->open_audio(mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8,
								mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample,
				  				mpg123_cfg.channels == 2 ? fr.stereo : 1))
							{
								audio_error = TRUE;
								mpg123_info->eof = TRUE;
							}
							mpg123_ip.output->flush(temp_time);
							mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
							memcpy(&fr,&temp_fr,sizeof(struct frame));
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
					}					
				}
				
				if (tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index] != mpg123_bitrate)
					mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
				
				if (!disp_count)
				{
					disp_count = 20;
					if (mpg123_bitrate != disp_bitrate)
					{
						/* FIXME networks streams */
						disp_bitrate = mpg123_bitrate;
						if(!have_xing_header && strncasecmp(filename,"http://",7))
						{
							double rel = mpg123_relative_pos();
							if (rel)
							{
								mpg123_length = mpg123_ip.output->written_time() / rel;
								vbr = TRUE;
							}

							if (rel == 0 || !(mpg123_length > 0))
							{
								mpg123_info->num_frames = mpg123_calc_numframes(&fr);
								mpg123_info->tpf = mpg123_compute_tpf(&fr);
								mpg123_length = mpg123_info->num_frames * mpg123_info->tpf * 1000;
							}


						}
						mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
					}
				}
				else
					disp_count--;
				play_frame(&fr);
			}
			else
			{
				mpg123_ip.output->buffer_free();
				mpg123_ip.output->buffer_free();
				mpg123_info->eof = TRUE;
				xmms_usleep(10000);
			}
		}
		else
		{
			xmms_usleep(10000);
		}
	}
	g_free(mpg123_title);
	mpg123_title = NULL;
	mpg123_stream_close();
	if (output_opened && !audio_error)
		mpg123_ip.output->close_audio();
	g_free(mpg123_pcm_sample);
	mpg123_filename = NULL;
	g_free(filename);
	pthread_exit(NULL);
}

static void play_file(char *filename)
{
	mpg123_info = g_malloc0(sizeof (PlayerInfo));
	mpg123_info->going = 1;
	mpg123_info->first_frame = TRUE;
	mpg123_info->output_audio = TRUE;
	mpg123_info->jump_to_time = -1;
	skip_frames = 0;
	audio_error = FALSE;
	output_opened = FALSE;
	dopause = FALSE;

	pthread_create(&decode_thread, NULL, decode_loop, g_strdup(filename));
}

static void stop(void)
{
	if (mpg123_info && mpg123_info->going)
	{
		mpg123_info->going = FALSE;
		pthread_join(decode_thread, NULL);
		g_free(mpg123_info);
		mpg123_info = NULL;
	}
}

static void seek(int time)
{
	mpg123_info->jump_to_time = time;

	while (mpg123_info->jump_to_time != -1)
		xmms_usleep(10000);
}

static void do_pause(short p)
{
	if (output_opened)
		mpg123_ip.output->pause(p);
	else
		dopause = p;
}

static int get_time(void)
{
	if (audio_error)
		return -2;
	if (!mpg123_info)
		return -1;
	if (!mpg123_info->going || (mpg123_info->eof && !mpg123_ip.output->buffer_playing()))
		return -1;
	return mpg123_ip.output->output_time();
}

static void aboutbox(void)
{
	static GtkWidget *aboutbox;

	if (aboutbox != NULL)
		return;
	
	aboutbox = xmms_show_message(
		_("About MPEG Layer 1/2/3 plugin"),
		_("mpg123 decoding engine by Michael Hipp <mh@mpg123.de>\n"
		  "Plugin by The XMMS team"),
		_("OK"), FALSE, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &aboutbox);
}

InputPlugin mpg123_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	init,
	aboutbox,
	mpg123_configure,
	is_our_file,
	NULL,
	play_file,
	stop,
	do_pause,
	seek,
	mpg123_set_eq,
	get_time,
	NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	get_song_info,
	mpg123_file_info_box,	/* file_info_box */
	NULL
};

InputPlugin *get_iplugin_info(void)
{
	mpg123_ip.description =
		g_strdup_printf(_("MPEG Layer 1/2/3 Player %s"), VERSION);
	return &mpg123_ip;
}
