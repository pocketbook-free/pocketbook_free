/*
 *  linux/drivers/video/eink/emulator/apollo_hal.c
 *  -- eInk frame buffer device HAL apollo
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "apollo.h"

#if PRAGMAS
    #pragma mark Definitions
#endif

#define APOLLO_FLASH_SIZE               0x80000
#define APOLLO_RAM_SIZE                 0x40000

#define EINKFB_PROC_WAVEFORM_VERSION    "waveform_version"
#define EINKFB_PROC_TEMPERATURE         "temperature"
#define EINKFB_PROC_EINK_ROM            "eink_rom"
#define EINKFB_PROC_EINK_RAM            "eink_ram"

// We default Apollo to its full resolution at its deepest:  600x800@2bpp.
//
#define XRES            APOLLO_X_RES
#define YRES            APOLLO_Y_RES
#define BPP             EINKFB_2BPP

#define APOLLO_SIZE     BPP_SIZE((XRES*YRES), BPP)

#define IS_PORTRAIT()                           \
    (APOLLO_ORIENTATION_270 == apollo_get_orientation())

#define IS_PORTRAIT_UPSIDE_DOWN()               \
    (APOLLO_ORIENTATION_90  == apollo_get_orientation())
    
#define IS_LANDSCAPE()                          \
    (APOLLO_ORIENTATION_180 == apollo_get_orientation())
    
#define IS_LANDSCAPE_UPSIDE_DOWN()              \
   (APOLLO_ORIENTATION_0    == apollo_get_orientation())

static struct fb_var_screeninfo apollo_var __INIT_DATA =
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

static struct fb_fix_screeninfo apollo_fix __INIT_DATA =
{
    .id                 = EINKFB_NAME,
    .smem_len           = APOLLO_SIZE,
    .type               = FB_TYPE_PACKED_PIXELS,
    .visual             = FB_VISUAL_STATIC_PSEUDOCOLOR,
    .xpanstep           = 0,
    .ypanstep           = 0,
    .ywrapstep          = 0,
    .line_length        = BPP_SIZE(XRES, BPP),
    .accel              = FB_ACCEL_NONE,
};

static struct proc_dir_entry *einkfb_proc_waveform_version = NULL;
static struct proc_dir_entry *einkfb_proc_temperature = NULL;
static struct proc_dir_entry *einkfb_proc_eink_rom = NULL;
static struct proc_dir_entry *einkfb_proc_eink_ram = NULL;

int g_eink_power_mode = 0;
int flash_fail_count = 0;

static unsigned long apollo_set_load_image_start;
static unsigned long apollo_load_image_start;
static unsigned long apollo_display_image_start;

static unsigned long apollo_image_start_time;
static unsigned long apollo_image_processing_time;
static unsigned long apollo_image_loading_time;
static unsigned long apollo_image_display_time;
static unsigned long apollo_image_stop_time;

#define APOLLO_IMAGE_TIMING_STRT   0
#define APOLLO_IMAGE_TIMING_PROC   1
#define APOLLO_IMAGE_TIMING_LOAD   2
#define APOLLO_IMAGE_TIMING_DISP   3
#define APOLLO_IMAGE_TIMING_STOP   4
#define APOLLO_NUM_IMAGE_TIMINGS    (APOLLO_IMAGE_TIMING_STOP + 1)
static unsigned long apollo_image_timings[APOLLO_NUM_IMAGE_TIMINGS];

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

// /proc/eink_fb/waveform_version (read-only)
//
static int read_waveform_version(char *page, unsigned long off, int count)
{
    return ( sprintf(page, "%s\n", apollo_get_waveform_version_string()) );
}

static int waveform_version_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_waveform_version) );
}

// /proc/eink_fb/temperature (read-only)
//
static int read_temperature(char *page, unsigned long off, int count)
{
    int result = 0;
    
    // We're done after one shot.
    //
    if ( 0 == off )
        result = sprintf(page, "%d\n", apollo_get_temperature());

    return ( result );
}

static int temperature_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, NULL, off, 0, eof, 0, read_temperature) );
}

// /proc/eink_fb/eink_rom (read/write)
//
static int read_from_flash(unsigned long addr, unsigned char *data, int count)
{
    int result = 0;
    
    if ( data && IN_RANGE((addr + count), 0, APOLLO_FLASH_SIZE) )
    {
        unsigned long i, j;
        
        for ( i = addr, j = 0; j < count; i++, j++ )
        {
            apollo_read_from_flash_byte(i, &data[j]);
            EINKFB_SCHEDULE_BLIT(j+1);
        }
            
        result = 1;
    }

    return ( result );
}

static int read_eink_rom(char *page, unsigned long off, int count)
{
    return ( einkfb_read_which(page, off, count, read_from_flash, APOLLO_FLASH_SIZE) );
}

static int eink_rom_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK(page, start, off, count, eof, APOLLO_FLASH_SIZE, read_eink_rom) );
}

static int write_eink_rom(char *buf, unsigned long count, int unused)
{
    return ( einkfb_write_which(buf, count, ProgramFlash, APOLLO_FLASH_SIZE) );
}

static int eink_rom_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
    return ( EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_rom) );
}

// /proc/eink_fb/eink_ram (read/write)
//
static int read_from_ram(unsigned long addr, unsigned char *data, int count)
{
    int result = 0;
    
    if ( data && IN_RANGE((addr + count), 0, APOLLO_RAM_SIZE) )
    {
        unsigned long i, j, n = count >> 2, *data_long = (unsigned long *)data;
        
        for ( i = addr, j = 0; j < n; i += 4, j++ )
        {
            ReadFromRam(i, &data_long[j]);
            EINKFB_SCHEDULE_BLIT(((j + 1) << 2));
        }
        
        result = 1;
    }

    return ( result );
}

static int read_eink_ram(char *page, unsigned long off, int count)
{
    return einkfb_read_which(page, off, count, read_from_ram, APOLLO_RAM_SIZE);
}

static int eink_ram_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return EINKFB_PROC_SYSFS_RW_LOCK(page, start, off, count, eof, APOLLO_RAM_SIZE, read_eink_ram);
}

static int write_eink_ram(char *buf, unsigned long count, int unused)
{
    return einkfb_write_which(buf, count, ProgramRam, APOLLO_RAM_SIZE);
}

static int eink_ram_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
    return EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_eink_ram);
}

// /sys/devices/platform/eink_fb0/flash_fail_count (read/write)
//
static ssize_t show_flash_fail_count(struct device *dev, char *buf)
{
    return ( sprintf(buf, "%d\n", flash_fail_count) );
}

static ssize_t store_flash_fail_count(struct device *dev, const char *buf, size_t count)
{
    int result = -EINVAL, fail_count;
    
    if ( sscanf(buf, "%d", &fail_count) )
    {
        flash_fail_count = fail_count;
        result = count;
    }
    
    return ( result );
}

// /sys/devices/platform/eink_fb0/off_delay (read/write)
//
static int read_off_delay(char *buf, unsigned long off, int unused)
{
    unsigned char off_delay = 0;
    
    apollo_read_register(APOLLO_OFF_DELAY_REGISTER, &off_delay);
    
    return ( sprintf(buf, "%d\n", off_delay) );
}

static ssize_t show_off_delay(struct device *dev, char *buf)
{
    return EINKFB_PROC_SYSFS_RW_LOCK(buf, NULL, 0, 0, NULL, 0, read_off_delay);
}

static int write_off_delay(char *buf, unsigned long count, int unused)
{
	int result = -EINVAL, off_delay;
	
	if ( sscanf(buf, "%d", &off_delay) )
	{
        off_delay = min(off_delay, APOLLO_OFF_DELAY_MAX);

        apollo_write_register(APOLLO_OFF_DELAY_REGISTER, off_delay);
        
        if ( APOLLO_OFF_DELAY_THRESH < off_delay )
            einkfb_set_blank_unblank_level(einkfb_power_level_on);
        else
            einkfb_set_blank_unblank_level(einkfb_power_level_standby);
        
        result = count;
    }

    return ( result );
}

static ssize_t store_off_delay(struct device *dev, const char *buf, size_t count)
{
    return EINKFB_PROC_SYSFS_RW_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_off_delay);
}

// /sys/devices/platform/eink_fb0/image_timings (read-only)
//
static unsigned long *apollo_get_image_timings(int *num_timings)
{
    unsigned long *timings = NULL;

    if ( num_timings )
    {
        apollo_image_timings[APOLLO_IMAGE_TIMING_STRT] = apollo_image_start_time;
        apollo_image_timings[APOLLO_IMAGE_TIMING_PROC] = apollo_image_processing_time;
        apollo_image_timings[APOLLO_IMAGE_TIMING_LOAD] = apollo_image_loading_time;
        apollo_image_timings[APOLLO_IMAGE_TIMING_DISP] = apollo_image_display_time;
        apollo_image_timings[APOLLO_IMAGE_TIMING_STOP] = apollo_image_stop_time;
        
        *num_timings = APOLLO_NUM_IMAGE_TIMINGS;
        timings = apollo_image_timings;
    }
    
    return ( timings );
}

static void apollo_set_image_timings(void)
{
    struct einkfb_info info;

    apollo_image_stop_time       = jiffies;
    einkfb_get_info(&info);
    
    apollo_image_start_time      = jiffies_to_msecs(apollo_set_load_image_start - info.jif_on);
    apollo_image_processing_time = jiffies_to_msecs(apollo_load_image_start     - apollo_set_load_image_start);
    apollo_image_loading_time    = jiffies_to_msecs(apollo_display_image_start  - apollo_load_image_start);
    apollo_image_display_time    = jiffies_to_msecs(apollo_image_stop_time      - apollo_display_image_start);
    apollo_image_stop_time       = jiffies_to_msecs(apollo_image_stop_time      - info.jif_on);
}

static ssize_t show_image_timings(FB_DSHOW_PARAMS)
{
	int i, len, num_timings;
	unsigned long *timings = apollo_get_image_timings(&num_timings);
	
	for ( i = 0, len = 0; i < num_timings; i++ )
	    len += sprintf(&buf[len], "%ld%s", timings[i], (i == (num_timings - 1)) ? "" : " " );
	    
	return ( len );
}

static void apollo_blank_screen(void)
{
    apollo_set_power_state(APOLLO_NORMAL_POWER_MODE);
    apollo_clear_screen(fx_update_full);
}

static void apollo_update_area_guts(unsigned char *buffer, int buffer_size, int xstart, int ystart, int xend, int yend)
{
    bool area_cleared;

    apollo_load_image_start = jiffies;

    area_cleared = (bool)apollo_load_partial_data(buffer, buffer_size, xstart, ystart, xend, yend) &&
                   (bool)get_screen_clear();
    
    if ( !area_cleared )
    {
        apollo_display_image_start = jiffies;
        apollo_display_partial();
    }
    else
        apollo_display_image_start = jiffies;
        
    set_screen_clear(area_cleared);
    apollo_set_image_timings();
}

#if PRAGMAS
    #pragma mark -
    #pragma mark HAL module operations.
    #pragma mark -
#endif

static bool apollo_sw_init(struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
    *var = apollo_var;
    *fix = apollo_fix;
    
    return ( EINKFB_SUCCESS );
}

static bool apollo_hw_init(struct fb_info *info, bool full)
{
    return ( (bool)apollo_init_kernel((int)(full || !FB_INITED())) );
}

static void apollo_hw_done(bool full)
{
    apollo_done_kernel((bool)full);
}

static void apollo_update_display(fx_type update_mode)
{
    bool screen_cleared;
    
    struct einkfb_info info;
    
    apollo_set_load_image_start = jiffies;
    einkfb_get_info(&info);

    apollo_load_image_start = jiffies;
    screen_cleared = (bool)apollo_load_image_data(info.start, info.size);
    
    if ( !screen_cleared || (screen_cleared && !get_screen_clear()) )
    {
        set_screen_clear(screen_cleared);
        
        apollo_display_image_start = jiffies;
        apollo_display(UPDATE_MODE(update_mode));
    }
    else
        apollo_display_image_start = jiffies;
    
    apollo_set_image_timings();
}

static DEVICE_ATTR(flash_fail_count,    DEVICE_MODE_RW, show_flash_fail_count,   store_flash_fail_count);
static DEVICE_ATTR(off_delay,		    DEVICE_MODE_RW, show_off_delay,          store_off_delay);
static DEVICE_ATTR(image_timings,       DEVICE_MODE_R,  show_image_timings,      NULL);

static void apollo_create_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);
    
    // Create Apollo-specific proc entries.
    //
    einkfb_proc_waveform_version = einkfb_create_proc_entry(EINKFB_PROC_WAVEFORM_VERSION, EINKFB_PROC_CHILD_R,
        waveform_version_read, NULL);
        
    einkfb_proc_temperature = einkfb_create_proc_entry(EINKFB_PROC_TEMPERATURE, EINKFB_PROC_CHILD_R,
        temperature_read, NULL);
    
    einkfb_proc_eink_rom = einkfb_create_proc_entry(EINKFB_PROC_EINK_ROM, EINKFB_PROC_CHILD_RW,
        eink_rom_read, eink_rom_write);
        
    einkfb_proc_eink_ram = einkfb_create_proc_entry(EINKFB_PROC_EINK_RAM, EINKFB_PROC_CHILD_RW,
        eink_ram_read, eink_ram_write);
        
    // Create Apollo-specific sysfs entries.
    //
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_flash_fail_count);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_off_delay);
    FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_image_timings);    
}

static void apollo_remove_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);
    
    // Remove Apollo-specific proc entries.
    //
    einkfb_remove_proc_entry(EINKFB_PROC_WAVEFORM_VERSION, einkfb_proc_waveform_version);
    einkfb_remove_proc_entry(EINKFB_PROC_TEMPERATURE, einkfb_proc_temperature);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_ROM, einkfb_proc_eink_rom);
    einkfb_remove_proc_entry(EINKFB_PROC_EINK_RAM, einkfb_proc_eink_ram);

    // Remove Apollo-specific sysfs entries.
    //
    device_remove_file(&info.dev->dev, &dev_attr_flash_fail_count);
    device_remove_file(&info.dev->dev, &dev_attr_off_delay);
	device_remove_file(&info.dev->dev, &dev_attr_image_timings);
}

static void apollo_update_area(update_area_t *update_area)
{
    int xstart = update_area->x1, xend = update_area->x2,
        ystart = update_area->y1, yend = update_area->y2,
        
        xres = xend - xstart,
        yres = yend - ystart,
        
        buffer_size;
        
    struct einkfb_info info;
        
    apollo_set_load_image_start = jiffies;
    einkfb_get_info(&info);

    buffer_size = BPP_SIZE((xres * yres), info.bpp);
    apollo_update_area_guts(update_area->buffer, buffer_size, xstart, ystart, xend, yend);
}

// Note:  In Apollo parlance, sleep is standby and standby is sleep.  It's very confusing.

static void apollo_set_power_level(einkfb_power_level power_level)
{
    unsigned char saved_init_display_speed = apollo_get_init_display_speed();
    int power_state = APOLLO_UNKNOWN_POWER_MODE;
    
    switch ( power_level )
    {
        // For internally generated wakes, we don't need to go through the full
        // init cycle.  So, we tell it to skip that process here.
        //
        case einkfb_power_level_on:
            apollo_set_init_display_speed(APOLLO_INIT_DISPLAY_SKIP);
            power_state = APOLLO_NORMAL_POWER_MODE;
        goto set_power_state;
        
        case einkfb_power_level_standby:
            power_state = APOLLO_SLEEP_POWER_MODE;
        goto set_power_state;

        case einkfb_power_level_sleep:
            power_state = APOLLO_STANDBY_POWER_MODE;
        /*goto set_power_state;*/

        set_power_state:
            apollo_set_power_state(power_state);
            
            // Restore the init-cycle setting if we changed it.
            //
            if ( apollo_get_init_display_speed() != saved_init_display_speed )
                apollo_set_init_display_speed(saved_init_display_speed);
        break;
        
        case einkfb_power_level_blank:
            apollo_blank_screen();
            apollo_set_power_state(APOLLO_SLEEP_POWER_MODE);
        break;
        
        case einkfb_power_level_off:
            apollo_blank_screen();
            apollo_set_power_state(APOLLO_STANDBY_OFF_POWER_MODE);
        break;

        // Prevent the compiler from complaining.
        //
        default:
        break;
    }
}

