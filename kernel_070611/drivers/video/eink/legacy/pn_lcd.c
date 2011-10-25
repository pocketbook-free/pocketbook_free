/******************************************************************
 *
 *  File:   pn_lcd.c
 *
 *  Author: Nick Vaccaro <nvaccaro@lab126.com>
 *
 *  Date:   12/07/05
 *
 * Copyright 2005-2006, Lab126, Inc.  All rights reserved.
 *
 *  Description:
 *      Driver for pn_lcd hardware.  This driver currently talks to
 *      the IOC driver to get to the pnlcd hardware.
 *
 ******************************************************************/

#include <linux/autoconf.h>
#include <stdarg.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/errno.h>
#include <asm/string.h>
#include <linux/string.h>
#include <asm/uaccess.h>
//#include <asm/arch/hardware.h>
#include <mach/hardware.h>

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
//#include <linux/fb.h>
#include <plat/einkfb.h>

//#include <linux/einkfb.h>

#define __PN_LCD_C_
#ifdef CONFIG_IOC
#include <asm/arch/boot_globals.h>
#include <linux/ioc.h>
#include <linux/pn_lcd.h>

#include "ioc_protocol.h"
#include "pnlcd_pm.h"
#else
#include "einkfb_pnlcd.h"

static int
PNLCD_Ioctl_Sys(unsigned int inCommand, unsigned long inArg)
{
    return ( pnlcd_sys_ioctl(inCommand, inArg) );
}
#endif
#undef __PN_LCD_C_

////////////////////////////////////////////////////////
// External References 
///////////////////////////////////////////////////////
extern FPOW_POWER_MODE g_pnlcd_power_mode;
extern struct file_operations ioc_fops;
extern int g_ioc_driver_present;

extern void set_pnlcd_segments(char *segments);
extern void restore_pnlcd_segments(void);
extern char *get_pnlcd_segments(void);

extern FPOW_ERR pnlcd_fpow_setmode(void *private, u32 state, u32 mode);

////////////////////////////////////////////////////////
// Forward References 
///////////////////////////////////////////////////////
static void pnlcd_blocking_dispatch(pnlcd_blocking_cmds which_blocking_cmd);

////////////////////////////////////////////////////////
// Local Defines
///////////////////////////////////////////////////////
#define PNLCD_DRIVER_VERSION    "v0.95"

////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////
static unsigned short g_enable = 0; // Remember enable state to prevent repeats.
static int g_progress_bar = 0;      // Used to show progress during the boot process.
static int g_dont_disable = 0;      // Don't disable progress bar when non-zero.
static int g_user_count = 0;
static int g_ioc_fd = 0;

static int g_fake_pnlcd_vert_count = 0;
static int g_pnlcd_driver_inited = 0;

//////////////////////////////
// Macros
/////////////////////////////
#define my_sys_ioctl(x,y,z)                                                                        \
    (pnlcd_awake() ? ioc_fops.ioctl((struct inode*)0xDEADBEEF,(struct file*)x,y,(unsigned long)z)  \
                   : 0)

#define CHECK_FRAMEWORK_RUNNING() (FRAMEWORK_STARTED() && !FRAMEWORK_RUNNING())

static inline int Is_IOC_Open(void)  {
return(1);  // FIXME
    return(g_ioc_fd != 0);
}

static inline int Is_PNLCD_Open(void) {
    return(g_user_count != 0);
}

//////////////////////////////
// Core Driver Prototypes
//////////////////////////////

#define PNLCD_OPEN()    PNLCD_Open(NULL, NULL)
#define PNLCD_CLOSE()   PNLCD_Close(NULL, NULL)

static int PNLCD_Open(struct inode* inNode, struct file* inFile);
static int PNLCD_Close(struct inode* inNode, struct file* inFile);
//static ssize_t PNLCD_Write(struct file *fp, const char *buf, size_t count, loff_t *ppos);
static int PNLCD_Ioctl(struct inode *inNode,
                struct file *inFile,
                unsigned int inCommand,
                unsigned long inArg);

// Core Driver Init and Destroy Functions
static int PNLCD_Driver_Init(void);
static void PNLCD_Driver_Destroy(void);

// Power Management
#ifdef LOCAL_CONFIG_PM
int pnlcd_suspend(u32 state);
int pnlcd_resume(u32 state);
#endif

// IOC driver-related prototypes
static int Do_Open_IOC(void);
static int Do_Close_IOC(void);

// HW Function Enable/Disable Routines
static int  Do_Enable_PNLCD(unsigned short enable);

// Segment On/Off Functions
static int  PNLCD_IOC_Sequential_Segment(unsigned short on, int startSeg, int endSeg, int vertical);
static int  PNLCD_IOC_Multi_Segment(unsigned short on, int num_segments, int *segment_array);
static int  PNLCD_IOC_Multi_Segment_Pairs(unsigned short on, int num_pairs, int *paired_segment_array, int vertical);

// /proc interface
static int lcd_proc_init(void);
static void lcd_cleanup_procfs(void);

static int driver_not_inited(void);
static int driver_not_inited(void) {
    return(!g_pnlcd_driver_inited);
}

/////////////////////////////////////////////
// HW Function Enable/Disable Routines
/////////////////////////////////////////////

static inline void 
IOC_Enable_Err(char *target, unsigned short enable, int rc, int size)
{
    printk("ERROR: Invalid response %sabling %s (only %d bytes returned,"
            " should be %d bytes)\n",
            (enable == ENABLE)?"en":"dis",target,rc,size);
}

static int
Do_Enable_PNLCD(unsigned short enable)
{
    int retVal = 0;
    
    IOC_ENABLE_REC enableRec;

    // Call IOC driver to enable or disable PN-LCD
    enableRec.enable = enable ? ENABLE : (g_dont_disable ? ENABLE: DISABLE);
    
    if (g_enable != enableRec.enable)
        retVal = my_sys_ioctl(g_ioc_fd, IOC_PNLCD_ENABLE_IOCTL, (unsigned long)&enableRec);

    if (0 == retVal)
        g_enable = enableRec.enable;

    return(retVal);
}

////////////////////////////////////////////////////////
//
// Segment On/Off Functions
//
////////////////////////////////////////////////////////

#define PROGRESS_BAR_BASE               (MAX_PN_LCD_SEGMENT - 1)
#define TICKS(t)                        (HZ/(t))

#define ANIMATION_THREAD_NAME           "pnlcd_animate"

#define num_animation_states            2
#define num_animation_ops               3

#define max_pnlcd_animation_frames      4

#define max_pnlcd_animation_segments    8
#define num_sp_animation_segments       4
#define num_usb_segments                4
#define sp_animation_stop_frame         0x00

#define animation_type_valid(a)         \
    ((sp_animation_rotate <= (a)) && (num_sp_animations > (a)))
#define frame_rate_valid(r)             \
    ((minimum_pnlcd_animation_rate <= (r)) && (maximum_pnlcd_animation_rate >= (r)))
#define ignore_frame_rate()             \
    (pnlcd_animation_ignore_rate == g_animation_op)

struct pnlcd_animation_sp
{
    int num_segments_per_frame,
        num_frames;

    unsigned char *segments,
                  frames[max_pnlcd_animation_frames];
};
typedef struct pnlcd_animation_sp pnlcd_animation_sp;

static unsigned char sp_animation_segments[num_sp_animation_segments] =
{
    0xC4, // +---------+     +---------+
    0xC6, // |0xC5|0xC4| --\ |bit3|bit0| --\ The bottom-most four...
    0xC7, // |0xC7|0xC6| --/ |bit2|bit1| --/ ...segments of the PNLCD.
    0xC5  // +---------+     +---------+
};

static unsigned char usb_internal_segments[num_usb_segments] =
{
    0xBE, // +---------+     +---------+
    0xC0, // |0xBF|0xBE| --\ |bit3|bit0| 
    0xC1, // |0xC1|0xC0| --/ |bit2|bit1| 
    0xBF  // +---------+     +---------+
};

