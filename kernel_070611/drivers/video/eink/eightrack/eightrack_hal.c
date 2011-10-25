/*
 *  linux/drivers/video/eink/eightrack/eightrack_hal.c
 *  -- eInk frame buffer device HAL 8-Track
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "eightrack.h"

#if PRAGMAS
    #pragma mark Definitions
#endif

/* Private functions for displaying framebuffer configuration data */
static void display_fb_fixed_info(struct fb_fix_screeninfo *fixed_info);
static void display_fb_var_info(struct fb_var_screeninfo *var_info);

// Make 8-Track, for now, statically look like the original
// Apollo-based Fiona:  600x800@2bpp.
//
#define XRES                600
#define YRES                800
#define BPP                 EINKFB_2BPP

#define EIGHTRACK_SIZE      BPP_SIZE((XRES*YRES), BPP)
#define EIGHTRACKFB_SIZE    BPP_SIZE((XRES*YRES), EINKFB_8BPP)

#define EIGHTRACK_LCD_FB    1 // The HAL is at fb0 and the LCD is at fb1.

static struct fb_var_screeninfo eightrack_var __INIT_DATA =
{
    .xres                   = XRES,
    .yres                   = YRES,
    .xres_virtual           = XRES,
    .yres_virtual           = YRES,
    .bits_per_pixel         = BPP,
    .grayscale              = 1,
    .activate               = FB_ACTIVATE_TEST,
    .height                 = -1,
    .width                  = -1,
};

static struct fb_fix_screeninfo eightrack_fix __INIT_DATA =
{
    .id                     = EINKFB_NAME,
    .smem_len               = EIGHTRACK_SIZE,
    .type                   = FB_TYPE_PACKED_PIXELS,
    .visual                 = FB_VISUAL_STATIC_PSEUDOCOLOR,
    .xpanstep               = 0,
    .ypanstep               = 0,
    .ywrapstep              = 0,
    .line_length            = BPP_SIZE(XRES, BPP),
    .accel                  = FB_ACCEL_NONE,
};

static struct fb_info *lcdfb_info = NULL;

static unsigned char *lcdfb_start = NULL;   // LCD framebuffer's raw starting address.
static unsigned char *lcdfb_image = NULL;   // Image offset into LCD framebuffer.

static unsigned long lcdfb_xres   = 0;      // Total line length (rowbytes).
static unsigned long lcdfb_xact   = 0;      // Active part of line.
static unsigned long lcdfb_xclr   = 0;      // Blanking area.
static unsigned long lcdfb_yres   = 0;      // Total lines.
static unsigned long lcdfb_size   = 0;      // Raw size of LCD framebuffer.

static unsigned char *eightrackfb = NULL;   // 8-Track transfer buffer.

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

static void wait_eightrack_ready(void)
{
    while ( !is_eightrack_ready() )
        ;
}

