/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_hal.c
 *  -- eInk frame buffer device HAL broadsheet
 *
 *      Copyright (C) 2005-2009 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "broadsheet.h"

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

// Default to Turing's resolution:  600x800@4bpp.
//
#define XRES                            600
#define YRES                            800
#define BPP                             EINKFB_4BPP

#define BROADSHEET_SIZE                 BPP_SIZE((XRES*YRES), BPP)
#define SCRATCHFB_SIZE                  BPP_SIZE((XRES*YRES), EINKFB_4BPP)

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
static int broadsheet_num_ram_banks     = BROADSHEET_NUM_RAM_BANKS;

static unsigned long broadsheet_size    = BROADSHEET_SIZE;
static unsigned long scratchfb_size     = SCRATCHFB_SIZE;
static unsigned long bs_bpp             = BPP;
static int broadsheet_flash_size        = BROADSHEET_FLASH_SIZE;
static int bs_orientation               = EINKFB_ORIENT_PORTRAIT;
static int bs_upside_down               = false;

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
    if ( 0 == off )
    {
        if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
        {
            broadsheet_read_from_ram(bs_cmd_get_sdr_img_base(), scratchfb, scratchfb_size);
            EINKFB_LOCK_EXIT();
        }
    }
	
	return ( EINKFB_PROC_SYSFS_RW_NO_LOCK(page, start, off, count, eof, scratchfb_size, read_hardwarefb) );
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
	return ( EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_reg) );
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
	return ( sprintf(buf, "%d\n", broadsheet_get_flash_select()) );
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
	return ( sprintf(buf, "%d\n", broadsheet_get_ram_select()) );
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
            bs_sw_init(FULL_BRINGUP_CONTROLLER, DONT_BRINGUP_PANEL);
            bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
            EINKFB_LOCK_EXIT();
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
    if ( !eink_ram_select_locked && IN_RANGE(ram_select, 0, (broadsheet_num_ram_banks-1)) )
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

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL module operations
    #pragma mark -
#endif

static bool broadsheet_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    u32 xres_hw, xres_sw, yres_hw, yres_sw;
    bool result = EINKFB_FAILURE;
    
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
    bs_sw_init_controller(FULL_BRINGUP_CONTROLLER, bs_orientation, &xres_hw, &xres_sw, &yres_hw, &yres_sw);
    scratchfb_size = BPP_SIZE((xres_hw*yres_hw), EINKFB_8BPP);
    
    broadsheet_num_ram_banks = broadsheet_get_ram_size()/BROADSHEET_RAM_BANK_SIZE;
    
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
                broadsheet_size                 = BPP_SIZE((xres_sw*yres_sw), bs_bpp);

                broadsheet_var.bits_per_pixel   = bs_bpp;
                
                broadsheet_fix.smem_len         = broadsheet_size;
                broadsheet_fix.line_length      = BPP_SIZE(xres_sw, bs_bpp);
            break;
        }
        
        broadsheet_var.xres = broadsheet_var.xres_virtual = xres_sw;
        broadsheet_var.yres = broadsheet_var.yres_virtual = yres_sw;

        *var = broadsheet_var;
        *fix = broadsheet_fix;
        
        result = EINKFB_SUCCESS;
    }

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
    bs_sw_done(DEALLOC);
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
        
    einkfb_proc_eink_ram = einkfb_create_proc_entry(EINKFB_PROC_EINK_RAM, EINKFB_PROC_CHILD_RW,
        eink_ram_read, eink_ram_write);
        
    einkfb_proc_eink_reg = einkfb_create_proc_entry(EINKFB_PROC_EINK_REG, EINKFB_PROC_CHILD_W,
        NULL, eink_reg_write);

    if ( broadsheet_supports_flash() )
        einkfb_proc_eink_rom = einkfb_create_proc_entry(EINKFB_PROC_EINK_ROM, EINKFB_PROC_CHILD_RW,
            eink_rom_read, eink_rom_write);
    
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
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_RAM, einkfb_proc_eink_ram);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_REG, einkfb_proc_eink_reg);

    if ( broadsheet_supports_flash() )
        einkfb_remove_proc_entry(EINKFB_PROC_EINK_ROM, einkfb_proc_eink_rom);
    
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
}

static void broadsheet_set_power_level(einkfb_power_level power_level)
{
    int power_state;
    
    switch ( power_level )
    {
        case einkfb_power_level_on:
            power_state = bs_power_state_run;
        goto set_power_state;
        
        case einkfb_power_level_standby:
            power_state = bs_power_state_sleep;
        goto set_power_state;

        case einkfb_power_level_sleep:
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
            broadsheet_blank_screen();
            broadsheet_set_power_state(bs_power_state_off_screen_clear);
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
            power_level = einkfb_power_level_on;
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

static bool broadsheet_set_display_orientation(orientation_t orientation)
{
    bool rotate = false;
    
    switch ( orientation )
    {
        case orientation_portrait:
            if ( !IS_PORTRAIT() )
            {
                bs_orientation = EINK_ORIENT_PORTRAIT;
                bs_upside_down = false;
                rotate = true;
            }
        break;
        
        case orientation_portrait_upside_down:
            if ( !IS_PORTRAIT_UPSIDE_DOWN() )
            {
                bs_orientation = EINK_ORIENT_PORTRAIT;
                bs_upside_down = true;
                rotate = true;
            }
        break;
        
        case orientation_landscape:
            if ( !IS_LANDSCAPE() )
            {
                bs_orientation = EINK_ORIENT_LANDSCAPE;
                bs_upside_down = false;
                rotate = true;
            }
        break;
        
        case orientation_landscape_upside_down:
            if ( !IS_LANDSCAPE_UPSIDE_DOWN() )
            {
                bs_orientation = EINK_ORIENT_LANDSCAPE;
                bs_upside_down = true;
                rotate = true;
            }
        break;
    }
    
    if ( rotate )
        bs_sw_init_panel(DONT_BRINGUP_PANEL);
    
    return ( rotate );
}

static orientation_t broadsheet_get_display_orientation(void)
{
    orientation_t orientation = orientation_portrait;
    
    switch ( bs_orientation )
    {
        case EINK_ORIENT_PORTRAIT:
            if ( bs_upside_down )
                orientation = orientation_portrait_upside_down;
        break;
        
        case EINK_ORIENT_LANDSCAPE:
            if ( bs_upside_down )
                orientation = orientation_landscape_upside_down;
            else
                orientation = orientation_landscape;
        break;
    }
    
    return ( orientation );
}

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
    .hal_get_display_orientation = broadsheet_get_display_orientation
};

static __INIT_CODE int broadsheet_init(void)
{
    // Preflight the Broadsheet controller.  Note that, if any of the
    // the preflight checks fail, the preflighting process will
    // automatically put us into bootstrapping mode.
    //
    if ( BS_BOOTSTRAPPED() || !FB_INITED() )
        broadsheet_preflight();
    
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

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif // MODULE
