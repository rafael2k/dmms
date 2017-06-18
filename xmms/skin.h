/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
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
#ifndef SKIN_H
#define SKIN_H

typedef enum
{
	SKIN_MAIN, SKIN_CBUTTONS, SKIN_TITLEBAR, SKIN_SHUFREP, SKIN_TEXT, SKIN_VOLUME,
	SKIN_BALANCE, SKIN_MONOSTEREO, SKIN_PLAYPAUSE, SKIN_NUMBERS, SKIN_POSBAR,
	SKIN_PLEDIT, SKIN_EQMAIN, SKIN_EQ_EX,
} SkinIndex;

typedef enum
{
	SKIN_MASK_MAIN, SKIN_MASK_EQ
} MaskIndex;

typedef enum
{
	SKIN_PLEDIT_NORMAL, SKIN_PLEDIT_CURRENT, SKIN_PLEDIT_NORMALBG,
	SKIN_PLEDIT_SELECTEDBG, SKIN_TEXTBG, SKIN_TEXTFG
} SkinColorIndex;

typedef struct
{
	GdkPixmap *pixmap, *def_pixmap;
	/* The real size of the pixmap */
	gint width, height;
	/* The size of the pixmap from the current skin,
	   which might be smaller */
	gint current_width, current_height;
} SkinPixmap;
	

typedef struct
{
	gchar *path;
	SkinPixmap main;
	SkinPixmap cbuttons;
	SkinPixmap titlebar;
	SkinPixmap shufrep;
	SkinPixmap text;
	SkinPixmap volume;
	SkinPixmap balance;
	SkinPixmap monostereo;
	SkinPixmap playpause;
	SkinPixmap numbers;
	SkinPixmap posbar;
	SkinPixmap pledit;
	SkinPixmap eqmain;
	SkinPixmap eq_ex;
	GdkColor textbg[6], def_textbg[6];
	GdkColor textfg[6], def_textfg[6];
	GdkColor *pledit_normal, def_pledit_normal;
	GdkColor *pledit_current, def_pledit_current;
	GdkColor *pledit_normalbg, def_pledit_normalbg;
	GdkColor *pledit_selectedbg, def_pledit_selectedbg;
	guchar vis_color[24][3];
	GdkBitmap *def_mask, *def_mask_ds;
	GdkBitmap *def_mask_shade, *def_mask_shade_ds;
	GdkBitmap *mask_main, *mask_main_ds;
	GdkBitmap *mask_shade, *mask_shade_ds;
	GdkBitmap *mask_eq, *mask_eq_ds;
	GdkBitmap *mask_eq_shade, *mask_eq_shade_ds;
}
Skin;

extern Skin *skin;

void init_skins(void);
void load_skin(const gchar * path);
void reload_skin(void);
void cleanup_skins(void);
GdkBitmap *skin_get_mask(MaskIndex mi, gboolean doublesize, gboolean shaded);
GdkColor *get_skin_color(SkinColorIndex si);
void get_skin_viscolor(guchar vis_color[24][3]);
int skin_get_id(void);
void skin_draw_pixmap(GdkDrawable *drawable, GdkGC *gc, SkinIndex si,
		      gint xsrc, gint ysrc, gint xdest, gint ydest,
		      gint width, gint height);
void skin_get_eq_spline_colors(guint32 (*colors)[19]);

#endif
