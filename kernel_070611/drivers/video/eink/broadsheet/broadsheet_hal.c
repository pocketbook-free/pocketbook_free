/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_hal.c
 *  -- eInk frame buffer device HAL broadsheet
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "broadsheet.h"

#define EPD_LOGO_PHY_BASE_ADDR	0x5FF00000
#define EPD_LOGO_PHY_BASE_SIZE	(512*1024)

#define ImageBufferaddr(x) 1024*1024*(2+(x))

#if PRAGMAS
    #pragma mark Definitions
#endif

#define BROADSHEET_RAM_BANK_SIZE        (1024 * 1024)
#define BROADSHEET_NUM_RAM_BANKS        16

#define BROADSHEET_FLASH_SIZE           (1024 * 128)

#define EINKFB_PROC_WAVEFORM_VERSION    "waveform_version"
#define EINKFB_PROC_COMMANDS_VERSION    "commands_version"
#define EINKFB_PROC_RECENT_COMMANDS     "recent_commands"
#define EINKFB_PROC_TEMPERATURE         "temperature"
#define EINKFB_PROC_HARDWAREFB          "hardware_fb"
#define EINKFB_PROC_EINK_ROM            "eink_rom"
#define EINKFB_PROC_EINK_RAM            "eink_ram"
#define EINKFB_PROC_EINK_REG            "eink_reg"

#define BROADSHEET_WATCHDOG_THREAD_NAME EINKFB_NAME"_bt"
#define BROADSHEET_WATCHDOG_TIMER_DELAY (HZ/4)

#define MAX_PREFLIGHT_RETRIES            3
#define MAX_RECENT_COMMANDS             10

#define IS_PORTRAIT()                           \
    ((EINK_ORIENT_PORTRAIT  == bs_orientation) && !bs_upside_down)

#define IS_PORTRAIT_UPSIDE_DOWN()               \
    ((EINK_ORIENT_PORTRAIT  == bs_orientation) &&  bs_upside_down)
    
#define IS_LANDSCAPE()                          \
    ((EINK_ORIENT_LANDSCAPE == bs_orientation) && !bs_upside_down)

#define IS_LANDSCAPE_UPSIDE_DOWN()              \
    ((EINK_ORIENT_LANDSCAPE == bs_orientation) &&  bs_upside_down)

// Default to EP3's resolution:  825x1200@4bpp.
/*Henry: Move to broadsheet.h */
/*
#define XRES                            828	//832
#define YRES                            1200
*/
static int EPD_BUFFER_INDEX;
//ennic£»modified default :
#ifdef EX3_ANDROID 
#define BPP                             EINKFB_4BPP
//#define BPP                             EINKFB_16BPP
#else
#define BPP                             EINKFB_4BPP
#endif

#define BROADSHEET_SIZE                 BPP_SIZE((XRES*YRES), BPP)
//ennic£»modified default :
//#define SCRATCHFB_SIZE                  BPP_SIZE((XRES*YRES), EINKFB_4BPP)
#define SCRATCHFB_SIZE                  BPP_SIZE((XRES*YRES), BPP)

#define BROADSHEET_BTYE_ALIGNMENT       2 // BPP_BYTE_ALIGN(2) -> 4 for 32-bit alignment (4 * 8)


static struct fb_var_screeninfo broadsheet_var __INIT_DATA =
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

static struct fb_fix_screeninfo broadsheet_fix __INIT_DATA =
{
    .id                 = EINKFB_NAME,
    .smem_len           = BROADSHEET_SIZE,
    .type               = FB_TYPE_PACKED_PIXELS,
    .visual             = FB_VISUAL_STATIC_PSEUDOCOLOR,
    .xpanstep           = 0,
    .ypanstep           = 0,
    .ywrapstep          = 0,
    .line_length        = BPP_SIZE(XRES, BPP),
    .accel              = FB_ACCEL_NONE,
};

static struct proc_dir_entry *einkfb_proc_waveform_version  = NULL;
static struct proc_dir_entry *einkfb_proc_commands_version  = NULL;
static struct proc_dir_entry *einkfb_proc_recent_commands   = NULL;
static struct proc_dir_entry *einkfb_proc_temperature       = NULL;
static struct proc_dir_entry *einkfb_proc_hardwarefb        = NULL;
static struct proc_dir_entry *einkfb_proc_eink_rom          = NULL;
static struct proc_dir_entry *einkfb_proc_eink_ram          = NULL;
static struct proc_dir_entry *einkfb_proc_eink_reg          = NULL;

static unsigned char *scratchfb         = NULL;

static char *recent_commands_page       = NULL;
static int  recent_commands_len         = 0;
static int  recent_commands_max         = MAX_RECENT_COMMANDS;

static bool ignore_hw_ready             = false;
static bool force_hw_not_ready          = false;
static int  override_upd_mode           = BS_UPD_MODE_INIT;

static bs_flash_select eink_rom_select  = bs_flash_waveform;
static bool eink_rom_select_locked      = false;
static int  eink_ram_select             = 0;
static bool eink_ram_select_locked      = false;

static unsigned long broadsheet_size    = BROADSHEET_SIZE;
static unsigned long scratchfb_size     = SCRATCHFB_SIZE;
static unsigned long bs_bpp             = BPP;
static int broadsheet_flash_size        = BROADSHEET_FLASH_SIZE;
static int bs_orientation               = EINKFB_ORIENT_PORTRAIT;
static int bs_upside_down               = false;
static int full_update_counter		= 0;
static unsigned char bs_rotation_mode = 1;	
				/*Henry: EPD Controller: 00b   0¡ã; 01b   90¡ã; 10b   180¡ã; 11b   270¡ã*/
				/*Henry: ioctl Set image orientation Data[0]: 0=270¡ã, 1=0¡ã, 2=90¡ã, 3=180¡ã */
static int 	global_left, global_top, global_left_width, global_top_height;
static int previous_left[15], previous_top[15], previous_left_width[15], previous_top_height[15],  previous_mode[15];
static int data_update_counter=0;
static unsigned long previous_moment[15];
static unsigned long global_moment;
static int global_lut_number=13;
static int global_display_stage=0;

static int last_update_full = 1;
static int last_update_x1;
static int last_update_x2;
static int last_update_y1;
static int last_update_y2;
static int fine_update = 1;

