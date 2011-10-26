/**
	\file glib/obex-lowlevel.h
	OpenOBEX glib bindings low level code.
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

#include <openobex/obex.h>

#ifdef OBEX_CLIENT
#undef OBEX_CLIENT
#endif

typedef struct {
	void (*connect_cfm)(obex_t *handle, void *data);
	void (*disconn_ind)(obex_t *handle, void *data);
	void (*progress_ind)(obex_t *handle, void *data);
	void (*command_ind)(obex_t *handle, int event, void *data);
} obex_callback_t;

obex_t *obex_open(int fd, obex_callback_t *callback, void *data);
void obex_close(obex_t *handle);
void obex_poll(obex_t *handle);
int obex_connect(obex_t *handle, const unsigned char *target, size_t size);
int obex_disconnect(obex_t *handle);
int obex_put(obex_t *handle, const char *type, const char *name, int size, time_t mtime);
int obex_get(obex_t *handle, const char *type, const char *name);
int obex_write(obex_t *handle, const char *buf, size_t count, size_t *bytes_written);
int obex_read(obex_t *handle, char *buf, size_t count, size_t *bytes_read);
int obex_abort(obex_t *handle);
int obex_flush(obex_t *handle);
int obex_close_transfer(obex_t *handle);
int obex_get_response(obex_t *handle);
void obex_do_callback(obex_t *handle);
int obex_setpath(obex_t *handle, const char *path, int create);
int obex_delete(obex_t *handle, const char *name);
