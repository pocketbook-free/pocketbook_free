/*
 *  linux/drivers/video/eink/hal/einkfb_hal_main.c -- eInk frame buffer device HAL
 *
 *      Copyright (C) 2005-2009 Lab126
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

#define _EINKFB_HAL_MAIN
#include "einkfb_hal.h"

#if PRAGMAS
	#pragma mark Kernel Abstract
	#pragma mark -
#endif

static int einkfb_reboot(struct notifier_block *self, unsigned long u, void *v);
static einkfb_reboot_hook_t einkfb_reboot_hook  = NULL;

static einkfb_suspend_resume_hook_t
       einkfb_suspend_resume_hook		= NULL;

static einkfb_info_hook_t einkfb_info_hook	= NULL;
static unsigned long einkfb_size		= 0;
static unsigned long einkfb_blen		= 0;
static unsigned long einkfb_mem			= 0;
static unsigned long einkfb_bpp			= 0;
static int einkfb_xres				= 0;
static int einkfb_yres				= 0;
static unsigned char *einkfb 			= NULL;
static unsigned char *einkfb_vfb		= NULL;
static unsigned char *einkfb_buf		= NULL;
static struct platform_device *einkfb_device	= NULL;
static struct fb_info *einkfb_fbinfo		= NULL;
static unsigned long einkfb_jif_on		= 0;
static unsigned long einkfb_align		= 0;
static bool einkfb_restore			= false;

static int einkfb_hw_shutdown_mode		= DONT_SHUTDOWN;
static int einkfb_hw_bringup_mode		= DONT_BRINGUP;
static int einkfb_hw_bringup_mode_actual	= DONT_BRINGUP;

static reboot_behavior_t einkfb_reboot_behavior = reboot_screen_clear;

einkfb_hal_ops_t hal_ops			= { 0 };

#ifndef MODULE
static int  einkfb_enable __INIT_DATA 		= 0;
#else
static int  einkfb_enable __INIT_DATA 		= 1;
static bool einkfb_hal_module			= false;

module_param_named(einkfb_hw_shutdown_mode, einkfb_hw_shutdown_mode, int, S_IRUGO);
MODULE_PARM_DESC(einkfb_hw_shutdown_mode, "non-zero to fully shut down hardware");

module_param_named(einkfb_hw_bringup_mode, einkfb_hw_bringup_mode, int, S_IRUGO);
MODULE_PARM_DESC(einkfb_hw_bringup_mode, "non-zero to fully bring up hardware");
#endif // MODULE

static struct fb_ops einkfb_ops 		=
{
	.owner		= THIS_MODULE,
	.fb_blank	= einkfb_blank,
	.fb_fillrect	= FB_FILLRECT,
	.fb_copyarea	= FB_COPYAREA,
	.fb_imageblit	= FB_IMAGEBLIT,
	.fb_ioctl	= einkfb_ioctl,
	.fb_mmap	= einkfb_mmap,
};

static struct notifier_block einkfb_reboot_nb	=
{
	.notifier_call	= einkfb_reboot,
	.priority	= REBOOT_PRIORITY,
};

static struct file_operations einkfb_events_ops =
{
	.owner		= THIS_MODULE,
	.open		= einkfb_events_open,
	.release	= einkfb_events_release,
	.read		= einkfb_events_read,
	.poll		= einkfb_events_poll
};

static struct miscdevice einkfb_events_dev	=
{
	MISC_DYNAMIC_MINOR,
	EINKFB_EVENTS,
	&einkfb_events_ops
};

FB_DEFIO();//add by jacky on oct 8
 
#ifndef MODULE
int __INIT_CODE einkfb_setup(char *options)
{
	char *this_opt;

	einkfb_enable = 1;

	if (!options || !*options)
		return 1;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (!strncmp(this_opt, "disable", 7))
			einkfb_enable = 0;
	}
	return 1;
}
#endif // MODULE

void einkfb_set_info_hook(einkfb_info_hook_t info_hook)
{
	// Need to make a queue of these if this becomes truly useful.
	//
	if ( info_hook )
		einkfb_info_hook = info_hook;
	else
		einkfb_info_hook = NULL;
}

EXPORT_SYMBOL(einkfb_set_info_hook);

void einkfb_get_info(struct einkfb_info *info)
{
	if ( info )
	{
		info->init   = einkfb_hw_bringup_mode_actual;
		info->done   = einkfb_hw_shutdown_mode;
		
		info->size   = einkfb_size;
		info->blen   = einkfb_blen;
		info->mem    = einkfb_mem;
		info->bpp    = einkfb_bpp;
		
		info->xres   = einkfb_xres;
		info->yres   = einkfb_yres;
		
		info->start  = einkfb_restore ? einkfb_vfb: einkfb;
		info->vfb    = einkfb_vfb;
		info->buf    = einkfb_buf;
		info->dev    = einkfb_device;
		info->fbinfo = einkfb_fbinfo;
		info->events = &einkfb_events_dev;
		
		info->jif_on = einkfb_jif_on;
		info->align  = einkfb_align;
		
		// If there's a hook, give it a call now.
		//
		if ( einkfb_info_hook )
			(*einkfb_info_hook)(info);
	}
}

EXPORT_SYMBOL(einkfb_get_info);

void einkfb_set_res_info(struct fb_info *info, int xres, int yres)
{
	einkfb_xres = xres;
	einkfb_yres = yres;
	
	if ( info )
	{
		bool swap_height_width = false;
		
		if ( (info->var.yres == xres) && (info->var.xres == yres) )
			swap_height_width = true;
		
		info->var.xres = info->var.xres_virtual = xres;
		info->var.yres = info->var.yres_virtual = yres;
		
		if ( swap_height_width )
		{
			u32 saved_height = info->var.height;
			
			info->var.height = info->var.width;
			info->var.width  = saved_height;
		}
		
		info->fix.line_length = BPP_SIZE(xres, einkfb_bpp);
	}
}

void einkfb_set_fb_restore(bool fb_restore)
{
	einkfb_restore = fb_restore;
}

void einkfb_set_jif_on(unsigned long jif_on)
{
	einkfb_jif_on = jif_on;
}

void einkfb_set_reboot_behavior(reboot_behavior_t behavior)
{
	einkfb_reboot_behavior = behavior;

	// Truly leave the screen (and controller's state) as is once we
	// get this message.
	//
	if ( reboot_screen_asis == behavior )
	{
		// We're going to be locking everyone out once the as-is
		// flag is set.  So, let's ensure that our locks are
		// in order now.
		//
		if ( !EINKFB_LOCK_READY_NOW() )
			EINFFB_LOCK_RELEASE();
		
		set_eink_init_display_flag(EINK_INIT_DISPLAY_ASIS);
	}
	else
		set_eink_init_display_flag(0);
}

reboot_behavior_t einkfb_get_reboot_behavior(void)
{
	return ( einkfb_reboot_behavior );
}

static unsigned long einkfb_set_byte_alignment(unsigned long byte_alignment)
{
	if ( hal_ops.hal_byte_alignment )
	{
		if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
		{
			byte_alignment = hal_ops.hal_byte_alignment();
			EINKFB_LOCK_EXIT();
		}
	}
	
	return ( byte_alignment );
}

static bool HAL_HW_INIT(struct fb_info *info, bool bringup_mode)
{
	einkfb_set_reboot_behavior(reboot_screen_clear);
	
	if ( hal_ops.hal_hw_init )
	{
		if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
		{
			bringup_mode = hal_ops.hal_hw_init(info, bringup_mode);
			EINKFB_LOCK_EXIT();
		}
	}
	else
		bringup_mode = FULL_BRINGUP;

	return ( bringup_mode );
}

static void HAL_HW_DONE(bool full)
{
	if ( hal_ops.hal_hw_done && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
	{
		hal_ops.hal_hw_done(full);
		EINKFB_LOCK_EXIT();
	}
}

static void BLANK_INIT(void)
{
	// If getting/setting the power-level is supported, get us unblanked now if we should,
	// and start the power timer.
	//
	// Otherwise, just say we're always on (by default, the blank_unblank
	// power-level is einkfb_power_level_standby, so we need to whack
	// to einkfb_power_level_on).
	//
	if ( hal_ops.hal_get_power_level )
	{
		einkfb_start_power_timer();
		EINKFB_BLANK(FB_BLANK_UNBLANK);
	}
	else
		einkfb_set_blank_unblank_level(einkfb_power_level_on);
		
#ifdef _EINKFB_BUILTIN
	einkfb_display_grayscale_ramp();
#endif
}

static void BLANK_DONE(void)
{
	// If getting/setting the power-level is supported, stop the power timer.
	//
	if ( hal_ops.hal_get_power_level )
		einkfb_stop_power_timer();
}

struct fb_info* einkfb_probe(struct platform_device *dev)
{
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	struct fb_info *info = NULL;
	int err = 0;

	/*
	 * Get the module's variable & fixed screeninfo.
	 */
	if ( EINKFB_FAILURE == HAL_SW_INIT(&var, &fix) )
		return NULL;
	
	/*
	 * Allocate an address space for our real, virtual, and scratch buffers.
	 */
	einkfb_blen = BPP_SIZE((var.xres * (var.yres + 1)), var.bits_per_pixel);
	einkfb_size = BPP_SIZE((var.xres * var.yres), var.bits_per_pixel);
	einkfb_mem  = FB_ROUNDUP(einkfb_blen, PAGE_SIZE) * 3;

	if (!(einkfb = vmalloc(einkfb_mem)))
		goto err0;

	/*
	 * Back our address space with physical ("wired") memory.
	 */
	if (EINKFB_FAILURE == einkfb_alloc_page_array())
		goto err1;

	/*
	 * Clear memory to prevent kernel info from "leaking"
	 * into userspace.
	 */
	einkfb_memset(einkfb, 0, einkfb_mem);

	info = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
	if (!info)
		goto err2;

	fix.smem_start = (unsigned long)einkfb;
	fix.smem_len = (einkfb_mem/3) * 2;	/* real & virtual */
	
	info->screen_base = (char __iomem *)einkfb;
	info->screen_size = einkfb_mem/3;	/* just real */
	info->fbops = &einkfb_ops;

	info->var = var;
	info->fix = fix;
	info->pseudo_palette = NULL;		/* info->par */
	info->par = NULL;
	info->flags = FBINFO_FLAG_DEFAULT;
	
	FB_DEFIO_INIT();//add by jacky on oct 8
	
	if (fb_alloc_cmap(&info->cmap, 256, 0) < 0)
		goto err3;

	einkfb_bpp   = info->var.bits_per_pixel;
	einkfb_xres  = info->var.xres;
	einkfb_yres  = info->var.yres;
	einkfb_vfb   = einkfb + info->screen_size;
	einkfb_buf   = einkfb_vfb + info->screen_size;

