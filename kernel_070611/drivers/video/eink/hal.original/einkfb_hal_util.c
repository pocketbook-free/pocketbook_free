/*
 *  linux/drivers/video/eink/hal/einkfb_hal_util.c -- eInk frame buffer device HAL utilities
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "einkfb_hal.h"

#if PRAGMAS
    #pragma mark Definitions & Globals
    #pragma mark -
#endif

extern einkfb_power_level EINKFB_GET_POWER_LEVEL(void);
extern void EINKFB_SET_POWER_LEVEL(einkfb_power_level power_level);

// 1bpp -> 2bpp
//
// 0 0 0 0 -> 00 00 00 00   0x0 -> 0x00
// 0 0 0 1 -> 00 00 00 11   0x1 -> 0x03
// 0 0 1 0 -> 00 00 11 00   0x2 -> 0x0C
// 0 0 1 1 -> 00 00 11 11   0x3 -> 0x0F
// 0 1 0 0 -> 00 11 00 00   0x4 -> 0x30
// 0 1 0 1 -> 00 11 00 11   0x5 -> 0x33
// 0 1 1 0 -> 00 11 11 00   0x6 -> 0x3C
// 0 1 1 1 -> 00 11 11 11   0x7 -> 0x3F
// 1 0 0 0 -> 11 00 00 00   0x8 -> 0xC0
// 1 0 0 1 -> 11 00 00 11   0x9 -> 0xC3
// 1 0 1 0 -> 11 00 11 00   0xA -> 0xCC
// 1 0 1 1 -> 11 00 11 11   0xB -> 0xCF
// 1 1 0 0 -> 11 11 00 00   0xC -> 0xF0
// 1 1 0 1 -> 11 11 00 11   0xD -> 0xF3
// 1 1 1 0 -> 11 11 11 00   0xE -> 0xFC
// 1 1 1 1 -> 11 11 11 11   0xF -> 0xFF
//
static u8 stretch_nybble_table_1_to_2bpp[16] =
{
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

// 2bpp -> 4bpp
//
// 00 00 -> 0000 0000       0x0 -> 0x00
// 00 01 -> 0000 0101       0x1 -> 0x05
// 00 10 -> 0000 1010       0x2 -> 0x0A
// 00 11 -> 0000 1111       0x3 -> 0x0F
// 01 00 -> 0101 0000       0x4 -> 0x50
// 01 01 -> 0101 0101       0x5 -> 0x55
// 01 10 -> 0101 1010       0x6 -> 0x5A
// 01 11 -> 0101 1111       0x7 -> 0x5F
// 10 00 -> 1010 0000       0x8 -> 0xA0
// 10 01 -> 1010 0101       0x9 -> 0xA5
// 10 10 -> 1010 1010       0xA -> 0xAA
// 10 11 -> 1010 1111       0xB -> 0xAF
// 11 00 -> 1111 0000       0xC -> 0xF0
// 11 01 -> 1111 0101       0xD -> 0xF5
// 11 10 -> 1111 1010       0xE -> 0xFA
// 11 11 -> 1111 1111       0xF -> 0xFF
//
static u8 stretch_nybble_table_2_to_4bpp[16] =
{
    0x00, 0x05, 0x0A, 0x0F, 0x50, 0x55, 0x5A, 0x5F,
    0xA0, 0xA5, 0xAA, 0xAF, 0xF0, 0xF5, 0xFA, 0xFF
};

// 4bpp -> 8bpp
//
// 0000 -> 00000000         0x0 -> 0x00
// 0001 -> 00010001         0x1 -> 0x11
// 0010 -> 00100010         0x2 -> 0x22
// 0011 -> 00110011         0x3 -> 0x33
// 0100 -> 01000100         0x4 -> 0x44
// 0101 -> 01010101         0x5 -> 0x55
// 0110 -> 01100110         0x6 -> 0x66
// 0111 -> 01110111         0x7 -> 0x77
// 1000 -> 10001000         0x8 -> 0x88
// 1001 -> 10011001         0x9 -> 0x99
// 1010 -> 10101010         0xA -> 0xAA
// 1011 -> 10111011         0xB -> 0xBB
// 1100 -> 11001100         0xC -> 0xCC
// 1101 -> 11011101         0xD -> 0xDD
// 1110 -> 11101110         0xE -> 0xEE
// 1111 -> 11111111         0xF -> 0xFF
//
static u8 stretch_nybble_table_4_to_8bpp[16] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

// 1bpp
//
//  0   0 0 0 0 0 0 0 0     0x00
//  1   1 1 1 1 1 1 1 1     0xFF
//
static u8 gray_table_1bpp[2] =
{
    0x00, 0xFF
};

// 2bpp
//
//  0   00 00 00 00         0x00
//  1   01 01 01 01         0x55
//  2   10 10 10 10         0xAA
//  3   11 11 11 11         0xFF
//
static u8 gray_table_2bpp[4] =
{
    0x00, 0x55, 0xAA, 0xFF
};

// 4bpp
//
//  0   0000 0000           0x00
//  1   0001 0001           0x11
//  2   0010 0010           0x22
//  3   0011 0011           0x33
//  4   0100 0100           0x44
//  5   0101 0101           0x55
//  6   0110 0110           0x66
//  7   0111 0111           0x77    
//  8   1000 1000           0x88
//  9   1001 1001           0x99
// 10   1010 1010           0xAA
// 11   1011 1011           0xBB
// 12   1100 1100           0xCC
// 13   1101 1101           0xDD
// 14   1110 1110           0xEE
// 15   1111 1111           0xFF
//
static u8 gray_table_4bpp[16] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

einkfb_power_level saved_power_level = einkfb_power_level_init;
static DECLARE_MUTEX(einkfb_lock);
static DEFINE_MUTEX(einkfb_ioctrl_lock);
#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilites
    #pragma mark -
#endif

struct vfb_blit_t
{
    bool buffers_equal;
    u8 *src, *dst;
};
typedef struct vfb_blit_t vfb_blit_t;

static void einkfb_vfb_blit(int x, int y, int rowbytes, int bytes, void *data)
{
    vfb_blit_t *vfb_blit = (vfb_blit_t *)data;
    int dst_bytes  = (rowbytes * y) + x;
    u8  old_pixels = vfb_blit->dst[dst_bytes];
    
    vfb_blit->dst[dst_bytes] = vfb_blit->src[bytes];
    vfb_blit->buffers_equal &= old_pixels == vfb_blit->dst[dst_bytes];
}

static bool einkfb_update_vfb_area(update_area_t *update_area)
{
    // If we get here, the update_area has already been validated.  So, all we
    // need to do is blit things into the virtual framebuffer at the right
    // spot.
    //
    u32 x_start, x_end, area_xres,
        y_start, y_end, area_yres;

    u8  *fb_start, *area_start;
    
    vfb_blit_t vfb_blit;
    
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    area_xres  = update_area->x2 - update_area->x1;
    area_yres  = update_area->y2 - update_area->y1;
    area_start = update_area->buffer;
    
    fb_start   = info.vfb;

    // Get the (x1, y1)...(x2, y2) offset into the framebuffer.
    //
    x_start = update_area->x1;
    x_end   = x_start + area_xres;
    
    y_start = update_area->y1;
    y_end   = y_start + area_yres;
    
    // Blit the area data into the virtual framebuffer.
    //
    vfb_blit.buffers_equal = true;
    vfb_blit.src = area_start;
    vfb_blit.dst = fb_start;
    
    einkfb_blit(x_start, x_end, y_start, y_end, einkfb_vfb_blit, (void *)&vfb_blit);

    // Say that an update-display event has occurred if the buffers aren't equal.
    //
    if ( !vfb_blit.buffers_equal )
    {
        einkfb_event_t event; einkfb_init_event(&event);
        
        event.update_mode = update_area->which_fx;
        event.x1 = update_area->x1; event.x2 = update_area->x2;
        event.y1 = update_area->y1; event.y2 = update_area->y2;
        event.event = einkfb_event_update_display_area;
        
        einkfb_post_event(&event);
    }
    
    return ( vfb_blit.buffers_equal );
}

static bool einkfb_update_vfb(fx_type update_mode)
{
    bool buffers_equal = true;
    
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    // Copy the real framebuffer into the virtual framebuffer if we're
    // not doing a restore.
    //
    if ( EINKFB_RESTORE(info) )
    {
        // On restores, say that the buffers aren't equal to force
        // an update.
        //
        buffers_equal = false;
    }
    else
    {    
        fb_apply_fx_t fb_apply_fx = get_fb_apply_fx();
        int i, len;

        if ( fb_apply_fx )
        {
            u8 old_pixels, *vfb = info.vfb, *fb = info.start;
            len = info.size;
            
            for ( i = 0; i < len; i++ )
            {
                old_pixels = vfb[i];
                vfb[i] = fb_apply_fx(fb[i], i);
                
                buffers_equal &= old_pixels == vfb[i];
                EINKFB_SCHEDULE_BLIT(i+1);
            }
        }
        else
        {
            u32 old_pixels, *vfb = (u32 *)info.vfb, *fb = (u32 *)info.start;
            len = info.size >> 2;
            
            for ( i = 0; i < len; i++ )
            {
                old_pixels = vfb[i];
                vfb[i] = fb[i];
                
                buffers_equal &= old_pixels == vfb[i];
            }
            
            if ( EINKFB_MEMCPY_MIN < i )
                EINKFB_SCHEDULE();
        }
    }

    // Say that an update-display event has occurred if the buffers aren't equal.
    //
    if ( !buffers_equal )
    {
        einkfb_event_t event; einkfb_init_event(&event);

        event.event = einkfb_event_update_display;
        event.update_mode = update_mode;
    
        einkfb_post_event(&event);
    }
    
    return ( buffers_equal );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Locking Utilities
    #pragma mark -
#endif

static int einkfb_begin_power_on_activity(void)
{
    int result = EINKFB_FAILURE;
    struct einkfb_info info;
    
    einkfb_get_info(&info);

    if ( info.dev )
    {
        // Save the current power level.
        //
        saved_power_level = EINKFB_GET_POWER_LEVEL();
           
        // Only power on if it's allowable to do so at the moment.
        //
        if ( POWER_OVERRIDE() )
        {
            result = EINKFB_SUCCESS;
        }
        else
        {
            switch ( saved_power_level )
            {
                case einkfb_power_level_init:
                    saved_power_level = einkfb_power_level_on;

                case einkfb_power_level_on:
                case einkfb_power_level_standby:
                    einkfb_prime_power_timer(EINKFB_DELAY_TIMER);
                case einkfb_power_level_blank:
                    result = EINKFB_SUCCESS;
                break;
                
                default:
                    einkfb_debug_power("power (%s) lockout, ignoring call\n",
                        einkfb_get_power_level_string(saved_power_level));
                    
                    result = EINKFB_FAILURE;
                break;
            }
        }

        // Power on if need be and allowable.
        //
        if ( EINKFB_SUCCESS == result )
        {
            einkfb_set_jif_on(jiffies);
            
            if ( einkfb_power_level_on != saved_power_level )
                EINKFB_SET_POWER_LEVEL(einkfb_power_level_on);
        }
    }
    
    return ( result );
}

static void einkfb_end_power_on_activity(void)
{
    struct einkfb_info info; einkfb_get_info(&info);

    if ( info.dev )
    {
        einkfb_power_level power_level = EINKFB_GET_POWER_LEVEL();
        
        // Only restore the saved power level if we haven't been purposely
        // taken out of the "on" level.
        //
        if ( einkfb_power_level_on == power_level )
            EINKFB_SET_POWER_LEVEL(saved_power_level);
        else
            saved_power_level = power_level;
    }
}

bool einkfb_lock_ready(bool release)
{
    bool ready = false;
    
    if ( release )
    {
        ready = EINKFB_SUCCESS == down_trylock(&einkfb_lock);
        
        if ( ready )
            einkfb_lock_release();
    }
    else
        ready = EINKFB_SUCCESS == down_interruptible(&einkfb_lock);
    
    return ( ready );
}

void einkfb_lock_release(void)
{
    up(&einkfb_lock);
} 

/* Henry: decrease einkfb_ioctrl_lock*/
void einkfb_block(void)
{
    mutex_lock(&einkfb_ioctrl_lock);	
}

