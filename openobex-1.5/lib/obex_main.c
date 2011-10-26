/**
	\file obex_main.c
	Implementation of the Object Exchange Protocol OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 2000 Pontus Fuchs, All Rights Reserved.
	Copyright (c) 1998, 1999 Dag Brattli, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#else /* _WIN32 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>

#endif /* _WIN32 */

#ifdef HAVE_BLUETOOTH
#include "bluez_compat.h"
#endif /*HAVE_BLUETOOTH*/

#include "obex_main.h"
#include "obex_header.h"
#include "obex_server.h"
#include "obex_client.h"

#include <openobex/obex_const.h>

#ifdef OBEX_DEBUG
int obex_debug;
#endif
#ifdef OBEX_DUMP
int obex_dump;
#endif

/*
 * Function obex_create_socket()
 *
 *    Create socket if needed.
 *
 */
socket_t obex_create_socket(obex_t *self, int domain)
{
	socket_t fd;
	int proto;
	DEBUG(4, "\n");

	proto = 0;

#ifdef HAVE_BLUETOOTH
	if (domain == AF_BLUETOOTH)
		proto = BTPROTO_RFCOMM;
#endif /*HAVE_BLUETOOTH*/

	fd = socket(domain, SOCK_STREAM, proto);
	return fd;
}

/*
 * Function obex_delete_socket()
 *
 *    Close socket if opened.
 *
 */
int obex_delete_socket(obex_t *self, socket_t fd)
{
	int ret;

	DEBUG(4, "\n");

	if(fd < 0)
		return fd;

#ifdef _WIN32
	ret = closesocket(fd);
#else /* _WIN32 */
	ret = close(fd);
#endif /* _WIN32 */
	return ret;
}


/*
 * Function obex_response_to_string(rsp)
 *
 *    Return a string of an OBEX-response
 *
 */
char *obex_response_to_string(int rsp)
{
	switch (rsp) {
	case OBEX_RSP_CONTINUE:
		return "Continue";
	case OBEX_RSP_SWITCH_PRO:
		return "Switching protocols";
	case OBEX_RSP_SUCCESS:
		return "OK, Success";
	case OBEX_RSP_CREATED:
		return "Created";
	case OBEX_RSP_ACCEPTED:
		return "Accepted";
	case OBEX_RSP_NO_CONTENT:
		return "No Content";
	case OBEX_RSP_BAD_REQUEST:
		return "Bad Request";
	case OBEX_RSP_UNAUTHORIZED:
		return "Unauthorized";
	case OBEX_RSP_PAYMENT_REQUIRED:
		return "Payment required";
	case OBEX_RSP_FORBIDDEN:
		return "Forbidden";
	case OBEX_RSP_NOT_FOUND:
		return "Not found";
	case OBEX_RSP_METHOD_NOT_ALLOWED:
		return "Method not allowed";
	case OBEX_RSP_CONFLICT:
		return "Conflict";
	case OBEX_RSP_INTERNAL_SERVER_ERROR:
		return "Internal server error";
	case OBEX_RSP_NOT_IMPLEMENTED:
		return "Not implemented!";
	case OBEX_RSP_DATABASE_FULL:
		return "Database full";
	case OBEX_RSP_DATABASE_LOCKED:
		return "Database locked";
	default:
		return "Unknown response";
	}
}

/*
 * Function obex_deliver_event ()
 *
 *    Deliver an event to app.
 *
 */
void obex_deliver_event(obex_t *self, int event, int cmd, int rsp, int del)
{
	obex_object_t *object = self->object;

	if (del == TRUE)
		self->object = NULL;

	if (self->state & MODE_SRV)
		self->eventcb(self, object, OBEX_MODE_SERVER, event, cmd, rsp);
	else
		self->eventcb(self, object, OBEX_MODE_CLIENT, event, cmd, rsp);
	
	if (del == TRUE)
		obex_object_delete(object);
}

/*
 * Function obex_response_request (self, opcode)
 *
 *    Send a response to peer device
 *
 */
void obex_response_request(obex_t *self, uint8_t opcode)
{
	buf_t *msg;

	obex_return_if_fail(self != NULL);

	msg = buf_reuse(self->tx_msg);

	obex_data_request(self, msg, opcode | OBEX_FINAL);
}

/*
 * Function obex_data_request (self, opcode, cmd)
 *
 *    Send response or command code along with optional headers/data.
 *
 */
