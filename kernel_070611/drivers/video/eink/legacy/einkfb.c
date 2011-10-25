/*
 * eInk display framebuffer driver
 * 
 * Copyright (C) 2005-2007 Lab126, Inc.  All rights reserved.
 */

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/suspend.h>
#include <asm/system.h>
#include <asm/arch/system.h>
#include <asm/arch/fiona.h>
#include <asm/arch/fpow.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/random.h>

#include <linux/einkfb.h>
#include <linux/pn_lcd.h>
#include "../apollo/apollo.h"

enum update_type
{
	update_overlay = 0,		// Hardware composite on top of existing framebuffer.
	update_overlay_mask,	// Hardware composite on top of existing framebuffer using mask.
	update_partial,			// Software composite *into* existing framebuffer.
	update_full,			// Fully replace existing framebuffer.
	update_full_refresh		// Fully replace existing framebuffer with manual refresh.
};
typedef enum update_type update_type;

enum update_which
{
	update_buffer = 0,		// Just update the buffer, not the screen.
	update_screen			// Update both the buffer and the screen.
};
typedef enum update_which update_which;

struct picture_info_type
{
    int				x, y;
    update_type		update;
    update_which	to_screen;
    int				headerless,
    				xres, yres,
    				bpp;
};
typedef struct picture_info_type picture_info_type;

struct slab_table_type
{
	int	slabs,
		lines,
		rowlongs;
};
typedef struct slab_table_type slab_table_type;

struct slab_type
{
	int	rowlong_start,		// Start...
		rowlong_end,		// ...end of slab in rowlongs.
	
		x1, y1,				// Top, left...
		x2, y2;				// ...bottom, right of slab in coordinates.
};
typedef struct slab_type slab_type;

struct raw_image_type
{
    int xres,
        yres,
        bpp;
    
    u8  start[];
};
typedef struct raw_image_type raw_image_type;

#include "einksp.h"

#define EINK_FB	"eink_fb"
#define VERSION	"1.02"

#undef DEBUG_LOCK_BUG
#ifdef DEBUG_LOCK_BUG
#define lockdebug(x...) printk(x)
#else
#define lockdebug(x...)
#endif

#define	pinfo(format, arg...)	    printk(EINK_FB": " format , ## arg )

#define IN_RANGE(n,m,M)             ((n) >= (m) && (n) <= (M))

// warning!!!  hack -- not true on all architectures
#define VMALLOC_VMADDR(x) ((unsigned long)(x))

// maximum memory used by the e-ink display in 2bpp mode
#define XRES						600
#define YRES						800
#define BPP							2

#define BPP_SIZE(r,b)				(((r)*(b))/8)

#define VIDEOMEMSIZE				BPP_SIZE((XRES*YRES),BPP)

#define PICTBUF						BPP_SIZE((XRES*(YRES+1)),BPP)
#define PICTBUFSIZE					(VIDEOMEMSIZE+PICTBUF)

#define FAKE_PNLCD_SEGMENT_RES      (YRES/(PN_LCD_SEGMENT_COUNT/PN_LCD_COLUMN_COUNT))
#define FAKE_PNLCD_YRES             FAKE_PNLCD_SEGMENT_RES
#define FAKE_PNLCD_XRES_MAX        	(FAKE_PNLCD_SEGMENT_RES*PN_LCD_COLUMN_COUNT)
#define FAKE_PNLCD_BUF_SIZE         BPP_SIZE((FAKE_PNLCD_XRES_MAX*YRES),BPP)

#define FAKE_PNLCD_X_START(n)       (((n) & 1) ? 0 : fake_pnlcd_xres)
#define FAKE_PNLCD_Y_START(n)       (((n) / 2) * FAKE_PNLCD_YRES)

#define FAKE_PNLCD_X_END(s)         ((s) + fake_pnlcd_xres)
#define FAKE_PNLCD_Y_END(s)         ((s) + FAKE_PNLCD_YRES)

#define MAX_SEG(s)					min((((s) | 1) + 1), MAX_PN_LCD_SEGMENT)

#define USE_FAKE_PNLCD(c)           (use_fake_pnlcd && fake_pnlcd_buffer && (c))
#define FAKE_PNLCD()                USE_FAKE_PNLCD(1)

#define FULL_SHUTDOWN				1
#define DONT_SHUTDOWN				0

#define NUM_SLAB_TABLE_ENTRIES		4	// [0]4, [1]8, [2]10, [3]20
#define SLAB_TABLE_ENTRY			3	// 3 -> 20
#define NUM_SLABS					20
#define MAX_SLABS					20					

#define TIMER_EXPIRES(t)			(jiffies + (HZ * (t)))
#define MIN_SCHEDULE_TIMEOUT		0
#define EXPIRE_TIMER				0
#define DELAY_TIMER					1

#define SCREEN_SAVER_PATH_USER		"/mnt/us/system/"
#define SCREEN_SAVER_PATH_SYS		"/opt/var/"

#define SCREEN_SAVER_PATH_TEMPLATE	"screen_saver/screen_saver_%d.gz"
#define SCREEN_SAVER_PATH_LAST		"screen_saver/screen_saver_last"
#define SCREEN_SAVER_PATH_SIZE		256
#define SCREEN_SAVER_DEFAULT		0

// 0.25-second minimum delay through 0.75-second delay-gap maximum.
//
#define UPDATE_DISPLAY_TIMEOUT_INC	(HZ/4)
#define UPDATE_DISPLAY_TIMEOUT_MIN	(UPDATE_DISPLAY_TIMEOUT_INC*1)
#define UPDATE_DISPLAY_TIMEOUT		(UPDATE_DISPLAY_TIMEOUT_INC*2)
#define UPDATE_DISPLAY_TIMEOUT_MAX	(UPDATE_DISPLAY_TIMEOUT_INC*3)

// 3-second time-out on no activity.
//
#define AUTO_POWER_TIMER_DELAY		3
#define AUTO_POWER_TIMER			0x72776F70	// 'powr'

// 1-second time-out on EINK_FULL_UPDATE_ON.
//
#define MAX_FULL_UPDATE_COUNT		5
#define AUTO_SYNC_TO_ASYNC_COUNT	5
#define AUTO_POWER_OFF_COUNT		5
#define AUTO_REFRESH_TIMER_DELAY	1
#define AUTO_REFRESH_TIMER			0x68736672	// 'rfsh'

// Allow the time-outs to vary by a factor.
//
#define AUTO_ADJUSTMENT_FACTOR(v,d)	\
	( 0 >= auto_adjustment_factor ) ? d : v * auto_adjustment_factor

// OSCR Timing Constants.
//
#define TIME_SECONDS				CLOCK_TICK_RATE
#define TIME_MILLISECONDS(t)		(((t) * 10)/(TIME_SECONDS/100))

// Externals
//
extern int fiona_gunzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);
extern int fiona_gzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);
extern int fpow_get_eink_disable_pending(void);
extern void verify_apollo_state(void);
extern int fiona_sys_getpid(void);

// Internals
//
#ifdef CONFIG_FIONA_PM_VIDEO
static struct fpow_component *g_eink_fpow_component_ptr = NULL;
FPOW_POWER_MODE g_eink_power_mode = FPOW_MODE_OFF;

static int __eink_fb_power( u32 mode );
static int __do_eink_fpow_setmode(u32 mode);
static int __do_eink_fpow_mode_sleep(void);
#endif

static void __eink_fb_power_off_clear_screen(void);

static u8 *screen_saver_buffer = NULL;
static u8 *fake_pnlcd_buffer = NULL;
static u8 *picture_buffer = NULL;
static u8 *frame_buffer = NULL;

static int videomemory_local = 0;
static void *videomemory = NULL;

struct page **videopages;
static int numpages;

static u_long fake_pnlcd_buffer_size = FAKE_PNLCD_BUF_SIZE;
static u_long picture_buffer_size = PICTBUFSIZE;
static u_long videomemorysize = VIDEOMEMSIZE;

static int bpp = EINK_DEPTH_2BPP;

static splash_screen_type splash_screen_up = splash_screen_logo;
static int fake_pnlcd_min_seg = FIRST_PNLCD_SEGMENT;
static int fake_pnlcd_max_seg = LAST_PNLCD_SEGMENT;
static int fake_pnlcd_xres = FAKE_PNLCD_XRES_MAX/2;
static int present_screen_savers_in_order = 1;
static int force_full_update_acknowledge = 1;
static int last_display_fx = APOLLO_FX_NONE;
static int use_screen_saver_banner = 0;
static int auto_adjustment_factor = 1;
static int alloc_screen_saver_buf = 0;
static int valid_screen_saver_buf = 0;
static int user_screen_saver_set = 0;
static int get_next_screen_saver = 0;
static int use_reverse_animation = 0;
static int screen_saver_random1 = 0;
static int screen_saver_random2 = 0;
static int update_display_async = 1;
static int auto_sync_to_async = 1;
static int fake_pnlcd_valid = 0;
static int clear_update_fx = 0;
static int dont_deanimate = 0;
static int buffers_synced = 0;
static int use_fake_pnlcd = 0;
static int fake_pnlcd_x = 0;
static int quick_entry = 1;
static int quick_exit = 0;
static int eink_debug = 0;
static int in_probe = 0;

int flash_fail_count = 0;

static char screen_saver_path_template[SCREEN_SAVER_PATH_SIZE];
static char screen_saver_path_last[SCREEN_SAVER_PATH_SIZE];

static int changed_slab_index_start = NUM_SLABS;
static int changed_slab_index_end = NUM_SLABS;
static int num_slabs = NUM_SLABS;

static slab_table_type slab_table[NUM_SLAB_TABLE_ENTRIES] =
{
	{	4,		200,	7500	},
	
	{   8,		100,	3750	},
	
	{  10,		 80,	3000	},
	
	{  20,		 40,	1500	}
};

static slab_type *slab_array = NULL;

static char segments_cache[PN_LCD_SEGMENT_COUNT];

#ifndef MODULE
static int eink_fb_enable __initdata = 0;
#else
static int eink_fb_enable __initdata = 1;
#endif

static unsigned long	 update_display_timeout = UPDATE_DISPLAY_TIMEOUT;
static int				 update_display_which = EINK_FULL_UPDATE_OFF;
static int				 update_display_pending = 0;
static int				 update_display_count = 0;
static int				 update_display_ready = 0;
static pid_t			 update_display_pid = 0;
static wait_queue_head_t update_display_wq;
static DECLARE_COMPLETION(update_display_thread_exited);

static int				 auto_power_timer_delay = AUTO_POWER_TIMER_DELAY;
static int				 auto_power_timer_active = 0;
static struct timer_list auto_power_timer;
static pid_t			 auto_power_pid = 0;
static wait_queue_head_t auto_power_wq;
static DECLARE_COMPLETION(auto_power_thread_exited);

static int				 auto_refresh_timer_update_display_which = EINK_FULL_UPDATE_ON;
static int				 auto_refresh_timer_delay = AUTO_REFRESH_TIMER_DELAY;
static int				 max_full_update_count = MAX_FULL_UPDATE_COUNT;
static int				 auto_refresh_timer_full_update_skip_count = 0;
static int			     auto_refresh_timer_acknowledge = 1;
static int				 auto_refresh_timer_active = 0;
static struct timer_list auto_refresh_timer;

static int				 screen_saver_last = SCREEN_SAVER_DEFAULT;
static pid_t			 screen_saver_pid = 0;
static wait_queue_head_t screen_saver_wq;
static DECLARE_COMPLETION(screen_saver_thread_exited);

DECLARE_MUTEX(apollo_lock);
LIST_HEAD(vma_list);

struct semaphore vma_list_semaphore;

struct fb_vma_list_entry {
	struct list_head list;
	struct vm_area_struct *vma;
};

static struct fb_var_screeninfo eink_fb_default __initdata = {
	.xres = XRES,
	.yres =	YRES,
	.xres_virtual =	XRES,
	.yres_virtual =	YRES,
	.bits_per_pixel = BPP,
	.grayscale = 1,
	.activate =	FB_ACTIVATE_TEST,
	.height = -1,
	.width = -1,
};

static struct fb_fix_screeninfo eink_fb_fix __initdata = {
	.id = EINK_FB,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_STATIC_PSEUDOCOLOR,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
	.accel = FB_ACCEL_NONE,
};

module_param(auto_refresh_timer_delay, int, S_IRUGO);
module_param(auto_power_timer_delay, int, S_IRUGO);
module_param(max_full_update_count, int, S_IRUGO);

module_param(alloc_screen_saver_buf, int, S_IRUGO);
module_param(use_reverse_animation, int, S_IRUGO);
module_param(splash_screen_up, int, S_IRUGO);
module_param(use_fake_pnlcd, int, S_IRUGO);
module_param(fake_pnlcd_x, int, S_IRUGO);
module_param(quick_entry, int, S_IRUGO);
module_param(quick_exit, int, S_IRUGO);
module_param(num_slabs, int, S_IRUGO);

module_param(bpp, int, S_IRUGO);

int eink_fb_setup(char *);
int eink_fb_init(void);

static void __update_display_partial(u8 *buffer, int buffer_size, int xstart, int xend, int ystart, int yend);
static void __update_display(int update_mode);
static void __sleep_update_display_thread(void);
static void wake_update_display_thread(void);
static void wake_screen_saver_thread(void);
static void prime_auto_refresh_timer(int delay_timer);
static void prime_auto_power_timer(int delay_timer);

static int _eink_sys_ioctl(unsigned int cmd, unsigned long arg);
void __eink_clear_screen(int update_mode);

#define EINK_IOCTL_FLAG             	0x6B6E6965	// 'eink'
#define EINK_IOCTL_LOCK_FLAG			0x6B636F6C	// 'lock'
#define EINK_IOCTL_FPOW_FLAG			0x776F7066	// 'fpow'
#define __eink_sys_ioctl_fpow(cmd, arg)	eink_fb_ops.fb_ioctl((struct inode *)EINK_IOCTL_FLAG, (struct file *)EINK_IOCTL_LOCK_FLAG, (unsigned int)cmd, (unsigned long)arg, (struct fb_info *)EINK_IOCTL_FPOW_FLAG)
#define __eink_sys_ioctl(cmd, arg)		eink_fb_ops.fb_ioctl((struct inode *)EINK_IOCTL_FLAG, (struct file *)EINK_IOCTL_LOCK_FLAG, (unsigned int)cmd, (unsigned long)arg, NULL)

#define LOCK_APOLLO_ENTRY()	lock_apollo_entry((char *)__FUNCTION__)
#define LOCK_APOLLO_EXIT()	lock_apollo_exit((char *)__FUNCTION__)

static int lock_apollo_entry(char *function_name)
{
	int result = 0;
	
	lockdebug("%s(pid=%d) getting lock...\n", function_name, fiona_sys_getpid());
	
	// Did we get the lock?
	//
	if ( down_interruptible(&apollo_lock) )
	{
		lockdebug("%s(pid=%d) COULD NOT get lock...\n", function_name, fiona_sys_getpid());
		result = -ERESTARTSYS;
	}
	else
		lockdebug("%s(pid=%d) got lock...\n", function_name, fiona_sys_getpid());

	// No, release it.
	//
	if ( result )
		up(&apollo_lock);

	return ( result );
}

static void lock_apollo_exit(char *function_name)
{
	up(&apollo_lock);
	lockdebug("%s(pid=%d) released lock...\n", function_name, fiona_sys_getpid());
}

static int begin_apollo_wake_on_activity(int *saved_apollo_power_state)
{
	unsigned char saved_init_display_speed = apollo_get_init_display_speed();
	int result;
	
	// Return the current power state if requested.
	//
	if ( saved_apollo_power_state )
		*saved_apollo_power_state = apollo_get_power_state();

	// For internally generated wakes, we don't need to go through the full
	// init cycle.  So, we tell it to skip that process here, but we also
	// preserve the overall setting.
	//
	apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_SKIP);
	result = apollo_wake_on_activity();
	
	apollo_set_init_display_speed(saved_init_display_speed);
	
	// Return whether we woke or not.
	//
	return ( result );
}

static void end_apollo_wake_on_activity(int saved_apollo_power_state)
{
	int skipped_restore = 1;
	
	// Only restore the saved power state if we haven't been purposely
	// been taken out of the wake ("normal") state for some reason.
	//
	if ( APOLLO_NORMAL_POWER_MODE == apollo_get_power_state() )
	{
		if ( AUTO_POWER_OFF_COUNT > auto_refresh_timer_full_update_skip_count )
		{
			apollo_set_power_state(saved_apollo_power_state);
			skipped_restore = 0;
		}
	}

	// Don't get back here in a hurry if we've skipped the restore.
	//
	if ( skipped_restore )
		prime_auto_power_timer(DELAY_TIMER);
}

#define splash_screen_activity_begin 1
#define splash_screen_activity_end	 0	

#define begin_splash_screen_activity(s) splash_screen_activity(splash_screen_activity_begin, s)
#define end_splash_screen_activity(s)	splash_screen_activity(splash_screen_activity_end, s)