einkfb_power_level apollo_get_power_level(void)
{
    einkfb_power_level power_level = einkfb_power_level_init;
    
    switch ( apollo_get_power_state() )
    {
        case APOLLO_NORMAL_POWER_MODE:
            power_level = einkfb_power_level_on;
        break;
        
        case APOLLO_SLEEP_POWER_MODE:
            power_level = einkfb_power_level_standby;
        break;
        
        case APOLLO_STANDBY_POWER_MODE:
            power_level = einkfb_power_level_sleep;
        break;
    
        case APOLLO_STANDBY_OFF_POWER_MODE:
            power_level = einkfb_power_level_off;
        break;
    }
    
    return ( power_level );
}

static bool apollo_set_display_orientation(orientation_t orientation)
{
    u8 apollo_orientation;
    bool rotate = false;
    
    switch ( orientation )
    {
        case orientation_portrait:
            if ( !IS_PORTRAIT() )
            {
                apollo_orientation = APOLLO_ORIENTATION_270;
                rotate = true;
            }
        break;
        
        case orientation_portrait_upside_down:
            if ( !IS_PORTRAIT_UPSIDE_DOWN() )
            {
                apollo_orientation = APOLLO_ORIENTATION_90;
                rotate = true;
            }
        break;
        
        case orientation_landscape:
            if ( !IS_LANDSCAPE() )
            {
                apollo_orientation = APOLLO_ORIENTATION_180;
                rotate = true;
            }
        break;
        
        case orientation_landscape_upside_down:
            if ( !IS_LANDSCAPE_UPSIDE_DOWN() )
            {
                apollo_orientation = APOLLO_ORIENTATION_0;
                rotate = true;
            }
        break;
    }
    
    if ( rotate )
        apollo_set_orientation(apollo_orientation);
        
    return ( rotate );
}

