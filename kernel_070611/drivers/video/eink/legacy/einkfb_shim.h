/*
 *  linux/drivers/video/eink/legacy/einkfb_shim.h -- eInk framebuffer device compatibility utility
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */
 
#ifndef _EINKFB_SHIM_H
#define _EINKFB_SHIM_H

#ifdef CONFIG_ARCH_FIONA    //------------- Fiona (Linux 2.6.10) Build

#include <asm/arch/fpow.h>
#include <asm/arch/pxa-regs.h>
#include <linux/pn_lcd.h>

#define SCREEN_SAVER_BPP				EINKFB_2BPP

#define SCREEN_SAVER_PATH_SYS_RO			"/opt/var/screen_saver/"
#define SCREEN_SAVER_PATH_SYS_RW			SCREEN_SAVER_PATH_SYS_RO
#define PNLCD_SYS_IOCTL(c, a)				\
	pnlcd_sys_ioctl(c, a)		

#define kobject_uevent_env(k, K, e)			((e) == (e))

#else   // -------------------------------- Mario (Linux 2.6.22) Build

#include <mach/hardware.h>
#include <asm/mach/time.h>
#include <linux/io.h>
#include <linux/zlib.h>

#include "pn_lcd.h"

#define SCREEN_SAVER_BPP				EINKFB_2BPP

#define SCREEN_SAVER_PATH_SYS_RO			"/opt/eink/images/"
#define SCREEN_SAVER_PATH_SYS_RW			"/var/local/eink/"
#define PNLCD_SYS_IOCTL(c, a)				\
	(get_pnlcd_ioctl()	? (*get_pnlcd_ioctl())((unsigned int)c, (unsigned long)a)	\
			   	: einkfb_pnlcd_ioctl_stub((unsigned int)c, (unsigned long)a))

extern int einkfb_pnlcd_ioctl_stub(unsigned int cmd, unsigned long arg);

#endif  // --------------------------------

#define FRAMEWORK_OR_DIAGS_RUNNING()			(FRAMEWORK_RUNNING() || running_diags)

enum update_type
{
	update_overlay = 0,	// Hardware composite on top of existing framebuffer.
	update_overlay_mask,	// Hardware composite on top of existing framebuffer using mask.
	update_partial,		// Software composite *into* existing framebuffer.
	update_full,		// Fully replace existing framebuffer.
	update_full_refresh,	// Fully replace existing framebuffer with manual refresh.
	update_none		// Buffer update only.
};
typedef enum update_type update_type;

enum update_which
{
	update_buffer = 0,	// Just update the buffer, not the screen.
	update_screen		// Update both the buffer and the screen.
};
typedef enum update_which update_which;

struct picture_info_type
{
	int		x, y;
	update_type	update;
	update_which	to_screen;
	int		headerless,
			xres, yres,
			bpp;
};
typedef struct picture_info_type picture_info_type;

struct system_screen_t
{
	u8			*picture_header,
				*picture_footer,
				*picture_body;
		
	int			picture_header_len,
				picture_footer_len,
				picture_body_len,
		
				header_width,
				footer_width,
				body_width,
				
				header_offset,
				footer_offset,
				body_offset;
		
	splash_screen_type	which_screen;
	update_which		to_screen;
	fx_type			which_fx;
};
typedef struct system_screen_t system_screen_t;

#define INIT_SYSTEM_SCREEN_T() { NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#define splash_screen_activity_begin			1
#define splash_screen_activity_end			0

#define begin_splash_screen_activity(s)			splash_screen_activity(splash_screen_activity_begin, s)
#define end_splash_screen_activity()			splash_screen_activity(splash_screen_activity_end, splash_screen_up)

extern splash_screen_type splash_screen_up;
extern bool power_level_initiated_call;
extern int running_diags;

// From einkfb_shim.c
//
extern void clear_screen(fx_type update_mode);

extern void splash_screen_activity(int which_activity, splash_screen_type which_screen);
extern void splash_screen_dispatch(splash_screen_type which_screen);

extern void display_system_screen(system_screen_t *system_screen);

// From einkfb_shim_<plaform>.c
//
extern int  einkfb_shim_gunzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);
extern int  einkfb_shim_gzip(unsigned char *dst, int dstlen, unsigned char *src, unsigned long *lenp);

extern unsigned char *einkfb_shim_alloc_kernelbuffer(unsigned long size);
extern void einkfb_shim_free_kernelbuffer(unsigned char *buffer);

extern void einkfb_shim_sleep_screen(unsigned int cmd, splash_screen_type which_screen);
extern void einkfb_shim_power_op_complete(void);
extern void einkfb_shim_power_off_screen(void);

extern bool einkfb_shim_override_power_lockout(unsigned int cmd, unsigned long flag);
extern bool einkfb_shim_enforce_power_lockout(void);
extern char *einkfb_shim_get_power_string(void);

extern bool einkfb_shim_platform_splash_screen_dispatch(splash_screen_type which_screen, int yres);

extern void einkfb_shim_platform_init(struct einkfb_info *info);
extern void einkfb_shim_platform_done(struct einkfb_info *info);

// From einksp_<platform>.h
//
extern u8 picture_usb_internal[];
extern u32 picture_usb_internal_len;
extern picture_info_type picture_usb_internal_info;

extern u8 picture_usb_external[];
extern u32 picture_usb_external_len;
extern picture_info_type picture_usb_external_info;

extern u8 picture_usb[];
extern u32 picture_usb_len;
extern picture_info_type picture_usb_info;

extern u8 picture_update[];
extern u32 picture_update_len;
extern picture_info_type picture_update_info;

#endif // _EINKFB_SHIM_H


