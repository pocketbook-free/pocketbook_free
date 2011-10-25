/*
 *  linux/drivers/video/eink/hal/einkfb_hal_builtin_base.c
 *  -- eInk frame buffer device HAL
 *
 *      Copyright (C) 2005-2008 BSP2
 *
 *  This file is based on:
 *
 *  linux/drivers/video/skeletonfb.c -- Skeleton for a frame buffer device
 *  linux/drivers/video/vfb.c -- Virtual frame buffer device
 *
 *      Copyright (C) 2002 James Simmons
 *      Copyright (C) 1997 Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

// For testing the eInk HAL built into the kernel, include all of its parts.
//
#define _EINKFB_BUILTIN

#include "einkfb_hal_main.c"
#include "einkfb_hal_events.c"
#include "einkfb_hal_util.c"
#include "einkfb_hal_proc.c"
#include "einkfb_hal_mem.c"
#include "einkfb_hal_io.c"
#include "einkfb_hal_pm.c"