/*--Start--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
#define BS_POWER_MODE_SWITCH

#ifdef BS_POWER_MODE_SWITCH
enum bs_power_mode
{
    bs_power_run_mode,
    bs_power_standby_mode,
    bs_power_sleep_mode,
    bs_power_mode_invalid 
};
typedef enum bs_power_mode bs_power_mode;

struct broadsheet_power_mode{

	struct delayed_work bs_delay_work ; 
};

enum bs_picture_mode
{
	bs_picture_normal_mode,
	bs_picture_inverted_mode
};
typedef enum bs_picture_mode bs_picture_mode;

static struct broadsheet_power_mode *broadsheet_power_mode = NULL;
//static bs_power_mode bs_current_power_mode = bs_power_sleep_mode;
static bs_power_mode bs_current_power_mode = bs_power_mode_invalid;	//Henry: correct value for next time release //bs_power_sleep_mode;
//static bs_power_mode bs_previous_power_mode = bs_power_sleep_mode;
static bool bs_handwrite = false;
static unsigned int bs_delaywork_time = 5*HZ;
static unsigned int bs_dspe_delaywork_time = 1;
static bs_picture_mode bs_current_picture_mode=bs_picture_normal_mode;
#endif 
/*--End--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
extern unsigned short __iomem* bsaddr;

static bool broadsheet_watchdog_timer_active = false;
static bool broadsheet_watchdog_timer_primed = false;
static struct timer_list broadsheet_watchdog_timer;
static int broadsheet_watchdog_thread_exit = 0;
static THREAD_ID(broadsheet_watchdog_thread_id);
static DECLARE_COMPLETION(broadsheet_watchdog_thread_exited);
static DECLARE_COMPLETION(broadsheet_watchdog_thread_complete);

#ifdef MODULE
module_param_named(bs_bpp, bs_bpp, long, S_IRUGO);
MODULE_PARM_DESC(bs_bpp, "2 or 4");

module_param_named(bs_orientation, bs_orientation, int, S_IRUGO);
MODULE_PARM_DESC(bs_orientation, "0 (portrait) or 1 (landscape)");

module_param_named(bs_upside_down, bs_upside_down, int, S_IRUGO);
MODULE_PARM_DESC(bs_upside_down, "0 (not upside down) or 1 (upside down)");
#endif // MODULE

#if PRAGMAS
    #pragma mark -
    #pragma mark Proc Entries
    #pragma mark -
#endif

static void do_bs_power_enable(void);
void epd_check_bs_handwrite(bool mode);
// /proc/eink_fb/waveform_version (read-only)
//
static int read_waveform_version(char *page, unsigned long off, int count)
{
    int result = 0;
    
    // We're done after one shot.
    //
    if ( 0 == off )
    {
        bs_flash_select saved_flash_select = broadsheet_get_flash_select();
        
        // Ensure that we're reading from the waveform area of flash.
        //
        broadsheet_set_flash_select(bs_flash_waveform);

        // Read the waveform from flash.
        //
        result = sprintf(page, "%s\n", broadsheet_get_waveform_version_string());
        
        // Restore whatever area of flash was previously set.
        //
        broadsheet_set_flash_select(saved_flash_select);
    }
    return ( result );
}

static int waveform_version_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_waveform_version) );
}

// /proc/eink_fb/commands_version (read-only)
//
static int read_commands_version(char *page, unsigned long off, int count)
{
    int result = 0;
    
    // We're done after one shot.
    //
    if ( 0 == off )
    {
        bs_flash_select saved_flash_select = broadsheet_get_flash_select();
        
        // Ensure that we're reading from the commands area of flash.
        //
        broadsheet_set_flash_select(bs_flash_commands);

        // Read the waveform from flash.
        //
        result = sprintf(page, "%s\n", broadsheet_get_commands_version_string());
        
        // Restore whatever area of flash was previously set.
        //
        broadsheet_set_flash_select(saved_flash_select);
    }
    
    return ( result );
}

static int commands_version_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_commands_version) );
}

// /proc/eink_fb/recent_commands (read-only)
//
static void broadsheet_commands_iterator(bs_cmd_queue_elem_t *bs_cmd_queue_elem)
{
    bs_cmd_block_t bs_cmd_block = bs_cmd_queue_elem->bs_cmd_block;
    char *page = recent_commands_page;
    int i, len = recent_commands_len;
    
    // Output the command packet and time stamp to the page.
    //
    len += sprintf(&page[len],  "command   = 0x%04X\n",
        bs_cmd_block.command);

    len += sprintf(&page[len],  "type      = %s\n", 
        (bs_cmd_type_write == bs_cmd_block.type ?
        "write" : "read"));

    len += sprintf(&page[len],  "num_args  = %d\n",
        bs_cmd_block.num_args);
        
    
    for ( i = 0; i < bs_cmd_block.num_args; i++ )
        len += sprintf(&page[len],  "args[%d]   = 0x%04X\n",
            i, bs_cmd_block.args[i]);

    len += sprintf(&page[len],  "data_size = %d\n",
        bs_cmd_block.data_size);
    
    len += sprintf(&page[len],  "time      = 0x%08lX\n\n",
        bs_cmd_queue_elem->time_stamp);

    // Return the page's length.
    //
    recent_commands_len = len;
}

static int read_recent_commands(char *page, unsigned long off, int count)
{
    int result = 0;
    
    // We're done after one shot.
    //
    if ( 0 == off )
    {
        // Provide the iterator with a place to put its result.
        //
        recent_commands_page = page;
        recent_commands_len  = 0;
    
        // Iterate through command queue with our iterator, and
        // get the result.
        //
        bs_iterate_cmd_queue(broadsheet_commands_iterator, recent_commands_max);
        result = recent_commands_len;
    }

    return ( result );
}

static int recent_commands_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_recent_commands) );
}

// /proc/eink_fb/temperature (read-only)
//
static int read_temperature(char *page, unsigned long off, int count)
{
    int result = 0;
    
    // We're done after one shot.
    //
    if ( 0 == off )
        result = sprintf(page, "%d\n", bs_read_temperature());

    return ( result );
}

static int temperature_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_temperature) );
}

// /proc/eink_fb/hardware_fb (read-only)
//
static int read_hardwarefb_count(unsigned long addr, unsigned char *data, int count)
{
    int result = 0;
    
    if ( scratchfb && data && IN_RANGE((addr + count), 0, scratchfb_size) )
    {
        EINKFB_MEMCPYK(data, &scratchfb[addr], count);
        result = 1;
    }
    
    return ( result );
}

static int read_hardwarefb(char *page, unsigned long off, int count)
{
	return ( einkfb_read_which(page, off, count, read_hardwarefb_count, scratchfb_size) );
}

static int hardwarefb_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    // Upon first access, dump the hardware's buffer into scratchfb.
    //
    int result = 0;

    if ( 0 == off )
    {
        if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
        {
            broadsheet_read_from_ram(bs_cmd_get_sdr_img_base(), scratchfb, scratchfb_size);
            EINKFB_LOCK_EXIT();
        }
    }
	
   result = EINKFB_PROC_SYSFS_RW_NO_LOCK(page, start, off, count, eof, scratchfb_size, read_hardwarefb);	

   return ( result );
}

// /proc/eink_fb/eink_rom (read/write)
//
static int read_eink_rom(char *page, unsigned long off, int count)
{
    return ( einkfb_read_which(page, off, count, (einkfb_read_t)broadsheet_read_from_flash, broadsheet_flash_size) );
}

static int eink_rom_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int result = EINKFB_FAILURE;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        eink_rom_select_locked = true;
        
        result = EINKFB_PROC_SYSFS_RW_NO_LOCK(page, start, off, count, eof, broadsheet_flash_size, read_eink_rom);
        
        eink_rom_select_locked = false;
        EINKFB_LOCK_EXIT();
    }

    return ( result );
}

static int write_eink_rom(char *buf, unsigned long count, int unused)
{
    return ( einkfb_write_which(buf, count, broadsheet_program_flash, broadsheet_flash_size) );
}

static int eink_rom_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
    int result = EINKFB_FAILURE;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        eink_rom_select_locked = true;
        
        result = EINKFB_PROC_SYSFS_RW_NO_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_rom);
        
        eink_rom_select_locked = false;
        EINKFB_LOCK_EXIT();
    }

    return ( result );
}

// /proc/eink_fb/eink_ram (read/write)
//
static int read_eink_ram(char *page, unsigned long off, int count)
{
    return ( einkfb_read_which(page, off, count, (einkfb_read_t)broadsheet_read_from_ram, BROADSHEET_RAM_BANK_SIZE) );
}

static int eink_ram_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int result = EINKFB_FAILURE;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        eink_ram_select_locked = true;
        
        result = EINKFB_PROC_SYSFS_RW_NO_LOCK(page, start, off, count, eof, BROADSHEET_RAM_BANK_SIZE, read_eink_ram);
        
        eink_ram_select_locked = false;
        EINKFB_LOCK_EXIT();
    }

    return ( result );
}

static int write_eink_ram(char *buf, unsigned long count, int unused)
{
    return einkfb_write_which(buf, count, broadsheet_program_ram, BROADSHEET_RAM_BANK_SIZE);
}

static int eink_ram_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
    int result = EINKFB_FAILURE;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        eink_ram_select_locked = true;
        
        result = EINKFB_PROC_SYSFS_RW_NO_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_ram);
        
        eink_ram_select_locked = false;
        EINKFB_LOCK_EXIT();
    }

    return ( result );
}

// /proc/eink_fb/eink_reg (write-only)
//
static int write_eink_reg(char *buf, unsigned long count, int unused)
{
	int result = count;
	u32 ra, wd;
	
	switch ( sscanf(buf, "%x %x", &ra, &wd) )
	{
	    // Read:  REG[ra]
	    //
	    case 1:
	        EINKFB_PRINT("0x%04X = 0x%04X\n", ra, BS_CMD_RD_REG((u16)ra));
	    break;
	    
	    // Write: REG[ra] = wd
	    //
	    case 2:
	        BS_CMD_WR_REG((u16)ra, (u16)wd);
	    break;
	    
	    // Huh?
	    //
	    default:
	        result = -EINVAL;
	    break;
	}
	
	return ( result );
}

static int eink_reg_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int result = 0;

	result = EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_reg);
	return ( result );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Sysfs Entries
    #pragma mark -
#endif

// /sys/devices/platform/eink_fb.0/eink_rom_select (read/write)
//
static ssize_t show_eink_rom_select(FB_DSHOW_PARAMS)
{
    int result = 0;
	
	result  = sprintf(buf, "%d\n", broadsheet_get_flash_select());
	return ( result );
}

static ssize_t store_eink_rom_select(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, select;

	if ( sscanf(buf, "%d", &select) )
	{
        broadsheet_set_flash_select((bs_flash_select)select);
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb.0/eink_ram_select (read/write)
//
static ssize_t show_eink_ram_select(FB_DSHOW_PARAMS)
{
    int result = 0;
	
	result =  sprintf(buf, "%d\n", broadsheet_get_ram_select()) ;
	return ( result );
}

static ssize_t store_eink_ram_select(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, select;
	
	if ( sscanf(buf, "%d", &select) )
	{
		broadsheet_set_ram_select(select);
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb.0/ignore_hw_ready (read/write)
//
static ssize_t show_ignore_hw_ready(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", ignore_hw_ready) );
}

static ssize_t store_ignore_hw_ready(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, ignore_ready;
	
	if ( sscanf(buf, "%d", &ignore_ready) )
	{
		ignore_hw_ready = ignore_ready ? true : false;
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb.0/force_hw_not_ready (read/write)
//
static ssize_t show_force_hw_not_ready(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", force_hw_not_ready) );
}

static ssize_t store_force_hw_not_ready(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, force_not_ready;
	
	if ( sscanf(buf, "%d", &force_not_ready) )
	{
		force_hw_not_ready = force_not_ready ? true : false;
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb.0/override_upd_mode (read/write)
//
static ssize_t show_override_upd_mode(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", override_upd_mode) );
}

static ssize_t store_override_upd_mode(FB_DSTOR_PARAMS)
{
	int result = -EINVAL, upd_mode;
	
	if ( sscanf(buf, "%d", &upd_mode) )
	{
		switch ( upd_mode )
		{
		    // Only override with valid upd_modes.
		    //
		    case BS_UPD_MODE_MU:
		    case BS_UPD_MODE_GU:
		    case BS_UPD_MODE_GC:
		    case BS_UPD_MODE_PU:
		        override_upd_mode = upd_mode;
		    break;
		    
		    // Default to not overriding if the value
		    // is invalid.
		    //
		    default:
		        override_upd_mode = BS_UPD_MODE_INIT;
		    break;
		}

		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb.0/preflight_failure (read-only)
//
static ssize_t show_preflight_failure(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", (int)bs_get_preflight_failure()) );
}

// /sys/devices/platform/eink_fb.0/image_timings (read-only)
//
static ssize_t show_image_timings(FB_DSHOW_PARAMS)
{
	int i, len, num_timings;
	unsigned long *timings = bs_get_img_timings(&num_timings);
	
	for ( i = 0, len = 0; i < num_timings; i++ )
	    len += sprintf(&buf[len], "%ld%s", timings[i], (i == (num_timings - 1)) ? "" : " " );
	    
	return ( len );
}

static DEVICE_ATTR(eink_rom_select,          DEVICE_MODE_RW, show_eink_rom_select,          store_eink_rom_select);
static DEVICE_ATTR(eink_ram_select,          DEVICE_MODE_RW, show_eink_ram_select,          store_eink_ram_select);
static DEVICE_ATTR(ignore_hw_ready,          DEVICE_MODE_RW, show_ignore_hw_ready,          store_ignore_hw_ready);
static DEVICE_ATTR(force_hw_not_ready,       DEVICE_MODE_RW, show_force_hw_not_ready,       store_force_hw_not_ready);
static DEVICE_ATTR(override_upd_mode,        DEVICE_MODE_RW, show_override_upd_mode,        store_override_upd_mode);
static DEVICE_ATTR(preflight_failure,        DEVICE_MODE_R,  show_preflight_failure,        NULL);
static DEVICE_ATTR(image_timings,            DEVICE_MODE_R,  show_image_timings,            NULL);

#if PRAGMAS
    #pragma mark -
    #pragma mark Watchdog Timer & Thread
    #pragma mark -
#endif

static void exit_broadsheet_watchdog_thread(void)
{
    broadsheet_watchdog_thread_exit = 1;
    complete(&broadsheet_watchdog_thread_complete);
}

static void broadsheet_watchdog_thread_body(void)
{
    // Repair us in one way or another if we still should.
    //
    if ( broadsheet_watchdog_timer_active && broadsheet_watchdog_timer_primed )
    {
        broadsheet_watchdog_timer_primed = false;
        
        // Reset Broadsheet if it's not still ready.
        //
        if ( !BS_STILL_READY() && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
        {
            // If we were forcing the hardware not to be ready, unforce it now.
            // Forcing the hardware not to be ready is for debugging purposes,
            // and we treat it as a one-shot.
            //
            force_hw_not_ready = false;
            
            bs_sw_init(FULL_BRINGUP_CONTROLLER, DONT_BRINGUP_PANEL);
            bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
            EINKFB_LOCK_EXIT();
            
            // If Broadsheet's still not ready after we've tried to bring it
            // back up, tell the eInk HAL that we're in real trouble.
            //
            if ( !BS_STILL_READY() )
                EINKFB_NEEDS_RESET();
        }
        
        // Do a panel update repair if it hasn't already been done.
        //
        if ( !BS_UPD_REPAIR() && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
        {
            bs_cmd_upd_repair();
            EINKFB_LOCK_EXIT();
        }
    }
    
    wait_for_completion_interruptible(&broadsheet_watchdog_thread_complete);
}

static int broadsheet_watchdog_thread(void *data)
{
    int  *exit_thread = (int *)data;
    bool thread_active = true;
    
    THREAD_HEAD(BROADSHEET_WATCHDOG_THREAD_NAME);

    while ( thread_active )
    {
        TRY_TO_FREEZE();

        if ( !THREAD_SHOULD_STOP() )
        {
            THREAD_BODY(*exit_thread, broadsheet_watchdog_thread_body);
        }
        else
            thread_active = false;
    }

    THREAD_TAIL(broadsheet_watchdog_thread_exited);
}

static void broadsheet_start_watchdog_thread(void)
{
    THREAD_START(broadsheet_watchdog_thread_id, &broadsheet_watchdog_thread_exit, broadsheet_watchdog_thread,
        BROADSHEET_WATCHDOG_THREAD_NAME);
}

static void broadsheet_stop_watchdog_thread(void)
{
    THREAD_STOP(broadsheet_watchdog_thread_id, exit_broadsheet_watchdog_thread, broadsheet_watchdog_thread_exited);
}

static void broadsheet_watchdog_timer_function(unsigned long unused)
{
    complete(&broadsheet_watchdog_thread_complete);
}

static void broadsheet_start_watchdog_timer(void)
{
	init_timer(&broadsheet_watchdog_timer);
	
	broadsheet_watchdog_timer.function = broadsheet_watchdog_timer_function;
	broadsheet_watchdog_timer.data = 0;

	broadsheet_watchdog_timer_active  = true;
    broadsheet_watchdog_timer_primed  = false;
    
    broadsheet_start_watchdog_thread();
}

static void broadsheet_stop_watchdog_timer(void)
{
	if ( broadsheet_watchdog_timer_active )
	{
		broadsheet_watchdog_timer_active  = false;
		broadsheet_watchdog_timer_primed  = false;

		broadsheet_stop_watchdog_thread();
		del_timer_sync(&broadsheet_watchdog_timer);
	}
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Misc. Local Utilities
    #pragma mark -
#endif

static void broadsheet_blank_screen(void)
{
    broadsheet_set_power_state(bs_power_state_run);
    broadsheet_clear_screen(fx_update_full);
}

static void broadsheet_preflight(void)
{
    int preflight_retry_count = 0;
    bool done = false;
    
    // Tell the controller-specific Broadsheet code to run its preflighting checks.
    //
    do
    {
        if ( !bs_preflight_passes() )
            preflight_retry_count++;
        else
            done = true;
    }
    while ( !done && (MAX_PREFLIGHT_RETRIES < preflight_retry_count) );
    
    // Put us into bootstrapping mode if the preflighting checks have failed or we're
    // supposed to leave the display as is.
    //
    if ( !done || BS_DISPLAY_ASIS() )
        broadsheet_set_bootstrap_state(true);
    
    // Until we know otherwise, say that the display should be brought up
    // fully (slowly) next time.
    //
    broadsheet_set_init_display_flag(BS_INIT_DISPLAY_SLOW);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL API
    #pragma mark -
#endif

unsigned char *broadsheet_get_scratchfb(void)
{
    return ( scratchfb );
}

unsigned long  broadsheet_get_scratchfb_size(void)
{
    return ( scratchfb_size );
}

bs_flash_select broadsheet_get_flash_select(void)
{
    return ( eink_rom_select );
}

void broadsheet_set_flash_select(bs_flash_select flash_select)
{
    if ( !(EINKFB_FLASHING_ROM() || eink_rom_select_locked) )
    {
        switch ( flash_select )
        {
            case bs_flash_waveform:
            case bs_flash_commands:
            case bs_flash_test:
                eink_rom_select = flash_select;
            break;
            
            // Prevent the compiler from complaining.
            //
            default:
            break;
        }
    }
}

int broadsheet_get_ram_select_size(void)
{
    return ( BROADSHEET_RAM_BANK_SIZE );
}

int broadsheet_get_ram_select(void)
{
    return ( eink_ram_select );
}

void broadsheet_set_ram_select(int ram_select)
{
    if ( !eink_ram_select_locked && IN_RANGE(ram_select, 0, (BROADSHEET_NUM_RAM_BANKS-1)) )
        eink_ram_select = ram_select;
}

bool broadsheet_get_orientation(void)
{
    return ( bs_orientation );
}

bool broadsheet_get_upside_down(void)
{
    return ( bs_upside_down );
}

int broadsheet_get_override_upd_mode(void)
{
    return ( override_upd_mode );
}

bool broadsheet_ignore_hw_ready(void)
{
    return ( ignore_hw_ready );
}

void broadsheet_set_ignore_hw_ready(bool value)
{
    ignore_hw_ready = value ? true : false;
}

bool broadsheet_force_hw_not_ready(void)
{
    return ( force_hw_not_ready );
}

int broadsheet_get_recent_commands(char *page, int max_commands)
{
    int result = 0;

    recent_commands_max = max_commands;
    result = read_recent_commands(page, 0, 0);
    
    recent_commands_max = MAX_RECENT_COMMANDS;
    
    return ( result );
}

void broadsheet_prime_watchdog_timer(bool delay_timer)
{
	if ( broadsheet_watchdog_timer_active )
	{
		unsigned long timer_delay;
		
		// If Broadsheet isn't still in its ready state, ensure
		// that we don't expire the timer.
		//
		if ( !BS_STILL_READY() )
		    delay_timer = true;
		
		// If requested, delay the timer.
		//
		if ( delay_timer )
		{
		    timer_delay = BROADSHEET_WATCHDOG_TIMER_DELAY;
		    broadsheet_watchdog_timer_primed = true;
		}
		else
		{
		    // Otherwise, expire the timer immediately.
		    //
		    timer_delay = MIN_SCHEDULE_TIMEOUT;
		    broadsheet_watchdog_timer_primed = false;
		}

        mod_timer(&broadsheet_watchdog_timer, TIMER_EXPIRES_JIFS(timer_delay));
	}
}

static bool broadsheet_ep3_picture_mode_switch(int picture_mode)
{
	if (bs_picture_normal_mode == picture_mode)
		{
		bs_current_picture_mode = bs_picture_normal_mode;
		return true;
		}
	else if (bs_picture_inverted_mode == picture_mode)
		{
		bs_current_picture_mode = bs_picture_inverted_mode;
		return true;
		}
	return false;
}

unsigned char broadsheet_ep3_get_picture_mode(void)
{
	return bs_current_picture_mode;
}
EXPORT_SYMBOL_GPL(broadsheet_ep3_get_picture_mode);

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL module operations
    #pragma mark -
#endif

/*--Start--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
#ifdef BS_POWER_MODE_SWITCH

static void do_bs_mode_switch(bs_power_mode bs_mode){

	if (bs_current_power_mode != bs_mode)
//		printk("----oooooooo----do_bs_mode_switch --> mode= %d  \n", bs_mode);
		switch (bs_mode)
		{
			case bs_power_run_mode:
				bs_current_power_mode = bs_power_run_mode;
				bs_wr_one_ready();
				bs_cmd_run_sys();
				bs_wr_one_ready();
			break;

			case bs_power_standby_mode:
				bs_current_power_mode = bs_power_standby_mode;
				bs_wr_one_ready();
				bs_cmd_stby();
				bs_wr_one_ready();
			break;

			case bs_power_sleep_mode:
				bs_current_power_mode = bs_power_sleep_mode;
				bs_wr_one_ready();
				bs_cmd_slp();
				bs_wr_one_ready();				
			break;

			default :
				printk("unknown bs_power mode = %d  \n", bs_mode);
			break;
		}
}

void save_bs_mode_var(bs_power_mode bs_mode){	

		switch (bs_mode)
		{
			case bs_power_run_mode:
				bs_current_power_mode = bs_power_run_mode;
			break;

			case bs_power_standby_mode:
				bs_current_power_mode = bs_power_standby_mode;
			break;

			case bs_power_sleep_mode:
				bs_current_power_mode = bs_power_sleep_mode;
			break;

			default :
				printk("unknown bs_power mode = %d  \n", bs_mode);
			break;
		}

}
static void do_bs_power_save(void)
{
	if (bs_handwrite == true){
//		printk("----oooooooo----can't go to sleep or standby mode during handwrite  \n");
		do_bs_mode_switch(bs_power_run_mode);
//		do_bs_mode_switch(bs_power_standby_mode);
	}
	else{
		do_bs_mode_switch(bs_power_sleep_mode);
	}

}

static void do_bs_power_enable(void)
{
//	printk("----oooooooo----do_bs_power_enable --> run \n");
	do_bs_mode_switch(bs_power_run_mode);
}

extern void epd_check_bs_handwrite(bool mode);
void epd_check_bs_handwrite(bool mode)
{
//	printk("----oooooooo----epd_check_bs_handwrite --> handwrite mode= %d  \n", mode);
	if (mode){
		bs_handwrite = true ;

		do_bs_power_enable();
	}else{
		bs_handwrite = false ;
	}
}

#endif 
/*--End--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/

static void do_wait_dspe_frend(void);
static bool broadsheet_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    bool result = EINKFB_FAILURE;
    bs_resolution_t res;
    
    // If we're overridden with a value other than portrait or landscape, switch us
    // back to the default.
    //
    switch ( bs_orientation )
    {
        case EINKFB_ORIENT_LANDSCAPE:
        case EINKFB_ORIENT_PORTRAIT:
        break;
        
        default:
            bs_orientation = EINKFB_ORIENT_PORTRAIT;
        break;
    }

    // Initalize enough of the hardware to ascertain the resolution we need to set up for.
    //
    bs_sw_init_controller(FULL_BRINGUP_CONTROLLER, bs_orientation, &res);
    scratchfb_size = BPP_SIZE((res.x_hw*res.y_hw), EINKFB_8BPP);
    
    // TODO:  Call einkfb_malloc() when it automatically handles
    //        vmalloc vs. kmalloc.
    //
    scratchfb = vmalloc(scratchfb_size);
    
    if ( scratchfb )
    {
        switch ( bs_bpp )
        {
            // If we're overridden with a value other than 2bpp or 4bpp, switch us
            // back to the default.
            //
            default:
                bs_bpp = BPP;

            case EINKFB_2BPP:
            case EINKFB_4BPP:
                broadsheet_size                 = BPP_SIZE((res.x_sw*res.y_sw), bs_bpp);

                broadsheet_var.bits_per_pixel   = bs_bpp;
                
                broadsheet_fix.smem_len         = broadsheet_size;
                broadsheet_fix.line_length      = BPP_SIZE(res.x_sw, bs_bpp);
            break;
        }
        
        broadsheet_var.xres = broadsheet_var.xres_virtual = res.x_sw;
        broadsheet_var.yres = broadsheet_var.yres_virtual = res.y_sw;
        
        broadsheet_var.width  = res.x_mm;
#ifdef EX3_ANDROID
     //   broadsheet_var.xres = broadsheet_var.xres_virtual = res.x_sw;
      //  broadsheet_var.yres = broadsheet_var.yres_virtual = res.y_sw;
        
        broadsheet_var.xres=XRES;
        broadsheet_var.yres=YRES;
        broadsheet_var.xres_virtual=XRES;
        broadsheet_var.yres_virtual=YRES;
        broadsheet_var.bits_per_pixel=BPP;
        broadsheet_fix.smem_len=BPP_SIZE((XRES*YRES), BPP);
        broadsheet_fix.line_length=BPP_SIZE(XRES, BPP);
        broadsheet_var.activate = FB_ACTIVATE_NOW;
        broadsheet_var.width 	= XRES;
        broadsheet_var.height = YRES;
        broadsheet_var.red.offset=11;
        broadsheet_var.red.length=5; 
        broadsheet_var.red.msb_right=0;
        broadsheet_var.green.offset=5;
        broadsheet_var.green.length=6;
        broadsheet_var.green.msb_right=0;
        broadsheet_var.blue.offset=0;
        broadsheet_var.blue.length=5;
        broadsheet_var.blue.msb_right=0;
        broadsheet_var.transp.offset=0;
        broadsheet_var.transp.length=0;
        broadsheet_var.transp.msb_right=0;
#endif
        broadsheet_var.height = res.y_mm;

        *var = broadsheet_var;
        *fix = broadsheet_fix;
        
        result = EINKFB_SUCCESS;
    }

/*--Start--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
#ifdef BS_POWER_MODE_SWITCH
//	printk("----oooooooo----INIT_DELAYED_WORK for BS_POWER_MODE  \n");
	//broadsheet_power_mode = kzalloc(sizeof(struct broadsheet_power_mode), GFP_KERNEL);
	//INIT_DELAYED_WORK(&broadsheet_power_mode->bs_delay_work, do_bs_power_save);
#endif 
/*--End--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
/* Henry Li: 20100828, execute EPD DSPE_FREND command in workqueue */
	broadsheet_power_mode = kzalloc(sizeof(struct broadsheet_power_mode), GFP_KERNEL);
	INIT_DELAYED_WORK(&broadsheet_power_mode->bs_delay_work, do_wait_dspe_frend);
    return ( result );
}

