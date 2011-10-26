/**
	\file glib/obex-error.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#define EISCONN WSAEISCONN
#define ENOTCONN WSAENOTCONN
#endif

#include <errno.h>

#include "obex-debug.h"
#include "obex-client.h"
#include "obex-error.h"

GQuark obex_error_quark(void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string("obex-error-quark");

	return q;
}

void err2gerror(int err, GError **gerr)
{
	debug("%s", g_strerror(err));

	if (!gerr)
		return;

	switch (err) {
	case ENOMEM:
            g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NO_MEMORY,
			    "Out of memory");
	    break;

	case EBUSY:
            g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_BUSY,
			    "Busy performing another operation");
	    break;

	case EINVAL:
	    g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_INVALID_PARAMS,
			    "Invalid parameters were given");
	    break;

	case EISCONN:
	    g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_ALREADY_CONNECTED,
			    "Already connected");
	    break;

	case ENOTCONN:
	    g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NOT_CONNECTED,
			    "Not connected");
	    break;

	case EINTR:
	    g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_ABORTED,
			    "The operation was aborted");
	    break;

	default:
	    g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_FAILED,
			    "Failed");
	    break;
	}
}

void rsp2gerror(int rsp, GError **gerr)
{
	if (!gerr)
		return;

	switch (rsp) {
	case 0x40:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_BAD_REQUEST,
				"Bad Request - server couldn't understand request");
		break;

	case 0x41:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_UNAUTHORIZED,
				"Unauthorized");
		break;

	case 0x42:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_PAYMENT_REQUIRED,
				"Payment required");
		break;

	case 0x43:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_FORBIDDEN,
				"Forbidden - operation is understood but refused");
		break;

	case 0x44:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NOT_FOUND,
				"Not Found");
		break;

	case 0x45:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NOT_ALLOWED,
				"Method not allowed");
		break;

	case 0x46:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NOT_ACCEPTABLE,
				"Not Acceptable");
		break;

	case 0x47:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_PROXY_AUTH_REQUIRED,
				"Proxy Authentication required");
		break;
		
	case 0x48:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_REQUEST_TIME_OUT,
				"Request Time Out");
		break;
		
	case 0x49:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_CONFLICT,
				"Conflict");
		break;

	case 0x4A:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_GONE,
				"Gone");
		break;

	case 0x4B:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_LENGTH_REQUIRED,
				"Length Required");
		break;

	case 0x4C:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_PRECONDITION_FAILED,
				"Precondition failed");
		break;

	case 0x4D:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_ENTITY_TOO_LARGE,
				"Requested entity too large");
		break;

	case 0x4E:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_URL_TOO_LARGE,
				"Request URL too large");
		break;

	case 0x4F:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_UNSUPPORTED_MEDIA_TYPE,
				"Unsupported media type");
		break;

	case 0x50:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_INTERNAL_SERVER_ERROR,
				"Internal Server Error");
		break;

	case 0x51:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_NOT_IMPLEMENTED,
				"Not Implemented");
		break;

	case 0x52:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_BAD_GATEWAY,
				"Bad Gateway");
		break;

	case 0x53:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_SERVICE_UNAVAILABLE,
				"Service Unavailable");
		break;

	case 0x54:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_GATEWAY_TIMEOUT,
				"Gateway Timeout");
		break;

	case 0x55:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_VERSION_NOT_SUPPORTED,
				"HTTP version not supported");
		break;

	case 0x60:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_DATABASE_FULL,
				"Database Full");
		break;

	case 0x61:
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_DATABASE_LOCKED,
				"Database Locked");
		break;

	default:
		debug("Unmapped OBEX response: %d", rsp);
		g_set_error(gerr, OBEX_ERROR, OBEX_ERROR_FAILED,
				"Failed");
		break;
	}
}