//	einkfb_align = einkfb_set_byte_alignment(einkfb_bpp);//add by jacky on oct 8
	
	einkfb_hw_bringup_mode_actual = HAL_HW_INIT(info, einkfb_hw_bringup_mode);
	einkfb_align = einkfb_set_byte_alignment(einkfb_bpp);

	if (register_framebuffer(info) < 0)
		goto err4;

	BLANK_INIT();
	
	einkfb_fbinfo = info;
	return info;

err4:

	FB_DEFIO_EXIT();//add by jacky on oct 8
	
	HAL_HW_DONE(einkfb_hw_shutdown_mode);
	fb_dealloc_cmap(&info->cmap);
	err++; // err = 5
err3:
	framebuffer_release(info);
	err++; // err = 4
err2:
	einkfb_free_page_array();
	err++; // err = 3
err1:
	vfree(einkfb);
	err++; // err = 2
err0:
	HAL_SW_DONE();
	err++; // err = 1

	einkfb_print_crit("failed to initialize, err = %d\n", err);
	
	return NULL;
}

static void einkfb_remove(struct fb_info *info)
{
	BLANK_DONE();
	unregister_framebuffer(info);

	FB_DEFIO_EXIT();//add by jacky on oct 8
	
	HAL_HW_DONE(einkfb_hw_shutdown_mode);
	fb_dealloc_cmap(&info->cmap);
	framebuffer_release(info);
	einkfb_free_page_array();
	vfree(einkfb);
	HAL_SW_DONE();
}

