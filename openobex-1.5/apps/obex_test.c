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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_BLUETOOTH
#include "../lib/bluez_compat.h"
#endif

#include <openobex/obex.h>

#include "obex_test.h"
#include "obex_test_client.h"
#include "obex_test_server.h"

#ifndef _WIN32
#include "obex_test_cable.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifndef in_addr_t
#define in_addr_t unsigned long
#endif

#define TRUE  1
#define FALSE 0

#define IR_SERVICE "OBEX"
#define BT_CHANNEL 4

//
// Called by the obex-layer when some event occurs.
//
void obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	switch (event)	{
	case OBEX_EV_PROGRESS:
		printf("Made some progress...\n");
		break;

	case OBEX_EV_ABORT:
		printf("Request aborted!\n");
		break;

	case OBEX_EV_REQDONE:
		if(mode == OBEX_MODE_CLIENT) {
			client_done(handle, object, obex_cmd, obex_rsp);
		}
		else	{
			server_done(handle, object, obex_cmd, obex_rsp);
		}
		break;
	case OBEX_EV_REQHINT:
		/* Accept any command. Not rellay good, but this is a test-program :) */
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;

	case OBEX_EV_REQ:
		server_request(handle, object, event, obex_cmd);
		break;

	case OBEX_EV_LINKERR:
		OBEX_TransportDisconnect(handle);
		printf("Link broken!\n");
		break;

	case OBEX_EV_STREAMEMPTY:
		fillstream(handle, object);
		break;

	default:
		printf("Unknown event %02x!\n", event);
		break;
	}
}

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

//
//
//
int inet_connect(obex_t *handle)
{
	struct sockaddr_in peer;

	get_peer_addr("localhost", &peer);
	return OBEX_TransportConnect(handle, (struct sockaddr *) &peer,
				  sizeof(struct sockaddr_in));
}
	