/* Henry: increase einkfb_ioctrl_lock */
void einkfb_unblock(void)
{
     mutex_unlock(&einkfb_ioctrl_lock);
}
int einkfb_lock_entry(char *function_name)
{
    int result = EINKFB_FAILURE;
    
    if ( EINK_DISPLAY_ASIS() )
    {
        einkfb_debug_power("%s: display asis, ignoring call\n", function_name);
    }
    else
    {
        einkfb_debug_lock("%s: getting power lock...\n", function_name);
    
        if ( EINKFB_LOCK_READY() )
        {
            einkfb_debug_lock("%s: got lock, getting power...\n", function_name);
            result = einkfb_begin_power_on_activity();
    
            if ( EINKFB_SUCCESS == result )
            {
                einkfb_debug_lock("%s: got power...\n", function_name);
            }
            else
            {
                einkfb_debug_lock("%s: could not get power, releasing lock\n", function_name);
                EINFFB_LOCK_RELEASE();
            }
        }
        else
        {
            einkfb_debug_lock("%s: could not get lock, bailing\n", function_name);
        }
    }

	return ( result );
}

void einkfb_lock_exit(char *function_name)
{
    if ( EINK_DISPLAY_ASIS() )
    {
        einkfb_debug_power("%s: display asis, ignoring call\n", function_name);
    }
    else
    {
        einkfb_end_power_on_activity();
        EINFFB_LOCK_RELEASE();
    
        einkfb_debug_lock("%s released power, released lock\n", function_name);
    }
}

