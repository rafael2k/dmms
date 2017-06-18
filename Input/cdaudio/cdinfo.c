/*
 *  cdinfo.c   Copyright 1999 Espen Skoglund <esk@ira.uka.de>
 *             Copyright 1999-2006 Håvard Kvålen <havardk@xmms.org>
 *
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


#include <glib.h>

#include "cdaudio.h"
#include "cdinfo.h"
#include "xmms/i18n.h"

/*
 * Function cdda_cdinfo_flush (cdinfo)
 *
 *    Free all information stored about the CD.
 *
 */
void cdda_cdinfo_flush(cdinfo_t * cdinfo)
{
	trackinfo_t *t;
	int i;

	if (cdinfo->albname)
		g_free(cdinfo->albname);
	if (cdinfo->artname)
		g_free(cdinfo->artname);

	cdinfo->albname = cdinfo->artname = NULL;

	for (t = cdinfo->tracks, i = 0; i < 100; i++, t++)
	{
		if (t->artist)
			g_free(t->artist);
		if (t->title)
			g_free(t->title);

		t->artist = t->title = NULL;
		t->num = -1;
	}
	cdinfo->is_valid = FALSE;
}


/*
 * Function cdda_cdinfo_delete (cdinfo)
 *
 *    Free the indicated `cdinfo' structure.
 *
 */
void cdda_cdinfo_delete(cdinfo_t * cdinfo)
{
	cdda_cdinfo_flush(cdinfo);
	g_free(cdinfo);
}


/*
 * Function cdda_cdinfo_new ()
 *
 *    Allocate a new `cdinfo' structure and return it.
 *
 */
cdinfo_t *cdda_cdinfo_new(void)
{
	cdinfo_t *ret;
	ret = g_malloc0(sizeof (cdinfo_t));
	cdda_cdinfo_flush(ret);

	return ret;
}


/*
 * Function cdda_cdinfo_track_set (cdinfo, num, artist, title)
 *
 *    Set `artist', and `title' for a track `num'.  If the CD is a
 *    singleartist disc, the `artist' on each track should be set to
 *    NULL.
 *
 */
void cdda_cdinfo_track_set(cdinfo_t *cdinfo, int num, char *artist, char *title)
{
	trackinfo_t *track;

	/* Check bounds */
	if (num < 1 || num >= 100)
		return;

	track = cdinfo->tracks + num;
	track->artist = artist;
	track->title = title;
	track->num = num;
	cdinfo->is_valid = TRUE;
}


/*
 * Function cdda_cdinfo_cd_set (cdinfo, cdname, cdartist)
 *
 *    Set name and artist for a cd.  If CD is a multiartist disc, the
 *    `artist' should be set to NULL.
 *
 */
void cdda_cdinfo_cd_set(cdinfo_t *cdinfo, char *cdname, char *cdartist)
{
	cdinfo->albname = cdname;
	cdinfo->artname = cdartist;
	cdinfo->is_valid = TRUE;
}


/*
 * Function cdda_cdinfo_get (cdinfo, num, artist, album, title)
 *
 *    Get artist, album, and title of the indicated track (i.e. store
 *    them in the specified pointers).  Return 0 upon success, or -1
 *    of track did not exist.  The returned name must be subsequently
 *    freed using g_free().
 *
 */
int cdda_cdinfo_get(cdinfo_t *cdinfo, int num, char **artist, char **album, char **title)
{
	trackinfo_t *track = cdinfo->tracks + num;

	/* Check validity */
	if (!cdinfo->is_valid || num < 1 || num >= 100)
		return -1;

	*artist = track->artist ? track->artist :
		cdinfo->artname ? cdinfo->artname : _("(unknown)");
	*album = cdinfo->albname ? cdinfo->albname : _("(unknown)");
	*title = track->title ? track->title : _("(unknown)");

	return track->num == -1 ? -1 : 0;
}


/*
 * Function cdda_cdinfo_write_file
 *
 * Writes the cdinfo_t structure to disk.
 */


void cdda_cdinfo_write_file(guint32 cddb_discid, cdinfo_t *cdinfo)
{
	/*
	 * We currently identify cdinfo on disk with the CDDB-discid.
	 * Maybe it would be smarter to use the cdindex id instead?
	 */
	
	char *filename;
	ConfigFile *cfg;
	char sectionname[10], trackstr[16];
	int i, numtracks = CLAMP(cddb_discid & 0xff, 0, 99);

	sprintf(sectionname, "%08x", cddb_discid);
	
	filename = g_strconcat(g_get_home_dir(), "/.xmms/cdinfo", NULL);
	if ((cfg = xmms_cfg_open_file(filename)) == NULL)
		cfg = xmms_cfg_new();

	if (cdinfo->albname)
		xmms_cfg_write_string(cfg, sectionname, "Albumname", cdinfo->albname);
	else
		xmms_cfg_write_string(cfg, sectionname, "Albumname", "");
	if (cdinfo->artname)
		xmms_cfg_write_string(cfg, sectionname, "Artistname", cdinfo->artname);
	for (i = 1; i <= numtracks; i++)
	{
		if (cdinfo->tracks[i].artist)
		{
			sprintf(trackstr, "track_artist%d", i);
			xmms_cfg_write_string(cfg, sectionname, trackstr, cdinfo->tracks[i].artist);
		}
		if (cdinfo->tracks[i].title)
		{
			sprintf(trackstr, "track_title%d", i);
			xmms_cfg_write_string(cfg, sectionname, trackstr, cdinfo->tracks[i].title);
		}
	}
	if (!xmms_cfg_write_file(cfg, filename))
		/*FIXME */;
	xmms_cfg_free(cfg);
	g_free(filename);
}

/*
 * Function cdda_cdinfo_read_file
 *
 * Tries to find and read a album from the disk-cache.
 *
 * Returns true if the album is found.
 */

gboolean cdda_cdinfo_read_file(guint32 cddb_discid, cdinfo_t *cdinfo)
{
	gchar *filename;
	ConfigFile *cfg;
	gchar sectionname[10], trackstr[16];
	gint i, numtracks = CLAMP(cddb_discid & 0xff, 0, 99);
	gboolean track_found;

	sprintf(sectionname, "%08x", cddb_discid);
	
	filename = g_strconcat(g_get_home_dir(), "/.xmms/cdinfo", NULL);
	if ((cfg = xmms_cfg_open_file(filename)) == NULL)
	{
		g_free(filename);
		return FALSE;
	}
	g_free(filename);

	if (!xmms_cfg_read_string(cfg, sectionname, "Albumname", &cdinfo->albname))
		return FALSE;

	xmms_cfg_read_string(cfg, sectionname, "Artistname", &cdinfo->artname);

	for (i = 1; i <= numtracks; i++)
	{
		track_found = FALSE;
		sprintf(trackstr, "track_artist%d", i);
		if (xmms_cfg_read_string(cfg, sectionname, trackstr, &cdinfo->tracks[i].artist))
			track_found = TRUE;
		sprintf(trackstr, "track_title%d", i);
		if (xmms_cfg_read_string(cfg, sectionname, trackstr, &cdinfo->tracks[i].title))
			track_found = TRUE;
		if (track_found)
			cdinfo->tracks[i].num = i;
	}
	cdinfo->is_valid = TRUE;
	xmms_cfg_free(cfg);
	return TRUE;
}
