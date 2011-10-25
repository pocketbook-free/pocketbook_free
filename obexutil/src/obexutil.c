/**
	\file apps/obex_test.c
	Test IrOBEX, TCPOBEX and OBEX over cable to R320s.
	OpenOBEX test applications and sample code.

	Copyright (c) 2000 Pontus Fuchs, All Rights Reserved.

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

#include "bluez_compat.h"

#include <openobex/obex.h>

#include "obex_io.h"
#include "obexutil.h"
#include "uiquery.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifndef in_addr_t
#define in_addr_t unsigned long
#endif

#define TRUE  1
#define FALSE 0

static int server = 0;

static char object_name[256] = { 0 };
static int object_size = 0;
static int bytes_done = 0;
static uint8_t send_data[4096];
static FILE *send_fp = NULL;
static FILE *recv_fp = NULL;
static int seq = 0;
static int received_ok = 0;

static int serverdone = FALSE;
static int clientdone = FALSE;

static void cleanup() {

	if (send_fp) fclose(send_fp);
	send_fp = NULL;
	if (recv_fp) fclose(recv_fp);
	recv_fp = NULL;
	if (seq > 0) uiquery_dismiss(seq);
	seq = 0;
	if (received_ok) uiquery_event(72, 0, 0);
	received_ok = 0;
	object_name[0] = 0;
	object_size = 0;
	bytes_done = 0;
	sync();

}

static void parse_request_data(obex_t *handle, obex_object_t *object) {

	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen)) {
		switch(hi)	{
		    case OBEX_HDR_NAME:
			unicode_to_utf8(hv.bs, hlen, object_name, sizeof(object_name));
			break;
		    case OBEX_HDR_LENGTH:
			object_size = hv.bq4;
			break;
		}
	}
	
}

static void process_server_request(obex_t *handle, obex_object_t *object, int event, int cmd)
{
	switch(cmd) {
	    case OBEX_CMD_CONNECT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	    case OBEX_CMD_DISCONNECT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	    case OBEX_CMD_GET:
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	    case OBEX_CMD_PUT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		if (bytes_done > 0) {
			if (object_name[0] == 0) strcpy(object_name, "unknown.txt");
			char *dname = destination_path(object_name);
			if (rename(TEMPFILE, dname) == 0) {
				received_ok = 1;
				usleep(200000);
			} else {
				fprintf(stderr, "could not rename file\n");
				unlink(TEMPFILE);
			}
			free(dname);
		} else {
			fprintf(stderr, "got object without body\n");
			unlink(TEMPFILE);
		}
		cleanup();
		break;
	    default:
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	}
	return;
}

void server_request_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp)
{
	switch (obex_cmd) {
	    case OBEX_CMD_DISCONNECT:
		OBEX_TransportDisconnect(handle);
		serverdone = TRUE;
		break;

	    default:
		break;
	}
}

void client_request_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp)
{
	switch (obex_cmd) {
	    case OBEX_CMD_CONNECT:
		if(obex_rsp == OBEX_RSP_SUCCESS) {
			fprintf(stderr, "Conneck OK!\n");
		} else {
			fprintf(stderr, "Conneck failed!\n");
		}
		break;
	    case OBEX_CMD_DISCONNECT:
		OBEX_TransportDisconnect(handle);
		break;
	    case OBEX_CMD_PUT:
		if(obex_rsp == OBEX_RSP_SUCCESS) {
			fprintf(stderr, "transfer successful!\n");
		} else {
			fprintf(stderr, "transfer failed (0x%02x)!\n", obex_rsp);
		}
		break;
	    default:
		break;
	}
	clientdone = TRUE;
}

//
// Called by the obex-layer when some event occurs.
//
void obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	const uint8_t *newdata;
	obex_headerdata_t hd;
	int pc, len;

	switch (event)	{

	    case OBEX_EV_PROGRESS:
		fprintf(stderr, ".");
		if (server) parse_request_data(handle, object);
		if (seq != 0 && uiquery_status(seq, NULL, 0) == UIQ_CANCEL) {
			fprintf(stderr, "request cancelled by user\n");
			OBEX_TransportDisconnect(handle);
			serverdone = clientdone = TRUE;
		}
		break;

	    case OBEX_EV_REQDONE:
		if(server) {
			server_request_done(handle, object, obex_cmd, obex_rsp);
		} else {
			client_request_done(handle, object, obex_cmd, obex_rsp);
		}
		cleanup();
		break;

	    case OBEX_EV_REQHINT:
		if (! server) break;
		switch (obex_cmd) {
		    case OBEX_CMD_CONNECT:
		    case OBEX_CMD_DISCONNECT:
			cleanup();
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			break;
		    case OBEX_CMD_PUT:
			seq = uiquery_progress(1, "Bluetooth", "@recv_file", 0);
			bytes_done = 0;
			mkdir(STORAGEPATH, 0777);
			unlink(TEMPFILE);
			recv_fp = fopen(TEMPFILE, "wb");
			if (recv_fp == NULL) {
				OBEX_ObjectSetRsp(object, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
			} else {
				OBEX_ObjectReadStream(handle, object, NULL);
				OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			}
			break;
		    default:
		        OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
			break;
		}
		break;

	    case OBEX_EV_REQ:
		if (server) {
			parse_request_data(handle, object);
			process_server_request(handle, object, event, obex_cmd);
		}
		break;

	    case OBEX_EV_ABORT:
		fprintf(stderr, "request aborted!\n");
		OBEX_TransportDisconnect(handle);
		cleanup();
		break;

	    case OBEX_EV_LINKERR:
		fprintf(stderr, "link broken!\n");
		OBEX_TransportDisconnect(handle);
		cleanup();
		break;

	    case OBEX_EV_STREAMEMPTY:
		len = fread(send_data, 1, sizeof(send_data), send_fp);
		hd.bs = send_data;
		if (len > 0) {
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, len, OBEX_FL_STREAM_DATA);
			bytes_done += len;
		} else {
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, 0, OBEX_FL_STREAM_DATAEND);
		}
		pc = (object_size == 0) ? 0 : (bytes_done * 100) / object_size;
		uiquery_update(seq, (object_name[0]==0) ? NULL : object_name, pc);
		break;

	    case OBEX_EV_STREAMAVAIL:
		len = OBEX_ObjectReadStream(handle, object, &newdata);
		if (len > 0) {
			fwrite(newdata, 1, len, recv_fp);
			bytes_done += len;
		}
		pc = (object_size == 0) ? 0 : (bytes_done * 100) / object_size;
		uiquery_update(seq, (object_name[0]==0) ? NULL : object_name, pc);
		break;

	default:
		//fprintf(stderr, "Unknown event %02x!\n", event);
		break;
	}
}

static int send_obex_request(obex_t *handle, obex_object_t *object) {

	int ret = 0;

 	OBEX_Request(handle, object);

	while (! clientdone) {
		ret = OBEX_HandleInput(handle, 10);
		if(ret < 0) {
			fprintf(stderr, "Error while doing OBEX_HandleInput()\n");
			break;
		}
		if(ret == 0) {
			fprintf(stderr, "Timeout waiting for data. Aborting\n");
			OBEX_CancelRequest(handle, FALSE);
			break;
		}
	}
	clientdone = FALSE;
	return (ret > 0);

}

static void usage() {

	fprintf(stderr,
		"Usage:\n"
		"  obexutil -s <bdaddr> <channel> filename [...]\n"
		"  obexutil -d <channel>\n"
	);
	exit(1);

}

int main (int argc, char *argv[])
{
	obex_t *handle;
	obex_object_t *object;
	obex_headerdata_t hd;
	bdaddr_t bdaddr;
	uint8_t channel = 0;
	char *name, *p;
	uint8_t ucname[256];
	int i, nlen, ret;

	if (argc < 2) usage();

	if(! (handle = OBEX_Init(OBEX_TRANS_BLUETOOTH, obex_event, 0))) {
		perror( "OBEX_Init failed");
		exit(1);
	}

	if (strcmp(argv[1], "-d") == 0) {

		if (argc < 3) usage();
		bacpy(&bdaddr, BDADDR_ANY);
		channel = atoi(argv[2]);
		server = 1;

		if (fork() == 0) {
			chdir("/");
			fclose(stdin);
			fclose(stdout);
			while (1) {
				if(BtOBEX_ServerRegister(handle, BDADDR_ANY, channel) < 0) {
					fprintf(stderr, "coult not register bluetooth service\n");
					return 1;
				}
				serverdone = FALSE;
				while (! serverdone) {
				        ret = OBEX_HandleInput(handle, 30);
					if(ret < 0) {
						fprintf(stderr, "Error while doing OBEX_HandleInput()\n");
						break;
					} else if (ret == 0) {
						continue;
					} else {
						//printf("OBEX_HandleInput() returned %d\n",ret);
					}
				}
				OBEX_TransportDisconnect(handle);
			}
		}

	} else if (strcmp(argv[1], "-s") == 0) {

		if (argc < 5) usage();
		str2ba(argv[2], &bdaddr);
		channel = atoi(argv[3]);
		if(BtOBEX_TransportConnect(handle, BDADDR_ANY, &bdaddr, channel) <0) {
			fprintf(stderr, "could not connect to %s\n", argv[2]);
			return 1;
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		if (! object) {
			fprintf(stderr, "could not create object\n");
			return 1;
		}

		hd.bs = (uint8_t *) DEVICENAME;
		if (OBEX_ObjectAddHeader(handle, object, OBEX_HDR_WHO, hd, strlen(DEVICENAME)+1, OBEX_FL_FIT_ONE_PACKET) < 0) {
			fprintf(stderr, "could not add header\n");
			return 1;
		}

		if (! send_obex_request(handle, object)) {
			fprintf(stderr, "error sending connect request\n");
			return 1;
		}

		for (i=4; i<argc; i++) {

			name = argv[i];
			send_fp = fopen(name, "rb");
			if (send_fp == NULL) {
				fprintf(stderr, "could not open %s\n", name);
				continue;
			}
			fseek(send_fp, 0, SEEK_END);
			object_size = ftell(send_fp);
			fseek(send_fp, 0, SEEK_SET);

			fprintf(stderr, "sending file %s (%i bytes)\n", name, object_size);

			seq = uiquery_progress(1, "Bluetooth", "@send_file", 0);

			object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);

			hd.bq4 = object_size;
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hd, 4, 0);

			p = strrchr(name, '/');
			if (p == NULL) p = name; else ++p;
			nlen = utf8_to_unicode(p, strlen(p), ucname, sizeof(ucname));
			hd.bs = (uint8_t *) ucname;
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd, nlen, 0);

			hd.bs = NULL;
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, object_size, OBEX_FL_STREAM_START);

			if (! send_obex_request(handle, object)) {
				fprintf(stderr, "error sending put request\n");
				cleanup();
				break;
			}

		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		send_obex_request(handle, object);
		cleanup();

	}


	return 0;
}
