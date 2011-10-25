/*
 * eInk display framebuffer driver
 * 
 * Copyright (C)2005-2008 Lab126, Inc.  All rights reserved.
 */

#ifndef _PLATFORM_WAVEFORM_H
#define _PLATFORM_WAVEFORM_H

#include "apollo_waveform.h"

// Apollo doesn't have a commands section of flash.  So, just make
// all of the commands code use the waveform code instead.
//
#define EINK_COMMANDS_FILESIZE                  EINK_WAVEFORM_FILESIZE
#define EINK_COMMANDS_COUNT                     EINK_WAVEFORM_COUNT

#define get_embedded_commands_checksum          get_embedded_waveform_checksum
#define get_computed_commands_checksum          get_computed_waveform_checksum
#define platform_get_commands_version_string    apollo_get_waveform_version_string

// Replace platform-specific routines with generic ones.
//
#define apollo_read_from_flash_byte             platform_read_from_flash_byte   
#define apollo_read_from_flash_short            platform_read_from_flash_short  
#define apollo_read_from_flash_long             platform_read_from_flash_long

// Replace generic routines with platform-specific ones.
//
#define platform_get_waveform_version_string    apollo_get_waveform_version_string

#endif // _PLATFORM_WAVEFORM_H
