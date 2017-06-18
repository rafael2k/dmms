/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
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
#ifndef EQUALIZER_H
#define EQUALIZER_H

#define EQUALIZER_DOUBLESIZE (cfg.doublesize && cfg.eq_doublesize_linked)
#define EQUALIZER_HEIGHT ((cfg.equalizer_shaded ? 14 : 116) * (EQUALIZER_DOUBLESIZE + 1))
#define EQUALIZER_WIDTH ( 275 * (EQUALIZER_DOUBLESIZE + 1))

void equalizerwin_set_doublesize(gboolean ds);
void equalizerwin_set_shade_menu_cb(gboolean shaded);
void equalizerwin_set_shade(gboolean shaded);
void equalizerwin_shade_toggle(void);
void equalizerwin_raise(void);
void equalizerwin_move(gint x, gint y);
void draw_equalizer_window(gboolean force);
void equalizerwin_create(void);
void equalizerwin_recreate(void);
void equalizerwin_show(gboolean show);
void equalizerwin_real_show(void);
void equalizerwin_real_hide(void);
void equalizerwin_load_auto_preset(gchar * filename);
void equalizerwin_set_back_pixmap(void);
void equalizerwin_set_volume_slider(gint percent);
void equalizerwin_set_balance_slider(gint percent);
void equalizerwin_eq_changed(void);
void equalizerwin_set_preamp(gfloat preamp);
void equalizerwin_set_band(gint band, gfloat value);
gfloat equalizerwin_get_preamp(void);
gfloat equalizerwin_get_band(gint band);
void equalizerwin_set_shape_mask(void);

#endif
