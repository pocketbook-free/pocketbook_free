/**
	\file btobex.h
	Bluetooth OBEX, Bluetooth transport for OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

 	Copyright (c) 2002 Marcel Holtmann, All Rights Reserved.

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

#ifndef BTOBEX_H
#define BTOBEX_H

#include "bluez_compat.h"

void btobex_prepare_connect(obex_t *self, bdaddr_t *src, bdaddr_t *dst, uint8_t channel);
void btobex_prepare_listen(obex_t *self, bdaddr_t *src, uint8_t channel);
int btobex_listen(obex_t *self);
int btobex_connect_request(obex_t *self);
int btobex_disconnect_request(obex_t *self);
int btobex_accept(obex_t *self);
int btobex_disconnect_server(obex_t *self);

#endif
