/**
	\file apps/obex_test_client.h
 	Client OBEX Commands.
	OpenOBEX test applications and sample code.

 */

#ifndef OBEX_TEST_CLIENT_H
#define OBEX_TEST_CLIENT_H

#include "obex_test.h"

void client_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp);

void connect_client(obex_t *handle);
void connect_client_done(obex_t *handle, obex_object_t *object, int obex_rsp);

void disconnect_client(obex_t *handle);
void disconnect_client_done(obex_t *handle, obex_object_t *object, int obex_rsp);

int fillstream(obex_t *handle, obex_object_t *object);
void push_client(obex_t *handle);

void put_client(obex_t *handle);
void put_client_done(obex_t *handle, obex_object_t *object, int obex_rsp);

void get_client(obex_t *handle, struct context *gt);
void get_client_done(obex_t *handle, obex_object_t *object, int obex_rsp, char *name);

void setpath_client(obex_t *handle);
void setpath_client_done(obex_t *handle, obex_object_t *object, int obex_rsp);

#endif