static unsigned char usb_external_segments[num_usb_segments] =
{
    0xBE, // +---------+     +---------+
    0xC0, // |0xBF|0xBE| --\ |bit3|bit0| 
    0xC1, // |0xC1|0xC0| --/ |bit2|bit1| 
    0xBF  // +---------+     +---------+
};

static pnlcd_animation_sp sp_animations[num_sp_animations] =
{
    {
        num_sp_animation_segments, 4, sp_animation_segments, { 0x01, 0x02, 0x04, 0x08 } // sp_animation_rotate
    },

    {
        num_sp_animation_segments, 4, sp_animation_segments, { 0x0E, 0x0D, 0x0B, 0x07 } // sp_animation_march
    },

    {
        num_sp_animation_segments, 2, sp_animation_segments, { 0x0A, 0x05, 0x00, 0x00 } // sp_animation_dice
    },

    {
        num_sp_animation_segments, 2, sp_animation_segments, { 0x0C, 0x03, 0x00, 0x00 } // sp_animation_pong
    },
    
    {
        num_usb_segments,          4, usb_internal_segments, { 0x0F, 0x0F, 0x0F, 0x00 } // sp_animation_usb_internal_read
    },
    
    {
        num_usb_segments,          4, usb_internal_segments, { 0x0F, 0x0F, 0x0F, 0x00 } // sp_animation_usb_internal_write
    },
    
    {
        num_usb_segments,          4, usb_external_segments, { 0x0F, 0x0F, 0x0F, 0x00 } // sp_animation_usb_external_read
    },
    
    {
        num_usb_segments,          4, usb_external_segments, { 0x0F, 0x0F, 0x0F, 0x00 } // sp_animation_usb_external_write
    },
    
    {
        num_sp_animation_segments, 4, sp_animation_segments, { 0x08, 0x04, 0x02, 0x01 } // sp_animation_rotate_reverse
    },
};

static char *pnlcd_animation_state_names[num_animation_states] =
{
    "started",                                                  // start_animation
    "stopped"                                                   // stop_animation
};

static char *pnlcd_animation_op_names[num_animation_ops] =
{
    "automatic",                                                // pnlcd_animation_auto
    "manual",                                                   // pnlcd_animation_manual
    "ignore rate"                                               // pnlcd_animation_ignore_rate
};

static char *pnlcd_animation_sp_animation_names[num_sp_animations] =
{
    "rotate",                                                   // sp_animation_rotate
    "march",                                                    // sp_animation_march
    "dice",                                                     // sp_animation_dice
    "pong",                                                     // sp_animation_pong
    
    "USB internal read",                                        // sp_animation_usb_internal_read
    "USB internal write",                                       // sp_animation_usb_internal_write
    "USB external read",                                        // sp_animation_usb_external_read
    "USB external write",                                       // sp_animation_usb_external_write
    
    "rotate reverse"                                            // sp_animation_rotate_reverse
};

static int                      g_animation_stop_delayed = 0;   // Remember if stopping animation was delayed.

static pnlcd_animations         g_animation_type  = sp_animation_rotate;
static pnlcd_animation_cmds     g_animation_state = stop_animation;
static unsigned long            g_frame_rate      = default_pnlcd_animation_rate;
static int                      g_animation_op    = pnlcd_animation_manual;
static int                      g_next_frame      = 0;
static unsigned long            g_start_time      = 0;

static int g_animation_thread_awake = 0;
static int g_animation_thread_exit  = 0;
static THREAD_ID(g_animation_thread_id);
static DECLARE_COMPLETION(g_animation_thread_exited);
static DECLARE_COMPLETION(g_animation_thread_complete);

static void
draw_frame(int num_animation_segments, char *segments, unsigned char frame)
{
    unsigned int seg_array_on[max_pnlcd_animation_segments],  num_segs_on,
                 seg_array_off[max_pnlcd_animation_segments], num_segs_off,
                 delayed = 0, i;

    PNLCD_MISC_SEGMENTS_REC rec;
    
    // Decode frame into on & off segments.
    //
    for ( i = num_segs_off = num_segs_on = 0; i < num_animation_segments; i++ )
    {
        if ( SEGMENT_ON == ((frame >> i) & SEGMENT_ON) )
            seg_array_on[num_segs_on++]   = segments[i];
        else
            seg_array_off[num_segs_off++] = segments[i];    
    }

    // Animations are allowed during delays.
    //
    if ( pnlcd_delayed() )
    {
        // Stop PNLCD delays but block all PNLCD IOCTL traffic.
        //
        pnlcd_blocking_dispatch(stop_delay);
        pnlcd_blocking_dispatch(start_block);
        
        delayed = 1;
    }

    // Turn on any on-segments in frame.
    //
    if ( num_segs_on )
    {
        rec.enable    = SEGMENT_ON;
        rec.num_segs  = num_segs_on;
        rec.seg_array = seg_array_on;
        
        my_sys_ioctl(g_ioc_fd, IOC_PNLCD_MISC_IOCTL, &rec);
    }
    
    // Turn off any off-segments in frame.
    //
    if ( num_segs_off )
    {
        rec.enable    = SEGMENT_OFF;
        rec.num_segs  = num_segs_off;
        rec.seg_array = seg_array_off;
        
        my_sys_ioctl(g_ioc_fd, IOC_PNLCD_MISC_IOCTL, &rec);
    }
    
    // If the PNLCD was delayed, restore it to that state.
    //
    if ( delayed )
    {
        // Stop blocking PNLCD IOCTL traffic and restart
        // the delay.
        //
        pnlcd_blocking_dispatch(stop_block);
        pnlcd_blocking_dispatch(start_delay);
    }
}

static unsigned long
update_frame(void)
{
    unsigned long timeout_time = TICKS(g_frame_rate);

    if ( ignore_frame_rate() || (0 == g_start_time) || time_after(jiffies, (g_start_time + timeout_time)) )
    {
        int num_animation_segments = sp_animations[g_animation_type].num_segments_per_frame;
        unsigned char frame = sp_animations[g_animation_type].frames[g_next_frame++],
                      *segments = sp_animations[g_animation_type].segments;
        
        draw_frame(num_animation_segments, segments, frame);
        
        g_next_frame %= sp_animations[g_animation_type].num_frames;
        g_start_time  = jiffies;
    }
    else
        timeout_time /= 2;
        
    return ( timeout_time );
}

static void
stop_frame(void)
{
    int num_animation_segments = sp_animations[g_animation_type].num_segments_per_frame;
    unsigned char *segments = sp_animations[g_animation_type].segments;
    
    draw_frame(num_animation_segments, segments, sp_animation_stop_frame);
    g_start_time = g_next_frame = 0;
    
    g_frame_rate = default_pnlcd_animation_rate;
    g_animation_type = sp_animation_rotate;
}

static void
wake_animation_thread(void)
{
    complete(&g_animation_thread_complete);
}

static void
exit_animation_thread(void)
{
    g_animation_thread_exit = 1;
    wake_animation_thread();
}

static void
animation_thread_body(void)
{
    unsigned long timeout_time = MAX_SCHEDULE_TIMEOUT;
    
    if ( pnlcd_animation_auto == g_animation_op )
        timeout_time = update_frame();
    
    g_animation_thread_awake = 0;
    wait_for_completion_interruptible_timeout(&g_animation_thread_complete, timeout_time);
}

static int
animation_thread(void *data)
{
    int *exit_thread = (int *)data;
    int thread_active = 1;
    
    THREAD_HEAD(ANIMATION_THREAD_NAME);

    while ( thread_active )
    {
        g_animation_thread_awake = 1;

        TRY_TO_FREEZE();

        if ( !THREAD_SHOULD_STOP() )
        {
            THREAD_BODY(*exit_thread, animation_thread_body);
        }
        else
        {
            if ( pnlcd_animation_auto == g_animation_op )
            {
                g_animation_op = pnlcd_animation_manual;
             
                stop_frame();
             
                g_animation_state = stop_animation;
            }
            
            g_animation_thread_awake = thread_active = 0;
         }
    }

    THREAD_TAIL(g_animation_thread_exited);
}