static int einkfb_driver_register(void);
static int einkfb_init(void)
{
#ifndef MODULE
	char *option = NULL;

	if (fb_get_options(EINKFB_NAME, &option))
		return -ENODEV;

	einkfb_setup(option);
#endif // MODULE

	if (!einkfb_enable)
		return -ENXIO;

	return einkfb_driver_register();
}

static int EINKFB_HAL_OPS_INIT(einkfb_hal_ops_t *einkfb_hal_ops)
{
	EINKFB_MEMCPYK(&hal_ops, einkfb_hal_ops, sizeof(einkfb_hal_ops_t));
	return ( einkfb_init() );
}

#ifdef MODULE
static int einkfb_hal_init(void);
int einkfb_hal_ops_init(einkfb_hal_ops_t *einkfb_hal_ops)
{
	int result = EINKFB_FAILURE;
	
	if ( einkfb_hal_ops && !einkfb_hal_module )
	{
		result = EINKFB_HAL_OPS_INIT(einkfb_hal_ops);
		
		if ( EINKFB_SUCCESS == result )
			einkfb_hal_module = true;
		else
			einkfb_hal_init();
	}

	return ( result );
}
#else
int einkfb_hal_ops_init(einkfb_hal_ops_t *einkfb_hal_ops)
{
	return ( EINKFB_HAL_OPS_INIT(einkfb_hal_ops) );
}
#endif // MODULE