static void broadsheet_sw_done(void)
{
    // TODO:  Call einkfb_free() once moved to einkfb_malloc().
    //
    if ( scratchfb )
        vfree(scratchfb);
}

static bool broadsheet_hw_init(struct fb_info *info, bool full)
{
    bool result = bs_sw_init_panel(full || !FB_INITED());
    
    broadsheet_flash_size = bs_get_sfm_size();
    broadsheet_start_watchdog_timer();

    return ( result );
}

static void broadsheet_hw_done(bool full)
{
    broadsheet_stop_watchdog_timer();
    bs_sw_done();
}

static void broadsheet_create_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);

    // Create Broadsheet-specific proc entries.
    //
    einkfb_proc_waveform_version = einkfb_create_proc_entry(EINKFB_PROC_WAVEFORM_VERSION, EINKFB_PROC_CHILD_R,
        waveform_version_read, NULL);
    
    einkfb_proc_commands_version = einkfb_create_proc_entry(EINKFB_PROC_COMMANDS_VERSION, EINKFB_PROC_CHILD_R,
        commands_version_read, NULL);
    
    einkfb_proc_recent_commands = einkfb_create_proc_entry(EINKFB_PROC_RECENT_COMMANDS, EINKFB_PROC_CHILD_R,
        recent_commands_read, NULL);

    einkfb_proc_temperature = einkfb_create_proc_entry(EINKFB_PROC_TEMPERATURE, EINKFB_PROC_CHILD_R,
        temperature_read, NULL);

    einkfb_proc_hardwarefb = einkfb_create_proc_entry(EINKFB_PROC_HARDWAREFB, EINKFB_PROC_CHILD_R,
        hardwarefb_read, NULL);

    einkfb_proc_eink_rom = einkfb_create_proc_entry(EINKFB_PROC_EINK_ROM, EINKFB_PROC_CHILD_RW,
        eink_rom_read, eink_rom_write);
        
    einkfb_proc_eink_ram = einkfb_create_proc_entry(EINKFB_PROC_EINK_RAM, EINKFB_PROC_CHILD_RW,
        eink_ram_read, eink_ram_write);
        
    einkfb_proc_eink_reg = einkfb_create_proc_entry(EINKFB_PROC_EINK_REG, EINKFB_PROC_CHILD_W,
        NULL, eink_reg_write);

    // Create Broadsheet-specific sysfs entries.
    //
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_eink_rom_select);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_eink_ram_select);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_ignore_hw_ready);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_force_hw_not_ready);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_override_upd_mode);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_preflight_failure);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_image_timings);
}

static void broadsheet_remove_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);
    
    // Remove Broadsheet-specific proc entries.
    //
    einkfb_remove_proc_entry(EINKFB_PROC_WAVEFORM_VERSION, einkfb_proc_waveform_version);
    einkfb_remove_proc_entry(EINKFB_PROC_COMMANDS_VERSION, einkfb_proc_commands_version);
    einkfb_remove_proc_entry(EINKFB_PROC_RECENT_COMMANDS, einkfb_proc_recent_commands);
    einkfb_remove_proc_entry(EINKFB_PROC_TEMPERATURE, einkfb_proc_temperature);
    einkfb_remove_proc_entry(EINKFB_PROC_HARDWAREFB, einkfb_proc_hardwarefb);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_ROM, einkfb_proc_eink_rom);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_RAM, einkfb_proc_eink_ram);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_REG, einkfb_proc_eink_reg);
    
    // Remove Broadsheet-specific sysfs entries.
    //
	device_remove_file(&info.dev->dev, &dev_attr_eink_rom_select);
	device_remove_file(&info.dev->dev, &dev_attr_eink_ram_select);
	device_remove_file(&info.dev->dev, &dev_attr_ignore_hw_ready);
	device_remove_file(&info.dev->dev, &dev_attr_force_hw_not_ready);
	device_remove_file(&info.dev->dev, &dev_attr_override_upd_mode);
	device_remove_file(&info.dev->dev, &dev_attr_preflight_failure);
	device_remove_file(&info.dev->dev, &dev_attr_image_timings);
}