static void
splash_screen_activity(int which_activity, splash_screen_type which_screen)
{
	if ( FRAMEWORK_RUNNING() )
	{
		int reprime_timers = 1, reprime_state = DELAY_TIMER,
			dont_ack_power_power_manager = 0,
			screen_saver_thread_wake = 0,
			ack_power_manager = 0,
			dont_delay = 0;
		pnlcd_blocking_cmds pnlcd_blocking_cmd;
		pnlcd_animation_t pnlcd_animation;

		switch ( which_screen )
		{
			// Block/unblock PNLCD activity, and, on entry, ACK the Power Manager.
			//
			case splash_screen_powering_off:
			case splash_screen_powering_off_wireless:
			case splash_screen_sleep:
			case splash_screen_power_off_clear_screen:
			case splash_screen_sleep_status_lo:
			case splash_screen_sleep_status_hi:
			case splash_screen_screen_saver_picture:
				if ( splash_screen_activity_begin == which_activity )
				{
					pnlcd_blocking_cmd = start_block;
					ack_power_manager = 1;
				}
				else
					pnlcd_blocking_cmd = stop_block;
				
				pnlcd_sys_ioctl(PNLCD_CLEAR_ALL_IOCTL, 0);
			goto entry_exit_common;
			
			// Start/stop PNLCD animation, delay other PNLCD activity if specified,
			// initiate getting the next screen saver if specified, and ACK the Power
			// Manager on exit if specified.
			//
			case splash_screen_logo:
			case splash_screen_usb:
			case splash_screen_usb_internal:
			case splash_screen_usb_external:
				dont_ack_power_power_manager = 1;
				dont_delay = 1;
			goto exit_common;

			case splash_screen_exit:
			case splash_screen_exit_fade:
				if ( get_next_screen_saver )
					screen_saver_thread_wake = 1;
				
			exit_common:
			case splash_screen_powering_on:
			case splash_screen_powering_on_wireless:
				if ( splash_screen_activity_begin == which_activity )
				{
					pnlcd_sys_ioctl(PNLCD_BLOCK_IOCTL, stop_block);
					pnlcd_sys_ioctl(PNLCD_CLEAR_ALL_IOCTL, 0);
					
					if ( use_reverse_animation )
					{
						pnlcd_animation.cmd = set_animation_type;
						pnlcd_animation.arg = sp_animation_rotate_reverse;
						
						pnlcd_sys_ioctl(PNLCD_ANIMATE_IOCTL, &pnlcd_animation);
					}

					pnlcd_animation.cmd = start_animation;
					pnlcd_animation.arg = pnlcd_animation_auto;
					
					pnlcd_blocking_cmd  = start_delay;
				}
				else
				{
					pnlcd_animation.cmd = stop_animation;
					pnlcd_animation.arg = 0;
					
					pnlcd_blocking_cmd  = stop_delay_restore;
					
					if ( !dont_ack_power_power_manager )
						ack_power_manager = 1;
				}

				if ( !dont_deanimate )
					pnlcd_sys_ioctl(PNLCD_ANIMATE_IOCTL, &pnlcd_animation);

			entry_exit_common:
				if ( !dont_delay )
					pnlcd_sys_ioctl(PNLCD_BLOCK_IOCTL, pnlcd_blocking_cmd);
			
			case splash_screen_framework_reset:
				
				// On entry, force all of our timers to expire and note
				// that the next (externally-generated) update needs to
				// be full.
				//
				// On exit, make sure that we don't reprime our timers.
				//
				if ( splash_screen_activity_begin == which_activity )
				{
					force_full_update_acknowledge = 0;
					reprime_state = EXPIRE_TIMER;
				}
				else
					reprime_timers = 0;
			break;

			default:
			break;
		}

		// If necessary, ACK the Power Manager.
		//
		if ( ack_power_manager )
			fpow_video_updated();
			
		// If necessary, wake the thread that gets the next screen
		// saver.
		//
		if ( screen_saver_thread_wake )
			wake_screen_saver_thread();
		
		// If necessary, reprime our timers in the appropriate state.
		//
		if ( reprime_timers )
		{
			prime_auto_refresh_timer(reprime_state);
			prime_auto_power_timer(reprime_state);
		}
	}

	// Perform general post-update activities.
	//
	if ( splash_screen_activity_end == which_activity )
	{
		splash_screen_up = splash_screen_invalid;
		__sleep_update_display_thread();
	}
}

#define from_videomemory_to_frame_buffer 0
#define from_frame_buffer_to_videomemory 1
#define from_screen_saver_to_videomemory 2
#define from_picture_bufr_to_videomemory 3
#define from_picture_bufr_to_screensaver 4

#define memcpy_from_frame_buffer_to_videomemory()	sync_buffer(from_frame_buffer_to_videomemory)
#define memcpy_from_videomemory_to_frame_buffer()	sync_buffer(from_videomemory_to_frame_buffer)
#define memcpy_from_screen_saver_to_videomemory()	sync_buffer(from_screen_saver_to_videomemory)
#define memcpy_from_picture_bufr_to_videomemory()	sync_buffer(from_picture_bufr_to_videomemory)
#define memcpy_from_picture_bufr_to_screensaver()	sync_buffer(from_picture_bufr_to_screensaver)

#define USE_SLAB_ARRAY() (EINK_DEPTH_2BPP == bpp)

static int
sync_buffer(int which)
{
	int buffers_equal = 1, slab_dirty[MAX_SLABS] = { 0 };
	register u32 *src, *dst;
	register int i, j, k;

	switch ( which )
	{
		case from_videomemory_to_frame_buffer:
			src = (u32 *)videomemory;
			dst = (u32 *)frame_buffer;
		break;
		
		case from_frame_buffer_to_videomemory:
			src = (u32 *)frame_buffer;
			dst = (u32 *)videomemory;
		break;
		
		case from_screen_saver_to_videomemory:
			src = (u32 *)screen_saver_buffer;
			dst = (u32 *)videomemory;
		break;
		
		case from_picture_bufr_to_videomemory:
			src = (u32 *)picture_buffer;
			dst = (u32 *)videomemory;
		break;
		
		case from_picture_bufr_to_screensaver:
			src = (u32 *)picture_buffer;
			dst = (u32 *)screen_saver_buffer;
		break;
		
		default:
			src = dst = NULL;
		break;
	}
	
	if ( src && dst )
	{
		if ( USE_SLAB_ARRAY() )
		{
			changed_slab_index_start = changed_slab_index_end = num_slabs;
		
			for ( i = 0; i < num_slabs; i++ )
			{
				for ( j = slab_array[i].rowlong_start, k = slab_array[i].rowlong_end; j < k; j++ )
				{
					if ( !slab_dirty[i] )
					{
						if ( dst[j] != src[j] )
						{
							slab_dirty[i] = 1;
							dst[j] = src[j];
						}
					}
					else
						dst[j] = src[j];
				}
			
				if ( slab_dirty[i] )
				{
					buffers_equal = 0;
					
					if ( num_slabs == changed_slab_index_start )
						changed_slab_index_start = changed_slab_index_end = i;
					else
						changed_slab_index_end = i;
				}
			}
		
			if ( !buffers_equal )
			{
				if ( (0 == changed_slab_index_start) && ((num_slabs-1) == changed_slab_index_end) )
					changed_slab_index_start = changed_slab_index_end = num_slabs;
			}
		}
		else
		{
			for ( i = 0, j = videomemorysize >> 2; i < j; i++ )
			{
				if ( buffers_equal )
				{
					if ( dst[i] != src[i] )
					{
						buffers_equal = 0;
						dst[i] = src[i];
					}
				}
				else
					dst[i] = src[i];
			}
		}
	}

	return ( buffers_equal );
}

static slab_type *
build_slab_array(void)
{
	int i, slab_table_entry = NUM_SLAB_TABLE_ENTRIES;
	
	// Validate num_slabs (could be passed-in as a module_param).
	//
	for ( i = 0; (i < NUM_SLAB_TABLE_ENTRIES) && (NUM_SLAB_TABLE_ENTRIES == slab_table_entry); i++ )
		if ( slab_table[i].slabs == num_slabs )
			slab_table_entry = i;
	
	// If invalid, use default.
	//
	if ( NUM_SLAB_TABLE_ENTRIES == slab_table_entry )
		slab_table_entry = SLAB_TABLE_ENTRY;

	// Build slab array.
	//
	num_slabs = slab_table[slab_table_entry].slabs;

	slab_array = (slab_type *)kmalloc(num_slabs * sizeof(slab_type), GFP_KERNEL);

	if ( slab_array )
	{
		for ( i = 0; i < num_slabs; i++ )
		{
			slab_array[i].rowlong_start = slab_table[slab_table_entry].rowlongs * i;
			slab_array[i].rowlong_end   = slab_table[slab_table_entry].rowlongs * (i + 1);
			
			slab_array[i].x1 			= 0;
			slab_array[i].x2 			= XRES;
			
			slab_array[i].y1 			= slab_table[slab_table_entry].lines * i;
			slab_array[i].y2 			= slab_table[slab_table_entry].lines * (i + 1);
		}
	}

	return ( slab_array );
}

static void
free_slab_array(void)
{
	if ( slab_array )
		kfree(slab_array);
}

static void
set_fake_pnlcd_x_values(int x)
{
	// For now, if x is non-zero, put the fake PNLCD on the right and make it full size.
	// Otherwise, put it on the left and make it half size.
	//
	if ( x )
	{
		fake_pnlcd_xres = FAKE_PNLCD_XRES_MAX;
		fake_pnlcd_x = XRES - fake_pnlcd_xres;
	}
	else
	{
		fake_pnlcd_xres = FAKE_PNLCD_XRES_MAX/2;
		fake_pnlcd_x = 0;
	}
}

static int
cache_segment_check(char *segments)
{
    int result = 0;
    
    if ( segments )
    {
        // When passed the segments' cache, always apply it.
        //
        if ( segments == segments_cache )
            result = !PNLCD_EVENT_PENDING();
        else
        {
            // Otherwise, check to see whether the passed-in segments
            // match the cache.
            //
            int i, segments_equal = 1;
            
            for ( i = 0; i < PN_LCD_SEGMENT_COUNT; i++ )
            {
                if ( segments_equal )
                {
                    // If not equal, update the cache.
                    //
                    if ( segments_cache[i] != segments[i] )
                    {
                        segments_cache[i] = segments[i];
                        segments_equal = 0;
                    }
                }
                else
                    segments_cache[i] = segments[i];
            }
            
            // If not equal, apply them.
            //
            result = !segments_equal;
        }
    }

    return ( result );
}

static void
update_fake_pnlcd_segment(int seg_num, int seg_state)
{
    if ( USE_FAKE_PNLCD(IN_RANGE(seg_num, FIRST_PNLCD_SEGMENT, LAST_PNLCD_SEGMENT))  )
    {
        u32 x, x_start, x_end, rowbytes, xres = fake_pnlcd_xres,
            y, y_start, y_end, bytes;
            
        u8  *fb_base = fake_pnlcd_buffer;
        
        // Get (x, y) starting position, based on segment number.
        //
        x_start  = FAKE_PNLCD_X_START(seg_num);
        y_start  = FAKE_PNLCD_Y_START(seg_num);
        
        x_end    = FAKE_PNLCD_X_END(x_start);
        y_end    = FAKE_PNLCD_Y_END(y_start);
        
        // Make bpp-related adjustments.
        //
        x_start  = BPP_SIZE(x_start, bpp);
        x_end    = BPP_SIZE(x_end,	 bpp);
        rowbytes = BPP_SIZE(xres,	 bpp);
        
        // Draw segment into buffer.
        //
        for (bytes = 0, y = y_start; y < y_end; y++)
            for ( x = x_start; x < x_end; x++)
                fb_base[(rowbytes * y) + x] = (SEGMENT_ON == seg_state) ? APOLLO_BLACK : APOLLO_WHITE;
    }
}

static unsigned long fake_pnlcd_start_draw, fake_pnlcd_end_draw;

static void
__update_fake_pnlcd(char *segments)
{
    if ( USE_FAKE_PNLCD(cache_segment_check(segments)) )
    {
        int x_start = fake_pnlcd_x,
            x_end   = x_start + fake_pnlcd_xres,
            y_start,
            y_end,
            
            buffer_size = fake_pnlcd_buffer_size,
            
            min_seg = 0,
            max_seg = 0,
            
            i;
            
        u8	*buffer = fake_pnlcd_buffer;

        pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();
    
        // Update each of the PNLCD segments in the fake PNLCD segment buffer with its
        // latest value, remembering the segment-on span, and clear the entire buffer
        // to white first.
        //
        memset(buffer, APOLLO_WHITE, buffer_size);
        
        for ( i = 0; i < PN_LCD_SEGMENT_COUNT; i++ )
        {
            if ( SEGMENT_ON == segments[i] )
            {
                if ( 0 == min_seg )
                	min_seg = i;
                	
               	if ( i > max_seg )
               		max_seg = i;
                
                update_fake_pnlcd_segment(i, segments[i]);
            }
        }
        
        // Skip to the next row to ensure that the previous segment is erased. 
        //
        max_seg = MAX_SEG(max_seg);
        
        // If we were that last ones to draw, then we need to erase where
        // were last.  Otherwise, we just need to draw where we are now.
        //
        if ( fake_pnlcd_valid )
        {
        	y_start = FAKE_PNLCD_Y_START(min(min_seg, fake_pnlcd_min_seg)), 
        	y_end   = FAKE_PNLCD_Y_START(max(max_seg, fake_pnlcd_max_seg));
        }
        else
        {
        	y_start = FAKE_PNLCD_Y_START(min_seg), 
        	y_end   = FAKE_PNLCD_Y_START(max_seg);
        }

        y_end = FAKE_PNLCD_Y_END(y_end);
        	
		buffer = &fake_pnlcd_buffer[BPP_SIZE(fake_pnlcd_xres, bpp) * y_start];
		buffer_size = BPP_SIZE((fake_pnlcd_xres * (y_end - y_start)), bpp);

        fake_pnlcd_min_seg = min_seg;
        fake_pnlcd_max_seg = max_seg;
        
        // Now, update the screen with the PNLCD segment buffer, using the segment buffer
        // itself as a mask; and, if there is one, using the last-set display FX.
        //
        apollo_set_display_fx(APOLLO_FX_BUF_IS_MASK);
        apollo_set_mask_fx(last_display_fx);
        
        pnlcd_flags->updating = 1;
        fake_pnlcd_start_draw = OSCR;
        
        __update_display_partial(buffer, buffer_size, x_start, x_end, y_start, y_end);
        fake_pnlcd_valid = 1;

        fake_pnlcd_end_draw = OSCR;
        pnlcd_flags->updating = 0;

        apollo_set_mask_fx(APOLLO_FX_NONE);
        apollo_set_display_fx(APOLLO_FX_NONE);
        
        // Report drawing time.
        //
        einkdebug("__update_fake_pnlcd: bytes sent = %d, time = %lums\n", buffer_size, 
        	TIME_MILLISECONDS(fake_pnlcd_end_draw - fake_pnlcd_start_draw));
    }
}

static void
fake_pnlcd_test(void)
{
	if ( FAKE_PNLCD() )
	{
		char segments[PN_LCD_SEGMENT_COUNT], saved_segments[PN_LCD_SEGMENT_COUNT];
		unsigned long draw_starts[(PN_LCD_SEGMENT_COUNT/4) + 1];
		int i, j;
		
		// Save the current segments' cache for restoring later.
		//
		memcpy(saved_segments, segments_cache, PN_LCD_SEGMENT_COUNT);
		
		// Clear the current segments.
		//
		memset(segments, SEGMENT_OFF, PN_LCD_SEGMENT_COUNT);
		_eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, (unsigned long)segments);
		
		// Draw and erase fifty 4-unit segments, gathering each of their start times as
		// we go.
		//
		for ( i = 0; i < PN_LCD_SEGMENT_COUNT; i+= 4 )
		{
			for ( j = i; j < (i + 4); j++ )
				segments[j] = SEGMENT_ON;
			
			_eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, (unsigned long)segments);
			draw_starts[i/4] = fake_pnlcd_start_draw;
			
			for ( j = i; j < (i + 4); j++ )
				segments[j] = SEGMENT_OFF;
		}
		
		// Restore the original segments.
		//
		_eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, (unsigned long)saved_segments);
		draw_starts[PN_LCD_SEGMENT_COUNT/4] = fake_pnlcd_start_draw;
		
		// Print out the screen-to-screen start-time deltas for the 4-unit segments we drew.
		//
		printk("fake_pnlcd_test, screen-to-screen drawing times:\n");
		
		for ( i = 0; i < PN_LCD_SEGMENT_COUNT/4; i++ )
		{
			printk("  segments 0x%02X-0x%02X time = %4lums\n", (i*4), (i*4)+3,
				TIME_MILLISECONDS(draw_starts[i+1] - draw_starts[i]));
		}
	}
}

static void
__update_display(
	int update_mode)
{
    u32 screen_cleared = apollo_load_image_data(videomemory, videomemorysize) && (APOLLO_FX_NONE == apollo_get_display_fx());

	if ( !screen_cleared || (screen_cleared && !get_screen_clear()) )
	{
#ifdef WRITE_FX_BACK_TO_BUFFER
		buffers_synced = 0;
#endif
		if ( EINK_FULL_UPDATE_ON == update_mode )
		{
			auto_refresh_timer_acknowledge = 1;
			force_full_update_acknowledge = 1;
		}
			
		apollo_display(update_mode);
		
		if ( !buffers_synced )
			memcpy_from_videomemory_to_frame_buffer();
		
		set_screen_clear(screen_cleared);
	}

	end_splash_screen_activity(splash_screen_up);
	fake_pnlcd_valid = 0;
}

static raw_image_type *uncompress_picture(u32 picture_length, u8 *picture)
{
	raw_image_type *result = NULL;

	if ( picture && picture_length )
	{
		unsigned long length = picture_length;
		
		if ( 0 == fiona_gunzip(picture_buffer, picture_buffer_size, picture, &length) )
			result = (raw_image_type *)picture_buffer;
	}

	return ( result );
}

static int compress_picture(u32 picture_length, u8 *picture)
{
	int result = 0;

	if ( picture && picture_length )
	{
		unsigned long length = picture_length;
		
		if ( 0 == fiona_gzip(picture_buffer, picture_buffer_size, picture, &length) )
			result = length;
	}

	return ( result );
}

static int
bounds_are_acceptable(
	int xstart, int xres,
	int ystart, int yres)
	
{
	int acceptable = 1;
	
    // The limiting boundary must be within the screen's boundaries.
    //
    acceptable &= ((xres >= 0) && (xres <= XRES));
    acceptable &= ((yres >= 0) && (yres <= YRES));
	
	// Our starting and ending points must be byte aligned.
	//
	acceptable &= (0 == (xstart % 8)) && (0 == (xres % 8));
	
	// Image must fit within framebuffer.
	//
	acceptable &= (xstart + xres) <= XRES;
	acceptable &= (ystart + yres) <= YRES;

	return ( acceptable );
}

static int
picture_is_acceptable(
	picture_info_type *picture_info,
	raw_image_type *picture)
{
	int acceptable = 0;

	if ( picture && picture_info )
	{
		int x = (0 > picture_info->x) ? 0 : picture_info->x,
			y = (0 > picture_info->y) ? 0 : picture_info->y;
		int picture_xres, picture_yres, picture_bpp;
		
		// If the picture is headerless, get the header information from picture_info.
		//
		if ( picture_info->headerless )
		{
			picture_xres = picture_info->xres;
			picture_yres = picture_info->yres;
			picture_bpp  = picture_info->bpp;
		}
		else
		{
			picture_xres = picture->xres;
			picture_yres = picture->yres;
			picture_bpp  = picture->bpp;
		}
		
		// Assume all is well.
		//
		acceptable = 1;
		
		// Picture and framebuffer need to be the same depth.
		//
		acceptable &= (bpp == picture_bpp);
		
		// Picture must be in bounds.
		//
		acceptable &= bounds_are_acceptable(x, picture_xres, y, picture_yres);
	}
	
	return ( acceptable );
}

