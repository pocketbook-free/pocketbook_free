/**
	\file apps/obex_test_cable.h
	OBEX over a serial port in Linux; Can be used with an Ericsson R320s phone.
	OpenOBEX test applications and sample code.

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

#ifndef OBEX_TEST_CABLE_H
#define OBEX_TEST_CABLE_H

#ifdef _WIN32
#include <windows.h>
#define tty_desc_t DCB
#else
#include <termios.h>
#define tty_desc_t struct termios
#endif

#if defined(_MSC_VER) && _MSC_VER < 1400
static void CDEBUG(char *format, ...) {}

#elif defined(CABLE_DEBUG)
#define CDEBUG(format, ...) printf("%s(): " format, __FUNCTION__ , ## __VA_ARGS__)

#else
#define CDEBUG(format, ...)
#endif

struct cobex_context
{
	const char *portname;
	int ttyfd;
	char inputbuf[500];
	tty_desc_t oldtio, newtio;
	int r320;
};

/* User function */
struct cobex_context *cobex_open(const char *port, int r320);
void cobex_close(struct cobex_context *gt);
int cobex_do_at_cmd(struct cobex_context *gt, char *cmd, char *rspbuf, int rspbuflen, int timeout);

/* Callbacks */
int cobex_handle_input(obex_t *handle, void * userdata, int timeout);
int cobex_write(obex_t *self, void * userdata, uint8_t *buffer, int length);
int cobex_connect(obex_t *handle, void * userdata);
int cobex_disconnect(obex_t *handle, void * userdata);

#endif
