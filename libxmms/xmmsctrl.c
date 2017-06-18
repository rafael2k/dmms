/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2004  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "xmmsctrl.h"
#include "../xmms/controlsocket.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


static int read_all(int fd, void *buf, size_t count)
{
	size_t left = count;
	int r;
	do {
		r = read(fd, buf, left);
		if (r < 0)
			return -1;
		left -= r;
		buf = (char *)buf + r;
	} while (left > 0 && r > 0);
	return count - left;
}

static gpointer remote_read_packet(int fd, ServerPktHeader * pkt_hdr)
{
	gpointer data = NULL;

	if (read_all(fd, pkt_hdr, sizeof (ServerPktHeader)) == sizeof (ServerPktHeader))
	{
		if (pkt_hdr->data_length)
		{
			data = g_malloc0(pkt_hdr->data_length);
			if (read_all(fd, data, pkt_hdr->data_length) != pkt_hdr->data_length)
			{
				g_free(data);
				data = NULL;
			}
		}
	}
	return data;
}

static void remote_read_ack(int fd)
{
	gpointer data;
	ServerPktHeader pkt_hdr;

	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
		g_free(data);

}

static int write_all(int fd, const void *buf, size_t count)
{
	size_t left = count;
	/* FIXME: This can take forever */
	do {
		int written = write(fd, buf, left);
		if (written < 0)
		{
			g_warning("remote_send_packet(): "
				  "Failed to send data to xmms: %s",
				  strerror(errno));
			return -1;
		}
		left -= written;
		buf = (char *)buf + written;
	} while (left > 0);
	return count;
}

static void remote_send_packet(int fd, guint32 command, gpointer data, guint32 data_length)
{
	ClientPktHeader pkt_hdr;

	pkt_hdr.version = XMMS_PROTOCOL_VERSION;
	pkt_hdr.command = command;
	pkt_hdr.data_length = data_length;
	if (write_all(fd, &pkt_hdr, sizeof (ClientPktHeader)) < 0)
		return;
	if (data_length && data)
		write_all(fd, data, data_length);
}

static void remote_send_guint32(int session, guint32 cmd, guint32 val)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, cmd, &val, sizeof (guint32));
	remote_read_ack(fd);
	close(fd);
}

static void remote_send_boolean(int session, guint32 cmd, gboolean val)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, cmd, &val, sizeof (gboolean));
	remote_read_ack(fd);
	close(fd);
}

static void remote_send_gfloat(int session, guint32 cmd, float value)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, cmd, &value, sizeof (float));
	remote_read_ack(fd);
	close(fd);
}

static void remote_send_string(int session, guint32 cmd, char * string)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, cmd, string, string ? strlen(string) + 1 : 0);
	remote_read_ack(fd);
	close(fd);
}

static gboolean remote_cmd(int session, guint32 cmd)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return FALSE;
	remote_send_packet(fd, cmd, NULL, 0);
	remote_read_ack(fd);
	close(fd);

	return TRUE;
}

