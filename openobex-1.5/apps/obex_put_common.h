/**
	\file apps/obex_put_common.h
	Utility-functions to act as a PUT server and client.
	OpenOBEX test applications and sample code.

	Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.

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

#ifndef OBEX_PUT_COMMON_H
#define OBEX_PUT_COMMON_H

int do_sync_request(obex_t *handle, obex_object_t *object, int async);
void obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp);

#endif
