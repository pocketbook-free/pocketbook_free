/*
 *  linux/drivers/video/eink/stephanie/stephanie_hal.c
 *  -- eInk frame buffer device HAL stephanie
 *
 *      Copyright (C) 2005-200* Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "../apollo/apollo.h"

#if PRAGMAS
    #pragma mark Definitions
#endif

// For now, default Stephanie to a resolution that's compatible with Apollo.
//
#define XRES            600
#define YRES            800
#define BPP             2

#define STEPHANIE_SIZE  BPP_SIZE((XRES*YRES), BPP)

static struct fb_var_screeninfo stephanie_var __INIT_DATA =
{
    .xres               = XRES,
    .yres               = YRES,
    .xres_virtual       = XRES,
    .yres_virtual       = YRES,
    .bits_per_pixel     = BPP,
    .grayscale          = 1,
    .activate           = FB_ACTIVATE_TEST,
    .height             = -1,
    .width              = -1,
};

static struct fb_fix_screeninfo stephanie_fix __INIT_DATA =
{
    .id                 = EINKFB_NAME,
    .smem_len           = STEPHANIE_SIZE,
    .type               = FB_TYPE_PACKED_PIXELS,
    .visual             = FB_VISUAL_STATIC_PSEUDOCOLOR,
    .xpanstep           = 0,
    .ypanstep           = 0,
    .ywrapstep          = 0,
    .line_length        = BPP_SIZE(XRES, BPP),
    .accel              = FB_ACCEL_NONE,
};

int g_eink_power_mode = 0;
int flash_fail_count = 0;

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

static void stephanie_update_area_guts(unsigned char *buffer, int buffer_size, int xstart, int ystart, int xend, int yend)
{
    bool area_cleared = (bool)apollo_load_partial_data(buffer, buffer_size, xstart, ystart, xend, yend) &&
                        (bool)get_screen_clear();
    
    if ( !area_cleared )
        apollo_display_partial();
        
    set_screen_clear(area_cleared);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL module operations.
    #pragma mark -
#endif

static bool stephanie_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    *var = stephanie_var;
    *fix = stephanie_fix;
    
    return ( EINKFB_SUCCESS );
}

static bool stephanie_hw_init(struct fb_info *info, bool full)
{
    return ( (bool)apollo_init_kernel(FULL_BRINGUP) );
}

static void stephanie_hw_done(bool full)
{
    apollo_done_kernel((bool)full);
}

static void stephanie_update_display(fx_type update_mode)
{
    bool screen_cleared;
    
	struct einkfb_info info;
	einkfb_get_info(&info);
    
    screen_cleared = (bool)apollo_load_image_data(info.start, info.size);
    
    if ( !screen_cleared || (screen_cleared && !get_screen_clear()) )
    {
        set_screen_clear(screen_cleared);
        apollo_display(UPDATE_MODE(update_mode));
    }
}

static void stephanie_update_area(update_area_t *update_area)
{
    int xstart = update_area->x1, xend = update_area->x2,
        ystart = update_area->y1, yend = update_area->y2,
        
        xres = xend - xstart,
        yres = yend - ystart,
        
        buffer_size;
        
	struct einkfb_info info;
	einkfb_get_info(&info);
        
    buffer_size = BPP_SIZE((xres * yres), info.bpp);
        
    stephanie_update_area_guts(update_area->buffer, buffer_size, xstart, ystart, xend, yend);
}

static struct einkfb_hal_ops_t stephanie_hal_ops =
{
    .hal_sw_init        = stephanie_sw_init,
    
    .hal_hw_init        = stephanie_hw_init,
    .hal_hw_done        = stephanie_hw_done,
    
    .hal_update_display = stephanie_update_display,
    .hal_update_area    = stephanie_update_area,
};

static __INIT_CODE int stephanie_hal_init(void)
{
    return ( einkfb_hal_ops_init(&stephanie_hal_ops) );
}

module_init(stephanie_hal_init);

#ifdef MODULE
static void stephanie_hal_exit(void)
{
    einkfb_hal_ops_done();
}

module_exit(stephanie_hal_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif // MODULE


