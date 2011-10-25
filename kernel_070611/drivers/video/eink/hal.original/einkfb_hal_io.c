/*
 *  linux/drivers/video/eink/hal/einkfb_hal_io.c -- eInk frame buffer device HAL I/O
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "einkfb_hal.h"
#include "../broadsheet/broadsheet.h"

#if PRAGMAS
    #pragma mark Definitions/Globals
#endif

static einkfb_ioctl_hook_t einkfb_ioctl_hook = NULL;
static update_area_t einkfb_update_area;

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

struct load_buffer_t
{
    u8 *src, *dst;
};
typedef struct load_buffer_t load_buffer_t;

static unsigned long global_data_chksum=0;
static void einkfb_load_buffer(int x, int y, int rowbytes, int bytes, void *data)
{
    load_buffer_t *load_buffer = (load_buffer_t *)data;
    load_buffer->dst[bytes] = load_buffer->src[(rowbytes * y) + x];
}

static int einkfb_validate_area_data(update_area_t *update_area)
{
    int xstart = update_area->x1, xend = update_area->x2,
        ystart = update_area->y1, yend = update_area->y2,
        
        xres = xend - xstart,
        yres = yend - ystart,
        
        buffer_size = 0;
    
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    if ( einkfb_bounds_are_acceptable(xstart, xres, ystart, yres) )
        buffer_size = BPP_SIZE((xres * yres), info.bpp);

    // Fix the bounds to the appropriate byte alignment if the passed-in bounds
    // aren't byte aligned and we're just doing an area update from the
    // framebuffer itself.
    //
    if ( (0 == buffer_size) && NOT_BYTE_ALIGNED() && (NULL == update_area->buffer) )
        if ( einkfb_align_bounds((rect_t *)update_area) )
            buffer_size = BPP_SIZE((xres * yres), info.bpp);

    return ( buffer_size );
}

static update_area_t *einkfb_set_area_data(unsigned long flag, update_area_t *update_area)
{
    update_area_t *result = NULL;
    
    if ( update_area )
    {
        result = &einkfb_update_area;
        
        // If debugging is enabled, output the passed-in update_area data.
        //
        einkfb_debug("x1       = %d\n",      update_area->x1);
        einkfb_debug("y1       = %d\n",      update_area->y1);
        einkfb_debug("x2       = %d\n",      update_area->x2);
        einkfb_debug("y2       = %d\n",      update_area->y2);
        einkfb_debug("which_fx = %d\n",      update_area->which_fx);
        einkfb_debug("buffer   = 0x%08lX\n", (unsigned long)update_area->buffer);
        
        if ( result )
        {
            int success = einkfb_memcpy(EINKFB_IOCTL_FROM_USER, flag, result, update_area, sizeof(update_area_t));
            unsigned char *buffer = NULL;

            if ( EINKFB_SUCCESS == success )
            {
                int buffer_size = einkfb_validate_area_data(result);
                
                success = EINKFB_FAILURE;
                result->buffer = NULL;

                if ( buffer_size )
                {
                    struct einkfb_info info;
                    einkfb_get_info(&info);
                    
                    buffer = info.buf;
    
                    if ( update_area->buffer )
                        success = einkfb_memcpy(EINKFB_IOCTL_FROM_USER, flag, buffer, update_area->buffer, buffer_size);
                    else
                    {
                        load_buffer_t load_buffer;

                        load_buffer.src = info.start;
                        load_buffer.dst = buffer;
                        
                        einkfb_blit(result->x1, result->x2, result->y1, result->y2, einkfb_load_buffer, (void *)&load_buffer);
                        success = EINKFB_SUCCESS;
                    }
                }
            }

            if ( EINKFB_SUCCESS == success )
                result->buffer = buffer;
            else
                result = NULL;
        }
    }
    
    return ( result );
}

static void einkfb_invert_area_data(update_area_t *update_area)
{
    int xstart = update_area->x1, xend = update_area->x2,
        ystart = update_area->y1, yend = update_area->y2,
        
        xres = xend - xstart,
        yres = yend - ystart,
        
        buffer_size = 0,
        i;
        
    u8  *buffer = update_area->buffer;
        
    struct einkfb_info info;
    einkfb_get_info(&info);
    
    buffer_size = BPP_SIZE((xres * yres), info.bpp);
    
    for ( i = 0; i < buffer_size; i++ )
    {
        buffer[i] = ~buffer[i];
        EINKFB_SCHEDULE_BLIT(i);
    }
}

static char unknown_cmd_string[32];

char *einkfb_get_cmd_string(unsigned int cmd)
{
    char *cmd_string = NULL;
    
    switch ( cmd )
    {
        // Supported by HAL.
        //
        case FBIO_EINK_UPDATE_DISPLAY:
            cmd_string = "update_display";
        break;
        
        case FBIO_EINK_UPDATE_DISPLAY_AREA:
            cmd_string = "update_display_area";
        break;
        
        case FBIO_EINK_RESTORE_DISPLAY:
            cmd_string = "restore_display";
        break;
        
        case FBIO_EINK_SET_REBOOT_BEHAVIOR:
            cmd_string = "set_reboot_behavior";
        break;

        case FBIO_EINK_GET_REBOOT_BEHAVIOR:
            cmd_string = "get_reboot_behavior";
        break;

        case FBIO_EINK_SET_DISPLAY_ORIENTATION:
            cmd_string = "set_display_orientation";
        break;
        
        case FBIO_EINK_GET_DISPLAY_ORIENTATION:
            cmd_string = "get_display_orientation";
        break;

        // Supported by Shim.
        //
        case FBIO_EINK_UPDATE_DISPLAY_FX:
            cmd_string = "update_dislay_fx";
        break;
        
        case FBIO_EINK_SPLASH_SCREEN:
            cmd_string = "splash_screen";
        break;
        
        case FBIO_EINK_SPLASH_SCREEN_SLEEP:
            cmd_string = "splash_screen_sleep";
        break;
        
        case FBIO_EINK_OFF_CLEAR_SCREEN:
            cmd_string = "off_clear_screen";
        break;
        
        case FBIO_EINK_CLEAR_SCREEN:
            cmd_string = "clear_screen";
        break;
        
        case FBIO_EINK_POWER_OVERRIDE:
            cmd_string = "power_override";
        break;
        
        case FBIO_EINK_FAKE_PNLCD:
            cmd_string = "fake_pnlcd";
        break;
        
        case FBIO_EINK_PROGRESSBAR:
            cmd_string = "progressbar";
        break;
        
        case FBIO_EINK_PROGRESSBAR_SET_XY:
            cmd_string = "progressbar_set_xy";
        break;
        
        case FBIO_EINK_PROGRESSBAR_BADGE:
            cmd_string = "progressbar_badge";
        break;
        
        case FBIO_EINK_PROGRESSBAR_BACKGROUND:
            cmd_string = "progressbar_background";
        break;

		/* Henry Li: support PB specific ioctl command */
	  case FBIOCMD_SENDCOMMAND:
            cmd_string = "Send an command to EPD";
        break;

	  case FBIOCMD_WAKEUP:
            cmd_string = "Wake up EPD";
        break;

	  case FBIOCMD_SUSPEND:
            cmd_string = "suspend EPD";
        break;

	  case FBIOCMD_RESET:
            cmd_string = "reset EPD";
        break;

	  case FBIOCMD_DYNAMIC:
            cmd_string = "Set EPD dynamic mode";
        break;
		
        // Unknown and/or unsupported.
        //
        default:
            sprintf(unknown_cmd_string, "unknown cmd = 0x%08X", cmd);
            cmd_string = unknown_cmd_string;
        break;
    }
    
    return ( cmd_string );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark External Interfaces
    #pragma mark -