static void
sleep_animation_thread(void)
{
    unsigned long start_check = jiffies, timeout_time = TICKS(g_frame_rate) * 2;

    while ( g_animation_thread_awake && !time_after(jiffies, (start_check + timeout_time)) )
        ;
}

static void
start_animation_thread(void)
{
    THREAD_START(g_animation_thread_id, &g_animation_thread_exit, animation_thread,
        ANIMATION_THREAD_NAME);
}

static void
stop_animation_thread(void)
{
    THREAD_STOP(g_animation_thread_id, exit_animation_thread, g_animation_thread_exited);
}

static int
animation_stop_delayed(void)
{
    return ( g_animation_stop_delayed );
}

static void
animate_pnlcd(pnlcd_animation_cmds cmd, int arg)
{
    int delayed = pnlcd_delayed();
    
    switch ( cmd )
    {
        // Only start when we're stopped.
        //
        case start_animation:
            if ( stop_animation == g_animation_state )
            {
                g_animation_state = start_animation; 
                g_animation_op = arg;
            
                if ( pnlcd_animation_auto == g_animation_op )
                    wake_animation_thread();
                else
                    update_frame();
            }
        break;
        
        // Only stop when we're started and the user doesn't need to wait (i.e.,
        // keep animating during user delays).
        //
        case stop_animation:
            if ( (start_animation == g_animation_state) && !delayed )
            {
                if ( pnlcd_animation_auto == g_animation_op )
                {
                    g_animation_op = pnlcd_animation_manual;
                    sleep_animation_thread();
                }

                stop_frame();

                g_animation_state = stop_animation;
                g_animation_stop_delayed = 0;
            }
            else
            {
                if ((start_animation == g_animation_state) && delayed)
                    g_animation_stop_delayed = 1;
            }
         break;
        
        // Only update when we're started and, even then, only update if we're not running
        // automatically and we haven't exceeded the designated frame rate.
        //
        case update_animation:
            if ( (start_animation == g_animation_state) && (pnlcd_animation_auto != g_animation_op) )
                update_frame();
        break;
        
        // Only set the animation rate and type when we're stopped.
        //
        case set_animation_rate:
            if ( stop_animation == g_animation_state )
                g_frame_rate = frame_rate_valid(arg) ? arg : default_pnlcd_animation_rate;
        break;
        
        case set_animation_type:
            if ( stop_animation == g_animation_state )
                g_animation_type = animation_type_valid(arg) ? arg : sp_animation_rotate;
        break;
    }
}

static void
clear_pnlcd(void)
{
    PNLCD_SEQ_SEGMENTS_REC rec;

    rec.enable    = SEGMENT_OFF;
    rec.start_seg = FIRST_PNLCD_SEGMENT;
    rec.end_seg   = LAST_PNLCD_SEGMENT;

    my_sys_ioctl(g_ioc_fd, IOC_PNLCD_SEQ_IOCTL, &rec);
}

static void
update_pnlcd_progress_bar(int n)
{
    if ( (n >= 0) && (n <= 100) )
    {
        if ( (n == 0) || (n < g_progress_bar) )
            clear_pnlcd();
            
        if ( n > 0 )
        {
            PNLCD_SEQ_SEGMENTS_REC rec;
            
            rec.enable    = SEGMENT_ON;
            rec.start_seg = PN_LCD_SEGMENT_COUNT - (n * 2);
            rec.end_seg   = PROGRESS_BAR_BASE;
            
            my_sys_ioctl(g_ioc_fd, IOC_PNLCD_VERT_SEQ_IOCTL, &rec);
        }
        
        g_progress_bar = n;
        g_dont_disable = 1;
        
        set_progress_bar_value(n);
        
        // This is the mechanism that determines whether the framework is running or not:
        // 
        // If the framework isn't currently running, but it's been started, and the
        // progress bar has reached 100%, then we've detected that the framework is
        // now running.
        //
        if ( CHECK_FRAMEWORK_RUNNING() && (100 == n) )
        {
            // Stop the framework-running detection mechanism and say
            // that the framework is now running.
            //
            set_framework_started(0);
            set_framework_running(1);
            
            // Refresh the logo for the framework.
            //
            eink_sys_ioctl(FBIO_EINK_SPLASH_SCREEN, splash_screen_logo);
            
            // If we're faking the PNLCD on the eInk display and we're hiding
            // the real PNLCD, the eInk driver's telling us to clear the PNLCD,
            // stop animation, etc., will have failed (i.e., the segments will
            // still be sitting at 100% instead of having been cleared).  So,
            // we side step things a bit here to achieve the appropriate effect.
            //
            if ( HIDE_PNLCD() )
                restore_pnlcd_segments();
         }
    }
    else
        g_dont_disable = 0;
}

static int
PNLCD_IOC_Sequential_Segment(unsigned short on, int startSeg, int endSeg, int vertical)
{
    int     err = 0;
    int     ioctl_id = 0;
    PNLCD_SEQ_SEGMENTS_REC  theRec;

    NDEBUG("@@@@ PNLCD_IOC_Sequential_Segment(on=%02x, start=%02x, end=%02x)\n",on,startSeg,endSeg);

    // Range-check values to assure segment < 200
    if (startSeg > MAX_PN_LCD_SEGMENT) {
        NDEBUG("@@@@ PNLCD_IOC_Sequential_Segment(start=%02x out of range)\n",startSeg);
        err = startSeg;
        goto exit;
    }
    else if (endSeg > MAX_PN_LCD_SEGMENT)  {
        NDEBUG("@@@@ PNLCD_IOC_Sequential_Segment(end=%02x out of range)\n",endSeg);
        err = endSeg;
        goto exit;
    }

    // Prepare calling structure
    theRec.enable = on;
    theRec.start_seg = startSeg;
    theRec.end_seg = endSeg;
    ioctl_id = (vertical ? IOC_PNLCD_VERT_SEQ_IOCTL : IOC_PNLCD_SEQ_IOCTL);
    err = my_sys_ioctl(g_ioc_fd, ioctl_id, &theRec);

exit:
    NDEBUG("@@@@ PNLCD_IOC_Sequential_Segment() returning %d\n",err);
    return(err);  // Return Error - not implemented yet
}

static int
PNLCD_IOC_Multi_Segment(unsigned short on, int num_segs, int *segment_array)
{
    int     sample_count = 0;
    int     x = 0;
    int     err = 0;
    PNLCD_MISC_SEGMENTS_REC theRec;

    NDEBUG("@@@@ PNLCD_IOC_Multi_Segment(on=%02x, numSegs=%02x, ptr=0x%08x, \n",on,num_segs,(int)segment_array);
    NDEBUG("        array=[%x",segment_array[0]);
    for (x=1;x<num_segs;x++)  {
        NDEBUG(", %x",segment_array[x]);
    }
    NDEBUG("])\n\n");

    sample_count = num_segs;

    // Range-check values to assure segment < 200
    if (num_segs > (MAX_COMMAND_BYTES-3))  {
        NDEBUG("The mon/moff command can only take %d segment addresses.\n",(MAX_COMMAND_BYTES-3));
        err = num_segs;
        goto exit;
    }

    // Range check parameters
    for (x=0;x<num_segs;x++) {
        if (segment_array[x] > MAX_PN_LCD_SEGMENT) {
            err = segment_array[x];
            goto exit;
        }
    }

    // Prepare calling structure
    theRec.enable = on;
    theRec.num_segs = num_segs;
    theRec.seg_array = segment_array;
    err = my_sys_ioctl(g_ioc_fd, IOC_PNLCD_MISC_IOCTL, &theRec);

exit:
    NDEBUG("@@@@ PNLCD_IOC_Multi_Segment() returning %d\n",err);
    return(err);  
}