static void broadsheet_update_display(fx_type update_mode)
{
    BS_CMD_LD_IMG_UPD_DATA(update_mode);
}



static void broadsheet_update_area(update_area_t *update_area)
{
    int xstart  = update_area->x1, xend = update_area->x2,
        ystart  = update_area->y1, yend = update_area->y2,
        xres    = xend - xstart,
        yres    = yend - ystart;

    bs_cmd_ld_img_area_upd_data(update_area->buffer, update_area->which_fx, xstart, ystart, xres, yres);
  //  ex3_bs_cmd_ld_img_area_upd_data(update_area->buffer, update_area->which_fx,  1,  xstart, ystart, xres, yres);
}



extern void bs_ep3_penwrite_mode(bool mode, unsigned char drawdata[10] );
static void broadsheet_set_power_level(einkfb_power_level power_level)
{
    int power_state;
    
    switch ( power_level )
    {
        case einkfb_power_level_on:
            power_state = bs_power_state_run;
        goto set_power_state;
        
        case einkfb_power_level_standby:
            //power_state = bs_power_state_sleep;
            power_state = bs_power_state_run;
        goto set_power_state;

        case einkfb_power_level_sleep:
	     bs_wr_one_ready();

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    u16 rd_reg;
    unsigned char dump[10];
	
    bs_ep3_penwrite_mode(1, dump);
    rd_reg = bs_cmd_rd_reg(0x2cc) | (1 << 6);
    bs_cmd_wr_reg(0x2cc, (rd_reg | (1 << 6)));	//transmit BREAK condition
    //bs_cmd_wr_reg(0x2cc, (rd_reg & ~(1 << 14)));
					//bit 14 = 0b, the UART is disable
#endif			
	     bs_wr_one_ready();
            bs_cmd_slp();
	     bs_wr_one_ready();			
            power_state = bs_power_state_off;
        /*goto set_power_state;*/

        set_power_state:
            broadsheet_set_power_state(power_state);
        break;
        
        // Blank screen in eInk HAL's standby mode.
        //
        case einkfb_power_level_blank:
            broadsheet_blank_screen();
            broadsheet_set_power_level(einkfb_power_level_standby);
        break;
        
        // Blank screen in Broadsheet's off_screen_clear mode.
        //
        case einkfb_power_level_off:
    //        broadsheet_blank_screen();
     //       broadsheet_set_power_state(bs_power_state_off_screen_clear);
        break;

        // Prevent the compiler from complaining.
        //
        default:
        break;
    }
}

einkfb_power_level broadsheet_get_power_level(void)
{
    einkfb_power_level power_level;
    
    switch ( broadsheet_get_power_state() )
    {
        case bs_power_state_run:
            //power_level = einkfb_power_level_on;
            power_level = einkfb_power_level_standby;
        break;
        
        case bs_power_state_sleep:
            power_level = einkfb_power_level_standby;
        break;
    
        case bs_power_state_off:
            power_level = einkfb_power_level_sleep;
        break;
        
        case bs_power_state_off_screen_clear:
            power_level = einkfb_power_level_off;
        break;
    
        default:
            power_level = einkfb_power_level_init;
        break;
    }
    
    return ( power_level );
}

static void broadsheet_reboot_hook(reboot_behavior_t behavior)
{
    // Remember how we were rebooted or shut down so that we can bring things back up
    // properly again next time, but only if we've been initialized properly.
    //
    if ( FB_INITED() )
    {
        switch ( behavior )
        {
            case reboot_screen_asis:
                broadsheet_set_init_display_flag(BS_INIT_DISPLAY_ASIS);
            break;
            
            case reboot_screen_splash:
                broadsheet_set_init_display_flag(BS_INIT_DISPLAY_FAST);
            break;
            
            case reboot_screen_clear:
                broadsheet_set_init_display_flag(BS_INIT_DISPLAY_SLOW);
            break;
        }
    }
}
	
static unsigned long broadsheet_byte_alignment(void)
{
    return ( BROADSHEET_BTYE_ALIGNMENT );
}
//-------------Henry Li: 2010.8.28 timer for display update operation-------
static struct timer_list broadsheet_update_timer;
static int broadsheet_update_operation=false;
static int broadsheet_update_timer_flag=false;
static int broadsheet_wait_dspe_frend=false;

static void broadsheet_stop_update_timer(void)
{
		del_timer_sync(&broadsheet_update_timer);
		broadsheet_update_timer_flag=false;
}
static void broadsheet_update_timer_function(unsigned long unused)
{
	broadsheet_update_operation = false;
}

static void broadsheet_init_update_timer(void)
{
	if (broadsheet_update_timer_flag != true)
	{
	init_timer(&broadsheet_update_timer);
	
	broadsheet_update_timer.function = broadsheet_update_timer_function;
	broadsheet_update_timer.data = 0;
	broadsheet_update_timer_flag=true;
	}
}

static void broadsheet_start_update_timer(unsigned long timer_delay)
{
	if (broadsheet_update_timer_flag != true)
	{
		broadsheet_init_update_timer();
	}
       broadsheet_update_operation = true;
       mod_timer(&broadsheet_update_timer, TIMER_EXPIRES_JIFS(timer_delay));	
}

extern unsigned char einkfb_get_dynamic_parameter(void);
/* Henry Li: check multi-LUT region overlap or not */
static void broadsheet_check_overlap(void)
{
      unsigned long interval=0;
	bool overlap_flag=false;
       int i,j;
	bool dump[15];
	int dump_counter;
	int next_left, next_top, next_left_width, next_top_height;
	unsigned long next_moment;
       int previous_datasize, next_datasize;		// 1004

       if  ( einkfb_get_dynamic_parameter() == 1 )
	   	return;
	memset(dump, false, 15);
	if (data_update_counter >= 15)
	{
		for (i=0; i < 14; i++)
		{
      		previous_left[i] = previous_left[i+1];
		previous_top[i] = previous_top[i+1];
		previous_left_width[i] = previous_left_width[i+1] ;
		previous_top_height[i] = previous_top_height[i+1];
		previous_moment[i] = previous_moment[i+1];	
		previous_mode[i] = previous_mode[i+1] ;
		}
		data_update_counter--;
	}
      	previous_left[data_update_counter] = global_left;
	previous_top[data_update_counter] = global_top;
	previous_left_width[data_update_counter] = global_left_width;
	previous_top_height[data_update_counter] = global_top_height;
	previous_moment[data_update_counter] = jiffies;

	if (data_update_counter <15)
		data_update_counter++;

	if (data_update_counter  < 2)
		return;

      	next_left = previous_left[data_update_counter-1];
	next_top = previous_top[data_update_counter-1];
	next_left_width = previous_left_width[data_update_counter-1] ;
	next_top_height = previous_top_height[data_update_counter-1];
	next_moment = previous_moment[data_update_counter-1];
	next_datasize =((next_left_width - next_left) * (next_top_height - next_top))/2;

	for (i=data_update_counter-2; i >= 0; i--)
	{
		if ( UPDATE_GC == previous_mode[i])
			interval = 1680;	//2080;
		else if ( UPDATE_GU == previous_mode[i])
			interval = 1000;		
		else
			interval = 900;	//1100;

/*
		previous_datasize =((previous_left_width[i] - previous_left[i]) * (previous_top_height[i] - previous_top[i]))/2;
		if ((next_datasize <= 2048) && (previous_datasize <= 2048) && (UPDATE_DU== previous_mode[i]))
		{		
	              if  (  ( ((next_left >= previous_left[i])  && (next_left < (previous_left[i]+64))) || ((next_left <= previous_left[i])  && ( (next_left +64) >=  previous_left[i]))) \
			   && ( ((next_top >= previous_top[i])  && (next_top < (previous_top[i]+64))) || ((next_top <= previous_top[i])  && ( (next_top +64) >=  previous_top[i])))  )
              	{
	            		  if (  ( ((next_left_width >= previous_left_width[i])  && (next_left_width < (previous_left_width[i]+64))) || ((next_left_width <= previous_left_width[i])  && ( (next_left_width +64) >=  previous_left_width[i]))) \
			  		&& ( ((next_top_height >= previous_top_height[i])  && (next_top_height < (previous_top_height[i]+64))) || ((next_top_height <= previous_top_height[i])  && ( (next_top_height +64) >=  previous_top_height[i])))  )
	            		  	{
					dump[i] = true;
					continue;
					}
              	}
	              if  (  ( ((next_left >= previous_left[i])  && (next_left < (previous_left[i]+32))) || ((next_left <= previous_left[i])  && ( (next_left +32) >=  previous_left[i]))) \
			   && ( ((next_top >= previous_top[i])  && (next_top < (previous_top[i]+32))) || ((next_top <= previous_top[i])  && ( (next_top +32) >=  previous_top[i])))  )
              	{
	            		  if (  ( ((next_left_width >= previous_left_width[i])  && (next_left_width < (previous_left_width[i]+128))) || ((next_left_width <= previous_left_width[i])  && ( (next_left_width +128) >=  previous_left_width[i]))) \
			  		&& ( ((next_top_height >= previous_top_height[i])  && (next_top_height < (previous_top_height[i]+128))) || ((next_top_height <= previous_top_height[i])  && ( (next_top_height +128) >=  previous_top_height[i])))  )
	            		  	{
					dump[i] = true;
					continue;
					}
              	}
            		  if (  ( ((next_left_width >= previous_left_width[i])  && (next_left_width < (previous_left_width[i]+32))) || ((next_left_width <= previous_left_width[i])  && ( (next_left_width +32) >=  previous_left_width[i]))) \
			  		&& ( ((next_top_height >= previous_top_height[i])  && (next_top_height < (previous_top_height[i]+32))) || ((next_top_height <= previous_top_height[i])  && ( (next_top_height +32) >=  previous_top_height[i])))  )
              	{
		              if  (  ( ((next_left >= previous_left[i])  && (next_left < (previous_left[i]+128))) || ((next_left <= previous_left[i])  && ( (next_left +128) >=  previous_left[i]))) \
		  		     && ( ((next_top >= previous_top[i])  && (next_top < (previous_top[i]+128))) || ((next_top <= previous_top[i])  && ( (next_top +128) >=  previous_top[i])))  )
	            		  	{
					dump[i] = true;
					continue;
					}
              	}
		}
*/		
		if (jiffies_to_msecs(next_moment  - previous_moment[i]) >= interval)
		{
		dump[i] = true;
		continue;
		}


		if (((next_left <= previous_left[i]) && (next_top <= previous_top[i]))  &&  \
			((  next_left_width >  previous_left[i] ) &&  (next_top_height > previous_top[i])))
		{
			overlap_flag = true;	
			break;
		}
		if (((next_left >= previous_left[i]) && (next_top > previous_top[i]))  &&  \
			(( next_left <  previous_left_width[i] ) &&  (next_top < previous_top_height[i])))
		{
			overlap_flag = true;	
			break;
		}

		if (((next_left > previous_left[i]) && (next_top >= previous_top[i]))  &&  \
			(( next_left <  previous_left_width[i] ) &&  (next_top <  previous_top_height[i])))
		{
		
			overlap_flag = true;	
			break;
		}

		if (((next_left == previous_left[i]) && (next_top == previous_top[i]))  &&  \
			(( next_left_width >  previous_left[i] ) &&  (next_top_height >  previous_top[i])) && \			
			(( next_left <  previous_left_width[i] ) &&  (next_top <  previous_top_height[i])))
		{		
			overlap_flag = true;	
			break;
		}

	}	

	dump_counter=0;
	for (i=data_update_counter-2; i >= 0; i--)
		{
		if (true == dump[i])
			{
			for (j=i; j < (data_update_counter-dump_counter-1); j++)
				{
			      	previous_left[j] = previous_left[j+1];
				previous_top[j] = previous_top[j+1];
				previous_left_width[j] = previous_left_width[j+1] ;
				previous_top_height[j] = previous_top_height[j+1];
				previous_moment[j] = previous_moment[j+1];	
				previous_mode[j] = previous_mode[j+1] ;				
				}			
			dump_counter++;
			}
		}
	data_update_counter=data_update_counter-dump_counter;
	
	if ( true == overlap_flag ) 
		{
		einkfb_debug_ioctl("do epd_write_command(0x29)\n");
  		epd_write_command(0x29);  
		broadsheet_wait_dspe_frend=false;
		overlap_flag=false;
		//reinit 
	      	previous_left[0] = global_left;
		previous_top[0] = global_top;
		previous_left_width[0] = global_left_width;
		previous_top_height[0] = global_top_height;
		previous_moment[0] = jiffies;
		data_update_counter = 1;		
		}
}
/* Henry Li: wait last display-update finished */
void broadsheet_check_finished(void)
{
      unsigned long interval=0;
	int next_left, next_top, next_left_width, next_top_height;
	unsigned long  next_moment;
	unsigned int delaycounter=0;
	
	while ( global_display_stage == 1) 
	{
		delaycounter++;
		msleep(1);
		if (delaycounter > 5000)
		{
			break;
		}
	}  
	delaycounter=0;
	if (data_update_counter  >= 1)
	{
		epd_write_command(0x29);  
		broadsheet_wait_dspe_frend=false;
	}

	if (data_update_counter  < 1)
		interval = 1680;	//1580;
	else
		{
	      	next_left = previous_left[data_update_counter-1];
		next_top = previous_top[data_update_counter-1];
		next_left_width = previous_left_width[data_update_counter-1] ;
		next_top_height = previous_top_height[data_update_counter-1];
		next_moment = previous_moment[data_update_counter-1];

		if ( UPDATE_GC == previous_mode[data_update_counter-1])
			interval = 1680*2;	//1580;
		else if ( UPDATE_GU == previous_mode[data_update_counter-1])
			interval = 1000*3;		
		else
			interval = 900*3;	//1100;
		}
	while (jiffies_to_msecs(jiffies- global_moment) <  interval)
	{
		delaycounter++;
		msleep(1);
		if (delaycounter > 5000)
		{
			break;
		}
	}  
	data_update_counter = 0;	
}
EXPORT_SYMBOL_GPL(broadsheet_check_finished);

