/**
	\file glib/test-lowlevel.c
	OpenOBEX glib bindings low level test code.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (C) 2005-2006  Marcel Holtmann <marcel@holtmann.org>

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

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#define SOME_TTY "COM4"
#else
#include <termios.h>
#define SOME_TTY "/dev/rfcomm42"
#endif

#include "obex-lowlevel.h"

static int open_device(const char *device)
{
	int fd;
#ifdef _WIN32
	HANDLE h;
	DCB ti;

	h = CreateFile(device,
			GENERIC_READ|GENERIC_WRITE,
			0,NULL,OPEN_EXISTING,0,NULL);
	if (h == INVALID_HANDLE_VALUE)
		return -1;

	//TODO: tcflush-equivalent function?
	ti.StopBits = ONESTOPBIT;
	ti.Parity = NOPARITY;
	ti.ByteSize = 8;
	ti.fNull = FALSE;
	SetCommState(h,&ti);
	fd = _open_osfhandle((intptr_t)h,0);
#else
	struct termios ti;

	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd < 0)
		return fd;

	tcflush(fd, TCIOFLUSH);

	cfmakeraw(&ti);
	tcsetattr(fd, TCSANOW, &ti);
#endif
	return fd;
}

int main(int argc, char *argv[])
{
	obex_t *handle;
	int fd;

	fd = open_device(SOME_TTY);
	if (fd < 0) {
		perror("Can't open device");
		exit(EXIT_FAILURE);
	}

	handle = obex_open(fd, NULL, NULL);

	obex_connect(handle, NULL, 0);
	obex_poll(handle);

	obex_get(handle, NULL, "telecom/devinfo.txt");
	obex_poll(handle);

	obex_disconnect(handle);
	obex_poll(handle);

	obex_close(handle);

	close(fd);

	return 0;
}