static void prepare_eightrackfb(int x1, int y1, int x2, int y2)
{
    int src_start_i, src_start_j, src_end, src_xres,
        xstart, xend, xres, i, j, k, l, n, m, x;
    unsigned char s, *src, *dst, pixels[14];
    struct einkfb_info info;
    
    // HAL -> src[], 8-Track -> dst[].
    //
    einkfb_get_info(&info);
    dst = eightrackfb;
    src = info.vfb;
    
    // Determine src[] & dst[] bounds.
    //
    xres        = (x2 - x1) + 1;
    src_start_j = (info.xres * y1) + x1;
    
    src_start_i = BPP_SIZE(src_start_j, info.bpp);
    src_end     = BPP_SIZE(((info.xres * y2) + x2), info.bpp);
    src_xres    = BPP_SIZE(info.xres, info.bpp);
    
    xstart      = BPP_SIZE(x1,   info.bpp);
    xend        = BPP_SIZE(x2,   info.bpp);
    xres        = BPP_SIZE(xres, info.bpp);
    
    // i, n -> src[]; j -> dst[].
    //
    l = m = 0;
    
    for ( i = src_start_i, j = src_start_j, x = 0, n = src_end; i <= n; )
    {
        // First, invert the source pixels.
        //
        s = ~src[i];
        
        // Next, stretch the pixels to a byte.
        //
        switch ( info.bpp )
        {
            case EINKFB_4BPP:
                pixels[ 0] = (s & 0xF0);
                pixels[ 1] = (s & 0x0F) << 4;
                l = 0; m = 2;
            break;
            
            case EINKFB_2BPP:
                pixels[ 0] = STRETCH_HI_NYBBLE(s, EINKFB_2BPP);
                pixels[ 1] = STRETCH_LO_NYBBLE(s, EINKFB_2BPP);
                
                pixels[ 2] = (pixels[0] & 0xF0);
                pixels[ 3] = (pixels[0] & 0x0F) << 4;
                pixels[ 4] = (pixels[1] & 0xF0);
                pixels[ 5] = (pixels[1] & 0x0F) << 4;
                l = 2; m = 6;
            break;
            
            case EINKFB_1BPP:
                pixels[ 0] = STRETCH_HI_NYBBLE(s, EINKFB_1BPP);
                pixels[ 1] = STRETCH_LO_NYBBLE(s, EINKFB_1BPP);

                pixels[2] = STRETCH_HI_NYBBLE(pixels[0], EINKFB_2BPP);
                pixels[3] = STRETCH_LO_NYBBLE(pixels[0], EINKFB_2BPP);
                pixels[4] = STRETCH_HI_NYBBLE(pixels[1], EINKFB_2BPP);
                pixels[5] = STRETCH_LO_NYBBLE(pixels[1], EINKFB_2BPP);
                
                pixels[ 6] = (pixels[2] & 0xF0);
                pixels[ 7] = (pixels[2] & 0x0F) << 4;
                pixels[ 8] = (pixels[3] & 0xF0);
                pixels[ 9] = (pixels[3] & 0x0F) << 4;
                pixels[10] = (pixels[4] & 0xF0);
                pixels[11] = (pixels[4] & 0x0F) << 4;
                pixels[12] = (pixels[5] & 0xF0);
                pixels[13] = (pixels[5] & 0x0F) << 4;
                l = 6; m = 14;
            break;
        }

        // Now, copy the stretched pixels to the destination.
        //
        for ( k = l; k < m; k++ )
            dst[j + (k - l)] = pixels[k];

        // Finally, determine the next index for src[].
        //
        if ( 0 == (++x % xres) )
        {
            int skip_factor = (src_xres - xend) + xstart;
            
            j += skip_factor * (m - l);
            i += skip_factor;
        }
        else
        {
            j += m - l;
            i++;
        }
    }
}

#define FULL_SCREEN(X, Y, x, y) (((X) == (x)) && ((Y) == (y)))
#define ROTATE_90(r, x, y)      ((x) * (((y) - (r % (y))) - 1) + (r)/(y))

