/**
	\file obex_main.h
	Implementation of the Object Exchange Protocol OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1998, 1999, 2000 Dag Brattli, All Rights Reserved.
	Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.

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

#ifndef OBEX_MAIN_H
#define OBEX_MAIN_H

#ifdef _WIN32
#include <winsock2.h>
#define socket_t SOCKET

#else /* _WIN32 */
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#define socket_t int
#define INVALID_SOCKET -1

#endif /* _WIN32 */

#include <time.h>

/* Forward decl */
typedef struct obex obex_t;

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

#define	TRUE		1
#define FALSE		0

#define obex_return_if_fail(test)	do { if (!(test)) return; } while(0);
#define obex_return_val_if_fail(test, val)	do { if (!(test)) return val; } while(0);

#include <openobex/obex_const.h>

#include "obex_object.h"
#include "obex_transport.h"
#include "databuffer.h"

#if defined(_MSC_VER) && _MSC_VER < 1400
static void log_debug(char *format, ...) {}
#define log_debug_prefix ""

#elif defined(OBEX_SYSLOG) && !defined(_WIN32)
#include <syslog.h>
#define log_debug(format, ...) syslog(LOG_DEBUG, format, ## __VA_ARGS__)
#define log_debug_prefix "OpenOBEX: "

#else
#include <stdio.h>
#define log_debug(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#define log_debug_prefix ""
#endif

/* use integer:  0 for production
 *               1 for verification
 *              >2 for debug
 */
#if defined(_MSC_VER) && _MSC_VER < 1400
static void DEBUG(int n, char *format, ...) {}

#elif OBEX_DEBUG
extern int obex_debug;
#  define DEBUG(n, format, ...) \
          if (obex_debug >= (n)) \
            log_debug("%s%s(): " format, log_debug_prefix, __FUNCTION__, ## __VA_ARGS__)

#else
#  define DEBUG(n, format, ...)
#endif


/* use bitmask: 0x1 for sendbuff
 *              0x2 for receivebuff
 */
#if OBEX_DUMP
extern int obex_dump;
#define DUMPBUFFER(n, label, msg) \
        if ((obex_dump & 0x3) & (n)) buf_dump(msg, label);
#else
#define DUMPBUFFER(n, label, msg)
#endif


#define OBEX_VERSION		0x10      /* OBEX Protocol Version 1.1 */

// Note that this one is also defined in obex.h
typedef void (*obex_event_t)(obex_t *handle, obex_object_t *obj, int mode, int event, int obex_cmd, int obex_rsp);

#define MODE_SRV	0x80
#define MODE_CLI	0x00

enum
{
	STATE_IDLE,
	STATE_START,
	STATE_SEND,
	STATE_REC,
};

struct obex {
	uint16_t mtu_tx;			/* Maximum OBEX TX packet size */
	uint16_t mtu_rx;			/* Maximum OBEX RX packet size */
	uint16_t mtu_tx_max;		/* Maximum TX we can accept */

	socket_t fd;		/* Socket descriptor */
	socket_t serverfd;
	socket_t writefd;	/* write descriptor - only OBEX_TRANS_FD */
	unsigned int state;

	int keepserver;		/* Keep server alive */
	int filterhint;		/* Filter devices based on hint bits */
	int filterias;		/* Filter devices based on IAS entry */

	buf_t *tx_msg;		/* Reusable transmit message */
	buf_t *rx_msg;		/* Reusable receive message */

	obex_object_t	*object;	/* Current object being transfered */
	obex_event_t	eventcb;	/* Event-callback */

	obex_transport_t trans;		/* Transport being used */
	obex_ctrans_t ctrans;

	obex_interface_t *interfaces;	/* Array of discovered interfaces */
	int interfaces_number;		/* Number of discovered interfaces */

	void * userdata;		/* For user */
};


socket_t obex_create_socket(obex_t *self, int domain);
int obex_delete_socket(obex_t *self, socket_t fd);

void obex_deliver_event(obex_t *self, int event, int cmd, int rsp, int del);
int obex_data_indication(obex_t *self, uint8_t *buf, int buflen);

void obex_response_request(obex_t *self, uint8_t opcode);
int obex_data_request(obex_t *self, buf_t *msg, int opcode);
int obex_cancelrequest(obex_t *self, int nice);

char *obex_response_to_string(int rsp);

#endif
