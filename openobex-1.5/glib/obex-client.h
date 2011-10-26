/**
	\file glib/obex-client.h
	OpenOBEX glib bindings client code.
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

#ifndef __OBEX_CLIENT_H
#define __OBEX_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

GQuark obex_error_quark(void);

#define OBEX_ERROR obex_error_quark()

enum {
	OBEX_ERROR_NO_MEMORY,
	OBEX_ERROR_BUSY,
	OBEX_ERROR_INVALID_PARAMS,
	OBEX_ERROR_ALREADY_CONNECTED,
	OBEX_ERROR_NOT_CONNECTED,
	OBEX_ERROR_LINK_FAILURE,
	OBEX_ERROR_ABORTED,

	/* Errors derived from OBEX response codes */
	OBEX_ERROR_BAD_REQUEST,
	OBEX_ERROR_UNAUTHORIZED,
	OBEX_ERROR_PAYMENT_REQUIRED,
	OBEX_ERROR_FORBIDDEN,
	OBEX_ERROR_NOT_FOUND,
	OBEX_ERROR_NOT_ALLOWED,
	OBEX_ERROR_NOT_ACCEPTABLE,
	OBEX_ERROR_PROXY_AUTH_REQUIRED,
	OBEX_ERROR_REQUEST_TIME_OUT,
	OBEX_ERROR_CONFLICT,
	OBEX_ERROR_GONE,
	OBEX_ERROR_LENGTH_REQUIRED,
	OBEX_ERROR_PRECONDITION_FAILED,
	OBEX_ERROR_ENTITY_TOO_LARGE,
	OBEX_ERROR_URL_TOO_LARGE,
	OBEX_ERROR_UNSUPPORTED_MEDIA_TYPE,
	OBEX_ERROR_INTERNAL_SERVER_ERROR,
	OBEX_ERROR_NOT_IMPLEMENTED,
	OBEX_ERROR_BAD_GATEWAY,
	OBEX_ERROR_SERVICE_UNAVAILABLE,
	OBEX_ERROR_GATEWAY_TIMEOUT,
	OBEX_ERROR_VERSION_NOT_SUPPORTED,
	OBEX_ERROR_DATABASE_FULL,
	OBEX_ERROR_DATABASE_LOCKED,

	OBEX_ERROR_FAILED
};

#define OBEX_TYPE_CLIENT (obex_client_get_type())
#define OBEX_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					OBEX_TYPE_CLIENT, ObexClient))
#define OBEX_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					OBEX_TYPE_CLIENT, ObexClientClass))
#define OBEX_IS_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							OBEX_TYPE_CLIENT))
#define OBEX_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							OBEX_TYPE_CLIENT))
#define OBEX_GET_CLIENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					OBEX_TYPE_CLIENT, ObexClientClass))

typedef struct _ObexClient ObexClient;
typedef struct _ObexClientClass ObexClientClass;

struct _ObexClient {
	GObject parent;
};

struct _ObexClientClass {
	GObjectClass parent_class;

	void (*connected)(ObexClient *self);
	void (*disconnect)(ObexClient *self);
	void (*canceled)(ObexClient *self);
	void (*progress)(ObexClient *self);
	void (*idle)(ObexClient *self);
};

GType obex_client_get_type(void);

ObexClient *obex_client_new(void);

void obex_client_destroy(ObexClient *self);

void obex_client_set_auto_connect(ObexClient *self, gboolean auto_connect);

gboolean obex_client_get_auto_connect(ObexClient *self);

typedef enum {
	OBEX_CLIENT_COND_IN	= 1 << 0,
	OBEX_CLIENT_COND_OUT 	= 1 << 1,
	OBEX_CLIENT_COND_DONE 	= 1 << 2,
	OBEX_CLIENT_COND_ERR 	= 1 << 3,
} ObexClientCondition;

typedef void (*ObexClientFunc)(ObexClient *client,
				ObexClientCondition condition, gpointer data);

void obex_client_add_watch(ObexClient *self, ObexClientCondition condition,
				ObexClientFunc func, gpointer data);

void obex_client_add_watch_full(ObexClient *self,
			ObexClientCondition condition, ObexClientFunc func,
					gpointer data, GDestroyNotify notify);

gboolean obex_client_attach_fd(ObexClient *self, int fd, GError **error);

gboolean obex_client_connect(ObexClient *self, const guchar *target,
						gsize size, GError **error);

gboolean obex_client_disconnect(ObexClient *self, GError **error);

gboolean obex_client_put_object(ObexClient *self, const gchar *type,
					const gchar *name, gint size,
					time_t mtime, GError **error);

gboolean obex_client_get_object(ObexClient *self, const gchar *type,
					const gchar *name, GError **error);

gboolean obex_client_read(ObexClient *self, gchar *buf, gsize count,
					gsize *bytes_read, GError **error);

gboolean obex_client_write(ObexClient *self, const gchar *buf, gsize count,
					gsize *bytes_written, GError **error);

gboolean obex_client_flush(ObexClient *self, GError **error);

gboolean obex_client_abort(ObexClient *self, GError **error);

gboolean obex_client_close(ObexClient *self, GError **error);

gboolean obex_client_get_error(ObexClient *self, GError **error);

gboolean obex_client_mkdir(ObexClient *self, const gchar *path, GError **error);

gboolean obex_client_chdir(ObexClient *self, const gchar *path, GError **error);

gboolean obex_client_delete(ObexClient *self, const gchar *name, GError **error);

G_END_DECLS

#endif /* __OBEX_CLIENT_H */
