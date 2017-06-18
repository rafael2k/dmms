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
#include "xmms.h"

void playstatus_draw(Widget * w)
{
	PlayStatus *ps = (PlayStatus *) w;
	GdkPixmap *obj;

	obj = ps->ps_widget.parent;

	if (ps->ps_status == STATUS_PLAY)
		skin_draw_pixmap(obj, ps->ps_widget.gc, SKIN_PLAYPAUSE,
				 36, 0, ps->ps_widget.x, ps->ps_widget.y, 3, 9);
	else
		skin_draw_pixmap(obj, ps->ps_widget.gc, SKIN_PLAYPAUSE,
				 27, 0, ps->ps_widget.x, ps->ps_widget.y, 2, 9);
	switch (ps->ps_status)
	{
		case STATUS_STOP:
			skin_draw_pixmap(obj, ps->ps_widget.gc,
					 SKIN_PLAYPAUSE, 18, 0,
					 ps->ps_widget.x + 2, ps->ps_widget.y, 9, 9);
			break;
		case STATUS_PAUSE:
			skin_draw_pixmap(obj, ps->ps_widget.gc,
					 SKIN_PLAYPAUSE, 9, 0,
					 ps->ps_widget.x + 2, ps->ps_widget.y, 9, 9);
			break;
		case STATUS_PLAY:
			skin_draw_pixmap(obj, ps->ps_widget.gc,
					 SKIN_PLAYPAUSE, 1, 0,
					 ps->ps_widget.x + 3, ps->ps_widget.y, 8, 9);
			break;
	}
}

void playstatus_set_status(PlayStatus * ps, PStatus status)
{
	ps->ps_status = status;
	draw_widget(ps);
}

PlayStatus *create_playstatus(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y)
{
	PlayStatus *ps;

	ps = g_malloc0(sizeof (PlayStatus));
	ps->ps_widget.parent = parent;
	ps->ps_widget.gc = gc;
	ps->ps_widget.x = x;
	ps->ps_widget.y = y;
	ps->ps_widget.width = 11;
	ps->ps_widget.height = 9;
	ps->ps_widget.visible = TRUE;
	ps->ps_widget.draw = playstatus_draw;
	add_widget(wlist, ps);
	return ps;
}
