/**
	\file apps/obex_test_server.h
	Server OBEX Commands.
	OpenOBEX test applications and sample code.

 */

#ifndef OBEX_TEST_SERVER_H
#define OBEX_TEST_SERVER_H

void server_do(obex_t *handle);
void server_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp);
void server_request(obex_t *handle, obex_object_t *object, int event, int cmd);

#endif
