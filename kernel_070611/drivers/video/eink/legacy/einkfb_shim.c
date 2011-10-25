/*
 *  linux/drivers/video/eink/legacy/einkfb_shim.c -- eInk framebuffer device compatibility utility
 *
 *      Copyright (C) 2005-2009 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */
 
#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/module.h>
#include <linux/syscalls.h>

#include "../hal/einkfb_hal.h"
#include "einkfb_shim.h"

#include "einksp.h"

#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/initrd.h>
#include <linux/string.h>

#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/mount.h>
#include <linux/major.h>
#include <linux/root_dev.h>


#if PRAGMAS
	#pragma mark Definitions/Globals
	#pragma mark -
#endif

static u8   einkfb_apply_fx_which(u8 data, int i, int which_fx);
static int  compress_picture(int picture_length, u8 *picture);
static void set_next_user_screen_saver(void);
static void wake_screen_saver_thread(void);
static void exit_screen_saver_thread(void);
static void set_fake_pnlcd_x_values(int x);
static void buffer_copy(int which);

#define from_screen_saver_to_kernelbuffer 		0
#define from_picture_buffer_to_kernelbuffer 		1
#define from_picture_buffer_to_screensaver 		2
#define from_framebuffer_to_kernelbuffer		3
#define from_kernelbuffer_to_framebuffer		4

#define memcpy_from_screen_saver_to_kernelbuffer()	buffer_copy(from_screen_saver_to_kernelbuffer)
#define memcpy_from_picture_buffer_to_kernelbuffer()	buffer_copy(from_picture_buffer_to_kernelbuffer)
#define memcpy_from_picture_buffer_to_screensaver()	buffer_copy(from_picture_buffer_to_screensaver)
#define memcpy_from_framebuffer_to_kernelbuffer()	buffer_copy(from_framebuffer_to_kernelbuffer)
#define memcpy_from_kernelbuffer_to_framebuffer()	buffer_copy(from_kernelbuffer_to_framebuffer)

#define EINKFB_PROC_EINK_SCREEN_SAVER			"screen_saver"
#define SCREEN_SAVER_THREAD_NAME			EINKFB_NAME"_sst"
#define SCREEN_SAVER_PATH_USER				"/mnt/us/system/screen_saver/"

#define SCREEN_SAVER_PATH_TEMPLATE			"screen_saver_%d.gz"
#define SCREEN_SAVER_PATH_LAST				"screen_saver_last"
#define SCREEN_SAVER_PATH_SIZE				256
#define SCREEN_SAVER_DEFAULT				0
#define SCREEN_SAVER_XRES				600
#define SCREEN_SAVER_YRES				800

#define FAKE_PNLCD_SEGMENT_RES				8
#define FAKE_PNLCD_YRES					FAKE_PNLCD_SEGMENT_RES
#define FAKE_PNLCD_XRES_MAX				(FAKE_PNLCD_SEGMENT_RES * PN_LCD_COLUMN_COUNT)

#define FAKE_PNLCD_X_START(n)				(((n) & 1) ? 0 : fake_pnlcd_xres)
#define FAKE_PNLCD_Y_START(n)				(((n) / 2) * FAKE_PNLCD_YRES)

#define FAKE_PNLCD_X_END(s)				((s) + fake_pnlcd_xres)
#define FAKE_PNLCD_Y_END(s)				((s) + FAKE_PNLCD_YRES)

#define MAX_SEG(s)					min((((s) | 1) + 1), MAX_PN_LCD_SEGMENT)

#define USE_FAKE_PNLCD(c)				(use_fake_pnlcd && fake_pnlcd_buffer && (c))
#define FAKE_PNLCD()					USE_FAKE_PNLCD(1)

#define CHECK_FRAMEWORK_RUNNING()			(FRAMEWORK_STARTED() && !FRAMEWORK_RUNNING())

#define ALREADY_UNCOMPRESSED				(-1)
#define PROGRESSBAR_Y					(-1)
#define PROGRESSBAR_X					(-1)

#define ORIENTATION_STRING_UP				"orientation=up"
#define ORIENTATION_STRING_DOWN				"orientation=down"
#define ORIENTATION_STRING_LEFT				"orientation=left"
#define ORIENTATION_STRING_RIGHT			"orientation=right"

#define FAKE_FLASHING_AREA_UPDATE_XRES			6

splash_screen_type splash_screen_up = splash_screen_boot;
static bool splash_screen_logo_buffer_only = false;
static int dont_deanimate = 0;

static int local_update_area = 0;
static int send_fake_rotate = 0;
static int eink_debug = 0;
int running_diags = 0;

static picture_info_type shim_picture_info = { 0 };
static u8 *shim_picture = NULL; 
static int shim_picture_len = 0; 

static int fake_pnlcd_min_seg = FIRST_PNLCD_SEGMENT;
static int fake_pnlcd_max_seg = LAST_PNLCD_SEGMENT;
static int fake_pnlcd_xres = FAKE_PNLCD_XRES_MAX/2;
static int fake_pnlcd_valid = 0;
static int use_fake_pnlcd = 0;
static int fake_pnlcd_x = 0;

static u_long fake_pnlcd_buffer_size = 0;
static u8 *fake_pnlcd_buffer = NULL;

static u_long picture_buffer_size = 0;
static u8 *picture_buffer = NULL;

static u_long screen_saver_buffer_size = 0;
static u8 *screen_saver_buffer = NULL;

static bool kernelbuffer_local = false;
static u_long kernelbuffer_size = 0;
static u8 *kernelbuffer = NULL;

static u8 *override_framebuffer = NULL;
static u8 *framebuffer = NULL;

static bool framebuffer_orientation = EINK_ORIENT_PORTRAIT;
static u_long framebuffer_align = 0;
static u_long framebuffer_size = 0;
static int framebuffer_xres = 0;
static int framebuffer_yres = 0;
static u32 framebuffer_bpp = 0;

static struct proc_dir_entry *einkfb_proc_screen_saver = NULL;
static int valid_screen_saver_buf = 0;
static int get_next_screen_saver = 0;

static u8  progressbar_background = EINKFB_WHITE;
static int progressbar_y = PROGRESSBAR_Y;
static int progressbar_x = PROGRESSBAR_X;
static int progressbar = 0;

static int screen_saver_last = SCREEN_SAVER_DEFAULT;
static int screen_saver_thread_exit = 0;
static THREAD_ID(screen_saver_id);
static DECLARE_COMPLETION(screen_saver_thread_exited);
static DECLARE_COMPLETION(screen_saver_thread_complete);

static char screen_saver_path_template[SCREEN_SAVER_PATH_SIZE];
static char screen_saver_path_last[SCREEN_SAVER_PATH_SIZE];

#define stipple_even_row_mask		0
#define stipple_even_row_mask_type	1

#define stipple_odd_row_mask		2
#define stipple_odd_row_mask_type	3

#define stipple_table_size		4

#define stipple_table_mask_and		1
#define stipple_table_mask_or		0

#define STIPPLE_TABLE_MASK(t, m, d)	(t ? (m & d) : (m | d))

#define posterize_even_row_mask		0
#define posterize_odd_row_mask		1
#define posterize_bpp			2

#define posterize_table_size		3

#define POSTERIZE_TABLE_MASK(b, m, d)	((EINKFB_4BPP == b)		\
					? posterize_4bpp_table[(m & d)]	\
					: posterize_2bpp_table[(m & d)])

#define einkfb_posterize_table		einkfb_stipple_table
#define einkfb_posterize_rowbytes	einkfb_stipple_rowbytes
#define einkfb_posterize_bytes		einkfb_stipple_bytes
#define einkfb_posterize_rows		einkfb_stipple_rows

static u8 stipple_lighten_dark_gray_2bpp_table[stipple_table_size] =
{
	0x33, stipple_table_mask_and,	// row 0: WXWX... (white keep  white keep...)
	0x33, stipple_table_mask_or	// row 1: XBXB... (keep  black keep  black...)
};

static u8 stipple_lighten_dark_gray_4bpp_table[stipple_table_size] =
{
	0x0F, stipple_table_mask_and,	// row 0: WX... (white keep  ...)
	0x0F, stipple_table_mask_or	// row 1: XB... (keep  black ...)
};

static u8 stipple_lighten_lite_gray_2bpp_table[stipple_table_size] =
{
	0x33, stipple_table_mask_and,	// row 0: WXWX...
	0xCC, stipple_table_mask_and	// row 1: XWXW...
};

static u8 stipple_lighten_lite_gray_4bpp_table[stipple_table_size] =
{
	0x0F, stipple_table_mask_and,	// row 0: WX...
	0xF0, stipple_table_mask_and	// row 1: XW...
};

static u8 posterize_dark_2bpp_table[posterize_table_size] = 
{
	0x33,				// row 0: WPWP... (white posterize white posterize...)
	0xFF,				// row 1: PBPB... (black posterize black posterize...)
	
	EINKFB_2BPP
};

static u8 posterize_dark_4bpp_table[posterize_table_size] =
{
	0x0F,				// row 0: WP...
	0xFF,				// row 1: PB...
	
	EINKFB_4BPP
};

static u8 posterize_lite_2bpp_table[posterize_table_size] =
{
	0x33,				// row 0: WPWP...
	0xCC,				// row 1: PWPW...
	
	EINKFB_2BPP
};

static u8 posterize_lite_4bpp_table[posterize_table_size] =
{
	0x0F,				// row 0: WP...
	0xF0,				// row 1: PW...
	
	EINKFB_4BPP
};

static u8 posterize_2bpp_table[256] =
{
	0x00, 0x03, 0x03, 0x03, 0x0C, 0x0F, 0x0F, 0x0F,	// 0x00..
	0x0C, 0x0F, 0x0F, 0x0F, 0x0C, 0x0F, 0x0F, 0x0F,	// ..0x0F
	
	0x30, 0x33, 0x33, 0x33, 0x3C, 0x3F, 0x3F, 0x3F, // 0x10..
	0x3C, 0x3F, 0x3F, 0x3F, 0x3C, 0x3F, 0x3F, 0x3F,	// ..0x1F
	
	0x30, 0x33, 0x33, 0x33, 0x3C, 0x3F, 0x3F, 0x3F, // 0x20..
	0x3C, 0x3F, 0x3F, 0x3F, 0x3C, 0x3F, 0x3F, 0x3F, // ..0x2F
	
	0x30, 0x33, 0x33, 0x33, 0x3C, 0x3F, 0x3F, 0x3F, // 0x30..
	0x3C, 0x3F, 0x3F, 0x3F, 0x3C, 0x3F, 0x3F, 0x3F,	// ..0x3F
	
	0xC0, 0xC3, 0xC3, 0xC3, 0xCC, 0xCF, 0xCF, 0xCF, // 0x40..
	0xCC, 0xCF, 0xCF, 0xCF, 0xCC, 0xCF, 0xCF, 0xCF,	// ..0x4F
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0x50..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0x5F
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF,	// 0x60..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0x6F
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF,	// 0x70..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0x7F
	
	0xC0, 0xC3, 0xC3, 0xC3, 0xCC, 0xCF, 0xCF, 0xCF, // 0x80..
	0xCC, 0xCF, 0xCF, 0xCF, 0xCC, 0xCF, 0xCF, 0xCF,	// ..0x8F
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF,	// 0x90..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0x9F
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0xA0..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0xAF
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0xB0..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0xBF

	0xC0, 0xC3, 0xC3, 0xC3, 0xCC, 0xCF, 0xCF, 0xCF, // 0xC0..
	0xCC, 0xCF, 0xCF, 0xCF, 0xCC, 0xCF, 0xCF, 0xCF,	// ..0xCF
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0xD0..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0xDF
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0xE0..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF,	// ..0xEF
	
	0xF0, 0xF3, 0xF3, 0xF3, 0xFC, 0xFF, 0xFF, 0xFF, // 0xF0..
	0xFC, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF	// ..0xFF
};

static u8 posterize_4bpp_table[256] =
{
	0x00, 0x05, 0x05, 0x05, 0x05, 0x05, 0x0A, 0x0A,	// 0x00..
	0x0A, 0x0A, 0x0A, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,	// ..0x0F
	
	0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A,	// 0x10..
	0x5A, 0x5A, 0x5A, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F,	// ..0x1F
	
	0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A,	// 0x20..
	0x5A, 0x5A, 0x5A, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F,	// ..0x2F

	0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A,	// 0x30..
	0x5A, 0x5A, 0x5A, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F,	// ..0x3F
	
	0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A,	// 0x40..
	0x5A, 0x5A, 0x5A, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F,	// ..0x4F
	
	0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A,	// 0x50..
	0x5A, 0x5A, 0x5A, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F,	// ..0x5F
	
	0xA0, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xAA, 0xAA,	// 0x60..
	0xAA, 0xAA, 0xAA, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF,	// ..0x6F
	
	0xA0, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xAA, 0xAA,	// 0x70..
	0xAA, 0xAA, 0xAA, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF,	// ..0x7F
	
	0xA0, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xAA, 0xAA,	// 0x80..
	0xAA, 0xAA, 0xAA, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF,	// ..0x8F
	
	0xA0, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xAA, 0xAA,	// 0x90..
	0xAA, 0xAA, 0xAA, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF,	// ..0x9F
	
	0xA0, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xAA, 0xAA,	// 0xA0..
	0xAA, 0xAA, 0xAA, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF,	// ..0xAF
	
	0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA,	// 0xB0..
	0xFA, 0xFA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// ..0xBF
	
	0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA,	// 0xC0..
	0xFA, 0xFA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// ..0xCF

	0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA,	// 0xD0..
	0xFA, 0xFA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// ..0xDF

	0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA,	// 0xE0..
	0xFA, 0xFA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// ..0xEF

	0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA,	// 0xF0..
	0xFA, 0xFA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF	// ..0xFF
};