static int transfer_eightrackfb(int x1, int y1, int x2, int y2)
{
    int src_xres, src_yres, dst_start_i, dst_start_j, dst_xres_i,
        dst_xres_j, dst_end, xstart, xend, xres, ystart, yend, 
        yres, i, j, k, n, result = GU_WAVEFORM;
    bool buffers_equal = true, monochrome_only = true;
    unsigned char *src, *dst, s, d;   
    struct einkfb_info info;

    // src[] -> 8-Track, dst[] -> LCD.
    //
    einkfb_get_info(&info);
    dst = lcdfb_image;
    src = eightrackfb;
    
    // Determine src[] & dst[] bounds.
    //
    src_xres    = info.xres;
    src_yres    = info.yres;
    dst_xres_i  = lcdfb_xact;
    dst_xres_j  = lcdfb_xact + lcdfb_xclr;
    
    xstart      = (src_yres - 1) - y2;
    xend        = (src_yres - 1) - y1;
    ystart      = x1;
    yend        = x2;
    xres        = (xend - xstart) + 1;
    yres        = (yend - ystart) + 1;

    dst_start_i = (dst_xres_i * ystart) + xstart;
    dst_end     = (dst_xres_i * yend)   + xend;
    
    dst_start_j = (dst_xres_j * ystart) + xstart;

    // If we're not updating the entire display with new data,
    // ensure that the last screen's worth of data is preserved.
    //
    // Note:  Recall that the LCD is rotated 90° with respect
    //        to the HAL.
    //
    if ( !FULL_SCREEN(info.xres, info.yres, yres, xres) )
    {
        unsigned long  *lcdfb_base_32 = (unsigned long *)lcdfb_image,
                        lcdfb_size_32 = (lcdfb_size >> 2),
                        lcdfb_xact_32 = (lcdfb_xact >> 2),
                        lcdfb_xclr_32 = (lcdfb_xclr >> 2),
                        halfb_size_32 = (info.size  >> 2),
                        lcdfb_bits_32;

        for ( i = 0, j = 0, n = lcdfb_size_32; i < n; i++)
        {
            lcdfb_bits_32    = (lcdfb_base_32[i] & 0xF0F0F0F0);
            lcdfb_base_32[i] = (lcdfb_bits_32 | (lcdfb_bits_32 >> 4));

            // Skip the blanking part of each line.
            //
            if ( 0 == (++j % lcdfb_xact_32) )
                i += lcdfb_xclr_32;
                
            // Skip the blanking lines.
            //
            if ( halfb_size_32 < j )
                i = lcdfb_size_32;
        }
    }

    // i, n -> src[]; j -> dst[].
    //
    for ( i = dst_start_i, j = dst_start_j, k = 0, n = dst_end; i <= n; )
    {
        // Accumulate whether the buffers are equal.
        //
        s = src[ROTATE_90(i, src_xres, src_yres)]; // Already in "& 0xF0" format.
        d = dst[j] & 0xF0;
        
        buffers_equal &= (s == d);
        
        // Accumulate whether the pixels we're transferring are only monochrome.
        //
        d = s | (d >> 4);
        
        monochrome_only &= ((0x00 == d) || (0xF0 == d) || (0x0F == d) || (0xFF == d));

        // Accumulate the combined new (upper nybble) and old (lower nybble) pixels
        // into the active part of each line.
        //
        dst[j] = d;
        
        // Skip the blanking parts of each line.
        //
        if ( 0 == (++k % xres) )
        {
            i += (dst_xres_i - xend) + xstart;
            j += (dst_xres_j - xend) + xstart;
        }
        else
        {
            i++;
            j++;
        }
    }
    
    // If the new and old pixels are the same or the new pixels are only monochrome,
    // then say that we want a monochrome update.
    //
    einkfb_debug("buffers_equal = %d, monochrome_only = %d\n",
        buffers_equal, monochrome_only);
    
    if ( buffers_equal || monochrome_only )
        result = MU_WAVEFORM;
    
    return ( result );
}

static void update_display_eightrack(bool full, int x1, int y1, int x2, int y2)
{
    int update_type, xstart = x1, ystart = y1, xend = x2 - 1, yend = y2 - 1;
    
    // Prepare to transfer the eightrackfb into the lcdfb.
    //
    prepare_eightrackfb(xstart, ystart, xend, yend);
    
    // Transfer the eightrackfb to the lcdfb in the appropriate
    // way once the hardware is ready to receive it.
    //
    wait_eightrack_ready();
    update_type = transfer_eightrackfb(xstart, ystart, xend, yend);

    send_fb_to_eightrack(full ? GC_WAVEFORM : update_type);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL module operations
    #pragma mark -
#endif

static bool eightrack_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    bool result = EINKFB_FAILURE;
    
    // Ensure that 8-Track's corresponding LCD driver is available.
    //
    lcdfb_info = registered_fb[EIGHTRACK_LCD_FB];
    
    if ( lcdfb_info )
    {
        int image_offset = 0;
        
        // When debugging, display the LCD framebuffer's fixed and variable information.
        //
        if ( EINKFB_DEBUG() )
        {
            display_fb_fixed_info(&lcdfb_info->fix);
            display_fb_var_info(&lcdfb_info->var);
        }

        // Clear the LCD framebuffer to white and get the image offset.
        //
        // Note: 8-Track's pixels are inverted from the HAL's.
        //
        lcdfb_start = lcdfb_info->screen_base;
        lcdfb_size  = lcdfb_info->fix.smem_len;
        memset(lcdfb_start, ~EINKFB_WHITE, lcdfb_size);

        image_offset = eightrack_init(lcdfb_info);

        einkfb_debug("lcdfb_start  = 0x%08lX\n", (unsigned long)lcdfb_start);
        einkfb_debug("lcdfb_size   = %ld\n", lcdfb_size);
        einkfb_debug("image_offset = %d\n", image_offset);
 
        if ( 0 < image_offset )
        {
            // TODO:  Call einkfb_malloc() when it automatically handles
            //        vmalloc vs. kmalloc.
            //
            eightrackfb = vmalloc(EIGHTRACKFB_SIZE);

            if ( eightrackfb )
            {
                lcdfb_image = &lcdfb_start[image_offset];
                lcdfb_xres  = lcdfb_info->fix.line_length;
                lcdfb_xact  = YRES;                         // Note:  The LCD framebuffer is
                lcdfb_xclr  = lcdfb_xres - YRES;            //        rotated 90°.
                lcdfb_yres  = lcdfb_info->var.yres;
    
                *var = eightrack_var;
                *fix = eightrack_fix;
                
                result = EINKFB_SUCCESS;
            }
        }
    }

    return ( result );
}

