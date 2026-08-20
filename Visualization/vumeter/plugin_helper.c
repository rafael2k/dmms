#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "plugin_main.h"

char *trim(char *src,char *dst)
{
  int slen=strlen(src),l1,l2,l3;

  // Trim from beginning
  for(l1=0; l1<slen; l1++)
  if(!isspace(src[l1]))	break;

  // Trim from end
  for(l2=(slen-1); l2>=0; l2--)
  if(!isspace(src[l2])) break;

  // Copy string
  for(l3=l1; l3<=l2; l3++) dst[l3-l1]=src[l3];
  dst[l3-l1]=0;

  return(dst);
}

void reset_win_structure(vumeter_window *pwin)
{
	pwin->width	=0;
	pwin->height	=0;
	pwin->xpos	=0;
	pwin->ypos	=0;
	pwin->skin_num	=-1;
	pwin->slot_num	=-1;

	pwin->win	=NULL;
	pwin->pixmap	=NULL;
	pwin->pen	=NULL;
	pwin->bg_buf	=NULL;
	pwin->frame_buf	=NULL;
}

/* Minimal base64 codec (GLib 1.2 has no g_base64_encode/decode) */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

gchar *vumeter_base64_encode(const guchar *data, gint len)
{
	gchar	*out;
	gint	i,o=0;
	guint32	val;

	out = g_malloc( ((len+2)/3)*4 + 1 );

	for(i=0; i<len; i+=3)
	{
		val = ((guint32)data[i]) << 16;
		if(i+1<len) val |= ((guint32)data[i+1]) << 8;
		if(i+2<len) val |= (guint32)data[i+2];

		out[o++] = b64_table[(val>>18)&0x3F];
		out[o++] = b64_table[(val>>12)&0x3F];
		out[o++] = (i+1<len) ? b64_table[(val>>6)&0x3F] : '=';
		out[o++] = (i+2<len) ? b64_table[val&0x3F]      : '=';
	}
	out[o]=0;

	return(out);
}

static gint b64_decode_char(gchar c)
{
	if(c>='A' && c<='Z') return(c-'A');
	if(c>='a' && c<='z') return(c-'a'+26);
	if(c>='0' && c<='9') return(c-'0'+52);
	if(c=='+') return(62);
	if(c=='/') return(63);
	return(-1);
}

guchar *vumeter_base64_decode(const gchar *data, gint *out_len)
{
	gint	len=strlen(data),
		i,o=0;
	guchar	*out;
	gint	vals[4];
	gint	nvals;

	out = g_malloc( (len/4+1)*3 + 1 );

	i=0;
	while(i<len)
	{
		nvals=0;
		while(nvals<4 && i<len)
		{
			gint v = b64_decode_char(data[i]);
			i++;
			if(v>=0)
				vals[nvals++]=v;
			else if(data[i-1]=='=')
				break;
		}

		if(nvals>=2)
			out[o++] = (vals[0]<<2) | (vals[1]>>4);
		if(nvals>=3)
			out[o++] = ((vals[1]&0xF)<<4) | (vals[2]>>2);
		if(nvals>=4)
			out[o++] = ((vals[2]&0x3)<<6) | vals[3];

		if(nvals<4)
			break;
	}
	out[o]=0;

	if(out_len!=NULL)
		*out_len=o;

	return(out);
}
