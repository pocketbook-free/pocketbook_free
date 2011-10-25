/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_commands.h --
 *  eInk frame buffer device HAL broadsheet commands defs
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef _BROADSHEET_COMMANDS_H
#define _BROADSHEET_COMMANDS_H

#define EINK_COMMANDS_FILESIZE          0x0886  // We need to have EXACTLY...
#define EINK_COMMANDS_COUNT             1       // ...2182 bytes of commands.

#define EINK_ADDR_COMMANDS_VERS_MAJOR   0x0003  // 1 byte
#define EINK_ADDR_COMMANDS_VERS_MINOR   0x0002  // 1 byte
#define EINK_ADDR_COMMANDS_TYPE         0x0000  // 2 bytes (big-endian, 0x0000 or 'bs' -> 0x6273)

#define EINK_ADDR_COMMANDS_CHECKSUM     0x0882  // 4 bytes (checksum of bytes 0x000-0x881)

#define EINK_ADDR_COMMANDS_VERSION      0x0000  // 4 bytes (little-endian)
#define EINK_ADDR_COMMANDS_CODE_SIZE    0x0004  // 2 bytes (big-endian, zero-based)

#define EINK_COMMANDS_FIXED_CODE_SIZE           \
                                        (  4 +  /* EINK_ADDR_COMMANDS_VERSION */ \
                                           2 +  /* EINK_ADDR_COMMANDS_CODE_SIZE_1 + EINK_ADDR_COMMANDS_CODE_SIZE_2 */ \
                                         128 +  /* 64-command vector table (64 * 2-bytes) */ \
                                           4 )  /* checksum */

#define EINK_COMMANDS_BROADSHEET        0
#define EINK_COMMANDS_ISIS              1

struct broadsheet_commands_info_t
{
    int             which;              // EINK_COMMANDS_BROADSHEET || EINK_COMMANDS_ISIS
    
    unsigned char   vers_major,         // Broadsheet
                    vers_minor;         //
    unsigned short  type;               //

    unsigned long   version;            // ISIS
    
    unsigned long   checksum;           // Broadsheet = CRC32, ISIS = CRC8
};
typedef struct broadsheet_commands_info_t broadsheet_commands_info_t;

extern void broadsheet_get_commands_version(broadsheet_commands_info_t *commands);
extern char *broadsheet_get_commands_version_string(void);

extern bool broadsheet_commands_valid(void);

#endif // _BROADSHEET_COMMANDS_H
