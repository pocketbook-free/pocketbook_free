/*
 *  linux/drivers/video/eink/hal/einkfb_hal.h -- eInk frame buffer device HAL definitions
 *
 *      Copyright (C) 2005-2009 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef _EINKFB_HAL_H
#define _EINKFB_HAL_H

#define	X3

#include <plat/boot_globals.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <plat/platform_lab126.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_ARCH_FIONA    //------------- Fiona (Linux 2.6.10) Build

#include <asm/arch/fiona.h>
#include "llog.h"


enum bool
{
        false = 0,
        true
};
typedef enum bool bool;

#define __INIT_DATA     __initdata
#define __INIT_CODE     __init

#define FB_DSHOW_PARAMS struct device *dev, char *buf
#define FB_DSTOR_PARAMS struct device *dev, const char *buf, size_t count
#define FB_IOCTL_PARAMS struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg, struct fb_info *info
#define FB_MMAP_PARAMS  struct fb_info *info, struct file *file, struct vm_area_struct *vma

#ifdef  _EINKFB_HAL_MAIN
#define FB_FILLRECT     cfb_fillrect
#define FB_COPYAREA     cfb_copyarea
#define FB_IMAGEBLIT    cfb_imageblit

#define REBOOT_PRIORITY FIONA_REBOOT_EINK_FB

#define FB_PROBE_PARAM  struct device *device
#define FB_PROBE_INIT   struct platform_device *dev = to_platform_device(device);
#define FB_SETDRVDATA() dev_set_drvdata(&dev->dev, info)

#define FB_REMOVE_PARAM FB_PROBE_PARAM
#define FB_GETDRVDATA() dev_get_drvdata(device)

#define FB_SUSPEND_PARM struct device *dev, u32 state, u32 level
#define FB_RESUME_PARAM struct device *dev, u32 level

#define FB_DRVREG(d)    driver_register(d)
#define FB_DRVUNREG(d)  driver_unregister(d)

#define FB_PLATREG(d)   platform_device_register(d)
#define FB_PLATFREE(d)  d = NULL

#define FB_PLATALLOC(d, n)                              \
static struct platform_device EINKFB_DEVICE =           \
{                                                       \
    .name	= n,                                        \
    .id	    = 0,                                        \
    .dev	=                                           \
    {                                                   \
        .release = einkfb_platform_release,             \
    }                                                   \
};                                                      \
d = &EINKFB_DEVICE

#define FB_DRIVER(d, n, p, r)                           \
static struct device_driver d =                         \
{                                                       \
    .name    = n,                                       \
    .bus     = &platform_bus_type,                      \
    .probe   = p,                                       \
    .remove  = r,                                       \
    .suspend = einkfb_suspend,                          \
	.resume  = einkfb_resume                            \
}

static void einkfb_platform_release(struct device *device)
{
}

#define FB_ROUNDUP(a, b) (a)

#endif  // _EINKFB_HAL_MAIN

#define FB_DEVICE_CREATE_FILE(d, f)                     \
    device_create_file(d, f)

#define	FB_GET_CLOCK_TICKS()                            1000000

#define FB_DECLARE_WORK(n, f)                           DECLARE_WORK(n, f, NULL)
#define FB_WORK_PARAM                                   void *unused

#else   // -------------------------------- Mario (Linux 2.6.22) Build

//#include <asm/arch/board_id.h>
#include <linux/platform_device.h>
#include <plat/llog.h>

#define __INIT_DATA     __devinitdata
#define __INIT_CODE     __devinit

#define FB_DSHOW_PARAMS struct device *dev, struct device_attribute *attr, char *buf
#define FB_DSTOR_PARAMS struct device *dev, struct device_attribute *attr, const char *buf, size_t count
#define FB_IOCTL_PARAMS struct fb_info *info, unsigned int cmd, unsigned long arg
#define FB_MMAP_PARAMS  struct fb_info *info, struct vm_area_struct *vma

#ifdef  _EINKFB_HAL_MAIN
#define FB_FILLRECT     cfb_fillrect
#define FB_COPYAREA     cfb_copyarea
#define FB_IMAGEBLIT    cfb_imageblit

#define REBOOT_PRIORITY 0x3FFFFFFF

#define FB_PROBE_PARAM  struct platform_device *dev
#define FB_PROBE_INIT
#define FB_SETDRVDATA() platform_set_drvdata(dev, info)  

#define FB_REMOVE_PARAM FB_PROBE_PARAM
#define FB_GETDRVDATA() platform_get_drvdata(dev)

#define FB_SUSPEND_PARM struct platform_device *pdev, pm_message_t state
#define FB_RESUME_PARAM struct platform_device *pdev

#define FB_DRVREG(d)    platform_driver_register(d)
#define FB_DRVUNREG(d)  platform_driver_unregister(d)

#define FB_PLATREG(d)   platform_device_add(d)
#define FB_PLATFREE(d)                                  \
    platform_device_put(d);                             \
    d = NULL

#define FB_PLATALLOC(d, n)                              \
    d = platform_device_alloc(n, 0)

#define FB_DRIVER(d, n, p, r)                           \
static struct platform_driver d =                       \
{                                                       \
    .probe   = p,                                       \
    .remove  = r,                                       \
    .suspend = einkfb_suspend,                          \
    .resume  = einkfb_resume,                           \
    .driver  =                                          \
    {                                                   \
       .name = n,                                       \
    }                                                   \
}

//add by jacky on Oct 8/////////////////////////////////////////

#define FB_DEFIO_INIT()                                 \
    info->fbdefio = &EINKFB_DEFIO;                      \
    fb_deferred_io_init(info)

#define FB_DEFIO_EXIT()                                 \
    fb_deferred_io_cleanup(info)

#define FB_DEFIO()                                      \
static struct fb_deferred_io EINKFB_DEFIO =             \
{                                                       \
    .delay       = HZ,                                  \
    .deferred_io = einkfb_deferred_io                   \
}

static void einkfb_deferred_io(struct fb_info *info, struct list_head *pagelist)
{
}
///////////////////////////////////////////////////////////////

#define FB_ROUNDUP(a, b) roundup((a), (b))

#endif  // _EINKFB_HAL_MAIN

#define FB_DEVICE_CREATE_FILE(d, f)                     \
{                                                       \
    int unused;                                         \
    unused = device_create_file(d, f);                  \
}

//#define GPT_BASE_ADDR					                (IO_ADDRESS(GPT1_BASE_ADDR))
//#define MXC_GPT_GPTCNT					                (GPT_BASE_ADDR + 0x24)
//#define FB_GET_CLOCK_TICKS()				            __raw_readl(MXC_GPT_GPTCNT)

#define FB_DECLARE_WORK(n, f)                           DECLARE_WORK(n, f)
#define FB_WORK_PARAM                                   struct work_struct *unused

#endif  // --------------------------------

#include <asm/types.h>
#include <plat/einkfb.h>

#define __EXIT_CODE             __exit
#define PRAGMAS                 0

#define EINKFB_NAME             "eink_fb"
#define EINKFB_EVENTS           "eink_events"
#define EINKFB_RESET_FILE       "/tmp/.einkfb_reset_file"

#define EINKFB_LOGGING_MASK(m)  (m & einkfb_logging)
#define EINKFB_DEBUG_FULL       (einkfb_logging_debug | einkfb_logging_debug_full)
#define EINKFB_DEBUG()          EINKFB_LOGGING_MASK(EINKFB_DEBUG_FULL)
#define EINKFB_PERF()           EINKFB_LOGGING_MASK(einkfb_logging_perf)

#define einkfb_print(i, f, ...)                                         \
    printk(EINKFB_NAME ": " i " %s:%s:" f,                              \
        __FUNCTION__, _LSUBCOMP_DEFAULT, ##__VA_ARGS__)

#define EINKFB_PRINT(f, ...)                                            \
    einkfb_print(LLOG_MSG_ID_INFO, f, ##__VA_ARGS__)

#define einkfb_print_crit(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_crit))                       \
        einkfb_print(LLOG_MSG_ID_CRIT, f, ##__VA_ARGS__)

#define einkfb_print_error(f, ...)                                      \
    if (EINKFB_LOGGING_MASK(einkfb_logging_error))                      \
        einkfb_print(LLOG_MSG_ID_ERR, f, ##__VA_ARGS__)

#define einkfb_print_warn(f, ...)                                       \
        einkfb_print(LLOG_MSG_ID_WARN, f, ##__VA_ARGS__)

#define einkfb_print_info(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_info))                       \
        einkfb_print(LLOG_MSG_ID_INFO, f, ##__VA_ARGS__)

#define einkfb_print_perf(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_perf))                       \
        einkfb_print(LLOG_MSG_ID_PERF, f, ##__VA_ARGS__)

#define EINKFB_PRINT_PERF_REL(i, t, n)                                  \
    einkfb_print_perf("id=%s,time=%ld,type=relative:%s\n", i, t, n)
    
#define EINKFB_PRINT_PERF_ABS(i, t, n)                                  \
    einkfb_print_perf("id=%s,time=%ld,type=absolute:%s\n", i, t, n)

#define einkfb_print_debug(f, ...)                                      \
    einkfb_print(LLOG_MSG_ID_DEBUG, f,  ##__VA_ARGS__)

#define einkfb_debug_full(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_debug_full))                 \
        einkfb_print_debug(f, ##__VA_ARGS__)

#define einkfb_debug(f, ...)                                            \
    if (EINKFB_LOGGING_MASK(EINKFB_DEBUG_FULL))                         \
        einkfb_print_debug(f, ##__VA_ARGS__)

#define einkfb_debug_lock(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_debug_lock))                 \
        einkfb_print_debug(f, ##__VA_ARGS__)

#define einkfb_debug_power(f, ...)                                      \
    if (EINKFB_LOGGING_MASK(einkfb_logging_debug_power))                \
        einkfb_print_debug(f, ##__VA_ARGS__)

#define einkfb_debug_ioctl(f, ...)                                      \
    if (EINKFB_LOGGING_MASK(einkfb_logging_debug_ioctl))                \
        einkfb_print_debug(f, ##__VA_ARGS__)

#define einkfb_debug_poll(f, ...)                                       \
    if (EINKFB_LOGGING_MASK(einkfb_logging_debug_poll))                 \
        einkfb_print_debug(f, ##__VA_ARGS__)

enum einkfb_logging_level
{
    einkfb_logging_crit         = LLOG_LEVEL_CRIT,    // Crash - can't go on as is.
    einkfb_logging_error        = LLOG_LEVEL_ERROR,   // Environment not right.
    einkfb_logging_warn         = LLOG_LEVEL_WARN,    // Oops, some bug.
    einkfb_logging_info         = LLOG_LEVEL_INFO,    // FYI.
    einkfb_logging_perf         = LLOG_LEVEL_PERF,    // Performance.

    einkfb_logging_debug        = LLOG_LEVEL_DEBUG0,  // Miscellaneous general debugging.
    einkfb_logging_debug_lock   = LLOG_LEVEL_DEBUG1,  // Mutex lock/unlock debugging.
    einkfb_logging_debug_power  = LLOG_LEVEL_DEBUG2,  // Power Management debugging.
    einkfb_logging_debug_ioctl  = LLOG_LEVEL_DEBUG3,  // Debugging for ioctls.
    einkfb_logging_debug_poll   = LLOG_LEVEL_DEBUG4,  // Debugging for polls.
    
    einkfb_logging_debug_full   = LLOG_LEVEL_DEBUG9,  // Full, general debugging.
    
    einkfb_logging_none         = 0x00000000,
    einkfb_logging_all          = LLOG_LEVEL_MASK_ALL,
    
    einkfb_logging_default      = _LLOG_LEVEL_DEFAULT,
    einkfb_debugging_default    = einkfb_logging_default | einkfb_logging_debug_power | einkfb_logging_debug_ioctl
};
typedef enum einkfb_logging_level einkfb_logging_level;

extern unsigned long einkfb_logging;

#define EINKFB_LOCK_ENTRY()     einkfb_lock_entry((char *)__FUNCTION__)
#define EINKFB_LOCK_EXIT()      einkfb_lock_exit((char *)__FUNCTION__)

#define EINKFB_LOCK_READY_NOW() einkfb_lock_ready(true)
#define EINKFB_LOCK_READY()     einkfb_lock_ready(false)
#define EINFFB_LOCK_RELEASE()   einkfb_lock_release()

#define EINKFB_TIMEOUT_MIN      1UL
#define EINKFB_TIMEOUT_MAX      10UL

#define TIMER_EXPIRES_SECS(t)	(jiffies + (HZ * (t)))
#define TIMER_EXPIRES_JIFS(t)   (jiffies + (t))
#define MIN_SCHEDULE_TIMEOUT	0

#define TIME_SECONDS			CLOCK_TICK_RATE
#define TIME_MILLISECONDS(t)	(((t) * 10)/(TIME_SECONDS/100))

#define EINKFB_MEMCPY_MIN       ((824 * 1200)/4)

#define BPP_BYTE_ALIGN(b)       (8/(b))
#define BPP_BYTE_ALIGN_DN(r, b) (((r)/BPP_BYTE_ALIGN(b))*BPP_BYTE_ALIGN(b))
#define BPP_BYTE_ALIGN_UP(r, b) (BPP_BYTE_ALIGN_DN(r, b) + BPP_BYTE_ALIGN(b))
#define NOT_BYTE_ALIGNED()      ((einkfb_bounds_failure_x == einkfb_get_last_bounds_failure()))

#define EINKFB_1BPP             EINK_1BPP
#define EINKFB_2BPP             EINK_2BPP
#define EINKFB_4BPP             EINK_4BPP
#define EINKFB_8BPP             EINK_8BPP
#define EINKFB_BPP_MAX          EINK_BPP_MAX

#define EINKFB_WHITE            EINK_WHITE  // For whacking all the pixels in a...
#define EINKFB_BLACK            EINK_BLACK  // ...byte (8, 4, 2, or 1) at once.

#define EINKFB_ORIENT_LANDSCAPE EINK_ORIENT_LANDSCAPE
#define EINKFB_ORIENT_PORTRAIT  EINK_ORIENT_PORTRAIT

#define EINKFB_FAILURE          true
#define EINKFB_SUCCESS          false

#define EINKFB_SUSPEND          true
#define EINKFB_RESUME           false

#define EINKFB_IOCTL_FAILURE    (-EINVAL)
#define EINKFB_EVENT_FAILURE    (-EINVAL)

#define EINKFB_IOCTL_DONE       true        // einkfb_ioctl_hook result

#define EINKFB_IOCTL_FROM_USER  true        // einkfb_memcpy() user-space direction
#define EINKFB_IOCTL_TO_USER    false       //

#define EINKFB_KERNEL_MEMCPY    false       // einkfb_memcpy() kernel-only

#define EINKFB_IOCTL_KERN       0x6B6E6965	// Kernel case ('eink').
#define EINKFB_IOCTL_USER       0x00000000  // User-space case.

#define EINKFB_IOCTL(c, a)      einkfb_ioctl_dispatch(EINKFB_IOCTL_KERN, NULL, c, a)
#define EINKFB_MEMCPYUF(d,s,l)  einkfb_memcpy(EINKFB_IOCTL_FROM_USER, EINKFB_IOCTL_USER, d, s, l)
#define EINKFB_MEMCPYUT(d,s,l)  einkfb_memcpy(EINKFB_IOCTL_TO_USER, EINKFB_IOCTL_USER, d, s, l)
#define EINKFB_MEMCPYK(d,s,l)   einkfb_memcpy(EINKFB_KERNEL_MEMCPY, EINKFB_IOCTL_KERN, d, s, l)

#define DEVICE_MODE_RW			(S_IWUGO | S_IRUGO)
#define DEVICE_MODE_R		    S_IRUGO
#define DEVICE_MODE_W		    S_IWUGO

#define EINKFB_PROC_PARENT	    (S_IFDIR | S_IRUGO | S_IXUGO)
#define EINKFB_PROC_CHILD_RW	DEVICE_MODE_RW
#define EINKFB_PROC_CHILD_R     DEVICE_MODE_R
#define EINKFB_PROC_CHILD_W     DEVICE_MODE_W

#define EINKFB_BLANK(b)         einkfb_blank(b, NULL)
#define EINKFB_EXPIRE_TIMER		false
#define EINKFB_DELAY_TIMER      true

#define EINKFB_FLASHING_ROM()   einkfb_get_flash_mode()
#define EINKFB_NEEDS_RESET()    einkfb_set_reset(true)
#define EINKFB_RESETTING()      einkfb_get_reset()

#define STRETCH_HI_NYBBLE(n, b) einkfb_stretch_nybble(((0xF0 & n) >> 4), b)
#define STRETCH_LO_NYBBLE(n, b) einkfb_stretch_nybble(((0x0F & n) >> 0), b)

#define EINKFB_RESTORE(i)       ((i).vfb == (i).start)

#define EINKFB_PROC_SYSFS_RW_LOCK(a0, a1, a2, a3, a4, a5, a7)       \
    einkfb_proc_sysfs_rw(a0, a1, a2, a3, a4, a5, true, a7)

#define EINKFB_PROC_SYSFS_RW_NO_LOCK(a0, a1, a2, a3, a4, a5, a7)    \
    einkfb_proc_sysfs_rw(a0, a1, a2, a3, a4, a5, false, a7)

#define einkfb_schedule_timeout_interruptible(t, r, d)              \
    einkfb_schedule_timeout(t, r, d, true)

#define EINKFB_SCHEDULE_TIMEOUT(t, r)                               \
    einkfb_schedule_timeout(t, r, NULL, false)

#define EINKFB_SCHEDULE_TIMEOUT_INTERRUPTIBLE(t, r)                 \
    einkfb_schedule_timeout_interruptible(t, r, NULL)

#define EINKFB_SCHEDULE()                                           \
    EINKFB_SCHEDULE_TIMEOUT(0, 0)

#define EINKFB_SCHEDULE_BLIT(b)                                     \
    if ( 0 == ((b) % EINKFB_MEMCPY_MIN) )                           \
        EINKFB_SCHEDULE()

#define POWER_OVERRIDE_BEGIN()  einkfb_power_override_begin()
#define POWER_OVERRIDE_END()    einkfb_power_override_end()
#define POWER_OVERRIDE()        (true == einkfb_get_power_override_state())

// HAL module operations.
//
#define HAL_SW_INIT(v, f)       (hal_ops.hal_sw_init ? hal_ops.hal_sw_init(v, f) : EINKFB_FAILURE)
#define HAL_SW_DONE()           if (hal_ops.hal_sw_done) hal_ops.hal_sw_done()

#define FULL_BRINGUP            true  // Bring hardware up from scratch.
#define DONT_BRINGUP            false // Do minimal hardware bring-up (e.g., avoid disturbing the screen).

#define FULL_SHUTDOWN           true  // Shut the hardware completely down.
#define DONT_SHUTDOWN           false // Do minimal hardware shutdown (e.g., avoid disturbing the screen).

enum einkfb_power_level
{
    einkfb_power_level_init = -1,

    einkfb_power_level_on = 0,
    einkfb_power_level_blank,
    
    einkfb_power_level_standby,
    einkfb_power_level_sleep,
    
    einkfb_power_level_off
};
typedef enum einkfb_power_level einkfb_power_level;

struct einkfb_hal_ops_t
{
    // Required operation, as the base eInk HAL driver doesn't implement any defaults
    // for screeninfo (i.e., the HAL itself isn't standalone).  Shouldn't require
    // hardware to be alive to be called.  The corresponding done call doesn't need
    // to be implemented if there's no corresponding work to do.
    //
    // Note:  Because the HAL exposes a memory-based framebuffer to /dev/fb*, the
    //        largest possible resolution and deepest depth should be returned here.
    //        It can always be pruned back at hal_hw_init time if necessary.
    //
    bool (*hal_sw_init)(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix);
    void (*hal_sw_done)(void);
    
    // Optional operation:  Should initialize the hardware and modify the fb_info fields
    // as necessaary to match the hardware.  The init routine returns whether it fully
    // initialized the hardware or not.
    //
    bool (*hal_hw_init)(struct fb_info *info, bool full);
    void (*hal_hw_done)(bool full);
    
    // Optional operation:  Create and remove any procfs/sysfs entries in the eInk HAL's
    // directory when requested to do so.
    //
    void (*hal_create_proc_entries)(void);
    void (*hal_remove_proc_entries)(void);
    
    // Optional operation:  Should cause the entire contents of einkfb to show up on
    // the display; refresh the display fully if requested, refresh partially,
    // otherwise.
    //
    // Note:  If writing to einkfb directly doesn't cause the contents to show up on
    // the display, then this is a required operation.
    //
    void (*hal_update_display)(fx_type update_mode);
    
    // Optional operation:  See einkfb.h for update_area_t.  If non-nil, the contents of
    // buffer need to be displayed at (x1, y1)..(x2, y2).  If buffer is nil, then the
    // contents of (x1, y1)..(x2, y2) in einkfb need to show up on the display.
    //
    // Note:  The rectangle defined by (x1, y1)..(x2, y2) should be within the bounds of the
    // the display, and, for non-nil buffers, should be a fully rasterized subset of einkfb
    // as it exists (e.g., at the same bit depth) at the time of the call.
    //
    void (*hal_update_area)(update_area_t *update_area);
    
    // Optional operation:  On set, should set the controller and/or display to the
    // corresponding power level; the get call just returns the current level.  The
    // levels are:
    //
    //      einkfb_power_level_on:        normal operation mode (used internally)
    //      einkfb_power_level_blank:     clear screen but stay in normal operation mode (not returned)
    //
    //      einkfb_power_level_standby:   power/speed reduced, can update screen
    //      einkfb_power_level_sleep:     power/speed minimal, can't update screen
    //
    //      einkfb_power_level_off:       power off, screen clear
    //
    void (*hal_set_power_level)(einkfb_power_level power_level);
    einkfb_power_level (*hal_get_power_level)(void);
    
    // Optional operation:  Hook into the reboot/shutdown process for anything that needs
    // to be done before the system is rebooted or shut down.
    //
    void (*hal_reboot_hook)(reboot_behavior_t behavior);
    
    // Optional operation:  If the horizontal byte-alignment is not based directly on bits-per-pixel,
    // tell the eInk HAL what it is (i.e., use whatever value makes the BPP_BYTE_ALIGN() macro work
    // properly).
    //
    unsigned long (*hal_byte_alignment)(void);
    
    // Optional operation:  On set, the controller should put the display in the specified
    // orientation and return whether it did so or not.  The get call just returns the current
    // orientation.
    //
    bool (*hal_set_display_orientation)(orientation_t orientation);
    orientation_t (*hal_get_display_orientation)(void);
};
typedef struct einkfb_hal_ops_t einkfb_hal_ops_t;

extern einkfb_hal_ops_t hal_ops;

// For use with einkfb_get_info().
//
struct einkfb_info
{
    bool                   init,    // how the hardware was brought up
                           done;    // how the hardware will be brought down
    
    unsigned long          size,    // screen size
                           blen,    // buffer size
                           mem,     // memory size
                           bpp;     // bits per pixel
    
    int                    xres,    // width in pixels
                           yres;    // ditto height
                    
    unsigned char          *start,  // real framebuffer's start
                           *vfb,    // virtual framebuffer
                           *buf;    // scratch buffer
    
    struct platform_device *dev;    // platform device
    struct fb_info         *fbinfo; // fb info
    struct miscdevice      *events; // events
    
    unsigned long          jif_on,  // jiffie tick at last power-on
                           align;   // byte alignment if not based on bpp
};

// For use with einkfb_set_info_hook().
//
typedef void (*einkfb_info_hook_t)(struct einkfb_info *info);

// For use with the einkfb_set_reboot_hook().
//
typedef void (*einkfb_reboot_hook_t)(reboot_behavior_t behavior);

// For use with the einkfb_set_suspend_resume_hook();
//
typedef int (*einkfb_suspend_resume_hook_t)(bool which);

// For use with einkfb_set_ioctl_hook().
//
enum einkfb_ioctl_hook_which
{
    einkfb_ioctl_hook_post,
    einkfb_ioctl_hook_pre,
    
    einkfb_ioctl_hook_insitu_begin,
    einkfb_ioctl_hook_insitu_end
};
typedef enum einkfb_ioctl_hook_which einkfb_ioctl_hook_which;

typedef bool (*einkfb_ioctl_hook_t)(einkfb_ioctl_hook_which which, unsigned long flag, unsigned int *cmd, unsigned long *arg);

// For use with einkfb_proc_sysfs_rw(), einkfb_read_which(), and einkfb_write_which().
//
typedef int  (*einkfb_rw_proc_sysf_t)(char *r_page_w_buf, unsigned long r_off_w_count, int r_count);
typedef int  (*einkfb_read_t)(unsigned long addr, unsigned char *data, int count);
typedef int  (*einkfb_write_t)(unsigned long start, unsigned char *data, unsigned long len);

// For use with einkfb_get_last_bounds_failure().
//
enum einkfb_bounds_failure
{
    einkfb_bounds_failure_x1    = 1 << 0,   // Coordinate x1 not in xres range.
    einkfb_bounds_failure_y1    = 1 << 1,   // Coordinate y1 not in yres range.
    einkfb_bounds_failure_x2    = 1 << 2,   // x1 + x2 exceeds xres.
    einkfb_bounds_failure_y2    = 1 << 3,   // y1 + y2 exceeds yres.
    
    einkfb_bounds_failure_x     = 1 << 4,   // Not byte aligned horizontally.
    einkfb_bounds_failure_y     = 1 << 5,   // Not byte aligned vertically (not yet used).

    einkfb_bounds_failure_none  = 0
};
typedef enum einkfb_bounds_failure einkfb_bounds_failure;

// For use with einkfb_schedule_timeout().
//
typedef bool (*einkfb_hardware_ready_t)(void *data);

// For use with einkfb_blit()
//
typedef void (*einkfb_blit_t)(int x, int y, int rowbytes, int bytes, void *data);

// From einkfb_hal_main.c:
//
extern void einkfb_set_info_hook(einkfb_info_hook_t info_hook);
extern void einkfb_get_info(struct einkfb_info *info);

extern void einkfb_set_res_info(struct fb_info *info, int xres, int yres);
extern void einkfb_set_fb_restore(bool fb_restore);
extern void einkfb_set_jif_on(unsigned long jif_on);

extern void einkfb_set_reboot_behavior(reboot_behavior_t behavior);
extern reboot_behavior_t einkfb_get_reboot_behavior(void);

extern void einkfb_set_reboot_hook(einkfb_reboot_hook_t reboot_hook);
extern void einkfb_set_suspend_resume_hook(einkfb_suspend_resume_hook_t suspend_resume_hook);

extern int  einkfb_hal_ops_init(einkfb_hal_ops_t *einkfb_hal_ops);
extern void einkfb_hal_ops_done(void);

// From einkfb_hal_util.c:
//
extern bool einkfb_lock_ready(bool release);
extern void einkfb_lock_release(void);

extern int  einkfb_lock_entry(char *function_name);
extern void einkfb_lock_exit(char *function_name);

extern int  einkfb_schedule_timeout(unsigned long hardware_timeout, einkfb_hardware_ready_t hardware_ready, void *data, bool interruptible);
extern void einkfb_blit(int xstart, int xend, int ystart, int yend, einkfb_blit_t blit, void *data);

extern einkfb_bounds_failure einkfb_get_last_bounds_failure(void);
extern bool einkfb_bounds_are_acceptable(int xstart, int xres, int ystart, int yres);
extern bool einkfb_align_bounds(rect_t *rect);
extern unsigned char einkfb_stretch_nybble(unsigned char nybble, unsigned long bpp);
extern void einkfb_display_grayscale_ramp(void);

extern void einkfb_update_display_area(update_area_t *update_area);
extern void einkfb_update_display(fx_type update_mode);
extern void einkfb_restore_display(fx_type update_mode);

extern void einkfb_clear_display(fx_type update_mode);

extern bool einkfb_set_display_orientation(orientation_t orientation);
extern orientation_t einkfb_get_display_orientation(void);

// From einkfb_hal_proc.c
//
extern bool einkfb_get_flash_mode(void);

extern bool einkfb_get_reset(void);
extern void einkfb_set_reset(bool reset);

extern int einkfb_read_which(char *page, unsigned long off, int count, einkfb_read_t which_read, int which_size);
extern int einkfb_write_which(char *buf, unsigned long count, einkfb_write_t which_write, int which_size);

extern int einkfb_proc_sysfs_rw(char *r_page_w_buf, char **r_start, unsigned long r_off_w_count, int r_count,
	int *r_eof, int r_eof_len, bool lock, einkfb_rw_proc_sysf_t einkfb_rw_proc_sysfs);

extern struct proc_dir_entry *einkfb_create_proc_entry(const char *name, mode_t mode, read_proc_t *read_proc, write_proc_t *write_proc);
extern void einkfb_remove_proc_entry(const char *name, struct proc_dir_entry *entry);

extern bool einkfb_create_proc_entries(void);
extern void einkfb_remove_proc_entries(void);

// From einkfb_hal_mem.c:
//
extern int  einkfb_mmap(FB_MMAP_PARAMS);

extern int  einkfb_alloc_page_array(void);
extern void einkfb_free_page_array(void);

extern void *einkfb_malloc(size_t size);
extern void einkfb_free(void *ptr);

extern int  einkfb_memcpy(bool direction, unsigned long flag, void *destination, const void *source, size_t length);
extern void einkfb_memset(void *destination, int value, size_t length);

// From einkfb_hal_io.c:
//
extern char *einkfb_get_cmd_string(unsigned int cmd);

extern int  einkfb_ioctl_dispatch(unsigned long flag, struct fb_info *info, unsigned int cmd, unsigned long arg);
extern int  einkfb_ioctl(FB_IOCTL_PARAMS);

extern void einkfb_set_ioctl_hook(einkfb_ioctl_hook_t ioctl_hook);

// From einkfb_hal_pm.c
//
extern einkfb_power_level einkfb_get_power_level(void);
extern void einkfb_set_power_level(einkfb_power_level power_level);
extern char *einkfb_get_power_level_string(einkfb_power_level power_level);

extern void einkfb_set_blank_unblank_level(einkfb_power_level power_level); // Call inside EINKFB_LOCK_ENTRY()/EINKFB_LOCK_EXIT().
extern void EINKFB_SET_BLANK_UNBLANK_LEVEL(einkfb_power_level power_level); // Calls EINKFB_LOCK_ENTRY()/EINKFB_LOCK_EXIT().

extern int  einkfb_blank(int blank, struct fb_info *info);

extern void einkfb_start_power_timer(void);
extern void einkfb_stop_power_timer(void);

extern void einkfb_prime_power_timer(bool delay_timer);

extern int  einkfb_get_power_timer_delay(void);
extern void einkfb_set_power_timer_delay(int power_timer_delay);

extern void einkfb_power_override_begin(void);
extern void einkfb_power_override_end(void);
extern bool einkfb_get_power_override_state(void);

// From einkfb_hal_events.c
//
extern void einkfb_init_event(einkfb_event_t *event);
extern void einkfb_post_event(einkfb_event_t *event);

extern int einkfb_events_open(struct inode *inode, struct file *file);
extern int einkfb_events_release(struct inode *inode, struct file *file);

extern ssize_t einkfb_events_read(struct file *file, char *buf, size_t count, loff_t *ofs);
extern unsigned int einkfb_events_poll(struct file *file, poll_table *wait);

#endif // _EINKFB_HAL_H