#endif

#define IOCTL_TIMING "ioctl_timing"

//&*&*&*20100202_JJ1 EPD check handwrite flag        
static int eink_x3_epd_set_bootup_logo(update_area_u16 *update_area, struct fb_info *info)
{		
    return true;		
}
//&*&*&*20100202_JJ2 EPD check handwrite flag    
//&*&*&*20100202_JJ1 EPD check handwrite flag        
static int eink_x3_epd_hangwrite_flag(update_area_u16 *update_area, struct fb_info *info)
{		
    return hal_ops.hal_x3_epd_hangwrite_flag(update_area, info);		
}

//&*&*&*1211ADD1 add EPD update function.    
static int eink_x3_update_display_all(fx_type update_mode, struct fb_info *info)
{		
	
//printk("[jimmy](einkfb_hal_io.c):eink_x3_update_display_all \n");	
    return hal_ops.hal_x3_update_display_all(update_mode, info);		
}

static int eink_x3_update_display_area(update_area_t *update_area, struct fb_info *info)
{		
	if((update_area->which_fx & 20) == 20/*UPDATE_DU*/)		//Henry 0x20 ==> 20
    	hal_ops.hal_x3_update_display_area_du(update_area, info);		
    else
    	hal_ops.hal_x3_update_display_area(update_area, info);		
    
	return 0;
}
//&*&*&*1211ADD2 add EPD update function.    

 /*Henry Li:  Transfer partial image data. The first 8 bytes contain coordinates of a rectangle:
	Data[0..1] =x1, Data[2..3] =y1, Data[4..5] =x2, Data[6..7] =y2
	(most significant byte goes fist) All coordinates should be aligned to 4 pixels */
bool broadsheet_ep3_update_display_image(update_area_t *update_area, struct fb_info *info);
static int eink_ep3_update_display_area_image(more_tag_tdisplaycommand *p_tag_tdisplaycommand, struct fb_info *info,  int data_size)
{
    update_area_t *result = NULL;
    char flag = 0;		//copy to/from user-space
    int success = EINKFB_IOCTL_FAILURE;	
    //u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);	//828 in broadsheet_hal.c
    u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+YRES*XRES/2);	//828 in broadsheet_hal.c

    if (p_tag_tdisplaycommand)
	if (p_tag_tdisplaycommand->BytesToWrite > 0)		
	if (p_tag_tdisplaycommand->Data)
    	{    	
        result = &einkfb_update_area;
	 result->x1 =  ((p_tag_tdisplaycommand->Data[0] << 8) | (p_tag_tdisplaycommand->Data[1] << 0));
	 result->x2 =  ((p_tag_tdisplaycommand->Data[4] << 8) | (p_tag_tdisplaycommand->Data[5] << 0)) + 1;		//Henry: As weight = x2 - x1
	 result->y1 =  ((p_tag_tdisplaycommand->Data[2] << 8) | (p_tag_tdisplaycommand->Data[3] << 0));
	 result->y2 =  ((p_tag_tdisplaycommand->Data[6] << 8) | (p_tag_tdisplaycommand->Data[7] << 0)) + 1;		//Henry: As height = y2 - y1
	 result->buffer = (__u8 *)epd_base+8;
	 
        int buffer_size = einkfb_validate_area_data(result);
        if (buffer_size > data_size)
			buffer_size=data_size;
	if  ((buffer_size ==  0) && (data_size > 8))	//Testing: 0917
		buffer_size=data_size - 8;

		global_data_chksum=0;
        if ( result->buffer )
        	{
               //memcpy(result->buffer, (u8 *) (&(p_tag_tdisplaycommand->Data[8])), buffer_size);
		int i;
		for (i=0; i < buffer_size; i++)
{
			result->buffer[i]=~(result->buffer[i]);	
global_data_chksum += result->buffer[i];
}
/*		
	       return hal_ops.hal_ep3_update_display_image(result, info);
*/
		return broadsheet_ep3_update_display_image(result, info);		
        	}
        else
		 return ( success );
        }

    return ( success );   	
}


