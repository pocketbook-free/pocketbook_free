/******************************************************************
 *
 *  File:   pn_lcd.h
 *
 *  Author: Nick Vaccaro <nvaccaro@lab126.com>
 *
 *  Date:   12/07/05
 *
 * Copyright 2005-2006, Lab126, Inc.  All rights reserved.
 *
 *  Description:
 *      Constants and definitions for PNLCD driver
 *
 ******************************************************************/

#ifndef __PN_LCD_H__
#define __PN_LCD_H__

#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/miscdevice.h>
#endif
#ifdef __PN_LCD_C_
#include <plat/platform_lab126.h>
//#include <plat/platform_labxxx.h>
#endif

#ifdef CONFIG_ARCH_FIONA    //------------- Fiona (Linux 2.6.10) Build

#ifdef __PN_LCD_C_
#define PNLCD_INIT()
#define PNLCD_DONE()
#endif

#else   // -------------------------------- Mario (Linux 2.6.22) Build

#ifdef __PN_LCD_C_
#define PNLCD_INIT()                                    set_pnlcd_ioctl(PNLCD_Ioctl_Sys)
#define PNLCD_DONE()                                    set_pnlcd_ioctl(NULL)
#endif

#endif  // --------------------------------

// PNLCD IOCTL Segment Record Definitions
//
typedef struct PNLCD_SEQ_SEGMENTS_REC {
    unsigned short  enable;         /* 1=ON, 0=OFF */
    unsigned int    start_seg;      /* First segment of sequential sequence */
    unsigned int    end_seg;        /* Last segment of sequential sequence */
}PNLCD_SEQ_SEGMENTS_REC, *PNLCD_SEQ_SEGMENTS_REC_PTR;

typedef struct PNLCD_MISC_SEGMENTS_REC {
    unsigned short  enable;         /* 1=ON, 0=OFF */
    unsigned int    num_segs;       /* number of segments to turn on or off */
    unsigned int    *seg_array;     /* ptr to array holding segment numbers */
}PNLCD_MISC_SEGMENTS_REC, *PNLCD_MISC_SEGMENTS_REC_PTR;

typedef struct PNLCD_SEGMENT_PAIRS_REC {
    unsigned short  enable;         /* 1=ON, 0=OFF */
    unsigned int    num_pairs;      /* number of pairs of segments to turn on or off */
    unsigned int    *seg_pair_array;/* ptr to array holding segment pair numbers */
}PNLCD_SEGMENT_PAIRS_REC, *PNLCD_SEGMENT_PAIRS_REC_PTR;

// PNLCD Animation Definitions
//
enum pnlcd_animation_cmds
{
    start_animation = 0,                // Takes arg pnlcd_animation_auto or pnlcd_animation_manual (default:  pnlcd_animation_manual)
    stop_animation,                     // Takes no args.
    update_animation,                   // Takes no args.
    
    set_animation_rate,                 // Takes arg minimum_pnlcd_animation_rate..maximum_pnlcd_animation_rate (default:  default_pnlcd_animation_rate).
    set_animation_type                  // Takes arg sp_animation_rotate..sp_animation_usb_external (default: sp_animation_rotate).
};
typedef enum pnlcd_animation_cmds pnlcd_animation_cmds;

enum pnlcd_animations
{
    sp_animation_rotate = 0,            // "spinner loop" animations
    sp_animation_march,
    sp_animation_dice,
    sp_animation_pong,
    
    sp_animation_usb_internal_read,
    sp_animation_usb_internal_write,
    
    sp_animation_usb_external_read,
    sp_animation_usb_external_write,
    
    sp_animation_rotate_reverse,
    
    num_sp_animations
};
typedef enum pnlcd_animations pnlcd_animations;

struct pnlcd_animation_t
{
    pnlcd_animation_cmds    cmd;
    int                     arg;
};
typedef struct pnlcd_animation_t pnlcd_animation_t;

#define pnlcd_animation_auto            0
#define pnlcd_animation_manual          1
#define pnlcd_animation_ignore_rate     2

#define maximum_pnlcd_animation_rate    60
#define default_pnlcd_animation_rate    10
#define minimum_pnlcd_animation_rate    1