static int
PNLCD_IOC_Multi_Segment_Pairs(unsigned short on, int num_pairs, int *paired_segment_array, int vertical)
{
    int                     sample_count = 0;
    int                     x = 0;
    int                     err = 0;
    int                     ioctl_id = 0;
    PNLCD_SEGMENT_PAIRS_REC theRec;

    NDEBUG("@@@@ PNLCD_IOC_Multi_Segment_Pairs(on=%02x, numPairs=%02x, array=%08x, vertical=%s)\n",on,num_pairs,(int)paired_segment_array,vertical?"YES":"NO");

    sample_count = (num_pairs*2);     // Count is given in pairs

    // Range-check values to assure segment < 200
    if (sample_count > (MAX_COMMAND_BYTES-3))  {
        NDEBUG("The multi segment pairs command can only take %d pairs of segment addresses.\n",(MAX_COMMAND_BYTES-3));
        err = sample_count;
        goto exit;
    }
    for (x=0;x<sample_count;x++) {
        if (paired_segment_array[x] > MAX_PN_LCD_SEGMENT) {
            err = paired_segment_array[x];
            goto exit;
        }
    }

    // Prepare calling structure
    theRec.enable = on;
    theRec.num_pairs = num_pairs;
    theRec.seg_pair_array = paired_segment_array;
    ioctl_id = (vertical ? IOC_PNLCD_VERT_PAIRS_IOCTL : IOC_PNLCD_PAIRS_IOCTL);
    err = my_sys_ioctl(g_ioc_fd, ioctl_id, &theRec);

exit:
    NDEBUG("@@@@ PNLCD_IOC_Multi_Segment_Pairs() returning %d\n",err);
    return(err);  
}

////////////////////////////////////////////////////////
//
// /proc Interface Routines
//
////////////////////////////////////////////////////////

/******************************************************************************/

static void print_proc_ioc_lcd_usage(const char *command_str)
{
    printk( "Invalid format for /proc/pnlcd (bad command name '%s').\n\n"
        "Usage: \n"
        "   on:segNum\n"
        "   off:segNum\n"
        "   son:startSegNum:endSegNum\n"
        "   soff:startSegNum:endSegNum\n"
        "   vson:startSegNum:endSegNum\n"
        "   vsoff:startSegNum:endSegNum\n"
        "   mon:segCount:seg1:seg2:...:segN-1:segN\n"
        "   moff:segCount:seg1:seg2:...:segN-1:segN\n"
        "   mpon:segPairCount:seg1:...:segN\n"
        "   mpoff:segPairCount:seg1:...:segN\n"
        "   vmpon:segPairCount:seg1:...:segN\n"
        "   vmpoff:segPairCount:seg1:...:segN\n"
        "\n"   
        "Example Usages: \n"
        "  Turn on single segment 0x20:\n"
        "        echo 'on:0x20' > /proc/pnlcd/io_txt\n"
        "  Turn off single segment 0x20:\n"
        "        echo 'off:0x20' > /proc/pnlcd/io_txt\n"
        "  Turn on sequential segments 0x20->0x40:\n"
        "        echo 'son:0x20:0x40' > /proc/pnlcd/io_txt\n"
        "  Turn off sequential segments 0x20->0x40:\n"
        "        echo 'soff:0x20:0x40' > /proc/pnlcd/io_txt\n"
        "  Turn on vertically sequential segments 0x20->0x40:\n"
        "        echo 'vson:0x20:0x40' > /proc/pnlcd/io_txt\n"
        "  Turn off vertically sequential segments 0x20->0x40:\n"
        "        echo 'vsoff:0x20:0x40' > /proc/pnlcd/io_txt\n"
        "  Turn on multiple non-sequential segments 0x20, 0x32, and 0x40:\n"
        "        echo 'mon:3:0x20:0x32:0x40' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 12 segment numbers.\n"
        "  Turn off multiple non-sequential segments 0x20, 0x32, and 0x40:\n"
        "        echo 'moff:3:0x20:0x32:0x40' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 12 segment numbers.\n"
        "  Turn on multiple non-sequential segment pairs 0x20 to 0x32, 0x40 to 0x42:\n"
        "        echo 'mon:2:0x20:0x32:0x40:0x42' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 6 pairs (12 segment numbers).\n"
        "  Turn off multiple non-sequential segment pairs 0x20, 0x32, 0x40 to 0x42:\n"
        "        echo 'moff:2:0x20:0x32:0x40:0x42' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 6 pairs (12 segment numbers).\n"
        "  Turn on multiple non-sequential vertical segment pairs 0x20 to 0x32, 0x40 to 0x42:\n"
        "        echo 'vmon:2:0x20:0x32:0x40:0x42' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 6 pairs (12 segment numbers).\n"
        "  Turn off multiple non-sequential vertical segment pairs 0x20, 0x32, 0x40 to 0x42:\n"
        "        echo 'vmoff:2:0x20:0x32:0x40:0x42' > /proc/pnlcd/io_txt\n"
        "        This proc interface takes a maximum of 6 pairs (12 segment numbers).\n\n",command_str);
}

/******************************************************************************/

static int 
process_lcd_proc_write(const char *cmd, char *cmd_buff, int *cmd_byte_count, int *result_count)
{
    int     buff[14] = { 0,0,0,0,0,0,0, 0,0,0,0,0,0,0 };
    char    command_str[256];
    char    *cmd_data = NULL;
    int     param_count = 0;
    int     bad_value = 0;
    int     sample_count = 0;
    int     x = 0;

    if (cmd == NULL)
        goto bad_usage_error;

    // If we're powered off, don't do anything
    if (0 == pnlcd_awake()) {
        goto exit;
    }

#ifdef DEBUG_IOC_COMMAND_SEND
    printk("Parsing ioc PN-LCD command = %s\n",cmd);
#endif  // DEBUG_IOC_COMMAND_SEND
    memclr(command_str,sizeof(command_str));
    strcpy(command_str,cmd);

    // Point to first :
    cmd_data = strchr(command_str,':');
    if (cmd_data == NULL)
        goto bad_usage_error;
    *cmd_data++ = '\0';

    // cmd_data now points to the parameters for the command
    // cmd now points to a NULL-terminated string representing command

    param_count = sscanf(cmd_data,"%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n", &buff[0], 
        &buff[1], &buff[2], &buff[3], &buff[4], &buff[5], &buff[6],
        &buff[7], &buff[8], &buff[9], &buff[10], &buff[11], &buff[12]); 

    // Make sure none of the parameters is too big for a char holding cell
    // Load them into a character array while we're at it
    for (x=0; x<param_count; x++) {
        if (buff[x] > 0x00FF) {
            bad_value = buff[x];
            goto bad_usage_error;
        }
    }

    // IOC responds with a single byte for all PN-LCD commands
    *result_count = 1;

    // Now look at command and generate appropriate cmd to send to IOC
    if (!strcmp(command_str,"on")) {
        if ((bad_value = PNLCD_IOC_Multi_Segment(SEGMENT_ON, 1, &buff[0])) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"off")) {
        if ((bad_value = PNLCD_IOC_Multi_Segment(SEGMENT_OFF, 1, &buff[0])) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"son")) {
        if ((bad_value = PNLCD_IOC_Sequential_Segment(SEGMENT_ON, buff[0], buff[1],0)) != 0)
            goto bad_value_error;
    }
    else if (!strcmp(command_str,"soff")) {
        if ((bad_value = PNLCD_IOC_Sequential_Segment(SEGMENT_OFF, buff[0], buff[1],0)) != 0)
            goto bad_value_error;
    }
    else if (!strcmp(command_str,"vson")) {
        if ((bad_value = PNLCD_IOC_Sequential_Segment(SEGMENT_ON, buff[0], buff[1],1)) != 0)
            goto bad_value_error;
    }
    else if (!strcmp(command_str,"vsoff")) {
        if ((bad_value = PNLCD_IOC_Sequential_Segment(SEGMENT_OFF, buff[0], buff[1],1)) != 0)
            goto bad_value_error;
    }
    else if (!strcmp(command_str,"mpon")) {
        sample_count = (buff[0]*2);     // Count is given in pairs
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x pairs of segments for mpon but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }

        if ((bad_value = PNLCD_IOC_Multi_Segment_Pairs(SEGMENT_ON, buff[0], &buff[1],0)) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"mpoff")) {
        sample_count = (buff[0]*2);     // Count is given in pairs
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x pairs of segments for mpoff but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }

        if ((bad_value = PNLCD_IOC_Multi_Segment_Pairs(SEGMENT_OFF, buff[0], &buff[1],0)) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"vmpon")) {
        sample_count = (buff[0]*2);     // Count is given in pairs
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x pairs of segments for vmpon but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }

        if ((bad_value = PNLCD_IOC_Multi_Segment_Pairs(SEGMENT_ON, buff[0], &buff[1],1)) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"vmpoff")) {
        sample_count = (buff[0]*2);     // Count is given in pairs
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x pairs of segments for vmpoff but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }

        if ((bad_value = PNLCD_IOC_Multi_Segment_Pairs(SEGMENT_OFF, buff[0], &buff[1],1)) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"mon")) {
        sample_count = buff[0];
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x segments for mon but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }

        if ((bad_value = PNLCD_IOC_Multi_Segment(SEGMENT_ON, buff[0], &buff[1])) != 0)
            goto bad_usage_error;
    }
    else if (!strcmp(command_str,"moff")) {
        sample_count = buff[0];

        // Make sure caller passed in all arguments
        if ((param_count-1) != sample_count) {
            printk("You specified 0x%x segments for moff but passed 0x%x segments.\n",
                sample_count,(param_count-1));
            goto bad_usage_error;
        }
        if ((bad_value = PNLCD_IOC_Multi_Segment(SEGMENT_OFF, buff[0], &buff[1])) != 0)
            goto bad_usage_error;
    }
    else 
        goto bad_usage_error;
    
