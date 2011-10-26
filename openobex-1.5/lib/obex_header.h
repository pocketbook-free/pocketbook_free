/**
	\file obex_header.h
	OBEX header releated functions.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.

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

#ifndef OBEX_HEADERS_H
#define OBEX_HEADERS_H

#include "obex_main.h"

#define OBEX_HI_MASK     0xc0
#define OBEX_UNICODE     0x00
#define OBEX_BYTE_STREAM 0x40
#define OBEX_BYTE        0x80
#define OBEX_INT         0xc0

/* Common header used by all frames */

#pragma pack(1)
struct obex_common_hdr {
	uint8_t  opcode;
	uint16_t len;
};
#pragma pack()

typedef struct obex_common_hdr obex_common_hdr_t;

/* Connect header */
#pragma pack(1)
struct obex_connect_hdr {
	uint8_t  version;
	uint8_t  flags;
	uint16_t mtu;
};
#pragma pack()

typedef struct obex_connect_hdr obex_connect_hdr_t;

#pragma pack(1)
struct obex_uint_hdr {
	uint8_t  hi;
	uint32_t hv;
};
#pragma pack()

#pragma pack(1)
struct obex_ubyte_hdr {
	uint8_t hi;
	uint8_t hv;
};
#pragma pack()

#pragma pack(1)
struct obex_unicode_hdr {
	uint8_t  hi;
	uint16_t hl;
	uint8_t  hv[0];
};
#pragma pack()

#define obex_byte_stream_hdr obex_unicode_hdr

typedef struct {
	uint8_t identifier;    /* Header ID */
	int  length;         /* Total lenght of header */

	int  val_size;       /* Size of value */
	union {
		int   integer;
		char   *string;
		uint8_t *oct_seq;
	} t;
} obex_header_t;

int insert_uint_header(buf_t *msg, uint8_t identifier, uint32_t value);
int insert_ubyte_header(buf_t *msg, uint8_t identifier, uint8_t value);
int insert_unicode_header(buf_t *msg, uint8_t opcode, const uint8_t *text,
				int size);

int insert_byte_stream_header(buf_t *msg, uint8_t opcode,
				const uint8_t *stream, int size);

int obex_extract_header(buf_t *msg, obex_header_t *header);

#endif