int obex_data_request(obex_t *self, buf_t *msg, int opcode)
{
	obex_common_hdr_t *hdr;
	int actual = 0;

	obex_return_val_if_fail(self != NULL, -1);
	obex_return_val_if_fail(msg != NULL, -1);

	/* Insert common header */
	hdr = (obex_common_hdr_t *) buf_reserve_begin(msg, sizeof(obex_common_hdr_t));

	hdr->opcode = opcode;
	hdr->len = htons((uint16_t)msg->data_size);

	DUMPBUFFER(1, "Tx", msg);
	DEBUG(1, "len = %d bytes\n", msg->data_size);

	actual = obex_transport_write(self, msg);
	return actual;
}

/*
 * Function obex_data_indication (self)
 *
 *    Read/Feed some input from device and find out which packet it is
 *
 */
int obex_data_indication(obex_t *self, uint8_t *buf, int buflen)
{
	obex_common_hdr_t *hdr;
	buf_t *msg;
	int final;
	int actual = 0;
	unsigned int size;
	int ret;
	
	DEBUG(4, "\n");

	obex_return_val_if_fail(self != NULL, -1);

	msg = self->rx_msg;
	
	/* First we need 3 bytes to be able to know how much data to read */
	if(msg->data_size < 3)  {
		actual = obex_transport_read(self, 3 - (msg->data_size), buf, buflen);
		
		DEBUG(4, "Got %d bytes\n", actual);

		/* Check if we are still connected */
		/* do not error if the data is from non-empty but partial buffer (custom transport) */
		if (buf == NULL && buflen == 0 && actual <= 0)	{
			obex_deliver_event(self, OBEX_EV_LINKERR, 0, 0, TRUE);
			return actual;
		}
		buf += actual;
		buflen -= actual;
	}

	/* If we have 3 bytes data we can decide how big the packet is */
	if(msg->data_size >= 3) {
		hdr = (obex_common_hdr_t *) msg->data;
		size = ntohs(hdr->len);

		actual = 0;
		if(msg->data_size < (int) ntohs(hdr->len)) {

			actual = obex_transport_read(self, size - msg->data_size, buf,
				buflen);

			/* Check if we are still connected */
			/* do not error if the data is from non-empty but partial buffer (custom transport) */
			if (buf == NULL && buflen == 0 && actual <= 0)	{
				obex_deliver_event(self, OBEX_EV_LINKERR, 0, 0, TRUE);
				return actual;
			}
		}
	}
        else {
		/* Wait until we have at least 3 bytes data */
		DEBUG(3, "Need at least 3 bytes got only %d!\n", msg->data_size);
		return actual;
        }


	/* New data has been inserted at the end of message */
	DEBUG(1, "Got %d bytes msg len=%d\n", actual, msg->data_size);

	/*
	 * Make sure that the buffer we have, actually has the specified
	 * number of bytes. If not the frame may have been fragmented, and
	 * we will then need to read more from the socket.  
	 */

	/* Make sure we have a whole packet */
	if (size > msg->data_size) {
		DEBUG(3, "Need more data, size=%d, len=%d!\n",
		      size, msg->data_size);

		/* I'll be back! */
		return msg->data_size;
	}

	DUMPBUFFER(2, "Rx", msg);

	actual = msg->data_size;
	final = hdr->opcode & OBEX_FINAL; /* Extract final bit */

	/* Dispatch to the mode we are in */
	if(self->state & MODE_SRV) {
		ret = obex_server(self, msg, final);
		buf_reuse(msg);
		
	}
	else	{
		ret = obex_client(self, msg, final);
		buf_reuse(msg);
	}
	/* Check parse errors */
	if(ret < 0)
		actual = ret;
	return actual;
}

/*
 * Function obex_cancel_request ()
 *
 *    Cancel an ongoing request
 *
 */
int obex_cancelrequest(obex_t *self, int nice)
{
	/* If we have no ongoing request do nothing */
	if(self->object == NULL)
		return 0;

	/* Abort request without sending abort */
	if (!nice) {
		/* Deliver event will delete the object */
		obex_deliver_event(self, OBEX_EV_ABORT, 0, 0, TRUE);
		buf_reuse(self->tx_msg);
		buf_reuse(self->rx_msg);
		/* Since we didn't send ABORT to peer we are out of sync
		   and need to disconnect transport immediately, so we signal
		   link error to app */
		obex_deliver_event(self, OBEX_EV_LINKERR, 0, 0, FALSE);
		return 1;
	} else {
		obex_object_t *object;

		object = obex_object_new();
		if (object == NULL)
			return -1;

		obex_object_setcmd(object, OBEX_CMD_ABORT, OBEX_CMD_ABORT);

		if (obex_object_send(self, object, TRUE, TRUE) < 0) {
			obex_object_delete(object);
			return -1;
		}

		obex_object_delete(object);

		self->object->abort = TRUE;
		self->state = STATE_REC;

		return 0;
	}
}
