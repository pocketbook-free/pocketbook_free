/*
 *  linux/drivers/video/eink/hal/einkfb_hal_proc.c -- eInk frame buffer device HAL procfs/sysfs
 *
 *      Copyright (C) 2005-2008 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "einkfb_hal.h"

#if PRAGMAS
    #pragma mark Definitions/Globals
#endif

#define READ_SIZE   (1 << PAGE_SHIFT)
#define WRITE_SIZE  ((unsigned long)0x20000) 

#define EINKFB_PROC_UPDATE_DISPLAY  "update_display"
#define EINKFB_PROC_POWER_LEVEL     "power_level"
#define EINKFB_PROC_VIRTUALFB       "virtual_fb"

static struct proc_dir_entry *einkfb_proc_parent         = NULL;
static struct proc_dir_entry *einkfb_proc_update_display = NULL;
static struct proc_dir_entry *einkfb_proc_power_level    = NULL;
static struct proc_dir_entry *einkfb_proc_virtualfb      = NULL;

extern int einkfb_power_timer_delay;

unsigned long einkfb_logging = einkfb_logging_default;
bool einkfb_flash_mode = false;
bool einkfb_reset = false;

#ifdef MODULE
module_param_named(einkfb_logging, einkfb_logging, long, S_IRUGO);
MODULE_PARM_DESC(einkfb_logging, "non-zero to enable debugging");
#endif // MODULE

#if PRAGMAS
    #pragma mark -
    #pragma mark Proc/Sysfs Entries
    #pragma mark -
#endif

// For now, we're passing through all of the legacy eInk driver's ioctls.
// This lets the eInk shim capture and process them as appropriate.
//
static int update_display_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int m = 0, n[12] = { 0 };
	char lbuf[64] = { 0 };

	if (count > 64) {
		return -EINVAL;
	}

	if (copy_from_user(lbuf, buf, count)) {
        return -EFAULT;
	}
	
	m = sscanf(lbuf, "%d %d %d %d %d %d %d %d %d %d %d %d", &n[0], &n[1], &n[2], &n[3], &n[4], &n[5],
	            &n[6], &n[7], &n[8], &n[9], &n[10], &n[11]);

	if (m) {
		power_override_t power_override;
		progressbar_xy_t progressbar_xy;
		update_area_t update_area;
		fx_t fx = INIT_FX_T();

		unsigned long arg = 0;
		unsigned int cmd = 0;

		switch (n[0]) {
			case PROC_EINK_UPDATE_DISPLAY_CLS:
				cmd = FBIO_EINK_CLEAR_SCREEN;
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_PART:
				cmd = FBIO_EINK_UPDATE_DISPLAY;
				arg = fx_update_partial;
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_FULL:
				cmd = FBIO_EINK_UPDATE_DISPLAY;
				arg = fx_update_full;
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_AREA:
				if (m > 4) {
					update_area.x1 = n[1], update_area.y1 = n[2];
					update_area.x2 = n[3], update_area.y2 = n[4];
					update_area.buffer = NULL;
					
					if (m > 5) {
						update_area.which_fx = n[5];
					} else {
						update_area.which_fx = fx_none;
					}

					cmd = FBIO_EINK_UPDATE_DISPLAY_AREA;
					arg = (unsigned long)&update_area;
				}
				break;
			
			case PROC_EINK_UPDATE_DISPLAY_SCRN:
				if (m > 1) {
					cmd = FBIO_EINK_SPLASH_SCREEN;
					arg = n[1];
				}
				break;
				
		    case PROC_EINK_UPDATE_DISPLAY_SCRN_SLP:
				if (m > 1) {
					cmd = FBIO_EINK_SPLASH_SCREEN_SLEEP;
					arg = n[1];
				}
		    break;
				
			case PROC_EINK_UPDATE_DISPLAY_OVRD:
				if (m > 2) {
					power_override.cmd = n[1];
					
					if (m > 3) {
						power_override.arg = (unsigned long)&n[2];
					}
					else {
						power_override.arg = n[2];
					}
					
					cmd = FBIO_EINK_POWER_OVERRIDE;
					arg = (unsigned long)&power_override;
				}
				break;
				
			case PROC_EINK_UPDATE_DISPLAY_FX:
				if (m > 2) {
					fx.update_mode	= n[1];
					fx.which_fx		= n[2];
					
					if (m > 7) {
						fx.num_exclude_rects   = n[ 3];
						
						// We only support 2 exclusion rectangles from the proc entry.
						//
					    fx.exclude_rects[0].x1 = n[ 4];
					    fx.exclude_rects[0].y1 = n[ 5];
					    fx.exclude_rects[0].x2 = n[ 6];
					    fx.exclude_rects[0].y2 = n[ 7];

					    fx.exclude_rects[1].x1 = n[ 8];
					    fx.exclude_rects[1].y1 = n[ 9];
					    fx.exclude_rects[1].x2 = n[10];
					    fx.exclude_rects[1].y2 = n[11];

					} else {
						fx.num_exclude_rects   = 0;
					}

					cmd = FBIO_EINK_UPDATE_DISPLAY_FX;
					arg = (unsigned long)&fx;
				}
				break;
		
		    case PROC_EINK_UPDATE_DISPLAY_PNLCD:
		        cmd = FBIO_EINK_FAKE_PNLCD;
		        break;
		        
		    case PROC_EINK_SET_REBOOT_BEHAVIOR:
				if (m > 1) {
					cmd = FBIO_EINK_SET_REBOOT_BEHAVIOR;
					arg = n[1];
				}
				break;
				
			case PROC_EINK_SET_PROGRESSBAR_XY:
			    if (m > 2) {
					cmd = FBIO_EINK_PROGRESSBAR_SET_XY;
					progressbar_xy.x = n[1];
					progressbar_xy.y = n[2];
					
					arg = (unsigned long)&progressbar_xy;
			    }
			    break;
			    
			case PROC_EINK_PROGRESSBAR_BADGE:
			    if (m > 1) {
					cmd = FBIO_EINK_PROGRESSBAR_BADGE;
					arg = n[1];
			    }
			    break;
			    
			case PROC_EINK_PROGRESSBAR_BACKGROUND:
			    if (m > 1) {
					cmd = FBIO_EINK_PROGRESSBAR_BACKGROUND;
					arg = n[1];
			    }
			    break;
		    
		    case PROC_EINK_SET_DISPLAY_ORIENTATION:
			    if (m > 1) {
					cmd = FBIO_EINK_SET_DISPLAY_ORIENTATION;
					arg = n[1];
			    }
			    break;
		        
		    case PROC_EINK_RESTORE_DISPLAY:
			    if (m > 1) {
					cmd = FBIO_EINK_RESTORE_DISPLAY;
					arg = n[1];
			    }
			    break;
		    
		    case PROC_EINK_FAKE_PNLCD_TEST:
		        cmd = FBIO_EINK_FAKE_PNLCD;
		        arg = PROC_EINK_FAKE_PNLCD_TEST;
		    	break;
		    	
		    case PROC_EINK_GRAYSCALE_TEST:
		        einkfb_display_grayscale_ramp();
		        break;
		}
		
		if (cmd) {
			EINKFB_IOCTL(cmd, arg);
		}
	}
	
	return count;
}

// /proc/eink_fb/power_level (read/write)
//
static int read_power_level(char *page, unsigned long off, int count)
{
	einkfb_power_level power_level = einkfb_get_power_level();

	return ( sprintf(page, "%d (%s)\n", power_level, einkfb_get_power_level_string(power_level)) );
}

static int power_level_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return EINKFB_PROC_SYSFS_RW_NO_LOCK(page, NULL, off, 0, eof, 0, read_power_level);
}

static int write_power_level(char *buf, unsigned long count, int unused)
{
	char power_level_string[16] = { 0 };
	int result = -EINVAL;
	
	if ( (16 > count) && sscanf(buf, "%s", power_level_string) )
	{
		int power_level = (int)simple_strtoul(power_level_string, NULL, 0);
		EINKFB_BLANK(power_level);
		
		result = count;
	}

	return ( result );
}

static int power_level_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	return ( EINKFB_PROC_SYSFS_RW_NO_LOCK((char *)buf, NULL, count, 0, NULL, 0, write_power_level) );
}

// /proc/eink_fb/virtual_fb (read-only)
//
static int read_virtualfb_count(unsigned long addr, unsigned char *data, int count)
{
    struct einkfb_info info;
    int result = 0;
    
    einkfb_get_info(&info);
    
    if ( info.vfb && data && IN_RANGE((addr + count), 0, info.size) )
    {
        EINKFB_MEMCPYK(data, &info.vfb[addr], count);
        result = 1;
    }

    return ( result );
}

static int read_virtualfb(char *page, unsigned long off, int count)
{
    struct einkfb_info info; einkfb_get_info(&info);
	return ( einkfb_read_which(page, off, count, read_virtualfb_count, info.size) );
}

static int virtualfb_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	struct einkfb_info info; einkfb_get_info(&info);
	return ( EINKFB_PROC_SYSFS_RW_NO_LOCK(page, start, off, count, eof, info.size, read_virtualfb) );
}

// /sys/devices/platform/eink_fb0/power_timer_delay (read/write)
//
static ssize_t show_einkfb_power_timer_delay(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", einkfb_get_power_timer_delay()) );
}

static ssize_t store_einkfb_power_timer_delay(FB_DSTOR_PARAMS)
{
	char power_timer_delay_string[16] = { 0 };
	int result = -EINVAL;
	
	if ( (16 > count) && sscanf(buf, "%s", power_timer_delay_string) )
	{
		einkfb_set_power_timer_delay((int)simple_strtoul(power_timer_delay_string, NULL, 0));
		result = count;
	}

	return ( result );
}

// /sys/devices/platform/eink_fb0/flash_mode (read/write)
//
bool einkfb_get_flash_mode(void)
{
    return ( einkfb_flash_mode );
}

static void einkfb_set_flash_mode(bool mode)
{
    if ( mode )
    {
        if ( hal_ops.hal_get_power_level )
        {
            // Ensure that we can set the power level.
            //
            einkfb_flash_mode = false;
            
            // Say that we want the power level on and that we want to keep it on.
            //
            EINKFB_SET_BLANK_UNBLANK_LEVEL(einkfb_power_level_on);
            EINKFB_BLANK(FB_BLANK_UNBLANK);
        }

        // Disable changing the power level.
        //
        einkfb_flash_mode = true;
    }
    else
    {
        // Ensure that we can set the power level.
        //
        einkfb_flash_mode = false;
        
        if ( hal_ops.hal_get_power_level )
        {
            // Say that we want the power level to drift back to standby when not
            // in use.
            //
            EINKFB_SET_BLANK_UNBLANK_LEVEL(einkfb_power_level_standby);
            EINKFB_BLANK(FB_BLANK_UNBLANK);
        }
    }
}

static ssize_t show_einkfb_flash_mode(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "%d\n", (int)einkfb_flash_mode) );
}

static ssize_t store_einkfb_flash_mode(FB_DSTOR_PARAMS)
{
	char flash_mode_string[16] = { 0 };
	int result = -EINVAL;
	
	if ( (16 > count) && sscanf(buf, "%s", flash_mode_string) )
	{
		bool flash_mode = simple_strtoul(flash_mode_string, NULL, 0) ? true : false;
		
		if ( einkfb_flash_mode != flash_mode )
		    einkfb_set_flash_mode(flash_mode);
		
		result = count;
	}

	return ( result );
}

// /sys/devices/platform/eink_fb0/logging (read/write)
//
static ssize_t show_einkfb_logging(FB_DSHOW_PARAMS)
{
	return ( sprintf(buf, "0x%08lX\n", einkfb_logging) );
}

static ssize_t store_einkfb_logging(FB_DSTOR_PARAMS)
{
	char logging_string[16] = { 0 };
	int result = -EINVAL;
	
	if ( (16 > count) && sscanf(buf, "%s", logging_string) )
	{
		einkfb_logging = simple_strtoul(logging_string, NULL, 0);
		result = count;
	}
	
	return ( result );
}

// /sys/devices/platform/eink_fb0/reset (read/write)
//
bool einkfb_get_reset(void)
{
    return ( einkfb_reset );
}

void einkfb_set_reset(bool reset)
{
    einkfb_reset = reset ? true : false;
    
    if ( reset )
    {
	    mm_segment_t saved_fs = get_fs();
	    int einkfb_reset_file;
	    
	    set_fs(get_ds());
	    
	    einkfb_reset_file = sys_open(EINKFB_RESET_FILE, O_WRONLY, 0);
	    
	    if ( 0 <= einkfb_reset_file )
	    {
	        sys_write(einkfb_reset_file, "1", 1);
	        sys_close(einkfb_reset_file);
	    }
    
        set_fs(saved_fs);
    }
}

static ssize_t show_einkfb_reset(FB_DSHOW_PARAMS)
{
    return ( sprintf(buf, "%d\n", einkfb_reset) );
}

static ssize_t store_einkfb_reset(FB_DSTOR_PARAMS)
{
	char reset_string[16] = { 0 };
	int result = -EINVAL;
	
	if ( (16 > count) && sscanf(buf, "%s", reset_string) )
	{
		bool reset = simple_strtoul(reset_string, NULL, 0) ? true : false;
		
		if ( einkfb_reset != reset )
		    einkfb_set_reset(reset);
		
		result = count;
	}

	return ( result );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

static struct proc_dir_entry *einkfb_create_proc_parent(void)
{
    einkfb_proc_parent = create_proc_entry(EINKFB_NAME, EINKFB_PROC_PARENT, NULL);
    return ( einkfb_proc_parent );
}

static DEVICE_ATTR(power_timer_delay, DEVICE_MODE_RW,   show_einkfb_power_timer_delay,  store_einkfb_power_timer_delay);
static DEVICE_ATTR(flash_mode,        DEVICE_MODE_RW,   show_einkfb_flash_mode,         store_einkfb_flash_mode);
static DEVICE_ATTR(logging,           DEVICE_MODE_RW,   show_einkfb_logging,            store_einkfb_logging);
static DEVICE_ATTR(reset,             DEVICE_MODE_RW,   show_einkfb_reset,              store_einkfb_reset);

static void einkfb_create_hal_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);
    
    // Create proc entries.
    //
    einkfb_proc_update_display = einkfb_create_proc_entry(EINKFB_PROC_UPDATE_DISPLAY, EINKFB_PROC_CHILD_W,
        NULL, update_display_write);
        
    einkfb_proc_power_level = einkfb_create_proc_entry(EINKFB_PROC_POWER_LEVEL, EINKFB_PROC_CHILD_R,
        power_level_read, power_level_write);
      
    einkfb_proc_virtualfb = einkfb_create_proc_entry(EINKFB_PROC_VIRTUALFB, EINKFB_PROC_CHILD_R,
        virtualfb_read, NULL);
    
    // Create sysfs entries.
    //
	FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_power_timer_delay);
	FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_flash_mode);
	FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_logging);
	FB_DEVICE_CREATE_FILE(&info.dev->dev, &dev_attr_reset);
}

static void einkfb_remove_hal_proc_entries(void)
{
    struct einkfb_info info; einkfb_get_info(&info);

    // Remove proc entries.
    //
    einkfb_remove_proc_entry(EINKFB_PROC_UPDATE_DISPLAY, einkfb_proc_update_display);
    einkfb_remove_proc_entry(EINKFB_PROC_POWER_LEVEL, einkfb_proc_power_level);
    einkfb_remove_proc_entry(EINKFB_PROC_VIRTUALFB, einkfb_proc_virtualfb);

    // Remove sysfs entries.
    //
    device_remove_file(&info.dev->dev, &dev_attr_power_timer_delay);
    device_remove_file(&info.dev->dev, &dev_attr_flash_mode);
    device_remove_file(&info.dev->dev, &dev_attr_logging);
    device_remove_file(&info.dev->dev, &dev_attr_reset);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark External Interfaces
    #pragma mark -
#endif

int einkfb_read_which(char *page, unsigned long off, int count, einkfb_read_t which_read, int which_size)
{
	int j = 0;

	einkfb_debug("offset=%d\n", (int)off);

	if ( (off < which_size) && count )
	{
	    int  i = off, total = min((which_size - off), (unsigned long)count), length = READ_SIZE,
	         num_loops = total/length, remainder = total % length;
	    bool done = false;
	        
	    do
	    {
	        if ( 0 >= num_loops )
	            length = remainder;
	            
	        (*which_read)(i, (page + j), length);
	        
	        i += length; j += length;
	        
	        if ( i < total )
	            num_loops--;
	        else
	            done = true;
	    }
	    while ( !done );
	}

	return ( j );
}

int einkfb_write_which(char *buf, unsigned long count, einkfb_write_t which_write, int which_size)
{
	unsigned char *lbuf;
	unsigned long curr_count = 0;
	unsigned long count_sav = 0;
	int status = 0;

	lbuf = vmalloc(WRITE_SIZE);
	if (lbuf == NULL) {
		return -1;
	}

	count_sav = count;
	count = min(count, (unsigned long)which_size);

	curr_count = 0;
	status = 1;

	einkfb_debug("size=%d\n", (int)count);

	while ((curr_count < count) && (status == 1)) {
		unsigned long write_size;
		write_size = min(WRITE_SIZE, (count - curr_count));

		einkfb_debug("curr_count=%d write_size=%d\n", (int) curr_count, (int) write_size);

		status = EINKFB_MEMCPYUF(lbuf, (buf + curr_count), write_size);
        if (!status) {
            status = (*which_write)(curr_count, lbuf, write_size);
        }

		einkfb_debug("done, status=%d\n", status);

		curr_count += write_size;
	}

	vfree(lbuf);
	
	return status == 1 ? count_sav : -1;
}

int einkfb_proc_sysfs_rw(char *r_page_w_buf, char **r_start, unsigned long r_off_w_count, int r_count,
	int *r_eof, int r_eof_len, bool lock, einkfb_rw_proc_sysf_t einkfb_rw_proc_sysfs)
{
	int result = EINKFB_SUCCESS;

	if ( !(r_eof && (r_off_w_count > r_eof_len)) )
	{
        bool unlock = lock;
        
        if ( lock && (EINKFB_SUCCESS == EINKFB_LOCK_ENTRY()) )
            unlock = true;
        
        if ( !lock || (lock && unlock) )
        {
            result = (*einkfb_rw_proc_sysfs)(r_page_w_buf, r_off_w_count, r_count);
            
            if ( r_start && r_page_w_buf )
                *r_start = r_page_w_buf;
        }
            
        if ( unlock )
            EINKFB_LOCK_EXIT();
    }

    if ( r_eof )
    	*r_eof = result >= r_eof_len;

    return ( result );
}

struct proc_dir_entry *einkfb_create_proc_entry(const char *name, mode_t mode, read_proc_t *read_proc, write_proc_t *write_proc)
{
    struct proc_dir_entry *einkfb_proc_entry = create_proc_entry(name, mode, einkfb_proc_parent);
    
    if ( einkfb_proc_entry )
    {
        einkfb_proc_entry->owner      = THIS_MODULE;
        einkfb_proc_entry->data       = NULL;
        einkfb_proc_entry->read_proc  = read_proc;
        einkfb_proc_entry->write_proc = write_proc;
    }
    
    return ( einkfb_proc_entry );
}

void einkfb_remove_proc_entry(const char *name, struct proc_dir_entry *entry)
{
    if ( entry )
    {
        struct proc_dir_entry *parent = NULL;
        
        if ( einkfb_proc_parent != entry )
            parent = einkfb_proc_parent;
        
        remove_proc_entry(name, parent); 
    }
}

bool einkfb_create_proc_entries(void)
{
    bool result = EINKFB_FAILURE;
    
    if ( einkfb_create_proc_parent() )
    {
        einkfb_create_hal_proc_entries();
        
        if ( hal_ops.hal_create_proc_entries )
            hal_ops.hal_create_proc_entries();
            
        result = EINKFB_SUCCESS;
    }
    
    return ( result );
}

void einkfb_remove_proc_entries(void)
{
    if ( einkfb_proc_parent )
    {
        if ( hal_ops.hal_remove_proc_entries )
            hal_ops.hal_remove_proc_entries();
            
        einkfb_remove_hal_proc_entries();

        einkfb_remove_proc_entry(EINKFB_NAME, einkfb_proc_parent);
    }
}

EXPORT_SYMBOL(einkfb_get_flash_mode);
EXPORT_SYMBOL(einkfb_logging);
EXPORT_SYMBOL(einkfb_get_reset);
EXPORT_SYMBOL(einkfb_set_reset);
EXPORT_SYMBOL(einkfb_read_which);
EXPORT_SYMBOL(einkfb_write_which);
EXPORT_SYMBOL(einkfb_proc_sysfs_rw);
EXPORT_SYMBOL(einkfb_create_proc_entry);
EXPORT_SYMBOL(einkfb_remove_proc_entry);