static void broadsheet_update_complete_verify(void)
{
	unsigned int delaycounter=0;
	while ((broadsheet_update_operation == true) || (broadsheet_wait_dspe_frend == true))
	{
		delaycounter++;
		//udelay(100);
		msleep(1);
		//if (delaycounter > 12000)
		if (delaycounter > 1200)
		{
			break;
		}
	}  
	delaycounter=0;
	while (broadsheet_wait_dspe_frend == true)
	{
		delaycounter++;
		//udelay(100);
		msleep(1);
		//if (delaycounter > 12000)
		if (delaycounter > 1200)
		{
			break;
		}

	}  

     u16 lutmsk=0xffff;
     bs_cmd_wait_dspe_mlutfree(lutmsk);
     u16 rd_reg;
     unsigned short value = 0;	
     rd_reg = bs_cmd_rd_reg(0x338) & (1 << 5);
     while (rd_reg == value)		//bit5 =0, no LUTs are available
    	{
		delaycounter++;
		udelay(100);
		if (delaycounter > 12000)
		{
			break;
		}
		rd_reg = bs_cmd_rd_reg(0x338) & (1 << 5);
    	}
      if  (broadsheet_wait_dspe_frend == true)
		cancel_delayed_work(&broadsheet_power_mode->bs_delay_work);	 
	  
}

static void do_wait_dspe_frend(void)
{
	if (broadsheet_wait_dspe_frend == true){
  		//WAIT_DSPE_FREND
  		epd_write_command(0x29);  
		broadsheet_wait_dspe_frend=false;
		data_update_counter = 0;
	}
}

static int get_recycle_lut_number(void)
{
     global_lut_number = (global_lut_number+1)%15;
     return global_lut_number;
}

//&*&*&*20100203_JJ1 HIRQ interrupt mode
#define Interrupt_Control_Register 0x0002
#define Display_Engine_Interrupt 0x0080

extern void epd_set_HIRQ(unsigned short cmd);
void epd_set_HIRQ(unsigned short cmd)
{
	volatile unsigned short reg;
	
	reg = epd_read_epson_reg(0x10, 0x033A);
	
	printk("!![epd_set_HIRQ.1][0x033Ah]=0x%04x (bit 7),index=%d	,cmd=%d\n",		reg,EPD_BUFFER_INDEX,cmd);
		
	/*cmd=0 init HIRQ*/
	if(reg&0x80 == 0x80 && cmd != 0)
	{		
	
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX)>>16);	//para2 REG[0146h]
			
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX)>>16);	//para2 REG[0312h]	
#endif

			//UPD_FULL_AREA	XStart,YStart,Width,Height
			epd_write_command(0x34);
			epd_write_data(0x0006|(0x2<<8));//para1 REG[0334h]
			epd_write_data(0);									//para2 Xstart REG[0340h]
			epd_write_data(40);									//para3 Ystart REG[0342h]
			epd_write_data(600);						//para4 X-End  REG[0344h]
			epd_write_data(800-40);						//para5 Y-End  REG[0346h]
					
			//WAIT_DSPE_TRG
			epd_write_command(0x28);
			
  		//WAIT_DSPE_FREND
  		//epd_write_command(0x29);  
  		
	printk("!![epd_set_HIRQ.2][0x033Ah]=0x%04x ,index=%d ,cmd=%d	\n",reg,EPD_BUFFER_INDEX,cmd);
  }		
		
	epd_write_epson_reg(0x11,0x0240, Interrupt_Control_Register);
	epd_write_epson_reg(0x11,0x0242, Interrupt_Control_Register);
	epd_write_epson_reg(0x11,0x0244, Interrupt_Control_Register);
	epd_write_epson_reg(0x11,0x033A, Display_Engine_Interrupt);
	epd_write_epson_reg(0x11,0x033C, Display_Engine_Interrupt);
	epd_write_epson_reg(0x11,0x033E, Display_Engine_Interrupt);
	
}
//&*&*&*20100203_JJ2 HIRQ interrupt mode

static bool broadsheet_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
#if X3_FOXCONN_IOCTL	
	unsigned short value = 0;
	int i,j;
	__u16 colorsample_pixel_3,colorsample_pixel_2,colorsample_pixel_1,colorsample_pixel_0;
	__u16 colorsample_pixel_7,colorsample_pixel_6,colorsample_pixel_5,colorsample_pixel_4;
	int x1,y1,x2,y2;
	int left,top,left_width,top_height;
	int mod_start_x,mod_end_x;
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
  //baseaddr = ioremap(0x10000000, 4);
	//u16 __iomem *epd_base = (u16 __iomem *)(info->screen_base+info->var.yoffset*2*XRES);
  	u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);
  
	x1 = var->reserved[1]&0xFFFF;
	y1  = (var->reserved[1]>>16)&0xFFFF;
	x2 = var->reserved[2]&0xFFFF;
	y2 = (var->reserved[2]>>16)&0xFFFF;

	mod_start_x  = x1 % 16;
	mod_end_x 	 = x2 % 16;
	
	left 			 = x1-(mod_start_x);
	top  			 = y1;
	top_height = y2;
	
	if(x2==XRES)
		left_width = x2;
	else
		left_width = x2+(16-mod_end_x);
  
#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{

#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
			//Buffer 0
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0146h]
			
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]
#endif

  		//INIT_ROTMODE[0140h] Rotation 90
			value = (0x1 << 8)|(0x2 << 4);
			epd_send_command(0x0B,value);	
    	
			//LD_IMG_AREA (0x22 + 5 parameters)
			epd_write_command(0x22);
			epd_write_data((0x2<<4));								//para1 			 REG[0140h]
			epd_write_data(left);										//para2 Xstart REG[014Ch]
			epd_write_data(top);										//para3 Ystart REG[014Eh]
			epd_write_data(left_width-left);				//para4 Width  REG[0150h]
			epd_write_data(top_height-top);					//para5 Height REG[0152h]
	  	
			udelay(1);
	  	
			//Host Memory Access Port Register[0154h]
			epd_send_command(0x11,(0x154));		

			for(j=top;j<top_height;j++) 
			{ 
				for(i=left;i<left_width;i+=16) 
				{
					int addr = (j)*(XRES)+i;

						colorsample_pixel_0 = epd_base[addr++] & 0xff;
						colorsample_pixel_1 = epd_base[addr++] & 0xff;
						colorsample_pixel_2 = epd_base[addr++] & 0xff;
						colorsample_pixel_3 = epd_base[addr++] & 0xff;
						colorsample_pixel_4 = epd_base[addr++] & 0xff;
						colorsample_pixel_5 = epd_base[addr++] & 0xff;		
						colorsample_pixel_6 = epd_base[addr++] & 0xff;
						colorsample_pixel_7 = epd_base[addr++] & 0xff;
																		 						
					__raw_writel((uint32_t)((colorsample_pixel_7<<28)|
																	(colorsample_pixel_6<<24)|
																	(colorsample_pixel_5<<20)|
																	(colorsample_pixel_4<<16)|
																	(colorsample_pixel_3<<12)|
																	(colorsample_pixel_2<<8)|
																	(colorsample_pixel_1<<4)|
																	(colorsample_pixel_0<<0)),baseaddr);	
																	
						colorsample_pixel_0 = epd_base[addr++] & 0xff;
						colorsample_pixel_1 = epd_base[addr++] & 0xff;
						colorsample_pixel_2 = epd_base[addr++] & 0xff;
						colorsample_pixel_3 = epd_base[addr++] & 0xff;
						colorsample_pixel_4 = epd_base[addr++] & 0xff;
						colorsample_pixel_5 = epd_base[addr++] & 0xff;
						colorsample_pixel_6 = epd_base[addr++] & 0xff;
						colorsample_pixel_7 = epd_base[addr++] & 0xff;
																		 						
					__raw_writel((uint32_t)((colorsample_pixel_7<<28)|
																	(colorsample_pixel_6<<24)|
																	(colorsample_pixel_5<<20)|
																	(colorsample_pixel_4<<16)|
																	(colorsample_pixel_3<<12)|
																	(colorsample_pixel_2<<8)|
																	(colorsample_pixel_1<<4)|
																	(colorsample_pixel_0<<0)),baseaddr);					
				} 
			}	   			
	
			//LD_IMG_END
			epd_write_command(0x23);

			{
					//UPD_PART_AREA	XStart,YStart,Width,Height
					epd_write_command(0x36);
					epd_write_data(0x0006|(0x2<<8));//para1 REG[0334h]
					epd_write_data(left);									//para2 Xstart REG[0340h]
					epd_write_data(top);									//para3 Ystart REG[0342h]
					epd_write_data(left_width-left);			//para4 X-End  REG[0344h]
					epd_write_data(top_height-top);				//para5 Y-End  REG[0346h]
			}
								
			//WAIT_DSPE_TRG
			epd_write_command(0x28);
			
  		//WAIT_DSPE_FREND
  		epd_write_command(0x29);  

#ifdef X3_EPD_POWER
  		EINKFB_LOCK_EXIT();
#endif
	}
	
#endif	
	return 0;
}


void epd_clean_buffer(unsigned short cmd)
{
	int i;	
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
  //baseaddr = ioremap(0x10000000, 4);
	
#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{		
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
		//LD_IMG_SETADR	(0x25 + 2 parameters) 
		epd_write_command(0x25);
		epd_write_data(ImageBufferaddr(cmd));			//para1 REG[0144h]
		epd_write_data(ImageBufferaddr(cmd)>>16);	//para2 REG[0146h]
		EPD_BUFFER_INDEX = cmd;
#endif
			
  	//INIT_ROTMODE[0140h] Rotation 90
		epd_send_command(0x0B,(0x1 << 8)|(0x2 << 4));	
		epd_send_command(0x20,(0x2 << 4));	
		udelay(1);
	
		//Host Memory Access Port Register[0154h]
		epd_send_command(0x11,(0x154));		
		
    for(i=0;i<XRES*YRES;i+=32) 
    {
				__raw_writel((uint32_t)(0xFFFFFFFF),baseaddr);	
				__raw_writel((uint32_t)(0xFFFFFFFF),baseaddr);				
				__raw_writel((uint32_t)(0xFFFFFFFF),baseaddr);	
				__raw_writel((uint32_t)(0xFFFFFFFF),baseaddr);				
		}
				
		//LD_IMG_END
		epd_write_command(0x23);
		
#ifdef X3_EPD_POWER
    EINKFB_LOCK_EXIT();
#endif

	}  		
}


/*Henry: Fill display with specified color 
pixel_mode: 0=black, 1=white, 2=dark gray, 3=light gray  */
void broadsheet_ep3_epd_fill_pixel(int pixel_mode)
{
    int value;
	int i;	
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
       //baseaddr = ioremap(0x10000000, 4);
	
    if (0 == pixel_mode)
		value = 0;
    else if ( 1 == pixel_mode)
		value = 0xffff;
    else if ( 2 == pixel_mode)
		value = 0x5555;
    else if ( 3 == pixel_mode)
		value = 0xaaaa;

#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{

#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
		//Buffer 0
		//LD_IMG_SETADR	(0x25 + 2 parameters) 
		epd_write_command(0x25);
		epd_write_data(ImageBufferaddr(0));			//para1 REG[0144h]
		epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0146h]
			
		//UPD_SET_IMGADR	(0x38 + 2 parameters) 
		epd_write_command(0x38);
		epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
		epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]
		EPD_BUFFER_INDEX = 0;