static void
__display_splash_screen(
	splash_screen_type which_screen)
{
	//  Whether we're being asked to draw the same screen or not,
	//	begin (or re-begin) any screen-specific activity.
	//
	begin_splash_screen_activity(which_screen);

	// Don't redraw if the screen we're being asked to display
	// is already up.
	//
	if ( splash_screen_up != which_screen )
	{
		picture_info_type *picture_info = NULL; 
		raw_image_type *picture = NULL;
		int splash_screen = 1;
		u32 picture_len = 0;
	
		switch ( which_screen )
		{
			case splash_screen_powering_off:
				if ( !quick_entry )
				{
					picture = (raw_image_type *)picture_powering_off;
					picture_info = &picture_powering_off_info;
					picture_len = picture_powering_off_len;
				}
				else
					goto entry_common;
			break;
			
			case splash_screen_powering_on:
				if ( !quick_exit )
				{
					//picture = (raw_image_type *)picture_powering_on;
					//picture_info = &picture_powering_on_info;
					//picture_len = picture_powering_on_len;
					goto logo_common;
				}
				else
					goto exit_common;
			break;
			
			case splash_screen_powering_off_wireless:
				if ( !quick_entry )
				{
					picture = (raw_image_type *)picture_powering_off_wireless;
					picture_info = &picture_powering_off_wireless_info;
					picture_len = picture_powering_off_wireless_len;
				}
				else
					goto entry_common;
			break;
			
			case splash_screen_powering_on_wireless:
				if ( !quick_exit )
				{
					//picture = (raw_image_type *)picture_powering_on_wireless;
					//picture_info = &picture_powering_on_wireless_info;
					//picture_len = picture_powering_on_wireless_len;
					goto logo_common;
				}
				else
					goto exit_common;
			break;
			
			case splash_screen_exit_fade:
				//if ( !quick_exit )
				//	__update_display(EINK_FULL_UPDATE_OFF);
			goto exit_common;
	
			case splash_screen_exit:
				//if ( !quick_exit )
				//__eink_clear_screen(EINK_FULL_UPDATE_OFF);
			goto exit_common;
			
			entry_common:
				__eink_clear_screen(EINK_FULL_UPDATE_ON);
			exit_common:
				set_drivemode_screen_ready(0);
			
			case splash_screen_framework_reset:
				splash_screen_up = which_screen;
				splash_screen = 0;
			break;
			
			case splash_screen_logo:
			logo_common:
				picture_len = get_user_boot_screen();
				
				if ( picture_len )
				{
					picture = (raw_image_type *)(videomemory + videomemorysize);
					picture_info = &picture_logo_info_user_boot_screen;
				}
				else
				{
					picture = (raw_image_type *)picture_logo;
					picture_len = picture_logo_len;
					
					if ( get_screen_clear() )
						picture_info = &picture_logo_info_update_partial;
					else
						picture_info = &picture_logo_info_update_full;
				}
			break;
			
			case splash_screen_usb_internal:
				picture = (raw_image_type *)picture_usb_internal;
				picture_info = &picture_usb_internal_info;
				picture_len = picture_usb_internal_len;
			break;
			
			case splash_screen_usb_external:
				picture = (raw_image_type *)picture_usb_external;
				picture_info = &picture_usb_external_info;
				picture_len = picture_usb_external_len;
			break;
			
			case splash_screen_usb:
				picture = (raw_image_type *)picture_usb;
				picture_info = &picture_usb_info;
				picture_len = picture_usb_len;
				
				set_drivemode_screen_ready(0);
				dont_deanimate = 1;
			break;
			
			case splash_screen_sleep:
				picture = (raw_image_type *)picture_sleep;
				picture_info = &picture_sleep_info;
				picture_len = picture_sleep_len;
			break;

			case splash_screen_update:
				picture = (raw_image_type *)picture_update;
				picture_info = &picture_update_info;
				picture_len = picture_update_len;
			break;

			case splash_screen_blank_status_lo:
			case splash_screen_blank_status_hi:
				picture = (raw_image_type *)picture_blank_status;
				picture_len = picture_blank_status_len;
				
				if ( splash_screen_blank_status_lo == which_screen )
					picture_info = &picture_blank_status_lo_info;
				else
					picture_info = &picture_blank_status_hi_info;
			break;
			
			case splash_screen_blank_status_stub_lo:
			case splash_screen_blank_status_stub_hi:
				picture = (raw_image_type *)picture_blank_status_stub;
				picture_len = picture_blank_status_stub_len;
				
				if ( splash_screen_blank_status_stub_lo == which_screen )
					picture_info = &picture_blank_status_lo_info;
				else
					picture_info = &picture_blank_status_hi_info;
			break;
			
			case splash_screen_sleep_status_lo:
			case splash_screen_sleep_status_hi:
				picture = (raw_image_type *)picture_sleep_status;
				picture_len = picture_sleep_status_len;
				
				if ( splash_screen_sleep_status_lo == which_screen )
					picture_info = &picture_sleep_status_lo_info;
				else
					picture_info = &picture_sleep_status_hi_info;
			break;
			
			case splash_screen_store_exit:
				picture = (raw_image_type *)picture_store_exit;
				picture_info = &picture_store_exit_info;
				picture_len = picture_store_exit_len;
			break;
			
			case splash_screen_screen_saver_picture:
				if ( use_screen_saver_banner )
				{
					picture = (raw_image_type *)picture_sleep;
					picture_info = &picture_screen_saver_info;
					picture_len = picture_sleep_len;
				}
				else
				{
					splash_screen_up = splash_screen_screen_saver_picture;
					splash_screen = 0;
				}
			break;
			
			// Prevent compiler from complaining.
			//
			default:
			break;
		}
				
		if ( splash_screen && (NULL != (picture = uncompress_picture(picture_len, (u8 *)picture))) )
		{
			update_type update = picture_info->update;

			// Our convention is that overlay masks are located immediately following the
			// source picture, and are, otherwise, exactly the same size as the source
			// picture.  Thus, their xres is the same but their yres is double.
			//
			if ( update_overlay_mask == update )
				picture->yres /= 2;
			
			if ( picture_is_acceptable(picture_info, picture) )
			{
				u32 x, x_start, x_end, rowbytes, xres = XRES,
					y, y_start, y_end, bytes,    yres = YRES;
					
				u8  *fb_base = (u8 *)videomemory, *picture_start;
				
				int picture_xres, picture_yres, picture_bpp;
				
				// If the picture is headerless, get the header information from picture_info.
				//
				if ( picture_info->headerless )
				{
					picture_xres  = picture_info->xres;
					picture_yres  = picture_info->yres;
					picture_bpp   = picture_info->bpp;
					picture_start = (u8 *)picture;
				}
				else
				{
					picture_xres  = picture->xres;
					picture_yres  = picture->yres;
					picture_bpp   = picture->bpp;
					picture_start = picture->start;
				}

				// Get (x, y) offset.
				//
				x_start = (0 > picture_info->x) ? ((xres - picture_xres) >> 1) : picture_info->x;
				x_end   = x_start + picture_xres;
				
				y_start = (0 > picture_info->y) ? ((yres - picture_yres) >> 1) : picture_info->y;
				y_end   = y_start + picture_yres;
					
				// If we're overlaying, use the picture's buffer.  Otherwise,
				// move the picture buffer to the framebuffer.
				//
				if ( (update_overlay == update) || (update_overlay_mask == update) )
				{
					int picture_size = BPP_SIZE((picture->xres * picture->yres), bpp);
				
					// If we're using an overlay mask, put ourselves into mask mode.
					//
					if ( update_overlay_mask == update )
						apollo_set_display_fx(APOLLO_FX_MASK);
	
					__update_display_partial(picture_start, picture_size, x_start, x_end, y_start, y_end);
					apollo_set_display_fx(APOLLO_FX_NONE);
				}
				else
				{
					// Make bpp-related adjustments.
					//
					x_start  = BPP_SIZE(x_start, bpp);
					x_end    = BPP_SIZE(x_end,	 bpp);
					rowbytes = BPP_SIZE(xres,	 bpp);
					
					// Clear background when we're doing full updates
					//
					if ( (update_full == update) || (update_full_refresh == update) )
						memset(videomemory, APOLLO_WHITE, videomemorysize);
					
					// Blit to framebuffer and update the display.
					//
					for (bytes = 0, y = y_start; y < y_end; y++)
						for ( x = x_start; x < x_end; x++)
							fb_base[(rowbytes * y) + x] = picture_start[bytes++];
		
					if ( picture_info->to_screen )
						__update_display((update_full_refresh == update) ? EINK_FULL_UPDATE_ON : EINK_FULL_UPDATE_OFF);
				}
	
				splash_screen_up = which_screen;
			}
		}
	}
	else
		einkdebug("__display_splash_screen(which_screen=%d) - skipping\n", splash_screen_up);
}

static void
__display_drivemode_screen(
	splash_screen_type which_screen)
{
	switch ( which_screen )
	{
		case splash_screen_drivemode_1:
			__display_splash_screen(splash_screen_usb_internal);
		break;
		
		case splash_screen_drivemode_2:
			__display_splash_screen(splash_screen_usb_external);
		break;
		
		case splash_screen_drivemode_3:
			__display_splash_screen(splash_screen_usb_internal);
			__display_splash_screen(splash_screen_usb_external);
		break;
		
		// Prevent compiler from complaining.
		//
		default:
		break;
	}

	// Stop PNLCD animation.
	//
	dont_deanimate = 0;
	end_splash_screen_activity(splash_screen_up);
	
	// Start drivemode screen animation.
	//
	set_drivemode_screen_ready(1);
}

// The status bar is usually at the bottom of the screen but,
// when the search bar is up (and it's automatically up when
// the store is running), it's higher.
//
#define lo_status_location_x1	0
#define lo_status_location_y1	767
#define lo_status_location_x2	596
#define	lo_status_location_y2	759

#define hi_status_location_x1  	0
#define hi_status_location_y1	703
#define hi_status_location_x2	596
#define hi_status_location_y2	695

#define status_value_2bpp_left	0xAA
#define status_value_2bpp_right	0x55

static int value_at_location(int location_x, int location_y, u8 value)
{
	u32 x = BPP_SIZE(location_x, bpp), y = location_y, rowbytes = BPP_SIZE(XRES, bpp);
	u8  *fb_base = (u8 *)videomemory;
	int value_status = 0;

	if ( value == fb_base[(rowbytes * y) + x] )
		value_status = 1;
		
	return ( value_status );
}

static splash_screen_type
get_which_status(void)
{
	splash_screen_type which_status = splash_screen_invalid;
	
	// The status bar can only be drawn when the framework is running.
	//
	if ( FRAMEWORK_RUNNING() )
	{
		u32 x = lo_status_location_x1 , y = lo_status_location_y1;
	
		// Check the lower, left-hand side first.  If we find a match, then say
		// that we want to check the lower, right-hand side.  Otherwise,
		// we want to check the upper, right-hand side.
		//
		if ( value_at_location(x, y, status_value_2bpp_left) )
		{
			x = lo_status_location_x2;
			y = lo_status_location_y2;
			
			which_status = splash_screen_blank_status_lo;
		}
		else
		{
			x = hi_status_location_x2;
			y = hi_status_location_y2;
			
			which_status = splash_screen_blank_status_hi;
		}
	
		// Check right-hand side, either high or low, depending on what was found
		// on the left-hand side.  If we find a match, then we need the stub.  Not
		// finding a match means that we're either in a book, where there's no
		// right-hand side bar, or a menu is up.  Either way, we don't need the stub.
		//
		if ( value_at_location(x, y, status_value_2bpp_right) )
			which_status++;
	}

	return ( which_status );
}

#define BLANK_STATUS	1
#define KEEP_STATUS		0

static splash_screen_type
__display_screen_faded(int which_fade, int blank_status)
{
	splash_screen_type which_status = get_which_status();
	
	// Blank the status area of the screen if requested.
	//
	if ( blank_status )
		__display_splash_screen(which_status);
	
	// Fade the entire screen out.
	//
	apollo_set_display_fx(which_fade);
	__update_display(EINK_FULL_UPDATE_OFF);
	apollo_set_display_fx(APOLLO_FX_NONE);
	
	return ( which_status );
}

static void
__display_screen_faded_with_overlay_mask(int which_fade, splash_screen_type which_screen)
{
	int which_status = KEEP_STATUS;
	
	// Fade the entire screen out first, as required by which_screen.
	//
	if ( splash_screen_sleep_fade == which_screen )
		which_status = BLANK_STATUS;

	which_status = __display_screen_faded(which_fade, which_status);
	
	// Put the sleep screen's message in the right place.
	//
	if ( splash_screen_sleep_fade == which_screen )
	{
		 if ( (splash_screen_blank_status_hi == which_status) || (splash_screen_blank_status_stub_hi == which_status) )
		 	which_screen = splash_screen_sleep_status_hi;
		 else
		 	which_screen = splash_screen_sleep_status_lo;
	}

	// Now, do the masked overlay.
	//
    apollo_set_mask_fx(which_fade);
	__display_splash_screen(which_screen);
	apollo_set_mask_fx(APOLLO_FX_NONE);
}

static void
__display_screen_saver(splash_screen_type screen_saver_default)
{
	int use_default = 1;
	
	// Check to see whether we can use the contents of the screen saver buffer or not.
	//
	if ( screen_saver_buffer && valid_screen_saver_buf )
	{
		screen_saver_t screen_saver = (screen_saver_t)valid_screen_saver_buf;
		
		switch ( screen_saver )
		{
			// Single-shot screen saver case.
			//
			case screen_saver_valid:
				memcpy_from_screen_saver_to_videomemory();
				use_default = 0;
			break;
			
			// Rotational screen saver case.
			//
			default:
				if ( uncompress_picture((u32)valid_screen_saver_buf, screen_saver_buffer) )
				{
					memcpy_from_picture_bufr_to_videomemory();
					get_next_screen_saver = 1;
					use_default = 0;
				}
			break;
		}
	}

	// Default back to the passed-in screen saver if we must.
	//
	if ( use_default )
	{
		switch ( screen_saver_default )
		{
			case splash_screen_sleep_fade:
				__display_screen_faded_with_overlay_mask(APOLLO_FX_STIPPLE_DARK_GRAY, splash_screen_sleep_fade);
			break;
			
			case splash_screen_sleep:
				__display_splash_screen(splash_screen_sleep);
			break;
			
			// Prevent compiler from complaining.
			//
			default:
			break;
		}
	}
	else
	{
		// Get the screen saver onto the display.
		//
		__update_display(EINK_FULL_UPDATE_ON);
		
		// Overlay it with the standard sleep screen's message.
		//
		__display_splash_screen(splash_screen_screen_saver_picture);
	}
}

static void
__update_display_partial(
	u8 *buffer, int buffer_size,
	int xstart, int xend,
	int ystart, int yend)
{
	if ( buffer && buffer_size && bounds_are_acceptable(xstart, (xend-xstart), ystart, (yend-ystart)) )
	{
		u32 area_cleared = apollo_load_partial_data(buffer, buffer_size, xstart, ystart, xend, yend);
		int screen_clear = get_screen_clear();
		
		if ( !(area_cleared && screen_clear) )
			apollo_display_partial();
			
		set_screen_clear(screen_clear && area_cleared);
		end_splash_screen_activity(splash_screen_up);
	}
}

static void load_buffer(u8* buffer, int xstart, int xend, int ystart, int yend)
{
	int x, y, rowbytes, bytes, xres = XRES;
	u8  *fb_base = (u8 *)videomemory;

	xstart	 = BPP_SIZE(xstart, bpp);
	xend	 = BPP_SIZE(xend,	bpp);
	rowbytes = BPP_SIZE(xres,	bpp);

	for ( bytes = 0, y = ystart; y < yend; y++ )
		for ( x = xstart; x < xend; x++ )
			buffer[bytes++] = fb_base[(rowbytes * y) + x];
}

static int get_partial_data(update_partial_t *update_partial_data)
{
	int xstart = update_partial_data->x1, xend = update_partial_data->x2,
		ystart = update_partial_data->y1, yend = update_partial_data->y2,
		
		xres = xend - xstart,
		yres = yend - ystart,
		
		buffer_size = BPP_SIZE((xres * yres), bpp);
		
	u8  *buffer = kmalloc(buffer_size, GFP_KERNEL);
	
	if ( buffer )
	{
		if ( update_partial_data->buffer )
		{
			if ( 0 != copy_from_user(buffer, update_partial_data->buffer, buffer_size) )
				buffer_size = 0;
		}
		else
		{
			if ( bounds_are_acceptable(xstart, (xend-xstart), ystart, (yend-ystart)) )
				load_buffer(buffer, xstart, xend, ystart, yend);
			else
				buffer_size = 0;
		}
	
		if ( buffer_size )
			update_partial_data->buffer = buffer;
		else
			kfree(buffer);
	}
	
	return ( buffer_size );
}

static int __set_partial_data(update_partial_t *update_partial_data, int use_local_buffer)
{
	int xstart		= update_partial_data->x1, xend = update_partial_data->x2,
		ystart		= update_partial_data->y1, yend = update_partial_data->y2,
		buffer_size = use_local_buffer ? BPP_SIZE(((xend - xstart) * (yend - ystart)), bpp)
									   : get_partial_data(update_partial_data),
		result		= 0;

	if ( buffer_size )
	{
		// Our convention is that the fx masks are located immediately following the
		// source picture, and are, otherwise, exactly the same size as the source
		// picture.  Thus, their xres is the same but their yres is double.
		//
		if ( APOLLO_FX_MASK == apollo_get_display_fx() )
		{
			yend = ystart + (yend - ystart)/2;
			buffer_size /= 2;
		}
		
		__update_display_partial(update_partial_data->buffer, buffer_size, xstart, xend, ystart, yend);
		
		if ( !use_local_buffer )
			kfree(update_partial_data->buffer);
	}
	else
		result = -ENOMEM;
	
	return ( result );
}

#define DONT_OVERRIDE_HW_ACCESS	0
#define IGNORE_HW_ACCESS()		ignore_hw_access(DONT_OVERRIDE_HW_ACCESS)

static int ignore_hw_access(int override_ignore_hw_access) {
    if (override_ignore_hw_access)
    	return(0);
    switch(g_eink_power_mode)  { 
        case FPOW_MODE_OFF:
        case FPOW_MODE_SLEEP:
            return(1);
        default:
            return(0);
    }
}

static char unknown_ioctl_cmd_str[32];

static char *eink_ioctl_to_str(int cmd)
{
	switch (cmd) {			
	    case FBIO_EINK_UPDATE_DISPLAY_SYNC :
	        return("UPDATE_DISPLAY_SYNC");
			
		case FBIO_EINK_SPLASH_SCREEN :
	        return("SPLASH_SCREEN");
			
		case FBIO_EINK_SPLASH_SCREEN_SLEEP :
	        return("SPLASH_SCREEN_SLEEP");
			
		case FBIO_EINK_UPDATE_PARTIAL :
	        return("UPDATE_PARTIAL");
						
        case FBIO_EINK_OFF_CLEAR_SCREEN:
	        return("OFF_CLEAR_SCREEN");
	        
	    case FBIO_EINK_CLEAR_SCREEN:
	    	return("CLEAR_SCREEN");
	    	
	   	case FBIO_EINK_RESTORE_SCREEN:
	   		return("RESTORE_SCREEN");
	   		
	   	case FBIO_EINK_FPOW_OVERRIDE:
	   		return("FPOW_OVERRIDE");
	   		
	   	case FBIO_EINK_UPDATE_DISPLAY_FX:
	   		return("UPDATE_DISPLAY_FX");

		case FBIO_EINK_SYNC_BUFFERS:
			return("SYNC_BUFFERS");
			
		case FBIO_EINK_SET_AUTO_POWER:
			return ("SET_AUTO_POWER");

		case FBIO_EINK_GET_AUTO_POWER:
			return ("GET_AUTO_POWER");

        case FBIO_EINK_FAKE_PNLCD:
            return ("FAKE_PNLCD");

		default:
	        sprintf(unknown_ioctl_cmd_str, "UNKNOWN (0x%08X)", cmd);
	        return(unknown_ioctl_cmd_str);
    }
}

