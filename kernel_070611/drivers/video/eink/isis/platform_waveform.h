/*
 * eInk display framebuffer driver
 * 
 * Copyright (C)2005-2008 Lab126, Inc.  All rights reserved.
 */

#ifndef _PLATFORM_WAVEFORM_H
#define _PLATFORM_WAVEFORM_H

enum bool
{
        false = 0,
        true
};
typedef enum bool bool;

#include "broadsheet_waveform.h"
#include "broadsheet_commands.h"

#define einkfb_print_error(format, arg...) printf("eu: %s> "format, __FUNCTION__, ##arg )

#undef  EINKFB_DEBUG
#ifdef  EINKFB_DEBUG
#define	einkfb_debug(format, arg...) einkfb_print_error(format, arg)
#else
#define einkfb_debug(format, arg...)
#endif

typedef int bs_flash_select;
#define bs_flash_waveform                       0
#define bs_flash_commands                       0
#define broadsheet_get_flash_select()           0
#define broadsheet_set_flash_select(f)          \
{                                               \
    int unused = 0; unused = (f) ? unused : 0;  \
}

// Replace platform-specific routines with generic ones.
//
#define broadsheet_read_from_flash_byte         platform_read_from_flash_byte   
#define broadsheet_read_from_flash_short        platform_read_from_flash_short  
#define broadsheet_read_from_flash_long         platform_read_from_flash_long

// Replace generic routines with platform-specific ones.
//
#define platform_get_waveform_version_string    broadsheet_get_waveform_version_string
#define platform_get_commands_version_string    broadsheet_get_commands_version_string

#endif // _PLATFORM_WAVEFORM_H
