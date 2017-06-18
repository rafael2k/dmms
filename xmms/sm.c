/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
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

#include "xmms.h"

#ifdef WITH_SM
#include <X11/SM/SMlib.h>

void mainwin_quit_cb(void);

static SmcConn smc_conn = NULL;
static IceConn ice_conn;
static char *session_id;

static void sm_save_yourself(SmcConn c, SmPointer p, int save_type, Bool shutdown,
			     int interact_style, Bool fast)
{
	GDK_THREADS_ENTER();
	save_config();
	SmcSaveYourselfDone(c, TRUE);
	GDK_THREADS_LEAVE();
}

static void sm_shutdown_cancelled(SmcConn c, SmPointer a)
{
}

void sm_save_complete(SmcConn c, SmPointer a)
{
}

static void sm_die(SmcConn c, SmPointer arg)
{
	GDK_THREADS_ENTER();
	mainwin_quit_cb();
	GDK_THREADS_LEAVE();
}

static void ice_handler(gpointer data, gint source, GdkInputCondition condition)
{
	IceProcessMessages(data, NULL, NULL);
}

const char * sm_init(int argc, char **argv, const char *previous_session_id)
{
	SmcCallbacks smcall;
	char errstr[256];

	if (!getenv("SESSION_MANAGER"))
		return NULL;

	memset(&smcall, 0, sizeof(smcall));
	smcall.save_yourself.callback = sm_save_yourself;
	smcall.die.callback = sm_die;
	smcall.save_complete.callback = sm_save_complete;
	smcall.shutdown_cancelled.callback = sm_shutdown_cancelled;

	smc_conn = SmcOpenConnection(NULL, NULL, SmProtoMajor, SmProtoMinor,
				     SmcSaveYourselfProcMask |
				     SmcSaveCompleteProcMask |
				     SmcShutdownCancelledProcMask |
				     SmcDieProcMask,
				     &smcall, (char*) previous_session_id,
                                     &session_id,
				     sizeof(errstr), errstr);
	if (smc_conn)
	{
		SmPropValue program_val, restart_val[3];
		SmPropValue userid_val  = { 0, NULL };
		SmProp program_prop, userid_prop, restart_prop, clone_prop, *props[4];

		program_val.length = strlen("xmms");
		program_val.value = "xmms";

		program_prop.name = SmProgram;
		program_prop.type = SmARRAY8;
		program_prop.num_vals = 1;
		program_prop.vals = &program_val;

		userid_prop.name = SmProgram;
		userid_prop.type = SmARRAY8;
		userid_prop.num_vals = 1;
		userid_prop.vals = &userid_val;

		userid_val.value = g_strdup_printf("%d", geteuid());
		userid_val.length = strlen(userid_val.value);

		restart_prop.name = SmRestartCommand;
		restart_prop.type = SmLISTofARRAY8;
		restart_prop.num_vals = session_id ? 3 : 1;
		restart_prop.vals = &restart_val[0];

		restart_val[0].length = strlen(argv[0]);
		restart_val[0].value = argv[0];
		if (session_id != NULL)
		{
			restart_val[1].length = strlen("--sm-client-id");
			restart_val[1].value = "--sm-client-id";
			restart_val[2].length = strlen(session_id);
			restart_val[2].value = session_id;
		}

		clone_prop.name = SmCloneCommand;
		clone_prop.type = SmLISTofARRAY8;
		clone_prop.num_vals = 1;
		clone_prop.vals = &restart_val[0];

		props[0] = &program_prop;
		props[1] = &userid_prop;
		props[2] = &restart_prop;
		props[3] = &clone_prop;                

		SmcSetProperties(smc_conn, sizeof(props) / sizeof(props[0]),
				 (SmProp **)&props);
		ice_conn = SmcGetIceConnection(smc_conn);
		gdk_input_add(IceConnectionNumber(ice_conn), GDK_INPUT_READ,
			      ice_handler, ice_conn);

		g_free(userid_val.value);
	}

	return session_id;
}

void sm_cleanup(void)
{
	if (smc_conn)
		SmcCloseConnection(smc_conn, 0, NULL);
}

#else

const char * sm_init(int argc, char **argv, const char *previous_session_id)
{
	return NULL;
}

void sm_cleanup(void)
{
}


#endif

