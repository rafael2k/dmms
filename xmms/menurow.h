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
#ifndef MENUROW_H
#define	MENUROW_H

typedef enum
{
	MENUROW_NONE, MENUROW_OPTIONS, MENUROW_ALWAYS, MENUROW_FILEINFOBOX, MENUROW_DOUBLESIZE, MENUROW_VISUALIZATION
}
MenuRowItem;

typedef struct
{
	Widget mr_widget;
	gint mr_nx, mr_ny;
	gint mr_sx, mr_sy;
	MenuRowItem mr_selected;
	gboolean mr_bpushed;
	gboolean mr_always_selected;
	gboolean mr_doublesize_selected;
	void (*mr_change_callback) (MenuRowItem);
	void (*mr_release_callback) (MenuRowItem);
	SkinIndex mr_skin_index;
}
MenuRow;

MenuRow *create_menurow(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint nx, gint ny, gint sx, gint sy, void (*ccb) (MenuRowItem), void (*rcb) (MenuRowItem), SkinIndex si);

#endif