exit:
    return(0);

bad_value_error:
    printk( "Segment value 0x%x is out of range.\n"
            "Segment values must be in hex and between 0 and 0xc7, inclusive.\n",bad_value);

bad_usage_error:
    print_proc_ioc_lcd_usage(cmd);
    return(1);
}

static int proc_lcd_write_prolog(const char *cmd_buffer, unsigned long cmd_count,
                                 char *local_buffer, unsigned long local_count,
                                 int *opened)
{
    int result = *opened = 0;
    
    // Don't do anything unless the PNLCD is awake.
    //
    if ( pnlcd_awake() )
    {
        // Make sure the command buffer won't overrun the local buffer.
        //
        if ( cmd_count <= local_count )
        {
            // Copy the command buffer into the local buffer after clearing
            // the local buffer.
            //
            memclr(local_buffer, local_count);
            
            if ( 0 == copy_from_user(local_buffer, cmd_buffer, cmd_count) )
            {
                // Don't re-open the PNLCD driver if it's already open.
                //
                if ( 0 == Is_PNLCD_Open() )
                {
                    // Remember if opened the PNLCD driver.
                    //
                    if ( 0 == PNLCD_OPEN() )
                        *opened = 1;
                    else
                        result = -EFAULT;
                }
            }
            else
                result = -EFAULT;
        }
        else
            result = -EINVAL;
    }
    else
        result = cmd_count;
    
    // Done.
    //
    return ( result );
}

static int proc_lcd_write_epilog(int cmd_count, int result, int opened)
{
    // If we opened the PNLCD driver, then close it.
    //
    if ( opened )
        PNLCD_CLOSE();
    
    // If the command has completed successfully, note that we consumed
    // its entire buffer.
    //
    if ( 0 == result )
        result = cmd_count;

    return ( result );
}

static int proc_write_ioc_lcd(struct file *file,
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
	char	cmd[256];
    int     rc = 0;
    char    cmd_bytes[5];
    int     result_count = 0;
    int     cmd_byte_count = 0;
    char    *new_line = NULL;
    int     opened = 0;

    if (0 == (rc = proc_lcd_write_prolog(buffer, count, cmd, 256, &opened))) {
        // Strip the new line
        new_line = strchr(cmd,'\n');
        if (new_line)
            *new_line = '\0';
    
        rc = process_lcd_proc_write(cmd, cmd_bytes, &cmd_byte_count, &result_count);
    }

    return proc_lcd_write_epilog(count, rc, opened);
}

#define SEGMENT(s) ((s == SEGMENT_ON) ? '*' : ' ')

static int proc_read_ioc_lcd(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
    char *segments = get_pnlcd_segments();
    int  i, len;

    for (i = 0, len = 0; i < PN_LCD_SEGMENT_COUNT; i += 2)
        len += sprintf(&page[len], "0x%02X:0x%02X:%c %c\n",
                    (i+1), (i+0), SEGMENT(segments[i+1]),
                    SEGMENT(segments[i+0]));

    *eof = 1;
    
    return ( len );
}

static int proc_write_binary(
	struct file *file,
	const char *buf,
	unsigned long count,
	void *data)
{
	char segments[PN_LCD_SEGMENT_COUNT], lbuf[(PN_LCD_SEGMENT_COUNT/8)], segment;
	int  i, j, opened, result;
	
    if ( 0 == (result = proc_lcd_write_prolog(buf, count, lbuf, (PN_LCD_SEGMENT_COUNT/8), &opened)) )
    {
        for (i = 0; i < (PN_LCD_SEGMENT_COUNT/8); i++)
        {
            for (j = 0; j < 8; j++)
            {
                segment = (lbuf[i] >> j) & SEGMENT_ON;
                segments[(8 * i) + j] = segment;
            }
        }
    
        set_pnlcd_segments(segments);
    }

    return ( proc_lcd_write_epilog(count, result, opened) );
}

static int proc_read_binary(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
    char segment, *segments = get_pnlcd_segments();
    int  i, j, len;
    
    for (i = 0, len = 0; i < (PN_LCD_SEGMENT_COUNT/8); i++)
    {
        for (j = 0, segment = 0; j < 8; j++)
            segment |= (segments[(8 * i) + j] & SEGMENT_ON) << j;
        
        len += sprintf(&page[i], "%c", segment);
    }

	*eof = 1;

	return ( len );
}

static int proc_write_pnlcd_progress_bar(
	struct file *file,
	const char *buf,
	unsigned long count,
	void *data)
{
    int  n, opened, result;
	char lbuf[16];

    if ( 0 == (result = proc_lcd_write_prolog(buf, count, lbuf, 16, &opened)) )
    {
        if ( sscanf(lbuf, "%d", &n) )
            update_pnlcd_progress_bar(n);
    }
    
	return ( proc_lcd_write_epilog(count, result, opened) );
}

static int proc_read_pnlcd_progress_bar(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	int len = sprintf(page, "%d\n", g_progress_bar);
	
	*eof = 1;

	return ( len );
}

static int proc_write_pnlcd_animate(
	struct file *file,
	const char *buf,
	unsigned long count,
	void *data)
{
    int opened, result;
    char lbuf[16];
    
    if ( 0 == (result = proc_lcd_write_prolog(buf, count, lbuf, 16, &opened)) )
    {
        int cmd, arg = 0;

        switch ( sscanf(lbuf, "%d %d", &cmd, &arg) )
        {
            case 1:
            case 2:
                animate_pnlcd((pnlcd_animation_cmds)cmd, arg);
            break;
        }
    }

    return ( proc_lcd_write_epilog(count, result, opened) );
}

