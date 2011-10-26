/**
	\file apps/obex_test_cable.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openobex/obex.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#define read _read
#define write _write
#define strcasecmp(a,b) _stricmp(a,b)
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include "obex_test_cable.h"

#define TRUE  1
#define FALSE 0

static void cobex_cleanup(struct cobex_context *gt, int force);

/*#define TTBT "AT*TTBT=0001EC4AF6E7\r"*/

//
// Send an AT-command and return back one line of answer if any.
// To read a line without sending anything set cmd as NULL
// (this function should be rewritten!)
//
int cobex_do_at_cmd(struct cobex_context *gt, char *cmd, char *rspbuf, int rspbuflen, int timeout)
{
#if defined(_WIN32)
#if ! defined(_MSC_VER)
#warning "Implementation for win32 is missing!"
#endif
	return -1;
#else
	fd_set ttyset;
	struct timeval tv;
	int fd;
	
	char *answer = NULL;
	char *answer_end = NULL;
	unsigned int answer_size;

	char tmpbuf[100] = {0,};
	int actual;
	int total = 0;
	int done = 0;
	
	CDEBUG("\n");
	
	fd = gt->ttyfd;
	rspbuf[0] = 0;
	if(fd < 0)
		return -1;

	if(cmd != NULL) {
		// Write command
		int cmdlen;
		
		cmdlen = strlen(cmd);
		CDEBUG("Sending command %s\n", cmd);
		if(write(fd, cmd, cmdlen) < cmdlen)	{
			perror("Error writing to port");
			return -1;
		}
	}
	
	while(!done)	{
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
			if(actual < 0)
				return actual;
			total += actual;

//			printf("tmpbuf=%d: %s\n", total, tmpbuf);

			/* Answer didn't come within the length of the buffer. Cancel! */
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = index(tmpbuf, '\n')) )	{
				// Remove first line (echo)
				if( (answer_end = index(answer+1, '\n')) )	{
					// Found end of answer
					done = 1;
				}
			}
		}
		else	{
			/* Anser didn't come in time. Cancel */
			CDEBUG("Timeout waiting for answer\n");
			return -1;
		}
	}


//	printf("buf:%08lx answer:%08lx end:%08lx\n", tmpbuf, answer, answer_end);


//	printf("Answer: %s\n", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
//	printf("Answer: %s\n", answer);

	answer_size = (answer_end) - answer +1;

//	printf("Answer size=%d\n", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
#endif
}