// PNLCD Blocking Definitions
//
enum pnlcd_blocking_cmds
{
    start_block = 0,                    // Block/unblock all PNLCD IOCTL traffic.
    stop_block,                         //
    
    start_delay,                        // Delay/undelay all PNLCD IOCTL traffic except animation.
    stop_delay,                         //

    stop_delay_restore                  // Restore PNLCD after a delay (and stop any animation).
};
typedef enum pnlcd_blocking_cmds pnlcd_blocking_cmds;

// PNLCD IOCTL Constants
//
#define PNLCD_SEQ_IOCTL                 _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 0, PNLCD_SEQ_SEGMENTS_REC *) 
#define PNLCD_VERT_SEQ_IOCTL            _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 1, PNLCD_SEQ_SEGMENTS_REC *) 
#define PNLCD_PAIRS_IOCTL               _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 2, PNLCD_SEGMENT_PAIRS_REC *)
#define PNLCD_VERT_PAIRS_IOCTL          _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 3, PNLCD_SEGMENT_PAIRS_REC *)
#define PNLCD_MISC_IOCTL                _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 4, PNLCD_MISC_SEGMENTS_REC *)
#define PNLCD_PROGRESS_BAR_IOCTL        _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 5, int)
#define PNLCD_CLEAR_ALL_IOCTL           _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 6, int)
#define PNLCD_ANIMATE_IOCTL             _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 7, pnlcd_animation_t *)
#define PNLCD_GET_SEG_STATE             _IOR(IOCTL_PNLCD_MAGIC_NUMBER, 8, char *)
#define PNLCD_SET_SEG_STATE             _IOW(IOCTL_PNLCD_MAGIC_NUMBER, 9, char *)
#define PNLCD_IOCTLLast                 9

// Special PNLCD IOCTL processed outside the normal IOCTLs
//
#define PNLCD_BLOCK_IOCTL               _IOW(IOCTL_PNLCD_MAGIC_NUMBER, (PNLCD_IOCTLLast+1), pnlcd_blocking_cmds)

// Segment Constants
//
#define PN_LCD_COLUMN_COUNT             2   // There are 2 PNLCD columns
#define PN_LCD_SEGMENT_COUNT            200 // There are 200 segments
#define MAX_PN_LCD_SEGMENT              199 // Segments 0-199

#define FIRST_PNLCD_SEGMENT             0
#define LAST_PNLCD_SEGMENT              MAX_PN_LCD_SEGMENT

#define SEGMENT_ON  1                   // Values for "on" parameter
#define SEGMENT_OFF 0

#define VERT_SEQ    1                   // Vertical sequential (just odds or just evens)
#define SEQ         0                   // Sequential (all)

// Driver Constants
//
#define IOCTL_PNLCD_MAGIC_NUMBER        'n'
#define PNLCD_MAJOR                     10
#define PNLCD_MINOR                     IOC_PNLCD_MINOR
#define PNLCD_DRIVER_NAME               "pnlcd"
#define PNLCD_DRIVER_PATH               "/dev/misc/pnlcd"

#define PNLCD_PROC_MODE                 0666
#define PNLCD_PROC_PARENT               PNLCD_DRIVER_NAME
#define PNLCD_PROC_IO_TXT               "io_txt"
#define PNLCD_PROC_IO_BIN               "io_bin"
#define PNLCD_PROC_PROGRESS_BAR         "progress_bar"
#define PNLCD_PROC_ANIMATE              "animate"

// Intra-kernel Access
//
extern int pnlcd_delayed(void);
extern int pnlcd_hidden(void);

// Inter-module/inter-driver PNLCD IOCTL Access
//
extern struct file_operations pnlcd_fops;

#define PNLCD_IOCTL_FLAG                0x64636C6E // 'nlcd'
#define pnlcd_sys_open()                pnlcd_fops.open((struct inode*)PNLCD_IOCTL_FLAG, NULL)
#define pnlcd_sys_close()               pnlcd_fops.release((struct inode*)PNLCD_IOCTL_FLAG, NULL)
#define pnlcd_sys_ioctl(cmd, arg)       pnlcd_fops.ioctl((struct inode*)PNLCD_IOCTL_FLAG, NULL, (unsigned int)cmd, (unsigned long)arg)

#endif  // __PN_LCD_H__

