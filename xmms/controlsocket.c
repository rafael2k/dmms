/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
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
#include "xmms.h"
#include "libxmms/xmmsctrl.h"

/*
 * This is just a *quick* implementation, it will change in the future
 * to something more sophisticated
 */

extern HSlider *mainwin_balance;
extern TButton *mainwin_repeat, *mainwin_shuffle;

static gint session_id = 0;
static gint ctrl_fd = 0;
static gchar *socket_name;

static void *ctrlsocket_func(void *);
static pthread_t ctrlsocket_thread;
static gboolean going = TRUE, started = FALSE;

typedef struct
{
	ClientPktHeader hdr;
	gpointer data;
	gint fd;
}
PacketNode;

static GList *packet_list;
static pthread_mutex_t packet_list_mutex = PTHREAD_MUTEX_INITIALIZER, start_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

gboolean setup_ctrlsocket(void)
{
	struct sockaddr_un saddr;
	gboolean retval = FALSE;
	gint i;

	if ((ctrl_fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
	{
		for (i = 0;; i++)
		{
			saddr.sun_family = AF_UNIX;
			g_snprintf(saddr.sun_path, 108, "%s/xmms_%s.%d",
				   g_get_tmp_dir(), g_get_user_name(), i);
			if (!xmms_remote_is_running(i))
			{
				if ((unlink(saddr.sun_path) == -1) && errno != ENOENT)
				{
					g_log(NULL, G_LOG_LEVEL_CRITICAL,
					      "setup_ctrlsocket(): Failed to unlink %s (Error: %s)",
					      saddr.sun_path, strerror(errno));
				}
			}
			else
			{
				if (cfg.allow_multiple_instances)
					continue;
				break;
			}
			if (bind(ctrl_fd, (struct sockaddr *) &saddr, sizeof (saddr)) != -1)
			{
				session_id = i;
				listen(ctrl_fd, 100);
				going = TRUE;

				pthread_create(&ctrlsocket_thread, NULL, ctrlsocket_func, NULL);
				socket_name = g_strdup(saddr.sun_path);
				retval = TRUE;
				break;
			}
			else
			{
				g_log(NULL, G_LOG_LEVEL_CRITICAL,
				      "setup_ctrlsocket(): Failed to assign %s to a socket (Error: %s)",
				      saddr.sun_path, strerror(errno));
				break;
			}
		}
	}
	else
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "setup_ctrlsocket(): Failed to open socket: %s", strerror(errno));

	if (!retval)
	{
		if (ctrl_fd != -1)
			close(ctrl_fd);
		ctrl_fd = 0;
	}
	return retval;
}

gint ctrlsocket_get_session_id(void)
{
	return session_id;
}

void cleanup_ctrlsocket(void)
{
	if (ctrl_fd)
	{
		pthread_mutex_lock(&start_mutex);
		going = FALSE;
		pthread_cond_signal(&start_cond);
		pthread_mutex_unlock(&start_mutex);
		pthread_join(ctrlsocket_thread, NULL);
		close(ctrl_fd);
		unlink(socket_name);
		g_free(socket_name);
		ctrl_fd = 0;
	}
}

void start_ctrlsocket(void)
{
	pthread_mutex_lock(&start_mutex);
	started = TRUE;
	pthread_cond_signal(&start_cond);
	pthread_mutex_unlock(&start_mutex);
}

int write_all(int fd, const void *buf, size_t count)
{
	size_t left = count;
	/* FIXME: This can take forever */
	do {
		int written = write(fd, buf, left);
		if (written < 0)
		{
			g_warning("ctrl_write_packet(): "
				  "Failed to send data: %s",
				  strerror(errno));
			return -1;
		}
		left -= written;
		buf = (char *)buf + written;
	} while (left > 0);
	return count;
}

void ctrl_write_packet(gint fd, gpointer data, gint length)
{
	ServerPktHeader pkthdr;

	/* Not strictly necessary, but zeroes padding */
	memset(&pkthdr, 0, sizeof (pkthdr));
	pkthdr.version = XMMS_PROTOCOL_VERSION;
	pkthdr.data_length = length;
	if (write_all(fd, &pkthdr, sizeof (ServerPktHeader)) < 0)
		return;
	if (data && length > 0)
		write_all(fd, data, length);
}

static void ctrl_write_gint(gint fd, gint val)
{
	ctrl_write_packet(fd, &val, sizeof (gint));
}

static void ctrl_write_gfloat(gint fd, gfloat val)
{
	ctrl_write_packet(fd, &val, sizeof (gfloat));
}

static void ctrl_write_gboolean(gint fd, gboolean bool)
{
	ctrl_write_packet(fd, &bool, sizeof (gboolean));
}

static void ctrl_write_string(gint fd, gchar * string)
{
	ctrl_write_packet(fd, string, string ? strlen(string) + 1 : 0);
}

static void ctrl_ack_packet(PacketNode * pkt)
{
	ctrl_write_packet(pkt->fd, NULL, 0);
	close(pkt->fd);
	if (pkt->data)
		g_free(pkt->data);
	g_free(pkt);
}

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

void *ctrlsocket_func(void *arg)
{
	fd_set set;
	struct timeval tv;
	struct sockaddr_un saddr;
	gint fd, b, i;
	guint32 v[2], info[3];
	PacketNode *pkt;
	gint len;
	gfloat fval[11];

	pthread_mutex_lock(&start_mutex);
	while (!started && going)
		pthread_cond_wait(&start_cond, &start_mutex);
	pthread_mutex_unlock(&start_mutex);

	while (going)
	{
		FD_ZERO(&set);
		FD_SET(ctrl_fd, &set);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		len = sizeof (saddr);
		if ((select(ctrl_fd + 1, &set, NULL, NULL, &tv) <= 0) ||
		    ((fd = accept(ctrl_fd, (struct sockaddr *) &saddr, &len)) == -1))
			continue;

		pkt = g_malloc0(sizeof (PacketNode));
		read_all(fd, &pkt->hdr, sizeof (ClientPktHeader));
		if (pkt->hdr.data_length)
		{
			pkt->data = g_malloc0(pkt->hdr.data_length);
			if (read_all(fd, pkt->data, pkt->hdr.data_length) < 0)
			{
				g_free(pkt->data);
				g_free(pkt);
				g_warning("ctrlsocket_func(): Incomplete data "
					  "packet dropped");
				continue;
			}
		}
		pkt->fd = fd;
		switch (pkt->hdr.command)
		{
			case CMD_GET_VERSION:
				ctrl_write_gint(pkt->fd, 0x09a3);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_PLAYING:
				ctrl_write_gboolean(pkt->fd, get_input_playing());
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_PAUSED:
				ctrl_write_gboolean(pkt->fd, get_input_paused());
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_POS:
				ctrl_write_gint(pkt->fd, get_playlist_position());
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_LENGTH:
				ctrl_write_gint(pkt->fd, get_playlist_length());
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_OUTPUT_TIME:
			{
				int time = input_get_time();
				if (time > 0)
					ctrl_write_gint(pkt->fd, time);
				else
					ctrl_write_gint(pkt->fd, 0);
				ctrl_ack_packet(pkt);
				break;
			}
			case CMD_GET_VOLUME:
				input_get_volume(&v[0], &v[1]);
				ctrl_write_packet(pkt->fd, v, sizeof (v));
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_BALANCE:
				input_get_volume(&v[0], &v[1]);
				if (v[0] > v[1])
					b = -100 + ((v[1] * 100) / v[0]);
				else if (v[1] > v[0])
					b = 100 - ((v[0] * 100) / v[1]);
				else
					b = 0;
				ctrl_write_gint(pkt->fd, b);
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_SKIN:
				ctrl_write_string(pkt->fd, skin->path);
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_FILE:
				if (pkt->data)
				{
					gchar *filename;
					filename = playlist_get_filename(*((guint32 *) pkt->data));
					ctrl_write_string(pkt->fd, filename);
					g_free(filename);
				}
				else
					ctrl_write_string(pkt->fd, NULL);
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_TITLE:
				if (pkt->data)
				{
					gchar *title;
					title = playlist_get_songtitle(*((guint32 *) pkt->data));
					ctrl_write_string(pkt->fd, title);
					g_free(title);
				}
				else
					ctrl_write_string(pkt->fd, NULL);
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_TIME:
				if (pkt->data)
					ctrl_write_gint(pkt->fd, playlist_get_songtime(*((guint32 *) pkt->data)));
				else
					ctrl_write_gint(pkt->fd, -1);
				
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_INFO:
			{
				int rate, freq, chn;
				mainwin_get_song_info(&rate, &freq, &chn);
				info[0] = rate;
				info[1] = freq;
				info[2] = chn;
				ctrl_write_packet(pkt->fd, info, 3 * sizeof (gint));
				ctrl_ack_packet(pkt);
				break;
			}
			case CMD_GET_EQ_DATA:
			case CMD_SET_EQ_DATA:
				/* obsolete */
				ctrl_ack_packet(pkt);
				break;
			case CMD_PING:
				ctrl_ack_packet(pkt);
				break;
			case CMD_PLAYLIST_ADD:
				if (pkt->data)
				{
					guint32* dataptr = pkt->data;
					while ((len = *dataptr) > 0)
					{
						gchar *filename;
						
						dataptr++;
						filename = g_malloc0(len);
						memcpy(filename, dataptr, len);
						playlist_add_url_string(filename);
						g_free(filename);
						dataptr += (len + 3) / 4;
					}
				}
				ctrl_ack_packet(pkt);
				break;
			case CMD_PLAYLIST_ADD_URL_STRING:
				playlist_add_url_string(pkt->data);
				ctrl_ack_packet(pkt);
				break;
			case CMD_PLAYLIST_INS_URL_STRING:
				if (pkt->data)
				{
					int pos = *(int*) pkt->data;
					char *ptr = pkt->data;
					ptr += sizeof(int);
					playlist_ins_url_string(ptr, pos);
				}
				ctrl_ack_packet(pkt);
				break;
			case CMD_PLAYLIST_DELETE:
				GDK_THREADS_ENTER();
				playlist_delete_index(*((guint32 *)pkt->data));
				GDK_THREADS_LEAVE();
				ctrl_ack_packet(pkt);
				break;
			case CMD_PLAYLIST_CLEAR:
				GDK_THREADS_ENTER();
				playlist_clear();
				mainwin_clear_song_info();
				mainwin_set_info_text();
				GDK_THREADS_LEAVE();
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_MAIN_WIN:
				ctrl_write_gboolean(pkt->fd, cfg.player_visible);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_PL_WIN:
				ctrl_write_gboolean(pkt->fd, cfg.playlist_visible);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_EQ_WIN:
				ctrl_write_gboolean(pkt->fd, cfg.equalizer_visible);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_REPEAT:
				ctrl_write_gboolean(pkt->fd, cfg.repeat);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_SHUFFLE:
				ctrl_write_gboolean(pkt->fd, cfg.shuffle);
				ctrl_ack_packet(pkt);
				break;
			case CMD_IS_ADVANCE:
				ctrl_write_gboolean(pkt->fd, !cfg.no_playlist_advance);
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_EQ:
				fval[0] = equalizerwin_get_preamp();
				for(i = 0; i < 10; i++)
					fval[i + 1] = equalizerwin_get_band(i);
				ctrl_write_packet(pkt->fd, fval, 11 * sizeof(gfloat));
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_EQ_PREAMP:
				ctrl_write_gfloat(pkt->fd, equalizerwin_get_preamp());
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_EQ_BAND:
				i = *((guint32 *) pkt->data);
				ctrl_write_gfloat(pkt->fd, equalizerwin_get_band(i));
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYQUEUE_LENGTH:
				ctrl_write_gint(pkt->fd, get_playlist_queue_length());
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYQUEUE_POS_FROM_PLAYLIST_POS:
				i = *((guint32 *) pkt->data);
				ctrl_write_gint(pkt->fd, playlist_get_playqueue_position_from_playlist_position(i));
				ctrl_ack_packet(pkt);
				break;
			case CMD_GET_PLAYLIST_POS_FROM_PLAYQUEUE_POS:
				i = *((guint32 *) pkt->data);
				ctrl_write_gint(pkt->fd, playlist_get_playlist_position_from_playqueue_position(i));
				ctrl_ack_packet(pkt);
				break;
			default:
				pthread_mutex_lock(&packet_list_mutex);
				packet_list = g_list_append(packet_list, pkt);
				ctrl_write_packet(pkt->fd, NULL, 0);
				close(pkt->fd);
				pthread_mutex_unlock(&packet_list_mutex);
				break;
		}
	}
	pthread_exit(NULL);
}

void check_ctrlsocket(void)
{
	GList *pkt_list, *next;
	PacketNode *pkt;
	gpointer data;
	guint32 v[2], i, num;
	gboolean tbool;
	gfloat *fval, f;

	pthread_mutex_lock(&packet_list_mutex);
	for (pkt_list = packet_list; pkt_list; pkt_list = next)
	{
		pkt = pkt_list->data;
		data = pkt->data;

		switch (pkt->hdr.command)
		{
			case CMD_PLAY:
				if (get_input_paused())
					input_pause();
				else if (get_playlist_length())
					playlist_play();
				else
					mainwin_eject_pushed();
				break;
			case CMD_PAUSE:
				input_pause();
				break;
			case CMD_STOP:
				input_stop();
				mainwin_clear_song_info();
				break;
			case CMD_PLAY_PAUSE:
				if (get_input_playing())
					input_pause();
				else
					playlist_play();
				break;
			case CMD_SET_PLAYLIST_POS:
				num = *((guint32 *) data);
				if (num < get_playlist_length())
					playlist_set_position(num);
				break;
			case CMD_JUMP_TO_TIME:
				num = *((guint32 *) data);
				if (playlist_get_current_length() > 0 &&
				    num < playlist_get_current_length())
					input_seek(num / 1000);
				break;
			case CMD_SET_VOLUME:
				v[0] = ((guint32 *) data)[0];
				v[1] = ((guint32 *) data)[1];
				for (i = 0; i < 2; i++)
				{
					if (v[i] > 100)
						v[i] = 100;
				}
				input_set_volume(v[0], v[1]);
				break;
			case CMD_SET_SKIN:
				load_skin(data);
				break;
			case CMD_PL_WIN_TOGGLE:
				tbool = *((gboolean *) data);
				playlistwin_show(!!tbool);
				break;
			case CMD_EQ_WIN_TOGGLE:
				tbool = *((gboolean *) data);
				equalizerwin_show(!!tbool);
				break;
			case CMD_SHOW_PREFS_BOX:
				show_prefs_window();
				break;
			case CMD_TOGGLE_AOT:
				tbool = *((gboolean *) data);
				mainwin_set_always_on_top(tbool);
				break;
			case CMD_SHOW_ABOUT_BOX:
				break;
			case CMD_EJECT:
				mainwin_eject_pushed();
				break;
			case CMD_PLAYLIST_PREV:
				playlist_prev();
				break;
			case CMD_PLAYLIST_NEXT:
				playlist_next();
				break;
			case CMD_TOGGLE_REPEAT:
				mainwin_repeat_pushed(!cfg.repeat);
				tbutton_set_toggled(mainwin_repeat, GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mainwin_options_menu, "/Repeat"))->active);
				break;
			case CMD_TOGGLE_SHUFFLE:
				mainwin_shuffle_pushed(!cfg.shuffle);
				tbutton_set_toggled(mainwin_shuffle, GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mainwin_options_menu, "/Shuffle"))->active);
				break;
			case CMD_TOGGLE_ADVANCE:
				mainwin_advance_pushed(!cfg.no_playlist_advance);
				/* there is no real button, so no need to call tbutton_set_toggled() */
				break;
			case CMD_MAIN_WIN_TOGGLE:
				tbool = *((gboolean *) data);
				mainwin_show(!!tbool);
				break;
			case CMD_SET_EQ:
				if(pkt->hdr.data_length >= 11 * sizeof(gfloat)) {
					fval = (gfloat *) data;
					equalizerwin_set_preamp(fval[0]);
					for(i = 0; i < 10; i++)
						equalizerwin_set_band(i, fval[i + 1]);
				}
				break;
			case CMD_SET_EQ_PREAMP:
				f = *((gfloat *) data);
				equalizerwin_set_preamp(f);
				break;
			case CMD_SET_EQ_BAND:
				if(pkt->hdr.data_length >= sizeof(gint) + sizeof(gfloat))
				{
					i = *((gint *) data);
					f = *((gfloat *) ((gchar *)data + sizeof(gint)));
					equalizerwin_set_band(i, f);
				}
				break;
			case CMD_QUIT:
				/*
				 * We unlock the packet_list_mutex to
				 * avoid that cleanup_ctrlsocket() can
				 * deadlock, mainwin_quit_cb() will
				 * never return anyway, so this will
				 * work ok.
				 */
				pthread_mutex_unlock(&packet_list_mutex);
				mainwin_quit_cb();
				break;
			case CMD_PLAYQUEUE_ADD:
				num = *((guint32 *) data);
				if (num < get_playlist_length())
					playlist_queue_position(num);
				break;
			case CMD_PLAYQUEUE_REMOVE:
				num = *((guint32 *) data);
				if (num < get_playlist_length())
					playlist_queue_remove(num);
				break;
			case CMD_PLAYQUEUE_CLEAR:
				playlist_clear_queue();
				break;
			default:
				g_log(NULL, G_LOG_LEVEL_MESSAGE, "Unknown socket command received");
				break;
		}
		next = g_list_next(pkt_list);
		packet_list = g_list_remove_link(packet_list, pkt_list);
		g_list_free_1(pkt_list);
		if (pkt->data)
			g_free(pkt->data);
		g_free(pkt);
	}
	pthread_mutex_unlock(&packet_list_mutex);
}
