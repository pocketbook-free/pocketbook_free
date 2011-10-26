/**
	\file inobex.h
	InOBEX, Inet transport for OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1999 Dag Brattli, All Rights Reserved.

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

#ifndef INOBEX_H
#define INOBEX_H

void inobex_prepare_connect(obex_t *self, struct sockaddr *saddr, int addrlen);
void inobex_prepare_listen(obex_t *self, struct sockaddr *saddr, int addrlen);
int inobex_listen(obex_t *self);
int inobex_accept(obex_t *self);
int inobex_connect_request(obex_t *self);
int inobex_disconnect_request(obex_t *self);
int inobex_disconnect_server(obex_t *self);

#endif
