/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_commands.h --
 *  eInk frame buffer device HAL broadsheet commands defs
 *
 *      Copyright (C) 2005-2008 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef _BROADSHEET_COMMANDS_H
#define _BROADSHEET_COMMANDS_H

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define EINK_COMMANDS_FILESIZE          0x0cd8  // We need to have EXACTLY...
#else
#define EINK_COMMANDS_FILESIZE          0x0886  // We need to have EXACTLY...
#endif
#define EINK_COMMANDS_COUNT             1       // ...2182 bytes of commands.

#define EINK_ADDR_COMMANDS_VERS_MAJOR   0x0003  // 1 byte
#define EINK_ADDR_COMMANDS_VERS_MINOR   0x0002  // 1 byte
#define EINK_ADDR_COMMANDS_TYPE         0x0000  // 2 bytes (big-endian, 0x0000 or 'bs' -> 0x6273)

#define EINK_ADDR_COMMANDS_CHECKSUM     0x0882  // 4 bytes (checksum of bytes 0x000-0x881)

struct broadsheet_commands_info_t
{
    unsigned char   vers_major,
                    vers_minor;
                    
    unsigned short  type;
                   
    unsigned long  checksum;
};
typedef struct broadsheet_commands_info_t broadsheet_commands_info_t;

extern void broadsheet_get_commands_version(broadsheet_commands_info_t *commands);
extern char *broadsheet_get_commands_version_string(void);

extern bool broadsheet_commands_valid(void);

#endif // _BROADSHEET_COMMANDS_H
