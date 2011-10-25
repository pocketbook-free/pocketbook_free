/*
 * e-ink display framebuffer driver
 * 
 * Copyright (C)2005-2007 Lab126, Inc.  All rights reserved.
 */

#ifndef _APOLLO_UNIVERSAL_H
#define _APOLLO_UNIVERSAL_H

#define STEPHANIE_FAST_DISP_IMAGE_CMD   0xA5 // Apollo/Stephanie conflict
#define STEPHANIE_DISP_IMAGE_CMD        0xA6
#define STEPHANIE_DRAW_LINE_CMD         0xA7
#define STEPHANIE_GET_STATUS_CMD        0xAB

#define STEPHANIE_POWER_ON_CMD          0xC0
#define STEPHANIE_POWER_OFF_CMD         0xC1

#define STEPHANIE_SOFTWARE_VERSION_CMD  0xE1
#define STEPHANIE_DISPLAY_SIZE_CMD      0xE2

#define STEPHANIE_SET_PANEL_SIZE_CMD    0xF3 // Apollo/Stephanie conflict

#define EINK_DEPTH_4BPP                 4

#define STEPHANIE_X_RES                 800
#define STEPHANIE_Y_RES                 600

enum apollo_t
{
    apollo_stephanie    = 0x05,         // PVI Trinity S Module 
    apollo_apollo       = 0x18,         // eInk Apollo
    
    apollo_unknown      = 0xFF
};
typedef enum apollo_t apollo_t;

struct stephanie_status_t
{
    unsigned char   cmd_finished   : 1;
    unsigned char   sys_busy       : 1;
    unsigned char   sys_status_0   : 1;
    unsigned char   sys_status_1   : 1;
    unsigned char   sys_status_2   : 1;
    unsigned char   sys_status_3   : 1;
    unsigned char   sys_status_4   : 1;
    unsigned char   cmd_ready      : 1;
};
typedef struct stephanie_status_t stephanie_status_t;

typedef u8 (*transform_data_t)(u8 *buffer, int i, int xres, int yres);

struct apollo_preprocess_command_t
{
  int header_size,
      skip_args,
      xres,
      yres;
      
  u8  *header;
  
  transform_data_t transform_data;
};
typedef struct apollo_preprocess_command_t apollo_preprocess_command_t;

typedef void (*apollo_controller_preprocess_command_t)(apollo_preprocess_command_t *apollo_preprocess_command,
  apollo_command_t *apollo_command);

typedef void (*apollo_controller_postprocess_command_t)(apollo_command_t *apollo_command);

#endif // _APOLLO_UNIVERSAL_H
