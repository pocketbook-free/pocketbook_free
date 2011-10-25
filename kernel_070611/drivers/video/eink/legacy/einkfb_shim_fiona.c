/*
 *  linux/drivers/video/eink/legacy/einkfb_shim_fiona.c -- eInk framebuffer device platform compatibility
 *
 *      Copyright (C) 2005-2007 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

#include "../hal/einkfb_hal.h"
#include "einkfb_shim.h"

#include "einksp_fiona.h"

#if PRAGMAS
	#pragma mark Definitions/Globals
	#pragma mark -
#endif

extern int fiona_gunzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);
extern int fiona_gzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);

static struct fpow_component *g_eink_fpow_component_ptr = NULL;
FPOW_POWER_MODE g_eink_power_mode = FPOW_MODE_OFF;

#if PRAGMAS
	#pragma mark -
	#pragma mark Utilities
	#pragma mark -
#endif

int einkfb_shim_gunzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp)
{
	return ( fiona_gunzip(dst, dstlen, src, lenp) );
}

int einkfb_shim_gzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp)
{
	return ( fiona_gzip(dst, dstlen, src, lenp) );
}

unsigned char *einkfb_shim_alloc_kernelbuffer(unsigned long size)
{
	return ( (unsigned char *)ioremap_nocache(get_framebuffer_start(), FRAMEBUFFER_SIZE) );
}

void einkfb_shim_free_kernelbuffer(unsigned char *buffer)
{
	if ( buffer )
		iounmap(buffer);
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Fpow
	#pragma mark -
#endif

#define FPOW_OVERRIDE_BEGIN()	POWER_OVERRIDE_BEGIN()
#define FPOW_OVERRIDE_END()	POWER_OVERRIDE_END()
#define FPOW_OVERRIDE()		POWER_OVERRIDE()

#define IN_FPOW_OVERRIDE(c, f)	((FPOW_OVERRIDE()			&&	\
				  (EINKFB_IOCTL_KERN == f))		||	\
				 (FBIO_EINK_SPLASH_SCREEN_SLEEP == c)	||	\
				 (FBIO_EINK_OFF_CLEAR_SCREEN == c)	||	\
				 (FBIO_EINK_POWER_OVERRIDE == c)) 

#define ENFORCE_FPOW_LOCKOUT()	(FPOW_MODE_ON != g_eink_power_mode)
#define FPOW_MODE_TO_STR()	fpow_mode_to_str(g_eink_power_mode)

static FPOW_ERR einkfb_shim_fpow_setmode(void *private, u32 state, u32 mode)
{
	int result = FPOW_NO_ERR;
	
	if ( g_eink_power_mode != mode )
	{
		switch ( mode )
		{
			case FPOW_MODE_ON:
				EINKFB_BLANK(FB_BLANK_UNBLANK);
			break;
			
			case FPOW_MODE_SLEEP:
				EINKFB_BLANK(FB_BLANK_VSYNC_SUSPEND);
			break;
			
			case FPOW_MODE_OFF:
				EINKFB_BLANK(FB_BLANK_HSYNC_SUSPEND);
			break;
			
			default:
				result = ~FPOW_NO_ERR;
			break;
		}
		
		if ( FPOW_NO_ERR == result )
			g_eink_power_mode = mode;
	}

	return ( result );
}

static int einkfb_shim_fpow_getmode(void *private)
{
	return ( g_eink_power_mode );
}

#define EINKFB_SHIM_FPOW_SETMODE(m) einkfb_shim_fpow_setmode(NULL, 0, m)

static void einkfb_shim_fpow_off_clear_screen(void)
{
	fpow_clear_eink_disable_pending();
	
	if ( FRAMEWORK_RUNNING() )
	{
		// Clear the screen and get into sleep mode ASAP.
		//
		EINKFB_BLANK(FB_BLANK_NORMAL);
		EINKFB_SHIM_FPOW_SETMODE(FPOW_MODE_SLEEP);
		
		// Clear the framebuffer.
		//
		FPOW_OVERRIDE_BEGIN();
			clear_screen(fx_update_partial);
		FPOW_OVERRIDE_END();
	}
	else
		EINKFB_SHIM_FPOW_SETMODE(FPOW_MODE_OFF);
	
	splash_screen_up = splash_screen_power_off_clear_screen;
	begin_splash_screen_activity(splash_screen_power_off_clear_screen);
}

static void einkfb_shim_fpow_sleep_screen(unsigned int cmd, splash_screen_type which_screen)
{
	if ( FBIO_EINK_SPLASH_SCREEN_SLEEP == cmd )
	{
		EINKFB_SHIM_FPOW_SETMODE(FPOW_MODE_SLEEP);
		FPOW_OVERRIDE_BEGIN();
	}

	splash_screen_dispatch(which_screen);
	
	if ( FBIO_EINK_SPLASH_SCREEN_SLEEP == cmd )
		FPOW_OVERRIDE_END();
}

static int einkfb_shim_register_with_fpow(struct device *dev, struct fpow_component **fpow_dev)
{
	struct fpow_registration_rec reg;
	int result = ~FPOW_NO_ERR;

	reg.device_class	= FPOW_CLASS_VIDEO_DISPLAY;
	reg.supported_modes	= (	FPOW_MODE_ON_SUPPORTED		| \
					FPOW_MODE_SLEEP_SUPPORTED	| \
					FPOW_MODE_OFF_SUPPORTED		);
	reg.private		= (void*)dev;
	reg.getmode 		= einkfb_shim_fpow_getmode;
	reg.setmode 		= einkfb_shim_fpow_setmode;
	
	strcpy(reg.name, EINKFB_NAME);

	*fpow_dev = fpow_register(&reg);

	if ( *fpow_dev )
		result = FPOW_NO_ERR;
		
	return ( result );
}

static void einkfb_shim_fpow_init(struct device *device, bool init)
{
	// Attempt to register with the Fiona Power Manager.
	//
	if ( FPOW_NO_ERR == einkfb_shim_register_with_fpow(device, &g_eink_fpow_component_ptr) )
	{
		// On success, set us into the Power Manager's "on" mode.
		//
		if ( FPOW_NO_ERR == einkfb_shim_fpow_setmode(NULL, 0, FPOW_MODE_ON) )
		{
			// But, check to see whether the Power Manager needs for us to
			// be off or not.
			//
			if ( fpow_get_eink_disable_pending() )
				einkfb_shim_fpow_off_clear_screen();
			else
			{
				// Otherwise, check to see whether the hardware needed
				// to be brought up from scratch.  And, if it did, put
				// up the logo.
				//
				if ( init )
				{
					splash_screen_up = splash_screen_invalid;
					splash_screen_dispatch(splash_screen_logo);
				}
			}
		}
	}
}

static void einkfb_shim_fpow_done(void)
{
	fpow_unregister(g_eink_fpow_component_ptr);
	g_eink_fpow_component_ptr = NULL;
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Power
	#pragma mark -
#endif

void einkfb_shim_sleep_screen(unsigned int cmd, splash_screen_type which_screen)
{
	einkfb_shim_fpow_sleep_screen(cmd, which_screen);
}

void einkfb_shim_power_op_complete(void)
{
	fpow_video_updated();
}

void einkfb_shim_power_off_screen(void)
{
	einkfb_shim_fpow_off_clear_screen();
}

bool einkfb_shim_override_power_lockout(unsigned int cmd, unsigned long flag)
{
	return ( IN_FPOW_OVERRIDE(cmd, flag) );
}

bool einkfb_shim_enforce_power_lockout(void)
{
	return ( ENFORCE_FPOW_LOCKOUT() );
}

char *einkfb_shim_get_power_string(void)
{
	return ( FPOW_MODE_TO_STR() );
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Splash Screen
	#pragma mark -
#endif

bool einkfb_shim_platform_splash_screen_dispatch(splash_screen_type which_screen, int yres)
{
	return ( false );
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Main
	#pragma mark -
#endif

void einkfb_shim_platform_init(struct einkfb_info *info)
{
	// Register with fpow.
	//
	einkfb_shim_fpow_init(&info->dev->dev, info->init);
}

void einkfb_shim_platform_done(struct einkfb_info *info)
{
	// Deregister with fpow.
	//
	einkfb_shim_fpow_done();
}
