/*
 * eInk display framebuffer driver
 * 
 * Copyright (C)2005-2007 Lab126, Inc.  All rights reserved.
 */

#ifndef _APOLLO_WAVEFORM_H
#define _APOLLO_WAVEFORM_H

#define EINK_WAVEFORM_FILESIZE          65536   // 64K
#define EINK_WAVEFORM_COUNT             1

#define EINK_ADDR_CHECKSUM              0xE1E8  // 4 bytes (0x00000000 in checksum calc)
#define EINK_ADDR_MFG_DATA_DEVICE       0xE1EC  // 4 bytes (0x00000000 from eInk)
#define EINK_ADDR_MFG_DATA_DISPLAY      0xE1F0  // 4 bytes (0x00000000 from eInk)
#define EINK_ADDR_SERIAL_NUMBER         0xE1F4  // 4 bytes (big-endian)

#define EINK_ADDR_RUN_TYPE              0xE1F8  // 1 byte  (00=[B]aseline, 01=[T]est/trial, 02=[P]roduction)

#define EINK_ADDR_FPL_PLATFORM          0xE1F9  // 1 byte  (00=2.0, 01=2.1, 02=2.3; 03=Vizplex 110; other values undefined)
#define EINK_ADDR_FPL_LOT_BIN           0xE1FA  // 2 bytes (stored as big-endian, not BCD)
#define EINK_ADDR_ADHESIVE_RUN_NUM      0xE1FC  // 1 byte  (undefined when EINK_ADDR_FPL_PLATFORM is 03)

#define EINK_ADDR_MIN_TEMP              0xE1FD  // 1 byte
#define EINK_ADDR_MAX_TEMP              0xE1FE  // 1 byte
#define EINK_ADDR_TEMP_STEPS            0xE1FF  // 1 byte

#define EINK_ADDR_RESERVED              0xE200  // 2 bytes

#define EINK_ADDR_WAVEFORM_VERSION      0xE202  // 1 byte (BCD)
#define EINK_ADDR_WAVEFORM_SUBVERSION   0xE203  // 1 byte (BCD)
#define EINK_ADDR_FPL_LOT_BCD           0xE204  // 1 byte (BCD)
#define EINK_ADDR_WAVEFORM_TYPE         0xE205  // 1 byte (00=WX, 01=WY, 02=WP, 03=WZ, 04=WQ, 05=TA, 06=WU; other values undefined)

#define EINK_INVALID_WAVEFORM_DATA      0xFF
#define LEGACY_WAVEFORM_DATA(d)         (EINK_INVALID_WAVEFORM_DATA == d)

// Initial 2.3 waveform version data in legacy waveform format and corresponding
// translation data.
//
#define EINK_INIT_23_WF_VERSION         0x00    // BL23_WX00_04
#define EINK_INIT_23_WF_SUBVERSION      0x04    //
#define EINK_INIT_23_WF_TYPE            0x00    //
#define EINK_INIT_23_WF_FPL_LOT_BCD     0x23    // 

#define EINK_INIT_23_RUN_TYPE           0x01    // T000_01_23_WX0004
#define EINK_INIT_23_FPL_PLATFORM       0x02    //
#define EINK_INIT_23_FPL_LOT_BIN        0x0000  //
#define EINK_INIT_23_ADHESIVE_RUN_NUM   0x01    //

struct apollo_info_t
{
    unsigned char   waveform_version,           // EINK_ADDR_WAVEFORM_VERSION
                    waveform_subversion,        // EINK_ADDR_WAVEFORM_SUBVERSION
                    waveform_type,              // EINK_ADDR_WAVEFORM_TYPE
                    fpl_lot_bcd,                // EINK_ADDR_FPL_LOT_BCD
                    run_type,                   // EINK_ADDR_RUN_TYPE (was EINK_ADDR_FPL_RESERVED)
                    fpl_platform,               // EINK_ADDR_FPL_PLATFORM (was EINK_ADDR_FPL_VERSION)
                    adhesive_run_number,        // EINK_ADDR_ADHESIVE_RUN_NUMBER (was EINK_ADDR_WAVEFORM_REVISION)
                    waveform_temp_min,          // EINK_ADDR_MIN_TEMP
                    waveform_temp_max,          // EINK_ADDR_MAX_TEMP
                    waveform_temp_inc,          // EINK_ADDR_TEMP_STEPS
                    last_temperature,           // APOLLO_GET_TEMPERATURE_CMD
                    controller_version;         // APOLLO_GET_VERSION_CMD
    
    unsigned short  fpl_lot_bin;                // EINK_ADDR_FPL_LOT_BIN
    
    unsigned long   mfg_data_device,            // EINK_ADDR_MFG_DATA_DEVICE
                    mfg_data_display,           // EINK_ADDR_MFG_DATA_DISPLAY
                    serial_number,              // EINK_ADDR_SERIAL_NUMBER
                    checksum;                   // EINK_ADDR_CHECKSUM
};
typedef struct apollo_info_t apollo_info_t;

struct apollo_waveform_t
{
    unsigned char   version,                    // Old:  BL<lot>_<type><version>_<subversion>
                    subversion,                 // New:  <run_type><lot>_<adhesive_run_number>_
                    type,                       //       <platform>_<type><version><subversion>
                    fpl_lot,                    //
                    run_type;                   //
    unsigned long   serial_number;              // New & <run_type> == baseline
};
typedef struct apollo_waveform_t apollo_waveform_t;

struct apollo_fpl_t
{
    unsigned char   platform;                   // lot, adhesive run number, platform
    unsigned short  lot;                        //
    unsigned char   adhesive_run_number;        //
    
    unsigned char   overridden;                 // if true, using data from device
};
typedef struct apollo_fpl_t apollo_fpl_t;

extern void apollo_get_info(apollo_info_t *info);
extern void apollo_get_waveform_version(apollo_waveform_t *waveform);
extern void apollo_get_fpl_version(apollo_fpl_t *fpl);
extern char *apollo_get_waveform_version_string(void);

extern unsigned short apollo_get_fpl_platform_from_str(char *str);
extern void apollo_get_panel_id_str_from_fpl(char *str, apollo_fpl_t *fpl);

#endif // _APOLLO_WAVEFORM_H