unsigned long get_chksum(void)
{
	return global_data_chksum;
}
EXPORT_SYMBOL_GPL(get_chksum);
extern unsigned char  broadsheet_get_rotation_mode(void);
/* Henry Li: Transfer image data (image format should be set with SetDepth command). */
//static int eink_ep3_update_display_image(tag_tdisplaycommand_t *p_tag_tdisplaycommand, struct fb_info *info, int data_size)
static int eink_ep3_update_display_image(more_tag_tdisplaycommand *p_tag_tdisplaycommand, struct fb_info *info, int data_size)
{
    update_area_t *result = NULL;
    char flag = 0;		//copy to/from user-space
    int success = EINKFB_IOCTL_FAILURE;	
    //u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);	//828  in broadsheet_hal.c
    u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+YRES*XRES/2);	//828 in broadsheet_hal.c    
    unsigned char angle=broadsheet_get_rotation_mode();
	
    if (p_tag_tdisplaycommand)
	if (p_tag_tdisplaycommand->Data)
    	{
        result = &einkfb_update_area;
	 if ((1 == angle ) || (3 == angle))	
	 {
	 result->x1 = 0;
	 result->x2 = XRES;		//828 in broadsheet_hal.c  
	 result->y1 = 0;
	 result->y2 = YRES;	//1200  in broadsheet_hal.c 
	 }
	 else
	 {
	 result->x1 = 0;
	 result->x2 = YRES;		//828 in broadsheet_hal.c  
	 result->y1 = 0;
	 result->y2 = XRES;	//1200  in broadsheet_hal.c 	 
	 }
	 result->buffer = (__u8 *)epd_base;
	 
        int buffer_size = einkfb_validate_area_data(result);
        if (buffer_size > data_size)
			buffer_size=data_size;
	if  (buffer_size ==  0) 		//Testing: 0917
			buffer_size=data_size;
        if ( result->buffer )
        	{
               //memcpy(result->buffer, p_tag_tdisplaycommand->Data, buffer_size);
		int i;
		global_data_chksum=0;
		for (i=0; i < buffer_size; i++)
			{
			result->buffer[i]=~(result->buffer[i]);	
			global_data_chksum += result->buffer[i];
			}
	       return hal_ops.hal_ep3_update_display_image(result, info);
        	}
        else
		 return ( success );
        }

    return ( success );   	
}


/*  Henry: waveform_update_type as same as  broadsheet_hal.c */
#define UPDATE_GU  0
#define UPDATE_GC  1
#define UPDATE_DU	20
static int eink_ep3_show_display_image(int update_mode)
{	
	if (hal_ops.hal_show_display_image)
		hal_ops.hal_show_display_image(update_mode);
	return EINKFB_SUCCESS;
}

static int eink_ep3_show_display_area_image(int update_mode)
{	
	if (hal_ops.hal_show_display_area_image)
		hal_ops.hal_show_display_area_image(update_mode);
	return EINKFB_SUCCESS;
}

/*Henry: Fill display with specified color 
pixel_mode: 0=black, 1=white, 2=dark gray, 3=light gray  */
static int eink_ep3_epd_fill_pixel(int pixel_mode)
{
	if (hal_ops.hal_epd_fill_pixel)
		hal_ops.hal_epd_fill_pixel(pixel_mode);
	return EINKFB_SUCCESS;
}

/* Henry: bs_power_run_mode  wakeup;
		 bs_power_sleep_mode sleep */
static void eink_ep3_do_bs_mode_switch(int bs_mode)
{	
	if (hal_ops.hal_do_bs_mode_switch)
		hal_ops.hal_do_bs_mode_switch(bs_mode);
}

/* Henry: bs_picture_normal_mode;
		 bs_picture_inverted_mode */
static int eink_ep3_picture_mode_switch(int picture_mode)
{	
	if (hal_ops.hal_do_normal_inverted_switch)
		{
		if (hal_ops.hal_do_normal_inverted_switch(picture_mode))
			return true;
		else
			return false;
		}
	else 
		return false;
}

/* Henry:
    // The temperature sensor is actually returning a signed, 8-bit
    // value from -25C to 80C.  But we go ahead and use the entire
    // 8-bit range:  0x00..0x7F ->  0C to  127C
    //               0xFF..0x80 -> -1C to -128C
*/    
static int eink_ep3_read_temperature(void)
{
	int sensor_value=0;
	if (hal_ops.hal_bs_read_temperature)
		sensor_value = hal_ops.hal_bs_read_temperature();
	return sensor_value;
}

