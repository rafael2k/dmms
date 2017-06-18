/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
#ifndef XMMS_H
#define XMMS_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>

#include "bmp.h"
#include "util.h"
#include "skin.h"
#include "plugin.h"
#include "output.h"
#include "input.h"
#include "effect.h"
#include "general.h"
#include "visualization.h"
#include "fullscreen.h"
#include "pluginenum.h"
#include "playlist.h"
#include "controlsocket.h"
#include "dock.h"
#include "widget.h"
#include "sbutton.h"
#include "pbutton.h"
#include "tbutton.h"
#include "textbox.h"
#include "menurow.h"
#include "hslider.h"
#include "monostereo.h"
#include "vis.h"
#include "svis.h"
#include "number.h"
#include "playstatus.h"
#include "playlist_list.h"
#include "playlist_slider.h"
#include "playlist_popup.h"
#include "eq_graph.h"
#include "eq_slider.h"
#include "main.h"
#include "skinwin.h"
#include "prefswin.h"
#include "playlistwin.h"
#include "equalizer.h"
#include "about.h"
#include "hints.h"
#include "i18n.h"
#include "sm.h"
#include "dnd.h"
#include "urldecode.h"

#include "config.h"

#endif