//
//
//
int main (int argc, char *argv[])
{
	char cmd[10];
	int num, end = 0;
	int cobex = FALSE, tcpobex = FALSE, btobex = FALSE, r320 = FALSE, usbobex = FALSE;
	obex_t *handle;
#ifdef HAVE_BLUETOOTH
	bdaddr_t bdaddr;
	uint8_t channel = 0;
#endif

#ifdef HAVE_USB
	obex_interface_t *obex_intf;
#endif

	struct context global_context = {0,};

#ifndef _WIN32

	char *port;
	obex_ctrans_t custfunc;

	if( (argc == 2 || argc ==3) && (strcmp(argv[1], "-s") == 0 ) )
		cobex = TRUE;
	if( (argc == 2 || argc ==3) && (strcmp(argv[1], "-r") == 0 ) ) {
		cobex = TRUE;
		r320 = TRUE;
	}
#endif

	if( (argc == 2) && (strcmp(argv[1], "-i") == 0 ) )
		tcpobex = TRUE;
	if( (argc >= 2) && (strcmp(argv[1], "-b") == 0 ) )
		btobex = TRUE;
	if( (argc >= 2) && (strcmp(argv[1], "-u") == 0 ) )
		usbobex = TRUE;

	if(cobex)	{
#ifndef _WIN32
		if(argc == 3)
			port = argv[2];
		else
			port = "/dev/ttyS0";

		if(r320)
			printf("OBEX to R320 on %s!\n", port);
		else
			printf("OBEX on %s!\n", port);

		custfunc.customdata = cobex_open(port, r320);

		if(custfunc.customdata == NULL) {
			printf("cobex_open() failed\n");
			return -1;
		}

		if(! (handle = OBEX_Init(OBEX_TRANS_CUSTOM, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			return -1;
		}

		custfunc.connect = cobex_connect;
		custfunc.disconnect = cobex_disconnect;
		custfunc.write = cobex_write;
		custfunc.handleinput = cobex_handle_input;
		custfunc.listen = cobex_connect;	// Listen and connect is 100% same on cable

		if(OBEX_RegisterCTransport(handle, &custfunc) < 0)	{
			printf("Custom transport callback-registration failed\n");
		}
#else
		printf("Not implemented in Win32 yet.\n");
#endif	// _WIN32
	}

	else if(tcpobex) {
		printf("Using TCP transport\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_INET, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}
	}
	else if(btobex) {
#ifndef _WIN32
		switch (argc) {
#ifdef HAVE_BLUETOOTH
		case 4:
			str2ba(argv[2], &bdaddr);
			channel = atoi(argv[3]);
			break;
		case 3:
			str2ba(argv[2], &bdaddr);
			if (bacmp(&bdaddr, BDADDR_ANY) == 0)
				channel = atoi(argv[2]);
			else
				channel = BT_CHANNEL;
			break;
		case 2:
			bacpy(&bdaddr, BDADDR_ANY);
			channel = BT_CHANNEL;
			break;
#endif
		default:
			printf("Wrong number of arguments\n");
			exit(0);
		}

		printf("Using Bluetooth RFCOMM transport\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_BLUETOOTH, obex_event, 0)))      {
			perror( "OBEX_Init failed");
			exit(0);
		}
#else
		printf("Not implemented in Win32 yet.\n");
#endif	// _WIN32
	}
	else if(usbobex) {
#ifdef HAVE_USB
		int i, interfaces_number, intf_num;
		switch (argc) {
		case 2:
			printf("Using USB transport, querying available interfaces\n");
			if(! (handle = OBEX_Init(OBEX_TRANS_USB, obex_event, 0)))      {
				perror( "OBEX_Init failed");
				exit(0);
			}
			interfaces_number = OBEX_FindInterfaces(handle, &obex_intf);
			for (i=0; i < interfaces_number; i++)
				printf("Interface %d: %s %s %s\n", i, 
					obex_intf[i].usb.manufacturer, 
					obex_intf[i].usb.product, 
					obex_intf[i].usb.control_interface);
			printf("Use '%s -u interface_number' to run interactive OBEX test client\n", argv[0]);
			OBEX_Cleanup(handle);
			exit(0);
			break;
		case 3:
			intf_num = atoi(argv[2]);
			printf("Using USB transport \n");
			if(! (handle = OBEX_Init(OBEX_TRANS_USB, obex_event, 0)))      {
				perror( "OBEX_Init failed");
				exit(0);
			}

			interfaces_number = OBEX_FindInterfaces(handle, &obex_intf);
			if (intf_num >= interfaces_number) {
				printf( "Invalid interface number\n");
				exit(0);
			}
			obex_intf += intf_num;	

			break;
		default:
			printf("Wrong number of arguments\n");
			exit(0);
		}
#else
		printf("Not compiled with USB support\n");
		exit(0);
#endif
	}
	else	{
		printf("Using IrDA transport\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}
	}

	OBEX_SetUserData(handle, &global_context);
	
	printf( "OBEX Interactive test client/server.\n");

	while (!end) {
		printf("> ");
		num = scanf("%s", cmd);
		switch (cmd[0] | 0x20)	{
			case 'q':
				end=1;
			break;
			case 'g':
				get_client(handle, &global_context);
			break;
			case 't':
				setpath_client(handle);
			break;
			case 'p':
				put_client(handle);
			break;
			case 'x':
				push_client(handle);
			break;
			case 'c':
				/* First connect transport */
				if(tcpobex) {
					if(TcpOBEX_TransportConnect(handle, NULL, 0) < 0) {
						printf("Transport connect error! (TCP)\n");
						break;
					}
				}
				if(cobex) {
					if(OBEX_TransportConnect(handle, (void*) 1, 0) < 0) {
						printf("Transport connect error! (Serial)\n");
						break;
					}
				}
				if(btobex) {
#ifdef HAVE_BLUETOOTH
					if (bacmp(&bdaddr, BDADDR_ANY) == 0) {
						printf("Device address error! (Bluetooth)\n");
						break;
					}
					if(BtOBEX_TransportConnect(handle, BDADDR_ANY, &bdaddr, channel) <0) {
						printf("Transport connect error! (Bluetooth)\n");
						break;
					}
#else
					printf("Transport not found! (Bluetooth)\n");
#endif
				}
				if (usbobex) {
#ifdef HAVE_USB
					if (OBEX_InterfaceConnect(handle, obex_intf) < 0) {
						printf("Transport connect error! (USB)\n");
						break;
					}
#else
					printf("Transport not found! (USB)\n");
#endif
				}	
				if (!tcpobex && !cobex && !btobex && !usbobex) {
					if(IrOBEX_TransportConnect(handle, IR_SERVICE) < 0) {
						printf("Transport connect error! (IrDA)\n");
						break;
					}
				}
				// Now send OBEX-connect.	
				connect_client(handle);
			break;
			case 'd':
				disconnect_client(handle);
			break;
			case 's':
				/* First register server */
				if(tcpobex) {
					if(TcpOBEX_ServerRegister(handle, NULL, 0) < 0) {
						printf("Server register error! (TCP)\n");
						break;
					}
				}
				if(cobex) {
					if(OBEX_ServerRegister(handle, (void*) 1, 0) < 0) {
						printf("Server register error! (Serial)\n");
						break;
					}
				}
				if(btobex) {
#ifdef HAVE_BLUETOOTH
					if(BtOBEX_ServerRegister(handle, BDADDR_ANY, channel) < 0) {
						printf("Server register error! (Bluetooth)\n");
						break;
					}
#else
					printf("Transport not found! (Bluetooth)\n");
#endif
				}
				if (usbobex) {
					printf("Transport not found! (USB)\n");
				}
				if (!tcpobex && !cobex && !btobex && !usbobex) {
					if(IrOBEX_ServerRegister(handle, IR_SERVICE) < 0)	{
						printf("Server register error! (IrDA)\n");
						break;
					}
				}
				/* No process server events */
				server_do(handle);
			break;
			default:
				printf("Unknown command %s\n", cmd);
		}
	}
#ifndef _WIN32
	if(cobex)
		cobex_close(custfunc.customdata);
#endif

	return 0;
}