#endif	
		//epd_send_command(0x0B,(0x2 << 4));	
	
		//broadsheet_update_complete_verify();
  	//INIT_ROTMODE[0140h] Rotation 90
		//epd_send_command(0x0B,(0x1 << 8));	
		epd_send_command(0x0B,(0x1 << 8)|(0x2 << 4));
		epd_send_command(0x20,(0x2 << 4));	
		udelay(1);
	
		//Host Memory Access Port Register[0154h]
		epd_send_command(0x11,(0x154));	


	      for(i=0;i<XRES*YRES/4;i++) 
	      {
			__raw_writew((uint32_t)value,baseaddr);		//DCBA
		}
	
		//LD_IMG_END
		epd_write_command(0x23);

		//UPD_FULL[0334h]
		epd_send_command(0x33,(0x4200));	
	
		//WAIT_DSPE_TRG
		epd_write_command(0x28);
    
//#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
	  	//WAIT_DSPE_FREND
	  	epd_write_command(0x29);
		broadsheet_wait_dspe_frend=false;
        	data_update_counter = 0;
//#else
//		broadsheet_start_update_timer((HZ/10)*6);	//Miss 10:6  HZ/4 (HZ/10)*3 (HZ/10)*4
//#endif			  	
#ifdef X3_EPD_POWER
    EINKFB_LOCK_EXIT();
#endif
	}
}

void epd_show_second_logo(unsigned short cmd)
{
}
EXPORT_SYMBOL_GPL(epd_show_second_logo);


//?? improve the system suspend and resume time
extern void epd_show_black(unsigned short cmd);
void epd_show_black(unsigned short cmd)
{		
	broadsheet_ep3_epd_fill_pixel(0);
}


#define UPDATE_GU  0
#define UPDATE_GC  1
#define UPDATE_DU	20

static u8 bs_4bpp_nybble_swap_table_inverted[256] =
{
    0xFF, 0xEF, 0xDF, 0xCF, 0xBF, 0xAF, 0x9F, 0x8F, 0x7F, 0x6F, 0x5F, 0x4F, 0x3F, 0x2F, 0x1F, 0x0F,
    0xFE, 0xEE, 0xDE, 0xCE, 0xBE, 0xAE, 0x9E, 0x8E, 0x7E, 0x6E, 0x5E, 0x4E, 0x3E, 0x2E, 0x1E, 0x0E,
    0xFD, 0xED, 0xDD, 0xCD, 0xBD, 0xAD, 0x9D, 0x8D, 0x7D, 0x6D, 0x5D, 0x4D, 0x3D, 0x2D, 0x1D, 0x0D,
    0xFC, 0xEC, 0xDC, 0xCC, 0xBC, 0xAC, 0x9C, 0x8C, 0x7C, 0x6C, 0x5C, 0x4C, 0x3C, 0x2C, 0x1C, 0x0C,
    0xFB, 0xEB, 0xDB, 0xCB, 0xBB, 0xAB, 0x9B, 0x8B, 0x7B, 0x6B, 0x5B, 0x4B, 0x3B, 0x2B, 0x1B, 0x0B,
    0xFA, 0xEA, 0xDA, 0xCA, 0xBA, 0xAA, 0x9A, 0x8A, 0x7A, 0x6A, 0x5A, 0x4A, 0x3A, 0x2A, 0x1A, 0x0A,
    0xF9, 0xE9, 0xD9, 0xC9, 0xB9, 0xA9, 0x99, 0x89, 0x79, 0x69, 0x59, 0x49, 0x39, 0x29, 0x19, 0x09,
    0xF8, 0xE8, 0xD8, 0xC8, 0xB8, 0xA8, 0x98, 0x88, 0x78, 0x68, 0x58, 0x48, 0x38, 0x28, 0x18, 0x08,
    0xF7, 0xE7, 0xD7, 0xC7, 0xB7, 0xA7, 0x97, 0x87, 0x77, 0x67, 0x57, 0x47, 0x37, 0x27, 0x17, 0x07,
    0xF6, 0xE6, 0xD6, 0xC6, 0xB6, 0xA6, 0x96, 0x86, 0x76, 0x66, 0x56, 0x46, 0x36, 0x26, 0x16, 0x06,
    0xF5, 0xE5, 0xD5, 0xC5, 0xB5, 0xA5, 0x95, 0x85, 0x75, 0x65, 0x55, 0x45, 0x35, 0x25, 0x15, 0x05,
    0xF4, 0xE4, 0xD4, 0xC4, 0xB4, 0xA4, 0x94, 0x84, 0x74, 0x64, 0x54, 0x44, 0x34, 0x24, 0x14, 0x04,
    0xF3, 0xE3, 0xD3, 0xC3, 0xB3, 0xA3, 0x93, 0x83, 0x73, 0x63, 0x53, 0x43, 0x33, 0x23, 0x13, 0x03,
    0xF2, 0xE2, 0xD2, 0xC2, 0xB2, 0xA2, 0x92, 0x82, 0x72, 0x62, 0x52, 0x42, 0x32, 0x22, 0x12, 0x02,
    0xF1, 0xE1, 0xD1, 0xC1, 0xB1, 0xA1, 0x91, 0x81, 0x71, 0x61, 0x51, 0x41, 0x31, 0x21, 0x11, 0x01,
    0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00,
};

#define bs_4bpp_nybble_swap_table(x)  (((x & 0xf) << 4) | ((x & 0xf0) >> 4))
 static orientation_t broadsheet_get_display_orientation(void);

int lut=0;
static bool broadsheet_x3_upadte_display_all(fx_type update_mode, struct fb_info *info)
{	
	unsigned short value = 0;
	int i;
	__u16 colorsample_pixel_3,colorsample_pixel_2,colorsample_pixel_1,colorsample_pixel_0;
	__u16 colorsample_pixel_7,colorsample_pixel_6,colorsample_pixel_5,colorsample_pixel_4;
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
  //baseaddr = ioremap(0x10000000, 4);
	//u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);
	u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base);		// Henry: 0921 Simplify for Diag
		
      int ratio=1;
    struct einkfb_info einkinfo;
    einkfb_get_info(&einkinfo);
	  
        switch ( einkinfo.bpp )
        {
            case EINKFB_4BPP:
                ratio = 2;
		fine_update = 1;
            break;
            
            case EINKFB_2BPP:
               ratio = 4;
		fine_update = 0;
            break;
        }

#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
			//Buffer 0
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0146h]
			
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]
			EPD_BUFFER_INDEX = 0;
#endif				
			//epd_send_command(0x0B,(0x2 << 4));	
			
  		//INIT_ROTMODE[0140h] Rotation 90
			//epd_send_command(0x0B,(0x1 << 8));	
      		       epd_send_command(0x0B,(0x1 << 8)|(0x2 << 4));

			epd_send_command(0x20,(0x2 << 4));	
			udelay(1);
			
			//Host Memory Access Port Register[0154h]
			epd_send_command(0x11,(0x154));	

			int j;
			int left,top,left_width,top_height;

			left=top=0;
			left_width=XRES;
			top_height=YRES;

			for(j=left;j<left_width;j++) 
			{ 
				for(i=top;i<top_height/ratio;i+=4) 
				{
					int addr = (j)*(YRES/ratio)+i;
					//colorsample_pixel_0 = bs_4bpp_nybble_swap_table_inverted[epd_base[addr++] ];
					//colorsample_pixel_2 = bs_4bpp_nybble_swap_table_inverted[epd_base[addr++] ];
					//colorsample_pixel_4 = bs_4bpp_nybble_swap_table_inverted[epd_base[addr++] ];
					//colorsample_pixel_6 = bs_4bpp_nybble_swap_table_inverted[epd_base[addr++] ];

					// DmitryZ
					colorsample_pixel_0 = bs_4bpp_nybble_swap_table(epd_base[addr++]);
					colorsample_pixel_2 = bs_4bpp_nybble_swap_table(epd_base[addr++]);
					colorsample_pixel_4 = bs_4bpp_nybble_swap_table(epd_base[addr++]);
					colorsample_pixel_6 = bs_4bpp_nybble_swap_table(epd_base[addr++]);

					//colorsample_pixel_0 = 0xff - epd_base[addr++] ;
					//colorsample_pixel_2 = 0xff - epd_base[addr++] ;
					//colorsample_pixel_4 = 0xff - epd_base[addr++] ;
					//colorsample_pixel_6 = 0xff - epd_base[addr++] ;
																	 						
				__raw_writel((uint32_t)((colorsample_pixel_6<<24)|
							(colorsample_pixel_4<<16)|
							(colorsample_pixel_2<<8)|
							(colorsample_pixel_0<<0)),
							baseaddr);
 				} 
			
			}
			//LD_IMG_END
			epd_write_command(0x23);
  		
			//UPD_FULL[0334h]
			if(update_mode==fx_update_full)
			{
				value = (0x4200);
				epd_send_command(0x33,value);	
			}
			else if(update_mode==fx_update_partial)
			{	
				value = (0x42f0);
				epd_send_command(0x35,value);
			}
  		
			//WAIT_DSPE_TRG
			epd_write_command(0x28);
  		  
  		//WAIT_DSPE_FREND
  		epd_write_command(0x29);  
  		
#ifdef X3_EPD_POWER
  		EINKFB_LOCK_EXIT();
#endif
  		
	}
	return 0;
}

extern void LIST_REG(void);
extern void bs_only_load_img_data(u8 *data, bool upd_area, u16 x, u16 y, u16 w, u16 h);
static bool broadsheet_x3_upadte_display_area(update_area_t *update_area, struct fb_info *info)
{
	
	unsigned short value = 0;
	int i,j;
	__u16 colorsample_pixel_3,colorsample_pixel_2,colorsample_pixel_1,colorsample_pixel_0;
	__u16 colorsample_pixel_7,colorsample_pixel_6,colorsample_pixel_5,colorsample_pixel_4;
	__u16 colorsample_pixel[8];
	int counter=0;
	int mod_start_y,mod_end_y;	
  fx_type     waveform_update_type;
	int left,top,left_width,top_height;
	int mod_start_x,mod_end_x;
	int     	which_fx,update_mode;
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
  //baseaddr = ioremap(0x10000000, 4);	 
	//u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);
	u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base);		// Henry: 0921 Simplify for Diag
    struct einkfb_info einkinfo;
    einkfb_get_info(&einkinfo);
      int ratio=1;
        switch ( einkinfo.bpp )
        {
            case EINKFB_4BPP:
                ratio = 2;
		fine_update = 1;
            break;
            
            case EINKFB_2BPP:
               ratio = 4;
		fine_update = 0;
            break;
        }
	
	mod_start_x  = update_area->x1 % 4;//4;
	mod_end_x 	 = update_area->x2 % 4;//4;
	
	left 			 = update_area->x1-(mod_start_x);
	mod_start_y  = update_area->y1 % 4;//4;
	mod_end_y 	 = update_area->y2 % 4;//4;	
	top  			 = update_area->y1-(mod_start_y);
	if  ((update_area->y2==YRES) || (mod_end_y == 0))
		top_height = update_area->y2;
	else
		top_height = update_area->y2+(4-mod_end_y);
	
	if ((update_area->x2==XRES) || (mod_end_x == 0))
		left_width = update_area->x2;
	else
		left_width = update_area->x2+(4-mod_end_x);
	
  which_fx = update_area->which_fx & 0x1F;
  update_mode = update_area->which_fx & 0x20;
  
	switch(which_fx)
	{
      case UPDATE_GU://GC4
        waveform_update_type = 0x3;
        break;
        
      case UPDATE_GC://GC16
        waveform_update_type = 0x2;
        break;
        
      case UPDATE_DU:
        waveform_update_type = 0x1;
        break;
        
      default:
        waveform_update_type = 0x2;
        break;
	}
	

#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
			//Buffer 0
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0146h]
			
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]
			EPD_BUFFER_INDEX = 0;
