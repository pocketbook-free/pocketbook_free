/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_waveform.h --
 *  eInk frame buffer device HAL broadsheet waveform defs
 *
 *      Copyright (C) 2005-2009 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef _BROADSHEET_WAVEFORM_H
#define _BROADSHEET_WAVEFORM_H

#define EINK_WAVEFORM_FILESIZE          131072  // 128K..
#define EINK_WAVEFORM_COUNT             0       // ..max

#define EINK_ADDR_CHECKSUM1             0x001F  // 1 byte  (checksum of bytes 0x00-0x1E)
#define EINK_ADDR_CHECKSUM2             0x002F  // 1 byte  (checksum of bytes 0x20-0x2E)
#define EINK_ADDR_CHECKSUM              0x0000  // 4 bytes (was EINK_ADDR_MFG_DATA_DEVICE)
#define EINK_ADDR_FILESIZE              0x0004  // 4 bytes (was EINK_ADDR_MFG_DATA_DISPLAY)
#define EINK_ADDR_MFG_CODE              0x0015  // 1 byte  (0x01=PVI, 0x02=LGD)
#define EINK_ADDR_SERIAL_NUMBER         0x0008  // 4 bytes (little-endian)

#define EINK_ADDR_RUN_TYPE              0x000C  // 1 byte  (0x00=[B]aseline, 0x01=[T]est/trial, 0x02=[P]roduction, 0x03=[Q]ualification, 0x04=V110[A])

#define EINK_ADDR_FPL_PLATFORM          0x000D  // 1 byte  (0x00=2.0, 0x01=2.1, 0x02=2.3; 0x03=Vizplex 110, 0x04=V110A; other values undefined)
#define EINK_ADDR_FPL_SIZE              0x0014  // 1 byte  (0x32=5", 0x3C=6", 0x50=8", 0x61=9.7")
#define EINK_ADDR_FPL_LOT               0x000E  // 2 bytes (little-endian)
#define EINK_ADDR_ADHESIVE_RUN_NUM      0x0010  // 1 byte  (mode version when EINK_ADDR_FPL_PLATFORM is 0x03 or later)
#define EINK_ADDR_MODE_VERSION          0x0010  // 1 byte  (0x01 -> 0 INIT, 1 DU, 2 GC16, 3 GC4)

#define EINK_ADDR_WAVEFORM_VERSION      0x0011  // 1 byte  (BCD)
#define EINK_ADDR_WAVEFORM_SUBVERSION   0x0012  // 1 byte  (BCD)
#define EINK_ADDR_WAVEFORM_TYPE         0x0013  // 1 byte  (0x0B=TE, 0x0E=WE; other values undefined)

#define EINK_FPL_SIZE_60                0x3C    // 6.0-inch panel,  800x600
#define EINK_FPL_SIZE_97                0x61    // 9.7-inch panel, 1200x825

struct broadsheet_waveform_info_t
{
    unsigned char   waveform_version,           // EINK_ADDR_WAVEFORM_VERSION
                    waveform_subversion,        // EINK_ADDR_WAVEFORM_SUBVERSION
                    waveform_type,              // EINK_ADDR_WAVEFORM_TYPE
                    run_type,                   // EINK_ADDR_RUN_TYPE
                    fpl_platform,               // EINK_ADDR_FPL_PLATFORM
                    fpl_size,                   // EINK_ADDR_FPL_SIZE
                    adhesive_run_number,        // EINK_ADDR_ADHESIVE_RUN_NUM
                    mode_version,               // EINK_ADDR_MODE_VERSION
                    mfg_code;                   // EINK_ADDR_MFG_CODE

    unsigned short  fpl_lot;                    // EINK_ADDR_FPL_LOT
    
    unsigned long   filesize,                   // EINK_ADDR_FILESIZE
                    serial_number,              // EINK_ADDR_SERIAL_NUMBER
                    checksum;                   // EINK_ADDR_FILESIZE ? EINK_ADDR_CHECKSUM : (EINK_ADDR_CHECKSUM2 << 16) | EINK_ADDR_CHECKSUM1
};
typedef struct broadsheet_waveform_info_t broadsheet_waveform_info_t;

struct broadsheet_waveform_t
{
    unsigned char   version,
                    subversion,
                    type,
                    run_type,
                    mode_version,
                    mfg_code;
    unsigned long   serial_number;
    
    bool            parse_wf_hex;
};
typedef struct broadsheet_waveform_t broadsheet_waveform_t;

struct broadsheet_fpl_t
{
    unsigned char   platform,
                    size,
                    adhesive_run_number;
                    
    unsigned short  lot;
};
typedef struct broadsheet_fpl_t broadsheet_fpl_t;

extern void broadsheet_get_waveform_info(broadsheet_waveform_info_t *info);
extern void broadsheet_get_waveform_version(broadsheet_waveform_t *waveform);
extern void broadsheet_get_fpl_version(broadsheet_fpl_t *fpl);
extern char *broadsheet_get_waveform_version_string(void);

extern bool broadsheet_waveform_valid(void);

#endif // _BROADSHEET_WAVEFORM_H
