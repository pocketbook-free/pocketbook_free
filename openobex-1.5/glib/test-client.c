/**
 	\file glib/test-client.c
	OpenOBEX glib bindings client test code.
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
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <termios.h>
#endif

#include "obex-client.h"

#define FTP_UUID (guchar *) \
	"\xF9\xEC\x7B\xC4\x95\x3C\x11\xD2\x98\x4E\x52\x54\x00\xDC\x9E\x09"

static GMainLoop *mainloop;

static void sig_term(int sig)
{
	g_main_loop_quit(mainloop);
}

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

static void transfer(ObexClient *client, ObexClientCondition cond, gpointer data)
{
	GError *gerr = NULL;
	int err, *io = data;

	if (cond & OBEX_CLIENT_COND_IN) {
		gchar buf[1024];
		gsize len;

		printf("OBEX_CLIENT_COND_IN\n");

		do {
			obex_client_read(client, buf, sizeof(buf), &len, &gerr);

			if (gerr != NULL) {
				printf("obex_client_read failed: %s\n", gerr->message);
				g_error_free(gerr);
				gerr = NULL;
				break;
			}

			printf("Data buffer with size %zd available\n", len);

			if (len > 0 && *io >= 0)
				err = write(*io, buf, len);	

		} while (len == sizeof(buf));
	}

	if (cond & OBEX_CLIENT_COND_OUT) {
		char buf[10000];
		ssize_t actual;

		printf("OBEX_CLIENT_COND_OUT\n");

		if (*io < 0) {
			printf("No data to send!\n");
			return;
		}

		actual = read(*io, buf, sizeof(buf));
		if (actual == 0) {
			obex_client_close(client, &gerr);
			if (gerr != NULL) {
				printf("obex_client_close failed: %s\n",
						gerr->message);
				g_error_free(gerr);
				gerr = NULL;
			}
			close(*io);
			*io = -1;
		} else if (actual > 0) {
			gsize written;

			obex_client_write(client, buf, actual, &written, &gerr);

			if (gerr != NULL) {
				printf("writing data failed: %s\n", gerr->message);
				g_error_free(gerr);
				gerr = NULL;
			} else if (written < actual)
				printf("Only %zd/%zd bytes were accepted by obex_client_write!\n",
						written, actual);
			else
				obex_client_flush(client, NULL);

		}
		else
			fprintf(stderr, "read: %s\n", strerror(errno));
	}
	
	if (cond & OBEX_CLIENT_COND_DONE) {
		obex_client_get_error(client, &gerr);
		if (gerr != NULL) {
			printf("Operation failed: %s\n", gerr->message);
			g_error_free(gerr);
			gerr = NULL;
		}
		else
			printf("Operation completed with success\n");
	}
		
	if (cond & OBEX_CLIENT_COND_ERR)
		printf("Error in transfer\n");
}

int main(int argc, char *argv[])
{
	ObexClient *client;
	int fd, io = -1;
#ifndef _WIN32
	struct sigaction sa;
#endif

	g_type_init();

	fd = open_device("/dev/rfcomm42");
	if (fd < 0) {
		perror("Can't open device");
		exit(EXIT_FAILURE);
	}

	mainloop = g_main_loop_new(NULL, FALSE);

	client = obex_client_new();

	obex_client_add_watch(client, 0, transfer, &io);

	obex_client_attach_fd(client, fd, NULL);

	if (argc > 1) {
		struct stat s;

		if (stat(argv[1], &s) < 0) {
			fprintf(stderr, "stat(%s): %s\n", argv[1], strerror(errno));
			return 1;
		}

		io = open(argv[1], O_RDONLY);
		if (io < 0) {
			fprintf(stderr, "open(%s): %s\n", argv[1], strerror(errno));
			return 1;
		}

		obex_client_put_object(client, NULL, argv[1], s.st_size, s.st_mtime, NULL);
	}
	else
		obex_client_get_object(client, NULL, "telecom/devinfo.txt", NULL);


#ifndef _WIN32
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
#endif

	g_main_loop_run(mainloop);

	obex_client_destroy(client);

	if (io >= 0)
		close(io);

	close(fd);

	return 0;
}