static int proc_read_pnlcd_animate(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
    int len = 0;

    len += sprintf(&page[len], "Animation state:  %d (%s, %s)\n", (int)g_animation_state, pnlcd_animation_state_names[(int)g_animation_state], pnlcd_animation_op_names[g_animation_op]);
    len += sprintf(&page[len], "          type:   %d (%s)\n",     (int)g_animation_type,  pnlcd_animation_sp_animation_names[(int)g_animation_type]);
    len += sprintf(&page[len], "          rate:   %d Hz\n",       (int)g_frame_rate);

    *eof = 1;

    return ( len );
}

static int reboot_pnlcd(struct notifier_block *self, unsigned long u, void *v)
{
    // Don't do anything unless the PNLCD is awake.
    //
    if ( pnlcd_awake() )
    {
        int open = 0, opened = 0;
        
        // Open the PNLCD driver only if it isn't already.
        //
        if ( 0 == Is_PNLCD_Open() )
        {
            if ( 0 == PNLCD_OPEN() )
                open = opened = 1;
        }
        else
            open = 1;
        
        // Clear the PNLCD and stop any animation.
        //
        if ( open )
        {
            animate_pnlcd(stop_animation, 0);
            clear_pnlcd();
        }
        
        // Close the PNLCD driver if we opened it.
        //
        if ( opened )
            PNLCD_CLOSE();
    }

    // Done.
    //
    return NOTIFY_DONE;
}

static struct notifier_block reboot_pnlcd_nb = {
	.notifier_call = reboot_pnlcd,
	.priority = FIONA_REBOOT_PNLCD,
};

/******************************************************************************/

static struct proc_dir_entry *proc_lcd_parent       = NULL;
static struct proc_dir_entry *proc_lcd_io_txt       = NULL;
static struct proc_dir_entry *proc_lcd_io_bin       = NULL;
static struct proc_dir_entry *proc_lcd_progress_bar = NULL;
static struct proc_dir_entry *proc_lcd_animate      = NULL;

static struct proc_dir_entry *create_lcd_proc_entry(const char *name, mode_t mode, struct proc_dir_entry *parent,
    read_proc_t *read_proc, write_proc_t *write_proc)
{
    struct proc_dir_entry *lcd_proc_entry = create_proc_entry(name, mode, parent);
    
    if ( lcd_proc_entry )
    {
        lcd_proc_entry->owner      = THIS_MODULE;
        lcd_proc_entry->data       = NULL;
        lcd_proc_entry->read_proc  = read_proc;
        lcd_proc_entry->write_proc = write_proc;
    }
    
    return ( lcd_proc_entry );
}

#define remove_lcd_proc_entry(name, entry, parent)  \
    do                                              \
    if ( entry )                                    \
    {                                               \
        remove_proc_entry(name, parent);            \
        entry = NULL;                               \
    }                                               \
    while ( 0 )

static void lcd_cleanup_procfs(void)
{
    if ( proc_lcd_parent )
    {
        remove_lcd_proc_entry(PNLCD_PROC_IO_TXT,       proc_lcd_io_txt,       proc_lcd_parent);
        remove_lcd_proc_entry(PNLCD_PROC_IO_BIN,       proc_lcd_io_bin,       proc_lcd_parent);
        remove_lcd_proc_entry(PNLCD_PROC_PROGRESS_BAR, proc_lcd_progress_bar, proc_lcd_parent);
        remove_lcd_proc_entry(PNLCD_PROC_ANIMATE,      proc_lcd_animate,      proc_lcd_parent);
        remove_lcd_proc_entry(PNLCD_PROC_PARENT,       proc_lcd_parent,       NULL);

        unregister_reboot_notifier(&reboot_pnlcd_nb);
    }
}

static int lcd_proc_init(void)
{
    int result = -ENOMEM;
    
    // Parent:  /proc/pnlcd.
    //
    proc_lcd_parent = create_proc_entry(PNLCD_PROC_PARENT, (S_IFDIR | S_IRUGO | S_IXUGO), NULL);
    
    if ( proc_lcd_parent )
    {
       int null_check = -1;

        // Child:  /proc/pnlcd/io_txt,
        //
        proc_lcd_io_txt = create_lcd_proc_entry(PNLCD_PROC_IO_TXT, PNLCD_PROC_MODE, proc_lcd_parent,
                            proc_read_ioc_lcd, proc_write_ioc_lcd);
                            
        null_check &= (int)proc_lcd_io_txt;
    
        //         /proc/pncld/io_bin,
        //
        proc_lcd_io_bin = create_lcd_proc_entry(PNLCD_PROC_IO_BIN, PNLCD_PROC_MODE, proc_lcd_parent,
                            proc_read_binary, proc_write_binary);
                            
        null_check &= (int)proc_lcd_io_bin;
        
        //          /proc/pnlcd/progress_bar,
        //
        proc_lcd_progress_bar = create_lcd_proc_entry(PNLCD_PROC_PROGRESS_BAR,  PNLCD_PROC_MODE, proc_lcd_parent,
                            proc_read_pnlcd_progress_bar, proc_write_pnlcd_progress_bar);
        
        null_check &= (int)proc_lcd_progress_bar;

        //          /proc/pnlcd/animate.
        //
        proc_lcd_animate = create_lcd_proc_entry(PNLCD_PROC_ANIMATE,  PNLCD_PROC_MODE, proc_lcd_parent,
                            proc_read_pnlcd_animate, proc_write_pnlcd_animate);
        
        null_check &= (int)proc_lcd_animate;
        
        // Reboot notifier.
        //
        register_reboot_notifier(&reboot_pnlcd_nb);

        // Success? 
        //
        if ( 0 == null_check )
            lcd_cleanup_procfs();
        else
            result = 0;
    }
    
    // Done.
    //
    return ( result );
}

//////////////////////////////
// Core Driver Routines
//////////////////////////////

enum pnlcd_blocking_state
{
    pnlcd_in_blocked_state = 0, // Blocks all PNLCD IOCTL traffic.
    pnlcd_in_delayed_state,     // Delays all PNLCD IOCTL traffic except animation.
    
    pnlcd_in_run_state          // Passes all PNLCD IOCTL traffic through.
};
typedef enum pnlcd_blocking_state pnlcd_blocking_state;

// Start the PNLCD off in the running (i.e., non-delayed, non-blocked) state.
//
static pnlcd_blocking_state g_blocking_state = pnlcd_in_run_state;

// Let the IOC know whether to delay PNLCD commands or not.
//
int
pnlcd_delayed(void)
{
    return ( pnlcd_in_delayed_state == g_blocking_state );
}

// Let the PNLCD know whether to block IOCTL traffic or not.
//
static int
pnlcd_blocked(void)
{
    return ( pnlcd_in_blocked_state == g_blocking_state );
}

static void
pnlcd_blocking_dispatch(pnlcd_blocking_cmds which_blocking_cmd)
{
    switch ( which_blocking_cmd )
    {
        // Block or unblock PNLCD IOCTL traffic.
        //
        case start_block:
            if ( pnlcd_in_run_state == g_blocking_state )
                 g_blocking_state = pnlcd_in_blocked_state;
        break;
        
        case stop_block:
            if ( pnlcd_in_blocked_state == g_blocking_state )
                g_blocking_state = pnlcd_in_run_state;
        break;
        
        // Delay or undelay PNLCD IOCTL traffic except animation.
        //
        case start_delay:
            if ( pnlcd_in_run_state == g_blocking_state )
                g_blocking_state = pnlcd_in_delayed_state;
        break;
        
        case stop_delay:
            if ( pnlcd_in_delayed_state == g_blocking_state )
                g_blocking_state = pnlcd_in_run_state;
        break;
        
        // Restore:  Block all PNLCD IOCTL traffic until we're
        //           done restoring things from a delay.
        //
        case stop_delay_restore:
            if ( pnlcd_in_delayed_state == g_blocking_state )
            {
                g_blocking_state = pnlcd_in_blocked_state;
                
                if ( animation_stop_delayed() )
                    animate_pnlcd(stop_animation, 0);
                
                if ( !HIDE_PNLCD() )
                    restore_pnlcd_segments();
                
                g_blocking_state = pnlcd_in_run_state;
            }
        break;
    }
}

