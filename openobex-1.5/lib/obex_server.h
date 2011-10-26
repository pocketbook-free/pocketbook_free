/**
	\file obex_server.h
	Handle server operations.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1999-2000 Pontus Fuchs, All Rights Reserved.

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

#ifndef OBEX_SERVER_H
#define OBEX_SERVER_H

int obex_server(obex_t *self, buf_t *msg, int final);

#endif
