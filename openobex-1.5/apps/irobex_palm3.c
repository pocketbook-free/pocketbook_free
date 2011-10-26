/**
	\file apps/irobex_palm3.c
 	Demonstrates use of PUT command.
	OpenOBEX test applications and sample code.

 	Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

/*
	The Palm 3 always sends a HEADER_CREATORID (0xcf).

	Start without arguments to receive a file.
	Start with filename as argument to send a file. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openobex/obex.h>
#include "obex_put_common.h"
#include "obex_io.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <string.h>
#endif

#define TRUE  1
#define FALSE 0

obex_t *handle = NULL;
volatile int finished = FALSE;
extern int last_rsp;

/*
 * Function usage (void)
 */
int usage(char *argv[]) {
	printf ("Usage: %s [-h id] [name]\n", argv[0]);
	printf ("       where id is the header_creator_id (default: memo)\n");
	return -1;
}

/*
 * Function main (argc, )
 *
 *    Starts all the fun!
 *
 */
int main(int argc, char *argv[])
{
	obex_object_t *object;
	int ret, exitval = EXIT_SUCCESS;
	uint32_t creator_id = 0;

	printf("Send and receive files to Palm3\n");
	if ((argc < 1) || (argc > 4))	{
		return usage(argv);
	}
	handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, 0);

	if (argc == 1)	{
		/* We are server*/

		printf("Waiting for files\n");
		IrOBEX_ServerRegister(handle, "OBEX");

		while (!finished)
			OBEX_HandleInput(handle, 1);
	}
	else {
		/* We are a client */

		/* Check for parameters */
		if(argc > 2) {
			if(strcmp("-h", argv[1]) || strlen(argv[2]) != 4)
				return usage(argv);
			creator_id = argv[2][0] << 24 | argv[2][1] << 16 | argv[2][2] << 8 | argv[2][3];
			printf("debug: %s %d;", argv[2], creator_id);
		}

		/* Try to connect to peer */
		ret = IrOBEX_TransportConnect(handle, "OBEX");
		if (ret < 0) {
			printf("Sorry, unable to connect!\n");
			return EXIT_FAILURE;
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		ret = do_sync_request(handle, object, FALSE);
		if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0)) {
			printf("Sorry, unable to connect!\n");
			return EXIT_FAILURE;
		}
		if ((object = build_object_from_file(handle, argv[(creator_id ? 3 : 1)], creator_id))) {
			ret = do_sync_request(handle, object, FALSE);
			if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0))
				exitval = EXIT_FAILURE;
		} else
			exitval = EXIT_FAILURE;

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		ret = do_sync_request(handle, object, FALSE);
		if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0))
			exitval = EXIT_FAILURE;

		if (exitval == EXIT_SUCCESS)
			printf("PUT successful\n");
		else
			printf("PUT failed\n");
	}
//	sleep(1);
	return exitval;
}
