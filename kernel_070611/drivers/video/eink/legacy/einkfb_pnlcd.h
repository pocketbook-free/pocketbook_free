/*
 *  linux/drivers/video/eink/legacy/einkfb_pnlcd.h -- eInk pnlcd emulator
 *
 *      Copyright (C) 2005-2007 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */
 
#ifndef _EINKFB_PNLCD_H
#define _EINKFB_PNLCD_H

#define PRAGMAS						0

#include <plat/boot_globals.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#include "pn_lcd.h"

// From miscdevice.h
//
#define IOC_PNLCD_MINOR					187

// From fiona.h
//
#define FIONA_REBOOT_PNLCD				0x7FFFFFFF

// From ioc_protocol.h
//
#define MAX_COMMAND_BYTES				32

#define IOC_CMD_PNLCD_LIGHT_RESPONSE_BYTES              1

#define IOC_CMD_PNLCD_SEQUENTIAL_ON			0x40
#define IOC_CMD_PNLCD_SEQUENTIAL_OFF			0x41
#define IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_ON		0x42
#define IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_OFF		0x43
#define IOC_CMD_PNLCD_MISC_ON				0x44
#define IOC_CMD_PNLCD_MISC_OFF				0x45
#define IOC_CMD_PNLCD_MISC_PAIR_ON			0x46
#define IOC_CMD_PNLCD_MISC_PAIR_OFF			0x47
#define IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_ON		0x48
#define IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_OFF		0x49


// From ioc.h
//
#undef DEBUG_PNLCD

#if defined(DEBUG_PNLCD)
#define NDEBUG(args...) printk(args)
#else
#define NDEBUG(args...) do {} while(0)
#endif

#define memclr(a,l)	memset(a,0,l)
#define MIN(x,y)	((x<y)?x:y)
#define MAX(x,y)	((x>y)?x:y)

#define ENABLE		1
#define DISABLE		0

typedef struct IOC_ENABLE_REC {
    unsigned short  enable;		/* 1=ON, 0=OFF */
}IOC_ENABLE_REC, *IOC_ENABLE_REC_PTR;

#define IOCTL_IOC_MAGIC_NUMBER		'i'
#define IOC_PNLCD_SEQ_IOCTL		_IOW(IOCTL_IOC_MAGIC_NUMBER, 0, PNLCD_SEQ_SEGMENTS_REC *)
#define IOC_PNLCD_VERT_SEQ_IOCTL	_IOW(IOCTL_IOC_MAGIC_NUMBER, 1, PNLCD_SEQ_SEGMENTS_REC *)
#define IOC_PNLCD_PAIRS_IOCTL		_IOW(IOCTL_IOC_MAGIC_NUMBER, 2, PNLCD_SEGMENT_PAIRS_REC *)
#define IOC_PNLCD_VERT_PAIRS_IOCTL	_IOW(IOCTL_IOC_MAGIC_NUMBER, 3, PNLCD_SEGMENT_PAIRS_REC *)
#define IOC_PNLCD_MISC_IOCTL		_IOW(IOCTL_IOC_MAGIC_NUMBER, 4, PNLCD_MISC_SEGMENTS_REC *)
#define IOC_PNLCD_ENABLE_IOCTL		_IOW(IOCTL_IOC_MAGIC_NUMBER, 6, IOC_ENABLE_REC *)

// From fpow.h
//
typedef enum FPOW_POWER_MODE {
	FPOW_MODE_OFF = 0,
	FPOW_MODE_ON,
	FPOW_MODE_OFF_NO_WAKE,
	FPOW_MODE_400_MHZ,
	FPOW_MODE_200_MHZ,
	FPOW_MODE_SLEEP,
} FPOW_POWER_MODE;

// From fiona_power.h
//
typedef enum FPOW_ERR {
	FPOW_NO_ERR = 0,
	FPOW_CANCELLED_EVENT_ERR,
	FPOW_COMPONENT_TRANSITION_ERR,
	FPOW_NOT_ENOUGH_POWER_ERR,
	FPOW_COMPONENT_NOT_POWERED_ERR,
	FPOW_NO_MATCHES_FOUND_ERR
} FPOW_ERR;

// From pnlcd_pm.h
//
extern int pnlcd_awake(void);

#endif // _EINKFB_PNLCD_H


