/**
	\file apps/obex_put_common.c
	Utility-functions to act as a PUT server and client.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openobex/obex.h>

#include "obex_put_common.h"
#include "obex_io.h"

#include <stdio.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0

extern obex_t *handle;
extern volatile int finished;

volatile int last_rsp = OBEX_RSP_BAD_REQUEST;

/*
 * Function put_done()
 *
 *    Parse what we got from a PUT
 *
 */
void put_done(obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;

	const uint8_t *body = NULL;
	int body_len = 0;
	char *name = NULL;
	char *namebuf = NULL;

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_BODY:
			body = hv.bs;
			body_len = hlen;
			break;
		case OBEX_HDR_NAME:
			if( (namebuf = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar((uint8_t *) namebuf, hv.bs, hlen);
				name = namebuf;
			}
			break;

		case OBEX_HDR_LENGTH:
			printf("HEADER_LENGTH = %d\n", hv.bq4);
			break;

		case HEADER_CREATOR_ID:
			printf("CREATORID = %#x\n", hv.bq4);
			break;
		
		default:
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}
	if(!body)	{
		printf("Got a PUT without a body\n");
		return;
	}
	if(!name)	{
		printf("Got a PUT without a name. Setting name to %s\n", name);
		name = "OBEX_PUT_Unknown_object";

	}
	safe_save_file(name, body, body_len);
	free(namebuf);
}


/*
 * Function server_indication()
 *
 * Called when a request is about to come or has come.
 *
 */
void server_request(obex_object_t *object, int event, int cmd)
{
	switch(cmd)	{
	case OBEX_CMD_SETPATH:
		printf("Received SETPATH command\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;
	case OBEX_CMD_PUT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		put_done(object);
		break;
	case OBEX_CMD_CONNECT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	case OBEX_CMD_DISCONNECT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	default:
		printf("%s() Denied %02x request\n", __FUNCTION__, cmd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	}
	return;
}

/*
 * Function client_done_indication()
 *
 * Called when a server-operation has finished
 *
 */
void server_done(obex_object_t *object, int obex_cmd)
{
	/* Quit if DISCONNECT has finished */
	if(obex_cmd == OBEX_CMD_DISCONNECT)
		finished = 1;
}


/*
 * Function client_done()
 *
 * Called when a client-operation has finished
 *
 */
void client_done(obex_object_t *object, int obex_cmd, int obex_rsp)
{
	last_rsp = obex_rsp;
	finished = TRUE;
}

/*
 * Function obex_event ()
 *
 *    Called by the obex-layer when some event occurs.
 *
 */
void obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	switch (event)	{
	case OBEX_EV_PROGRESS:
		printf(".");
		break;
	case OBEX_EV_REQDONE:
		printf("\n");
		/* Comes when a command has finished. */
		if(mode == OBEX_MODE_CLIENT)
			client_done(object, obex_cmd, obex_rsp);
		else
			server_done(object, obex_cmd);
		break;
	case OBEX_EV_REQHINT:
		/* Comes BEFORE the lib parses anything. */
		switch(obex_cmd) {
		case OBEX_CMD_PUT:
		case OBEX_CMD_CONNECT:
		case OBEX_CMD_DISCONNECT:
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			break;
		default:
			OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
			break;
		}
		break;
	case OBEX_EV_REQ:
		/* Comes when a server-request has been received. */
		server_request(object, event, obex_cmd);
		break;
	case OBEX_EV_LINKERR:
		printf("Link broken (this does not have to be an error)!\n");
		finished = 1;
		break;
	default:
		printf("Unknown event!\n");
		break;
	}
}

/*
 * Function wait_for_rsp()
 *
 *    Wait for a request to finish!
 *
 * Timeout set to 10s. Should be more than good enough for most transport.
 */
int wait_for_rsp()
{
	int ret;

	while(!finished) {
		//printf("wait_for_rsp()\n");
		ret = OBEX_HandleInput(handle, 10);
		if (ret < 0)
			return ret;
	}
	return last_rsp;
}

/*
 * Function do_sync_request()
 *
 *    Execute an OBEX-request synchronously.
 *
 */
int do_sync_request(obex_t *handle, obex_object_t *object, int async)
{
	int ret;
	OBEX_Request(handle, object);
	ret = wait_for_rsp();
	finished = FALSE;
	return ret;
}