static int eink_ep3_read_flash(unsigned long addr, unsigned char *data)
{
	int result = 0	;
	if (hal_ops.hal_bs_readfromflash)
		result = hal_ops.hal_bs_readfromflash(addr, data);	
	return result;
}							   
static int eink_ep3_write_flash(unsigned long start_addr, unsigned char *buffer, unsigned long blen)
{
	int result = 0	;
	if (hal_ops.hal_bs_writetoflash)
		result = hal_ops.hal_bs_writetoflash(start_addr, buffer, blen);	
	return result;
}							   
extern  void bs_cmd_wr_reg(u16 ra, u16 wd);
extern void LIST_REG(void);
extern void einkfb_block(void);
extern void einkfb_unblock(void);
extern void einkfb_set_dynamic_parameter(unsigned char parameter);
extern int  einkfb_get_global_suspend_resume_state(void);
int einkfb_ioctl_dispatch(unsigned long flag, struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    bool done = !EINKFB_IOCTL_DONE, bad_arg = false;
    unsigned long start_time = jiffies, stop_time;
    orientation_t old_orientation;
    int result = EINKFB_SUCCESS;

    unsigned long local_arg = arg;
    unsigned int local_cmd = cmd;
   /* Henry Li: handle specific Pocket Book ioctl */	
    tag_tdisplaycommand_t local_tag_tdisplaycommand;
    less_tag_tdisplaycommand less_local_tag_tdisplaycommand;
    more_tag_tdisplaycommand more_local_tag_tdisplaycommand;
    unsigned char *Pointer_BytesToRead;
    unsigned long  flash_address;
    //u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);	//828  in broadsheet_hal.c
    u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+YRES*XRES/2);	//828 in broadsheet_hal.c
		
    einkfb_debug_ioctl("%s(0x%08lX)\n", einkfb_get_cmd_string(cmd), arg);
    EINKFB_PRINT_PERF_REL(IOCTL_TIMING, 0UL, einkfb_get_cmd_string(cmd));

    // If there's a hook, give it the pre-command call.
    //
    if ( einkfb_ioctl_hook )
        done = (*einkfb_ioctl_hook)(einkfb_ioctl_hook_pre, flag, &local_cmd, &local_arg);
    
    // Process the command if it hasn't already been handled.
    //
    if ( !done )
    {
        update_area_t *update_area = NULL;
        
        switch ( local_cmd )
        {
//&*&*&*20100202_JJ1 EPD check handwrite flag        	
            case FBIO_EINK_HANDWRITE_FLAG:
            		result = eink_x3_epd_hangwrite_flag((update_area_u16 *)local_arg, info);
            break;
//&*&*&*20100202_JJ2 EPD check handwrite flag    
            
            case FBIO_EINK_UPDATE_DISPLAY:
// EPD update function.   
#if 0            	
	    	 				printk("eink driver: begin to update display full (command =  0x33)\n");
                einkfb_update_display(UPDATE_MODE(local_arg));
		 						printk("CMD = %x,arg = %d\n",FBIO_EINK_UPDATE_DISPLAY,UPDATE_MODE(local_arg));
#else		 
//printk("[jimmy](EPD):FBIO_EINK_UPDATE_DISPLAY \n");	
								eink_x3_update_display_all(UPDATE_MODE(local_arg), info);
#endif								
//EPD update function.   
            break;
            
            case FBIO_EINK_UPDATE_DISPLAY_AREA:
//&*&*&*1211MODIFY1 modify EPD update function.   
#if 0            	
	   printk("eink driver: begin to update display area (command =  0x34)\n");
		 update_area = einkfb_set_area_data(flag,(update_area_t *)local_arg);
		 printk("CMD = %x,arg = %ld\n",FBIO_EINK_UPDATE_DISPLAY_AREA,local_arg);
		if ( update_area )
                {
                    fx_type saved_fx = update_area->which_fx;
                    bool flash_area = false;
                    
                    // If there's a hook, give it the beginning in-situ call.
                    //
                    if ( einkfb_ioctl_hook )
                        (*einkfb_ioctl_hook)(einkfb_ioctl_hook_insitu_begin, flag, &local_cmd,
                            (unsigned long *)&update_area);

                    // Update the display with the area data, preserving and
                    // normalizing which_fx as we go.
                    //
                    if ( fx_flash == update_area->which_fx )
                    {
                        einkfb_invert_area_data(update_area);
                        saved_fx = fx_update_full;
                        flash_area = true;
                    }
                    
                    update_area->which_fx = UPDATE_AREA_MODE(saved_fx);
                    einkfb_update_display_area(update_area);
                    
                    if ( flash_area )
                    {
                        update_area->which_fx = fx_update_partial;
                        einkfb_invert_area_data(update_area);
                        
                        einkfb_update_display_area(update_area);
                    }
                    
                    update_area->which_fx = saved_fx;

                    // If there's a hook, give it the ending in-situ call.
                    //
                    if ( einkfb_ioctl_hook )
                        (*einkfb_ioctl_hook)(einkfb_ioctl_hook_insitu_end, flag, &local_cmd,
                            (unsigned long *)&update_area);
                }
                else
                    goto failure;
#else
//printk("[jimmy](EPD):FBIO_EINK_UPDATE_DISPLAY_AREA \n");	
			result = eink_x3_update_display_area((update_area_t *)local_arg, info);
#endif                    
//&*&*&*1211MODIFY2 modify EPD update function.   
            break;


/* --> Henry Li: 20100701 Implenent pocketbook display translation layer */
            case FBIOCMD_SENDCOMMAND:
			if (!access_ok(VERIFY_WRITE, (tag_tdisplaycommand_t *)local_arg, sizeof(less_local_tag_tdisplaycommand)))
	                    goto failure;
			  if (einkfb_get_global_suspend_resume_state() == 0)
			                    goto failure;					
					einkfb_block();
			copy_from_user(&local_tag_tdisplaycommand, (tag_tdisplaycommand_t *)local_arg, sizeof(less_local_tag_tdisplaycommand));
			copy_from_user(&more_local_tag_tdisplaycommand, (tag_tdisplaycommand_t *)local_arg, sizeof(less_local_tag_tdisplaycommand));			
			einkfb_debug_ioctl("%s FBIOCMD_SENDCOMMAND(%x BytesToWrite=%d)\n", __FUNCTION__,local_tag_tdisplaycommand.cmd, local_tag_tdisplaycommand.BytesToWrite);
			//udelay(100);
			if (local_tag_tdisplaycommand.BytesToWrite > 0)
				{
				if ((local_tag_tdisplaycommand.cmd != FB_NEWIMAGE) && (local_tag_tdisplaycommand.cmd != FB_PARTIALIMAGE))
				{
				more_local_tag_tdisplaycommand.Data = einkfb_malloc(local_tag_tdisplaycommand.BytesToWrite);
				if (!access_ok(VERIFY_WRITE, ((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.BytesToWrite))
					{
					if (more_local_tag_tdisplaycommand.Data )
						einkfb_free(more_local_tag_tdisplaycommand.Data);					
		                    goto failure;
					}
				}
				else 	//local_tag_tdisplaycommand == FB_NEWIMAGE
				{
					//einkfb_block();
				#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)
					if (local_tag_tdisplaycommand.BytesToWrite > 496808)
			                    goto failure;					
				#endif
				#if  defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
					if (local_tag_tdisplaycommand.BytesToWrite > 240008)
			                    goto failure;					
				#endif
					
				more_local_tag_tdisplaycommand.Data = epd_base;	
				}
				if (local_tag_tdisplaycommand.BytesToWrite > 16)
					copy_from_user(local_tag_tdisplaycommand.Data, ((tag_tdisplaycommand_t *)local_arg)->Data, 16);
				else
					copy_from_user(local_tag_tdisplaycommand.Data, ((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.BytesToWrite);
				copy_from_user(more_local_tag_tdisplaycommand.Data, ((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.BytesToWrite);				
				}
			  if (einkfb_get_global_suspend_resume_state() == 0)
			                    goto failure;					
		         switch ( local_tag_tdisplaycommand.cmd )
		         {
				 /* Transfer image data (image format should be set with SetDepth command). */		         
            		  case FB_NEWIMAGE:
				   result = eink_ep3_update_display_image(&more_local_tag_tdisplaycommand, info, local_tag_tdisplaycommand.BytesToWrite);
            		 	  break;
				/*Stop image transfer */
            		  case FB_STOPNEWIMAGE:
            		          result = 0;
            		 	  break;
				/*Update whole display using Grayscale Clearing waveform */
            		  case FB_DISPLAYIMAGE:
            		          result = eink_ep3_show_display_image(UPDATE_GC|0x20);	/*fx_update_full*/
            		 	  break;
				 /* Transfer partial image data. The first 8 bytes contain coordinates of a rectangle:
							Data[0..1] =x1, Data[2..3] =y1, Data[4..5] =x2, Data[6..7] =y2
							(most significant byte goes fist) All coordinates should be aligned to 4 pixels */
            		  case FB_PARTIALIMAGE:
				   result = eink_ep3_update_display_area_image(&more_local_tag_tdisplaycommand, info, local_tag_tdisplaycommand.BytesToWrite);							  
            		 	  break;
				/* Update a rectangle transferred with PartialImage command, 
								using Monochrome Update waveform*/
            		  case FB_DISPLAYPARTIALMU:
            		          result = eink_ep3_show_display_area_image(UPDATE_DU);
            		 	  break;
				/*Update a rectangle transferred with PartialImage command, 
								using Grayscale Update waveform */
            		  case FB_DISPLAYPARTIALGU:
            		          result = eink_ep3_show_display_area_image(UPDATE_GU);
            		 	  break;

				/*Update a rectangle transferred with PartialImage command, 
								using Grayscale Clearing waveform */
            		  case FB_DISPLAYPARTIALGC:
            		          result = eink_ep3_show_display_area_image(UPDATE_GC);
            		 	  break;

            		  case FB_DISPLAYPARTIALMU_F:
            		          result = eink_ep3_show_display_area_image(UPDATE_DU | 0x20);
            		 	  break;
				/*Update a rectangle transferred with PartialImage command, 
								using Grayscale Update waveform */
            		  case FB_DISPLAYPARTIALGU_F:
            		          result = eink_ep3_show_display_area_image(UPDATE_GU | 0x20);
            		 	  break;

				/*Update a rectangle transferred with PartialImage command, 
								using Grayscale Clearing waveform */
            		  case FB_DISPLAYPARTIALGC_F:
            		          result = eink_ep3_show_display_area_image(UPDATE_GC | 0x20);
            		 	  break;
						  
				/*Set format of image data: Data[0]: 0=1BPP, 2=2BPP, 4=4BPP */				
            		  case FB_SETDEPTH:
            		           result = 0;
	                        struct einkfb_info hal_info;
	                        einkfb_get_info(&hal_info);

	                        if ( !info )
	                            info = hal_info.fbinfo;

				   if (local_tag_tdisplaycommand.BytesToWrite > 0)
	                         if ( local_tag_tdisplaycommand.Data )
	                        	{
	                        	if ((1 == local_tag_tdisplaycommand.Data[0]) || (2 == local_tag_tdisplaycommand.Data[0]) || (4 == local_tag_tdisplaycommand.Data[0]))
	                        		{
						hal_info.bpp = local_tag_tdisplaycommand.Data[0];
						einkfb_set_bpp(hal_info.bpp);
	                        		}
					else
						einkfb_debug_ioctl("Error image depth setting (%d)\n", local_tag_tdisplaycommand.Data[0]);
					}
            		           //result = 0;
            		 	  break;
						  
				/* Erase display with specified color 
								Data[0]: 0=black, 1=white, 2=dark gray, 3=light gray  */
            		  case FB_ERASEDISPLAY:
				  if (local_tag_tdisplaycommand.BytesToWrite > 0)
				 {
	                        if ( local_tag_tdisplaycommand.Data )		  	
            		         	 result = eink_ep3_epd_fill_pixel(local_tag_tdisplaycommand.Data[0]);
				    else
			                result = EINKFB_IOCTL_FAILURE;
				  }
				    else
			                result = EINKFB_IOCTL_FAILURE;						
            		 	  break;
						  
				/* Set image orientation Data[0]: 0=270бу, 1=0бу, 2=90бу, 3=180бу */
				/* Swap 0 <--> 3, 1<--2> as Kostiantyn said */
            		  case FB_ROTATE:
				/* Same as FBIO_EINK_SET_DISPLAY_ORIENTATION */
			        result = EINKFB_IOCTL_FAILURE;
				orientation_t local_orientation;
#if 1				
				const unsigned char cmd_trans_orientation_table[4] = {1, 2, 0, 3};
				//const unsigned char cmd_trans_controller_register_table[4] = {3, 0, 1, 2};			
#else
				const unsigned char cmd_trans_orientation_table[4] = {3, 0, 2, 1}; 	/* 0812*/ /* Swap 0 <--> 3, 1<--2> as Kostiantyn said */
#endif
		// ioctl 0 =>  orientation_portrait_upside_down (1),    1 => orientation_landscape (2), 
		// ioctl  2 => orientation_portrait (0),  3 =>  orientation_landscape_upside_down (3),
				if (local_tag_tdisplaycommand.BytesToWrite > 0)
     	                     if ( local_tag_tdisplaycommand.Data )
     	                     {
     	                         if ( 4 > local_tag_tdisplaycommand.Data[0] )
     	                         {
				/*Henry: EPD Controller: 00b   0бу; 01b   90бу; 10b   180бу; 11b   270бу*/
				/*Henry: ioctl Set image orientation Data[0]: 0=270бу, 1=0бу, 2=90бу, 3=180бу */
				/*Kostiantyn said: ioctl Set image orientation Data[0]: 3=270бу, 2=0бу, 1=90бу, 0=180бу */

					local_orientation = cmd_trans_orientation_table[local_tag_tdisplaycommand.Data[0]];
     	                            //bs_rotation_mode = cmd_trans_controller_register_table[local_tag_tdisplaycommand.Data[0]];
		                	old_orientation = einkfb_get_display_orientation();

		                	if ( einkfb_set_display_orientation((orientation_t)local_orientation) )
		                	{
		                    		orientation_t new_orientation = einkfb_get_display_orientation();
                    
			                    if ( !ORIENTATION_SAME(old_orientation, new_orientation) )
			                    {
			                        struct einkfb_info hal_info;
			                        einkfb_get_info(&hal_info);
                        
			                        if ( !info )
			                            info = hal_info.fbinfo;
	
			                        if ( info )
			                            einkfb_set_res_info(info, info->var.yres, info->var.xres);
			                    }
			                }
	            		         result = 0;							
     	                          }
     	                       }
            		 	  break;
				/* Set normal picture mode (default) */
            		  case FB_POSITIVE:
				 // 0: bs_picture_normal_mode 
				 // 1: bs_picture_inverted_mode
				  if (eink_ep3_picture_mode_switch(0) )
					  result = 0;
				  else
			                result = EINKFB_IOCTL_FAILURE;
            		 	  break;
				/*Set inverted picture mode */
            		  case FB_NEGATIVE:
				  if (eink_ep3_picture_mode_switch(1) )
					  result = 0;
				  else
			                result = EINKFB_IOCTL_FAILURE;
            		 	  break;
				/*Enable automatic refresh timer*/
            		  case FB_SETREFRESH:
            		          result = 0;
            		 	  break;
				/* Disable automatic refresh timer  */
            		  case FB_CANCELREFRESH:
            		          result = 0;
            		 	  break;
				/* Set automatic refresh interval Data[0]=refresh time (in 6-second units) */
            		  case FB_SETREFRESHTIMER:
            		          result = 0;
            		 	  break;
				/* Force refresh */
            		  case FB_MANUALREFRESH:
            		          result = 0;
            		 	  break;
				/*  Read temperature sensor data Return: Data[0]=value (буC)*/
            		  case FB_TEMPERATURE:
				einkfb_debug_ioctl("%s FBIOCMD_SENDCOMMAND(%x BytesToRead=%d)\n", __FUNCTION__,local_tag_tdisplaycommand.cmd, local_tag_tdisplaycommand.BytesToRead);
				if ((local_tag_tdisplaycommand.BytesToRead> 0) && (local_tag_tdisplaycommand.BytesToWrite == 0))
					{
					if (!access_ok(VERIFY_READ, ((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.BytesToRead))
						{
						if (more_local_tag_tdisplaycommand.Data )
							einkfb_free(more_local_tag_tdisplaycommand.Data);					
			                    goto failure;
						}
					//local_tag_tdisplaycommand.Data = einkfb_malloc(local_tag_tdisplaycommand.BytesToRead);
	            		       local_tag_tdisplaycommand.Data[0] = eink_ep3_read_temperature();
					copy_to_user(((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.Data, local_tag_tdisplaycommand.BytesToRead);
					einkfb_debug_ioctl("temperature is 0x%x \n", local_tag_tdisplaycommand.Data[0]);
					result = 0;
					//if (local_tag_tdisplaycommand.Data )
					//	einkfb_free(local_tag_tdisplaycommand.Data);
					}
				  else
		                	result = EINKFB_IOCTL_FAILURE;	
            		 	  break;
				 /* Write data to flash (AMD protocol is used for flash programming)
								Data[0..2]=address (MSB first) Data[3]=value  */
            		  case FB_WRITETOFLASH:
				einkfb_debug_ioctl("%s FBIOCMD_SENDCOMMAND(%x BytesToWrite=%d BytesToRead=%d)\n", __FUNCTION__,local_tag_tdisplaycommand.cmd, local_tag_tdisplaycommand.BytesToWrite, local_tag_tdisplaycommand.BytesToRead);					  	
				if (!capable(CAP_SYS_ADMIN))
					{
		                	result = EINKFB_IOCTL_FAILURE;	
					break;
					}
				if (local_tag_tdisplaycommand.BytesToWrite >=  4)
				{
     	                     	if ( more_local_tag_tdisplaycommand.Data )
     	                     	{
					flash_address = (more_local_tag_tdisplaycommand.Data[0] << 16) | (more_local_tag_tdisplaycommand.Data[1] << 8) |	\
						(more_local_tag_tdisplaycommand.Data[2] << 0);
					result = eink_ep3_write_flash(flash_address, (unsigned char *)(&( more_local_tag_tdisplaycommand.Data[3] )), more_local_tag_tdisplaycommand.BytesToWrite - 3);
     	                     	}
					else
			                	result = EINKFB_IOCTL_FAILURE;							
				}
				else
			                	result = EINKFB_IOCTL_FAILURE;
            		 	  break;
				/*Read data from flash Data[0..2]=address (MSB first) 
								Return: Data[0]=value  */				
            		  case FB_READFROMFLASH:
				einkfb_debug_ioctl("%s FBIOCMD_SENDCOMMAND(%x BytesToWrite=%d BytesToRead=%d)\n", __FUNCTION__,local_tag_tdisplaycommand.cmd, local_tag_tdisplaycommand.BytesToWrite, local_tag_tdisplaycommand.BytesToRead);
				if (!capable(CAP_SYS_ADMIN))
					{
		                	result = EINKFB_IOCTL_FAILURE;	
					break;
					}
				if (local_tag_tdisplaycommand.BytesToWrite >=  3)
				{
     	                     	if ( local_tag_tdisplaycommand.Data )
     	                     	{
					flash_address = (local_tag_tdisplaycommand.Data[0] << 16) | (local_tag_tdisplaycommand.Data[1] << 8) |	\
						(local_tag_tdisplaycommand.Data[2] << 0);
     	                     	}
				}
				else
					{
		                	result = EINKFB_IOCTL_FAILURE;						
					break;
					}
				if (local_tag_tdisplaycommand.BytesToRead> 0) 
					{
					if (!access_ok(VERIFY_READ, ((tag_tdisplaycommand_t *)local_arg)->Data, local_tag_tdisplaycommand.BytesToRead))
						{
						if (more_local_tag_tdisplaycommand.Data )
							einkfb_free(more_local_tag_tdisplaycommand.Data);					
			                    goto failure;
						}					
					Pointer_BytesToRead = NULL;
					Pointer_BytesToRead = einkfb_malloc(local_tag_tdisplaycommand.BytesToRead);
	            		       result = eink_ep3_read_flash(flash_address, Pointer_BytesToRead);							   
					copy_to_user(((tag_tdisplaycommand_t *)local_arg)->Data, Pointer_BytesToRead, local_tag_tdisplaycommand.BytesToRead);
					einkfb_debug_ioctl("Flash data in 0x%x  is 0x%x \n", flash_address, Pointer_BytesToRead[0]);
					if (Pointer_BytesToRead )
						einkfb_free(Pointer_BytesToRead);
					}
				  else
		                	result = EINKFB_IOCTL_FAILURE;	
            		 	  break;
						  
            		  case FB_PEN_CALIBRATE:
				  if (local_tag_tdisplaycommand.BytesToWrite >= 6)
				 {
				  u16 HSize; 
				  u16 VSize;
				  u16 xyswap;
	                        if ( more_local_tag_tdisplaycommand.Data )
	                        	{
	                        	HSize = (more_local_tag_tdisplaycommand.Data[0] << 8) | more_local_tag_tdisplaycommand.Data[1];
					VSize = (more_local_tag_tdisplaycommand.Data[2] << 8) | more_local_tag_tdisplaycommand.Data[3];
					xyswap =  (more_local_tag_tdisplaycommand.Data[4] << 8) | more_local_tag_tdisplaycommand.Data[5];
					if (hal_ops.hal_bs_pen_calibrate)
            		         	   hal_ops.hal_bs_pen_calibrate(HSize, VSize, xyswap);
					else
			                result = EINKFB_IOCTL_FAILURE;						
	                        	}
				    else
			                result = EINKFB_IOCTL_FAILURE;
				  }
				    else
			                result = EINKFB_IOCTL_FAILURE;						
            		 	  break;
            		  case FB_PENWRITE_MODE:
				  if (local_tag_tdisplaycommand.BytesToWrite >= 11)
				 {
				  bool mode;
				  unsigned char drawdata[10];
	                        if ( more_local_tag_tdisplaycommand.Data )
	                        	{
	                        	mode = more_local_tag_tdisplaycommand.Data[0];
					memcpy(drawdata, &(more_local_tag_tdisplaycommand.Data[1]), 10);
					if (hal_ops.hal_bs_penwrite_mode)
            		         	    hal_ops.hal_bs_penwrite_mode(mode, drawdata);
					else
			                result = EINKFB_IOCTL_FAILURE;						
	                        	}
				    else
			                result = EINKFB_IOCTL_FAILURE;
				  }
				    else
			                result = EINKFB_IOCTL_FAILURE;						
            		 	  break;
						  
                       default:
		                result = EINKFB_IOCTL_FAILURE;
          			  break;
		         }
                //einkfb_restore_display(UPDATE_MODE(local_arg));
			//if ((local_tag_tdisplaycommand.cmd == FB_NEWIMAGE) || (local_tag_tdisplaycommand.cmd == FB_PARTIALIMAGE))
					//einkfb_unblock();
      			if (more_local_tag_tdisplaycommand.BytesToWrite > 0)
				if (more_local_tag_tdisplaycommand.Data )
				       if ((local_tag_tdisplaycommand.cmd != FB_NEWIMAGE) &&  (local_tag_tdisplaycommand.cmd != FB_PARTIALIMAGE))
						einkfb_free(more_local_tag_tdisplaycommand.Data);
			einkfb_unblock();
            break;

            case FBIOCMD_WAKEUP:
		einkfb_debug_ioctl("%s FBIOCMD_WAKEUP\n", __FUNCTION__);				
				// 0:  bs_power_run_mode,
				// 1:  bs_power_standby_mode,
				// 2:  bs_power_sleep_mode,
				// 3:  bs_power_mode_invalid 
                eink_ep3_do_bs_mode_switch(0);
		  result = 0;
            break;

            case FBIOCMD_SUSPEND:
		einkfb_debug_ioctl("%s FBIOCMD_SUSPEND\n", __FUNCTION__);				
				// 0:  bs_power_run_mode,
				// 1:  bs_power_standby_mode,
				// 2:  bs_power_sleep_mode,
				// 3:  bs_power_mode_invalid 				
                eink_ep3_do_bs_mode_switch(2);
		  result = 0;
            break;

            case FBIOCMD_RESET:
	       einkfb_debug_ioctl("%s FBIOCMD_RESET\n", __FUNCTION__);				
            break;
			
            case FBIOCMD_DYNAMIC:
		einkfb_debug_ioctl("%s FBIOCMD_DYNAMIC\n", __FUNCTION__);				
			if (!access_ok(VERIFY_WRITE, local_arg, sizeof(unsigned long)))
	                    goto failure;
		
			/* Henry Li: preserve DYNAMIC parameter setting*/
			/* parameter=0 => update slow but guaranted   */
			/* parameter=1 => update fast not check for overlapping */
            		           result = 0;
	                        	if ((0  == local_arg) || (1 == local_arg))
	                        		{
						einkfb_set_dynamic_parameter(local_arg);
	                        		}
					else
						einkfb_debug_ioctl("Error image depth setting (%d)\n", local_arg);

            break;
				
/*<-- Henry Li: 20100701 Implenent pocketbook display translation layer */
      
            case FBIO_EINK_RESTORE_DISPLAY:
                einkfb_restore_display(UPDATE_MODE(local_arg));
            break;
            
            case FBIO_EINK_SET_REBOOT_BEHAVIOR:
                einkfb_set_reboot_behavior((reboot_behavior_t)local_arg);
            break;
            
            case FBIO_EINK_GET_REBOOT_BEHAVIOR:
                if ( local_arg )
                {
                    reboot_behavior_t reboot_behavior = einkfb_get_reboot_behavior();

                    einkfb_memcpy(EINKFB_IOCTL_TO_USER, flag, (reboot_behavior_t *)local_arg,
                        &reboot_behavior, sizeof(reboot_behavior_t));
                }
                else
                    goto failure;
            break;
            
            case FBIO_EINK_SET_DISPLAY_ORIENTATION:
                old_orientation = einkfb_get_display_orientation();
                
                if ( einkfb_set_display_orientation((orientation_t)local_arg) )
                {
                    orientation_t new_orientation = einkfb_get_display_orientation();
                    
                    if ( !ORIENTATION_SAME(old_orientation, new_orientation) )
                    {
                        struct einkfb_info hal_info;
                        einkfb_get_info(&hal_info);
                        
                        if ( !info )
                            info = hal_info.fbinfo;

                        if ( info )
                            einkfb_set_res_info(info, info->var.yres, info->var.xres);
                    }
                }
            break;
            
            case FBIO_EINK_GET_DISPLAY_ORIENTATION:
                if ( local_arg )
                {
                    old_orientation = einkfb_get_display_orientation();
                    
                    einkfb_memcpy(EINKFB_IOCTL_TO_USER, flag, (orientation_t *)local_arg,
                        &old_orientation, sizeof(orientation_t));
                }
            break;
            
            failure:
                bad_arg = true;
            default:
                result = EINKFB_IOCTL_FAILURE;
            break;
        }
    }

    // If there's a hook and we haven't determined that we've received a bad argument, give it
    // the post-command call.  Use the originally passed-in cmd & arg here instead of local copies
    // in case they were changed in the pre-command processing.
    //
    if ( !bad_arg && einkfb_ioctl_hook )
    {
        done = (*einkfb_ioctl_hook)(einkfb_ioctl_hook_post, flag, &cmd, &arg);
        
        // If the hook processed the command, don't pass along the HAL's failure to do so.
        //
        if ( done && (EINKFB_IOCTL_FAILURE == result) )
            result = EINKFB_SUCCESS;
    }
    
    stop_time = jiffies;

    einkfb_debug_ioctl("result = %d\n", result);
    EINKFB_PRINT_PERF_ABS(IOCTL_TIMING, (unsigned long)jiffies_to_msecs(stop_time - start_time),
        einkfb_get_cmd_string(cmd));   
    
    return ( result );
}

int einkfb_ioctl(FB_IOCTL_PARAMS)
{
    return ( einkfb_ioctl_dispatch(EINKFB_IOCTL_USER, info, cmd, arg) );
}

void einkfb_set_ioctl_hook(einkfb_ioctl_hook_t ioctl_hook)
{
    // Need to make a queue of these if this becomes truly useful.
    //
    if ( ioctl_hook )
        einkfb_ioctl_hook = ioctl_hook;
    else
        einkfb_ioctl_hook = NULL;
}

EXPORT_SYMBOL(einkfb_get_cmd_string);
EXPORT_SYMBOL(einkfb_ioctl_dispatch);
EXPORT_SYMBOL(einkfb_ioctl);
EXPORT_SYMBOL(einkfb_set_ioctl_hook);
