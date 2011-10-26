/**
	\file obex_server.c
	Handle server operations.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1999-2000 Pontus Fuchs, All Rights Reserved.

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

#include "obex_main.h"
#include "obex_header.h"
#include "obex_connect.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*
 * Function obex_server ()
 *
 *    Handle server-operations
 *
 */
int obex_server(obex_t *self, buf_t *msg, int final)
{
	obex_common_hdr_t *request;
	int cmd, ret, deny = 0;
	unsigned int len;

	DEBUG(4, "\n");

	request = (obex_common_hdr_t *) msg->data;
	cmd = request->opcode & ~OBEX_FINAL;
	len = ntohs(request->len);

	switch (self->state & ~MODE_SRV) {
	case STATE_IDLE:
		/* Nothing has been recieved yet, so this is probably a new request */
		DEBUG(4, "STATE_IDLE\n");

		if (self->object) {
			/* What shall we do here? I don't know!*/
			DEBUG(0, "Got a new server-request while already having one!\n");
			obex_response_request(self, OBEX_RSP_INTERNAL_SERVER_ERROR);
			return -1;
		}

		self->object = obex_object_new();

		if (self->object == NULL) {
			DEBUG(1, "Allocation of object failed!\n");
			obex_response_request(self, OBEX_RSP_INTERNAL_SERVER_ERROR);
			return -1;
		}
		/* Remember the initial command of the request.*/
		self->object->cmd = cmd;

		/* Hint app that something is about to come so that
		   the app can deny a PUT-like request early, or
		   set the header-offset */
		obex_deliver_event(self, OBEX_EV_REQHINT, cmd, 0, FALSE);

		/* Some commands needs special treatment (data outside headers) */
		switch (cmd) {
		case OBEX_CMD_CONNECT:
			DEBUG(4, "Got CMD_CONNECT\n");
			/* Connect needs some extra special treatment */
			if (obex_parse_connect_header(self, msg) < 0) {
				obex_response_request(self, OBEX_RSP_BAD_REQUEST);
				obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
				return -1;
			}
			self->object->headeroffset = 4;
			break;
		case OBEX_CMD_SETPATH:
			self->object->headeroffset = 2;
			break;
		}

		self->state = MODE_SRV | STATE_REC;
		/* no break! Fallthrough */

	case STATE_REC:
		DEBUG(4, "STATE_REC\n");
		/* In progress of receiving a request */

		/* Abort? */
		if (cmd == OBEX_CMD_ABORT) {
			DEBUG(1, "Got OBEX_ABORT request!\n");
			obex_response_request(self, OBEX_RSP_SUCCESS);
			self->state = MODE_SRV | STATE_IDLE;
			obex_deliver_event(self, OBEX_EV_ABORT, self->object->opcode, cmd, TRUE);
			/* This is not an Obex error, it is just that the peer
			 * aborted the request, so return 0 - Jean II */
			return 0;
		}

		/* Sanity check */
		if (cmd != self->object->cmd) {
			/* The cmd-field of this packet is not the
			   same as int the first fragment. Bail out! */
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			self->state = MODE_SRV | STATE_IDLE;
			obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, cmd, TRUE);
			return -1;
		}

		/* Get the headers... */
		if (obex_object_receive(self, msg) < 0) {
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			self->state = MODE_SRV | STATE_IDLE;
			obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
			return -1;
		}

		if (!final) {
			/* Let the user decide whether to accept or deny a multi-packet
			 * request by examining all headers in the first packet */
			if (!self->object->checked) {
				obex_deliver_event(self, OBEX_EV_REQCHECK, cmd, 0, FALSE);
				self->object->checked = 1;
			}

			/* Everything except 0x1X and 0x2X means that the user callback
			 * denied the request. In the denied cases treat the last
			 * packet as a final one but don't signal OBEX_EV_REQ */
			switch ((self->object->opcode & ~OBEX_FINAL) & 0xF0) {
				case 0x10:
				case 0x20:
					break;
				default:
					final = 1;
					deny = 1;
					break;
			}
		}

		if (!final) {
			/* As a server, the final bit is always SET- Jean II */
			if (obex_object_send(self, self->object, FALSE, TRUE) < 0) {
				obex_deliver_event(self, OBEX_EV_LINKERR, cmd, 0, TRUE);
				return -1;
			} else
				obex_deliver_event(self, OBEX_EV_PROGRESS, cmd, 0, FALSE);
			break; /* Stay in this state if not final */
		} else if (!self->object->first_packet_sent) {
			DEBUG(4, "We got a request!\n");
			/* More connect-magic woodoo stuff */
			if (cmd == OBEX_CMD_CONNECT)
				obex_insert_connectframe(self, self->object);

			/* Tell the app that a whole request has arrived. While
			   this event is delivered the app should append the
			   headers that should be in the response */
			if (!deny)
				obex_deliver_event(self, OBEX_EV_REQ, cmd, 0, FALSE);
			self->state = MODE_SRV | STATE_SEND;
			len = 3; /* Otherwise sanitycheck later will fail */
		}
		/* Note the conditional fallthrough! */

	case STATE_SEND:
		/* Send back response */
		DEBUG(4, "STATE_SEND\n");

		/* Abort? */
		if (cmd == OBEX_CMD_ABORT) {
			DEBUG(1, "Got OBEX_ABORT request!\n");
			obex_response_request(self, OBEX_RSP_SUCCESS);
			self->state = MODE_SRV | STATE_IDLE;
			obex_deliver_event(self, OBEX_EV_ABORT, self->object->opcode, cmd, TRUE);
			/* This is not an Obex error, it is just that the peer
			 * aborted the request, so return 0 - Jean II */
			return 0;
		}

		if (len > 3) {
			DEBUG(1, "STATE_SEND Didn't expect data from peer (%d)\n", len);
			DUMPBUFFER(4, "unexpected data", msg);
			/* At this point, we are in the middle of sending
			 * our response to the client, and it is still
			 * sending us some data ! This break the whole
			 * Request/Response model of HTTP !
			 * Most often, the client is sending some out of band
			 * progress information for a GET.
			 * This is the way we will handle that :
			 * Save this header in our Rx header list. We can have
			 * duplicated header, so no problem...
			 * The user has already parsed headers, so will most
			 * likely ignore those new headers.
			 * User can check the header in the next EV_PROGRESS,
			 * doing so will hide the header (until reparse).
			 * If not, header can be parsed at 'final'.
			 * Don't send any additional event to the app to not
			 * break compatibility and because app can just check
			 * this condition itself...
			 * No headeroffset needed because 'connect' is
			 * single packet (or we deny it).
			 * Jean II */
			if (cmd == OBEX_CMD_CONNECT ||
					obex_object_receive(self, msg) < 0) {
				obex_response_request(self, OBEX_RSP_BAD_REQUEST);
				self->state = MODE_SRV | STATE_IDLE;
				obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
				return -1;
			}
			obex_deliver_event(self, OBEX_EV_UNEXPECTED, self->object->opcode, 0, FALSE);
			/* Note : we may want to get rid of received header,
			 * however they are mixed with legitimate headers,
			 * and the user may expect to consult them later.
			 * So, leave them here (== overhead). Jean II */
		}

		/* As a server, the final bit is always SET, and the
		 * "real final" packet is distinguish by beeing SUCCESS
		 * instead of CONTINUE.
		 * It doesn't make sense to me, but that's the spec.
		 * See Obex spec v1.2, chapter 3.2, page 21 and 22.
		 * See also example on chapter 7.3, page 47.
		 * So, force the final bit here. - Jean II */
		self->object->continue_received = 1;

		if (self->object->suspend)
			break;

		ret = obex_object_send(self, self->object, TRUE, TRUE);
		if (ret == 0) {
			/* Made some progress */
			obex_deliver_event(self, OBEX_EV_PROGRESS, cmd, 0, FALSE);
			self->object->first_packet_sent = 1;
			self->object->continue_received = 0;
		} else if (ret < 0) {
			/* Error sending response */
			obex_deliver_event(self, OBEX_EV_LINKERR, cmd, 0, TRUE);
			return -1;
		} else {
			/* Response sent! */
			if (cmd == OBEX_CMD_DISCONNECT) {
				DEBUG(2, "CMD_DISCONNECT done. Resetting MTU!\n");
				self->mtu_tx = OBEX_MINIMUM_MTU;
			}
			self->state = MODE_SRV | STATE_IDLE;
			obex_deliver_event(self, OBEX_EV_REQDONE, cmd, 0, TRUE);
		}
		break;

	default:
		DEBUG(0, "Unknown state\n");
		obex_response_request(self, OBEX_RSP_BAD_REQUEST);
		obex_deliver_event(self, OBEX_EV_PARSEERR, cmd, 0, TRUE);
		return -1;
	}
	return 0;
}
