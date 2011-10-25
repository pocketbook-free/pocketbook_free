/*
 * e-ink display framebuffer driver
 * 
 * Copyright (C)2005-2007 Lab126, Inc.  All rights reserved.
 */

#ifndef _APOLLO_H
#define _APOLLO_H

#ifndef FOR_UBOOT_BUILD //------------- Linux Build

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/syscalls.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/segment.h>
#include <asm/hardware.h>
#include <asm/io.h>

#include <asm/arch/fiona.h>
#include <asm/arch/fpow.h>
#include <asm/delay.h>
#include <asm/arch/pxa-regs.h>

#include <asm/arch/boot_globals.h>

extern wait_queue_head_t apollo_ack_wq;
extern void apollo_init_irq(void);
extern void apollo_shutdown_irq(void);
extern int apollo_init_kernel(int full);
extern void apollo_done_kernel(int full);

extern int g_fiona_power_debug;
extern int eink_debug;

#define einkdebug(x...)	if (g_fiona_power_debug || eink_debug) printk(x)

extern unsigned char einkfb_stretch_nybble(unsigned char nybble, unsigned long bpp);

#define STRETCH_HI_NYBBLE(n, b)         einkfb_stretch_nybble(((0xF0 & n) >> 4), b)
#define STRETCH_LO_NYBBLE(n, b)         einkfb_stretch_nybble(((0x0F & n) >> 0), b)

#ifdef _EINKFB_HAL_H    //------------- eInk HAL Build
inline u8 *eink_fb_get_framebuffer(void)
{
	struct einkfb_info info;
	einkfb_get_info(&info);
	
    return ( (u8 *)info.start );
}

inline unsigned long eink_fb_get_bpp(void)
{
	struct einkfb_info info;
	einkfb_get_info(&info);
	
    return ( info.bpp );
}
#endif

#else   // ---------------------------- UBoot Build

#include <common.h>
#include <asm/arch/pxa-regs.h>

#include "../board/fiona/ub_boot_globals.h"

#define STRETCH_HI_NYBBLE(n, b)     stretch_nybble(((0xF0 & n) >> 4), b)
#define STRETCH_LO_NYBBLE(n, b)     stretch_nybble(((0x0F & n) >> 0), b)

#define init_waitqueue_head(q)		do {} while(0)
#define apollo_init_irq()			do {} while(0)
#define apollo_shutdown_irq()		do {} while(0)

#define writel(v,r)					(*(r) = (v))
#define readl(r)					(*(r))
#define ioremap_nocache(a,l)		((a))
#define iounmap(a)					do {} while(0)
#define printk						printf

#define HZ                          CFG_HZ

extern unsigned char stretch_nybble(unsigned char nybble, unsigned long bpp);
extern void pxa_gpio_mode(int gpio_mode);
extern int do_reboot(void);

extern bool g_ack_timed_out;

#endif  // ----------------------------

typedef u32 * volatile reg_addr_t;

#undef ACTIVATE_DO_DEBUG
#ifdef ACTIVATE_DO_DEBUG
#define do_debug(x...) printk(x)
#else
#define do_debug(x...)
#endif

#define	GAFR_bit(x) (3 << (((x) & 0x0f) << 1))

#define MAX_TIMEOUT  (HZ * 4)
#define FAST_TIMEOUT 1000
#define MAX_TIMEOUTS 5

#define BASE_ADDR 0x40E00000
#define MAP_SIZE 4096
#define MAP_MASK (MAP_SIZE - 1)

#define GPSR1_ADDR 0x40E0001C
#define GPSR2_ADDR 0x40E00020
#define GPCR1_ADDR 0x40E00028
#define GPCR2_ADDR 0x40E0002C
#define GPLR1_ADDR 0x40E00004
#define GPLR2_ADDR 0x40E00008

#define GPDR1_ADDR 0x40E00010
#define GPDR2_ADDR 0x40E00014

#define apollo_write_reg(reg, val) writel((val), (reg))
#define apollo_read_reg(reg) readl((reg))

#define APOLLO_NOP                      0x00

#define APOLLO_FLASH_WRITE				0x01
#define APOLLO_FLASH_READ				0x02

#define APOLLO_WRITE_REGISTER			0x10
#define APOLLO_READ_REGISTER			0x11

#define APOLLO_GET_TEMPERATURE_CMD		0x21

#define APOLLO_LOAD_IMAGE_CMD			0xA0
#define APOLLO_STOP_IMGLD_CMD			0xA1
#define APOLLO_DISP_IMAGE_CMD			0xA2
#define APOLLO_ERASE_SCRN_CMD			0xA3
#define APOLLO_INIT_DISPLAY_CMD			0xA4
#define APOLLO_RESTORE_IMAGE_CMD		0xA5
#define APOLLO_GET_STATUS_CMD			0xAA

#define APOLLO_LOAD_PARTIAL_IMAGE_CMD	0xB0
#define APOLLO_DISP_PARTIAL_IMAGE_CMD	0xB1

#define APOLLO_GET_VERSION_CMD			0xE0
#define APOLLO_RESET_CMD				0xEE