static void eightrack_sw_done(void)
{
    // TODO:  Call einkfb_free() once moved to einkfb_malloc().
    //
    if ( eightrackfb )
        vfree(eightrackfb);
}

static bool eightrack_hw_init(struct fb_info *info, bool full)
{
    wait_eightrack_ready();
    send_fb_to_eightrack(INIT_WAVEFORM);

    return ( FULL_BRINGUP );
}

static void eightrack_hw_done(bool full)
{
    wait_eightrack_ready();
    eightrack_exit();
}

static void eightrack_update_display(fx_type update_mode)
{
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    // Say that we want to update the entire display.
    //
    update_display_eightrack(UPDATE_MODE(update_mode), 0, 0, info.xres, info.yres);
}

static void eightrack_update_area(update_area_t *update_area)
{
    // Update the display.
    //
    update_display_eightrack(fx_update_partial,
        update_area->x1, update_area->y1,
        update_area->x2, update_area->y2);
}

static struct einkfb_hal_ops_t eightrack_hal_ops =
{
    .hal_sw_init            = eightrack_sw_init,
    .hal_sw_done            = eightrack_sw_done,
    
    .hal_hw_init            = eightrack_hw_init,
    .hal_hw_done            = eightrack_hw_done,
    
    .hal_update_display     = eightrack_update_display,
    .hal_update_area        = eightrack_update_area
};

static __INIT_CODE int eightrack_hal_init(void)
{
    return ( einkfb_hal_ops_init(&eightrack_hal_ops) );
}

module_init(eightrack_hal_init);

#ifdef MODULE
static void eightrack_hal_exit(void)
{
    einkfb_hal_ops_done();
}

