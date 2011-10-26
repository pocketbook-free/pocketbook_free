/**
	\file glib/obex-debug.h
	OpenOBEX glib bindings debug code.
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

#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER < 1400
static void debug (char *fmt, ...) {}
#else
#define debug(fmt, ...) fprintf(stderr, "== %s: " fmt "\n" , __FUNCTION__ , ## __VA_ARGS__)
#endif