static void __splash_screen_dispatch(splash_screen_type which_screen)
{
	// Simple (one-shot) splash screen cases.
	//
	if ( FBIO_SCREEN_IN_RANGE(which_screen) )
	{
		// Everything here is simple but the sleep screen.  We must either
		// use the sleep screen as is (i.e., default back to it on failure),
		// or put up the screen saver.
		//
		if ( splash_screen_sleep != which_screen )
			__display_splash_screen(which_screen);
		else
			__display_screen_saver(splash_screen_sleep);
	}
	else
	{
		// Composite splash screen cases.
		//
		switch ( which_screen )
		{
			// Drivemode cases.
			//
			case splash_screen_drivemode_0:
			case splash_screen_drivemode_1:
			case splash_screen_drivemode_2:
			case splash_screen_drivemode_3:
				__display_drivemode_screen(which_screen);
			break;

			// Faded-screen/blank-status-area/screen-saver cases.
			//
			case splash_screen_blank_status:
				__display_screen_faded(APOLLO_FX_NONE, BLANK_STATUS);
			break;
			
			case splash_screen_fade:
				__display_screen_faded(APOLLO_FX_STIPPLE_DARK_GRAY, BLANK_STATUS);
			break;
			
			case splash_screen_sleep_fade:
				__display_screen_saver(splash_screen_sleep_fade);
			break;
			
			case splash_screen_store_exit:
				__display_screen_faded_with_overlay_mask(APOLLO_FX_STIPPLE_LIGHTEN_LITE, splash_screen_store_exit);
			break;

			case splash_screen_exit_fade:
				__display_splash_screen(splash_screen_exit_fade);
			break;
			
			// Only reset if a reset isn't already underway (i.e., if we have a splash screen already
			// up, we will, in effect, be resetting).
			//
			case splash_screen_framework_reset:
				if ( splash_screen_invalid == splash_screen_up )
				{
					__display_splash_screen(splash_screen_framework_reset);
					end_splash_screen_activity(splash_screen_framework_reset);
				}
			break;

			// Huh?
			//
			default:
			break;
		}
	}
}

#define IS_FX_CMD(c)				((FBIO_EINK_UPDATE_DISPLAY_FX == c)		||	\
									 (FBIO_EINK_UPDATE_PARTIAL == c))

#define IS_UPDATE_DISPLAY_CMD(c)	((FBIO_EINK_UPDATE_DISPLAY_SYNC == c)	||	\
									 (FBIO_EINK_SYNC_BUFFERS == c)			||	\
									 IS_FX_CMD(c))

static int
__eink_fb_ioctl(
	struct inode *inode,
	struct file *file,
	u_int cmd,
	u_long arg,
	struct fb_info *info)
{
    int rc = 0, fpow_flag = EINK_IOCTL_FPOW_FLAG == (unsigned long)info;

    // If we're locked out, ignore the call and return "OK".
    if (ignore_hw_access(fpow_flag)) {
        powdebug("__eink_fb_ioctl(0x%08X:%d) HW off - ignoring call\n",cmd,fiona_sys_getpid());
    } else {
    	// Return AUTO_POWER state.
   		if (FBIO_EINK_GET_AUTO_POWER == cmd) {
   			if (arg) {
   				int power_state = (APOLLO_NORMAL_POWER_MODE == apollo_get_power_state()) ? EINK_AUTO_POWER_OFF
   																						 : EINK_AUTO_POWER_ON;
   				if (EINK_IOCTL_FLAG == (unsigned long)inode) {
   					memcpy((int *)arg, &power_state, sizeof(int));
   				} else {
   					copy_to_user((int *)arg, &power_state, sizeof(int));
   				}
   			} else {
   				rc = -EINVAL;
   			}
		} else {
			int buffers_equal = 0;
	
			if (IS_UPDATE_DISPLAY_CMD(cmd)) {
				// Don't override the lock-out here.
				if (!ignore_hw_access(DONT_OVERRIDE_HW_ACCESS)) {
					// Even if the buffers are equal, don't say they are equal if we're doing an
					// FX, clearing one out, or need to acknowledge a full-refresh update.  Also,
					// if the buffers aren't equal, say they are equal if we're just sync'ing them.
					buffers_equal  = ((memcpy_from_frame_buffer_to_videomemory()								&&
									  !(IS_FX_CMD(cmd) || clear_update_fx || !force_full_update_acknowledge))	||
									  (FBIO_EINK_SYNC_BUFFERS == cmd));
					// But always note that the buffers are now sync'd at this point.
					buffers_synced = 1;
				}
			}
	
			// Don't bother waking Apollo up if the buffers are equal (they'll only be equal
			// on UPDATE_DISPLAY-related commands).
			if (!buffers_equal) {
				int saved_apollo_power_state;
				
				rc = begin_apollo_wake_on_activity(&saved_apollo_power_state);
				if (0 == rc) {
					update_partial_t update_partial_data = { 0 };
					char segments[PN_LCD_SEGMENT_COUNT];
					int buffer_local_state = 0;
					fx_t fx;
					
					einkdebug("__eink_fb_ioctl(%s, arg=0x%08lX, pid=%d)\n",eink_ioctl_to_str(cmd),arg,fiona_sys_getpid());
					switch (cmd) {			
						case FBIO_EINK_UPDATE_DISPLAY_FX:
							if (EINK_IOCTL_FLAG == (unsigned long)inode) {
								memcpy(&fx, (void *)arg, sizeof(fx));
							} else if (0 != copy_from_user(&fx, (void *)arg, sizeof(fx_t))) {
								rc = -EFAULT;
							}
							
							if (0 == rc) {
								if (fx.exclude_rect && bounds_are_acceptable(fx.x1, (fx.x2-fx.x1), fx.y1, (fx.y2-fx.y1))) {
									apollo_set_fx_exclude_rect(fx.x1, fx.y1, fx.x2, fx.y2);
								}
								if (fx_mask != fx.which_fx) {
									apollo_set_display_fx(fx.which_fx);
									last_display_fx = fx.which_fx;
								}
								arg = fx.update_mode ? EINK_FULL_UPDATE_ON : EINK_FULL_UPDATE_OFF;
								clear_update_fx = 1;
								goto update_common;
							}
							break; 
							
						case FBIO_EINK_UPDATE_DISPLAY_SYNC:
							if ((EINK_FULL_UPDATE_ON == arg) || clear_update_fx || !USE_SLAB_ARRAY()) {
								goto update_display;
							} else {
								if (num_slabs != changed_slab_index_start) {
									u8  *fb_base = (u8 *)videomemory;
									
									update_partial_data.x1 = slab_array[changed_slab_index_start].x1;
									update_partial_data.y1 = slab_array[changed_slab_index_start].y1;
									update_partial_data.x2 = slab_array[changed_slab_index_end].x2;
									update_partial_data.y2 = slab_array[changed_slab_index_end].y2;
									update_partial_data.which_fx = APOLLO_FX_NONE;
									update_partial_data.buffer = &fb_base[BPP_SIZE(XRES, bpp) * update_partial_data.y1];
									   
									buffer_local_state = 1;
									clear_update_fx = 0;
									buffers_synced = 0;
									
									einkdebug("__eink_fb_ioctl(full-to-partial update: x1 = %d, y1 = %d, x2 = %d, y2 = %d)\n",
										update_partial_data.x1, update_partial_data.y1,
										update_partial_data.x2, update_partial_data.y2);
	
									goto partial_update_common;
								}
							}
	
						update_display:
						    last_display_fx = APOLLO_FX_NONE;
							clear_update_fx = 0;
						update_common:
							__update_display(arg);
							__update_fake_pnlcd(segments_cache);
							
							apollo_set_display_fx(APOLLO_FX_NONE);
							apollo_clear_fx_exclude_rect();
							buffers_synced = 0;
							break;
									
						case FBIO_EINK_UPDATE_PARTIAL:
							last_display_fx = APOLLO_FX_NONE;
							clear_update_fx = 1;
							if (EINK_IOCTL_FLAG == (unsigned long)inode) {
								memcpy(&update_partial_data, (void *)arg, sizeof(update_partial_t));
							} else if (0 != copy_from_user(&update_partial_data, (void *)arg, sizeof(update_partial_t))) {
								rc = -EFAULT;
							}
						partial_update_common:
							if (0 == rc) {
								apollo_set_display_fx(update_partial_data.which_fx);
								rc = __set_partial_data(&update_partial_data, buffer_local_state);
								if (0 == rc) {
									__update_fake_pnlcd(segments_cache);
								}

								apollo_set_display_fx(APOLLO_FX_NONE);
							}
							break;
								
						case FBIO_EINK_FAKE_PNLCD:
                            if (arg) {
                                if (EINK_IOCTL_FLAG == (unsigned long)inode) {
                                    memcpy(segments, (void *)arg, PN_LCD_SEGMENT_COUNT);
                                } else if (0 != copy_from_user(segments, (void *)arg, PN_LCD_SEGMENT_COUNT)) {
                                    rc = -EFAULT;
                                }
                            } else {
                                rc = pnlcd_sys_ioctl(PNLCD_GET_SEG_STATE, segments);
                            }
                            if (0 == rc) {
                                __update_fake_pnlcd(segments);
                            }
						    break;
						
						case FBIO_EINK_SPLASH_SCREEN:
						case FBIO_EINK_SPLASH_SCREEN_SLEEP:
							extpowdebug("__eink_fb_ioctl(FBIO_EINK_SPLASH_SCREEN, screen=%d)\n",(int)arg);
							__splash_screen_dispatch((splash_screen_type)arg);
							
							if (FBIO_EINK_SPLASH_SCREEN_SLEEP == cmd) {
								goto fpow_sleep_common;
							}
							break;
						
						case FBIO_EINK_CLEAR_SCREEN:
							__eink_clear_screen(EINK_FULL_UPDATE_ON);
							break;
							
						case FBIO_EINK_RESTORE_SCREEN:
							apollo_restore();
							/* If possible, should copy restored screen contents back to framebuffer here.*/
							break;
		
						case FBIO_EINK_SET_AUTO_POWER:
							if (EINK_AUTO_POWER_OFF == arg) {
								saved_apollo_power_state = APOLLO_NORMAL_POWER_MODE;
							} else {
								saved_apollo_power_state = APOLLO_SLEEP_POWER_MODE;
							}
							break;
		
						case FBIO_EINK_OFF_CLEAR_SCREEN:
							fpow_clear_eink_disable_pending();
							extpowdebug("__eink_fb_ioctl(OFF_CLEAR) clearing screen...\n");
							apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_SLOW);
							__eink_fb_power_off_clear_screen();
							/* fall thru to fpow_common */
							
						fpow_sleep_common:
							// We need to disable via FPOW, because we don't want anything
							// else BUT FPOW to be able to wake us up...
							//
							// NOTE:  We're not obtaining the FPOW lock here.  The case we
							// care about is being called from FPOW, so lock is already held...
							// MUST assure that this call is not used from other places....
							// SEARCH AND FIXME -njv
							extpowdebug("__eink_fb_ioctl(SLEEP) disabling via __do_eink_fpow_mode_sleep()\n");
							rc = __do_eink_fpow_mode_sleep();
							if (FPOW_NO_ERR != rc) {
								powdebug("__eink_fb_ioctl(SLEEP) mode off call returned error %d\n",rc);
							} else {
								rc = 0;
							}
							break;
		
						default:
							rc = -EINVAL;
							break;
					}
					end_apollo_wake_on_activity(saved_apollo_power_state);
				}
				else {
					powdebug("__eink_fb_ioctl(0x%08X:%d) - apollo_wake error, no update\n", cmd,fiona_sys_getpid());
				}
			}
			else {
				einkdebug("__eink_fb_ioctl(UPDATE_DISPLAY) - no update because buffers equal\n");
			}
		}
	}
	return rc;
}

/*******************************************************************
 * Routine:     eink_fb_ioctl
 *
 * Purpose:     EINK video driver's IOCTL routine.
 *              This routine is responsible for grabbing the apollo_lock
 *              while it's operating to assure no HW access contention.
 *
 * Parameters:  inode - ignored (or flag)
 *              file  - ignored (or flag)
 *              cmd   - IOCTL command, one of the following:
 *                      FBIO_EINK_UPDATE_DISPLAY_ASYNC
 *                      FBIO_EINK_UPDATE_DISPLAY_SYNC
 *                      FBIO_EINK_SPLASH_SCREEN
 *                      FBIO_EINK_UPDATE_PARTIAL
 *                      FBIO_EINK_OFF_CLEAR_SCREEN
 *                      FBIO_EINK_SPLASH_SCREEN_SLEEP
 *                      FBIO_EINK_CLEAR_SCREEN
 *                      FBIO_EINK_RESTORE_SCREEN
 *                      FBIO_EINK_FPOW_OVERRIDE
 *                      FBIO_EINK_UPDATE_DISPLAY_FX
 *                      FBIO_EINK_SYNC_BUFFERS
 *                      FBIO_EINK_SET_AUTO_POWER
 *                      FBIO_EINK_GET_AUTO_POWER
 *                      FBIO_EINK_UPDATE_NOW
 *                      FBIO_EINK_FAKE_PNLCD
 *              arg   - argument for particular IOCTL cmd
 *              info  - pointer to frame buffer information (or flag)
 *
 * Returns:     0            - Success
 *              -EINVAL      - Invalid cmd
 *              -ERESTARTSYS - couldn't get apollo_lock
 *              -EFAULT      - couldn't copy data from user space
 *              -ENOMEM      - out of (kernel) memory for operation
 *               
 * Assumptions: none
 *******************************************************************/
static int
eink_fb_ioctl(
	struct inode *inode,
	struct file *file,
	u_int cmd,
	u_long arg,
	struct fb_info *info)
{
   	int rc = 0, no_lock = 0;
	
	// Don't waste any time with the fake PNLCD if it's not in use.
	if ((FBIO_EINK_FAKE_PNLCD == cmd) && !FAKE_PNLCD_IN_USE()) {
	    goto exit;
	}

    no_lock = EINK_IOCTL_LOCK_FLAG == (unsigned long)file;
    rc = -EINVAL;

	// Convert UPDATE_NOW calls into UPDATE_DISPLAY(EINK_FULL_UPDATE_ON).
	if (FBIO_EINK_UPDATE_NOW == cmd) {
		einkdebug("eink_fb_ioctl(UPDATE_NOW -> UPDATE_DISPLAY)\n");
		cmd = FBIO_EINK_UPDATE_DISPLAY_ASYNC;
		arg = EINK_FULL_UPDATE_ON;
	}

	// Normalize UPDATE_DISPLAY's arg and perform misc. transforms as required...
	if ((FBIO_EINK_UPDATE_DISPLAY_ASYNC == cmd) || (FBIO_EINK_UPDATE_DISPLAY_SYNC == cmd)) {
		int prefer_full_update = !(force_full_update_acknowledge || auto_refresh_timer_acknowledge);
		
		if (prefer_full_update && !arg) {
			einkdebug("eink_fb_ioctl(UPDATE_DISPLAY: partial -> full refresh)\n");
			arg = EINK_FULL_UPDATE_ON;
		}

		if (arg && !(prefer_full_update || EINK_IOCTL_FLAG == (unsigned long)inode)) {
			arg = auto_refresh_timer_update_display_which;
			
			if (!arg) {
				einkdebug("eink_fb_ioctl(UPDATE_DISPLAY: full -> partial refresh, count = %d)\n", auto_refresh_timer_full_update_skip_count);
				auto_refresh_timer_full_update_skip_count++;
				
				if (AUTO_SYNC_TO_ASYNC_COUNT < auto_refresh_timer_full_update_skip_count) {
					if (auto_sync_to_async && (FBIO_EINK_UPDATE_DISPLAY_SYNC == cmd)) {
						einkdebug("eink_fb_ioctl(UPDATE_DISPLAY: sync -> async)\n");
						cmd = FBIO_EINK_UPDATE_DISPLAY_ASYNC;
					}
				}
			}
		}
	}

	// Async...
    if (update_display_async && (FBIO_EINK_UPDATE_DISPLAY_ASYNC == cmd)) {
		wake_update_display_thread();
		
		if (!no_lock) {
			if (LOCK_APOLLO_ENTRY()) {
				rc = -ERESTARTSYS;
				goto exit;
			}
		}
		
		einkdebug("eink_fb_ioctl(UPDATE_DISPLAY_ASYNC, arg=0x%08lX, count=%d, pid=%d)\n", arg, update_display_count, fiona_sys_getpid());

		if (update_display_which && update_display_count) {
				update_display_ready = 0;
		}
		
		update_display_which |= arg;
		update_display_pending = 1;
		update_display_count++;
		
		rc = 0;
		
		if (!no_lock) {
			LOCK_APOLLO_EXIT();
		}

		goto exit;
		
    } else {
    	if (FBIO_EINK_UPDATE_DISPLAY_ASYNC == cmd) {
    		einkdebug("eink_fb_ioctl(UPDATE_DISPLAY: async -> sync)\n");
    		cmd = FBIO_EINK_UPDATE_DISPLAY_SYNC;
    	}
    }

	// Sync...
	if (!no_lock) {
		if (LOCK_APOLLO_ENTRY()) {
			rc = -ERESTARTSYS;
			goto exit;
		}
	}
	
	if (FBIO_EINK_FPOW_OVERRIDE == cmd) {
		fpow_override_t fpow_override;
	    rc = 0;
	    
		if (EINK_IOCTL_FLAG == (unsigned long)inode) {
			memcpy(&fpow_override, (void *)arg, sizeof(fpow_override_t));
		} else if (0 != copy_from_user(&fpow_override, (void *)arg, sizeof(fpow_override_t))) {
			rc = -EFAULT;
		}

		if (rc == 0) {
			rc = __eink_fb_ioctl(inode, file, fpow_override.cmd, fpow_override.arg, (struct fb_info *)EINK_IOCTL_FPOW_FLAG);
		}
		
	} else {
		rc = __eink_fb_ioctl(inode, file, cmd, arg, info);
	}

	if (!no_lock) {
		LOCK_APOLLO_EXIT();
	}

exit:
	return rc;
}

static int
_eink_sys_ioctl(
	unsigned int cmd,
	unsigned long arg)
{
	return eink_fb_ioctl((struct inode*)EINK_IOCTL_FLAG, NULL, cmd, arg, NULL);
}

static int
eink_fb_blank(
    int blank,
    struct fb_info *info)
{
    int rc = 0, cmd = 0, arg = 0;
    