#define APOLLO_NORMAL_MODE_CMD			0xF0
#define APOLLO_SLEEP_MODE_CMD			0xF1
#define APOLLO_STANDBY_MODE_CMD			0xF2
#define APOLLO_SET_DEPTH_CMD			0xF3
#define APOLLO_SET_ORIENTATION_CMD		0xF5
#define APOLLO_POSITIVE_PICTURE_CMD		0xF7
#define APOLLO_NEGATIVE_PICTURE_CMD		0xF8
#define APOLLO_AUTO_REFRESH_CMD         0xF9
#define APOLLO_CANCEL_AUTO_REFRESH_CMD	0xFA
#define APOLLO_SET_REFRESH_TIMER_CMD    0xFB
#define APOLLO_MANUAL_REFRESH_CMD		0xFC
#define APOLLO_READ_REFRESH_TIMER_CMD   0xFD

#define APOLLO_MAX_REFRESH_TIME         0xFF

#define IS_LOAD_IMAGE_CMD(c)            (((c) == APOLLO_LOAD_IMAGE_CMD)         ||   \
                                         ((c) == APOLLO_LOAD_PARTIAL_IMAGE_CMD))

#define IS_DISPLAY_IMAGE_CMD(c)         (((c) == APOLLO_DISP_IMAGE_CMD)         ||   \
                                         ((c) == APOLLO_DISP_PARTIAL_IMAGE_CMD))

#define APOLLO_FLASH_MAX_RETRIES		5

#define APOLLO_DEPTH_1BPP				0x00
#define APOLLO_DEPTH_2BPP				0x02

#define APOLLO_WHITE					0x00
#define APOLLO_BLACK					0xFF

#define APOLLO_ORIENTATION_0			0x00        // landscape upside down
#define APOLLO_ORIENTATION_90			0x01        // portrait  upside down
#define APOLLO_ORIENTATION_180			0x02        // landscape
#define APOLLO_ORIENTATION_270			0x03        // portrait

// For use with apollo_set_init_display_speed() & apollo_get_init_display_speed().
// These are local to either the kernel or bootloader.
//
#define APOLLO_INIT_DISPLAY_FAST		0x00
#define APOLLO_INIT_DISPLAY_SLOW		0x01
#define APOLLO_INIT_DISPLAY_SKIP		0x02		// flag, only used by kernel
#define APOLLO_INIT_DISPLAY_WAKE		0x03		// flag, only used bootloader

// For use with set_apollo_init_display_flag() && get_apollo_init_display_flag()
// These are global and are used between the kernel & bootloader.
//
#define INIT_DISPLAY_FAST				0x74736166
#define INIT_DISPLAY_SLOW				0x776F6C73
#define INIT_DISPLAY_SKIP				0x70696B63	// flag, only used by kernel
#define INIT_DISPLAY_WAKE				0x656B6177	// flag, only used bootloader

#define APOLLO_ERASE_SCREEN_BLACK		0x00
#define APOLLO_ERASE_SCREEN_WHITE		0x01
#define APOLLO_ERASE_SCREEN_DARK_GRAY	0x02
#define APOLLO_ERASE_SCREEN_LITE_GRAY	0x03

#define APOLLO_STANDBY_CMD_DELAY		600		// uSec delay after Standby cmd is issued
#define APOLLO_RESET_CMD_DELAY			20		// uSec delay after Reset cmd is issued
#define APOLLO_SEND_DELAY				100		// uSec delay after sending cmds or data prior to initialization

#define APOLLO_T1_DELAY_REGISTER		0x20
#define APOLLO_T1_DELAY_DEFAULT			0x40

#define APOLLO_OFF_DELAY_REGISTER		0x22
#define APOLLO_OFF_DELAY_DEFAULT		0
#define APOLLO_OFF_DELAY_THRESH			4
#define APOLLO_OFF_DELAY_MAX			255

#define APOLLO_STANDBY_OFF_POWER_MODE	0		// standby + off
#define APOLLO_NORMAL_POWER_MODE		1
#define APOLLO_SLEEP_POWER_MODE			2
#define APOLLO_STANDBY_POWER_MODE		3		// just standby

#define APOLLO_UNKNOWN_POWER_MODE		42

#define IS_APOLLO_STANDBY_MODE(m)		((APOLLO_STANDBY_OFF_POWER_MODE == m)	||	\
										 (APOLLO_STANDBY_POWER_MODE == m))

#define APOLLO_POWER_DOWN_SLEEP			0
#define APOLLO_POWER_DOWN_FULL			1

#define EINK_DEPTH_1BPP					1 // Apollo
#define EINK_DEPTH_2BPP					2 // Apollo

#define APOLLO_X_RES					600
#define APOLLO_Y_RES					800

#define APOLLO_ROWBYTES_1BPP			((APOLLO_X_RES * EINK_DEPTH_1BPP)/8)
#define APOLLO_ROWBYTES_2BPP			((APOLLO_X_RES * EINK_DEPTH_2BPP)/8)
#define APOLLO_ROWBYTES_4BPP			((APOLLO_X_RES * EINK_DEPTH_4BPP)/8)