static int
Do_Open_IOC(void)
{
    // If IOC driver present, return no error
    if (g_ioc_driver_present)
        return(0);
    else
        return(1);
}

static int
Do_Close_IOC(void)
{
    // Close the IOC driver
#if 0
    close(g_ioc_fd);
    g_ioc_fd = 0;
#endif
    return(0);
}

static int
PNLCD_Open(struct inode* inNode, struct file* inFile)
{
    // Inc user count
    if (!g_user_count++)  {

        // Open the IOC driver
        if (Do_Open_IOC())
            return(-EREMOTEIO);  // Error - couldn't open

        // If we're powered off, don't do anything
        if (g_pnlcd_power_mode != FPOW_MODE_ON)
            goto exit;

        // Enable the PN-LCD
        if (Do_Enable_PNLCD(ENABLE)) {
            Do_Close_IOC();         // Close IOC driver
            return(-EREMOTEIO);     // Error - couldn't open
        }
    }

exit:
    // Return success
    return 0;
}

static int
PNLCD_Close(struct inode* inNode, struct file* inFile)
{
    if (--g_user_count == 0)  {
        Do_Close_IOC();
    }

    // Return success
    return 0;
}

static int
PNLCD_MemcpyW(unsigned long flag, void *destination, void *source, int length)
{
    int result = -EINVAL;
 
    if ( destination && source && length )
    {
        result = 0;
        
        // Do a memcpy() from kernel space; otherwise, copy_from_user() space.
        //
        if ( PNLCD_IOCTL_FLAG == flag )
            memcpy(destination, source, length);
        else
            result = copy_from_user(destination, source, length);
    }
        
    return ( result );
}

static int
PNLCD_MemcpyR(unsigned long flag, void *destination, void *source, int length)
{
    int result = -EINVAL;
    
    if ( destination && source && length )
    {
        result = 0;
        
        // Do a memcpy() to kernel space; otherwise, copy_to_user() space.
        //
        if ( PNLCD_IOCTL_FLAG == flag )
            memcpy(destination, source, length);
        else
            result = copy_to_user(destination, source, length);
    }
        
    return ( result );
}

static void
mask_out_animation_segments(char *segments)
{
    int i;
    
    // Mask out any of the animation segments that might have been lit up
    // on another thread.
    //
    for ( i = 0; i < num_sp_animation_segments; i++ )
        segments[sp_animation_segments[i]] = SEGMENT_OFF;
}

static void
mask_out_animation(char *segments)
{
    if ( FRAMEWORK_RUNNING() )
    {
        // If the Framework is potentially animating the PNLCD, mask out all of
        // the segments.  Otherwise, just mask out the PNLCD driver's animation
        // segments. 
        //
        if ( 1 == g_fake_pnlcd_vert_count )
            memset(segments, SEGMENT_OFF, PN_LCD_SEGMENT_COUNT);
        else
            mask_out_animation_segments(segments);
    }
}

static void
fake_pnlcd(int vert, PNLCD_SEQ_SEGMENTS_REC *theRec, char *segments)
{
    // The Framework only uses PNLCD_SEQ_IOCTL & PNLCD_VERT_SEQ_IOCTL calls.  So, we fake
    // the PNLCD on the eInk display on all non-clearing PNLCD_SEQ_IOCTL calls.  But we
    // only fake the PNLCD on non-clearing PNLCD_VERT_SEQ_IOCTL calls when a pair of
    // left-then-right calls comes through.
    //
    if ( FRAMEWORK_RUNNING() && theRec->enable )
    {
        int fake = 0;

        // Are the segments vertical?
        //
        if ( vert )
        {
            int left_column = theRec->start_seg & 1;
            
            // Left column and first in pair?
            //
            if ( left_column )
            {
                if ( 0 == g_fake_pnlcd_vert_count )
                    g_fake_pnlcd_vert_count++;
            }
            else
            {
                // Right column and second in pair?
                //
                if ( 1 == g_fake_pnlcd_vert_count )
                    g_fake_pnlcd_vert_count++;
                else
                    g_fake_pnlcd_vert_count = 0;
            }


            // Got a left-then-right pair?
            //
            if ( 2 == g_fake_pnlcd_vert_count )
            {
                g_fake_pnlcd_vert_count = 0;
                fake = 1;
            }
        }
        else
        {
            g_fake_pnlcd_vert_count = 0;
            fake = 1;
        }
        
        if ( fake && DEC_PNLCD_EVENT_COUNT() )
        {   
            mask_out_animation_segments(segments);
            eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, segments);
        }
    }
}

int
pnlcd_hidden(void)
{
    return ( FRAMEWORK_RUNNING() && HIDE_PNLCD() );
}

static int
PNLCD_Ioctl_Real(struct inode *inNode, struct file *inFile, 
            unsigned int inCommand, unsigned long inArg)
{
    int local_segment_array[PN_LCD_SEGMENT_COUNT];
    unsigned long flag = (unsigned long)inNode;
    int rc = 0;

#define MEMCPYW(d,s,l) PNLCD_MemcpyW(flag, d, s, l)
#define MEMCPYR(d,s,l) PNLCD_MemcpyR(flag, d, s, l)
    
    NDEBUG("PNLCD_Ioctl(): inCommand = %x\n", inCommand);

    if (_IOC_TYPE(inCommand) != IOCTL_PNLCD_MAGIC_NUMBER) return -EINVAL;
    if (_IOC_NR(inCommand) > PNLCD_IOCTLLast) return -EINVAL;

    // Powered off...
    if (0 == pnlcd_awake()) {
        // Let the framework tell us that it's running...
        if ((PNLCD_PROGRESS_BAR_IOCTL == inCommand) && CHECK_FRAMEWORK_RUNNING()) {
            update_pnlcd_progress_bar((int)inArg); // Honors pnlcd_awake().
        }

        rc = -FPOW_COMPONENT_NOT_POWERED_ERR;
        goto exit;
    }

    switch(inCommand)  {
        case PNLCD_VERT_SEQ_IOCTL:
        case PNLCD_SEQ_IOCTL:
            {
            int vert = (inCommand == PNLCD_SEQ_IOCTL) ? 0 : 1;
            
            PNLCD_SEQ_SEGMENTS_REC  theRec;
            NDEBUG("PNLCD%sSEQ_IOCTL case seen. \n",vert?"_VERT_":"_");

            // Get user data
            if (MEMCPYW(&theRec,(void *)inArg, sizeof(theRec)))
               return(-EFAULT);

            rc = PNLCD_IOC_Sequential_Segment(theRec.enable, theRec.start_seg, theRec.end_seg, vert);
            if ((0 == rc) && FAKE_PNLCD_IN_USE()) {
                memcpy(local_segment_array, get_pnlcd_segments(), PN_LCD_SEGMENT_COUNT);
                fake_pnlcd(vert, &theRec, (char *)local_segment_array);
            }
            }
            break;

        case PNLCD_MISC_IOCTL:
            {
            PNLCD_MISC_SEGMENTS_REC  theRec;
            NDEBUG("PNLCD_MISC_IOCTL case seen. \n");

            // Get user data
            if (MEMCPYW(&theRec,(void *)inArg, sizeof(theRec))) {
                NDEBUG("PNLCD_MISC_IOCTL failed to get user data\n");
                return(-EFAULT);
            }

            // Get user data segment array
            if (MEMCPYW(local_segment_array,(void *)theRec.seg_array, 
                MIN(sizeof(local_segment_array),(theRec.num_segs*sizeof(*theRec.seg_array))))) {
                NDEBUG("PNLCD_MISC_IOCTL failed to get user data segment_array for %d bytes (numsegs=%d)\n", MIN(sizeof(local_segment_array),(theRec.num_segs*sizeof(*theRec.seg_array))),theRec.num_segs);
                    return(-EFAULT);
            }

            NDEBUG("PNLCD_MISC_IOCTL calling PNLCD_IOC_Multi_Segment\n");
            rc = PNLCD_IOC_Multi_Segment(theRec.enable, theRec.num_segs, local_segment_array);
            }
            break;

        case PNLCD_VERT_PAIRS_IOCTL:
        case PNLCD_PAIRS_IOCTL:
            {
            PNLCD_SEGMENT_PAIRS_REC  theRec;
            NDEBUG("PNLCD%sPAIRS_IOCTL case seen. \n",(inCommand==PNLCD_PAIRS_IOCTL)?"_":"_VERT_");

            // Get user data
            if (MEMCPYW(&theRec,(void *)inArg, sizeof(theRec)))
                return(-EFAULT);

            // Get user data segment array
            if (MEMCPYW(local_segment_array,(void *)theRec.seg_pair_array, 
                MIN(sizeof(local_segment_array),(theRec.num_pairs*2*sizeof(*theRec.seg_pair_array)))))
                    return(-EFAULT);

            rc = PNLCD_IOC_Multi_Segment_Pairs(theRec.enable, theRec.num_pairs, local_segment_array, (inCommand==PNLCD_PAIRS_IOCTL)?0:1);
            }
            break;

        case PNLCD_PROGRESS_BAR_IOCTL:
            update_pnlcd_progress_bar((int)inArg);
            break;
        
        case PNLCD_CLEAR_ALL_IOCTL:
            clear_pnlcd();
            break;
        
        case PNLCD_ANIMATE_IOCTL:
            {
            pnlcd_animation_t pnlcd_animate;
            
            if (MEMCPYW(&pnlcd_animate, (void *)inArg, sizeof(pnlcd_animation_t)))
                return(-EFAULT);

            animate_pnlcd(pnlcd_animate.cmd, pnlcd_animate.arg);
            }
            break;
            
        case PNLCD_GET_SEG_STATE:
             rc = MEMCPYR((char *)inArg, get_pnlcd_segments(), PN_LCD_SEGMENT_COUNT);
             if ((0 == rc) && (PNLCD_IOCTL_FLAG == flag) && FAKE_PNLCD_IN_USE()) {
                mask_out_animation((char *)inArg);
             }
             break;
        
        case PNLCD_SET_SEG_STATE:
            if (inArg) {
                if (MEMCPYW(local_segment_array, (char *)inArg, PN_LCD_SEGMENT_COUNT))
                    return(-EFAULT);
                set_pnlcd_segments((char *)local_segment_array);
            }
            restore_pnlcd_segments();
            break;

        default:
            NDEBUG("PNLCD_Ioctl: Default IOCTL case seen. \n");
            break;
    }

exit:
    return(rc);
}

