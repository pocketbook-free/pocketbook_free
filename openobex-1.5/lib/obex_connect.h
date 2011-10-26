/**
	\file obex_connect.h
	Parse and create connect-command.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

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

#ifndef OBEX_CONNECT_H
#define OBEX_CONNECT_H

#include "obex_main.h"

int obex_insert_connectframe(obex_t *self, obex_object_t *object);
int obex_parse_connect_header(obex_t *self, buf_t *msg);

#endif