    switch (blank) {
        case FB_BLANK_UNBLANK:
            _eink_sys_ioctl(FBIO_EINK_SET_AUTO_POWER, EINK_AUTO_POWER_ON);
            break;
            
        case FB_BLANK_NORMAL:
        case FB_BLANK_VSYNC_SUSPEND:
        case FB_BLANK_HSYNC_SUSPEND:
            _eink_sys_ioctl(FBIO_EINK_SET_AUTO_POWER, EINK_AUTO_POWER_OFF);
            cmd = FBIO_EINK_CLEAR_SCREEN;
            break;
            
        case FB_BLANK_POWERDOWN:
            cmd = FBIO_EINK_OFF_CLEAR_SCREEN;
            break;
    }
    
    if (cmd) {
        rc = _eink_sys_ioctl(cmd, arg);
    }

    return rc;
}

static int
add_vma(
	struct vm_area_struct *vma)
{
	struct fb_vma_list_entry *list_entry;

	list_entry = kmalloc(sizeof(struct fb_vma_list_entry), GFP_KERNEL);
	list_entry->vma = vma;

	down_interruptible(&vma_list_semaphore);

	list_add_tail((struct list_head *)list_entry, &vma_list);

	up(&vma_list_semaphore);

	return 0;
}

static struct fb_vma_list_entry *
find_fb_vma_entry(
	struct vm_area_struct *vma)
{
	struct list_head *ptr;
	struct fb_vma_list_entry *entry;
  
	for (ptr = vma_list.next; ptr != &vma_list; ptr = ptr->next) {
		entry = list_entry(ptr, struct fb_vma_list_entry, list);
		if (entry->vma == vma) {
			return entry;
		}
	}

	return NULL;
}

static int
remove_vma(
	struct vm_area_struct *vma)
{
	struct fb_vma_list_entry *entry;

	down_interruptible(&vma_list_semaphore);

	entry = find_fb_vma_entry(vma);
	if (entry != NULL) {
		list_del((struct list_head *) entry);
		kfree(entry);
	}

	up(&vma_list_semaphore);

	return 0;
}

static void
eink_fb_vma_open(
	struct vm_area_struct *vma)
{
	add_vma(vma);
}

static void
eink_fb_vma_close(
	struct vm_area_struct *vma)
{
	remove_vma(vma);
}

static struct page *
eink_fb_vma_nopage(
	struct vm_area_struct *vma,
	unsigned long address,
	int *type)
{
	unsigned long offset;
	struct page *page = NOPAGE_SIGBUS;

	offset = address - vma->vm_start + (vma->vm_pgoff << PAGE_SHIFT);

	if (offset > videomemorysize) {
		return page;
	}

	page = videopages[offset >> PAGE_SHIFT];
	get_page(page);

	return page;
}

static struct vm_operations_struct eink_fb_vma_ops = {
	open: eink_fb_vma_open,
	close: eink_fb_vma_close,
	nopage: eink_fb_vma_nopage,
};

static int
eink_fb_mmap(
	struct fb_info *info,
	struct file *file,
	struct vm_area_struct *vma)
{
	vma->vm_ops = &eink_fb_vma_ops;
	vma->vm_flags |= VM_RESERVED;
	eink_fb_vma_open(vma);

	return 0;
}

static void
eink_fb_fillrect(
	struct fb_info *p,
	const struct fb_fillrect *region)
{
}

static void
eink_fb_copyarea(
	struct fb_info *p,
	const struct fb_copyarea *area) 
{
}

static void
eink_fb_imageblit(
	struct fb_info *p,
	const struct fb_image *image) 
{
}

struct fb_ops eink_fb_ops = {
	.owner = THIS_MODULE,
	.fb_blank = eink_fb_blank,
	.fb_fillrect = eink_fb_fillrect,
	.fb_copyarea = eink_fb_copyarea,
	.fb_imageblit = eink_fb_imageblit,
	.fb_cursor = soft_cursor,
	.fb_ioctl = eink_fb_ioctl,
	.fb_mmap = eink_fb_mmap,
};

static int
update_display_thread(
	void *unused)
{
	int thread_active = 1;
	
	daemonize(EINK_FB"_udt");
	allow_signal(SIGKILL);
	
	while ( thread_active )
	{
#ifdef CONFIG_PM
		if ( current->flags & PF_FREEZE )
			refrigerator(PF_FREEZE);
#endif

		if ( !signal_pending(current) )
		{
			unsigned long timeout_time = update_display_timeout;
			
			if ( 0 == LOCK_APOLLO_ENTRY() )
			{
				if ( update_display_ready )
				{
					__eink_sys_ioctl(FBIO_EINK_UPDATE_DISPLAY_SYNC, update_display_which);
					update_display_ready = 0;
				}

				if ( update_display_pending )
				{
					if ( update_display_which )
						update_display_timeout = UPDATE_DISPLAY_TIMEOUT_MAX;
					else
						update_display_timeout = UPDATE_DISPLAY_TIMEOUT_MIN;
					
					update_display_pending = 0;
					update_display_ready = 1;
				}
				
				if ( !update_display_pending && !update_display_ready )
				{
					update_display_timeout = UPDATE_DISPLAY_TIMEOUT;
					update_display_which = EINK_FULL_UPDATE_OFF;
					update_display_count = 0;
					
					timeout_time = MAX_SCHEDULE_TIMEOUT;
				}
			
				LOCK_APOLLO_EXIT();
			}

			interruptible_sleep_on_timeout(&update_display_wq, timeout_time);
		}
		else
			thread_active = 0;
	}
	
	complete_and_exit(&update_display_thread_exited, 0);
}

static void
start_update_display_thread(
	void)
{
	init_waitqueue_head(&update_display_wq);
	
	if ( 0 > (update_display_pid = kernel_thread(update_display_thread, NULL, CLONE_KERNEL)) )
		update_display_pid = 0;
}

static void
stop_update_display_thread(
	void)
{
	if ( 0 < update_display_pid )
		if ( 0 == kill_proc(update_display_pid, SIGKILL, 1) )
			wait_for_completion(&update_display_thread_exited);
}

static void
wake_update_display_thread(
	void)
{
	if ( 0 == LOCK_APOLLO_ENTRY() )
	{
		if ( !update_display_pending && !update_display_ready )
			wake_up(&update_display_wq);
			
		LOCK_APOLLO_EXIT();
	}
}

static void
__sleep_update_display_thread(
	void)
{
	update_display_pending = update_display_ready = 0;
}

static void
save_the_last_screen_saver(
	void)
{
	mm_segment_t saved_fs = get_fs();
	int screen_saver_last_file;

	set_fs(get_ds());
	
	screen_saver_last_file = sys_open(screen_saver_path_last, O_WRONLY, 0);
	
	if ( 0 <= screen_saver_last_file )
	{
        char buf[128];
        int len = sprintf(buf, "%d", screen_saver_last);
		
		if ( 0 < len )
			sys_write(screen_saver_last_file, buf, len);
		
		sys_close(screen_saver_last_file);
	}

	set_fs(saved_fs);
}

static int
load_the_screen_saver(
	char *screen_saver_path)
{
	int screen_saver_file, result = 0;
	mm_segment_t saved_fs = get_fs();

	set_fs(get_ds());
	
	screen_saver_file = sys_open(screen_saver_path, O_RDONLY, 0);
	
	if ( 0 <= screen_saver_file )
	{
		int len = sys_read(screen_saver_file, screen_saver_buffer, videomemorysize);
		
		if ( 0 <= len )
		{
			valid_screen_saver_buf = (videomemorysize == len) ? screen_saver_valid : len;
			result = 1;
		}

		sys_close(screen_saver_file);
	}
	
	set_fs(saved_fs);
	
	return ( result );
}

#define EXISTS_SCREEN_SAVER_PATH_CREATE(p)	exists_screen_saver_path(p, 1)
#define EXISTS_SCREEN_SAVER_PATH(p)			exists_screen_saver_path(p, 0)

static int
exists_screen_saver_path(
	char *screen_saver_path,
	int create)
{
	int screen_saver_file, flags = O_RDONLY | (create ? O_CREAT : 0),
		result = 0;
	mm_segment_t saved_fs = get_fs();

	set_fs(get_ds());
	
	screen_saver_file = sys_open(screen_saver_path, flags, 0);
	
	if ( 0 <= screen_saver_file )
	{
		sys_close(screen_saver_file);
		result = 1;
	}
	
	set_fs(saved_fs);
	
	return ( result );
}

static void
set_screen_saver_user_path(
	void)
{
	strcpy(screen_saver_path_template, SCREEN_SAVER_PATH_USER);
	strcat(screen_saver_path_template, SCREEN_SAVER_PATH_TEMPLATE);
	
	strcpy(screen_saver_path_last, SCREEN_SAVER_PATH_USER);
	strcat(screen_saver_path_last, SCREEN_SAVER_PATH_LAST);
	
	user_screen_saver_set = 1;
}

static void
set_screen_saver_sys_path(
	void)
{
	strcpy(screen_saver_path_template, SCREEN_SAVER_PATH_SYS);
	strcat(screen_saver_path_template, SCREEN_SAVER_PATH_TEMPLATE);
	
	strcpy(screen_saver_path_last, SCREEN_SAVER_PATH_SYS);
	strcat(screen_saver_path_last, SCREEN_SAVER_PATH_LAST);
	
	user_screen_saver_set = 0;
}

static void
set_screen_saver_last(
	void)
{
	
	if ( present_screen_savers_in_order )
		screen_saver_last++;
	else
	{
		char screen_saver_path[SCREEN_SAVER_PATH_SIZE];
		unsigned int random_number = 0;
		int num_screen_savers = 0;
		
		// Determine how many screen savers currently exist.
		do
		{
			sprintf(screen_saver_path, screen_saver_path_template, num_screen_savers++);
		}
		while ( EXISTS_SCREEN_SAVER_PATH(screen_saver_path) );
		
		if ( num_screen_savers )
			num_screen_savers--;

		// Pick a screen saver that's between 0 and num_screen_savers-1, but not one that
		// we've used the last couple of times.
		do
		{
			get_random_bytes(&random_number, sizeof(unsigned int));
			random_number %= num_screen_savers;
		}
		while ( (screen_saver_random1 == random_number) || (screen_saver_random2 == random_number) );
		
		// Keep track of the last couple of screen savers we've used.
		//
		screen_saver_random2 = screen_saver_last;
		screen_saver_random1 = random_number;
		
		screen_saver_last = random_number;
	}
}

static void
get_the_next_screen_saver(
	void)
{
	char screen_saver_path[SCREEN_SAVER_PATH_SIZE];
	
	// Generate the path for the next screen saver we're supposed to load, trying the user
	// path first and then falling back to the system path.
	//
	set_screen_saver_user_path();
	
	if ( !EXISTS_SCREEN_SAVER_PATH(screen_saver_path_last) )
		set_screen_saver_sys_path();
	
	set_screen_saver_last();
	
	sprintf(screen_saver_path, screen_saver_path_template, screen_saver_last);
	
	// Load the next screen saver in sequence if we can.  Otherwise, load the
	// default screen saver.
	//
	if ( !load_the_screen_saver(screen_saver_path) )
	{
		screen_saver_last = SCREEN_SAVER_DEFAULT;
		
		sprintf(screen_saver_path, screen_saver_path_template, screen_saver_last);
		load_the_screen_saver(screen_saver_path);
	}
	
	// Remember where we are in the sequence.
	//
	save_the_last_screen_saver();
}

static int
load_the_user_boot_screen(
	void)
{
	int result = 0;
	
	// If the user boot screen is already loaded, we're done.
	//
	if ( user_screen_saver_set && (SCREEN_SAVER_DEFAULT == screen_saver_last) )
		result = 1;
	else
	{
		char screen_saver_user_template[SCREEN_SAVER_PATH_SIZE],
			 screen_saver_user_path[SCREEN_SAVER_PATH_SIZE];
	
		// Generate the path for the default user screen saver that, if it exists and
		// is the right size, we'll also use it as the boot screen.
		//
		strcpy(screen_saver_user_template, SCREEN_SAVER_PATH_USER);
		strcat(screen_saver_user_template, SCREEN_SAVER_PATH_TEMPLATE);
		
		sprintf(screen_saver_user_path, screen_saver_user_template, SCREEN_SAVER_DEFAULT);
		
		if ( EXISTS_SCREEN_SAVER_PATH(screen_saver_user_path) )
			if ( load_the_screen_saver(screen_saver_user_path) )
				result = 1;
	}
	
	return ( result );
}

static void
load_user_boot_screen(
	void)
{
	int user_boot_screen = 0;
	
	// Use the default user screen saver screen as the boot screen if it'll fit.
	//
	if ( !videomemory_local && load_the_user_boot_screen() )
	{
		if ( (FRAMEBUFFER_SIZE - videomemorysize) >= valid_screen_saver_buf )
		{
			memcpy((videomemory + videomemorysize), screen_saver_buffer, valid_screen_saver_buf);
			user_boot_screen = valid_screen_saver_buf;
		}
	}

	set_user_boot_screen(user_boot_screen);
}

static void
save_user_screen_saver(
	char *screen_saver_user_path)
{
	mm_segment_t saved_fs = get_fs();
	int screen_saver_file;

	set_fs(get_ds());
	
	screen_saver_file = sys_open(screen_saver_user_path, (O_WRONLY | O_CREAT), 0);
	
	if ( 0 <= screen_saver_file )
	{
		int len = compress_picture(videomemorysize, screen_saver_buffer);
	
		if ( 0 < len )
		{
			sys_write(screen_saver_file, picture_buffer, len);
			
			memcpy_from_picture_bufr_to_screensaver();
			valid_screen_saver_buf = len;
		}
		
		sys_close(screen_saver_file);
	}
	
	set_fs(saved_fs);
}

static void
set_next_user_screen_saver(
	void)
{
	char screen_saver_user_path[SCREEN_SAVER_PATH_SIZE];
	
	// Build up the path for the user screen saver last-shown
	// file to see whether it exits or not.  
	//
	strcpy(screen_saver_user_path, SCREEN_SAVER_PATH_USER);
	strcat(screen_saver_user_path, SCREEN_SAVER_PATH_LAST);
	
	if ( EXISTS_SCREEN_SAVER_PATH_CREATE(screen_saver_user_path) )
	{
		char screen_saver_user_template[SCREEN_SAVER_PATH_SIZE];
		int screen_saver_user_next = 0;
		
		// Cycle through the user screen saver directory looking
		// for the next available screen saver file.
		//
		strcpy(screen_saver_user_template, SCREEN_SAVER_PATH_USER);
		strcat(screen_saver_user_template, SCREEN_SAVER_PATH_TEMPLATE);
		
		do
		{
			sprintf(screen_saver_user_path, screen_saver_user_template, screen_saver_user_next++);
		}
		while ( EXISTS_SCREEN_SAVER_PATH(screen_saver_user_path) );
		
		// Save out the current screen saver as the next one in the list, and
		// update screen saver's sequence.
		//
		save_user_screen_saver(screen_saver_user_path);
		
		screen_saver_last = screen_saver_user_next;
		save_the_last_screen_saver();
	}
}

static int
screen_saver_thread(
	void *unused)
{
	int thread_active = 1;
	
	daemonize(EINK_FB"_sst");
	allow_signal(SIGKILL);
	
	while ( thread_active )
	{
#ifdef CONFIG_PM
		if ( current->flags & PF_FREEZE )
			refrigerator(PF_FREEZE);
#endif

		if ( !signal_pending(current) )
		{
			if ( get_next_screen_saver )
			{
				get_the_next_screen_saver();
				get_next_screen_saver = 0;
			}

			interruptible_sleep_on_timeout(&screen_saver_wq, MAX_SCHEDULE_TIMEOUT);
		}
		else
			thread_active = 0;
	}
	
	complete_and_exit(&screen_saver_thread_exited, 0);
}

static void
start_screen_saver_thread(
	void)
{
	if ( screen_saver_buffer )
	{
		init_waitqueue_head(&screen_saver_wq);
		
		if ( 0 > (screen_saver_pid = kernel_thread(screen_saver_thread, NULL, CLONE_KERNEL)) )
			screen_saver_pid = 0;
	}
}

static void
stop_screen_saver_thread(
	void)
{
	if ( screen_saver_buffer )
		if ( 0 < screen_saver_pid )
			if ( 0 == kill_proc(screen_saver_pid, SIGKILL, 1) )
				wait_for_completion(&screen_saver_thread_exited);
}

static void
wake_screen_saver_thread(
	void)
{
	if ( screen_saver_buffer )
		wake_up(&screen_saver_wq);
}

static void
timer_function(
	unsigned long selector)
{
	switch ( selector )
	{
		case AUTO_POWER_TIMER:
			wake_up(&auto_power_wq);
		/* fall thru */
		
		case AUTO_REFRESH_TIMER:
			auto_refresh_timer_update_display_which = EINK_FULL_UPDATE_ON;
			auto_refresh_timer_full_update_skip_count = 0;
			auto_refresh_timer_acknowledge = 0;
		break;
	}
}

static void
start_auto_refresh_timer(
	void)
{
	init_timer(&auto_refresh_timer);
	
	auto_refresh_timer.function = timer_function;
	auto_refresh_timer.data = AUTO_REFRESH_TIMER;
	
	auto_refresh_timer_active = 1;
}

static void
stop_auto_refresh_timer(
	void)
{
	if ( auto_refresh_timer_active )
	{
		del_timer_sync(&auto_refresh_timer);
		auto_refresh_timer_active = 0;
	}
}

static void
prime_auto_refresh_timer(
	int delay_timer)
{
	if ( auto_refresh_timer_active && auto_refresh_timer_acknowledge )
	{
		unsigned long timer_delay = MIN_SCHEDULE_TIMEOUT;

		if ( delay_timer )
		{
			if ( AUTO_ADJUSTMENT_FACTOR(max_full_update_count, MAX_FULL_UPDATE_COUNT) > auto_refresh_timer_full_update_skip_count )
			{
				timer_delay = AUTO_ADJUSTMENT_FACTOR(auto_refresh_timer_delay, AUTO_REFRESH_TIMER_DELAY);
				auto_refresh_timer_update_display_which = EINK_FULL_UPDATE_OFF;
			}
		}

		mod_timer(&auto_refresh_timer, TIMER_EXPIRES(timer_delay));
	}
}

