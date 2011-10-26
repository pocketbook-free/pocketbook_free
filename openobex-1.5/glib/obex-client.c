/**
	\file glib/obex-client.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>

#include "obex-debug.h"
#include "obex-lowlevel.h"
#include "obex-error.h"
#include "obex-marshal.h"
#include "obex-client.h"

#define OBEX_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
					OBEX_TYPE_CLIENT, ObexClientPrivate))

typedef struct _ObexClientPrivate ObexClientPrivate;

struct _ObexClientPrivate {
	gboolean auto_connect;

#ifdef G_THREADS_ENABLED
	GMutex *mutex;
#endif

	GMainContext *context;
	GIOChannel *channel;
	obex_t *handle;

	gpointer watch_data;
	ObexClientFunc watch_func;
	GDestroyNotify watch_destroy;

	GSource *idle_source;

	gboolean connected;
};

G_DEFINE_TYPE(ObexClient, obex_client, G_TYPE_OBJECT)

static void obex_client_init(ObexClient *self)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

#ifdef G_THREADS_ENABLED
	priv->mutex = g_mutex_new();
#endif

	priv->auto_connect = TRUE;

	priv->context = g_main_context_default();
	g_main_context_ref(priv->context);

	priv->connected = FALSE;
}

static void obex_client_finalize(GObject *object)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	if (priv->connected == TRUE && priv->auto_connect == TRUE)
		obex_disconnect(priv->handle);

	if (priv->idle_source) {
		g_source_destroy(priv->idle_source);
		priv->idle_source = NULL;
	}

	obex_close(priv->handle);
	priv->handle = NULL;

	g_main_context_unref(priv->context);
	priv->context = NULL;

#ifdef G_THREADS_ENABLED
	g_mutex_free(priv->mutex);
#endif
}

enum {
	PROP_0,
	PROP_AUTO_CONNECT
};

static void obex_client_set_property(GObject *object, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_AUTO_CONNECT:
		priv->auto_connect = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void obex_client_get_property(GObject *object, guint prop_id,
					GValue *value, GParamSpec *pspec)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_AUTO_CONNECT:
		g_value_set_boolean(value, priv->auto_connect);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

enum {
	CONNECTED_SIGNAL,
	DISCONNECT_SIGNAL,
	CANCELED_SIGNAL,
	PROGRESS_SIGNAL,
	IDLE_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void obex_client_class_init(ObexClientClass *klass)
{
#ifdef G_THREADS_ENABLED
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif

	g_type_class_add_private(klass, sizeof(ObexClientPrivate));

	G_OBJECT_CLASS(klass)->finalize = obex_client_finalize;

	G_OBJECT_CLASS(klass)->set_property = obex_client_set_property;
	G_OBJECT_CLASS(klass)->get_property = obex_client_get_property;

	g_object_class_install_property(G_OBJECT_CLASS(klass),
			PROP_AUTO_CONNECT, g_param_spec_boolean("auto-connect",
					NULL, NULL, TRUE, G_PARAM_READWRITE));

	signals[CONNECTED_SIGNAL] = g_signal_new("connected",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, connected),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[DISCONNECT_SIGNAL] = g_signal_new("disconnect",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, disconnect),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CANCELED_SIGNAL] = g_signal_new("canceled",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, canceled),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[PROGRESS_SIGNAL] = g_signal_new("progress",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, progress),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[IDLE_SIGNAL] = g_signal_new("idle",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, idle),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static gboolean obex_client_callback(GIOChannel *source,
					GIOCondition cond, gpointer data)
{
	ObexClient *self = data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		debug("link error");
		return FALSE;
	}

	if (OBEX_HandleInput(priv->handle, 1) < 0)
		debug("input error");
	else
		obex_do_callback(priv->handle);

	return TRUE;
}

static gboolean obex_client_put_idle(ObexClient *self)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	debug("obex_client_put_idle");

	g_source_destroy(priv->idle_source);
	priv->idle_source = NULL;

	if (priv->watch_func)
		priv->watch_func(self, OBEX_CLIENT_COND_OUT, priv->watch_data);

	return FALSE;
}

ObexClient *obex_client_new(void)
{
	return OBEX_CLIENT(g_object_new(OBEX_TYPE_CLIENT, NULL));
}

void obex_client_destroy(ObexClient *self)
{
	g_object_unref(self);
}

void obex_client_set_auto_connect(ObexClient *self, gboolean auto_connect)
{
	g_object_set(self, "auto-connect", auto_connect, NULL);
}

gboolean obex_client_get_auto_connect(ObexClient *self)
{
	gboolean auto_connect;

	g_object_get(self, "auto-connect", &auto_connect, NULL);

	return auto_connect;
}

void obex_client_add_watch_full(ObexClient *self,
			ObexClientCondition condition, ObexClientFunc func,
					gpointer data, GDestroyNotify notify)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	priv->watch_data = data;
	priv->watch_func = func;
	priv->watch_destroy = notify;
}

void obex_client_add_watch(ObexClient *self, ObexClientCondition condition,
				ObexClientFunc func, gpointer data)
{
	obex_client_add_watch_full(self, condition, func, data, NULL);
}

static void obex_connect_cfm(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	debug("connected");

	priv->connected = TRUE;

	g_signal_emit(self, signals[CONNECTED_SIGNAL], 0, NULL);
}

static void obex_disconn_ind(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;

	debug("disconnect");

	g_signal_emit(self, signals[DISCONNECT_SIGNAL], 0, NULL);
}

static void obex_progress_ind(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;

	debug("progress");

	g_signal_emit(self, signals[PROGRESS_SIGNAL], 0, NULL);
}

static void obex_command_ind(obex_t *handle, int event, void *user_data)
{
	ObexClient *self = user_data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (!priv->watch_func)
		return;

	switch (event) {
	case OBEX_EV_LINKERR:
	case OBEX_EV_PARSEERR:
		priv->watch_func(self, OBEX_CLIENT_COND_ERR, priv->watch_data);
		break;

	case OBEX_EV_STREAMAVAIL:
		priv->watch_func(self, OBEX_CLIENT_COND_IN, priv->watch_data);
		break;

	case OBEX_EV_STREAMEMPTY:
		priv->watch_func(self, OBEX_CLIENT_COND_OUT, priv->watch_data);
		break;

	case OBEX_EV_ABORT:
	case OBEX_EV_REQDONE:
		priv->watch_func(self, OBEX_CLIENT_COND_DONE, priv->watch_data);
		break;
		
	default:
		debug("unhandled event %d", event);
		break;
	}
}

static obex_callback_t callback = {
	&obex_connect_cfm,
	&obex_disconn_ind,
	&obex_progress_ind,
	&obex_command_ind,
};

gboolean obex_client_attach_fd(ObexClient *self, int fd, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	GSource *source;

	priv->handle = obex_open(fd, &callback, self);
	if (priv->handle == NULL) {
		err2gerror(errno, error);
		return FALSE;
	}

	priv->channel = g_io_channel_unix_new(fd);

	source = g_io_create_watch(priv->channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL);

	g_source_set_callback(source, (GSourceFunc) obex_client_callback,
								self, NULL);

	g_source_attach(source, priv->context);

	g_source_unref(source);

	return TRUE;
}

gboolean obex_client_connect(ObexClient *self, const guchar *target,
						gsize size, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_connect(priv->handle, target, size);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_disconnect(ObexClient *self, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_disconnect(priv->handle);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_put_object(ObexClient *self, const gchar *type,
					const gchar *name, gint size,
					time_t mtime, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	gboolean add_idle = FALSE;
	int err;

	if (priv->connected == FALSE && priv->auto_connect == TRUE)
		obex_connect(priv->handle, NULL, 0);
	else
		add_idle = TRUE;

	err = obex_put(priv->handle, type, name, size, mtime);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	/* So the application gets OBEX_CLIENT_COND_OUT immediately when
	 * returning to the mainloop */
	if (add_idle) {
		priv->idle_source = g_idle_source_new();
		g_source_set_callback(priv->idle_source, (GSourceFunc) obex_client_put_idle,
					self, NULL);
		g_source_attach(priv->idle_source, priv->context);
		g_source_unref(priv->idle_source);
	}

	return TRUE;
}