static int einkfb_display_fx		= fx_none;
static int last_display_fx 		= fx_none;
static int einkfb_mask_fx		= fx_none;

static int einkfb_fx_num_exclude_rects	= 0;
static rect_t einkfb_fx_exclude_rects[MAX_EXCLUDE_RECTS];
static int einkfb_fx_exclude_x		= 0;
static int einkfb_fx_exclude_y		= 0;

static u8* einkfb_stipple_table		= NULL;
static int einkfb_stipple_rowbytes	= 0;
static int einkfb_stipple_bytes		= 0;
static int einkfb_stipple_rows		= 0;

static u8* einkfb_mask_framebuffer	= NULL;
static int einkfb_mask_fb_rowbytes	= 0;
static u8* einkfb_mask_buffer		= NULL;
static int einkfb_mask_bufsize		= 0;
static int einkfb_mask_rowbytes		= 0;
static int einkfb_mask_x_start		= 0;
static int einkfb_mask_x		= 0;
static int einkfb_mask_y		= 0;

static int external_fx_update		= 0;

#if PRAGMAS
	#pragma mark -
	#pragma mark Common
	#pragma mark -
#endif

#define LOCAL_UPDATE()	((override_framebuffer || local_update_area) && !external_fx_update)

struct buffer_blit_t
{
	u8 *src, *dst;
};
typedef struct buffer_blit_t buffer_blit_t;

static void blit_buffer(int x, int y, int rowbytes, int bytes, void *data)
{
	buffer_blit_t *buffer_blit = (buffer_blit_t *)data;
	buffer_blit->dst[(rowbytes * y) + x] = buffer_blit->src[bytes];
}

static void update_display_area_buffer(update_area_t *update_area)
{
	// If we get here, the update_area has already been validated.	So, all we
	// need to do is blit things into the framebuffer at the right spot.
	//
	u32 x_start, x_end, area_xres,
		y_start, y_end, area_yres;

	u8	*fb_start, *area_start;
	
	buffer_blit_t buffer_blit;
		
	area_xres  = update_area->x2 - update_area->x1;
	area_yres  = update_area->y2 - update_area->y1;
	area_start = update_area->buffer;
	
	fb_start   = framebuffer;

	// Get the (x1, y1)...(x2, y2) offset into the framebuffer.
	//
	x_start = update_area->x1;
	x_end	= x_start + area_xres;
	
	y_start = update_area->y1;
	y_end	= y_start + area_yres;
	
	// Blit the area data into the framebuffer.
	//
	buffer_blit.src = area_start;
	buffer_blit.dst = fb_start;
	
	einkfb_blit(x_start, x_end, y_start, y_end, blit_buffer, (void *)&buffer_blit);
	memcpy_from_framebuffer_to_kernelbuffer();
}

static void update_display_area(fx_type which_fx, u8 *buffer, int xstart, int xend, int ystart, int yend, update_which to_screen)
{
	update_area_t update_area = { xstart, ystart, xend, yend, which_fx, buffer };
	
	if ( to_screen )
	{
		local_update_area = 1;
		
		EINKFB_IOCTL(FBIO_EINK_UPDATE_DISPLAY_AREA, (unsigned long)&update_area);
		local_update_area = 0;
	}
	else
		if ( buffer )
			update_display_area_buffer(&update_area);
}

static void update_display(fx_type update_mode)
{
	// To avoid contention, the shim does all of its work in an alternate buffer.  So,
	// we need to tell the eInk HAL to use this buffer instead.
	//
	override_framebuffer = kernelbuffer;
	
	EINKFB_IOCTL(FBIO_EINK_UPDATE_DISPLAY, (unsigned long)update_mode);
	
	// Done.
	//
	override_framebuffer = NULL;
}

static void buffer_copy(int which)
{
	u8 *src, *dst;

	switch ( which )
	{
		case from_screen_saver_to_kernelbuffer:
			src = screen_saver_buffer;
			dst = kernelbuffer;
		break;
		
		case from_picture_buffer_to_kernelbuffer:
			src = picture_buffer;
			dst = kernelbuffer;
		break;
		
		case from_picture_buffer_to_screensaver:
			src = picture_buffer;
			dst = screen_saver_buffer;
		break;
		
		case from_framebuffer_to_kernelbuffer:
			src = framebuffer;
			dst = kernelbuffer;
		break;
		
		case from_kernelbuffer_to_framebuffer:
			src = kernelbuffer;
			dst = framebuffer;
		break;
		
		default:
			src = dst = NULL;
		break;
	}
	
	if ( src && dst )
		EINKFB_MEMCPYK(dst, src, kernelbuffer_size);
}

static void clear_frame_buffer(void)
{
	einkfb_memset(framebuffer, EINKFB_WHITE, framebuffer_size);
}

static void clear_kernel_buffer(void)
{
//	einkfb_memset(kernelbuffer, EINKFB_WHITE, kernelbuffer_size);
}

static void clear_picture_buffer(u8 clear)
{
//	einkfb_memset(picture_buffer, clear, picture_buffer_size);
}

void clear_screen(fx_type update_mode)
{
//	clear_kernel_buffer();
//	update_display(update_mode);
	
//	memcpy_from_kernelbuffer_to_framebuffer();
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Proc/Sysf
	#pragma mark -
#endif

// /proc/eink_fb/screen_saver
//
static int read_from_screen_saver_buffer(unsigned long addr, unsigned char *data, int count)
{
	int result = 0;
	
	if ( screen_saver_buffer && data && IN_RANGE((addr + count), 0, screen_saver_buffer_size) )
	{
		EINKFB_MEMCPYK(data, &screen_saver_buffer[addr], count);
		result = 1;
	}
	
	return ( result );
}

static int write_to_screen_saver_buffer(unsigned long start, unsigned char *data, unsigned long len)
{
	int result = 0;
	
	if ( 0 == start )
		valid_screen_saver_buf = screen_saver_invalid;
	
	if ( screen_saver_buffer && data && IN_RANGE((start + len), 0, screen_saver_buffer_size) )
	{
		EINKFB_MEMCPYK(&screen_saver_buffer[start], data, len);
		result = 1;
	}
	
	return ( result );
}

static int read_einkfb_screen_saver(char *page, unsigned long off, int count)
{
	return ( einkfb_read_which(page, off, count, read_from_screen_saver_buffer, screen_saver_buffer_size) );
}

static int write_einkfb_screen_saver(char *buf, unsigned long count, int unused)
{
	return ( einkfb_write_which(buf, count, write_to_screen_saver_buffer, screen_saver_buffer_size) );
}

static int einkfb_screen_saver_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return ( EINKFB_PROC_SYSFS_RW_LOCK(page, start, off, count, eof, screen_saver_buffer_size, read_einkfb_screen_saver) );
}

static int einkfb_screen_saver_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	return ( EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_einkfb_screen_saver) );
}

// /sys/devices/platform/eink_fb0/valid_screen_saver_buf
//
static ssize_t show_valid_screen_saver_buf(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d %d\n", valid_screen_saver_buf, screen_saver_last) );
}

static ssize_t store_valid_screen_saver_buf(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, valid, last;

	switch ( sscanf(buf, "%d %d", &valid, &last) )
	{
		case 2:	screen_saver_last = last;
		
		case 1: valid_screen_saver_buf = min((unsigned long)valid, screen_saver_buffer_size);

			if ( screen_saver_valid == valid_screen_saver_buf )
				set_next_user_screen_saver();

			result = count;
		break;
	}

	return ( result );
}

// /sys/devices/platform/eink_fb0/splash_screen_up
//
static ssize_t show_splash_screen_up(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", splash_screen_up) );
}

// /sys/devices/platform/eink_fb0/send_fake_rotate
//
static ssize_t show_send_fake_rotate(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", send_fake_rotate) );
}

static void report_event(char *value)
{
	char *envp[] = {value, "DRIVER=accelerometer", NULL};
	struct einkfb_info info; einkfb_get_info(&info);

	einkfb_debug("sending uevent %s\n", value);

	if ( kobject_uevent_env(&info.events->this_device->kobj, KOBJ_CHANGE, envp) )
		einkfb_debug("failed to send uevent\n");
}