static int
auto_power_thread(
	void *unused)
{
	int thread_active = 1;
	
	daemonize(EINK_FB"_apt");
	allow_signal(SIGKILL);
	
	while ( thread_active )
	{
#ifdef CONFIG_PM
		if ( current->flags & PF_FREEZE )
			refrigerator(PF_FREEZE);
#endif

		if ( !signal_pending(current) )
		{
			if ( 0 == LOCK_APOLLO_ENTRY() )
			{
				if ( !IGNORE_HW_ACCESS() )
				{
					unsigned char off_delay;
					
					apollo_read_register(APOLLO_OFF_DELAY_REGISTER, &off_delay);
	
					if ( (APOLLO_OFF_DELAY_THRESH > off_delay) && (APOLLO_NORMAL_POWER_MODE == apollo_get_power_state()) )
						__eink_sys_ioctl(FBIO_EINK_SET_AUTO_POWER, EINK_AUTO_POWER_ON);
				}

				LOCK_APOLLO_EXIT();
			}
		}
		else
			thread_active = 0;
		
		interruptible_sleep_on_timeout(&auto_power_wq, MAX_SCHEDULE_TIMEOUT);
	}
	
	complete_and_exit(&auto_power_thread_exited, 0);
}

static void
start_auto_power_thread(
	void)
{
	init_waitqueue_head(&auto_power_wq);
	
	if ( 0 > (auto_power_pid = kernel_thread(auto_power_thread, NULL, CLONE_KERNEL)) )
		auto_power_pid = 0;
}

static void
stop_auto_power_thread(
	void)
{
	if ( 0 < auto_power_pid )
		if ( 0 == kill_proc(auto_power_pid, SIGKILL, 1) )
			wait_for_completion(&auto_power_thread_exited);
}

static void
start_auto_power_timer(
	void)
{
	init_timer(&auto_power_timer);
	
	auto_power_timer.function = timer_function;
	auto_power_timer.data = AUTO_POWER_TIMER;
	
	start_auto_power_thread();
	auto_power_timer_active	= 1;
}

static void
stop_auto_power_timer(
	void)
{
	if ( auto_power_timer_active )
	{
		stop_auto_power_thread();
		del_timer_sync(&auto_power_timer);
		
		auto_power_timer_active = 0;
	}
}

static void
prime_auto_power_timer(
	int delay_timer)
{
	if ( auto_power_timer_active )
	{
		unsigned long timer_delay = delay_timer ? AUTO_ADJUSTMENT_FACTOR(auto_power_timer_delay, AUTO_POWER_TIMER_DELAY)
												: MIN_SCHEDULE_TIMEOUT;
		
		mod_timer(&auto_power_timer, TIMER_EXPIRES(timer_delay));
	}
}

int 
__init eink_fb_setup(
	char *options)
{
	char *this_opt;

	eink_fb_enable = 1;

	if (!options || !*options)
		return 1;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (!strncmp(this_opt, "disable", 7))
			eink_fb_enable = 0;
	}
	return 1;
}

static void
eink_fb_platform_release(
	struct device *device)
{
	// This is called when the reference count goes to zero.
}

void
__eink_clear_screen(
	int update_mode)
{
	memset(videomemory, APOLLO_WHITE, videomemorysize);
	__update_display(update_mode);
}

static
u_long get_line_length(
	int xres_virtual,
	int lbpp)
{
	return xres_virtual * lbpp / 8;
}

unsigned long
get_screen_size(
	unsigned long xres,
	unsigned long yres,
	unsigned long lbpp)
{
	return xres * yres * lbpp / 8;
}

unsigned long 
eink_fb_get_bpp(
    void)
{
    return (unsigned long)bpp;
}

u8 *
eink_fb_get_framebuffer(
	void)
{
	return (u8 *)videomemory;
}

static int
eink_set_bpp(
	struct fb_info *info,
	unsigned long lbpp)
{
	if (info == NULL || lbpp < EINK_DEPTH_1BPP || lbpp > EINK_DEPTH_2BPP) {
		return -1;
	}

	info->var.bits_per_pixel = lbpp;
	info->fix.line_length = get_line_length(info->var.xres_virtual, info->var.bits_per_pixel);
	info->screen_size = get_screen_size(info->var.xres_virtual, info->var.yres_virtual, info->var.bits_per_pixel);

	videomemorysize = info->fix.smem_len = info->screen_size;

	return 0;
}

static int
eink_init(
	int full,
	struct fb_info *info,
	unsigned long lbpp)
{
	if (0 == LOCK_APOLLO_ENTRY()) {
		pnlcd_flags_t pnlcd_flags = { 0 };
		
		full = apollo_init_kernel(full);
		eink_set_bpp(info, lbpp);

	    if (FAKE_PNLCD()) {
	        pnlcd_flags.enable_fake = 1;
	        set_pnlcd_flags(&pnlcd_flags);
	    }

		set_fb_ioctl(_eink_sys_ioctl);
	
		LOCK_APOLLO_EXIT();
	}
	
	return full;
}

static void
eink_done(
	int full)
{
	if (0 == LOCK_APOLLO_ENTRY()) {
		pnlcd_flags_t pnlcd_flags = { 0 };
		
		if (full) {
			__eink_sys_ioctl_fpow(FBIO_EINK_CLEAR_SCREEN, 0);
		} else {
			if (FPOW_MODE_OFF == g_eink_power_mode) {
				fpow_set_eink_disable_pending();
			}
		}
		apollo_done_kernel(full);
		
		if (full) {
			set_fb_init_flag(0);
		}
		set_pnlcd_flags(&pnlcd_flags);
		set_fb_ioctl(NULL);
		
		LOCK_APOLLO_EXIT();
	}
}

#ifdef CONFIG_PM_DEBUG
static void
print_mem(char *buf, int size, char *log_header)
{
    int x = 0;
    int bytes_to_print = size;
    char *cur_char = buf;

    return;

    // Print the header if given one
    if (log_header != NULL)
        printk(log_header);

    while (bytes_to_print)  {
        if (bytes_to_print >= 32)  {
            for (x=0;x<32;x++)  {
                printk("%02x",*cur_char++);
                if (x && (((x+1)%4)==0))
                    printk(" ");
            }
            bytes_to_print -= 32;
            printk("\n");
        }
        else if (bytes_to_print >= 16)  {
            for (x=0;x<16;x++)  {
                printk("%02x",*cur_char++);
                if (x && (((x+1)%4)==0))
                    printk(" ");
            }
            bytes_to_print -= 16;
        }
        else if (bytes_to_print >= 8) {
            for (x=0;x<8;x++)  {
                printk("%02x ",*cur_char++);
                if (x && (((x+1)%4)==0))
                    printk(" ");
            }
            bytes_to_print -= 8;
        }
        else  {
            for (x=0;x<bytes_to_print;x++)  {
                printk("%02x",*cur_char++);
            }
            bytes_to_print = 0;
        }
    }
    printk("\n");
}
#else
#define print_mem(x,y,z)    do {} while(0)
#endif

/*
 * Power management hooks.  Note that we won't be called from IRQ context,
 * so we may sleep.
 */

#ifdef CONFIG_FIONA_PM_VIDEO
static int __do_eink_fpow_mode_sleep(void) {
    return(__do_eink_fpow_setmode(FPOW_MODE_SLEEP));
}

static int __do_eink_fpow_setmode(u32 mode) {
    int retVal = FPOW_NO_ERR;
    retVal = __eink_fb_power(mode);
    if (retVal == FPOW_NO_ERR) {
        g_eink_power_mode = mode;
        verify_apollo_state();
    }
    else if (retVal != -FPOW_NOT_ENOUGH_POWER_ERR) {
        powdebug("__do_eink_fpow_setmode: setmode got error %d switching to %s\n",retVal, fpow_mode_to_str(mode));
        retVal = -FPOW_COMPONENT_TRANSITION_ERR;
    }
    else
        powdebug("__do_eink_fpow_setmode: setmode got error FPOW_NOT_ENOUGH_POWER_ERR (%d) switching to %s\n",retVal, fpow_mode_to_str(mode));
    return(retVal);
}

/*******************************************************************
 * Routine:     eink_fb_fpow_setmode
 *
 * Purpose:     Handles Fiona Power Management callbacks
 *              for doing EINK / APOLLO power management.
 *
 * Paramters:   private - eink_fb device *
 *              state   - ignored
 *              mode    - Power mode to switch to
 *                        Currently supported Modes:
 *                          FPOW_MODE_ON
 *                          FPOW_MODE_SLEEP
 *                          FPOW_MODE_OFF
 *
 * Returns:     
 *  FPOW_NO_ERR                     - Transitioned to mode with no errors.
 *  -FPOW_NOT_ENOUGH_POWER_ERR      - not enough power to transition
 *  -FPOW_COMPONENT_TRANSITION_ERR  - Failed to transition to mode
 *  -EINVAL                         - Invalid mode
 *  -ERESTARTSYS                    - Interrupted from getting apollo_lock
 *
 * Assumptions: none
 *******************************************************************/

static FPOW_ERR
eink_fb_fpow_setmode(void *private, u32 state, u32 mode)
{
    int retVal = FPOW_NO_ERR;

    powdebug("eink_fb_fpow_setmode(%s)\n",fpow_mode_to_str(mode));

	if (LOCK_APOLLO_ENTRY()) {
		return(-ERESTARTSYS);
	}

    // Check the state
    if (mode == g_eink_power_mode)  {
        extpowdebug("eink_fb_fpow_setmode(): driver already in %s mode\n",fpow_mode_to_str(g_eink_power_mode));
        goto unlock;
    }

    // Switch to the appropriate power mode
    switch (mode)  {
        case FPOW_MODE_OFF:
        case FPOW_MODE_ON:
        case FPOW_MODE_SLEEP:
            retVal = __do_eink_fpow_setmode(mode);
            break;

        default:
            powdebug("eink_fb_fpow_setmode: ERROR - setmode got unknown mode switch %d request.\n",mode);
            retVal = -EINVAL;
            break;
    }
    powdebug("eink_fb_fpow_setmode: driver now in %s mode\n",fpow_mode_to_str(g_eink_power_mode));

unlock:
	LOCK_APOLLO_EXIT();
    return(retVal);
}

static int
eink_fb_fpow_getmode(void *private)
{
    powdebug("eink_fb_fpow_getmode(0x%x) returning %s\n",(int)private,fpow_mode_to_str(g_eink_power_mode));
    return (g_eink_power_mode);
}

static int
eink_register_with_fpow(struct device *dev, struct fpow_component **fpow_dev)
{
    struct fpow_registration_rec   reg;
    int rc = 0;

    powdebug("eink_register_with_fpow(0x%x, 0x%x)\n",(int)dev,(int)fpow_dev);
    memclr(&reg,sizeof(reg));

    strcpy(reg.name,EINK_FB);
    reg.device_class = FPOW_CLASS_VIDEO_DISPLAY;
    reg.supported_modes =  (FPOW_MODE_ON_SUPPORTED					| \
                            FPOW_MODE_SLEEP_SUPPORTED				| \
                            FPOW_MODE_OFF_SUPPORTED);
    reg.private = (void*) dev;
    reg.getmode = eink_fb_fpow_getmode;
    reg.setmode = eink_fb_fpow_setmode;
    *fpow_dev = fpow_register(&reg);
    if (*fpow_dev == NULL) {
        printk("EINK: Failed to register with fpow !!\n");
        rc = -1;
    }
    return(rc);
}

static inline void
eink_unregister_with_fpow(struct fpow_component *fpow_dev)
{
    fpow_unregister(fpow_dev);
}

static void
__eink_fb_power_off_clear_screen(void)
{
	powdebug("__eink_fb_power_off_clear_screen() clearing screen...\n");
	
	apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_FAST);
	
	if (!FRAMEWORK_RUNNING()) {
		if (!get_screen_clear()) {
			// Don't set the screen_clear global here since we're not clearing the framebuffer.
			apollo_clear_screen(EINK_FULL_UPDATE_OFF);
		}
	} else {
		__eink_clear_screen(EINK_FULL_UPDATE_OFF);
	}
	
	splash_screen_up = splash_screen_power_off_clear_screen;
	begin_splash_screen_activity(splash_screen_power_off_clear_screen);
}

static int 
__eink_fb_power( u32 mode )
{
    int err = FPOW_NO_ERR;
    print_mem(videomemory,videomemorysize,"__eink_fb_power - videomemory : \n");

    powdebug("__eink_fb_power(%s) entering...\n",fpow_mode_to_str(mode));

    switch(mode)  {
        case FPOW_MODE_ON:
            extpowdebug ("__eink_fb_power waking Apollo...\n");
            err = apollo_set_power_state(APOLLO_SLEEP_POWER_MODE);
            if (0 == err) {
            	fpow_clear_eink_enable_pending();
            	// Don't update the display if the framework is running or we're first coming up.
            	if (!(FRAMEWORK_RUNNING() || in_probe)) {
                	__eink_sys_ioctl_fpow(FBIO_EINK_UPDATE_DISPLAY_SYNC, EINK_FULL_UPDATE_ON);
            	}
            }
            break;

		case FPOW_MODE_OFF:
			apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_SLOW);
			 
            // Entire system is going to sleep.
            extpowdebug ("__eink_fb_power() sleeping Apollo via APOLLO_STANDBY_OFF_POWER_MODE...\n");
            err = apollo_set_power_state(APOLLO_STANDBY_OFF_POWER_MODE);
            break;

        case FPOW_MODE_SLEEP:
            extpowdebug("__eink_fb_power(SLEEP) calling apollo_set_init_display_speed...\n");
            apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_FAST);
            extpowdebug("__eink_fb_power(SLEEP) sleeping Apollo via APOLLO_STANDBY_POWER_MODE...\n");
            err = apollo_set_power_state(APOLLO_STANDBY_POWER_MODE);
            break;

        default:
            powdebug("__eink_fb_power: Unknown mode %d passed to __eink_fb_power()\n",mode);
            err = FPOW_COMPONENT_TRANSITION_ERR;
            break;
    }

    powdebug("__eink_fb_power() returning %d\n",err);
    return(err);
}
#endif  // CONFIG_FIONA_PM_VIDEO

#define APOLLO_FLASH_SIZE				0x80000
#define APOLLO_RAM_SIZE					APOLLO_FLASH_SIZE

#define READ_SIZE						(1 << PAGE_SHIFT)
#define WRITE_SIZE						((unsigned long)0x20000) 

#define EINK_IGNORE_HW_ACCESS			APOLLO_UNKNOWN_POWER_MODE
#define EINK_PROC_BEGIN()				eink_proc_begin((char *)__FUNCTION__)

#define EINK_DONT_RESTORE_POWER_STATE	EINK_IGNORE_HW_ACCESS
#define EINK_RESTORE_POWER_STATE		0
#define EINK_PROC_END(p)												\
	if (p)																\
		proc_saved_apollo_power_state = EINK_DONT_RESTORE_POWER_STATE;	\
	eink_proc_end((char *)__FUNCTION__)

typedef int (*eink_rw_proc_sysf_t)(char *r_page_w_buf, unsigned long r_off_w_count, int *power_state);

static int proc_saved_apollo_power_state;

static int eink_proc_begin(char *function_name)
{
	int result = 0;
	
	if ( lock_apollo_entry(function_name) )
		result = -ERESTARTSYS;
		
	if ( IGNORE_HW_ACCESS() || begin_apollo_wake_on_activity(&proc_saved_apollo_power_state) )
	{
		einkdebug("EINK IGNORE: %s(pid=%d) HW off - ignoring call\n", function_name, fiona_sys_getpid());
		result = proc_saved_apollo_power_state = EINK_IGNORE_HW_ACCESS;
	}
	
	return ( result );
}

static void eink_proc_end(char *function_name)
{
	if ( EINK_IGNORE_HW_ACCESS != proc_saved_apollo_power_state )
		end_apollo_wake_on_activity(proc_saved_apollo_power_state);
	else
		prime_auto_power_timer(DELAY_TIMER);

	lock_apollo_exit(function_name);
}

static int
eink_proc_sysfs_rw(
	char *r_page_w_buf,
	char **r_start,
	unsigned long r_off_w_count,
	int *r_eof,
	int r_eof_len,
	eink_rw_proc_sysf_t eink_rw_proc_sysfs)
{
    int power_state = EINK_RESTORE_POWER_STATE;
    int result = 0;

	if (r_eof && (r_off_w_count > r_eof_len)) {
		goto skip;
	}

	result = EINK_PROC_BEGIN();

	if (EINK_IGNORE_HW_ACCESS == result) {
		goto unlock;
	} else if (result) {
		result = -ERESTARTSYS;
		goto done;
	}

	result = (*eink_rw_proc_sysfs)(r_page_w_buf, r_off_w_count, &power_state);
	
	if (r_start && r_page_w_buf) {
		*r_start = r_page_w_buf;
	}
	
unlock:
	EINK_PROC_END(power_state);
skip:
    if (r_eof) {
    	*r_eof = result >= r_eof_len;
    }
done:
    return result;
}

static int
update_display_write(
	struct file *file,
	const char __user *buf,
	unsigned long count,
	void *data)
{
	int m = 0, n[8] = { 0 };
	char lbuf[64] = { 0 };

	if (count > 64) {
		return -EINVAL;
	}

	if (copy_from_user(lbuf, buf, count)) {
        return -EFAULT;
	}
	
	m = sscanf(lbuf, "%d %d %d %d %d %d %d %d", &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], &n[6], &n[7]);

	if (m) {
		update_partial_t update_partial;
		fpow_override_t fpow_override;
		fx_t fx;

		unsigned long arg = 0;
		unsigned int cmd = 0;

		switch (n[0]) {
			case PROC_EINK_UPDATE_DISPLAY_CLS:
				cmd = FBIO_EINK_CLEAR_SCREEN;
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_ASIS:
				cmd = FBIO_EINK_UPDATE_DISPLAY_SYNC;
				arg = EINK_FULL_UPDATE_OFF;
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_FULL:
				cmd = FBIO_EINK_UPDATE_DISPLAY_SYNC;
				arg = EINK_FULL_UPDATE_ON;
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_PART:
				if (m > 4) {
					update_partial.x1 = n[1], update_partial.y1 = n[2];
					update_partial.x2 = n[3], update_partial.y2 = n[4];
					update_partial.buffer = NULL;
					
					if (m > 5) {
						update_partial.which_fx = n[5];
					} else {
						update_partial.which_fx = APOLLO_FX_NONE;
					}

					cmd = FBIO_EINK_UPDATE_PARTIAL;
					arg = (unsigned long)&update_partial;
				}
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_REST:
				cmd = FBIO_EINK_RESTORE_SCREEN;
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_SCRN:
				if (m > 1) {
					cmd = FBIO_EINK_SPLASH_SCREEN;
					arg = n[1];
				}
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_OVRD:
				if (m > 2) {
					fpow_override.cmd = n[1];
					
					if (m > 3) {
						fpow_override.arg = (unsigned long)&n[2];
					}
					else {
						fpow_override.arg = n[2];
					}
					
					cmd = FBIO_EINK_FPOW_OVERRIDE;
					arg = (unsigned long)&fpow_override;
				}
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_FX:
				if (m > 2) {
					fx.update_mode	= n[1];
					fx.which_fx		= n[2];
					
					if (m > 7) {
						fx.exclude_rect = n[3];
						fx.x1			= n[4];
						fx.y1			= n[5];
						fx.x2			= n[6];
						fx.y2			= n[7];
					} else {
						fx.exclude_rect = 0;
					}

					cmd = FBIO_EINK_UPDATE_DISPLAY_FX;
					arg = (unsigned long)&fx;
				}
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_SYNC:
				cmd = FBIO_EINK_SYNC_BUFFERS;
				break;
				
		    case PROC_EINK_UPDATE_DISPLAY_PNLCD:
		        cmd = FBIO_EINK_FAKE_PNLCD;
		        break;
		        
		    case PROC_EINK_FAKE_PNLCD_TEST:
		    	fake_pnlcd_test();
		    	break;
		}
		
		if (cmd) {
			eink_sys_ioctl(cmd, arg);
		}
	}
	
	return count;
}