int einkfb_schedule_timeout(unsigned long hardware_timeout, einkfb_hardware_ready_t hardware_ready, void *data, bool interruptible)
{
    int result = EINKFB_SUCCESS;
    
    if ( hardware_timeout && hardware_ready )
    {
        unsigned long start_time = jiffies, stop_time = start_time + hardware_timeout,
            timeout = EINKFB_TIMEOUT_MIN;

        // Ask the hardware whether it's ready or not.  And, if it's not ready, start
        // yielding the CPU for EINKFB_TIMEOUT_MIN jiffies, increasing the yield time
        // up to EINKFB_TIMEOUT_MAX jiffies.  Time out after the requested number of
        // of jiffies have occurred.
        //
        while ( !(*hardware_ready)(data) && time_before_eq(jiffies, stop_time) )
        {
            timeout = min(timeout++, EINKFB_TIMEOUT_MAX);
            
            if ( interruptible )
                schedule_timeout_interruptible(timeout);
            else
                schedule_timeout(timeout);
        }

        if ( time_after(jiffies, stop_time) )
        {
           einkfb_print_crit("Timed out waiting for the hardware to become ready!\n");
           result = EINKFB_FAILURE;
        }
        else
        {
            // For debugging purposes, dump the time it took for the hardware to
            // become ready if it was more than EINKFB_TIMEOUT_MAX.
            //
            stop_time = jiffies - start_time;
            
            if ( EINKFB_TIMEOUT_MAX < stop_time )
                einkfb_debug("Timeout time = %ld\n", stop_time);
        }
    }
    else
    {
        // Yield the CPU with schedule.
        //
        einkfb_debug("Yielding CPU with schedule.\n");
        schedule();
    }
    
    return ( result );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Display Utilities
    #pragma mark -
#endif

void einkfb_blit(int xstart, int xend, int ystart, int yend, einkfb_blit_t blit, void *data)
{
    if ( blit )
    {
        int x, y, rowbytes, bytes;
        
        struct einkfb_info info;
        einkfb_get_info(&info);
    
        // Make bpp-related adjustments.
        //
        xstart   = BPP_SIZE(xstart,    info.bpp);
        xend     = BPP_SIZE(xend,	   info.bpp);
        rowbytes = BPP_SIZE(info.xres, info.bpp);
    
        // Blit EINKFB_MEMCPY_MIN bytes at a time before yielding.
        //
        for (bytes = 0, y = ystart; y < yend; y++ )
        {
            for ( x = xstart; x < xend; x++ )
            {
                (*blit)(x, y, rowbytes, bytes, data);
                EINKFB_SCHEDULE_BLIT(++bytes);
            }
        }
    }
}

static einkfb_bounds_failure bounds_failure = einkfb_bounds_failure_none;

einkfb_bounds_failure einkfb_get_last_bounds_failure(void)
{
    return ( bounds_failure );
}

bool einkfb_bounds_are_acceptable(int xstart, int xres, int ystart, int yres)
{
    bool acceptable = true;
    int alignment;
    
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    bounds_failure = einkfb_bounds_failure_none;
    alignment = BPP_BYTE_ALIGN(info.align);

    // The limiting boundary must be of non-zero size and be within the screen's boundaries.
    //
    acceptable &= ((0 < xres) && IN_RANGE(xres, 0, info.xres));
    
    if ( !acceptable )
        bounds_failure |= einkfb_bounds_failure_x1;

    acceptable &= ((0 < yres) && IN_RANGE(yres, 0, info.yres));
    
    if ( !acceptable )
        bounds_failure |= einkfb_bounds_failure_y1;
    
    // The bounds must be and fit within the framebuffer.
    //
    acceptable &= ((0 <= xstart) && ((xstart + xres) <= info.xres));

    if ( !acceptable )
        bounds_failure |= einkfb_bounds_failure_x2;

    acceptable &= ((0 <= ystart) && ((ystart + yres) <= info.yres));
    
    if ( !acceptable )
        bounds_failure |= einkfb_bounds_failure_y2;

    // Our horizontal starting and ending points must be byte aligned.
    //
    acceptable &= ((0 == (xstart % alignment)) && (0 == (xres % alignment)));
    
    if ( !acceptable )
        bounds_failure |= einkfb_bounds_failure_x;
        
    // If debugging is enabled, print out the apparently errant
    // passed-in values.
    //
    if ( !acceptable )
    {
        einkfb_debug("Image couldn't be rendered:\n");
        einkfb_debug(" Screen bounds:         %4d x %4d\n", info.xres, info.yres);
        einkfb_debug(" Image resolution:      %4d x %4d\n", xres, yres);
        einkfb_debug(" Image offset:          %4d x %4d\n", xstart, ystart);
        einkfb_debug(" Image row start align: %4d\n",       (xstart % alignment));
        einkfb_debug(" Image width align:     %4d\n",       (xres % alignment));
	}

	return ( acceptable );
}

bool einkfb_align_bounds(rect_t *rect)
{
    int xstart = rect->x1, xend = rect->x2,
        ystart = rect->y1, yend = rect->y2,
        
        xres = xend - xstart,
        yres = yend - ystart,
        
        alignment;
    
    struct einkfb_info info;
    bool aligned = false;
    
    einkfb_get_info(&info);
    alignment = BPP_BYTE_ALIGN(info.align);
    
    // Only re-align the bounds that aren't aligned.
    //
    if ( 0 != (xstart % alignment) )
    {
        xstart = BPP_BYTE_ALIGN_DN(xstart, info.align);
        xres = xend - xstart;
    }

    if ( 0 != (xres % alignment) )
    {
        xend = BPP_BYTE_ALIGN_UP(xend, info.align);
        xres = xend - xstart;
    }

    // If the re-aligned bounds are acceptable, use them.
    //
    if ( einkfb_bounds_are_acceptable(xstart, xres, ystart, yres) )
    {
        einkfb_debug("x bounds re-aligned, x1: %d -> %d; x2: %d -> %d\n",
            rect->x1, xstart, rect->x2, xend);
        
        rect->x1 = xstart;
        rect->x2 = xend;
        
        aligned = true;
    }
    
    return ( aligned );
}

unsigned char einkfb_stretch_nybble(unsigned char nybble, unsigned long bpp)
{
    unsigned char *which_nybble_table = NULL, result = nybble;

    switch ( nybble )
    {
        // Special-case the table endpoints since they're always the same.
        //
        case 0x00:
            result = EINKFB_WHITE;
        break;
        
        case 0x0F:
            result = EINKFB_BLACK;
        break;
        
        // Handle everything else on a bit-per-pixel basis.
        //
        default:
            switch ( bpp )
            {
                case EINKFB_1BPP:
                    which_nybble_table = stretch_nybble_table_1_to_2bpp;
                break;
                
                case EINKFB_2BPP:
                    which_nybble_table = stretch_nybble_table_2_to_4bpp;
                break;
                
                case EINKFB_4BPP:
                    which_nybble_table = stretch_nybble_table_4_to_8bpp;
                break;
            }
            
            if ( which_nybble_table )
                result = which_nybble_table[nybble];
        break;
    }

    return ( result );
}

void einkfb_display_grayscale_ramp(void)
{
    int row, num_rows, row_bytes, row_size, row_height, height, adj_count, adj_start;
    u8 row_gray, *gray_table = NULL, *row_start = NULL;
    struct einkfb_info info;

    einkfb_get_info(&info);
    
    // Get the table that translates pixels into bytes per bit depth.
    //
    switch ( info.bpp )
    {
        case EINKFB_1BPP:
            gray_table = gray_table_1bpp;
        break;
        
        case EINKFB_2BPP:
            gray_table = gray_table_2bpp;
        break;
        
        case EINKFB_4BPP:
            gray_table = gray_table_4bpp;
        break;
    }
    
    // Divide the display into the appropriate number of rows.
    //
    row_bytes = BPP_SIZE(info.xres, info.bpp);
    num_rows  = 1 << info.bpp;
    row_start = info.start;
    
    // In order to run the rows to the end, we need to adjust
    // row_height adj_count times.  We do this by "centering"
    // adj_count to the num_rows.  We're zero-based (i), so we
    // don't add 1 to adj_start here
    //
    row_height = info.yres / num_rows;
    adj_count  = info.yres % num_rows;
    adj_start  = adj_count ? (num_rows - adj_count) >> 1 : 0;
    
    for ( row = 0; row < num_rows; row++ )
    {
        // Determine height.
        //
        height = row_height;
        
        // Quit adjusting height when/if adj_count is zero or
        // less (as noted above, adj_count is "centered" to
        // num_rows).
        //
        if ( 0 < adj_count )
        {
            if ( row >= adj_start )
            {
                adj_count--;
                height++;
            }
        }
        
        // Blit the appropriate gray into the framebuffer for
        // this row.
        //
        row_gray = gray_table ? gray_table[row] : (u8)row;
        row_size = row_bytes * height;
        
        einkfb_memset(row_start, row_gray, row_size);
        row_start += row_size;
    }

    // Now, update the display with our grayscale ramp.
    //
    einkfb_update_display(fx_update_full);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Update Utilities
    #pragma mark -
#endif

#define UPDATE_TIMING_TOTL_TYPE 0
#define UPDATE_TIMING_VIRT_TYPE 1
#define UPDATE_TIMING_REAL_TYPE 2

#define UPDATE_TIMING_ID_LEN    32

#define UPDATE_TIMING           "update_timing_%s"
#define UPDATE_TIMING_TOTL_NAME "totl"
#define UPDATE_TIMING_VIRT_NAME "virt"
#define UPDATE_TIMING_REAL_NAME "real"

#define UPDATE_TIMING_AREA      "area"
#define UPDATE_TIMING_DISP      "disp"

static void einkfb_print_update_timing(unsigned long time, int which, char *kind)
{
    char *name = NULL, id[UPDATE_TIMING_ID_LEN];

    sprintf(id, UPDATE_TIMING, kind);
    
    switch ( which )
    {
        case UPDATE_TIMING_VIRT_TYPE:
            name = UPDATE_TIMING_VIRT_NAME;
        goto relative_common;
        
        case UPDATE_TIMING_REAL_TYPE:
            name = UPDATE_TIMING_REAL_NAME;
        /* goto relative_common; */
        
        relative_common:
            EINKFB_PRINT_PERF_REL(id, time, name);
        break;
        
        case UPDATE_TIMING_TOTL_TYPE:
            EINKFB_PRINT_PERF_ABS(id, time, UPDATE_TIMING_TOTL_NAME);
        break;
    }
}

void einkfb_update_display_area(update_area_t *update_area)
{
    if ( update_area )
    {
        unsigned long strt_time = jiffies, virt_strt = strt_time, virt_stop,
                      real_strt = 0, real_stop = 0, stop_time;
        
        // Update the virtual display.
        //
        bool buffers_equal = einkfb_update_vfb_area(update_area);
        virt_stop = jiffies;
        
        // Update the real display if the buffer actually changed.
        //
        if ( !buffers_equal && hal_ops.hal_update_area && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
        {
            real_strt = jiffies;
            hal_ops.hal_update_area(update_area);
            real_stop = jiffies;
            
            EINKFB_LOCK_EXIT();
        }
        
        stop_time = jiffies;
        
        if ( buffers_equal )
            einkfb_debug("buffers equal; hardware not accessed\n");
            
        if ( EINKFB_PERF() )
        {
            einkfb_print_update_timing(jiffies_to_msecs(virt_stop - virt_strt),
                UPDATE_TIMING_VIRT_TYPE, UPDATE_TIMING_AREA);
            
            if ( real_strt )
                einkfb_print_update_timing(jiffies_to_msecs(real_stop - real_strt),
                    UPDATE_TIMING_REAL_TYPE, UPDATE_TIMING_AREA);
        
            einkfb_print_update_timing(jiffies_to_msecs(stop_time - strt_time),
                UPDATE_TIMING_TOTL_TYPE, UPDATE_TIMING_AREA);
        }
    }
}

void einkfb_update_display(fx_type update_mode)
{
    unsigned long strt_time = jiffies, virt_strt = strt_time, virt_stop,
                  real_strt = 0, real_stop = 0,  stop_time;

    // Update the virtual display.
    //
    bool buffers_equal  = einkfb_update_vfb(update_mode),
         cancel_restore = false;

    virt_stop = jiffies;

    // If the buffers aren't the same, check to see whether we're doing a restore.
    //
    if ( !buffers_equal )
    {
        struct einkfb_info info;
        einkfb_get_info(&info);
        
        // If we're not doing a restore but we applied an FX, force a restore so
        // that the module doesn't have to apply the FX.
        //
        if ( !EINKFB_RESTORE(info) )
        {
            fb_apply_fx_t fb_apply_fx = get_fb_apply_fx();
            
            if ( fb_apply_fx )
            {
                einkfb_set_fb_restore(true);
                cancel_restore = true;
            }
        }
    }
    
    // Update the real display if the buffer actually changed.
    //
    if ( !buffers_equal && hal_ops.hal_update_display && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
    {
        real_strt = jiffies;
        hal_ops.hal_update_display(update_mode);
        real_stop = jiffies;
        
        EINKFB_LOCK_EXIT();
    }

    stop_time = jiffies;

    // Take us out of restore mode if we put ourselves there.
    //
    if ( cancel_restore )
        einkfb_set_fb_restore(false);

    if ( buffers_equal )
        einkfb_debug("buffers equal; hardware not accessed\n");

    if ( EINKFB_PERF() )
    {
        einkfb_print_update_timing(jiffies_to_msecs(virt_stop - virt_strt),
            UPDATE_TIMING_VIRT_TYPE, UPDATE_TIMING_DISP);
        
        if ( real_strt )
            einkfb_print_update_timing(jiffies_to_msecs(real_stop - real_strt),
                UPDATE_TIMING_REAL_TYPE, UPDATE_TIMING_DISP);
    
        einkfb_print_update_timing(jiffies_to_msecs(stop_time - strt_time),
            UPDATE_TIMING_TOTL_TYPE, UPDATE_TIMING_DISP);
    }
}

void einkfb_restore_display(fx_type update_mode)
{
    // Switch the real display to the virtual one.
    //
    einkfb_set_fb_restore(true);
    
    // Update the real display with the virtual one's data.
    //
    einkfb_update_display(update_mode); 
    
    // Switch back to the real display.
    //
    einkfb_set_fb_restore(false);
}

void einkfb_clear_display(fx_type update_mode)
{
    struct einkfb_info info; einkfb_get_info(&info);
    einkfb_memset(info.start, EINKFB_WHITE, info.size);
    
    einkfb_update_display(update_mode);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Orientation Utilities
    #pragma mark -
#endif

bool einkfb_set_display_orientation(orientation_t orientation)
{
    bool result = false;
    
    if ( hal_ops.hal_set_display_orientation && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
    {
        result = hal_ops.hal_set_display_orientation(orientation);
        EINKFB_LOCK_EXIT();
        
        // Post a rotate event if setting display orientation succeeded.
        //
        if ( result )
        {
            einkfb_event_t event;
            einkfb_init_event(&event);
            
            event.event = einkfb_event_rotate_display;
            event.orientation = orientation;
            
            einkfb_post_event(&event);
        }
    }

    return ( result );
}

orientation_t einkfb_get_display_orientation(void)
{
    orientation_t orientation = orientation_portrait;

    if ( hal_ops.hal_get_display_orientation && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
    {
        orientation = hal_ops.hal_get_display_orientation();
        EINKFB_LOCK_EXIT();
    }
    else
    {
        struct einkfb_info info; einkfb_get_info(&info);
        
        if ( EINK_ORIENT_LANDSCAPE == ORIENTATION(info.xres, info.yres) )
            orientation = orientation_landscape;
    }

    return ( orientation );
}





#if PRAGMAS
    #pragma mark -
    #pragma mark Exports
    #pragma mark -
#endif

EXPORT_SYMBOL(einkfb_lock_ready);
EXPORT_SYMBOL(einkfb_lock_release);
EXPORT_SYMBOL(einkfb_lock_entry);
EXPORT_SYMBOL(einkfb_lock_exit);
EXPORT_SYMBOL(einkfb_schedule_timeout);
EXPORT_SYMBOL(einkfb_blit);
EXPORT_SYMBOL(einkfb_stretch_nybble);
EXPORT_SYMBOL(einkfb_get_last_bounds_failure);
EXPORT_SYMBOL(einkfb_bounds_are_acceptable);
EXPORT_SYMBOL(einkfb_align_bounds);