#endif									

  		//INIT_ROTMODE[0140h] Rotation 90
			value = (0x1 << 8)|(0x2 << 4);
			epd_send_command(0x0B,value);	
    	
			//LD_IMG_AREA (0x22 + 5 parameters)
			epd_write_command(0x22);
			epd_write_data((0x2<<4));								//para1 			 REG[0140h]
			epd_write_data(left);										//para2 Xstart REG[014Ch]
			epd_write_data(top);										//para3 Ystart REG[014Eh]
			epd_write_data(left_width-left);				//para4 Width  REG[0150h]
			epd_write_data(top_height-top);					//para5 Height REG[0152h]
	  	
			udelay(1);
	  	
			//Host Memory Access Port Register[0154h]
			epd_send_command(0x11,(0x154));	
			for(j=top;j<top_height;j++) 
			{ 
			counter = 0;			
				for(i=left;i<left_width;i+=2) 
				{
					int addr = (j)*(XRES/ratio)+i/ratio;
						colorsample_pixel[counter] = bs_4bpp_nybble_swap_table(epd_base[addr++]);
						counter++;
						if ( counter == 2 )	
							{
							counter=0;
					__raw_writew((uint32_t) ((colorsample_pixel[1]<<8) | 	(colorsample_pixel[0]<<0)),baseaddr);	

							}
				} 
			}	   	
	
			//LD_IMG_END
			epd_write_command(0x23);

			if(update_mode == 0x20/*fx_update_full*/)
			{
					//UPD_FULL_AREA	XStart,YStart,Width,Height
					epd_write_command(0x34);
					epd_write_data(0x0006|(waveform_update_type<<8));//para1 REG[0334h]
					epd_write_data(left);									//para2 Xstart REG[0340h]
					epd_write_data(top);									//para3 Ystart REG[0342h]
					epd_write_data(left_width-left);						//para4 X-End  REG[0344h]
					epd_write_data(top_height-top);						//para5 Y-End  REG[0346h]
			}
			else
			{
					//UPD_PART_AREA	XStart,YStart,Width,Height
					epd_write_command(0x36);
					epd_write_data(0x0008|(waveform_update_type<<8));//para1 REG[0334h]
					epd_write_data(left);									//para2 Xstart REG[0340h]
					epd_write_data(top);									//para3 Ystart REG[0342h]
					epd_write_data(left_width-left);			//para4 X-End  REG[0344h]
					epd_write_data(top_height-top);				//para5 Y-End  REG[0346h]
			}
								
			//WAIT_DSPE_TRG
			epd_write_command(0x28);
			
  		//WAIT_DSPE_FREND
  		epd_write_command(0x29);  

              //udelay(1000);  
#ifdef X3_EPD_POWER
  		EINKFB_LOCK_EXIT();
#endif
	}
				
	return 0;
}

/*Henry:   Transfer image data to EPD controller*/
extern unsigned long get_chksum(void);
bool broadsheet_ep3_update_display_image(update_area_t *update_area, struct fb_info *info)
{
	unsigned short value = 0;
	int i,j;
	int counter=0;
	int tempcounter = 0;
	int mod_start_y,mod_end_y;	
       fx_type     waveform_update_type;
	int left,top,left_width,top_height;
	int mod_start_x,mod_end_x;
	int     	which_fx,update_mode;
	unsigned short __iomem* baseaddr;
	baseaddr = bsaddr;
       //baseaddr = ioremap(0x10000000, 4);	 
	u8 __iomem *epd_base = (u8 __iomem *)(info->screen_base+info->var.yoffset*XRES/2);

	orientation_t local_orientation= broadsheet_get_display_orientation();
	const unsigned char orientation_trans_controller_register_table[4] = {1, 3, 0, 2};
		// orientation_portrait (0)  => 1,  orientation_portrait_upside_down (1) => 3
		// orientation_landscape (2) =>  0,  orientation_landscape_upside_down (3)  => 2 
		/*Henry: EPD Controller: 00b   0¡ã; 01b   90¡ã; 10b   180¡ã; 11b   270¡ã*/
	local_orientation = orientation_trans_controller_register_table[local_orientation];

	global_moment = jiffies;
	global_display_stage=1;
      struct einkfb_info einkinfo;
      einkfb_get_info(&einkinfo);
      int ratio=1;
printk(":::::::::::::::: update_image: bpp=%d\n", einkinfo.bpp);
        switch ( einkinfo.bpp )
        {
            case EINKFB_4BPP:
                ratio = 2;
		fine_update = 1;
            break;
            
            case EINKFB_2BPP:
                ratio = 4;
		fine_update = 0;
            break;

            case EINKFB_1BPP:
                ratio = 8;
		fine_update = 0;
            break;
        }

	mod_start_x  = update_area->x1 % 4;//4;
	mod_end_x 	 = update_area->x2 % 4;//4;
	
	left 			 = update_area->x1-(mod_start_x);
	mod_start_y  = update_area->y1 % 4;//4;
	mod_end_y 	 = update_area->y2 % 4;//4;	
	top  			 = update_area->y1-(mod_start_y);
	if  ((update_area->y2==YRES) || (mod_end_y == 0))
		top_height = update_area->y2;
	else
		top_height = update_area->y2+(4-mod_end_y);
	
	if ((update_area->x2==XRES) || (mod_end_x == 0))
		left_width = update_area->x2;
	else
		left_width = update_area->x2+(4-mod_end_x);


  which_fx = update_area->which_fx & 0x1F;
  update_mode = update_area->which_fx & 0x20;
  
	switch(which_fx)
	{
      case UPDATE_GU://GC4
        waveform_update_type = 0x3;
        break;
        
      case UPDATE_GC://GC16
        waveform_update_type = 0x2;
        break;
        
      case UPDATE_DU:
        waveform_update_type = 0x1;
        break;
        
      default:
        waveform_update_type = 0x2;
        break;
	}

	global_left = left;
	global_top = top;
	global_left_width = left_width;
	global_top_height = top_height;
     static unsigned long tempfb_size=0;

// DmitryZ - moved directly to bs_only_load_img_data
//     tempfb_size = BPP_SIZE(((left_width-left)*(top_height-top)), einkinfo.bpp);
//     unsigned long *ptr = update_area->buffer;
//     unsigned long v;
//     for (i=0; i < tempfb_size; i+=4)
//    	{
//		v = *ptr;
//		*(ptr++) = ((v << 4) & 0xf0f0f0f0) | ((v >> 4) & 0x0f0f0f0f);
//    	}

// DmitryZ - commented out
// if  ( get_chksum() != chksum)
// 	return;

	broadsheet_check_overlap();

	
#ifdef X3_EPD_POWER
	if ( (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
#endif
	{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
			//Buffer 0
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0146h]
			
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]
			EPD_BUFFER_INDEX = 0;
#endif

			u16 rd_reg;
		       rd_reg =  0x84;
			bs_cmd_wr_reg(0x0330, rd_reg);	
			

  		//INIT_ROTMODE[0140h] Rotation orientation angle
  		
			value = (local_orientation << 8)|(0x2 << 4);
			epd_send_command(0x0B,value);	
			
    	
			//LD_IMG_AREA (0x22 + 5 parameters)
			epd_write_command(0x22);
			epd_write_data((0x2<<4));								//para1 			 REG[0140h]
			epd_write_data(left);										//para2 Xstart REG[014Ch]
			epd_write_data(top);										//para3 Ystart REG[014Eh]
			epd_write_data(left_width-left);				//para4 Width  REG[0150h]
			epd_write_data(top_height-top);					//para5 Height REG[0152h]

			global_left = left;
			global_top = top;
			global_left_width = left_width;
			global_top_height = top_height;
	  	
			udelay(1);
	  	

			//Host Memory Access Port Register[0154h]
			epd_send_command(0x11,(0x154));	

	
/*
     static unsigned long tempfb_size=0;

     __u8 tempchar;
     tempfb_size = BPP_SIZE(((left_width-left)*(top_height-top)), einkinfo.bpp);
     for (i=0; i < tempfb_size; i++)
    	{
    	tempchar = update_area->buffer[i];
	update_area->buffer[i] = bs_4bpp_nybble_swap_table(tempchar);
    	}
*/
    	
			bs_only_load_img_data(update_area->buffer, true, left, top, left_width - left, top_height - top);


			//LD_IMG_END
			epd_write_command(0x23);
			global_moment = jiffies;			
 		
#ifdef X3_EPD_POWER
  		EINKFB_LOCK_EXIT();
#endif
	}			
	return 0;

}
EXPORT_SYMBOL_GPL(broadsheet_ep3_update_display_image);


static int check_area_overlap(int ax1, int ax2, int ay1, int ay2, int bx1, int bx2, int by1, int by2) {

	int ovx = ((bx1 <= ax1 && bx2 >= ax1) || (ax1 <= bx1 && ax2 >= bx1));
	int ovy = ((by1 <= ay1 && by2 >= ay1) || (ay1 <= by1 && ay2 >= by1));
	return (ovx && ovy);

}


/*Henry: Update whole display using Grayscale Clearing waveform */
static bool broadsheet_ep3_show_display_image(int para_update_mode)
{
	int     value, update_mode;

	global_moment = jiffies;
	// DmitryZ - wait DSPE FREND
	//broadsheet_update_complete_verify();
	epd_write_command(0x29);  
	update_mode = para_update_mode & 0x20;
	//UPD_FULL[0334h]
printk(":::::::::::::::: show_image: fine_update=%d\n", fine_update);
	if(update_mode == 0x20/*fx_update_full*/)
	{
		value = (fine_update ? 0x4200 : 0x4300) | (get_recycle_lut_number() << 4);
		epd_send_command(0x33,value);	
	}
	else
	{	
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
		value = (fine_update ? 0x4200 : 0x4300) | (get_recycle_lut_number() << 4);
		epd_send_command(0x35,value);
#else	
		value = (fine_update ? 0x42f0 : 0x43f0) ; //Henry: Check 0x4200 latter ??? Display Update LUT Select bits 3-0 = 0???
		epd_send_command(0x35,value);
#endif		
	}
  		
	//WAIT_DSPE_TRG
	epd_write_command(0x28);
  		  
//#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
  	//DmitryZ - no WAIT_DSPE_FREND here
	//epd_write_command(0x29);  
	broadsheet_wait_dspe_frend=false;
	data_update_counter = 0;
	global_moment = jiffies;
//#else
//	broadsheet_wait_dspe_frend=true;
//	schedule_delayed_work(&broadsheet_power_mode->bs_delay_work, bs_dspe_delaywork_time);
//#endif

	last_update_full = 1;

}
bool get_resume_after_suspend(void);
static bool broadsheet_ep3_show_display_area_image(int para_update_mode)
{
	int     which_fx, update_mode;
       fx_type     waveform_update_type;
       int updatepixel= (global_left_width-global_left) * (global_top_height-global_top) / XRES;
	global_moment = jiffies;
	// DmitryZ - commented out
	//broadsheet_update_complete_verify();


	if (einkfb_get_dynamic_parameter() == 0) {
		if (last_update_full == 1 || check_area_overlap(global_left, global_left_width,
								global_top, global_top_height,
								last_update_x1, last_update_x2,
								last_update_y1, last_update_y2))
		{
			//WAIT_DSPE_FREND
			epd_write_command(0x29);
		}
	}

	which_fx = para_update_mode & 0x1F;
	update_mode = para_update_mode & 0x20;
       int lut_number = get_recycle_lut_number();

printk(":::::::::::::::: show_image_area: fine_update=%d\n", fine_update);
	switch(which_fx)
	{
      case UPDATE_GU://GC4
        waveform_update_type = 0x43;
        break;
        
      case UPDATE_GC://GC16
        waveform_update_type = (fine_update ? 0x42 : 0x43);
        break;
        
      case UPDATE_DU:
        waveform_update_type = 0x41;
        break;
        
      default:
        waveform_update_type = 0x42;
        break;
	}

	if (data_update_counter > 0)
       	previous_mode[data_update_counter-1] = which_fx;
	else
		previous_mode[0] = which_fx;
	if(update_mode == 0x20/*fx_update_full*/)
	{
		//UPD_FULL_AREA	XStart,YStart,Width,Height
		epd_write_command(0x34);
		epd_write_data( (lut_number << 4) |(waveform_update_type<<8));//para1 REG[0334h]
		epd_write_data(global_left);									//para2 Xstart REG[0340h]
		epd_write_data(global_top);									//para3 Ystart REG[0342h]
		epd_write_data(global_left_width-global_left);						//para4 X-End  REG[0344h]
		epd_write_data(global_top_height-global_top);						//para5 Y-End  REG[0346h]
	}
	else
	{
		//UPD_PART_AREA	XStart,YStart,Width,Height
		epd_write_command(0x36);
		epd_write_data( (lut_number << 4)|(waveform_update_type<<8));//para1 REG[0334h]
		epd_write_data(global_left);							//para2 Xstart REG[0340h]
		epd_write_data(global_top);									//para3 Ystart REG[0342h]
		epd_write_data(global_left_width-global_left);			//para4 X-End  REG[0344h]
		epd_write_data(global_top_height-global_top);				//para5 Y-End  REG[0346h]
	}
	
	//WAIT_DSPE_TRG
	epd_write_command(0x28);
       if  ( einkfb_get_dynamic_parameter() == 0 )
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT) 
	if ( (global_left_width-global_left) * (global_top_height-global_top) >= (BS97_INIT_HSIZE * BS97_INIT_VSIZE) /100 )
#elif  defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
	if ( (global_left_width-global_left) * (global_top_height-global_top) >= (BS60_INIT_HSIZE * BS60_INIT_VSIZE) /100 )
#endif
		{
		// DmitryZ - no wait here
	  	//WAIT_DSPE_FREND
		//epd_write_command(0x29);  
		//broadsheet_wait_dspe_frend=false;
		//data_update_counter = 0;
		}
		global_moment = jiffies;		
			
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)			
	//EVT ONLY - WAIT_DSPE_FREND
	epd_write_command(0x29);  
	broadsheet_wait_dspe_frend=false;	
	data_update_counter = 0;
