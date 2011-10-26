/**
	\file glib/obex-error.h
	OpenOBEX glib bindings error handling code.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

 	Copyright (C) 2005-2006  Marcel Holtmann <marcel@holtmann.org>

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

#ifndef __OBEX_ERROR_H
#define __OBEX_ERROR_H

#include <glib.h>

void err2gerror(int err, GError **gerr);

void rsp2gerror(int rsp, GError **gerr);

#endif /* __OBEX_ERROR_H */
