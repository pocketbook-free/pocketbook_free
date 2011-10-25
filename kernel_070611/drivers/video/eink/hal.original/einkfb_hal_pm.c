/*
 *  linux/drivers/video/eink/hal/einkfb_hal_pm.c -- eInk frame buffer device HAL power management
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

#define EINKFB_POWER_TIMER_DELAY_DEFAULT    2 // All in seconds, uses TIMER_EXPIRES_SECS().
#define EINKFB_POWER_TIMER_DELAY_MIN        1 //
#define EINFFB_POWER_TIMER_DELAY_MAX        5 //

#define EINKFB_POWER_THREAD_NAME            EINKFB_NAME"_pt"
#define FB_BLANK_UNKNOWN                    (-1)

static char *power_level_strings[] =
{
    "init",             // einkfb_power_level_init

    "on",               // einkfb_power_level_on
    "blank",            // einkfb_power_level_blank
    "standby",          // einkfb_power_level_standby 
    "sleep",            // einkfb_power_level_sleep
    "off"               // einkfb_power_level_off
};

static char *blanking_level_strings[] =
{
    "init",             // initial (unknown) state
    
    "unblank",          // FB_BLANK_UNBLANK
    "normal",           // FB_BLANK_NORMAL
    "vsync_suspend",    // FB_BLANK_VSYNC_SUSPEND
    "hsync_suspend",    // FB_BLANK_HSYNC_SUSPEND
    "powerdown"         // FB_BLANK_POWERDOWN
};

static einkfb_power_level blank_unblank_level = einkfb_power_level_standby;
static bool power_level_initiated_call = false;

static int				 einkfb_power_timer_delay   = EINKFB_POWER_TIMER_DELAY_DEFAULT;
static bool				 einkfb_power_timer_active  = false;
static bool              einkfb_power_timer_primed  = false;
static struct timer_list einkfb_power_timer;
static int einkfb_power_thread_exit = 0;
static THREAD_ID(einkfb_power_thread_id);
static DECLARE_COMPLETION(einkfb_power_thread_exited);
static DECLARE_COMPLETION(einkfb_power_thread_complete);

extern einkfb_power_level saved_power_level;

#if PRAGMAS
    #pragma mark -
    #pragma mark Local Utilities
    #pragma mark -
#endif

static void exit_einkfb_power_thread(void)
{
    einkfb_power_thread_exit = 1;
    complete(&einkfb_power_thread_complete);
}

static void einkfb_power_thread_body(void)
{
    // Take us to the blank_unblank_level if we still should.
    //
    if ( einkfb_power_timer_active && einkfb_power_timer_primed )
    {
        // Only attempt to go down now if we can; otherwise,
        // try again later.
        //
        if ( EINKFB_LOCK_READY_NOW() )
        {
            einkfb_power_timer_primed = false;
            EINKFB_BLANK(FB_BLANK_UNBLANK);
        }
        else
            einkfb_prime_power_timer(EINKFB_DELAY_TIMER);
    }
    
    wait_for_completion_interruptible(&einkfb_power_thread_complete);
}

static int einkfb_power_thread(void *data)
{
	int  *exit_thread = (int *)data;
    bool thread_active = true;
    
    THREAD_HEAD(EINKFB_POWER_THREAD_NAME);

    while ( thread_active )
    {
        TRY_TO_FREEZE();

        if ( !THREAD_SHOULD_STOP() )
        {
            THREAD_BODY(*exit_thread, einkfb_power_thread_body);
        }
        else
            thread_active = false;
    }

    THREAD_TAIL(einkfb_power_thread_exited);
}

static void einkfb_start_power_thread(void)
{
    THREAD_START(einkfb_power_thread_id, &einkfb_power_thread_exit, einkfb_power_thread,
        EINKFB_POWER_THREAD_NAME);
}

static void einkfb_stop_power_thread(void)
{
    THREAD_STOP(einkfb_power_thread_id, exit_einkfb_power_thread, einkfb_power_thread_exited);
}

static void einkfb_power_timer_function(unsigned long unused)
{
    einkfb_debug_power("power timer expired\n");
    complete(&einkfb_power_thread_complete);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Internal Interfaces
    #pragma mark -
#endif

einkfb_power_level EINKFB_GET_POWER_LEVEL(void)
{
    einkfb_power_level power_level = blank_unblank_level;     
        
    if ( hal_ops.hal_get_power_level )
        power_level = (*hal_ops.hal_get_power_level)();

    einkfb_debug_power("power_level = %d (%s)\n",
        power_level, einkfb_get_power_level_string(power_level));
    
    return ( power_level );
}

void EINKFB_SET_POWER_LEVEL(einkfb_power_level power_level)
{
    einkfb_power_level saved_level = power_level;
    
    einkfb_debug_power("power_level = %d (%s)\n",
        power_level, einkfb_get_power_level_string(power_level));

    if ( hal_ops.hal_set_power_level )
    {
        // Keep us up if we shouldn't go down now.
        //
        if ( einkfb_power_timer_primed )
        {
            switch ( power_level )
            {
                case einkfb_power_level_on:
                case einkfb_power_level_standby:
                    einkfb_prime_power_timer(EINKFB_DELAY_TIMER);
                    power_level = einkfb_power_level_on;
                break;
                
                // Expire the timer everywhere else.
                //
                default:
                    einkfb_prime_power_timer(EINKFB_EXPIRE_TIMER);
                break;
            }
        }

        // Skip changing the power level altogether if we're flashing the ROM.
        //
        if ( !EINKFB_FLASHING_ROM() )
            (*hal_ops.hal_set_power_level)(power_level);
        
        // Note whether we had to change the passed-in power level or not.
        //
        if ( power_level != saved_level )
            einkfb_debug_power("power_level = %d (%s) -> %d (%s)\n",
                saved_level, einkfb_get_power_level_string(saved_level),
                power_level, einkfb_get_power_level_string(power_level));
    }
}

#if PRAGMAS
    #pragma mark -
    #pragma mark External Interfaces
    #pragma mark -
#endif

einkfb_power_level einkfb_get_power_level(void)
{
    einkfb_power_level power_level = einkfb_power_level_init;
    
    power_level_initiated_call = true;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        power_level = saved_power_level;
        EINKFB_LOCK_EXIT();
    }
    
    power_level_initiated_call = false;

    return ( power_level );
}

void einkfb_set_power_level(einkfb_power_level power_level)
{
    power_level_initiated_call = true;
    
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        EINKFB_SET_POWER_LEVEL(power_level);
        EINKFB_LOCK_EXIT();
    }
    
    power_level_initiated_call = false;
}

char *einkfb_get_power_level_string(einkfb_power_level power_level)
{
     char *power_level_string = NULL;
     
     switch ( power_level )
     {
        case einkfb_power_level_on:
        case einkfb_power_level_blank:
        case einkfb_power_level_standby:
        case einkfb_power_level_sleep:
        case einkfb_power_level_off:
            power_level_string = power_level_strings[power_level+1];
        break;

        default:
            power_level_string = power_level_strings[0];
        break;
     }
     
     return ( power_level_string );
}

static char *einkfb_get_blanking_level_string(int blank)
{
    char *blanking_level_string = NULL;
    
    switch ( blank )
    {
        case FB_BLANK_UNBLANK:
        case FB_BLANK_NORMAL:
        case FB_BLANK_VSYNC_SUSPEND:
        case FB_BLANK_HSYNC_SUSPEND:
        case FB_BLANK_POWERDOWN:
            blanking_level_string = blanking_level_strings[blank + 1];
        break;
        
        default:
            blanking_level_string = blanking_level_strings[0];
        break;
    }
    
    return ( blanking_level_string );
}

void einkfb_set_blank_unblank_level(einkfb_power_level power_level)
{
    switch ( power_level )
    {
        case einkfb_power_level_on:
            saved_power_level = einkfb_power_level_on;
        case einkfb_power_level_standby:
            blank_unblank_level = power_level;
        break;
        
        default:
            blank_unblank_level = einkfb_power_level_standby;
        break;
    }
}

void EINKFB_SET_BLANK_UNBLANK_LEVEL(einkfb_power_level power_level)
{
    if ( EINKFB_SUCCESS == EINKFB_LOCK_ENTRY() )
    {
        einkfb_set_blank_unblank_level(power_level);
        EINKFB_LOCK_EXIT();
    }
}

int einkfb_blank(int blank, struct fb_info *info)
{
    einkfb_power_level power_level;
    int result = EINKFB_SUCCESS;
    einkfb_event_t event;
    
    einkfb_debug_power("blank = %d (%s)\n",
        blank, einkfb_get_blanking_level_string(blank));
        
    einkfb_init_event(&event);
    event.event = einkfb_event_blank_display;

    switch ( blank )
    {
        case FB_BLANK_UNBLANK:
            power_level = blank_unblank_level;
        goto set_power_level;
        
        case FB_BLANK_NORMAL:
            power_level = einkfb_power_level_blank;
            einkfb_post_event(&event);
        goto set_power_level;
        
        case FB_BLANK_VSYNC_SUSPEND:
            power_level = einkfb_power_level_standby;
        goto set_power_level;
        
        case FB_BLANK_HSYNC_SUSPEND:
            power_level = einkfb_power_level_sleep;
        goto set_power_level;
        
        case FB_BLANK_POWERDOWN:
            power_level = einkfb_power_level_off;
            einkfb_post_event(&event);
        /* goto set_power_level; */
        
        set_power_level:
            einkfb_set_power_level(power_level);
        break;
        
        default:
            result = EINKFB_IOCTL_FAILURE;
        break;
    }
    
    return ( result );
}

