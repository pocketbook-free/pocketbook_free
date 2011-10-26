/**
	\file irobex.h
	IrOBEX, IrDA transport for OBEX.
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

#ifndef IROBEX_H
#define IROBEX_H

#define MAX_DEVICES 10     /* Max devices to discover */

void irobex_prepare_connect(obex_t *self, const char *service);
void irobex_prepare_listen(obex_t *self, const char *service);
int irobex_listen(obex_t *self);
int irobex_connect_request(obex_t *self);
int irobex_disconnect_request(obex_t *self);
int irobex_accept(obex_t *self);
int irobex_disconnect_server(obex_t *self);

#endif