#else
//1004
//DmitryZ - commented out
/*
       if  ( einkfb_get_dynamic_parameter() == 0 )	
      	{
          if (updatepixel <= 1)
		broadsheet_start_update_timer((HZ/10)*4);
         else if (updatepixel <= 100)
		broadsheet_start_update_timer((HZ/10)*4);
         else if ((updatepixel <= 600) && (updatepixel > 100))
		broadsheet_start_update_timer((HZ/10)*5);	
         else if ((updatepixel <= 900) && (updatepixel > 600))
		broadsheet_start_update_timer((HZ/10)*4);	
	  else
		{
		broadsheet_start_update_timer((HZ/10)*4);			
//		
//		if (waveform_update_type == 0x43)	//UPDATE_GU fx_update_full
//			broadsheet_start_update_timer((HZ/10)*2);	
//		if (waveform_update_type == 0x42) 	//UPDATE_GC
//			broadsheet_start_update_timer(HZ+(HZ/10)*2);	
//			
		}
	}	
*/
#endif
	global_display_stage=2;

	last_update_full = 0;
	last_update_x1 = global_left;
	last_update_x2 = global_left_width;
	last_update_y1 = global_top;
	last_update_y2 = global_top_height;

	return 0;

}

//EPD DU mode update function.    
static bool broadsheet_x3_upadte_display_area_DU(update_area_t *update_area, struct fb_info *info)
{	
	broadsheet_x3_upadte_display_area(update_area, info);			
	return 0;
}

//&*&*&*JJ1_1212ADD add EPD handwriting function.
extern void epd_memroywrite_draw(unsigned int x0, unsigned int y0, int pen);
void epd_memroywrite_draw(unsigned int x0, unsigned int y0, int pen)
{				
	
			int i;
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
			//Buffer 0
			//LD_IMG_SETADR	(0x25 + 2 parameters) 
			epd_write_command(0x25);
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX));			//para1 REG[0144h]
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX)>>16);	//para2 REG[0146h]
#endif			
  		//INIT_ROTMODE[0140h] Rotation 90
			epd_send_command_du(0x0B,(0x1 << 8)|(0x3 << 4));													
  		
  		//LD_IMG_AREA (0x22 + 5 parameters)
			epd_write_command_du(0x22);
			epd_write_data_du((0x3<<4));				//para1 			 REG[0140h]
			epd_write_data_du(x0);							//para2 Xstart REG[0340h]
			epd_write_data_du(y0);							//para3 Ystart REG[0342h]
			epd_write_data_du(pen);							//para4 X-End  REG[0344h]
			epd_write_data_du(pen);							//para5 Y-End  REG[0346h]
			
			//Host Memory Access Port Register[0154h]
			epd_send_command_du(0x11,0x154);		
			
			for(i=0;i<((pen*pen)/2);i++)
				epd_write_data_du(0x0000);	
			
			//LD_IMG_END
			epd_write_command_du(0x23);
			
}


extern void epd_memroywrite_update(unsigned int left, unsigned int left_width, unsigned int top, unsigned int top_height);
void epd_memroywrite_update(unsigned int left, unsigned int left_width, unsigned int top, unsigned int top_height)
{	
  
			//UPD_SET_IMGADR	(0x38 + 2 parameters) 
			epd_write_command(0x38);
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX));			//para1 REG[0310h]
			epd_write_data(ImageBufferaddr(EPD_BUFFER_INDEX)>>16);	//para2 REG[0312h]
															 
			//UPD_PART_AREA	XStart,YStart,Width,Height
			epd_write_command_du(0x36);
			epd_write_data_du(0x0006|(0x1<<8));					//para1 REG[0334h]
			epd_write_data_du(left);										//para2 Xstart REG[0340h]
			epd_write_data_du(top);											//para3 Ystart REG[0342h]
			epd_write_data_du(left_width-left);					//para4 X-End  REG[0344h]
			epd_write_data_du(top_height-top);					//para5 Y-End  REG[0346h]
								
}
//&*&*&*JJ2_1212ADD add EPD handwriting function.
//&*&*&*20100206_JJ1:epd_hangwrite_flag
static bool broadsheet_epd_hangwrite_flag(update_area_u16 *update_area, struct fb_info *info)
{
	int epd_hangwrite_flag_enable;  
			
  epd_hangwrite_flag_enable = update_area->which_fx & 0x1;	
  
//	//UPD_SET_IMGADR	(0x38 + 2 parameters) 
//	epd_write_command(0x38);
//	epd_write_data(ImageBufferaddr(0));			//para1 REG[0310h]
//	epd_write_data(ImageBufferaddr(0)>>16);	//para2 REG[0312h]

	return 0;
}
//&*&*&*20100206_JJ1:epd_hangwrite_flag
	
/* Henry Li:  Set image orientation Data[0]: 0=270¡ã, 1=0¡ã, 2=90¡ã, 3=180¡ã */
/*Henry: EPD Controller: 00b   0¡ã; 01b   90¡ã; 10b   180¡ã; 11b   270¡ã*/
/* Modify to support FB_ROTATE 0xF5  ioctl */
static bool broadsheet_set_display_orientation(orientation_t orientation)
{
    bool rotate = false;
    
    // If Broadsheet isn't still in its ready state when we're attempting
    // a rotation, tell the eInk HAL that we're in real trouble.
    //
    if ( !BS_STILL_READY() )
        EINKFB_NEEDS_RESET();
    else
    {
        switch ( orientation )
        {
            case orientation_portrait:
                if ( !IS_PORTRAIT() )
                {
                    bs_orientation = EINK_ORIENT_PORTRAIT;
                    bs_upside_down = false;
                    rotate = true;
                    bs_rotation_mode = 1;
                }
            break;
            
            case orientation_portrait_upside_down:
                if ( !IS_PORTRAIT_UPSIDE_DOWN() )
                {
                    bs_orientation = EINK_ORIENT_PORTRAIT;
                    bs_upside_down = true;
                    rotate = true;
		      bs_rotation_mode = 3;				
                }
            break;
            
            case orientation_landscape:
                if ( !IS_LANDSCAPE() )
                {
                    bs_orientation = EINK_ORIENT_LANDSCAPE;
                    bs_upside_down = false;
                    rotate = true;
		      bs_rotation_mode = 0;
                }
            break;
            
            case orientation_landscape_upside_down:
                if ( !IS_LANDSCAPE_UPSIDE_DOWN() )
                {
                    bs_orientation = EINK_ORIENT_LANDSCAPE;
                    bs_upside_down = true;
                    rotate = true;
		      bs_rotation_mode = 2;
                }
            break;
        }
        
/*	//1005
        if ( rotate )
            bs_sw_init_panel(DONT_BRINGUP_PANEL);
*/            
    }

    return ( rotate );
}

static orientation_t broadsheet_get_display_orientation(void)
{
    orientation_t orientation = orientation_portrait;
    bs_rotation_mode = 1;
		
    switch ( bs_orientation )
    {
        case EINK_ORIENT_PORTRAIT:
            if ( bs_upside_down )
            	{
                orientation = orientation_portrait_upside_down;
		  bs_rotation_mode = 3;				
            	}
        break;
        
        case EINK_ORIENT_LANDSCAPE:
            if ( bs_upside_down )
            	{
                orientation = orientation_landscape_upside_down;
		  bs_rotation_mode = 2;				
            	}
            else
            	{
                orientation = orientation_landscape;
		  bs_rotation_mode = 0;				
            	}
        break;
    }
    
    return ( orientation );
}

unsigned char  broadsheet_get_rotation_mode(void)
{
	return bs_rotation_mode;
}	
EXPORT_SYMBOL_GPL(broadsheet_get_rotation_mode);
/* Henry Li: read flash data in specific addres from beginning */
int bs_ep3_readfromflash(unsigned long addr, unsigned char *data)
{
	int result=0;
        bs_flash_select saved_flash_select = broadsheet_get_flash_select();
        
        broadsheet_set_flash_select(bs_flash_commands);

        // Read data from flash.
        //
        result = broadsheet_read_from_flash_byte(addr, data);
        
        // Restore whatever area of flash was previously set.
        //
        broadsheet_set_flash_select(saved_flash_select);
	 return result;
}

int  bs_ep3_writetoflash(unsigned long start_addr, unsigned char *buffer, unsigned long blen)
{
	int result=0;
	if ((start_addr != BS_CMD_ADDR) && (start_addr != BS_WFM_ADDR))
		return EINKFB_FAILURE;
       if (blen < 2048)
		return EINKFB_FAILURE;
       bs_flash_select saved_flash_select = broadsheet_get_flash_select();

	if (start_addr == BS_CMD_ADDR)  
	        broadsheet_set_flash_select(bs_flash_commands);
	else if (start_addr == BS_WFM_ADDR)
		{
	        broadsheet_set_flash_select(bs_flash_waveform);
		start_addr = 0;
		}
        result = broadsheet_program_flash(start_addr, buffer, blen);
		
        // Restore whatever area of flash was previously set.
        //
        broadsheet_set_flash_select(saved_flash_select);
	 return result;
}

extern void bs_ep3_pen_calibrate(u16 HSize, u16 VSize, u16 xyswap);
//extern void bs_ep3_penwrite_mode(bool mode, unsigned char drawdata[10] );

static einkfb_hal_ops_t broadsheet_hal_ops =
{
    .hal_sw_init                 = broadsheet_sw_init,
    .hal_sw_done                 = broadsheet_sw_done,
    
    .hal_hw_init                 = broadsheet_hw_init,
    .hal_hw_done                 = broadsheet_hw_done,
    
    .hal_create_proc_entries     = broadsheet_create_proc_entries,
    .hal_remove_proc_entries     = broadsheet_remove_proc_entries,
    
    .hal_update_display          = broadsheet_update_display,
    .hal_update_area             = broadsheet_update_area,
    
    .hal_set_power_level         = broadsheet_set_power_level,
    .hal_get_power_level         = broadsheet_get_power_level,

    .hal_reboot_hook             = broadsheet_reboot_hook,
    .hal_byte_alignment          = broadsheet_byte_alignment,
    
    .hal_set_display_orientation = broadsheet_set_display_orientation,
    .hal_get_display_orientation = broadsheet_get_display_orientation,
    
    .hal_pan_display			= broadsheet_pan_display,	
    .hal_x3_update_display_all	 = broadsheet_x3_upadte_display_all,	
    .hal_x3_update_display_area	 = broadsheet_x3_upadte_display_area,
    .hal_x3_update_display_area_du = broadsheet_x3_upadte_display_area_DU,
    .hal_x3_epd_hangwrite_flag	 = broadsheet_epd_hangwrite_flag,

    .hal_do_bs_mode_switch = do_bs_mode_switch,
    .hal_do_normal_inverted_switch = broadsheet_ep3_picture_mode_switch, 
    .hal_ep3_update_display_image = broadsheet_ep3_update_display_image,
    .hal_show_display_image = broadsheet_ep3_show_display_image,
    .hal_show_display_area_image = broadsheet_ep3_show_display_area_image,
    .hal_epd_fill_pixel = broadsheet_ep3_epd_fill_pixel,
    .hal_bs_read_temperature = bs_read_temperature,
    .hal_bs_readfromflash = bs_ep3_readfromflash,
    .hal_bs_writetoflash = bs_ep3_writetoflash,
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)    
    .hal_bs_pen_calibrate = bs_ep3_pen_calibrate,
    .hal_bs_penwrite_mode = bs_ep3_penwrite_mode
#endif    
};

static __INIT_CODE int broadsheet_init(void)
{
    // Preflight the Broadsheet controller.  Note that, if any of the
    // the preflight checks fail, the preflighting process will
    // automatically put us into bootstrapping mode.
    //
    
    /*--Start--, 20090205, Jimmy_Su@CDPG, fixed the reset twice problem*/
    if ( BS_BOOTSTRAPPED() || !FB_INITED() )	//Henry: OSC26M
    //if ( BS_BOOTSTRAPPED() /*|| !FB_INITED()*/ )
        broadsheet_preflight();
    /*--End--, 20090205, Jimmy_Su@CDPG, fixed the reset twice problem*/
    
    // Don't talk to the display, don't do power management, and don't
    // remember our state on reboot when we're being bootstrapped.
    //
    if ( BS_BOOTSTRAPPED() )
    {
        broadsheet_hal_ops.hal_update_display   = NULL;
        broadsheet_hal_ops.hal_update_area      = NULL;
        
        broadsheet_hal_ops.hal_set_power_level  = NULL;
        broadsheet_hal_ops.hal_get_power_level  = NULL;
        
        broadsheet_hal_ops.hal_reboot_hook      = NULL;
    }

    return ( einkfb_hal_ops_init(&broadsheet_hal_ops) );
}

module_init(broadsheet_init);

#ifdef MODULE
static void broadsheet_exit(void)
{
    einkfb_hal_ops_done();
}

module_exit(broadsheet_exit);

MODULE_AUTHOR("Henry Li");
MODULE_LICENSE("GPL");
#endif // MODULE