void einkfb_start_power_timer(void)
{
	init_timer(&einkfb_power_timer);
	
	einkfb_power_timer.function = einkfb_power_timer_function;
	einkfb_power_timer.data = 0;

	einkfb_power_timer_active  = true;
    einkfb_power_timer_primed  = false;
    
    einkfb_start_power_thread();
}

void einkfb_stop_power_timer(void)
{
	if ( einkfb_power_timer_active )
	{
		einkfb_power_timer_active  = false;
		einkfb_power_timer_primed  = false;

		einkfb_stop_power_thread();
		del_timer_sync(&einkfb_power_timer);
	}
}

void einkfb_prime_power_timer(bool delay_timer)
{
	if ( einkfb_power_timer_active )
	{
		unsigned long timer_delay;
		
		// If requested, delay the timer.
		//
		if ( delay_timer )
		{
		    timer_delay = einkfb_power_timer_delay;
		    einkfb_power_timer_primed = true;
		}
		else
		{
		    timer_delay = MIN_SCHEDULE_TIMEOUT;
		    einkfb_power_timer_primed = false;
		}

        mod_timer(&einkfb_power_timer, TIMER_EXPIRES_SECS(timer_delay));
	}
}

int einkfb_get_power_timer_delay(void)
{
    return ( einkfb_power_timer_delay );
}

void einkfb_set_power_timer_delay(int power_timer_delay)
{
    if ( IN_RANGE(power_timer_delay, EINKFB_POWER_TIMER_DELAY_MIN, EINFFB_POWER_TIMER_DELAY_MAX) )
        einkfb_power_timer_delay = power_timer_delay;
    else
        einkfb_power_timer_delay = EINKFB_POWER_TIMER_DELAY_DEFAULT;
}

void einkfb_power_override_begin(void)
{
    power_level_initiated_call = true;
}

void einkfb_power_override_end(void)
{
    power_level_initiated_call = false;
}

bool einkfb_get_power_override_state(void)
{
    return ( power_level_initiated_call );
}

EXPORT_SYMBOL(power_level_initiated_call);
EXPORT_SYMBOL(einkfb_set_blank_unblank_level);
EXPORT_SYMBOL(einkfb_blank);
EXPORT_SYMBOL(einkfb_power_override_begin);
EXPORT_SYMBOL(einkfb_power_override_end);
EXPORT_SYMBOL(einkfb_get_power_override_state);
