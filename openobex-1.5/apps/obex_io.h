/**
	\file apps/obex_io.h
	Some useful disk-IO functions.
	OpenOBEX test applications and sample code.

 */

#ifndef OBEX_IO_H
#define OBEX_IO_H

/* Application defined headers */
#define HEADER_CREATOR_ID  0xcf

#define ADDRESS_BOOK   0x61646472 /* "addr" *.vcf */
#define MEMO_PAD       0x6d656d6f /* "memo" *.txt */
#define TO_DO_LIST     0x746f646f /* "todo" *.vcs */
#define DATE_BOOK      0x64617465 /* "date" *.vcs */
#define PILOT_RESOURCE 0x6c6e6368 /* "Inch" *.prc */

int get_filesize(const char *filename);
obex_object_t *build_object_from_file(obex_t *handle, const char *filename, uint32_t creator_id);
int safe_save_file(char *name, const uint8_t *buf, int len);
uint8_t* easy_readfile(const char *filename, int *file_size);

/* hack to distinguish between different obex protocols */
extern int obex_protocol_type;
#define OBEX_PROTOCOL_GENERIC		0
#define OBEX_PROTOCOL_WIN95_IRXFER	1
	/* win95 irxfer ( does not like palm creatorid header ) */

#endif