//
// Open serial port and if we are using an r320, set it in OBEX-mode.
//
static int cobex_init(struct cobex_context *gt)
{
#ifdef _WIN32
#if ! defined(_MSC_VER)
#warning "Implementation for win32 is missing!"
#endif
#else
	char rspbuf[200];

	CDEBUG("\n");

	if( (gt->ttyfd = open(gt->portname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 )	{
		perror("Can' t open tty");
		return -1;
	}

	tcgetattr(gt->ttyfd, &gt->oldtio);
	bzero(&gt->newtio, sizeof(struct termios));
	gt->newtio.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;
	gt->newtio.c_iflag = IGNPAR;
	gt->newtio.c_oflag = 0;
	tcflush(gt->ttyfd, TCIFLUSH);
	tcsetattr(gt->ttyfd, TCSANOW, &gt->newtio);

	// If we don't speak to an R320s we are happy here.
	if(!gt->r320)
		return 1;

	// Set up R320s phone in OBEX mode.
	if(cobex_do_at_cmd(gt, "ATZ\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
		printf("Comm-error sending ATZ\n");
		goto err;
	}

#ifdef TTBT
	/* Special BT-mode */
	if(cobex_do_at_cmd(gt, TTBT, rspbuf, sizeof(rspbuf), 10) < 0) {
		printf("Comm-error sending AT*TTBT\n");
		goto err;
	}

	if(strcasecmp("OK", rspbuf) != 0)       {
		printf("Error doing AT*TTBT (%s)\n", rspbuf);
		goto err;
	}
#endif
	
	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(gt, "AT*EOBEX\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
		printf("Comm-error sending AT*EOBEX\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		printf("Error doing AT*EOBEX (%s)\n", rspbuf);
		goto err;
	}
		return 1;
err:
	cobex_cleanup(gt, TRUE);
#endif
	return -1;
}

//
// Close down. If force is TRUE. Try to break out of OBEX-mode.
//
static void cobex_cleanup(struct cobex_context *gt, int force)
{
#ifndef _WIN32
	if(force)	{
		// Send a break to get out of OBEX-mode
#ifdef TCSBRKP
		if(ioctl(gt->ttyfd, TCSBRKP, 0) < 0) {
#elif defined(TCSBRK)
		if(ioctl(gt->ttyfd, TCSBRK, 0) < 0) {
#else
		if(tcsendbreak(gt->ttyfd, 0) < 0) {
#endif /* TCSBRKP */
			printf("Unable to send break!\n");
		}
	}
	close(gt->ttyfd);
	gt->ttyfd = -1;
#endif
}

//
// Open up cable OBEX
//
struct cobex_context * cobex_open(const char *port, int r320)
{
	struct cobex_context *gt;
	
	CDEBUG("\n");
	gt = malloc(sizeof(struct cobex_context));
	if (gt == NULL)
		return NULL;
	memset(gt, 0, sizeof(struct cobex_context));
	
	gt->ttyfd = -1;
	gt->portname = port;
	gt->r320 = r320;
	return gt;
}

//
// Close down cable OBEX.
//
void cobex_close(struct cobex_context *gt)
{
	free(gt);
}
	

//
// Do transport connect or listen
//
int cobex_connect(obex_t *handle, void * userdata)
{
	struct cobex_context *gt;

	CDEBUG("\n");
	
	gt = userdata;

	if(gt->ttyfd >= 0)	{
		CDEBUG("fd already exist. Using it\n");
		return 1;

	}
	if(cobex_init(gt) < 0)
		return -1;
	return 1;
}

//
// Do transport disconnect
//
int cobex_disconnect(obex_t *handle, void * userdata)
{
	char rspbuf[20];
	struct cobex_context *gt;

	CDEBUG("\n");
	gt = userdata;

	/* The R320 will send back OK after OBEX disconnect */
	if(gt->r320) {
		CDEBUG("R320!!!\n");
		if(cobex_do_at_cmd(gt, NULL, rspbuf, sizeof(rspbuf), 1) < 0)
			printf("Comm-error waiting for OK after disconnect\n");
		else if(strcasecmp(rspbuf, "OK") != 0)
			printf("Excpected OK after OBEX diconnect got %s\n", rspbuf);
	
#ifdef TTBT
		sleep(2);
		if(cobex_do_at_cmd(gt, "---", rspbuf, sizeof(rspbuf), 5) < 0)
			printf("Comm-error Sending ---\n");
		else if(strcasecmp(rspbuf, "DISCONNECT") != 0)
			 printf("Error waiting for DISCONNECT (%s)\n", rspbuf);
		sleep(2);
#endif

	}
	
	cobex_cleanup(gt, FALSE);
	return 1;
}

//
//  Called when data needs to be written
//
int cobex_write(obex_t *handle, void * userdata, uint8_t *buffer, int length)
{
	struct cobex_context *gt;
	int actual;

	CDEBUG("\n");
	gt = userdata;
	actual = write(gt->ttyfd, buffer, length);
	CDEBUG("Wrote %d bytes (expected %d)\n", actual, length);
	return actual;
}

//
// Called when more data is needed.
//
int cobex_handle_input(obex_t *handle, void * userdata, int timeout)
{
	int actual;
	struct cobex_context *gt;
	struct timeval time;
	fd_set fdset;
        int ret;
	
	CDEBUG("\n");

	gt = userdata;
	
	/* Return if no fd */
	if(gt->ttyfd < 0) {
		CDEBUG("No fd!");
		return -1;
	}

	time.tv_sec = timeout;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(gt->ttyfd, &fdset);

	ret = select(gt->ttyfd+1, &fdset, NULL, NULL, &time);
	
	/* Check if this is a timeout (0) or error (-1) */
	if (ret < 1) {
		return ret;
		CDEBUG("Timeout or error (%d)\n", ret);
	}
	actual = read(gt->ttyfd, &gt->inputbuf, sizeof(gt->inputbuf));
	if(actual <= 0)
		return actual;
	CDEBUG("Read %d bytes\n", actual);
#ifdef CABLE_DEBUG
	{
		int i = 0;
		for(i=0;i<actual;i++) {
			printf("[%0X",gt->inputbuf[i]);
			if(gt->inputbuf[i] >= 32) {
				printf(",%c",gt->inputbuf[i]);
			}
			printf("]");
		}
		printf("\n");
	}
#endif
	OBEX_CustomDataFeed(handle, (uint8_t *) gt->inputbuf, actual);
	return actual;
}