module_exit(eightrack_hal_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif // MODULE

#if PRAGMAS
    #pragma mark -
    #pragma mark LCD Framebuffer Debugging
    #pragma mark -
#endif

static void display_fb_fixed_info(struct fb_fix_screeninfo *fixed_info)
{
  /* Here are the values for type and visual (from "fb.h"). */
  char type[5][20] = {"PACKED_PIXELS",
                      "PLANES",
                      "INTERLEAVED_PLANES",
                      "TEXT",
                      "VGA_PLANES"};

  char visual[6][20] = {"MONO01",
                        "MONO10",
                        "TRUECOLOR",
                        "PSEUDOCOLOR",
                        "DIRECTCOLOR",
                        "STATIC_PSEUDOCOLOR"};

  /* Display fixed FB info. */
  printk("\nFixed framebuffer info\n"
         "    id:         %16s"       
         "    smem_start:       0x%08lX\n"  
         "    smem_len:         %10u"      
         "    type:   %20s\n"      
         "    type_aux:         %10u"      
         "    visual: %20s\n"      
         "    xpanstep:         %10u"      
         "    ypanstep:         %10u\n"      
         "    ywrapstep:        %10u"      
         "    line_length:      %10u\n"      
         "    mmio_start:       0x%08lx"  
         "    mmio_len:         %10u\n"      
         "    accel:            %10u\n",     
         fixed_info->id,           /* identification string eg "TT Builtin" */    
         fixed_info->smem_start,   /* Start of frame buffer mem */                
         fixed_info->smem_len,     /* Length of frame buffer mem */               
         type[fixed_info->type],   /* see FB_TYPE_* */                            
         fixed_info->type_aux,     /* Interleave for interleaved Planes */        
         visual[fixed_info->visual], /* see FB_VISUAL_* */                          
         fixed_info->xpanstep,     /* zero if no hardware panning  */             
         fixed_info->ypanstep,     /* zero if no hardware panning  */             
         fixed_info->ywrapstep,    /* zero if no hardware ywrap    */             
         fixed_info->line_length,  /* length of a line in bytes    */             
         fixed_info->mmio_start,   /* Start of Memory Mapped I/O (phys addr) */   
         fixed_info->mmio_len,     /* Length of Memory Mapped I/O  */             
         fixed_info->accel);       /* Indicate to driver the specific chip/card */
}

static void display_fb_var_info(struct fb_var_screeninfo *var_info)
{
  /* Display interesting, variable FB info. */
  /* (We break up format strings for C89 compiler compliance.) */
  printk("\nVariable framebuffer info\n"
         "    xres:             %10u"
         "    yres:             %10u\n"
         "    xres_virtual:     %10u"
         "    yres_virtual:     %10u\n"
         "    xoffset:          %10u"
         "    yoffset:          %10u\n"
         "    bits_per_pixel:   %10u"
         "    grayscale:        %10u\n",

         var_info->xres,              /* visible resolution*/                
         var_info->yres,                                                     
         var_info->xres_virtual,      /* virtual resolution*/                
         var_info->yres_virtual,                                             
         var_info->xoffset,           /* offset from virtual to visible */   
         var_info->yoffset,           /* resolution*/                        
         var_info->bits_per_pixel,    /* guess what*/                        
         var_info->grayscale);        /* != 0 Graylevels instead of colors */

         /* Transpose these, for better readability with a column display. */
  printk("    red.offset:       %10u"
         "    green.offset:     %10u\n"
         "    red.length:       %10u"
         "    green.length:     %10u\n"
         "    red.msb_right:    %10u"
         "    green.msb_right:  %10u\n"

         "    blue.offset:      %10u"
         "    transp.offset:    %10u\n"
         "    blue.length:      %10u"
         "    transp.length:    %10u\n"
         "    blue.msb_right:   %10u"
         "    transp.msb_right: %10u\n",

         var_info->red.offset,        /* bitfield in fb mem if true color, */
         var_info->green.offset,                                             
         var_info->red.length,        /* else only length is significant */  
         var_info->green.length,                                             
         var_info->red.msb_right,                                            
         var_info->green.msb_right,                                          

         var_info->blue.offset,                                              
         var_info->transp.offset,
         var_info->blue.length,                                              
         var_info->transp.length,                                            
         var_info->blue.msb_right,                                           
         var_info->transp.msb_right);

  printk("    nonstd:           %10u"     
         "    activate:         %10u\n"
         "    height:           %10d"
         "    width:            %10d\n"
         "    accel_flags:      %10u" 
         "    pixclock:         %10u\n"
         "    left_margin:      %10u"
         "    right_margin:     %10u\n"
         "    upper_margin:     %10u"
         "    lower_margin:     %10u\n"
         "    hsync_len:        %10u"
         "    vsync_len:        %10u\n"
         "    sync:             %10u"
         "    vmode:            %10u\n"
         "    rotate:           %10u\n",

         var_info->nonstd,            /* != 0 Non standard pixel format */   
         var_info->activate,          /* see FB_ACTIVATE_**/                 
         var_info->height,            /* height of picture in mm    */       
         var_info->width,             /* width of picture in mm     */       
         var_info->accel_flags,       /* (OBSOLETE) see fb_info->flags */     
         var_info->pixclock,          /* pixel clock in ps (pico seconds) */ 
         var_info->left_margin,       /* time from sync to picture*/         
         var_info->right_margin,      /* time from picture to sync*/         
         var_info->upper_margin,      /* time from sync to picture*/         
         var_info->lower_margin,                                             
         var_info->hsync_len,         /* length of horizontal sync*/         
         var_info->vsync_len,         /* length of vertical sync*/           
         var_info->sync,              /* see FB_SYNC_**/                     
         var_info->vmode,             /* see FB_VMODE_**/                    
         var_info->rotate);           /* angle we rotate counter clockwise */
}