static ssize_t store_send_fake_rotate(FB_DSTOR_PARAMS)
{		
	int result = count;
	
	if ( sscanf(buf, "%d", &send_fake_rotate) )
	{
		switch ( send_fake_rotate )
		{
			case orientation_portrait:
				report_event(ORIENTATION_STRING_UP);
			break;
			
			case orientation_portrait_upside_down:
				report_event(ORIENTATION_STRING_DOWN);
			break;
			
			case orientation_landscape:
				report_event(ORIENTATION_STRING_RIGHT);
			break;
			
			case orientation_landscape_upside_down:
				report_event(ORIENTATION_STRING_LEFT);
			break;
			
			default:
				result = -EINVAL;
			break;
		}
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb0/use_fake_pnlcd
//
static ssize_t show_use_fake_pnlcd(FB_DSHOW_PARAMS)
{
	pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();
	
	return ( sprintf(buf, "%d %d %d\n", use_fake_pnlcd, (fake_pnlcd_x ? 1 : 0), pnlcd_flags->hide_real) );
}

static ssize_t store_use_fake_pnlcd(FB_DSTOR_PARAMS)
{
	char clear_segments[PN_LCD_SEGMENT_COUNT] = { SEGMENT_OFF };
	int result = -EINVAL, fake_pnlcd = 0, hide_pnlcd = 0, x = 0;
	pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();

	switch ( sscanf(buf, "%d %d %d", &fake_pnlcd, &x, &hide_pnlcd) )
	{
		case 3:
			// Stop any pending animation and clear it if we're going to be
			// hiding the PNLCD.
			//
			if ( FRAMEWORK_OR_DIAGS_RUNNING() && hide_pnlcd )
			{
				pnlcd_animation_t pnlcd_animation;
				
				pnlcd_animation.cmd = stop_animation;
				pnlcd_animation.arg = 0;

				PNLCD_SYS_IOCTL(PNLCD_ANIMATE_IOCTL, &pnlcd_animation);
				PNLCD_SYS_IOCTL(PNLCD_CLEAR_ALL_IOCTL, 0);
			}
			
			pnlcd_flags->hide_real = hide_pnlcd ? 1 : 0;

		case 2:
			// Clear out the old fake PNLCD state.
			//
			EINKFB_IOCTL(FBIO_EINK_FAKE_PNLCD, (unsigned long)clear_segments);

			// Set fake_pnlcd_xres & fake_pnlcd_x based on x.
			//
			set_fake_pnlcd_x_values(x);
		
		case 1:
			// Update to new state (both fake and real).
			//
			use_fake_pnlcd = fake_pnlcd ? 1 : 0;
			pnlcd_flags->enable_fake = FAKE_PNLCD() ? 1 : 0;
		
			if ( FRAMEWORK_OR_DIAGS_RUNNING() )
			{
				PNLCD_SYS_IOCTL(PNLCD_SET_SEG_STATE, NULL);
				EINKFB_IOCTL(FBIO_EINK_FAKE_PNLCD, 0);
			}

			// Done.
			//
			result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb0/running_diags
//
static ssize_t show_running_diags(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", running_diags) );
}

static ssize_t store_running_diags(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, diags;
	
	if ( sscanf(buf, "%d", &diags) )
	{
		running_diags = diags ? 1 : 0;
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb0/progressbar
//
static ssize_t show_progressbar(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", progressbar) );
}

static ssize_t store_progressbar(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, progress;
	
	if ( sscanf(buf, "%d", &progress) )
	{
		EINKFB_IOCTL(FBIO_EINK_PROGRESSBAR, progress);
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb0/eink_debug
//
static ssize_t show_eink_debug(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", eink_debug) );
}

static ssize_t store_eink_debug(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, debug;
	
	if ( sscanf(buf, "%d", &debug) )
	{
		eink_debug = debug ? 1 : 0;
		
		if ( eink_debug )
			einkfb_logging = einkfb_debugging_default;
		else
			einkfb_logging = einkfb_logging_default;
		
		result = count;
	}
	
	return ( result );
}

#if PRAGMAS
	#pragma mark -
	#pragma mark FX
	#pragma mark -
#endif

static bool einkfb_fx_exclude_rects_acceptable(int num_exclude_rects, rect_t *exclude_rects)
{
	bool acceptable = MAX_EXCLUDE_RECTS >= num_exclude_rects;
	int i;
	
	for ( i = 0; (i < num_exclude_rects) && acceptable; i++ )
	{
		acceptable = einkfb_bounds_are_acceptable(
				exclude_rects[i].x1,
				exclude_rects[i].x2 - exclude_rects[i].x1,

				exclude_rects[i].y1,
				exclude_rects[i].y2 - exclude_rects[i].y1);
		
		// We really don't care about byte alignment here.
		//
		if ( !acceptable && NOT_BYTE_ALIGNED() )
			acceptable = true;
	}
	
	return ( acceptable );
}

static void einkfb_set_fx_exclude_rects(int num_exclude_rects, rect_t *exclude_rects)
{
	int i;
	
	einkfb_fx_num_exclude_rects = num_exclude_rects;
	
	for ( i = 0; i < einkfb_fx_num_exclude_rects; i++ )
	{
		einkfb_fx_exclude_rects[i].x1 = BPP_SIZE(exclude_rects[i].x1, framebuffer_bpp);
		einkfb_fx_exclude_rects[i].y1 = exclude_rects[i].y1;
		einkfb_fx_exclude_rects[i].x2 = BPP_SIZE(exclude_rects[i].x2, framebuffer_bpp);
		einkfb_fx_exclude_rects[i].y2 = exclude_rects[i].y2;
	}
}

static void einkfb_clear_fx_exclude_rects(void)
{
	int i;
	
	for ( i = 0; i < einkfb_fx_num_exclude_rects; i++ )
	{
		einkfb_fx_exclude_rects[i].x1 = 0;
		einkfb_fx_exclude_rects[i].y1 = 0;
		einkfb_fx_exclude_rects[i].x2 = 0;
		einkfb_fx_exclude_rects[i].y2 = 0;
	}
	
	einkfb_fx_num_exclude_rects = 0;
}

static bool einkfb_fx_in_exclude_rects(void)
{
	bool in_exclude_rects = false;
  
	if ( einkfb_fx_num_exclude_rects )
	{
		int i;
		
		for ( i = 0; (i < einkfb_fx_num_exclude_rects) && !in_exclude_rects; i++ )
		{
			if (	IN_RANGE(einkfb_fx_exclude_x, einkfb_fx_exclude_rects[i].x1, einkfb_fx_exclude_rects[i].x2) &&
				IN_RANGE(einkfb_fx_exclude_y, einkfb_fx_exclude_rects[i].y1, einkfb_fx_exclude_rects[i].y2) )
				
				in_exclude_rects = true;
		}

	}

	return ( in_exclude_rects );
}

static void einkfb_set_display_fx(int which_fx)
{
	// Default to no FX.
	//
	einkfb_display_fx = fx_none;
	
	switch ( which_fx )
	{
		case fx_stipple_posterize_dark:
		case fx_stipple_posterize_lite:
		
		case fx_stipple_lighten_dark:
		case fx_stipple_lighten_lite:

		case fx_buf_is_mask:
		case fx_mask:
			
			// Only do FX at 2bpp and 4bpp.
			//
			switch ( framebuffer_bpp )
			{
				case EINKFB_2BPP:
				case EINKFB_4BPP:
					goto fx_common;
				break;
			}
		break;
		
		fx_common:
			einkfb_display_fx = which_fx;
		break;
	}
}

static void einkfb_set_mask_fx(int which_fx)
{
	// Default to no FX.
	//
	einkfb_mask_fx = fx_none;
	
	switch ( which_fx )
	{
		case fx_stipple_posterize_dark:
		case fx_stipple_posterize_lite:

		case fx_stipple_lighten_dark:
		case fx_stipple_lighten_lite:
			
			// Only do FX at 2bpp and 4bpp.
			//
			switch ( framebuffer_bpp )
			{
				case EINKFB_2BPP:
				case EINKFB_4BPP:
					goto fx_common;
				break;
			}
		break;
		
		fx_common:
			einkfb_mask_fx = which_fx;
		break;
	}
}

static u8 einkfb_posterize(u8 data)
{
	int index = (einkfb_posterize_rows & 1) ? posterize_odd_row_mask : posterize_even_row_mask;
	u8  mask  = einkfb_posterize_table[index++], mask_bpp = einkfb_posterize_table[posterize_bpp];

	if ( 0 == (++einkfb_posterize_bytes % einkfb_posterize_rowbytes) )
		einkfb_posterize_rows++;
		
	return ( POSTERIZE_TABLE_MASK(mask_bpp, mask, data ) );
}

static u8 einkfb_stipple(u8 data)
{
	int index = (einkfb_stipple_rows & 1) ? stipple_odd_row_mask : stipple_even_row_mask;
	u8  mask  = einkfb_stipple_table[index++], mask_type = einkfb_stipple_table[index];

	if ( 0 == (++einkfb_stipple_bytes % einkfb_stipple_rowbytes) )
		einkfb_stipple_rows++;

	return ( STIPPLE_TABLE_MASK(mask_type, mask, data) );
}

static u8 einkfb_mask(u8 data, int i)
{
	register u8 mask, byte;

	byte = einkfb_mask_framebuffer[(einkfb_mask_fb_rowbytes * einkfb_mask_y) + einkfb_mask_x++];
	byte = einkfb_apply_fx_which(byte, 0, einkfb_mask_fx);

	mask = einkfb_mask_buffer[einkfb_mask_bufsize + i];
  
	if ( 0 == ((einkfb_mask_x - einkfb_mask_x_start) % einkfb_mask_rowbytes) )
	{
		einkfb_mask_x = einkfb_mask_x_start;
		einkfb_mask_y++;
	}

	return ( (data & mask) | (byte & ~mask) );
}

static void einkfb_begin_fx(u8 *buffer, int bufsize, int rowbytes, int x_start, int y_start)
{
	int which_fx = einkfb_display_fx;

	reapply_fx:
	switch ( which_fx )
	{
		case fx_stipple_posterize_dark:
			einkfb_posterize_table		= (EINKFB_2BPP == framebuffer_bpp)	?
							posterize_dark_2bpp_table	:
							posterize_dark_4bpp_table;
		goto posterize_common;
		
		case fx_stipple_posterize_lite:
			einkfb_posterize_table		= (EINKFB_2BPP == framebuffer_bpp)	?
							posterize_lite_2bpp_table	:
							posterize_lite_4bpp_table;
		/* goto posterize_common; */

		posterize_common:
			einkfb_posterize_bytes		= 0;
			einkfb_posterize_rows		= 0;
			einkfb_posterize_rowbytes	= rowbytes;
		break;
		
		case fx_stipple_lighten_dark:
			einkfb_stipple_table		= (EINKFB_2BPP == framebuffer_bpp)	?
							stipple_lighten_dark_gray_2bpp_table	:
							stipple_lighten_dark_gray_4bpp_table;
		goto stipple_common;
		
		case fx_stipple_lighten_lite:
			einkfb_stipple_table		= (EINKFB_2BPP == framebuffer_bpp)	?
							stipple_lighten_lite_gray_2bpp_table	:
							stipple_lighten_lite_gray_4bpp_table;
		/* goto stipple_common; */

		stipple_common:
			einkfb_stipple_bytes		= 0;
			einkfb_stipple_rows		= 0;
			einkfb_stipple_rowbytes		= rowbytes;
		break;

		case fx_buf_is_mask:
		case fx_mask:
			einkfb_mask_framebuffer		= framebuffer;
			einkfb_mask_fb_rowbytes		= BPP_SIZE(framebuffer_xres, framebuffer_bpp);
			einkfb_mask_buffer		= buffer;
			einkfb_mask_bufsize		= (fx_mask == which_fx) ? bufsize : 0;
			einkfb_mask_rowbytes		= rowbytes;
			einkfb_mask_x_start		= x_start;
			einkfb_mask_x			= x_start;
			einkfb_mask_y			= y_start;
			which_fx			= einkfb_mask_fx;
		goto reapply_fx;
	}

	if ( einkfb_fx_num_exclude_rects )
		einkfb_fx_exclude_x = einkfb_fx_exclude_y = 0;
}

static void einkfb_end_fx(void)
{
	int which_fx = einkfb_display_fx;
  
	reapply_fx:
	switch ( which_fx )
	{
		case fx_stipple_posterize_dark:
		case fx_stipple_posterize_lite:
			einkfb_posterize_bytes		= 0;
			einkfb_posterize_rows		= 0;
			einkfb_posterize_rowbytes	= 0;
			einkfb_posterize_table		= NULL;
		break;
		
		case fx_stipple_lighten_dark:
		case fx_stipple_lighten_lite:
			einkfb_stipple_bytes		= 0;
			einkfb_stipple_rows		= 0;
			einkfb_stipple_rowbytes		= 0;
			einkfb_stipple_table		= NULL;
		break;

		case fx_buf_is_mask:
		case fx_mask:
			einkfb_mask_framebuffer		= NULL;
			einkfb_mask_fb_rowbytes		= 0;
			einkfb_mask_buffer		= NULL;
			einkfb_mask_bufsize		= 0;
			einkfb_mask_rowbytes		= 0;
			einkfb_mask_x_start		= 0;
			einkfb_mask_x			= 0;
			einkfb_mask_y			= 0;
			which_fx			= einkfb_mask_fx;
		goto reapply_fx;
	}

	if ( einkfb_fx_num_exclude_rects )
		einkfb_fx_exclude_x = einkfb_fx_exclude_y = 0;
}

static u8 einkfb_apply_fx_which(u8 data, int i, int which_fx)
{
	bool exclude = false;
	u8 fx_data = data;

	if ( einkfb_fx_num_exclude_rects )
	{
		if ( einkfb_fx_in_exclude_rects() )
			exclude = true;

		if ( 0 == (++einkfb_fx_exclude_x % BPP_SIZE(framebuffer_xres, framebuffer_bpp)) )
		{
			einkfb_fx_exclude_x = 0;
			einkfb_fx_exclude_y++;
		}
		
		if ( 0 == (einkfb_fx_exclude_y % framebuffer_yres) )
			einkfb_fx_exclude_y = 0;
	}

	switch ( which_fx )
	{
		case fx_stipple_posterize_dark:
		case fx_stipple_posterize_lite:
			fx_data = einkfb_posterize(data);
		break;
		
		case fx_stipple_lighten_dark:
		case fx_stipple_lighten_lite:
			fx_data = einkfb_stipple(data);
		break;

		case fx_buf_is_mask:
		case fx_mask:
			fx_data = einkfb_mask(data, i);
		break;
	}

	return ( exclude ? data : fx_data );
}

static void einkfb_buffer_fx(u8 *buffer, int buffer_size, int rowbytes, int x_start, int y_start)
{
	int i;

	einkfb_begin_fx(buffer, buffer_size, rowbytes, x_start, y_start);
		
	for ( i = 0; i < buffer_size; i++ )
	{
		buffer[i] = einkfb_apply_fx_which(buffer[i], i, einkfb_display_fx);
		EINKFB_SCHEDULE_BLIT(i+1);
	}

	einkfb_end_fx();
}

static u8 einkfb_apply_fx(u8 data, int i)
{
	return ( einkfb_apply_fx_which(data, i, einkfb_display_fx) );
}

static void einkfb_update_display_fx(fx_t *fx)
{
	int rowbytes = BPP_SIZE(framebuffer_xres, framebuffer_bpp);
	
	// If debugging is enabled, output the passed-in FX data.
	//
	einkfb_debug("update_mode  = %d\n", fx->update_mode);
	einkfb_debug("which_fx     = %d\n", fx->which_fx);

	if ( EINKFB_DEBUG() && fx->num_exclude_rects )
	{
		int i;
		
		einkfb_debug("num rects    = %d\n", fx->num_exclude_rects);
		
		for ( i = 0; i < fx->num_exclude_rects; i++ )
		{
			einkfb_debug("rect[%d].x1   = %d\n", i, fx->exclude_rects[i].x1);
			einkfb_debug("rect[%d].y1   = %d\n", i, fx->exclude_rects[i].y1);
			einkfb_debug("rect[%d].x2   = %d\n", i, fx->exclude_rects[i].x2);
			einkfb_debug("rect[%d].y2   = %d\n", i, fx->exclude_rects[i].y2);
		}
	}
	
	// Set up for passed-in FX.
	//
	if ( fx->exclude_rects && einkfb_fx_exclude_rects_acceptable(fx->num_exclude_rects, fx->exclude_rects) )
		einkfb_set_fx_exclude_rects(fx->num_exclude_rects, fx->exclude_rects);
	
	einkfb_set_display_fx(fx->which_fx);
	last_display_fx = fx->which_fx;

	// Set up to get the FX applied to the display.
	//
	einkfb_begin_fx(framebuffer, framebuffer_size, rowbytes, 0, 0);
	set_fb_apply_fx(einkfb_apply_fx);
	
	// Update the display with the FX.
	//
	EINKFB_IOCTL(FBIO_EINK_UPDATE_DISPLAY, (unsigned long)fx->update_mode);
	
	// Undo the FX.
	//
	set_fb_apply_fx(NULL);
	einkfb_end_fx();

	einkfb_set_display_fx(fx_none);
	einkfb_clear_fx_exclude_rects();
}

#define EINKFB_UPDATE_DISPLAY_FX_AREA_BEGIN_ADJUST_Y(u) einkfb_update_display_fx_area_begin(u, true)
#define EINKFB_UPDATE_DISPLAY_FX_AREA_BEGIN(u)		einkfb_update_display_fx_area_begin(u, false)

static void einkfb_update_display_fx_area_begin(update_area_t *update_area, bool adjust_y)
{
	if ( UPDATE_AREA_FX(update_area->which_fx) )
	{
		int	xstart, xend, ystart, yend, xres, yres,
			rowbytes, buffer_size;
		
		// Our convention is that the fx masks are located immediately following the
		// source picture, and are, otherwise, exactly the same size as the source
		// picture.  Thus, their xres is the same but their yres is double (so, we
		// half it here).
		//
		if ( adjust_y && (fx_mask == update_area->which_fx) )
			update_area->y2 = update_area->y1 + (update_area->y2 - update_area->y1)/2;
		
		xstart	= update_area->x1, xend = update_area->x2;
		ystart	= update_area->y1, yend = update_area->y2;

		xres	= xend - xstart,
		yres	= yend - ystart,

		rowbytes	= BPP_SIZE((xend - xstart), framebuffer_bpp),
		buffer_size	= BPP_SIZE((xres * yres), framebuffer_bpp);
		
		// If debugging is enabled, output the passed-in FX data.
		//
		einkfb_debug("which_fx = %d\n", update_area->which_fx);
		einkfb_debug("mask_fx  = %d\n", einkfb_mask_fx);
		einkfb_debug("x1       = %d\n", xstart);
		einkfb_debug("y1       = %d\n", ystart);
		einkfb_debug("x2       = %d\n", xend);
		einkfb_debug("y2       = %d\n", yend);
		
		// Set up for passed-in FX.
		//
		einkfb_set_display_fx(update_area->which_fx);
		
		// Perform FX on the buffer.
		//
		einkfb_buffer_fx(update_area->buffer, buffer_size, rowbytes,
			BPP_SIZE(xstart, framebuffer_bpp), ystart);
	}
}

static void einkfb_update_display_fx_area_end(update_area_t *update_area)
{
	if ( UPDATE_AREA_FX(update_area->which_fx) )
	{
		// Undo FX.
		//
		einkfb_set_display_fx(fx_none);
	}
}

static void einkfb_update_display_fx_area(fx_type which_fx, u8 *buffer, int xstart, int xend, int ystart, int yend, update_which to_screen)
{
	update_area_t update_area = { xstart, ystart, xend, yend, which_fx, buffer };
	fx_type saved_fx = which_fx;
	
	// Apply FX to the buffer.
	//
	EINKFB_UPDATE_DISPLAY_FX_AREA_BEGIN(&update_area);
	
	// Move the contents of the FX'd buffer to the screen.
	//
	update_area.which_fx = UPDATE_AREA_MODE(saved_fx);
	update_display_area(update_area.which_fx, buffer, xstart, xend, ystart, yend, to_screen);
	update_area.which_fx = saved_fx;
	
	// End FX.
	//
	einkfb_update_display_fx_area_end(&update_area);
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Fake PNLCD
	#pragma mark -
#endif

static void set_fake_pnlcd_x_values(int x)
{
	// For now, if x is non-zero, put the fake PNLCD on the right and make it full size.
	// Otherwise, put it on the left and make it half size.
	//
	if ( x )
	{
		fake_pnlcd_xres	= FAKE_PNLCD_XRES_MAX;
		fake_pnlcd_x	= framebuffer_xres - fake_pnlcd_xres;
	}
	else
	{
		fake_pnlcd_xres = FAKE_PNLCD_XRES_MAX/2;
		fake_pnlcd_x	= 0;
	}
}

static bool get_pnlcd_segments(char *segments)
{
	bool segments_enabled_status = false;
	
	if ( USE_FAKE_PNLCD(segments) )
	{
		einkfb_memset(segments, SEGMENT_OFF, PN_LCD_SEGMENT_COUNT);

		if ( !PNLCD_EVENT_PENDING() )
		{
			int i;
			
			PNLCD_SYS_IOCTL(PNLCD_GET_SEG_STATE, segments);
			
			for ( i = 0; (i < PN_LCD_SEGMENT_COUNT) && (false == segments_enabled_status); i++ )
				if ( SEGMENT_ON == segments[i] )
					segments_enabled_status = true; 
		}
	}
	
	return ( segments_enabled_status );
}

static void update_fake_pnlcd_segment(int seg_num, int seg_state)
{
	if ( USE_FAKE_PNLCD(IN_RANGE(seg_num, FIRST_PNLCD_SEGMENT, LAST_PNLCD_SEGMENT))	 )
	{
		u32	x, x_start, x_end, rowbytes, xres = fake_pnlcd_xres,
			y, y_start, y_end, bytes;
			
		u8	*fb_base = fake_pnlcd_buffer;
		
		// Get (x, y) starting position, based on segment number.
		//
		x_start	 = FAKE_PNLCD_X_START(seg_num);
		y_start	 = FAKE_PNLCD_Y_START(seg_num);
		
		x_end	 = FAKE_PNLCD_X_END(x_start);
		y_end	 = FAKE_PNLCD_Y_END(y_start);
		
		// Make bpp-related adjustments.
		//
		x_start	 = BPP_SIZE(x_start, framebuffer_bpp);
		x_end	 = BPP_SIZE(x_end,   framebuffer_bpp);
		rowbytes = BPP_SIZE(xres,    framebuffer_bpp);
		
		// Draw segment into buffer.
		//
		for ( bytes = 0, y = y_start; y < y_end; y++ )
		{
			for ( x = x_start; x < x_end; x++)
			{
				fb_base[(rowbytes * y) + x] = (SEGMENT_ON == seg_state) ? EINKFB_BLACK : EINKFB_WHITE;
				EINKFB_SCHEDULE_BLIT(++bytes);
			}
		}
	}
}

static unsigned long fake_pnlcd_start_draw, fake_pnlcd_end_draw;

static void update_fake_pnlcd(char *segments)
{
	pnlcd_flags_t *pnlcd_flags = get_pnlcd_flags();
	
	if ( USE_FAKE_PNLCD(!pnlcd_flags->updating) )
	{
		int	x_start = fake_pnlcd_x,
			x_end	= x_start + fake_pnlcd_xres,
			y_start,
			y_end,
			
			buffer_size = fake_pnlcd_buffer_size,
			
			min_seg = 0,
			max_seg = 0,
			
			i;
			
		u8	*buffer = fake_pnlcd_buffer;

		// Update each of the PNLCD segments in the fake PNLCD segment buffer with its
		// latest value, remembering the segment-on span, and clear the entire buffer
		// to white first.
		//
		einkfb_memset(buffer, EINKFB_WHITE, buffer_size);
		
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
			y_end	= FAKE_PNLCD_Y_START(max(max_seg, fake_pnlcd_max_seg));
		}
		else
		{
			y_start = FAKE_PNLCD_Y_START(min_seg), 
			y_end	= FAKE_PNLCD_Y_START(max_seg);
		}

		y_end = FAKE_PNLCD_Y_END(y_end);
			
		buffer = &fake_pnlcd_buffer[BPP_SIZE(fake_pnlcd_xres, framebuffer_bpp) * y_start];
		buffer_size = BPP_SIZE((fake_pnlcd_xres * (y_end - y_start)), framebuffer_bpp);

		fake_pnlcd_min_seg = min_seg;
		fake_pnlcd_max_seg = max_seg;
		
		// Now, update the screen with the PNLCD segment buffer, using the segment buffer
		// itself as a mask; and, if there is one, using the last-set display FX.
		//
		einkfb_set_mask_fx(last_display_fx);
		
		pnlcd_flags->updating = 1;
		fake_pnlcd_start_draw = jiffies;
		
		einkfb_update_display_fx_area(fx_buf_is_mask, buffer, x_start, x_end, y_start, y_end, update_screen);
		fake_pnlcd_valid = 1;

		fake_pnlcd_end_draw = jiffies;
		pnlcd_flags->updating = 0;

		einkfb_set_mask_fx(fx_none);
		
		// Report drawing time.
		//
		einkfb_debug("bytes sent = %d, time = %ums\n", buffer_size, 
			jiffies_to_msecs(fake_pnlcd_end_draw - fake_pnlcd_start_draw));
	}
}

static void fake_pnlcd_test(void)
{
	if ( FAKE_PNLCD() )
	{
		unsigned long draw_starts[(PN_LCD_SEGMENT_COUNT/4) + 1];
		char segments[PN_LCD_SEGMENT_COUNT];
		int i, j;
		
		// Clear the current segments.
		//
		einkfb_memset(segments, SEGMENT_OFF, PN_LCD_SEGMENT_COUNT);
		EINKFB_IOCTL(FBIO_EINK_FAKE_PNLCD, (unsigned long)segments);
		
		// Draw and erase fifty 4-unit segments, gathering each of their start times as
		// we go.
		//
		for ( i = 0; i < PN_LCD_SEGMENT_COUNT; i+= 4 )
		{
			for ( j = i; j < (i + 4); j++ )
				segments[j] = SEGMENT_ON;
			
			EINKFB_IOCTL(FBIO_EINK_FAKE_PNLCD, (unsigned long)segments);
			draw_starts[i/4] = fake_pnlcd_start_draw;
			
			for ( j = i; j < (i + 4); j++ )
				segments[j] = SEGMENT_OFF;
		}
		
		draw_starts[PN_LCD_SEGMENT_COUNT/4] = fake_pnlcd_start_draw;
		
		// Restore the original segments.
		//
		get_pnlcd_segments(segments);
		EINKFB_IOCTL(FBIO_EINK_FAKE_PNLCD, (unsigned long)segments);

		// Print out the screen-to-screen start-time deltas for the 4-unit segments we drew.
		//
		EINKFB_PRINT("screen-to-screen drawing times:%s", "\n");
		
		for ( i = 0; i < PN_LCD_SEGMENT_COUNT/4; i++ )
		{
			EINKFB_PRINT("  segments 0x%02X-0x%02X time = %4ums\n", (i*4), (i*4)+3,
				jiffies_to_msecs(draw_starts[i+1] - draw_starts[i]));
		}
	}
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Screen Saver
	#pragma mark -
#endif

static void save_the_last_screen_saver(void)
{
	mm_segment_t saved_fs = get_fs();
	int screen_saver_last_file;

	set_fs(get_ds());
	
//	screen_saver_last_file = sys_open(screen_saver_path_last, (O_WRONLY | O_TRUNC), 0);
	
	if ( 0 <= screen_saver_last_file )
	{
		char buf[128];
		int len = sprintf(buf, "%d", screen_saver_last);
		
		if ( 0 < len )
	//		sys_write(screen_saver_last_file, buf, len);
		
		sys_close(screen_saver_last_file);
	}

	set_fs(saved_fs);
}

static int load_the_screen_saver(char *screen_saver_path)
{
	int screen_saver_file, result = 0;
	mm_segment_t saved_fs = get_fs();

	set_fs(get_ds());
	
	//screen_saver_file = sys_open(screen_saver_path, O_RDONLY, 0);
	
	if ( 0 <= screen_saver_file )
	{
		//int len = sys_read(screen_saver_file, screen_saver_buffer, screen_saver_buffer_size);
		int len = 0;
		if ( 0 <= len )
		{
			valid_screen_saver_buf = (screen_saver_buffer_size == len) ? screen_saver_valid : len;
			result = 1;
		}

		sys_close(screen_saver_file);
	}
	
	set_fs(saved_fs);
	
	return ( result );
}

#define EXISTS_SCREEN_SAVER_PATH_CREATE(p)	exists_screen_saver_path(p, 1)
#define EXISTS_SCREEN_SAVER_PATH(p)		exists_screen_saver_path(p, 0)

static int exists_screen_saver_path(char *screen_saver_path, int create)
{
	int	screen_saver_file, flags = O_RDONLY | (create ? O_CREAT : 0),
		result = 0;
	mm_segment_t saved_fs = get_fs();

	set_fs(get_ds());
	
	//screen_saver_file = sys_open(screen_saver_path, flags, 0);
	
	if ( 0 <= screen_saver_file )
	{
		sys_close(screen_saver_file);
		result = 1;
	}
	
	set_fs(saved_fs);
	
	return ( result );
}

static void set_screen_saver_user_path(void)
{
	strcpy(screen_saver_path_template, SCREEN_SAVER_PATH_USER);
	strcat(screen_saver_path_template, SCREEN_SAVER_PATH_TEMPLATE);
	
	strcpy(screen_saver_path_last, SCREEN_SAVER_PATH_USER);
	strcat(screen_saver_path_last, SCREEN_SAVER_PATH_LAST);
}

static void set_screen_saver_sys_path(void)
{
	strcpy(screen_saver_path_template, SCREEN_SAVER_PATH_SYS_RO);
	strcat(screen_saver_path_template, SCREEN_SAVER_PATH_TEMPLATE);
	
	strcpy(screen_saver_path_last, SCREEN_SAVER_PATH_SYS_RW);
	strcat(screen_saver_path_last, SCREEN_SAVER_PATH_LAST);
}

static void get_the_next_screen_saver(void)
{
	char screen_saver_path[SCREEN_SAVER_PATH_SIZE];
	
	// Generate the path for the next screen saver we're supposed to load, trying the user
	// path first and then falling back to the system path.
	//
	set_screen_saver_user_path();
	
	if ( !EXISTS_SCREEN_SAVER_PATH(screen_saver_path_last) )
		set_screen_saver_sys_path();
	
	sprintf(screen_saver_path, screen_saver_path_template, ++screen_saver_last);
	
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

static void save_user_screen_saver(char *screen_saver_user_path)
{
	mm_segment_t saved_fs = get_fs();
	int screen_saver_file;

	set_fs(get_ds());
	
	//screen_saver_file = sys_open(screen_saver_user_path, (O_WRONLY | O_CREAT), 0);
	
	if ( 0 <= screen_saver_file )
	{
		int len = compress_picture((int)screen_saver_buffer_size, screen_saver_buffer);
	
		if ( 0 < len )
		{
			//sys_write(screen_saver_file, picture_buffer, len);
			
			memcpy_from_picture_buffer_to_screensaver();
			valid_screen_saver_buf = len;
		}
		
		sys_close(screen_saver_file);
	}
	
	set_fs(saved_fs);
}

static void set_next_user_screen_saver(void)
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

static void wake_screen_saver_thread(void)
{
	if ( screen_saver_buffer )
		complete(&screen_saver_thread_complete);
}

static void exit_screen_saver_thread(void)
{
	if ( screen_saver_buffer )
	{
		screen_saver_thread_exit = 1;
		wake_screen_saver_thread();
	}
}

static void screen_saver_thread_body(void)
{
	if ( get_next_screen_saver )
	{
		get_the_next_screen_saver();
		get_next_screen_saver = 0;
	}
	
	wait_for_completion_interruptible(&screen_saver_thread_complete);
}

static int screen_saver_thread(void *data)
{
	int  *exit_thread = (int *)data;
	bool thread_active = true;
	
	THREAD_HEAD(SCREEN_SAVER_THREAD_NAME);
	
	while ( thread_active )
	{
		TRY_TO_FREEZE();
		
		if ( !THREAD_SHOULD_STOP() )
		{
			THREAD_BODY(*exit_thread, screen_saver_thread_body);
		}
		else
			thread_active = false;
	}
	
	THREAD_TAIL(screen_saver_thread_exited);
}

static void start_screen_saver_thread(void)
{
	if ( screen_saver_buffer )
		THREAD_START(screen_saver_id, &screen_saver_thread_exit, screen_saver_thread,
			SCREEN_SAVER_THREAD_NAME);
}

static void stop_screen_saver_thread(void)
{
	if ( screen_saver_buffer )
		THREAD_STOP(screen_saver_id, exit_screen_saver_thread, screen_saver_thread_exited);
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Splash Screen
	#pragma mark -
#endif

void splash_screen_activity(int which_activity, splash_screen_type which_screen)
{
	if ( FRAMEWORK_OR_DIAGS_RUNNING() )
	{
		int	dont_ack_power_power_manager = 0,
			screen_saver_thread_wake = 0,
			ack_power_manager = 0,
			dont_delay = 0;
		
		pnlcd_blocking_cmds pnlcd_blocking_cmd;
		pnlcd_animation_t pnlcd_animation;

		switch ( which_screen )
		{
			// Block/unblock PNLCD activity; and, on entry, ACK the Power Manager.
			//
			case splash_screen_powering_off:
			case splash_screen_powering_off_wireless:
			case splash_screen_sleep:
			case splash_screen_power_off_clear_screen:
			case splash_screen_screen_saver_picture:
			return;
/*				if ( splash_screen_activity_begin == which_activity )
				{
					pnlcd_blocking_cmd = start_block;
					ack_power_manager = 1;
				}
				else
					pnlcd_blocking_cmd = stop_block;
				
				PNLCD_SYS_IOCTL(PNLCD_CLEAR_ALL_IOCTL, 0);
			goto entry_exit_common;
*/			
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
				if ( get_next_screen_saver )
					screen_saver_thread_wake = 1;
				
			exit_common:
			case splash_screen_powering_on:
			case splash_screen_powering_on_wireless:
				if ( splash_screen_activity_begin == which_activity )
				{
					PNLCD_SYS_IOCTL(PNLCD_BLOCK_IOCTL, stop_block);
					PNLCD_SYS_IOCTL(PNLCD_CLEAR_ALL_IOCTL, 0);
					
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
					PNLCD_SYS_IOCTL(PNLCD_ANIMATE_IOCTL, &pnlcd_animation);

			entry_exit_common:
				if ( !dont_delay )
					PNLCD_SYS_IOCTL(PNLCD_BLOCK_IOCTL, pnlcd_blocking_cmd);
			
				// Note that the next (externally-generated) update needs to
				// be full.
				//
				//if ( splash_screen_activity_begin == which_activity )
				//	force_full_update_acknowledge = 0;
			break;

			// Prevent compiler from complaining.
			//
			default:
			break;
		}

		// If necessary, ACK the Power Manager.
		//
		if ( ack_power_manager )
			einkfb_shim_power_op_complete();
			
		// If necessary, wake the thread that gets the next screen
		// saver.
		//
		if ( screen_saver_thread_wake )
			wake_screen_saver_thread();
	}

	// Perform general post-update activities.
	//
	if ( splash_screen_activity_end == which_activity )
		splash_screen_up = splash_screen_invalid;
}

static raw_image_t *uncompress_picture(int picture_length, u8 *picture)
{
	raw_image_t *result = NULL;

	if ( picture && picture_length )
	{
		if ( 0 < picture_length )
		{	
			unsigned long length = (unsigned long)picture_length;
			
			if ( 0 == einkfb_shim_gunzip(picture_buffer, picture_buffer_size, picture, &length) )
				result = (raw_image_t *)picture_buffer;
		}
		else
			result = (raw_image_t *)picture;
	}

	return ( result );
}

static int compress_picture(int picture_length, u8 *picture)
{
	int result = 0;

	if ( picture && picture_length )
	{
		unsigned long length = picture_length;
		
		if ( 0 == einkfb_shim_gzip(picture_buffer, picture_buffer_size, picture, &length) )
			result = length;
	}

	return ( result );
}

static int picture_is_acceptable(picture_info_type *picture_info, raw_image_t *picture)
{
	int acceptable = 0;

	if ( picture && picture_info )
	{
		int	x = (0 > picture_info->x) ? 0 : picture_info->x,
			y = (0 > picture_info->y) ? 0 : picture_info->y,
			picture_xres, picture_yres, picture_bpp;
		
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
		
		// The picture's depth must be less than or the same as the framebuffer's
		//
		acceptable &= (picture_bpp <= framebuffer_bpp);
		
		// The picture must be in bounds.
		//
		acceptable &= einkfb_bounds_are_acceptable(x, picture_xres, y, picture_yres);
	}
	
	return ( acceptable );
}

static u8 get_picture_byte(u8 *picture, int picture_bpp, int byte)
{
	u8 picture_byte = EINKFB_WHITE;
	
	// If the framebuffer and pictures depths match, we're just byte for byte.
	//
	if ( picture_bpp == framebuffer_bpp )
		picture_byte = picture[byte];
	else
	{
		// We only support the following stretches:
		//
		//	1bpp -> 2bpp: 0, 1
		//	1bpp -> 4bpp: 0, 1, 2, 3
		//
		//	2bpp -> 4bpp: 0, 1
		//
		int	stretches = framebuffer_bpp/picture_bpp, p = 0;
		u8	pixels[6];
		
		switch ( stretches )
		{
			case 2:
			case 4:
				// Grab the appropriate pixels we need to stretch.
				//
				picture_byte = picture[byte/stretches];
				
				// Don't bother stretching white since it's already "stretched."
				//
				if ( EINKFB_WHITE != picture_byte ) 
				{
					// Do the first two stretches.
					//
					pixels[0] = STRETCH_HI_NYBBLE(picture_byte, picture_bpp);
					pixels[1] = STRETCH_LO_NYBBLE(picture_byte, picture_bpp);
					
					// Do the next four if necessary.
					//
					if ( 4 == stretches )
					{
						pixels[2] = STRETCH_HI_NYBBLE(pixels[0], (picture_bpp << 1));
						pixels[3] = STRETCH_LO_NYBBLE(pixels[0], (picture_bpp << 1));
						pixels[4] = STRETCH_HI_NYBBLE(pixels[1], (picture_bpp << 1));
						pixels[5] = STRETCH_LO_NYBBLE(pixels[1], (picture_bpp << 1));
						
						p = 2;
					}
					
					// Return the appropriate stretched pixels.
					//
					picture_byte = pixels[p + (byte % stretches)];
				}
			break;
		}
	}
	
	return ( picture_byte );
}

static void stretch_bits(u8 *picture, int *picture_bpp, int *picture_size)
{
	int i, j, k, l, m, n = *picture_size, p_bpp = *picture_bpp;
	u8 pixels[14];

	// We only support the following stretches:
	//
	//	1bpp -> 2bpp: 0, 1
	//	1bpp -> 4bpp: 0, 1, 2, 3
	//
	//	2bpp -> 4bpp: 0, 1
	//
	switch ( framebuffer_bpp/p_bpp )
	{
		case 2:
			l = 0; m = 2;
		break;
		
		case 4:
			l = 2; m = 4;
		break;
		
		default:
			l = 0; m = 0;
		break;
	}
	
	// Walk through the picture bytes, stretching them into the kernelbuffer as we go.
	//
	for ( i = 0, j = 0; i < n; i++, j += m )
	{
		pixels[0] = STRETCH_HI_NYBBLE(picture[i], p_bpp);
		pixels[1] = STRETCH_LO_NYBBLE(picture[i], p_bpp);
		
		if ( 4 == m )
		{
			pixels[2] = STRETCH_HI_NYBBLE(pixels[0], (p_bpp << 1));
			pixels[3] = STRETCH_LO_NYBBLE(pixels[0], (p_bpp << 1));
			pixels[4] = STRETCH_HI_NYBBLE(pixels[1], (p_bpp << 1));
			pixels[5] = STRETCH_LO_NYBBLE(pixels[1], (p_bpp << 1));
		}
		
		for ( k = l; k < m; k++ )
			kernelbuffer[j + (k - l)] = pixels[k];
			
		EINKFB_SCHEDULE_BLIT(i+1);
	}
	
	// Update the picture info now that we've stretched it.
	//
	EINKFB_MEMCPYK(picture, kernelbuffer, j);
	*picture_bpp  = framebuffer_bpp;
	*picture_size = j;
}

struct blit_picture_t
{
	u8	*start;
	int	bpp;
};
typedef struct blit_picture_t blit_picture_t;

static void blit_splash_screen(int x, int y, int rowbytes, int bytes, void *data)
{
	blit_picture_t *blit_picture = (blit_picture_t *)data;
	kernelbuffer[(rowbytes * y) + x] = get_picture_byte(blit_picture->start, blit_picture->bpp, bytes);
}

static void display_splash_screen(splash_screen_type which_screen)
{
	// Whether we're being asked to draw the same screen or not,
	// begin (or re-begin) any screen-specific activity.
	//
	
	return;	
	begin_splash_screen_activity(which_screen);

	// Don't redraw if the screen we're being asked to display
	// is already up.
	//
	if ( splash_screen_up != which_screen )
	{
		picture_info_type *picture_info = NULL;
		reboot_behavior_t reboot_behavior;
		raw_image_t *picture = NULL;
		int splash_screen = 1;
		int picture_len = 0;
	
		switch ( which_screen )
		{
			case splash_screen_powering_off:
			case splash_screen_powering_off_wireless:
			//	clear_screen(fx_update_full);
			/* break; */

			case splash_screen_screen_saver_picture:
			case splash_screen_exit:
				set_drivemode_screen_ready(0);
				splash_screen_up = which_screen;
				splash_screen = 0;
			break;

			case splash_screen_powering_on:
			case splash_screen_powering_on_wireless:
			case splash_screen_logo:
				EINKFB_IOCTL(FBIO_EINK_GET_REBOOT_BEHAVIOR, (unsigned long)&reboot_behavior);
				
				picture = (raw_image_t *)picture_logo;
				picture_len = picture_logo_len;
				
				if ( reboot_screen_splash == reboot_behavior )
				{
					picture_info =	splash_screen_logo_buffer_only		?
							&picture_logo_info_update_buffer_only	:
							&picture_logo_info_update_full_refresh;
				}
				else
				{
					if ( get_screen_clear() )
						picture_info = &picture_logo_info_update_partial;
					else
						picture_info = &picture_logo_info_update_full;
				}
			break;
			
			case splash_screen_usb_internal:
				picture = (raw_image_t *)picture_usb_internal;
				picture_info = &picture_usb_internal_info;
				picture_len = picture_usb_internal_len;
			break;
			
			case splash_screen_usb_external:
				picture = (raw_image_t *)picture_usb_external;
				picture_info = &picture_usb_external_info;
				picture_len = picture_usb_external_len;
			break;
			
			case splash_screen_usb:
				picture = (raw_image_t *)picture_usb;
				picture_info = &picture_usb_info;
				picture_len = picture_usb_len;
				
				set_drivemode_screen_ready(0);
				dont_deanimate = 1;
			break;
			
			case splash_screen_sleep:
				picture = (raw_image_t *)picture_sleep;
				picture_info = &picture_sleep_info;
				picture_len = picture_sleep_len;
			break;

			case splash_screen_update:
				picture = (raw_image_t *)picture_update;
				picture_info = &picture_update_info;
				picture_len = picture_update_len;
			break;
			
			case splash_screen_shim_picture:
				picture = (raw_image_t *)shim_picture;
				picture_info = &shim_picture_info;
				picture_len = shim_picture_len;
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
				int	picture_xres, picture_yres, picture_bpp;
				u32	x_start, x_end, y_start, y_end;
				u8  	*picture_start;

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
				x_start = (0 > picture_info->x) ? ((framebuffer_xres - picture_xres) >> 1) : picture_info->x;
				x_end   = x_start + picture_xres;
				
				y_start = (0 > picture_info->y) ? ((framebuffer_yres - picture_yres) >> 1) : picture_info->y;
				y_end   = y_start + picture_yres;
					
				// If we're overlaying, use the picture's buffer.  Otherwise,
				// move the picture buffer to the framebuffer.
				//
				if ( (update_overlay == update) || (update_overlay_mask == update) )
				{
					// Stretch the picture's depth to the framebuffer's if necessary.
					//
					if ( picture_bpp != framebuffer_bpp )
					{
						int picture_size;
						
						// Make sure we re-include the mask in the size calculation since
						// we've discluded it in the placement calculation.
						//
						if ( update_overlay_mask == update )
							picture_yres *= 2;
					
						picture_size = BPP_SIZE((picture_xres * picture_yres), picture_bpp);
						stretch_bits(picture_start, &picture_bpp, &picture_size);
					}

					// If we're using an overlay mask, put ourselves into mask mode.
					//
					if ( update_overlay_mask == update )
						einkfb_update_display_fx_area(fx_mask, picture_start, x_start, x_end, y_start, y_end,
							picture_info->to_screen);
					else
						update_display_area(fx_none, picture_start, x_start, x_end, y_start, y_end,
							picture_info->to_screen);
				}
				else
				{
					blit_picture_t blit_picture;
					
					// Clear background when we're doing full updates
					//
					if ( (update_full == update) || (update_full_refresh == update) )
						clear_kernel_buffer();
					
					// Blit to kernelbuffer and update the display.
					//
					blit_picture.start = picture_start;
					blit_picture.bpp   = picture_bpp;

					einkfb_blit(x_start, x_end, y_start, y_end, blit_splash_screen, (void *)&blit_picture);
		
					if ( picture_info->to_screen )
						update_display((update_full_refresh == update) ? fx_update_full : fx_update_partial);
				
					// If necessary, write the kernelbuffer back to the framebuffer.
					//
					switch ( update)
					{
						case update_partial:
						case update_full:
						case update_full_refresh:
							memcpy_from_kernelbuffer_to_framebuffer();
						break;
						
						// Prevent compiler from complaining.
						//
						default:
						break;
					}
				}
	
				splash_screen_up = (splash_screen_shim_picture != which_screen) ? which_screen : splash_screen_invalid;
			}
		}
	}
}

static void stretch_screen_saver(u8 *buffer)
{
	int	buffer_bpp		= SCREEN_SAVER_BPP,
		buffer_size		= BPP_SIZE((SCREEN_SAVER_XRES * SCREEN_SAVER_YRES), buffer_bpp);

	stretch_bits(buffer, &buffer_bpp, &buffer_size);
}

static void fit_screen_saver(u8 *buffer)
{
	shim_picture_info.x		= 0;
	shim_picture_info.y		= 0;
	shim_picture_info.update	= update_none;
	shim_picture_info.to_screen	= update_buffer;
	shim_picture_info.headerless	= true;
	shim_picture_info.xres		= SCREEN_SAVER_XRES;
	shim_picture_info.yres		= SCREEN_SAVER_YRES;
	shim_picture_info.bpp		= SCREEN_SAVER_BPP;
	
	shim_picture_len		= ALREADY_UNCOMPRESSED;
	shim_picture			= buffer;

	display_splash_screen(splash_screen_shim_picture);
}

static void display_screen_saver(void)
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
				if ( SCREEN_SAVER_XRES != framebuffer_xres )
					fit_screen_saver(screen_saver_buffer);
				else
				{
					if ( SCREEN_SAVER_BPP == framebuffer_bpp )
						memcpy_from_screen_saver_to_kernelbuffer();
					else
						stretch_screen_saver(screen_saver_buffer);
				}
				
				use_default = 0;
			break;
			
			// Rotational screen saver case.
			//
			default:
				if ( uncompress_picture(valid_screen_saver_buf, screen_saver_buffer) )
				{
					if ( SCREEN_SAVER_XRES != framebuffer_xres )
						fit_screen_saver(picture_buffer);
					else
					{
						if ( SCREEN_SAVER_BPP == framebuffer_bpp )
							memcpy_from_picture_buffer_to_kernelbuffer();
						else
							stretch_screen_saver(picture_buffer);
					}

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
		display_splash_screen(splash_screen_sleep);
	}
	else
	{
		// Get the screen saver onto the display.
		//
		update_display(fx_update_full);
		
		// Overlay it with the standard sleep screen's message.
		//
		display_splash_screen(splash_screen_screen_saver_picture);
	}
}

static void display_drivemode_screen(splash_screen_type which_screen)
{
	switch ( which_screen )
	{
		case splash_screen_drivemode_1:
			display_splash_screen(splash_screen_usb_internal);
		break;
		
		case splash_screen_drivemode_2:
			display_splash_screen(splash_screen_usb_external);
		break;
		
		case splash_screen_drivemode_3:
			display_splash_screen(splash_screen_usb_internal);
			display_splash_screen(splash_screen_usb_external);
		break;
		
		// Prevent compiler from complaining.
		//
		default:
		break;
	}

	// Stop PNLCD animation.
	//
	dont_deanimate = 0;
	end_splash_screen_activity();
	
	// Start drivemode screen animation.
	//
	set_drivemode_screen_ready(1);
}

static int get_progressbar_y(int y, int progressbar_height)
{
	int result = y, alignment = BPP_BYTE_ALIGN(framebuffer_align);
	
	if ( !IN_RANGE(result, 0, (framebuffer_yres - progressbar_height)) )
		result = ((framebuffer_yres * 4)/5) - (progressbar_height/2);
		
	// Ensure that our computed y-coordinate is byte aligned.
	//
	if ( 0 != (result % alignment) )
		result = BPP_BYTE_ALIGN_DN(result, framebuffer_align);
		
	return ( result );
}

#define get_system_header_y(o)	(UPDATE_HEADER_OFFSET_Y + (o))
#define get_system_footer_y(o)	(UPDATE_FOOTER_OFFSET_Y + (o))
#define get_system_body_y(o)    (UPDATE_PROGRESS_OFFSET_Y + (o))

static int get_progressbar_x(int progressbar_width)
{
	int result, alignment = BPP_BYTE_ALIGN(framebuffer_align);
	
	// Ensure that our computed x-coordinate is byte aligned.
	//
	result = (framebuffer_xres - progressbar_width) >> 1;
	
	if ( 0 != (result % alignment) )
		result = BPP_BYTE_ALIGN_DN(result, framebuffer_align);
	
	return ( result );
}

#define get_update_header_x	get_progressbar_x
#define get_update_footer_x 	get_progressbar_x
#define get_system_header_x 	get_progressbar_x
#define get_system_footer_x 	get_progressbar_x
#define get_system_body_x	get_progressbar_x

static void draw_progressbar_item(int x, int y, int picture_len, u8 *picture)
{
	shim_picture_info.x		= x;
	shim_picture_info.y		= y;
	shim_picture_info.update	= update_none;
	shim_picture_info.to_screen	= update_buffer;
	shim_picture_info.headerless	= false;
	
	shim_picture_len		= picture_len;
	shim_picture			= picture;
	
	display_splash_screen(splash_screen_shim_picture);
}

static void draw_progressbar_empty(void)
{
	draw_progressbar_item
	(
		progressbar_x,
		progressbar_y,
		PICTURE_PROGRESSBAR_EMPTY_LEN(progressbar_background),
		PICTURE_PROGRESSBAR_EMPTY(progressbar_background)
	);
}

static void draw_progressbar_left_full(void)
{
	draw_progressbar_item
	(
		progressbar_x,
		progressbar_y,
		PICTURE_PROGRESSBAR_LEFT_FULL_LEN(progressbar_background),
		PICTURE_PROGRESSBAR_LEFT_FULL(progressbar_background)
	);
}

static void draw_progressbar_right_full(void)
{
	draw_progressbar_item
	(
		progressbar_x + (PROGRESSBAR_WIDTH - PROGRESSBAR_WIDTH_END),
		progressbar_y,
		PICTURE_PROGRESSBAR_RIGHT_FULL_LEN(progressbar_background),
		PICTURE_PROGRESSBAR_RIGHT_FULL(progressbar_background)
	);
}

static void draw_progressbar_slice(int i)
{
	draw_progressbar_item
	(
		progressbar_x + (PROGRESSBAR_WIDTH_END + (PROGRESSBAR_WIDTH_SLICE * i)),
		progressbar_y,
		PICTURE_PROGRESSBAR_SLICE_LEN(progressbar_background),
		PICTURE_PROGRESSBAR_SLICE(progressbar_background)
	);
}

static void DISPLAY_PROGRESSBAR(update_which to_screen)
{
	// Prevent the progressbar from being display when the Framework is running.
	//
	if ( !FRAMEWORK_RUNNING() )
	{
		int	xstart	= progressbar_x,
			xend	= xstart + PROGRESSBAR_WIDTH,
			ystart	= progressbar_y,
			yend	= ystart + PROGRESSBAR_HEIGHT;
	
		memcpy_from_kernelbuffer_to_framebuffer();
		update_display_area(fx_update_partial, NULL, xstart, xend, ystart, yend, to_screen);
	}
}

static void clear_progressbar(void)
{
	shim_picture_info.x		= progressbar_x;
	shim_picture_info.y		= progressbar_y;
	shim_picture_info.update	= update_none;
	shim_picture_info.to_screen	= update_buffer;
	shim_picture_info.headerless	= true;
	shim_picture_info.xres		= PROGRESSBAR_WIDTH;
	shim_picture_info.yres		= PROGRESSBAR_HEIGHT;
	shim_picture_info.bpp		= framebuffer_bpp;
	
	shim_picture_len		= ALREADY_UNCOMPRESSED;
	shim_picture			= picture_buffer;

	clear_picture_buffer(progressbar_background);
	display_splash_screen(splash_screen_shim_picture);
}

static void display_progressbar_badge(progressbar_badge_t badge, update_which to_screen)
{
	int picture_len = 0;
	u8 *picture = NULL;

	switch ( badge )
	{
		case progressbar_badge_success:
			picture_len = picture_progressbar_badge_success_len;
			picture = picture_progressbar_badge_success;
		break;
		
		case progressbar_badge_failure:
			picture_len = picture_progressbar_badge_fail_len;
			picture = picture_progressbar_badge_fail;
		break;
		
		// Prevent compiler from complaining.
		//
		default:
		break;
	}

	if ( picture_len && picture )
	{
		int y = get_progressbar_y(progressbar_y, PROGRESSBAR_HEIGHT);

		shim_picture_info.x		= get_progressbar_x(PROGRESSBAR_BADGE_WIDTH);
		shim_picture_info.y		= y - ((PROGRESSBAR_BADGE_HEIGHT - PROGRESSBAR_HEIGHT) >> 1);
		shim_picture_info.update	= update_overlay_mask;
		shim_picture_info.to_screen	= to_screen;
		shim_picture_info.headerless	= false;
		
		shim_picture_len		= picture_len;
		shim_picture			= picture;
	
		display_splash_screen(splash_screen_shim_picture);
	}
}

static void display_progressbar(int n, update_which to_screen)
{
	if ( IN_RANGE(n, 0, 100) )
	{
		// Reset the progressbar's y-coordinate if it's bogus, and recache the
		// main x-coordinate.
		//
		if ( !IN_RANGE(progressbar_y, 0, (framebuffer_yres - PROGRESSBAR_HEIGHT)) )
		{
			progressbar_y = get_progressbar_y(progressbar_y, PROGRESSBAR_HEIGHT);
			progressbar_x = get_progressbar_x(PROGRESSBAR_WIDTH);
		}
		
		// Draw 0%.
		//
		draw_progressbar_empty();
		
		if ( 0 != n )
		{
			int i, m = n - 1;
			
			// Draw 1%.
			//
			draw_progressbar_left_full();
			
			// Draw 2% - 99%
			//
			for ( i = 0; i < m; i++ )
				draw_progressbar_slice(i);
				
			// Draw 100%.
			//
			if ( 100 == n )
				draw_progressbar_right_full();
		}
		
		progressbar = n;
	}
	else
	{
		// Erase it.
		//
		clear_progressbar();
		progressbar = 0;
	}
	
	set_progress_bar_value(progressbar);
	
	// Display it.
	//
	DISPLAY_PROGRESSBAR(to_screen);
	
	// This is the mechanism that determines whether the Framework is running or not:
	// 
	// If the Framework isn't currently running, but it's been started, and the
	// progressbar has reached 100%, then we've detected that the Framework is
	// now running.
	//
	if ( CHECK_FRAMEWORK_RUNNING() && (100 == progressbar) )
	{
		// Stop the Framework-running detection mechanism and say
		// that the Framework is now running.
		//
		set_framework_started(0);
		set_framework_stopped(0);
		set_framework_running(1);
		
		// Refresh the logo for the Framework, which also takes down
		// the progressbar.
		//
		EINKFB_IOCTL(FBIO_EINK_SPLASH_SCREEN, splash_screen_boot);

		// We might have put the screen to sleep to prevent screen corruption,
		// for example, when the Framework crashes.  So, ensure that it's
		// now awake again.
		// 
		EINKFB_BLANK(FB_BLANK_UNBLANK);
	}
}

static int set_progressbar_xy(progressbar_xy_t *progressbar_xy)
{
	int result = EINKFB_FAILURE;
	
	if ( progressbar_xy )
	{
		// For now, we only allow the y-coordinate to be changed if it's in range.
		//
		if ( IN_RANGE(progressbar_xy->y, 0, (framebuffer_yres - PROGRESSBAR_HEIGHT)) )
		{
			progressbar_y = progressbar_xy->y;
			progressbar_x = get_progressbar_x(PROGRESSBAR_WIDTH);
			
			result = EINKFB_SUCCESS;
		}
		else
			progressbar_y = PROGRESSBAR_Y;
	}
	
	return ( result );
}

static void set_progressbar_background(u8 background)
{
	switch ( background )
	{
		case EINKFB_WHITE:
		case EINKFB_BLACK:
			progressbar_background = background;
		break;
		
		default:
			progressbar_background = EINKFB_WHITE;
		break;
	}
}

static void display_boot_screen(splash_screen_type which_screen)
{
	if ( splash_screen_up != which_screen )
	{
		// Ensure that the entire buffer is clear.
		//
		clear_kernel_buffer();
		
		// Put the black boot background picture into the buffer.
		//
		shim_picture_info.x		= PICTURE_BOOT_BACKGROUND_X();
		shim_picture_info.y		= PICTURE_BOOT_BACKGROUND_Y(framebuffer_yres);
		shim_picture_info.update	= update_none;
		shim_picture_info.to_screen	= update_buffer;
		shim_picture_info.headerless	= false;
		
		shim_picture_len		= PICTURE_BOOT_BACKGROUND_LEN(framebuffer_yres);
		shim_picture			= PICTURE_BOOT_BACKGROUND(framebuffer_yres);
	
		display_splash_screen(splash_screen_shim_picture);
		set_progressbar_background(EINKFB_BLACK);
	
		// Put the logo into the buffer where it should be when the black boot background
		// picture is up.
		//
		shim_picture_info.x		= PICTURE_BOOT_LOGO_X();
		shim_picture_info.y		= PICTURE_BOOT_LOGO_Y(framebuffer_yres);
		shim_picture_info.update	= update_none;
		shim_picture_info.to_screen	= update_buffer;
		shim_picture_info.headerless	= false;
		
		shim_picture_len		= picture_logo_len;
		shim_picture			= picture_logo;
	
		display_splash_screen(splash_screen_shim_picture);
		
		// Put the restart message into the buffer where the progress bar normally
		// goes if we're rebooting.
		//
		if ( splash_screen_reboot == which_screen )
		{
			shim_picture_info.x		= get_progressbar_x(PICTURE_RESTART_WIDTH);
			shim_picture_info.y		= get_progressbar_y(PROGRESSBAR_Y, PICTURE_RESTART_HEIGHT);
			shim_picture_info.update	= update_none;
			shim_picture_info.to_screen	= update_buffer;
			shim_picture_info.headerless	= false;
			
			shim_picture_len		= picture_restart_len;
			shim_picture			= picture_restart;
		
			display_splash_screen(splash_screen_shim_picture);
		}
		
		// Say that we now want a flashing update.
		//
		update_display(fx_update_full);
		memcpy_from_kernelbuffer_to_framebuffer();
	
		// Prevent back-to-back redraws.
		//
		splash_screen_up = which_screen;
	}
}

static void set_update_screen_progressbar_xy(void)
{
	progressbar_xy_t progressbar_xy = { 0 };
	
	switch ( framebuffer_yres )
	{
		case 800:
		default:
			progressbar_xy.y = UPDATE_PROGRESS_OFFSET_Y_800;
		break;
		
		case 1200:
			progressbar_xy.y = UPDATE_PROGRESS_OFFSET_Y_1200;
		break;
	}
	
	set_progressbar_xy(&progressbar_xy);
}

void display_system_screen(system_screen_t *system_screen)
{
	if ( system_screen && (splash_screen_up != system_screen->which_screen) )
	{
		// Ensure that the entire buffer is clear.
		//
		clear_kernel_buffer();
		
		// Buffer the header.
		//
		if ( system_screen->picture_header_len && system_screen->picture_header )
		{
			shim_picture_info.x		= get_system_header_x(system_screen->header_width);
			shim_picture_info.y		= get_system_header_y(system_screen->header_offset);
			shim_picture_info.update	= update_none;
			shim_picture_info.to_screen	= update_buffer;
			shim_picture_info.headerless	= false;
			
			shim_picture_len		= system_screen->picture_header_len;
			shim_picture			= system_screen->picture_header;
		
			display_splash_screen(splash_screen_shim_picture);
		}

		// Buffer the body.
		//
		if ( system_screen->picture_body_len && system_screen->picture_body )
		{
			shim_picture_info.x		= get_system_body_x(system_screen->body_width);
			shim_picture_info.y		= get_system_body_y(system_screen->body_offset);
			shim_picture_info.update	= update_none;
			shim_picture_info.to_screen	= update_buffer;
			shim_picture_info.headerless	= false;
			
			shim_picture_len		= system_screen->picture_body_len;
			shim_picture			= system_screen->picture_body;
		
			display_splash_screen(splash_screen_shim_picture);
		}
		
		// Buffer the footer.
		//
		if ( system_screen->picture_footer_len && system_screen->picture_footer )
		{
			shim_picture_info.x		= get_system_footer_x(system_screen->footer_width);
			shim_picture_info.y		= get_system_footer_y(system_screen->footer_offset);
			shim_picture_info.update	= update_none;
			shim_picture_info.to_screen	= update_buffer;
			shim_picture_info.headerless	= false;
			
			shim_picture_len		= system_screen->picture_footer_len;
			shim_picture			= system_screen->picture_footer;
		
			display_splash_screen(splash_screen_shim_picture);
		}
		
		// Now get everything onto the screen if we're supposed to and copied
		// back to the framebuffer.
		//
		if ( system_screen->to_screen )
		{
			update_display(system_screen->which_fx);
			memcpy_from_kernelbuffer_to_framebuffer();

			// Prevent back-to-back redraws.
			//
			splash_screen_up = system_screen->which_screen;
		}
	}
}

static void display_update_screen(splash_screen_type which_screen)
{
	if ( splash_screen_up != which_screen )
	{
		int 	update_footer_offset = 0,
			update_footer_width = 0,
			picture_footer_len = 0;
		u8 	*picture_footer = NULL;
			
		system_screen_t system_screen = INIT_SYSTEM_SCREEN_T();
		fx_type	which_fx = fx_update_partial;
		
		progressbar_badge_t badge = progressbar_badge_none;
		int	progress = 0;

		switch ( which_screen )
		{
			case splash_screen_update_initial:
				which_fx = fx_update_full;
				
				picture_footer_len = PICTURE_UPDATE_FOOTER_INITIAL_LEN(framebuffer_yres);
				picture_footer = PICTURE_UPDATE_FOOTER_INITIAL(framebuffer_yres);
				
				update_footer_offset = UPDATE_FOOTER_OFFSET_INITIAL(framebuffer_yres);
				update_footer_width = UPDATE_FOOTER_WIDTH_INITIAL(framebuffer_yres);
			break;
			
			case splash_screen_update_success:
				badge    = progressbar_badge_success;
				progress = 100;
			
				picture_footer_len = PICTURE_UPDATE_FOOTER_SUCCESS_LEN(framebuffer_yres);
				picture_footer = PICTURE_UPDATE_FOOTER_SUCCESS(framebuffer_yres);
				
				update_footer_offset = UPDATE_FOOTER_OFFSET_SUCCESS(framebuffer_yres);
				update_footer_width = UPDATE_FOOTER_WIDTH_SUCCESS(framebuffer_yres);
			break;
			
			case splash_screen_update_failure:
				badge 	 = progressbar_badge_failure;
				progress = get_progress_bar_value();
			
				picture_footer_len = PICTURE_UPDATE_FOOTER_FAILURE_LEN(framebuffer_yres);
				picture_footer = PICTURE_UPDATE_FOOTER_FAILURE(framebuffer_yres);
				
				update_footer_offset = UPDATE_FOOTER_OFFSET_FAILURE(framebuffer_yres);
				update_footer_width = UPDATE_FOOTER_WIDTH_FAILURE(framebuffer_yres);
			break;
			
			case splash_screen_update_failure_no_wait:
				badge 	 = progressbar_badge_failure;
				progress = get_progress_bar_value();
			
				picture_footer_len = PICTURE_UPDATE_FOOTER_FAIL_NOWAIT_LEN(framebuffer_yres);
				picture_footer = PICTURE_UPDATE_FOOTER_FAIL_NOWAIT(framebuffer_yres);
				
				update_footer_offset = UPDATE_FOOTER_OFFSET_FAIL_NOWAIT(framebuffer_yres);
				update_footer_width = UPDATE_FOOTER_WIDTH_FAIL_NOWAIT(framebuffer_yres);
			break;

			// Prevent compiler from complaining.
			//
			default:
			break;
		}
		
		// Buffer the header and footer.
		//
		system_screen.picture_header_len = PICTURE_UPDATE_HEADER_LEN(framebuffer_yres);
		system_screen.picture_header = PICTURE_UPDATE_HEADER(framebuffer_yres);
		system_screen.header_offset = UPDATE_HEADER_OFFSET(framebuffer_yres);
		system_screen.header_width = UPDATE_HEADER_WIDTH(framebuffer_yres);
		
		system_screen.picture_footer_len = picture_footer_len;
		system_screen.picture_footer = picture_footer;
		system_screen.footer_offset = update_footer_offset;
		system_screen.footer_width = update_footer_width;
		
		system_screen.picture_body_len = 0;
		system_screen.picture_body = NULL;
		system_screen.body_width = 0;
		
		system_screen.which_screen = which_screen;
		system_screen.to_screen = update_buffer;
		system_screen.which_fx = update_none;
		
		display_system_screen(&system_screen);

		// Buffer the (white background) progressbar and badge.
		//
		set_progressbar_background(EINKFB_WHITE);
		set_update_screen_progressbar_xy();

		display_progressbar(progress, update_buffer);
		display_progressbar_badge(badge, update_buffer);
		
		// Now get everything onto the screen and copied back to
		// the framebuffer.
		//
		update_display(which_fx);
		memcpy_from_kernelbuffer_to_framebuffer();

		// Prevent back-to-back redraws.
		//
		splash_screen_up = which_screen;
	}
}

#define IS_REBOOT_SCREEN(s)			\
	((splash_screen_lowbatt == (s))	||	\
	 (splash_screen_reboot == (s)))

void splash_screen_dispatch(splash_screen_type which_screen)
{
	orientation_t saved_orientation = orientation_portrait;
	bool restore_orientation = false;
	
	// Except for the logo screen, which is always entirely centered and
	// fits in both portrait and landscape orientations, force us into
	// the portrait orientation after remembering whether we need to
	// put ourselves back into the orientation we started with.
	//
	if ( splash_screen_logo != which_screen ) 
	{
		// Save the display's current orientation, and force it into portrait
		// mode in order to display the splash screen.
		//
		EINKFB_IOCTL(FBIO_EINK_GET_DISPLAY_ORIENTATION, (unsigned long)&saved_orientation);
		EINKFB_IOCTL(FBIO_EINK_SET_DISPLAY_ORIENTATION, orientation_portrait);
		
		// Say to restore the starting orientation if it changed but not if we're
		// about to reboot.
		//
		restore_orientation = (saved_orientation != orientation_portrait) && !IS_REBOOT_SCREEN(which_screen);
	}

	// Don't do anything if the platform handles the call itself.
	//
	if ( !einkfb_shim_platform_splash_screen_dispatch(which_screen, framebuffer_yres) )
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
				display_splash_screen(which_screen);
			else
				display_screen_saver();
		}
		else
		{
			// Composite splash screen cases.
			//
			switch ( which_screen )
			{
				// Drivemode screens.
				//
				case splash_screen_drivemode_0:
				case splash_screen_drivemode_1:
				case splash_screen_drivemode_2:
				case splash_screen_drivemode_3:
					display_drivemode_screen(which_screen);
				break;
					
				// Boot/reboot screens.
				//
				case splash_screen_boot:
				case splash_screen_reboot:
					display_boot_screen(which_screen);
				break;
				
				// Update screens.
				//
				case splash_screen_update_initial:
				case splash_screen_update_success:
				case splash_screen_update_failure:
				case splash_screen_update_failure_no_wait:
					display_update_screen(which_screen);
				break;
	
				// Otherwise, just let the splash screen code itself
				// handle things.
				//
				default:
					display_splash_screen(which_screen);
				break;
			}
		}
	}
	
	// Restore the orientation if we should.
	//
	if ( restore_orientation )
	{
		EINKFB_IOCTL(FBIO_EINK_SET_DISPLAY_ORIENTATION, saved_orientation);
		
		// If we swapped orientations, invalidate the framebuffer's contents.
		//
		if ( !ORIENTATION_SAME(orientation_portrait, saved_orientation) )
			clear_frame_buffer();
	}
}

#if PRAGMAS
	#pragma mark -
	#pragma mark Main
	#pragma mark -
#endif

static void einkfb_shim_reboot_hook(reboot_behavior_t behavior)
{
	behavior = reboot_screen_clear;
	switch ( behavior )
	{
		case reboot_screen_clear:
			clear_kernel_buffer();
		break;
		
		case reboot_screen_splash:
			splash_screen_dispatch(splash_screen_reboot);
		break;
		
		default:
		break;
	}
}

static void einkfb_shim_info_hook(struct einkfb_info *info)
{
	if ( info && override_framebuffer )
		info->start = override_framebuffer;
}

static void einkfb_shim_check_orientation(void)
{
	struct einkfb_info info; einkfb_get_info(&info);
	
	// If the orientation changed (x <-> y), then update our world to match the new one.
	//
	if ( ORIENTATION(info.xres, info.yres) != framebuffer_orientation )
	{
		// Invalidate any screen-related caches.
		//
		splash_screen_up = splash_screen_invalid;

		progressbar_y = PROGRESSBAR_Y;
		progressbar_x = PROGRESSBAR_X;

		// Update any resolution-oriented caches.
		//
		framebuffer_xres = info.xres;
		framebuffer_yres = info.yres;
		
		framebuffer_orientation = ORIENTATION(framebuffer_xres, framebuffer_yres);
	}
}

static void einkfb_shim_check_for_cursor(update_area_t *update_area)
{
	if ( update_area )
	{
		int	xstart = update_area->x1, xend = update_area->x2,
			xres   = xend - xstart;
		
		if (	FRAMEWORK_RUNNING()				&&
			(FAKE_FLASHING_AREA_UPDATE_XRES == xres)	&&
			(fx_update_full == update_area->which_fx)	&&
			(NULL == update_area->buffer)			)
		{	
			update_area->which_fx = fx_flash;
			einkfb_debug("detected Reader's blinking cursor; simulating\n");
		}
	}
}

static int einkfb_shim_ioctl(unsigned int cmd, unsigned long arg)
{
	return ( EINKFB_IOCTL(cmd, arg) );
}

static bool einkfb_shim_ioctl_hook(einkfb_ioctl_hook_which which, unsigned long flag, unsigned int *cmd, unsigned long *arg)
{
	char segments[PN_LCD_SEGMENT_COUNT] = { SEGMENT_OFF };
	bool result = EINKFB_IOCTL_DONE;
	unsigned long local_arg = *arg;
	unsigned int local_cmd = *cmd;
	int success = EINKFB_FAILURE;
	
	// Enforce Power Management rules before dispatching.
	//
	if ( einkfb_shim_override_power_lockout(local_cmd, flag) )
	{
		goto override;
	}
	else
	{
		if ( einkfb_shim_enforce_power_lockout() )
		{
			einkfb_debug_power("%s lockout, ignoring %s\n",
				einkfb_shim_get_power_string(), einkfb_get_cmd_string(local_cmd));

			goto done;
		}
	}

	override:
	switch ( local_cmd )
	{
		// Post-process both of the screen-updating commands, and perform some
		// in-situ work on the partial-updating command.
		//
		case FBIO_EINK_UPDATE_DISPLAY_AREA:
			switch ( which )
			{
				// Exit directly on in-situ cases.
				//
				case einkfb_ioctl_hook_insitu_begin:
					EINKFB_UPDATE_DISPLAY_FX_AREA_BEGIN_ADJUST_Y((update_area_t *)local_arg);
				goto done;
				
				case einkfb_ioctl_hook_insitu_end:
					einkfb_update_display_fx_area_end((update_area_t *)local_arg);
				goto done;
				
				// Fall thru everywhere else.
				//
				default:;
			}
		
		case FBIO_EINK_UPDATE_DISPLAY:
			if ( einkfb_ioctl_hook_post == which )
			{
				// Clean up after any splash screen activity.
				//
				end_splash_screen_activity();
				
				// Handle the fake PNLCD.
				//
				if ( FBIO_EINK_UPDATE_DISPLAY == local_cmd )
					fake_pnlcd_valid = 0;
				
				if ( !LOCAL_UPDATE() )
					if ( get_pnlcd_segments(segments) )
					        update_fake_pnlcd(segments);
				
				// Sync up with the HAL's buffer if we didn't
				// initiate the screen-update ourselves.
				//
				if ( !LOCAL_UPDATE() )
					memcpy_from_framebuffer_to_kernelbuffer();
			}
			else
			{
				// Clear the last FX only if the update wasn't
				// initiated locally.
				//
				if ( !LOCAL_UPDATE() )
					last_display_fx = fx_none;

				// On area updates, check to see whether we need to simulate a
				// flashing update or not (for Reader's "cursor").
				//
				if ( FBIO_EINK_UPDATE_DISPLAY_AREA == local_cmd )
					einkfb_shim_check_for_cursor((update_area_t *)local_arg);
				
				goto not_done;
			}
		break;
		
		// Handle the splash screen, clear screen, fake PNLCD, and progressbar calls here
		// (we're always "done" with these).
		//
		case FBIO_EINK_SPLASH_SCREEN:
		case FBIO_EINK_SPLASH_SCREEN_SLEEP:
			if ( einkfb_ioctl_hook_pre == which )
				einkfb_shim_sleep_screen(local_cmd, (splash_screen_type)local_arg);
		break;

		case FBIO_EINK_CLEAR_SCREEN:
			if ( einkfb_ioctl_hook_pre == which )
				clear_screen(fx_update_full);
		break;
		
		case FBIO_EINK_OFF_CLEAR_SCREEN:
//			if ( einkfb_ioctl_hook_pre == which )
//				einkfb_shim_power_off_screen();
		break;
		
		case FBIO_EINK_FAKE_PNLCD:
			if ( einkfb_ioctl_hook_pre == which )
			{
				// If we're passed the segments, use those.  Otherwise, extract
				// them from the PNLCD driver.
				//
				if ( local_arg )
				{
					if ( PROC_EINK_FAKE_PNLCD_TEST == local_arg )
						fake_pnlcd_test();
					else
						success = einkfb_memcpy(EINKFB_IOCTL_FROM_USER, flag, segments,
							(void *)local_arg, PN_LCD_SEGMENT_COUNT);
				}
				else
				{
					if ( get_pnlcd_segments(segments) )
						success = EINKFB_SUCCESS;
				}

				if ( EINKFB_SUCCESS == success )
					update_fake_pnlcd(segments);
			}
		break;
		
		case FBIO_EINK_PROGRESSBAR:
			if ( einkfb_ioctl_hook_pre == which )
				display_progressbar((int)local_arg, update_screen);
		break;

		case FBIO_EINK_PROGRESSBAR_SET_XY:
			if ( einkfb_ioctl_hook_pre == which )
			{
				if ( EINKFB_FAILURE == set_progressbar_xy((progressbar_xy_t *)local_arg) )
					goto not_done;
			}
			else
			{
				if ( PROGRESSBAR_Y == progressbar_y )
					goto not_done;
			}
		break;
		
		case FBIO_EINK_PROGRESSBAR_BADGE:
			if ( einkfb_ioctl_hook_pre == which )
				display_progressbar_badge((progressbar_badge_t)local_arg, update_screen);
		break;

		case FBIO_EINK_PROGRESSBAR_BACKGROUND:
			if ( einkfb_ioctl_hook_pre == which )
				set_progressbar_background((u8)local_arg);
		break;

		// Even though we're always "done" with this call, the test suite checks whether
		// this ioctl handles NULL or not.  So, keep the test suite happy by truly
		// saying whether we actually handled this call or not.
		//
		case FBIO_EINK_UPDATE_DISPLAY_FX:
			if ( einkfb_ioctl_hook_pre == which )
			{
				fx_t fx;
				
				success = einkfb_memcpy(EINKFB_IOCTL_FROM_USER, flag, &fx,
					(void *)local_arg, sizeof(fx_t));
					
				if ( EINKFB_SUCCESS == success )
				{
					external_fx_update = 1;
					einkfb_update_display_fx(&fx);
					
					external_fx_update = 0;
				}
				else
					goto not_done;
			}
			else
			{
				if ( !local_arg )
					goto not_done;
			}
		break;

		// At pre-command process time, extract the underlying cmd & arg from the power
		// override call, and attempt to process them here first.
		//
		case FBIO_EINK_POWER_OVERRIDE:
			if ( einkfb_ioctl_hook_pre == which )
			{
				power_override_t power_override;

				success = einkfb_memcpy(EINKFB_IOCTL_FROM_USER, flag, &power_override, (void *)local_arg,
					sizeof(power_override_t));
				
				if ( EINKFB_SUCCESS == success )
				{
					POWER_OVERRIDE_BEGIN();
					
					*cmd = local_cmd = power_override.cmd;
					*arg = local_arg = power_override.arg;
				
					goto override;
				}
				else
					goto not_done;
				
			}
			else
			{
				if ( local_arg )
					POWER_OVERRIDE_END();
				else
					goto not_done;
			}
		break;
		
		// At post-command process time, check to see whether the orientation has changed.
		//
		case FBIO_EINK_SET_DISPLAY_ORIENTATION:
			if ( einkfb_ioctl_hook_post == which )
				einkfb_shim_check_orientation();
			else
				goto not_done;
		break;

		// Say that we didn't handle the call here.
		//
		not_done:
		default:
			result = !EINKFB_IOCTL_DONE;
		break;
	}

	done:
	return ( result );
}

static void einkfb_shim_dealloc(void)
{
	// Perform memory deallocations and unmappings.
	//
	if ( screen_saver_buffer )
		vfree(screen_saver_buffer);
		
	if ( fake_pnlcd_buffer )
		kfree(fake_pnlcd_buffer);
	
	if ( kernelbuffer )
	{
		if ( kernelbuffer_local )
			vfree(kernelbuffer);
		else
			einkfb_shim_free_kernelbuffer(kernelbuffer);
	}
}

static DEVICE_ATTR(valid_screen_saver_buf,	DEVICE_MODE_RW, show_valid_screen_saver_buf,	store_valid_screen_saver_buf);
static DEVICE_ATTR(splash_screen_up,		DEVICE_MODE_R,  show_splash_screen_up,		NULL);
static DEVICE_ATTR(send_fake_rotate,		DEVICE_MODE_RW, show_send_fake_rotate,		store_send_fake_rotate);
static DEVICE_ATTR(use_fake_pnlcd,		DEVICE_MODE_RW, show_use_fake_pnlcd,		store_use_fake_pnlcd);
static DEVICE_ATTR(running_diags,		DEVICE_MODE_RW,	show_running_diags,		store_running_diags);
static DEVICE_ATTR(progressbar,			DEVICE_MODE_RW, show_progressbar,		store_progressbar);
static DEVICE_ATTR(eink_debug,  		DEVICE_MODE_RW, show_eink_debug,		store_eink_debug);

static int __init einkfb_shim_init(void)
{
	struct einkfb_info info;
	int result = -ENOMEM;
	
	einkfb_get_info(&info);
	
	// Attempt to perform necessary memory allocations and remappings.
	//
	screen_saver_buffer_size = BPP_SIZE((SCREEN_SAVER_XRES * SCREEN_SAVER_YRES), info.bpp);
	screen_saver_buffer = vmalloc(screen_saver_buffer_size);

	fake_pnlcd_buffer_size = BPP_SIZE((FAKE_PNLCD_XRES_MAX * info.yres), info.bpp);
	fake_pnlcd_buffer = kmalloc(fake_pnlcd_buffer_size, GFP_KERNEL);
	
	kernelbuffer_size = info.size;
	kernelbuffer = (u8 *)einkfb_shim_alloc_kernelbuffer(kernelbuffer_size);
	
	if ( !kernelbuffer && kernelbuffer_size )
	{
		kernelbuffer = vmalloc(kernelbuffer_size);
		
		if ( kernelbuffer )
		{
			kernelbuffer_local = true;
			clear_kernel_buffer();
		}
	}
	
	if ( screen_saver_buffer && fake_pnlcd_buffer && kernelbuffer )
	{
		// If debugging is enabled, output the returned einkfb_info. 
		//
		einkfb_debug("init  = %d\n",     info.init);
		einkfb_debug("done  = %d\n",   	 info.done);
		einkfb_debug("size  = %lu\n",  	 info.size);
		einkfb_debug("blen  = %lu\n",  	 info.blen);
		einkfb_debug("mem   = %lu\n",  	 info.mem);
		einkfb_debug("bpp   = %ld\n",  	 info.bpp);
		einkfb_debug("xres  = %d\n",   	 info.xres);
		einkfb_debug("yres  = %d\n",     info.yres);
		einkfb_debug("align = %lu\n",    info.align);
		einkfb_debug("start = 0x%08X\n", (unsigned int)info.start);
		einkfb_debug("vfb   = 0x%08X\n", (unsigned int)info.vfb);
		einkfb_debug("buf   = 0x%08X\n", (unsigned int)info.buf);
		einkfb_debug("dev   = 0x%08X\n", (unsigned int)info.dev);
		
		// Use the eInk HAL's scratch buffer for the picture buffer (both uses
		// are for a temporary screen-sized transfer buffer).
		//
		picture_buffer_size = info.blen;
		picture_buffer = info.buf;

		// Cache the framebuffer info and copy the kernelbuffer to the framebuffer
		// so that we capture what, if anything, the bootloader put on the screen.
		//
		framebuffer_xres  = info.xres;
		framebuffer_yres  = info.yres;
		framebuffer_bpp   = info.bpp;
		framebuffer_size  = info.size;
		framebuffer_align = info.align;
		
		framebuffer_orientation = ORIENTATION(framebuffer_xres, framebuffer_yres);
		framebuffer = info.start;

		if ( !kernelbuffer_local )
			memcpy_from_kernelbuffer_to_framebuffer();

		// Attempt to create the proc/sysfs entries we need. 
		//
		einkfb_proc_screen_saver = einkfb_create_proc_entry(EINKFB_PROC_EINK_SCREEN_SAVER,
			EINKFB_PROC_CHILD_RW, einkfb_screen_saver_read, einkfb_screen_saver_write);
		
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_valid_screen_saver_buf);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_splash_screen_up);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_send_fake_rotate);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_use_fake_pnlcd);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_running_diags);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_progressbar);
		FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_eink_debug);
		
		// Tell the HAL that we want to be able to override its info.
		//
		einkfb_set_info_hook(einkfb_shim_info_hook);
		
		// Ask the eInk HAL to call us back so that we handle
		// any user-space ioctls it can't handle.
		//
		einkfb_set_ioctl_hook(einkfb_shim_ioctl_hook);
		
		// Direct the kernel-space ioctls to the shim.
		//
		set_fb_ioctl(einkfb_shim_ioctl);
		
		// Ask the eInk HAL to call us back at reboot time.
		//
		einkfb_set_reboot_hook(einkfb_shim_reboot_hook);
		
		// Start up the screen saver thread.
		//
		start_screen_saver_thread();
		
		// Do platform-specific init.
		//
		einkfb_shim_platform_init(&info);
		
		// Say all is well.
		//
		result = EINKFB_SUCCESS;
	}
	else
		einkfb_shim_dealloc();
	
	return ( result );
}

static void __exit einkfb_shim_exit(void)
{
	struct einkfb_info info; einkfb_get_info(&info);
	
	// Done with platform.
	//
	einkfb_shim_platform_done(&info);
	
	// Stop the screen saver thread.
	//
	stop_screen_saver_thread();
	
	// Clear the hooks.
	//
	einkfb_set_reboot_hook(NULL);
	einkfb_set_ioctl_hook(NULL);
	einkfb_set_info_hook(NULL);
	set_fb_ioctl(NULL);

	// Remove any of the proc/sysfs entries we created.
	//
	einkfb_remove_proc_entry(EINKFB_PROC_EINK_SCREEN_SAVER, einkfb_proc_screen_saver);
	device_remove_file(&info.dev->dev, &dev_attr_valid_screen_saver_buf);
	device_remove_file(&info.dev->dev, &dev_attr_splash_screen_up);
	device_remove_file(&info.dev->dev, &dev_attr_send_fake_rotate);
	device_remove_file(&info.dev->dev, &dev_attr_use_fake_pnlcd);
	device_remove_file(&info.dev->dev, &dev_attr_running_diags);
	device_remove_file(&info.dev->dev, &dev_attr_progressbar);
	device_remove_file(&info.dev->dev, &dev_attr_eink_debug);

	// Clean up memory allocations.
	//
	einkfb_shim_dealloc();
}

module_init(einkfb_shim_init);
module_exit(einkfb_shim_exit);

module_param_named(splash_screen_up, splash_screen_up, int, S_IRUGO);
MODULE_PARM_DESC(splash_screen_up, "sets which splash screen is currently up");
module_param_named(use_fake_pnlcd, use_fake_pnlcd, int, S_IRUGO);
MODULE_PARM_DESC(use_fake_pnlcd, "enables/disables use of fake_pnlcd");

MODULE_DESCRIPTION("eInk Legacy-to-HAL shim");
MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