EXPORT_SYMBOL(einkfb_hal_ops_init);

#ifdef MODULE
static int einkfb_hal_init(void)
{
	einkfb_memset(&hal_ops, 0, sizeof(einkfb_hal_ops_t));
	einkfb_hal_module = false;
	
	return ( EINKFB_SUCCESS );
}

static void einkfb_driver_unregister(void);
static void einkfb_exit(void)
{
	einkfb_driver_unregister();
}

static void einkfb_hal_exit(void)
{
	if ( einkfb_hal_module )
		einkfb_exit();
}

void einkfb_hal_ops_done(void)
{
	einkfb_hal_exit();
	einkfb_hal_init();
}

EXPORT_SYMBOL(einkfb_hal_ops_done);

module_init(einkfb_hal_init);
module_exit(einkfb_hal_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif	// MODULE

static int einkfb_reboot(struct notifier_block *self, unsigned long u, void *v)
{
	// Override any power lock-out behavior for reboot.
	//
	POWER_OVERRIDE_BEGIN();
	
	// Force the display back into portrait mode before rebooting.
	//
	EINKFB_IOCTL(FBIO_EINK_SET_DISPLAY_ORIENTATION, orientation_portrait);
	
	// Override the requested reboot behavior if we must.
	//
	switch ( u )
	{
		// If we're truly restarting, leave the reboot behavior alone.
		//
		case SYS_RESTART:
		break;
		
		// On actual shutdowns (or halts), don't put up the splash screen
		// if it's been requested; just clear the screen instead.
		//
		default:
			if ( reboot_screen_splash == einkfb_reboot_behavior )
				einkfb_reboot_behavior = reboot_screen_clear;
		break;
	}

	// If there are hooks, give them a call now.
	//
	if ( einkfb_reboot_hook )
		(*einkfb_reboot_hook)(einkfb_reboot_behavior);

	if ( hal_ops.hal_reboot_hook && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
	{
		(*hal_ops.hal_reboot_hook)(einkfb_reboot_behavior);
		EINKFB_LOCK_EXIT();
	}

	// If getting/setting the power-level is supported, get us either
	// powered off in the screen-clearing case or just suspended in
	// the cases where we're not supposed to clear the screen.
	//
	if ( hal_ops.hal_get_power_level )
	{
		switch ( einkfb_reboot_behavior )
		{
			case reboot_screen_asis:
			case reboot_screen_splash:
				EINKFB_BLANK(FB_BLANK_HSYNC_SUSPEND);
			break;
			
			case reboot_screen_clear:
				EINKFB_BLANK(FB_BLANK_POWERDOWN);
			break;
		}
	}
	else
	{
		// Otherwise, only clear the screen if we're supposed to.
		//
		if ( einkfb_reboot_behavior )
			einkfb_clear_display(fx_update_full);
	}

	POWER_OVERRIDE_END();
	
	// Clear all of our hooks, note that the framebuffer is
	// no longer initialized, and clear the ioctl callbacks.
	//
	einkfb_set_reboot_hook(NULL);
	einkfb_set_ioctl_hook(NULL);
	einkfb_set_info_hook(NULL);

	set_fb_init_flag(0);

	set_pnlcd_ioctl(NULL);
	set_fb_ioctl(NULL);
	
	return ( NOTIFY_DONE );
}

void einkfb_set_reboot_hook(einkfb_reboot_hook_t reboot_hook)
{
	// Need to make a queue of these if this becomes truly useful.
	//
	if ( reboot_hook )
		einkfb_reboot_hook = reboot_hook;
	else
		einkfb_reboot_hook = NULL;
}

EXPORT_SYMBOL(einkfb_set_reboot_hook);

static int einkfb_suspend(FB_SUSPEND_PARM)
{
	int result = EINKFB_SUCCESS;

	if ( einkfb_suspend_resume_hook )
		result = (*einkfb_suspend_resume_hook)(EINKFB_SUSPEND);

	return ( result );
}

static int einkfb_resume(FB_RESUME_PARAM)
{
	int result = EINKFB_SUCCESS;
	
	if ( einkfb_suspend_resume_hook )
		result = (*einkfb_suspend_resume_hook)(EINKFB_RESUME);
	
	return ( result );
}

void einkfb_set_suspend_resume_hook(einkfb_suspend_resume_hook_t suspend_resume_hook)
{
	// Need to make a queue of these if this becomes truly useful.
	//
	if ( suspend_resume_hook )
		einkfb_suspend_resume_hook = suspend_resume_hook;
	else
		einkfb_suspend_resume_hook = NULL;
}

EXPORT_SYMBOL(einkfb_set_suspend_resume_hook);

#if PRAGMAS
	#pragma mark -
	#pragma mark Kernel Concrete
	#pragma mark -
#endif

static int EINKFB_PROBE(FB_PROBE_PARAM)
{
	FB_PROBE_INIT

	struct fb_info *info = einkfb_probe(dev);
	int result = -ENXIO;
	
	if ( info )
	{
		FB_SETDRVDATA();
	
		einkfb_print_info(
			"fb%d using %ldK of RAM for framebuffer\n",
			info->node, einkfb_mem >> 10);
			
		result = EINKFB_SUCCESS;
	}

	return ( result );
}

static int EINKFB_REMOVE(FB_REMOVE_PARAM)
{
	struct fb_info *info = FB_GETDRVDATA();

	if ( info )
		einkfb_remove(info);
	
	return ( EINKFB_SUCCESS );
}

FB_DRIVER(einkfb_driver, EINKFB_NAME, EINKFB_PROBE, EINKFB_REMOVE);

static void einkfb_device_driver_free(void)
{
	FB_PLATFREE(einkfb_device);
	FB_DRVUNREG(&einkfb_driver);
}

static int einkfb_driver_register(void)
{
	int ret = FB_DRVREG(&einkfb_driver);

	if ( !ret )
	{
		FB_PLATALLOC(einkfb_device, EINKFB_NAME);

		if ( einkfb_device )
		{
			ret = FB_PLATREG(einkfb_device);
			
			if ( !ret )
			{
				ret = einkfb_create_proc_entries();
				
				if ( !ret )
				{
					register_reboot_notifier(&einkfb_reboot_nb);
					misc_register(&einkfb_events_dev);
				}
			}
		}
		else
			ret = -ENOMEM;

		if ( ret )
			einkfb_device_driver_free();
	}

	return ret;
}

#ifdef MODULE
static void einkfb_driver_unregister(void)
{
	misc_deregister(&einkfb_events_dev);
	unregister_reboot_notifier(&einkfb_reboot_nb);
	einkfb_remove_proc_entries();
	platform_device_unregister(einkfb_device);
	
	einkfb_device_driver_free();
}
#endif // MODULE
