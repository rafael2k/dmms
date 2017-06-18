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

#include "config.h"
#include "urldecode.h"
#include <glib.h>
#include <string.h>
#include <stdio.h>

/* URL-decode a file: URL path, return NULL if it's not what we expect */
char *xmms_urldecode_path(char *encoded_path)
{
	char *tmp, *cur, *ext;

	if (!encoded_path || *encoded_path == '\0' ||
	    strncasecmp(encoded_path, "file:", 5))
		return NULL;
	cur = encoded_path + 5;
	if (!strncasecmp(cur, "//localhost", 11))
		cur += 11;
	tmp = g_malloc0(strlen(cur) + 1);
	while ((ext = strchr(cur, '%')) != NULL)
	{
		int realchar;

		strncat(tmp, cur, ext - cur);
		ext++;
		cur = ext + 2;
		if (!sscanf(ext, "%2x", &realchar))
		{
			/*
			 * Assume it is a literal '%'.  Several file
			 * managers send unencoded file: urls on on
			 * drag and drop.
			 */
			realchar = '%';
			cur -= 2;
		}
		tmp[strlen(tmp)] = realchar;
	}
	strcat(tmp, cur);
	strcpy(encoded_path, tmp);
	g_free(tmp);
	
	return encoded_path;
}