static gboolean remote_get_gboolean(int session, int cmd)
{
	ServerPktHeader pkt_hdr;
	gboolean ret = FALSE;
	gpointer data;
	gint fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return ret;
	remote_send_packet(fd, cmd, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *((gboolean *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);

	return ret;
}

static guint32 remote_get_gint(int session, int cmd)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	int fd, ret = 0;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return ret;
	remote_send_packet(fd, cmd, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *((int *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

static guint32 remote_get_gint_pos(int session, int cmd, guint32 pos)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	int fd, ret = 0;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return ret;
	remote_send_packet(fd, cmd, &pos, sizeof (guint32));
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *((int *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

static float remote_get_gfloat(int session, int cmd)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	gint fd;
	gfloat ret = 0.0;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return ret;
	remote_send_packet(fd, cmd, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *((float *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

char *remote_get_string(int session, int cmd)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	gint fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return NULL;
	remote_send_packet(fd, cmd, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	remote_read_ack(fd);
	close(fd);
	return data;
}

char *remote_get_string_pos(int session, int cmd, guint32 pos)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return NULL;
	remote_send_packet(fd, cmd, &pos, sizeof (guint32));
	data = remote_read_packet(fd, &pkt_hdr);
	remote_read_ack(fd);
	close(fd);
	return data;
}

int xmms_connect_to_session(int session)
{
	int fd;
	uid_t stored_uid, euid;
	struct sockaddr_un saddr;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
	{
		saddr.sun_family = AF_UNIX;
		stored_uid = getuid();
		euid = geteuid();
		setuid(euid);
		g_snprintf(saddr.sun_path, 108, "%s/xmms_%s.%d", g_get_tmp_dir(), g_get_user_name(), session);
		setreuid(stored_uid, euid);
		if (connect(fd, (struct sockaddr *) &saddr, sizeof (saddr)) != -1)
			return fd;
	}
	close(fd);
	return -1;
}

void xmms_remote_playlist(int session, char ** list, int num, gboolean enqueue)
{
	int fd, i;
	char *data, *ptr;
	int data_length;
	guint32 len;

	g_return_if_fail(list != NULL);
	g_return_if_fail(num > 0);
	
	if (!enqueue)
		xmms_remote_playlist_clear(session);

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;

	for (i = 0, data_length = 0; i < num; i++)
		data_length += (((strlen(list[i]) + 1) + 3) / 4) * 4 + 4;
	if (data_length)
	{
		data_length += 4;
		data = g_malloc(data_length);
		for (i = 0, ptr = data; i < num; i++)
		{
			len = strlen(list[i]) + 1;
			*((guint32 *) ptr) = len;
			ptr += 4;
			memcpy(ptr, list[i], len);
			ptr += ((len + 3) / 4) * 4;
		}
		*((guint32 *) ptr) = 0;
		remote_send_packet(fd, CMD_PLAYLIST_ADD, data, data_length);
		remote_read_ack(fd);
		close(fd);
		g_free(data);
	}

	if (!enqueue)
		xmms_remote_play(session);
}

int xmms_remote_get_version(int session)
{
	return remote_get_gint(session, CMD_GET_VERSION);
}

void xmms_remote_play_files(int session, GList * list)
{
	g_return_if_fail(list != NULL);

	xmms_remote_playlist_clear(session);
	xmms_remote_add_files(session, list);
	xmms_remote_play(session);
}

void xmms_remote_playlist_add(int session, GList * list)
{
	char **str_list;
	GList *node;
	int i, num;

	g_return_if_fail(list != NULL);
	
	num = g_list_length(list);
	str_list = g_malloc0(num * sizeof (char *));
	for (i = 0, node = list; i < num && node; i++, node = g_list_next(node))
		str_list[i] = node->data;

	xmms_remote_playlist(session, str_list, num, TRUE);
	g_free(str_list);
}

void xmms_remote_playlist_delete(int session, int pos)
{
	remote_send_guint32(session,CMD_PLAYLIST_DELETE, pos);
}

void xmms_remote_play(int session)
{
	remote_cmd(session, CMD_PLAY);
}

void xmms_remote_pause(int session)
{
	remote_cmd(session, CMD_PAUSE);
}

void xmms_remote_stop(int session)
{
	remote_cmd(session, CMD_STOP);
}

void xmms_remote_play_pause(int session)
{
	remote_cmd(session, CMD_PLAY_PAUSE);
}

gboolean xmms_remote_is_playing(int session)
{
	return remote_get_gboolean(session, CMD_IS_PLAYING);
}

gboolean xmms_remote_is_paused(int session)
{
	return remote_get_gboolean(session, CMD_IS_PAUSED);
}

gint xmms_remote_get_playlist_pos(int session)
{
	return remote_get_gint(session, CMD_GET_PLAYLIST_POS);
}

void xmms_remote_set_playlist_pos(int session, int pos)
{
	remote_send_guint32(session, CMD_SET_PLAYLIST_POS, pos);
}

gint xmms_remote_get_playlist_length(int session)
{
	return remote_get_gint(session, CMD_GET_PLAYLIST_LENGTH);
}

void xmms_remote_playlist_clear(int session)
{
	remote_cmd(session, CMD_PLAYLIST_CLEAR);
}

void xmms_remote_playqueue_add(gint session, gint pos)
{
	remote_send_guint32(session, CMD_PLAYQUEUE_ADD, pos);
}

void xmms_remote_playqueue_remove(gint session, gint pos)
{
	remote_send_guint32(session, CMD_PLAYQUEUE_REMOVE, pos);
}

void xmms_remote_playqueue_clear(gint session)
{
	remote_cmd(session, CMD_PLAYQUEUE_CLEAR);
}
		
gint xmms_remote_get_playqueue_length(gint session)
{
	return remote_get_gint(session, CMD_GET_PLAYQUEUE_LENGTH);
}

gint xmms_remote_get_playqueue_pos_from_playlist_pos(gint session, gint pos)
{
	return remote_get_gint_pos(session, CMD_GET_PLAYQUEUE_POS_FROM_PLAYLIST_POS, pos);
}

gint xmms_remote_get_playlist_pos_from_playqueue_pos(gint session, gint pos)
{
	return remote_get_gint_pos(session, CMD_GET_PLAYLIST_POS_FROM_PLAYQUEUE_POS, pos);
}

int xmms_remote_get_output_time(int session)
{
	return remote_get_gint(session, CMD_GET_OUTPUT_TIME);
}

void xmms_remote_jump_to_time(int session, int pos)
{
	remote_send_guint32(session, CMD_JUMP_TO_TIME, pos);
}

void xmms_remote_get_volume(int session, int * vl, int * vr)
{
	ServerPktHeader pkt_hdr;
	gint fd;
	gpointer data;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;

	remote_send_packet(fd, CMD_GET_VOLUME, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		*vl = ((guint32 *) data)[0];
		*vr = ((guint32 *) data)[1];
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
}

int xmms_remote_get_main_volume(int session)
{
	gint vl, vr;

	xmms_remote_get_volume(session, &vl, &vr);

	return MAX(vl, vr);
}

int xmms_remote_get_balance(int session)
{
	return remote_get_gint(session, CMD_GET_BALANCE);
}

void xmms_remote_set_volume(int session, int vl, int vr)
{
	gint fd;
	guint32 v[2];

	vl = CLAMP(vl, 0, 100);
	vr = CLAMP(vr, 0, 100);

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	v[0] = vl;
	v[1] = vr;
	remote_send_packet(fd, CMD_SET_VOLUME, v, 2 * sizeof (guint32));
	remote_read_ack(fd);
	close(fd);
}

void xmms_remote_set_main_volume(int session, int v)
{
	int b, vl, vr;

	b = xmms_remote_get_balance(session);

	v = CLAMP(v, 0, 100);
	
	if (b < 0)
	{
		vl = v;
		vr = rint((v * (100 + b)) / 100.0);
	}
	else if (b > 0)
	{
		vl = rint((v * (100 - b)) / 100.0);
		vr = v;
	}
	else
		vl = vr = v;
	xmms_remote_set_volume(session, vl, vr);
}

void xmms_remote_set_balance(int session, int b)
{
	int v, vl, vr;

	b = CLAMP(b, -100, 100);

	v = xmms_remote_get_main_volume(session);

	if (b < 0)
	{
		vl = v;
		vr = (v * (100 + b)) / 100;
	}
	else if (b > 0)
	{
		vl = (v * (100 - b)) / 100;
		vr = v;
	}
	else
		vl = vr = v;
	xmms_remote_set_volume(session, vl, vr);
}

char *xmms_remote_get_skin(int session)
{
	return remote_get_string(session, CMD_GET_SKIN);
}

void xmms_remote_set_skin(int session, char * skinfile)
{
	remote_send_string(session, CMD_SET_SKIN, skinfile);
}

char *xmms_remote_get_playlist_file(int session, int pos)
{
	return remote_get_string_pos(session, CMD_GET_PLAYLIST_FILE, pos);
}

char *xmms_remote_get_playlist_title(int session, int pos)
{
	return remote_get_string_pos(session, CMD_GET_PLAYLIST_TITLE, pos);
}

int xmms_remote_get_playlist_time(int session, int pos)
{
	ServerPktHeader pkt_hdr;
	gpointer data;
	int fd, ret = 0;
	guint32 p = pos;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return ret;
	remote_send_packet(fd, CMD_GET_PLAYLIST_TIME, &p, sizeof (guint32));
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *((gint *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

void xmms_remote_get_info(int session, int * rate, int * freq, int * nch)
{
	ServerPktHeader pkt_hdr;
	gint fd;
	gpointer data;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, CMD_GET_INFO, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		*rate = ((guint32 *) data)[0];
		*freq = ((guint32 *) data)[1];
		*nch = ((guint32 *) data)[2];
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
}

void xmms_remote_get_eq_data(int session)
{
	/* Obsolete */
}

void xmms_remote_set_eq_data(int session)
{
	/* Obsolete */
}

void xmms_remote_pl_win_toggle(int session, gboolean show)
{
	remote_send_boolean(session, CMD_PL_WIN_TOGGLE, show);
}

void xmms_remote_eq_win_toggle(int session, gboolean show)
{
	remote_send_boolean(session, CMD_EQ_WIN_TOGGLE, show);
}

void xmms_remote_main_win_toggle(int session, gboolean show)
{
	remote_send_boolean(session, CMD_MAIN_WIN_TOGGLE, show);
}

gboolean xmms_remote_is_main_win(int session)
{
	return remote_get_gboolean(session, CMD_IS_MAIN_WIN);
}

gboolean xmms_remote_is_pl_win(int session)
{
	return remote_get_gboolean(session, CMD_IS_PL_WIN);
}

gboolean xmms_remote_is_eq_win(int session)
{
	return remote_get_gboolean(session, CMD_IS_EQ_WIN);
}

void xmms_remote_show_prefs_box(int session)
{
	remote_cmd(session, CMD_SHOW_PREFS_BOX);
}

void xmms_remote_toggle_aot(int session, gboolean ontop)
{
	remote_send_boolean(session, CMD_TOGGLE_AOT, ontop);
}

void xmms_remote_show_about_box(int session)
{
	remote_cmd(session, CMD_SHOW_ABOUT_BOX);
}

void xmms_remote_eject(int session)
{
	remote_cmd(session, CMD_EJECT);
}

void xmms_remote_playlist_prev(int session)
{
	remote_cmd(session, CMD_PLAYLIST_PREV);
}

void xmms_remote_playlist_next(int session)
{
	remote_cmd(session, CMD_PLAYLIST_NEXT);
}

void xmms_remote_playlist_add_url_string(int session, char * string)
{
	g_return_if_fail(string != NULL);
	remote_send_string(session, CMD_PLAYLIST_ADD_URL_STRING, string);
}

void xmms_remote_playlist_ins_url_string(int session, char * string, int pos)
{
	int fd, size;
	char* packet;

	g_return_if_fail(string != NULL);

	size = strlen(string) + 1 + sizeof(int);

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;

	packet = g_malloc0(size);
	*((int*) packet) = pos;
	strcpy(packet + sizeof(int), string);
	remote_send_packet(fd, CMD_PLAYLIST_INS_URL_STRING, packet, size);
	remote_read_ack(fd);
	close(fd);
	g_free(packet);
}

gboolean xmms_remote_is_running(int session)
{
	return remote_cmd(session, CMD_PING);
}

void xmms_remote_toggle_repeat(int session)
{
	remote_cmd(session, CMD_TOGGLE_REPEAT);
}

void xmms_remote_toggle_shuffle(int session)
{
	remote_cmd(session, CMD_TOGGLE_SHUFFLE);
}

void xmms_remote_toggle_advance(int session)
{
	remote_cmd(session, CMD_TOGGLE_ADVANCE);
}

gboolean xmms_remote_is_repeat(gint session)
{
	return remote_get_gboolean(session, CMD_IS_REPEAT);
}

gboolean xmms_remote_is_shuffle(gint session)
{
	return remote_get_gboolean(session, CMD_IS_SHUFFLE);
}

gboolean xmms_remote_is_advance(int session)
{
	return remote_get_gboolean(session, CMD_IS_ADVANCE);
}

void xmms_remote_get_eq(int session, float *preamp, float **bands)
{
	ServerPktHeader pkt_hdr;
	int fd;
	gpointer data;

	if (preamp)
		*preamp = 0.0;

	if (bands)
		*bands = NULL;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, CMD_GET_EQ, NULL, 0);
	data = remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		if (pkt_hdr.data_length >= 11 * sizeof(gfloat))
		{
			if (preamp)
				*preamp = *((float *) data);
			if (bands)
				*bands = g_memdup((float *)data + 1, 10 * sizeof(float));
		}
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
}

float xmms_remote_get_eq_preamp(int session)
{
	return remote_get_gfloat(session, CMD_GET_EQ_PREAMP);
}

float xmms_remote_get_eq_band(int session, int band)
{
	ServerPktHeader pkt_hdr;
	gint fd;
	gpointer data;
	gfloat val = 0.0;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return val;
	remote_send_packet(fd, CMD_GET_EQ_BAND, &band, sizeof(band));
	data = remote_read_packet(fd, &pkt_hdr);
	if (data) {
		val = *((gfloat *) data);
		g_free(data);
	}
	remote_read_ack(fd);
	close(fd);
	return val;
}

void xmms_remote_set_eq(int session, float preamp, float *bands)
{
	int fd, i;
	float data[11];

	g_return_if_fail(bands != NULL);

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	data[0] = preamp;
	for (i = 0; i < 10; i++)
		data[i + 1] = bands[i];
	remote_send_packet(fd, CMD_SET_EQ, data, sizeof(data));
	remote_read_ack(fd);
	close(fd);
}

void xmms_remote_set_eq_preamp(int session, float preamp)
{
	remote_send_gfloat(session, CMD_SET_EQ_PREAMP, preamp);
}

void xmms_remote_set_eq_band(int session, int band, float value)
{
	int fd;
	char data[sizeof(int) + sizeof(float)];

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	*((int *) data) = band;
	*((float *) (data + sizeof(int))) = value;
	remote_send_packet(fd, CMD_SET_EQ_BAND, data, sizeof(data));
	remote_read_ack(fd);
	close(fd);
}

void xmms_remote_quit(int session)
{
	int fd;

	if ((fd = xmms_connect_to_session(session)) == -1)
		return;
	remote_send_packet(fd, CMD_QUIT, NULL, 0);
	remote_read_ack(fd);
	close(fd);
}
