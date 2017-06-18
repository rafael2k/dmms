/*
 * Copyright (C) 2001,  Espen Skoglund <esk@ira.uka.de>
 * Copyright (C) 2001, 2004, 2006  Haavard Kvaalen <havardk@xmms.org>
 * Copyright (C) 2004,  Matti Hämäläinen <ccr@tnsp.org>
 *                
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *                
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "titlestring.h"
#include "../xmms/i18n.h"

struct padding {
	int side, width, precision;
	char padchar;
};

enum {
	PAD_SIDE_LEFT,
	PAD_SIDE_RIGHT,
};


static int xmms_vputstr(GString *output, char *pstr, struct padding *pad)
{
	int i;
	/* Lenght of the string that is actually printed */
	int plen;
	
	if (!pstr)
		return FALSE;

	/* Calculate printed size */
	plen = strlen(pstr);
	if (pad->precision >= 0 && pad->precision < plen)
		plen = pad->precision;

	/* Do left-padding */
	if (pad->width > 0)
	{
		if (pad->side == PAD_SIDE_LEFT)
		{
			for (i = pad->width - plen; i > 0; i--)
				g_string_append_c(output, pad->padchar);
		}
	}

	/* Insert the string */
	if (pad->precision >= 0)
	{
		for (i = 0; i < plen; i++)
			g_string_append_c(output, *pstr++);
	}
	else
		g_string_append(output, pstr);
	
	/* Do right-padding */
	if (pad->side == PAD_SIDE_RIGHT && pad->width > 0)
		for (i = pad->width - plen; i > 0; i--)
			g_string_append_c(output, ' ');
	return TRUE;
}


static int xmms_vputnum(GString *output, int ival, struct padding *pad)
{
	int i, n, ndigits;
	char *pstr;
	char padchar = pad->padchar;

	/* FIXME: Never print the value '0'? */
	if (ival == 0)
		return FALSE;
	
	/* Create string */
	pstr = g_strdup_printf("%d", ival);
	ndigits = strlen(pstr);

	n = MAX(ndigits, pad->precision);

	/* Do left-padding */
	if (pad->side == PAD_SIDE_LEFT && pad->width > n)
	{
		if (pad->precision >= 0)
			padchar = ' ';
		for (i = pad->width - n; i > 0; i--)
			g_string_append_c(output, padchar);
	}

	/* Do zero-padding (precision) */
	for (i = n - ndigits; i-- > 0;)
		g_string_append_c(output, '0');
	
	/* Add the value */
	g_string_append(output, pstr);
	g_free(pstr);

	/* Do right-padding */
	if (pad->side == PAD_SIDE_RIGHT && pad->width > 0)
		for (i = pad->width - n; i > 0; i--)
			g_string_append_c(output, ' ');
	return TRUE;
}

static int parse_variable(char **fmt, GString *string, TitleInput *input)
{
	struct padding padding;
	char *ptr = *fmt;
	int exp = 0;
	char c;

	padding.side = PAD_SIDE_LEFT;
	padding.width = -1;
	padding.precision = -1;
	padding.padchar = ' ';

	/* Check for right padding */
	if (*ptr == '-')
	{
		padding.side = PAD_SIDE_RIGHT;
		ptr++;
	}
	/* Parse field width */
	if (isdigit(*ptr))
	{
		if (*ptr == '0')
		{
			padding.padchar = '0';
			ptr++;
		}
		padding.width = 0;
		while (isdigit(*ptr))
		{
			padding.width *= 10;
			padding.width += *ptr - '0';
			ptr++;
		}
	}
	
	/* Parse precision */
	if (*ptr == '.')
	{
		ptr++;
		if (isdigit(*ptr))
		{
			padding.precision = 0;
			while (isdigit(*ptr))
			{
				padding.precision *= 10;
				padding.precision += *ptr - '0';
				ptr++;
			}
		}
	}
	
	/* Parse format conversion */
	switch (c = *ptr++)
	{
		case 'a':
			exp = xmms_vputstr(string, input->album_name, &padding);
			break;
			
		case 'c':
			exp = xmms_vputstr(string, input->comment, &padding);
			break;
			
		case 'd':
			exp = xmms_vputstr(string, input->date, &padding);
			break;
			
		case 'e':
			exp = xmms_vputstr(string, input->file_ext, &padding);
			break;
			
		case 'f':
			exp = xmms_vputstr(string, input->file_name, &padding);
			break;
			
		case 'F':
			exp = xmms_vputstr(string, input->file_path, &padding);
			break;
			
		case 'g':
			exp = xmms_vputstr(string, input->genre, &padding);
			break;
			
		case 'n':
			exp = xmms_vputnum(string, input->track_number,
					   &padding);
			break;
			
		case 'p':
			exp = xmms_vputstr(string, input->performer, &padding);
			break;

		case 't':
			exp = xmms_vputstr(string, input->track_name, &padding);
			break;
			
		case 'y':
			exp = xmms_vputnum(string, input->year, &padding);
			break;
					
		case '%':
			g_string_append_c(string, '%');
			break;

		default:
			g_string_append_c(string, '%');
			if (c)
				g_string_append_c(string, c);
			break;
	}
	*fmt = ptr;
	return exp;
}


