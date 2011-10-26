/**
	\file apps/obex_test_server.c
	Server OBEX Commands.
	OpenOBEX test applications and sample code.

	Copyright (c) 2000, Pontus Fuchs, All Rights Reserved.

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

#include "obex_io.h"
#include "obex_test.h"
#include "obex_test_server.h"

#include <stdio.h>

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

//
//
//
void put_server(obex_t *handle, obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;

	const uint8_t *body = NULL;
	int body_len = 0;
	char *name = NULL;
	char *namebuf = NULL;

	printf("%s()\n", __FUNCTION__);

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_BODY:
			printf("%s() Found body\n", __FUNCTION__);
			body = hv.bs;
			body_len = hlen;
			break;
		case OBEX_HDR_NAME:
			printf("%s() Found name\n", __FUNCTION__);
			if( (namebuf = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar((uint8_t *) namebuf, hv.bs, hlen);
				name = namebuf;
			}
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
		name = "OBEX_PUT_Unknown_object";
		printf("Got a PUT without a name. Setting name to %s\n", name);

	}
	safe_save_file(name, body, body_len);
	free(namebuf);
}

//
//
//
void get_server(obex_t *handle, obex_object_t *object)
{
	uint8_t *buf;

	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;
	int file_size;

	char *name = NULL;
	char *namebuf = NULL;

	printf("%s()\n", __FUNCTION__);

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			printf("%s() Found name\n", __FUNCTION__);
			if( (namebuf = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar((uint8_t *) namebuf, hv.bs, hlen);
				name = namebuf;
			}
			break;
		
		default:
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}

	if(!name)	{
		printf("%s() Got a GET without a name-header!\n", __FUNCTION__);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}
	printf("%s() Got a request for %s\n", __FUNCTION__, name);

	buf = easy_readfile(name, &file_size);
	if(buf == NULL) {
		printf("Can't find file %s\n", name);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}

	OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
	hv.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, file_size, 0);
	hv.bq4 = file_size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
	free(buf);
	return;
}

//
//
//
void connect_server(obex_t *handle, obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;

	const uint8_t *who = NULL;
	int who_len = 0;
	printf("%s()\n", __FUNCTION__);

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		if(hi == OBEX_HDR_WHO)	{
			who = hv.bs;
			who_len = hlen;
		}
		else	{
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}
	if (who_len == 6)	{
		if(memcmp("Linux", who, 6) == 0)	{
			printf("Weeeha. I'm talking to the coolest OS ever!\n");
		}
	}
	OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
}

//
//
//
void server_request(obex_t *handle, obex_object_t *object, int event, int cmd)
{
	switch(cmd)	{
	case OBEX_CMD_CONNECT:
		connect_server(handle, object);
		break;
	case OBEX_CMD_DISCONNECT:
		printf("We got a disconnect-request\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	case OBEX_CMD_GET:
		/* A Get always fits one package */
		get_server(handle, object);
		break;
	case OBEX_CMD_PUT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		put_server(handle, object);
		break;
	case OBEX_CMD_SETPATH:
		printf("Got a SETPATH request\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;
	default:
		printf("%s() Denied %02x request\n", __FUNCTION__, cmd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	}
	return;
}

//
//
//
void server_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp)
{
	struct context *gt;
	gt = OBEX_GetUserData(handle);

	printf("Server request finished!\n");

	switch (obex_cmd) {
	case OBEX_CMD_DISCONNECT:
		printf("Disconnect done!\n");
		OBEX_TransportDisconnect(handle);
		gt->serverdone = TRUE;
		break;

	default:
		printf("%s() Command (%02x) has now finished\n", __FUNCTION__, obex_cmd);
		break;
	}
}

//
//
//
void server_do(obex_t *handle)
{
        int ret;
	struct context *gt;
	gt = OBEX_GetUserData(handle);

	gt->serverdone = FALSE;
	while(!gt->serverdone) {
	        ret = OBEX_HandleInput(handle, 10);
		if(ret < 0) {
			printf("Error while doing OBEX_HandleInput()\n");
			break;
		} else if (ret == 0) {
			printf("Timeout while doing OBEX_HandleInput()\n");
			break;
		} else {
			printf("OBEX_HandleInput() returned %d\n",ret);
		}
	}
}