static orientation_t apollo_get_display_orientation(void)
{
    u8 apollo_orientation = apollo_get_orientation();
    orientation_t orientation;
   
    switch ( apollo_orientation )
    {
        case APOLLO_ORIENTATION_270:
        default:
            orientation = orientation_portrait;
        break;
        
        case APOLLO_ORIENTATION_90:
            orientation = orientation_portrait_upside_down;
        break;
        
        case APOLLO_ORIENTATION_180:
            orientation = orientation_landscape;
        break;
        
        case APOLLO_ORIENTATION_0:
            orientation = orientation_landscape_upside_down;
        break;
    }
    
    return ( orientation );
}

static struct einkfb_hal_ops_t apollo_hal_ops =
{
    .hal_sw_init                 = apollo_sw_init,
    
    .hal_hw_init                 = apollo_hw_init,
    .hal_hw_done                 = apollo_hw_done,
    
    .hal_create_proc_entries     = apollo_create_proc_entries,
    .hal_remove_proc_entries     = apollo_remove_proc_entries,
    
    .hal_update_display          = apollo_update_display,
    .hal_update_area             = apollo_update_area,
    
    .hal_set_power_level         = apollo_set_power_level,
    .hal_get_power_level         = apollo_get_power_level,

    .hal_set_display_orientation = apollo_set_display_orientation,
    .hal_get_display_orientation = apollo_get_display_orientation
};

static __INIT_CODE int apollo_hal_init(void)
{
    return ( einkfb_hal_ops_init(&apollo_hal_ops) );
}

module_init(apollo_hal_init);

#ifdef MODULE
static void apollo_hal_exit(void)
{
    einkfb_hal_ops_done();
}

module_exit(apollo_hal_exit);

MODULE_AUTHOR("Lab126");
MODULE_LICENSE("GPL");
#endif // MODULE