char *xmms_get_titlestring(char *fmt, TitleInput *input)
{
	int f_output = 0;
	GString *string;
	char *strptr;

	if (!fmt || !input || input->__size < XMMS_TITLEINPUT_SIZE)
		return NULL;

	string = g_string_new("");

	while (*fmt)
	{
		if (*fmt == '%')
		{
			fmt++;
			f_output += parse_variable(&fmt, string, input);
		}
		else
			/* Normal character */
			g_string_append_c(string, *fmt++);
	}

	/* Check if we actually output anything */
	if (!f_output)
	{
		g_string_free(string, TRUE);
		return NULL;
	}

	/* Return the result */
	strptr = string->str;
	g_string_free(string, FALSE);
	return strptr;
}



static struct tagdescr {
	char tag;
	char *description;
} descriptions[] = {
	{'p', N_("Performer/Artist")},
	{'a', N_("Album")},
	{'g', N_("Genre")},
	{'f', N_("File name")},
	{'F', N_("File path")},
	{'e', N_("File extension")},
	{'t', N_("Track name")},
	{'n', N_("Track number")},
	{'d', N_("Date")},
	{'y', N_("Year")},
	{'c', N_("Comment")},
};

GtkWidget* xmms_titlestring_descriptions(char* tags, int columns)
{
	GtkWidget *table, *label;
	char tagstr[5];
	int num, r, c, i;

	g_return_val_if_fail(tags != NULL, NULL);
	num = strlen(tags);
	g_return_val_if_fail(columns <= num, NULL);

	table = gtk_table_new((num + columns - 1) / columns, columns * 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	for (c = 0; c < columns; c++)
	{
		for (r = 0; r < (num + columns - 1 - c) / columns; r++)
		{
			sprintf(tagstr, "%%%c:", *tags);
			label = gtk_label_new(tagstr);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_table_attach(GTK_TABLE(table), label, 2 * c, 2 * c + 1, r, r + 1,
					 GTK_FILL, GTK_FILL, 0, 0);
			gtk_widget_show(label);

			for (i = 0; i <= sizeof(descriptions) / sizeof(struct tagdescr); i++)
			{
				if (i == sizeof(descriptions) / sizeof(struct tagdescr))
					g_warning("xmms_titlestring_descriptions(): Invalid tag: %c", *tags);
				else if (*tags == descriptions[i].tag)
				{
					label = gtk_label_new(gettext(descriptions[i].description));
					gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
					gtk_table_attach(GTK_TABLE(table), label, 2 * c + 1, 2 * c + 2, r, r + 1,
							 GTK_EXPAND | GTK_FILL,  GTK_EXPAND | GTK_FILL, 0 ,0);
					gtk_widget_show(label);
					break;
				}
			}
			tags++;
		}
	}

	return table;
}