static int
PNLCD_Ioctl(struct inode *inNode, struct file *inFile, 
            unsigned int inCommand, unsigned long inArg)
{
    int result = 0;

    // If driver not inited yet, ignore this call
    if (driver_not_inited())  {
        printk("PNLCD driver not inited yet, exiting PNLCD_Ioctl()\n");
        return(result);
    }

    // Process any blocking command to maintain state.
    //
    if ( PNLCD_BLOCK_IOCTL == inCommand )
        pnlcd_blocking_dispatch((pnlcd_blocking_cmds)inArg);
    else
    {
        // If we're not blocked, go ahead and process the command
        // as usual (delays are simply cached at the IOC level).
        //
        if ( !pnlcd_blocked() )
            result = PNLCD_Ioctl_Real(inNode, inFile, inCommand, inArg);
    }

    return ( result );
}

//////////////////////////////
// Power Management Support
//////////////////////////////
#ifdef LOCAL_CONFIG_PM
/**
 *  pnlcd_suspend - Disable / power down the pnlcd
 *  @state: suspend mode (PM_SUSPEND_xxx)
 */
int PNLCD_Suspend(u32 state)
{
    printk("PNLCD_Suspend()\n");
    Do_Enable_PNLCD(DISABLE);
    return(0);
}

/**
 *  pnlcd_resume - Enable / power up the pnlcd
 *  @state: suspend mode (PM_SUSPEND_xxx)
 */
int PNLCD_Resume(u32 state)
{
    printk("PNLCD_Resume()\n");
    Do_Enable_PNLCD(ENABLE);
    return(0);
}
#else
#define PNLCD_Suspend   NULL
#define PNLCD_Resume    NULL
#endif

//////////////////////////////
// Exported File Operators
//  Init and Destroy Routines
//////////////////////////////

struct file_operations pnlcd_fops = {
        owner:          THIS_MODULE,
        read:           NULL,
        write:          NULL,
        ioctl:          PNLCD_Ioctl,
#ifdef LOCAL_CONFIG_PM
        suspend:        PNLCD_Suspend,
        resume:         PNLCD_Resume,
#endif
        open:           PNLCD_Open,
        release:        PNLCD_Close,
};

EXPORT_SYMBOL(pnlcd_fops);

static struct miscdevice pnlcd_device = {
    minor:  PNLCD_MINOR,
    name:   PNLCD_DRIVER_NAME,
    fops:   &pnlcd_fops,
};


static int
__init PNLCD_Driver_Init(void)
{
    int retVal = 0;

    // Register /proc/pnlcd
    lcd_proc_init();

    // Open IOC driver
    if (Do_Open_IOC()) {
        printk(KERN_ERR "Failed to initialize Fiona PN-LCD driver - no IOC driver present!\n");
        retVal = -EIO;
        goto exit;
    }

    // Register the pnlcd driver
    retVal = misc_register(&pnlcd_device);
    if (retVal < 0)  {
        misc_deregister(&pnlcd_device);
        lcd_cleanup_procfs();
        return retVal;
    }
        
#ifdef CONFIG_FIONA_PM_PNLCD
    // Register IOC PNLCD driver with Fiona Power Manager,
    // we're ignoring result here as we'll still allow driver to
    // work even if we can't do efficient power management...
    if (pnlcd_register_with_fpow(NULL, &g_pnlcd_fpow_component_ptr))
        printk("PNLCD: Failed to register with fpow !!\n");
    else {
        g_pnlcd_driver_inited = 1;
        if (fpow_get_pnlcd_disable_pending()) {
            // Clear and Disable Video
            powdebug("PNLCD: Driver calling FPOW_MODE_OFF\n");
            pnlcd_fpow_setmode(NULL, 0, FPOW_MODE_OFF);
            fpow_clear_pnlcd_disable_pending();
        }
        else if (fpow_get_pnlcd_enable_pending()) {
            // Enable Video
            powdebug("PNLCD: Driver calling FPOW_MODE_ON\n");
            pnlcd_fpow_setmode(NULL, 0, FPOW_MODE_ON);
            fpow_clear_pnlcd_enable_pending();
        }
    }
#else
    g_pnlcd_driver_inited = 1;
#endif

    start_animation_thread();
    PNLCD_INIT();

    printk("PNLCD: Fiona PNLCD Driver %s\n",PNLCD_DRIVER_VERSION);
exit:
    return(retVal);
}

static void
__exit PNLCD_Driver_Destroy(void)
{
#ifdef CONFIG_FIONA_PM_KEYBOARD
    // Unregister IOC driver with Fiona Power Manager
    pnlcd_unregister_with_fpow(g_pnlcd_fpow_component_ptr);
    g_pnlcd_fpow_component_ptr  = NULL;
#endif

    lcd_cleanup_procfs();
    misc_deregister(&pnlcd_device);
    
    stop_animation_thread();
    PNLCD_DONE();
}

module_init(PNLCD_Driver_Init);
module_exit(PNLCD_Driver_Destroy);

MODULE_AUTHOR("Nick Vaccaro");
MODULE_LICENSE("GPL");