gboolean obex_client_get_object(ObexClient *self, const gchar *type,
					const gchar *name, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	if (priv->connected == FALSE && priv->auto_connect == TRUE)
		obex_connect(priv->handle, NULL, 0);

	err = obex_get(priv->handle, type, name);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_read(ObexClient *self, gchar *buf, gsize count,
					gsize *bytes_read, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_read(priv->handle, buf, count, bytes_read);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_write(ObexClient *self, const gchar *buf, gsize count,
					gsize *bytes_written, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_write(priv->handle, buf, count, bytes_written);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_flush(ObexClient *self, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_flush(priv->handle);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_abort(ObexClient *self, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	err = obex_abort(priv->handle);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_close(ObexClient *self, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	debug("");

	err = obex_close_transfer(priv->handle);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_get_error(ObexClient *self, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int rsp;

	rsp = obex_get_response(priv->handle);
	if (rsp == OBEX_RSP_SUCCESS)
		return TRUE;

	rsp2gerror(rsp, error);

	return FALSE;
}

gboolean obex_client_mkdir(ObexClient *self, const gchar *path, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	debug("");

	err = obex_setpath(priv->handle, path, 1);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_chdir(ObexClient *self, const gchar *path, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	debug("");

	err = obex_setpath(priv->handle, path, 0);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}

gboolean obex_client_delete(ObexClient *self, const gchar *name, GError **error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	debug("");

	err = obex_delete(priv->handle, name);
	if (err < 0) {
		err2gerror(-err, error);
		return FALSE;
	}

	return TRUE;
}
