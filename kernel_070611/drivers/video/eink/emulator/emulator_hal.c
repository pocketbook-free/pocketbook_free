/*
 *  linux/drivers/video/eink/emulator/emulator_hal.c
 *  -- eInk frame buffer device HAL emulator
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"

// Default to Turing's resolution:  600x800@4bpp.
//
#define XRES_6          600
#define YRES_6          800

#define XRES_9          824
#define YRES_9          1200

#define BPP             EINKFB_4BPP

#define EMULATOR_SIZE   BPP_SIZE((XRES_6*YRES_6), BPP)

#define IS_PORTRAIT()                           \
    (EINK_ORIENT_PORTRAIT  == emu_orientation)

#define IS_LANDSCAPE()                          \
    (EINK_ORIENT_LANDSCAPE == emu_orientation)

static struct fb_var_screeninfo emulator_var __INIT_DATA =
{
    .xres               = XRES_6,
    .yres               = YRES_6,
    .xres_virtual       = XRES_6,
    .yres_virtual       = YRES_6,
    .bits_per_pixel     = BPP,
    .grayscale          = 1,
    .activate           = FB_ACTIVATE_TEST,
    .height             = -1,
    .width              = -1,
};

static struct fb_fix_screeninfo emulator_fix __INIT_DATA =
{
    .id                 = EINKFB_NAME,
    .smem_len           = EMULATOR_SIZE,
    .type               = FB_TYPE_PACKED_PIXELS,
    .visual             = FB_VISUAL_STATIC_PSEUDOCOLOR,
    .xpanstep           = 0,
    .ypanstep           = 0,
    .ywrapstep          = 0,
    .line_length        = BPP_SIZE(XRES_6, BPP),
    .accel              = FB_ACCEL_NONE,
};

static int emulator_xres     = XRES_6;
static int emulator_yres     = YRES_6;

static unsigned long emu_bpp = BPP;
static int emu_orientation   = EINKFB_ORIENT_PORTRAIT;
static int emu_size          = 0;

#ifdef MODULE
module_param_named(emu_bpp, emu_bpp, long, S_IRUGO);
MODULE_PARM_DESC(emu_bpp, "1, 2, 4, or 8");

module_param_named(emu_orientation, emu_orientation, int, S_IRUGO);
MODULE_PARM_DESC(emu_orientation, "0 (portrait) or 1 (landscape)");

module_param_named(emu_size, emu_size, int, S_IRUGO);
MODULE_PARM_DESC(emu_size, "0 (default, 6-inch), 6 (6-inch), or 9 (9.7-inch)");
#endif // MODULE

#if PRAGMAS
    #pragma mark HAL module operations
    #pragma mark -
#endif

static bool emulator_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    // If we're overridden with a value other 6-inch or 9-inch, switch us
    // back to the default (6-inch).
    //
    switch ( emu_size )
    {
        case 6:
        default:
            emulator_xres = XRES_6;
            emulator_yres = YRES_6;
        break;
            
        case 9:
            emulator_xres = XRES_9;
            emulator_yres = YRES_9;
        break;
    }

    // If we're overridden with a value other than 1, 2, 4, or 8bpp, switch us
    // back to the default.
    //
    switch ( emu_bpp )
    {
        default:
            emu_bpp = BPP;
            
        case EINKFB_1BPP:
        case EINKFB_2BPP:
        case EINKFB_4BPP:
        case EINKFB_8BPP:
            emulator_var.bits_per_pixel = emu_bpp;
        break;
    }
    
    // If we're overridden with a value other than portrait or landscape, switch us
    // back to the default (portrait).
    //
    switch ( emu_orientation )
    {
        case EINKFB_ORIENT_PORTRAIT:
        default:
            emu_orientation = EINKFB_ORIENT_PORTRAIT;
        
            emulator_var.xres = emulator_var.xres_virtual = emulator_xres;
            emulator_var.yres = emulator_var.yres_virtual = emulator_yres;
        break;

        case EINKFB_ORIENT_LANDSCAPE:
            emulator_var.xres = emulator_var.xres_virtual = emulator_yres;
            emulator_var.yres = emulator_var.yres_virtual = emulator_xres;
        break;
    }
    
    emulator_fix.smem_len = BPP_SIZE((emulator_var.xres*emulator_var.yres), emu_bpp);
    emulator_fix.line_length = BPP_SIZE(emulator_var.xres, emu_bpp);

    *var = emulator_var;
    *fix = emulator_fix;
    
    return ( EINKFB_SUCCESS );
}

static bool emulator_set_display_orientation(orientation_t orientation)
{
    bool rotate = false;
    
    switch ( orientation )
    {
        case orientation_portrait:
        case orientation_portrait_upside_down:
            if ( !IS_PORTRAIT() )
            {
                emu_orientation = EINK_ORIENT_PORTRAIT;
                rotate = true;
            }
        break;
        
        case orientation_landscape:
        case orientation_landscape_upside_down:
            if ( !IS_LANDSCAPE() )
            {
                emu_orientation = EINK_ORIENT_LANDSCAPE;
                rotate = true;
            }
        break;
    }
    
    return ( rotate );
}

static orientation_t emulator_get_display_orientation(void)
{
    orientation_t orientation;
    
    switch ( emu_orientation )
    {
        case EINK_ORIENT_PORTRAIT:
        default:
             orientation = orientation_portrait;
        break;
        
        case EINK_ORIENT_LANDSCAPE:
            orientation = orientation_landscape;
        break;
    }
    
    return ( orientation );
}

static einkfb_hal_ops_t emulator_hal_ops =
{
    .hal_sw_init = emulator_sw_init,
    
    .hal_set_display_orientation = emulator_set_display_orientation,
    .hal_get_display_orientation = emulator_get_display_orientation
};

static __INIT_CODE int emulator_hal_init(void)
{
    return ( einkfb_hal_ops_init(&emulator_hal_ops) );
}

module_init(emulator_hal_init);

#ifdef MODULE
static void emulator_hal_exit(void)
{
    einkfb_hal_ops_done();
}

module_exit(emulator_hal_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif // MODULE