static int
read_power_mode(
	char *page,
	unsigned long off,
	int *power_state)
{
	return sprintf(page, "%d (%s)\n", proc_saved_apollo_power_state, apollo_get_power_state_str(proc_saved_apollo_power_state));
}

static int
power_mode_read(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	return eink_proc_sysfs_rw(page, NULL, off, eof, 0, read_power_mode);
}

static int
write_power_mode(
	char *buf,
	unsigned long count,
	int *power_state)
{
    int requested_power_state = 0xff;
	char lbuf[16] = { 0 };

	if ((count <= 16) && copy_from_user(lbuf, buf, count)) {
		goto err_exit;
	}

    if (0 == sscanf(lbuf,"%d",&requested_power_state)) {
        goto bad_usage_exit;
    }

    // Validate mode before switching.
    switch (requested_power_state)  {
        case APOLLO_STANDBY_OFF_POWER_MODE:
        case APOLLO_SLEEP_POWER_MODE:
 		case APOLLO_NORMAL_POWER_MODE:
 		case APOLLO_STANDBY_POWER_MODE:
            break;

        default:
            goto bad_usage_exit;
    }

    // Switch power mode.
    if (0 == apollo_set_power_state(requested_power_state)) {
        powdebug("Apollo chip now in %s\n", apollo_get_power_state_str(requested_power_state));
        *power_state = EINK_DONT_RESTORE_POWER_STATE;
    } else {
        printk("ERROR transitioning Apollo chip into %s\n", apollo_get_power_state_str(requested_power_state));
	}

	goto done;

bad_usage_exit:
    printk( "Unsuported power mode: %d.\n"
            "Supported power modes:\n"
            "   0 - Standby-off mode (most power savings)\n"
            "   1 - Normal mode (least power savings)\n"
            "   2 - Sleep mode (some power savings)\n"
            "   3 - Standby mode (more power savings than sleep, less than standby-off)\n\n",
            requested_power_state);
	goto done;

err_exit:
    count = -EFAULT;

done:
    return count;
}

static int
power_mode_write(
	struct file *file,
	const char __user *buf,
	unsigned long count,
	void *data)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_power_mode);
}

static int
read_waveform_version(
	char *page,
	unsigned long off,
	int *power_state)
{
	return sprintf(page, "%s\n", apollo_get_waveform_version_string());
}

static int
waveform_version_read(
    char *page,
    char **start,
    off_t off,
    int count,
    int *eof,
    void *data)
{
	return eink_proc_sysfs_rw(page, NULL, off, eof, 0, read_waveform_version);
}

static int
read_info(
	char *page,
	unsigned long off,
	int *power_state)
{
    apollo_status_t status;
    apollo_info_t info;

	apollo_get_status(&status);
    apollo_get_info(&info);

	return sprintf(page,	" Waveform version:  0x%02X\n"
							"       subversion:  0x%02X\n"
							"             type:  0x%02X\n"
							"          FPL lot:  0x%02X\n"
							"         run type:  0x%02X\n"
							"\n"
							"     FPL platform:  0x%02X\n"
							"              lot:  0x%04X\n"
							" adhesive run no.:  0x%02X\n"
							"\n"
							"  Mfg data device:  0x%08lX\n"
							"          display:  0x%08lX\n"
							"       serial no.:  0x%08lX\n"
							"         checksum:  0x%08lX\n"
							"\n"
							"  Min temperature:  %d\n"
							"  Max temperature:  %d\n"
							"            steps:  %d\n"
							"             last:  %d\n"
							"\n"
							"   Operation mode:  0x%02X\n"
							"    screen status:  0x%02X\n"
							"     auto refresh:  0x%02X\n"
							"              bpp:  0x%02X\n"
							"\n"
							"   Apollo version:  0x%02X\n",
							
							info.waveform_version,
							info.waveform_subversion,
							info.waveform_type,
							info.fpl_lot_bcd,
							info.run_type,
							
							info.fpl_platform,
							info.fpl_lot_bin,
							info.adhesive_run_number,
							
							info.mfg_data_device,
							info.mfg_data_display,
							info.serial_number,
							info.checksum,
							
							info.waveform_temp_min,
							info.waveform_temp_max,
							info.waveform_temp_inc,
							info.last_temperature,
							
							status.operation_mode,
							status.screen,
							status.auto_refresh,
							status.bpp,
							
							info.controller_version);
}

static int
info_read(
    char *page,
    char **start,
    off_t off,
    int count,
    int *eof,
    void *data)
{
	return eink_proc_sysfs_rw(page, NULL, off, eof, 0, read_info);
}

#define RW_EINK_RAM 0 // Apollo Controller's RAM
#define RW_EINK_ROM 1 // Apollo Controller's FLash
#define RW_EINK_BUF 2 // eInk driver's screen_saver_buffer

static int
ReadFromBuf(
	int addr,
	unsigned char *data)
{
	int result = 0;
	
	if (data && (addr < videomemorysize) && screen_saver_buffer) {
		*data = screen_saver_buffer[addr];
		result = 1;
	}
	
	return result;
}

static int
WriteToBuf(
	int start,
	unsigned char *data,
	int len)
{
	int result = 0;
	
	if (data && ((start + len) <= videomemorysize) && screen_saver_buffer) {
		unsigned long *lptr_buf = (unsigned long *)screen_saver_buffer;
		unsigned long *lptr_dat = (unsigned long *)data;
		int i, j;

		for (i = start, j = 0; i < (start + (len >> 2)); i++, j++) {
			lptr_buf[i] = lptr_dat[j];
		}
		
		result = 1;
	}
	
	return result;
}

static int
read_eink_which(
	char *page,
	unsigned long off,
	int *power_state,
	int which,
	int which_size)
{
	char *cptr;
	int i, j;

	pinfo("%s, offset=%d\n", __FUNCTION__, (int)off);

	for (i = off, j = 0; (j < READ_SIZE) && (i < which_size);) {
		switch (which) {
			case RW_EINK_RAM:
				cptr = page + j;
				ReadFromRam(i, (unsigned long *)cptr);
				i += 4; j += 4;
				break;
				
			case RW_EINK_ROM:
				ReadFromFlash(i++, (page + j++));
				break;
				
			case RW_EINK_BUF:
				ReadFromBuf(i++, (page + j++));
				break;
		}
	}
  
  	if (power_state) {
  		*power_state = (i >= which_size) ? EINK_RESTORE_POWER_STATE : EINK_DONT_RESTORE_POWER_STATE;
  	}

	return j;
}

static int
read_eink_rom(
	char *page,
	unsigned long off,
	int *power_state)
{
	return read_eink_which(page, off, power_state, RW_EINK_ROM, APOLLO_FLASH_SIZE);
}

static int
eink_rom_read(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	return eink_proc_sysfs_rw(page, start, off, eof, APOLLO_FLASH_SIZE, read_eink_rom);
}

static int
write_eink_which(
	char *buf,
	unsigned long count,
	int *power_state,
	int which,
	int which_size)
{
	unsigned char *lbuf;
	unsigned long curr_count = 0;
	unsigned long count_sav = 0;
	int status = 0;

	lbuf = vmalloc(WRITE_SIZE);
	if (lbuf == NULL) {
		return -1;
	}

	if (RW_EINK_BUF == which) {
		valid_screen_saver_buf = screen_saver_invalid;
	}

	count_sav = count;
	count = min(count, (unsigned long)which_size);

	curr_count = 0;
	status = 1;

	pinfo("%s, size=%d\n", __FUNCTION__, (int)count);

	while ((curr_count < count) && (status == 1)) {
		unsigned long write_size;
		write_size = min(WRITE_SIZE, (count - curr_count));

		pinfo("curr_count=%d write_size=%d\n", (int) curr_count, (int) write_size);

		copy_from_user(lbuf, (buf + curr_count), write_size);
		switch (which) {
			case RW_EINK_RAM:
				status = ProgramRam(curr_count, lbuf, write_size);
				break;
							
			case RW_EINK_ROM:
				status = ProgramFlash(curr_count, lbuf, write_size);
				break;
				
			case RW_EINK_BUF:
				status = WriteToBuf(curr_count, lbuf, write_size);
				break;
		}

		pinfo("done, status=%d\n", status);

		curr_count += write_size;
	}

	vfree(lbuf);
	
	if ((RW_EINK_ROM == which) && status) {
		panel_id_t *panel_id = get_panel_id();
		
		if (panel_id && panel_id->override) {
			panel_id->override = 0;
		}
	}

	return status == 1 ? count_sav : -1;
}

static int
write_eink_rom(
	char *buf,
	unsigned long count,
	int *power_state)
{
	return write_eink_which(buf, count, power_state, RW_EINK_ROM, APOLLO_FLASH_SIZE);
}

static int
eink_rom_write(
	struct file *file,
	const char __user *buf,
	unsigned long count, void *data)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_eink_rom);
}

static int
read_eink_ram(
	char *page,
	unsigned long off,
	int *power_state)
{
	return read_eink_which(page, off, power_state, RW_EINK_RAM, APOLLO_RAM_SIZE);
}

static int
eink_ram_read(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	return eink_proc_sysfs_rw(page, start, off, eof, APOLLO_RAM_SIZE, read_eink_ram);
}

static int
write_eink_ram(
	char *buf,
	unsigned long count,
	int *power_state)
{
	return write_eink_which(buf, count, power_state, RW_EINK_RAM, APOLLO_RAM_SIZE);
}

static int
eink_ram_write(
	struct file *file,
	const char __user *buf,
	unsigned long count, void *data)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_eink_ram);
}

static int
read_eink_screen_saver(
	char *page,
	unsigned long off,
	int *power_state)
{
	return read_eink_which(page, off, power_state, RW_EINK_BUF, videomemorysize);
}

static int
eink_screen_saver_read(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	return eink_proc_sysfs_rw(page, start, off, eof, videomemorysize, read_eink_screen_saver);
}

static int
write_eink_screen_saver(
	char *buf,
	unsigned long count,
	int *power_state)
{
	return write_eink_which(buf, count, power_state, RW_EINK_BUF, videomemorysize);
}

static int
eink_screen_saver_write(
	struct file *file,
	const char __user *buf,
	unsigned long count, void *data)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_eink_screen_saver);
}

#define eink_adjustment_factor		0
#define eink_adjustment_async		1
#define eink_adjustment_auto_async	2
#define eink_adjustment_reverse		3
#define eink_adjustment_last		eink_adjustment_reverse

#define num_eink_adjustments		(eink_adjustment_last + 1)
#define len_eink_adjustments		((num_eink_adjustments * 5) + 1)

static int
read_eink_adjustments(
	char *page,
	unsigned long off,
	int *power_state)
{
	return sprintf(page,	"auto_adjustment_factor = %d: "
							"auto_refresh_timer_delay = %d, "
						 	"auto_power_timer_delay = %d, "
						 	"max_full_update_count = %d; "
						 	"update_display_async = %d; "
						 	"auto_sync_to_async = %d; "
						 	"use_reverse_animation = %d.\n",
							auto_adjustment_factor,
							AUTO_ADJUSTMENT_FACTOR(auto_refresh_timer_delay, AUTO_REFRESH_TIMER_DELAY),
							AUTO_ADJUSTMENT_FACTOR(auto_power_timer_delay, AUTO_POWER_TIMER_DELAY),
							AUTO_ADJUSTMENT_FACTOR(max_full_update_count, MAX_FULL_UPDATE_COUNT),
							update_display_async,
							auto_sync_to_async,
							use_reverse_animation);
}

static int
eink_adjustments_read(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	return eink_proc_sysfs_rw(page, NULL, off, eof, 0, read_eink_adjustments);
}

static int
write_eink_adjustments(
	char *buf,
	unsigned long count,
	int *power_state)
{
	int m = 0, n[num_eink_adjustments] = { 0 };
	char lbuf[len_eink_adjustments] = { 0 };

	if (count > len_eink_adjustments) {
		return -EINVAL;
	}

	if (copy_from_user(lbuf, buf, count)) {
        return -EFAULT;
	}
	
	m = sscanf(lbuf, "%d %d %d %d", &n[eink_adjustment_factor],
									&n[eink_adjustment_async],
									&n[eink_adjustment_auto_async],
									&n[eink_adjustment_reverse]);
	
	switch (m - 1) {
		default:
			if (m > num_eink_adjustments) {
				goto adjustments;
			}
			break;
		
		adjustments:
		case eink_adjustment_reverse:
			use_reverse_animation	= n[eink_adjustment_reverse];
		case eink_adjustment_auto_async:
			auto_sync_to_async		= n[eink_adjustment_auto_async];
		case eink_adjustment_async:
			update_display_async	= n[eink_adjustment_async];
		case eink_adjustment_factor:
			auto_adjustment_factor	= n[eink_adjustment_factor];
			
			__eink_sys_ioctl(FBIO_EINK_SPLASH_SCREEN, splash_screen_framework_reset);
			break;
	}
	
	return count;
}

static int
eink_adjustments_write(
	struct file *file,
	const char __user *buf,
	unsigned long count, void *data)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_eink_adjustments);
}

struct page *
vmalloc_2_page(
	void *addr)
{
	unsigned long lpage;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	struct page *page;
  
	lpage = VMALLOC_VMADDR(addr);
	spin_lock(&init_mm.page_table_lock);

	pgd = pgd_offset(&init_mm, lpage);
	pmd = pmd_offset(pgd, lpage);
	pte = pte_offset_map(pmd, lpage);
	page = pte_page(*pte);

	spin_unlock(&init_mm.page_table_lock);

	return page;

    //return vmalloc_to_page(addr);
}

int
init_page_array(
	void)
{
	int i;

	numpages = (videomemorysize >> PAGE_SHIFT) + 1;

	videopages = kmalloc(numpages * sizeof(struct page *), GFP_KERNEL);
	if (videopages == NULL) {
		return 1;
	}

	for (i = 0; (i << PAGE_SHIFT) < videomemorysize; i++) {
		videopages[i] =  vmalloc_2_page(frame_buffer + (i << PAGE_SHIFT));
	}

	return 0;
}

int
clear_page_array(
	void)
{
	if (videopages != NULL) {
		kfree(videopages);
	}

	return 0;
}

static int
__init eink_fb_probe(
	struct device *device)
{
	struct platform_device *dev = to_platform_device(device);
	struct fb_info *info;
	int total_buf_size = 0,
		retval = -ENOMEM,
		init_apollo = 0;

	if (!(frame_buffer = vmalloc(videomemorysize))) {
		return retval;
	}
	total_buf_size += videomemorysize;

	if (!(picture_buffer = vmalloc(picture_buffer_size))) {
		return retval;
	}
	total_buf_size += picture_buffer_size;

	videomemory = (void *)ioremap_nocache(get_framebuffer_start(), FRAMEBUFFER_SIZE);
	if (!videomemory) {
		if (!(videomemory = vmalloc(videomemorysize))) {
			return retval;
		}
		
		videomemory_local = 1;
	}
	total_buf_size += videomemory_local ? videomemorysize : FRAMEBUFFER_SIZE;
	
	if (USE_SLAB_ARRAY() && !build_slab_array()) {
		return retval;
	}
	
	in_probe = 1;

	if (alloc_screen_saver_buf) {
		screen_saver_buffer = vmalloc(videomemorysize);
		
		if (screen_saver_buffer) {
			total_buf_size += videomemorysize;
		}
	}
	
    fake_pnlcd_buffer = kmalloc(fake_pnlcd_buffer_size, GFP_KERNEL);
    if (fake_pnlcd_buffer) {
        total_buf_size += fake_pnlcd_buffer_size;
    }

	init_page_array();

	if (videomemory_local || !FB_INITED()) {
		memset(videomemory, APOLLO_WHITE, videomemorysize);
		splash_screen_up = splash_screen_invalid;
		init_apollo = 1;
	}
	
	memcpy_from_videomemory_to_frame_buffer();
  
	info = framebuffer_alloc(sizeof(u32) * 256, &dev->dev);

	if (!info) {
		goto err;
	}
  
	eink_fb_fix.smem_start = (unsigned long)frame_buffer; 
	
	info->screen_base = (char __iomem *)frame_buffer;
	info->fbops = &eink_fb_ops;
  
	info->var = eink_fb_default;
	info->fix = eink_fb_fix;
	info->pseudo_palette = NULL;/*info->par;*/
	info->par = NULL;
	info->flags = FBINFO_FLAG_DEFAULT;

	retval = fb_alloc_cmap(&info->cmap, 256, 0);
	if (retval < 0) {
		goto err1;
	}

	retval = register_framebuffer(info);
	if (retval < 0) {
		goto err2;
	}
	
	init_MUTEX(&vma_list_semaphore);

	init_apollo = eink_init(init_apollo, info, bpp);
	
	dev_set_drvdata(&dev->dev, info);

#ifdef CONFIG_FIONA_PM_VIDEO
    // Register EINK driver with Fiona Power Manager
    if (eink_register_with_fpow(device, &g_eink_fpow_component_ptr)) {
        printk("eink_fb_probe: failed to register with fpow !!\n");
    } else {
		// Enable Video and clear pending flag if everything went well
		powdebug("eink_fb_probe: calling eink_fb_fpow_setmode(FPOW_MODE_ON) from pending enable\n");
		if (FPOW_NO_ERR == eink_fb_fpow_setmode(NULL, 0, FPOW_MODE_ON)) {
			if (fpow_get_eink_disable_pending()) {
				// Clear and Disable Video
				powdebug("eink_fb_probe: calling eink_sys_ioctl(OFF_CLEAR)\n");
				if (eink_sys_ioctl(FBIO_EINK_OFF_CLEAR_SCREEN, 0)) {
					powdebug("eink_fb_probe: eink_sys_ioctl(OFF_CLEAR) failed\n");
				}
			} else if (init_apollo) {
				powdebug("eink_fb_probe: controller initialized from kernel\n");
				eink_sys_ioctl(FBIO_EINK_SPLASH_SCREEN, splash_screen_logo);
			}
		}
    }
#endif

	pinfo("/dev/fb/%d frame buffer device, using %dK for video memory, bpp = %d\n", info->node, (total_buf_size >> 10), bpp);
	retval = 0; goto not_in_probe;

err2:
	fb_dealloc_cmap(&info->cmap);

err1:
	framebuffer_release(info);

err:
	clear_page_array();
	if (fake_pnlcd_buffer) {
	    kfree(fake_pnlcd_buffer);
	}
	if (screen_saver_buffer) {
	    vfree(screen_saver_buffer);
	}
	free_slab_array();
	if (videomemory_local) {
		vfree(videomemory);
	} else {
		iounmap(videomemory);
	}
	vfree(picture_buffer);
	vfree(frame_buffer);

not_in_probe:
	in_probe = 0;