#define	INIT_DISPLAY_FLAG_UNKNOWN(f)	(!((INIT_DISPLAY_FAST == f)	|| \
										   (INIT_DISPLAY_SLOW == f)))

#define ReadFromFlash(a,d)				apollo_read_from_flash_byte(a,d)

#define APOLLO_WRITE_COMMAND            0 // For apollo_read_write().
#define APOLLO_WRITE_DATA               1 //

#define APOLLO_WRITE_ONLY               0 // For apollo_send_command().
#define APOLLO_WRITE_NEEDS_DELAY        1 //

#define APOLLO_READ                     2 // For both apollo_read_write() & apollo_send_command().

#define apollo_write_command(c)         apollo_read_write(APOLLO_WRITE_COMMAND, c)
#define apollo_write_data(d)            apollo_read_write(APOLLO_WRITE_DATA, d)
#define apollo_read()                   apollo_read_write(APOLLO_READ, 0)

#define apollo_simple_set(c)            apollo_simple_send_command(c, APOLLO_WRITE_ONLY)
#define apollo_simple_get(c)            apollo_simple_send_command(c, APOLLO_READ)

#define APOLLO_ACK                      0
#define APOLLO_NACK                     1

#define APOLLO_MAX_CMD_ARGS             8

struct apollo_command_t
{
    u8  command,                        // APOLLO_XXX_CMD
        type,                           // APOLLO_WRITE_ONLY, APOLLO_WRITE_NEEDS_DELAY, APOLLO_READ
        num_args,                       // 
        args[APOLLO_MAX_CMD_ARGS];      // 
    u32 io;                             // [io = ]command(args[0], args[1], ...args[num_args-1])

    u32 buffer_size;                    //
    u8* buffer;                         // 
};
typedef struct apollo_command_t apollo_command_t;

struct apollo_temperature_t
{
	unsigned char	min,
					max,
					inc,
					cur,
					valid;			    // if true, min.max.inc aren't 0xFF
};
typedef struct apollo_temperature_t apollo_temperature_t;

struct apollo_status_t
{
	unsigned char	operation_mode : 1;
	unsigned char	screen		   : 2;
	unsigned char	auto_refresh   : 1;
	unsigned char	bpp			   : 1;
	unsigned char	reserved	   : 3;
};
typedef struct apollo_status_t apollo_status_t;

#include "apollo_waveform.h"
#include "apollo_universal.h"

extern unsigned long eink_fb_get_bpp(void);
extern u8 *eink_fb_get_framebuffer(void);
extern void eink_fb_clear_screen(void);

extern int needs_to_enable_temp_sensor(void);

extern void apollo_set_init_display_speed(unsigned char init_display_speed);
extern unsigned char apollo_get_init_display_speed(void);

extern unsigned char apollo_get_version(void);
extern unsigned char apollo_get_temperature(void);

extern void apollo_init(unsigned long bpp);
extern u32  apollo_get_ack(void);
extern int 	apollo_get_status(apollo_status_t *status);
extern void apollo_init_display(unsigned char init_display_speed);
extern void apollo_erase_screen(unsigned char color);
extern void apollo_clear_screen(int full);
extern u32  apollo_load_image_data(unsigned char *buffer, int bufsize);
extern int  apollo_wake_on_activity(void);

extern u32 apollo_load_partial_data(unsigned char *buffer, int bufsize, 
			     u16 x1, u16 y1, u16 x2, u16 y2);
extern void apollo_display(int full);
extern void apollo_display_partial(void);
extern void apollo_restore(void);

extern char *apollo_get_power_state_str(int state);
extern int apollo_set_power_state(int target_mode);
extern int apollo_get_power_state(void);

extern u32 apollo_read_write(int which, u8 wdata);
extern u8 apollo_simple_send_command(u8 command, u8 type);
extern void apollo_send_command(apollo_command_t *apollo_command);
extern void apollo_power_up(void);
extern void apollo_power_down(int full);

extern int apollo_comm_init(void);
extern int apollo_comm_shutdown(void);

extern void apollo_write_register(unsigned char regAddress, unsigned char data);
extern void apollo_read_register(unsigned char regAddress, unsigned char *data);

extern int apollo_read_from_flash_byte(unsigned long addr, unsigned char *data);
extern int apollo_read_from_flash_short(unsigned long addr, unsigned short *data);
extern int apollo_read_from_flash_long(unsigned long addr, unsigned long *data);

extern void apollo_get_temperature_info(apollo_temperature_t *temperature);
extern void apollo_override_fpl(apollo_fpl_t *fpl);

extern int ReadFromRam(unsigned long Address, unsigned long *Data);
extern int WriteToRam(unsigned long Address, unsigned long Data);

extern int ProgramFlash(unsigned long start_addr, 
		unsigned char *buffer, unsigned long blen);

extern int ProgramRam(unsigned long start_addr, 
		unsigned char *buffer, unsigned long blen);

extern int wait_for_ack_irq(int which);

extern u8 apollo_get_orientation(void);
extern void apollo_set_orientation(u8 orientation);

#endif // _APOLLO_H
