/**
	\file apps/obex_tcp.c
	Do an OBEX PUT over TCP.
	OpenOBEX test applications and sample code.

	Copyright (c) 1999 Dag Brattli, All Rights Reserved.

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

#ifdef _WIN32
#include <winsock2.h>
#define in_addr_t unsigned long
#else

#include <sys/stat.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif /* _WIN32 */

#include <openobex/obex.h>

#include "obex_put_common.h"
#include "obex_io.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define TRUE  1
#define FALSE 0

obex_t *handle = NULL;
volatile int finished = FALSE;

/*
 * Function get_peer_addr (name, peer)
 *
 *    
 *
 */
int get_peer_addr(char *name, struct sockaddr_in *peer) 
{
	struct hostent *host;
	in_addr_t inaddr;
        
	/* Is the address in dotted decimal? */
	if ((inaddr = inet_addr(name)) != INADDR_NONE) {
		memcpy((char *) &peer->sin_addr, (char *) &inaddr,
		      sizeof(inaddr));  
	}
	else {
		if ((host = gethostbyname(name)) == NULL) {
			printf( "Bad host name: ");
			exit(-1);
                }
		memcpy((char *) &peer->sin_addr, host->h_addr,
				host->h_length);
        }
	return 0;
}

/*
 * Function main (argc, )
 *
 *    Starts all the fun!
 *
 */
int main(int argc, char *argv[])
{
	struct sockaddr_in peer;

	obex_object_t *object;
	int ret;

	printf("Send and receive files over TCP OBEX\n");
	if ( ((argc < 3) || (argc > 3)) && (argc != 1) )	{
		printf ("Usage: %s [name] [peer]\n", argv[0]); 
		return -1;
	}

	handle = OBEX_Init(OBEX_TRANS_INET, obex_event, 0);

	if (argc == 1)	{
		printf("Waiting for files\n");
		ret = InOBEX_ServerRegister(handle);
		if(ret < 0) {
                        printf("Cannot listen to socket\n");
			exit(ret);
		}

		while (!finished) {
			ret = OBEX_HandleInput(handle, 10);
			if (ret == 0) {
				printf("Timeout waiting for connection\n");
				break;
			} else if (ret < 0) {
			        printf("Error waiting for connection\n");
				break;
			}
		}
	}
	else {
		/* We are a client */

		get_peer_addr(argv[2], &peer);
		ret = OBEX_TransportConnect(handle, (struct sockaddr *) &peer,
					  sizeof(struct sockaddr_in));

		if (ret < 0) {
			printf("Sorry, unable to connect!\n");
			exit(ret);
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		ret = do_sync_request(handle, object, 0);

		if( (object = build_object_from_file(handle, argv[1], 0)) )	{
			ret = do_sync_request(handle, object, 0);
		}
		else	{
			perror("PUT failed");
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		ret = do_sync_request(handle, object, 0);

		printf("PUT successful\n");
	}
	return 0;
}