	return retval;
}

static int
eink_fb_remove(
	struct device *device)
{
	struct fb_info *info = dev_get_drvdata(device);

#ifdef CONFIG_FIONA_PM_VIDEO
    // Unregister EINK driver with Fiona Power Manager
    eink_unregister_with_fpow(g_eink_fpow_component_ptr);
    g_eink_fpow_component_ptr = NULL;
#endif

	eink_done(DONT_SHUTDOWN);

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
		clear_page_array();
        if (fake_pnlcd_buffer) {
            kfree(fake_pnlcd_buffer);
        }
        if (screen_saver_buffer) {
            vfree(screen_saver_buffer);
        }
		free_slab_array();
		if (videomemory_local) {
			vfree(videomemory);
		} else {
			iounmap(videomemory);
		}
		vfree(picture_buffer);
		vfree(frame_buffer);
	}

	return 0;
}

static int
eink_fb_reboot(struct notifier_block *self, unsigned long u, void *v)
{
	eink_done(FULL_SHUTDOWN);
	return NOTIFY_DONE;
}

static struct device_driver eink_fb_driver = {
	.name = EINK_FB,
	.bus = &platform_bus_type,
	.probe = eink_fb_probe,
	.remove = eink_fb_remove,
};

static struct platform_device eink_fb_device = {
	.name = EINK_FB,
	.id	= 0,
	.dev = {
		.release = eink_fb_platform_release,
	}
};

static struct notifier_block eink_fb_reboot_nb = {
	.notifier_call = eink_fb_reboot,
	.priority = FIONA_REBOOT_EINK_FB,
};

#define EINK_PROC_MODE_PARENT		(S_IFDIR | S_IRUGO | S_IXUGO)
#define EINK_PROC_MODE_CHILD_RW		(S_IWUGO | S_IRUGO)
#define EINK_PROC_MODE_CHILD_R_ONLY S_IRUGO
#define EINK_PROC_MODE_CHILD_W_ONLY S_IWUGO

#define EINK_PROC_PARENT			EINK_FB
#define EINK_PROC_UPDATE_DISPLAY	"update_display"
#define EINK_PROC_POWER				"power"
#define EINK_PROC_WAVEFORM_VERSION	"waveform_version"
#define EINK_PROC_INFO				"info"
#define EINK_PROC_EINK_ROM			"eink_rom"
#define EINK_PROC_EINK_RAM			"eink_ram"
#define EINK_PROC_EINK_SCREEN_SAVER	"screen_saver"
#define EINK_PROC_EINK_ADJUSTMENTS	"eink_adjustments"

static struct proc_dir_entry *proc_eink_parent;
static struct proc_dir_entry *proc_eink_update_display;
static struct proc_dir_entry *proc_apollo_power_mode_update; 
static struct proc_dir_entry *proc_eink_waveform_version;
static struct proc_dir_entry *proc_eink_info;
static struct proc_dir_entry *proc_eink_rom;
static struct proc_dir_entry *proc_eink_ram;
static struct proc_dir_entry *proc_eink_screen_saver;
static struct proc_dir_entry *proc_eink_adjustments;

static struct proc_dir_entry *create_eink_proc_entry(const char *name, mode_t mode, struct proc_dir_entry *parent,
    read_proc_t *read_proc, write_proc_t *write_proc)
{
    struct proc_dir_entry *eink_proc_entry = create_proc_entry(name, mode, parent);
    
    if ( eink_proc_entry )
    {
        eink_proc_entry->owner      = THIS_MODULE;
        eink_proc_entry->data       = NULL;
        eink_proc_entry->read_proc  = read_proc;
        eink_proc_entry->write_proc = write_proc;
    }
    
    return ( eink_proc_entry );
}

#define remove_eink_proc_entry(name, entry, parent)		\
    do                                              	\
    if ( entry )                                    	\
    {                                               	\
        remove_proc_entry(name, parent);            	\
        entry = NULL;                               	\
    }                                               	\
    while ( 0 )

#define DEVICE_MODE_RW			(S_IWUGO | S_IRUGO)
#define DEVICE_MODE_R_ONLY		S_IRUGO
#define DEVICE_MODE_W_ONLY		S_IWUGO

static ssize_t
show_splash_screen_up(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", splash_screen_up);
}

static int
read_off_delay(
	char *buf,
	unsigned long off,
	int *power_state)
{
	unsigned char off_delay;
	
	apollo_read_register(APOLLO_OFF_DELAY_REGISTER, &off_delay);
	return sprintf(buf, "%d\n", off_delay);
}

static int
write_off_delay(
	char *buf,
	unsigned long count,
	int *power_state)
{
	int result = -EINVAL, off_delay;
	
	if (sscanf(buf, "%d", &off_delay)) {
		off_delay = min(off_delay, APOLLO_OFF_DELAY_MAX);

		if (APOLLO_OFF_DELAY_THRESH < off_delay) {
			*power_state = EINK_DONT_RESTORE_POWER_STATE;
		}
				
		apollo_write_register(APOLLO_OFF_DELAY_REGISTER, off_delay);
		result = count;
	}
	
	return result;
}

static ssize_t
show_off_delay(
	struct device *dev,
	char *buf)
{
	return eink_proc_sysfs_rw(buf, NULL, 0, NULL, 0, read_off_delay);
}

static ssize_t
store_off_delay(
	struct device *dev,
	const char *buf,
	size_t count)
{
	return eink_proc_sysfs_rw((char *)buf, NULL, count, NULL, 0, write_off_delay);
}

static ssize_t
show_eink_debug(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", eink_debug);
}

static ssize_t
store_eink_debug(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, debug;
	
	if (sscanf(buf, "%d", &debug)) {
		eink_debug = debug ? 1 : 0;
		result = count;
	}
	
	return result;
}

static ssize_t
show_flash_fail_count(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", flash_fail_count);
}

static ssize_t
store_flash_fail_count(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, fail_count;
	
	if (sscanf(buf, "%d", &fail_count)) {
		flash_fail_count = fail_count;
		result = count;
	}
	
	return result;
}

static ssize_t
show_update_display_async(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", update_display_async);
}

static ssize_t
store_update_display_async(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, async;
	
	if (sscanf(buf, "%d", &async)) {
		update_display_async = async ? 1 : 0;
	
		if (!update_display_async) {
			auto_sync_to_async = 0;
		}

		result = count;
	}
	
	return result;
}

static ssize_t
show_auto_sync_to_async(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", auto_sync_to_async);
}

static ssize_t
store_auto_sync_to_async(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, async;
	
	if (sscanf(buf, "%d", &async)) {
		if (update_display_async) {
			auto_sync_to_async = async ? 1 : 0;
		}

		result = count;
	}

	return result;
}

static ssize_t
show_auto_adjustment_factor(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d\n", auto_adjustment_factor);
}

static ssize_t
store_auto_adjustment_factor(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, factor;
	
	if (sscanf(buf, "%d", &factor)) {
		auto_adjustment_factor = factor;
		result = count;
	}
	
	return result;
}

static ssize_t
show_valid_screen_saver_buf(
	struct device *dev,
	char *buf)
{
	return sprintf(buf, "%d %d %d %d\n",
						valid_screen_saver_buf,
						screen_saver_last,
						use_screen_saver_banner,
						present_screen_savers_in_order);
}

static ssize_t
store_valid_screen_saver_buf(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, valid, last, banner, order;

	switch (sscanf(buf, "%d %d %d %d", &valid, &last, &banner, &order)) {
		case 4:
			present_screen_savers_in_order = order;
		case 3:
			use_screen_saver_banner = banner;
		case 2:
			screen_saver_last = last;
		case 1:
			valid_screen_saver_buf = min((unsigned long)valid, videomemorysize);

			if (screen_saver_valid == valid_screen_saver_buf) {
				set_next_user_screen_saver();
			}

			result = count;
			break;
	}

	return result;
}

static ssize_t
show_use_fake_pnlcd(
	struct device *dev,
	char *buf)
{
	pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();
	
	return sprintf(buf, "%d %d %d\n", use_fake_pnlcd, (fake_pnlcd_x ? 1 : 0), pnlcd_flags->hide_real);
}

static ssize_t
store_use_fake_pnlcd(
	struct device *dev,
	const char *buf,
	size_t count)
{
	char clear_segments[PN_LCD_SEGMENT_COUNT] = { SEGMENT_OFF };
	int result = -EINVAL, fake_pnlcd = 0, hide_pnlcd = 0, x = 0;
	pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();

	switch (sscanf(buf, "%d %d %d", &fake_pnlcd, &x, &hide_pnlcd)) {
		case 3:
		    // Stop any pending animation and clear it if we're going to be
		    // hiding the PNLCD.
		    if (FRAMEWORK_RUNNING() && hide_pnlcd) {
                pnlcd_animation_t pnlcd_animation;
                
                pnlcd_animation.cmd = stop_animation;
                pnlcd_animation.arg = 0;

		        pnlcd_sys_ioctl(PNLCD_ANIMATE_IOCTL, &pnlcd_animation);
		        pnlcd_sys_ioctl(PNLCD_CLEAR_ALL_IOCTL, 0);
		    }
		    pnlcd_flags->hide_real = hide_pnlcd ? 1 : 0;

		case 2:
            // Clear out the old fake PNLCD state.
            _eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, (unsigned long)clear_segments);

            // Set fake_pnlcd_xres & fake_pnlcd_x based on x.
            set_fake_pnlcd_x_values(x);
		
		case 1:
            // Update to new state (both fake and real).
            use_fake_pnlcd = fake_pnlcd ? 1 : 0;
            pnlcd_flags->enable_fake = FAKE_PNLCD() ? 1 : 0;
		
			if (FRAMEWORK_RUNNING()) {
				pnlcd_sys_ioctl(PNLCD_SET_SEG_STATE, NULL);
				_eink_sys_ioctl(FBIO_EINK_FAKE_PNLCD, 0);
			}

            // Done.
		    result = count;
	}
	
	return result;
}

static ssize_t
store_fs_reboot_notification(
	struct device *dev,
	const char *buf,
	size_t count)
{
	int result = -EINVAL, notify;
	
	if (sscanf(buf, "%d", &notify)) {
		load_user_boot_screen();
		result = count;
	}
	
	return result;
}

static DEVICE_ATTR(fs_reboot_notification,  DEVICE_MODE_W_ONLY, NULL,							NULL);
static DEVICE_ATTR(use_fake_pnlcd,          DEVICE_MODE_R_ONLY, show_use_fake_pnlcd,            NULL);
static DEVICE_ATTR(valid_screen_saver_buf,  DEVICE_MODE_R_ONLY, show_valid_screen_saver_buf,	NULL);
static DEVICE_ATTR(auto_adjustment_factor,	DEVICE_MODE_R_ONLY,	show_auto_adjustment_factor,	NULL);
static DEVICE_ATTR(update_display_async,	DEVICE_MODE_R_ONLY, show_update_display_async,	 	NULL);
static DEVICE_ATTR(auto_sync_to_async,		DEVICE_MODE_R_ONLY, show_auto_sync_to_async,	 	NULL);
static DEVICE_ATTR(splash_screen_up,		DEVICE_MODE_R_ONLY,	show_splash_screen_up,		 	NULL);
static DEVICE_ATTR(flash_fail_count,		DEVICE_MODE_R_ONLY, show_flash_fail_count,			NULL);
static DEVICE_ATTR(eink_debug,  			DEVICE_MODE_R_ONLY, show_eink_debug,				NULL);
static DEVICE_ATTR(off_delay,				DEVICE_MODE_R_ONLY,	show_off_delay,				 	NULL);

int
__init eink_fb_init(
	void)
{
	int ret = 0;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options(EINK_FB, &option)) {
		return -ENODEV;
	}
	eink_fb_setup(option);
#endif

	if (!eink_fb_enable) {
		return -ENXIO;
	}

	ret = driver_register(&eink_fb_driver);
	
	if (!ret) {
		ret = platform_device_register(&eink_fb_device);
		if (ret) {
			driver_unregister(&eink_fb_driver);
		}
	}

	proc_eink_parent = create_proc_entry(EINK_PROC_PARENT, EINK_PROC_MODE_PARENT, NULL);

	if (proc_eink_parent) {
		proc_eink_update_display = create_eink_proc_entry(EINK_PROC_UPDATE_DISPLAY, EINK_PROC_MODE_CHILD_W_ONLY,
			proc_eink_parent, NULL, update_display_write);

		proc_apollo_power_mode_update = create_eink_proc_entry(EINK_PROC_POWER, EINK_PROC_MODE_CHILD_RW,
			proc_eink_parent, power_mode_read, power_mode_write);

		proc_eink_waveform_version = create_eink_proc_entry(EINK_PROC_WAVEFORM_VERSION, EINK_PROC_MODE_CHILD_R_ONLY,
			proc_eink_parent, waveform_version_read, NULL);

		proc_eink_info = create_eink_proc_entry(EINK_PROC_INFO, EINK_PROC_MODE_CHILD_R_ONLY,
			proc_eink_parent, info_read, NULL);

		proc_eink_rom = create_eink_proc_entry(EINK_PROC_EINK_ROM, EINK_PROC_MODE_CHILD_RW,
			proc_eink_parent, eink_rom_read, eink_rom_write);

		proc_eink_ram = create_eink_proc_entry(EINK_PROC_EINK_RAM, EINK_PROC_MODE_CHILD_RW,
			proc_eink_parent, eink_ram_read, eink_ram_write);
			
		proc_eink_screen_saver = create_eink_proc_entry(EINK_PROC_EINK_SCREEN_SAVER, EINK_PROC_MODE_CHILD_RW,
			proc_eink_parent, eink_screen_saver_read, eink_screen_saver_write);
		
		proc_eink_adjustments = create_eink_proc_entry(EINK_PROC_EINK_ADJUSTMENTS, EINK_PROC_MODE_CHILD_RW,
			proc_eink_parent, eink_adjustments_read, eink_adjustments_write);
	}

	dev_attr_off_delay.attr.mode = DEVICE_MODE_RW;
	dev_attr_off_delay.store = store_off_delay;
	device_create_file(&eink_fb_device.dev, &dev_attr_off_delay);

	dev_attr_eink_debug.attr.mode = DEVICE_MODE_RW;
	dev_attr_eink_debug.store = store_eink_debug;
	device_create_file(&eink_fb_device.dev, &dev_attr_eink_debug);

	dev_attr_flash_fail_count.attr.mode = DEVICE_MODE_RW;
	dev_attr_flash_fail_count.store = store_flash_fail_count;
	device_create_file(&eink_fb_device.dev, &dev_attr_flash_fail_count);
	
	device_create_file(&eink_fb_device.dev, &dev_attr_splash_screen_up);
	
	dev_attr_auto_sync_to_async.attr.mode = DEVICE_MODE_RW;
	dev_attr_auto_sync_to_async.store = store_auto_sync_to_async;
	device_create_file(&eink_fb_device.dev, &dev_attr_auto_sync_to_async);
	
	dev_attr_update_display_async.attr.mode = DEVICE_MODE_RW;
	dev_attr_update_display_async.store = store_update_display_async;
	device_create_file(&eink_fb_device.dev, &dev_attr_update_display_async);
	
	dev_attr_auto_adjustment_factor.attr.mode = DEVICE_MODE_RW;
	dev_attr_auto_adjustment_factor.store = store_auto_adjustment_factor;
	device_create_file(&eink_fb_device.dev, &dev_attr_auto_adjustment_factor);
	
	dev_attr_valid_screen_saver_buf.attr.mode = DEVICE_MODE_RW;
	dev_attr_valid_screen_saver_buf.store = store_valid_screen_saver_buf;
	device_create_file(&eink_fb_device.dev, &dev_attr_valid_screen_saver_buf);

	dev_attr_use_fake_pnlcd.attr.mode = DEVICE_MODE_RW;
	dev_attr_use_fake_pnlcd.store = store_use_fake_pnlcd;
	device_create_file(&eink_fb_device.dev, &dev_attr_use_fake_pnlcd);
	
	dev_attr_fs_reboot_notification.attr.mode = DEVICE_MODE_W_ONLY;
	dev_attr_fs_reboot_notification.store = store_fs_reboot_notification;
	device_create_file(&eink_fb_device.dev, &dev_attr_fs_reboot_notification);
	
	register_reboot_notifier(&eink_fb_reboot_nb);
	
	start_update_display_thread();
	start_screen_saver_thread();
	start_auto_refresh_timer();
	start_auto_power_timer();
	
	pinfo("Apollo Display Driver %s\n", VERSION);
	
	return ret;
}

static void __exit eink_fb_exit(void)
{
	stop_auto_power_timer();
	stop_auto_refresh_timer();
	stop_screen_saver_thread();
	stop_update_display_thread();
	
	unregister_reboot_notifier(&eink_fb_reboot_nb);
	
	device_remove_file(&eink_fb_device.dev, &dev_attr_fs_reboot_notification);
	device_remove_file(&eink_fb_device.dev, &dev_attr_use_fake_pnlcd);
	device_remove_file(&eink_fb_device.dev, &dev_attr_valid_screen_saver_buf);
	device_remove_file(&eink_fb_device.dev, &dev_attr_auto_adjustment_factor);
	device_remove_file(&eink_fb_device.dev, &dev_attr_update_display_async);
	device_remove_file(&eink_fb_device.dev, &dev_attr_auto_sync_to_async);
	device_remove_file(&eink_fb_device.dev, &dev_attr_splash_screen_up);
	device_remove_file(&eink_fb_device.dev, &dev_attr_flash_fail_count);
	device_remove_file(&eink_fb_device.dev, &dev_attr_eink_debug);
	device_remove_file(&eink_fb_device.dev, &dev_attr_off_delay);
	
	if (proc_eink_parent) {
		remove_eink_proc_entry(EINK_PROC_UPDATE_DISPLAY,	proc_eink_update_display,		proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_POWER,				proc_apollo_power_mode_update,	proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_WAVEFORM_VERSION,	proc_eink_waveform_version,		proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_INFO,				proc_eink_info,					proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_EINK_ROM,			proc_eink_rom,					proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_EINK_RAM,			proc_eink_ram,					proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_EINK_SCREEN_SAVER,	proc_eink_screen_saver,			proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_EINK_ADJUSTMENTS,	proc_eink_adjustments,			proc_eink_parent);
		remove_eink_proc_entry(EINK_PROC_PARENT,			proc_eink_parent,				NULL);
	}

	platform_device_unregister(&eink_fb_device);
	driver_unregister(&eink_fb_driver);
}

module_init(eink_fb_init);
module_exit(eink_fb_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
