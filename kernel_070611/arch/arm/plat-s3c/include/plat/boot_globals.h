/* 
 * Globals used to communicate between the bootloader (U-Boot) and the
 * the kernel (Linux) across boot-ups and restarts.
 * 
 * Copyright (C)2006-2009 Lab126, Inc.  All rights reserved.
 */

#ifndef _BOOT_GLOBALS_H
#define _BOOT_GLOBALS_H

#ifndef FOR_UBOOT_BUILD     //------------- Linux Build

#include <linux/module.h>
#include <linux/string.h>

#include <asm/page.h>
#include <asm/setup.h>

#define FRAMEBUFFER_SIZE            (PAGE_SIZE * 30)
#define ENV_SCRIPT_STR_SZ           256

#ifdef CONFIG_ARCH_FIONA    //------------- Fiona (Linux 2.6.10) Build

#define PLFRM_MEM_BASE              0xA0000000
#define PLFRM_MEM_SIZE              0x04000000
#define PLFRM_MEM_MAX               (PLFRM_MEM_BASE + PLFRM_MEM_SIZE)

#else                       // ------------ Mario (Linux 2.6.22) Build

#define PLFRM_MEM_BASE              0x80000000
#define PLFRM_MEM_SIZE              0x08000000
#define PLFRM_MEM_MAX               (PLFRM_MEM_BASE + PLFRM_MEM_SIZE)

#endif

#else   // -------------------------------- U-Boot Build

#include <common.h>

#define EXPORT_SYMBOL(s)
#define PAGE_SIZE                   (RESERVE_RAM_SIZE)

#endif  // ----------------------------

#define BOOT_GLOBALS_SIZE           PAGE_SIZE
#define BOOT_GLOBALS_BASE           (PLFRM_MEM_MAX - BOOT_GLOBALS_SIZE)

#define OOPS_SAVE_SIZE              PAGE_SIZE
#define OOPS_SAVE_BASE              (BOOT_GLOBALS_BASE - OOPS_SAVE_SIZE)

#define BOOT_GLOBALS_WARM_FLAG      0x6D726177      // 'warm': Warm Boot.
#define BOOT_GLOBALS_COLD_FLAG      0x646C6F63      // 'cold': Cold Boot.

#define BOOT_GLOBALS_UPDATE_FLAG    0x61647075      // 'upda': Firmware Update.
#define BOOT_GLOBALS_REPART_FLAG    0x61706572      // 'repa': Repartition.
#define BOOT_GLOBALS_RESTOR_FLAG    0x74736572      // 'rest': Firmware Restore.
#define BOOT_GLOBALS_SERVIC_FLAG    0x76726573      // 'serv': Service Menu.

#define BOOT_GLOBALS_FB_INIT_FLAG   0x66756266      // 'fbuf': Framebuffer Init'ed.

#define BOOT_GLOBALS_ACK_FLAG       0

#define EINK_INIT_DISPLAY_ASIS      0x7065656B      // 'keep': Keep whatever is on the display.

#define FREE_SIZE_0                 19              
#define BUILD_REV_SIZE              252
#define SCRATCH_SIZE                256
#define SERIAL_NUM_SIZE             BOARD_SERIALNUM_SIZE
#define ENV_SCRIPT_SIZE             ENV_SCRIPT_STR_SZ

#define NUM_BAT_LEVEL_STAGES        6
#define BAT_LEVEL_UPDT              0
#define BAT_LEVEL_WAN               1
#define BAT_LEVEL_LOW               2
#define BAT_LEVEL_CRIT              3
#define BAT_LEVEL_RSRV1             4
#define BAT_LEVEL_RSRV2             5

#define CLEAR_DBS_UNKNOWN           0               // clear_dbs not set yet
#define CLEAR_DBS_TRUE              1               // for stacked parts
#define CLEAR_DBS_FALSE             2               // for monolithic parts

#define SYNC_SETTINGS_UNKNOWN       0               // sync settings not set
#define SYNC_SETTINGS_INITED        1               // sync settings initialized

#define UNINITIALIZED_RVE_VALUE     0               // could change so make it a symbol
#define UNINITIALIZED_GAIN_VALUE    0               // could change so make it a symbol
#define UNINITIALIZED_OP_AMP_OFFSET 0xFFFF          // op-amp offset not yet initialized

#define SOFTWARE_UPDATE()           ((BOOT_GLOBALS_UPDATE_FLAG == get_update_flag()) || \
                                     (BOOT_GLOBALS_REPART_FLAG == get_update_flag()) || \
                                     (BOOT_GLOBALS_RESTOR_FLAG == get_update_flag()) || \
                                     (BOOT_GLOBALS_SERVIC_FLAG == get_update_flag()))

#define DISABLE_PDC_SUSPEND()       (1 == get_hw_flags()->disable_pdc_suspend)
#define INVERT_VIDEO()              (1 == get_hw_flags()->invert_video)

#define APOLLO_IS_APOLLO()          (0 == get_panel_id()->apollo_is_pvi)
#define APOLLO_IS_PVI()             (1 == get_panel_id()->apollo_is_pvi)

#define FRAMEWORK_STARTED()         (0 != get_framework_started())
#define FRAMEWORK_RUNNING()         (0 != get_framework_running())
#define FRAMEWORK_STOPPED()         (0 != get_framework_stopped())

#define IN_DEV_MODE()               (0 != get_dev_mode())

#define FB_INITED()                 (BOOT_GLOBALS_FB_INIT_FLAG == get_fb_init_flag())
#define EINK_DISPLAY_ASIS()         (EINK_INIT_DISPLAY_ASIS == get_eink_init_display_flag())

#define FAKE_PNLCD_IN_USE()         (1 == get_pnlcd_flags()->enable_fake)
#define FAKE_PNLCD_UPDATE()         (FAKE_PNLCD_IN_USE() && (1 == get_pnlcd_flags()->updating))
#define HIDE_PNLCD()                (FAKE_PNLCD_IN_USE() && (1 == get_pnlcd_flags()->hide_real))

#define INC_PNLCD_EVENT_COUNT()     inc_pnlcd_event_count()
#define DEC_PNLCD_EVENT_COUNT()     (0 == dec_pnlcd_event_count())

#define PNLCD_EVENT_PENDING()       (0 < get_pnlcd_flags()->event_count)

#define PANEL_ID_6_0_INCH_SIZE      6
#define PANEL_ID_9_7_INCH_SIZE      9

#define PANEL_ID_6_0_INCH_PARAMS    "mode:416x622-16,active,hsynclen:28,right:34,left:34,vsynclen:25,upper:0,lower:2,hsync:1,vsync:1,pixclock:59800,pixclockpol:0,acbias:24"
#define PANEL_ID_9_7_INCH_PARAMS    "mode:640x848-16,active,hsynclen:4,right:4,left:4,vsynclen:4,upper:3,lower:3,hsync:1,vsync:1,pixclock:40000,pixclockpol:0,acbias:24"

struct panel_id_t                                   // 8 bytes
{
    unsigned char   override,
                    platform;
    unsigned short  lot;
    unsigned char   adhesive_run_number,
                    size,
                    apollo_is_pvi,
                    reserved;
};
typedef struct panel_id_t panel_id_t;

struct sync_async_t                                 // 32 bytes
{
    int             initialized;
    unsigned long   onenand_sys_config_1;
    unsigned long   pxa_sxcnfg;
    unsigned long   pxa_mdrefr;
    unsigned long   pxa_msc0;
    unsigned long   reserved[3];
};
typedef struct sync_async_t sync_async_t;

struct hw_flags_t                                   // 4 bytes
{
    unsigned int    disable_pdc_suspend :  1;
    unsigned int    invert_video        :  1;
    unsigned int    reserved            : 30;
};
typedef struct hw_flags_t hw_flags_t;

struct bat_level_t                                  // 16 bytes
{
    int             update_env;
    unsigned short  stage[NUM_BAT_LEVEL_STAGES];
};
typedef struct bat_level_t bat_level_t;

typedef int (*sys_ioctl_t)(unsigned int cmd, unsigned long arg); 

struct pnlcd_flags_t                                // 4 bytes
{
    unsigned short  enable_fake         :  1;
    unsigned short  updating            :  1;
    unsigned short  hide_real           :  1;
    unsigned short  reserved            : 13;
    short           event_count;
};
typedef struct pnlcd_flags_t pnlcd_flags_t;

struct mw_flags_t                                   // 4 bytes
{
    unsigned int    debug               :  1;
    unsigned int    needs_remap         :  1;
    unsigned int    async_deprecated    :  1;
    unsigned int    missing_buttons     :  1;
    unsigned int    legacy_deprecated   :  1;
    unsigned int    blitsrc_debug       :  1;
    unsigned int    blitsrc_original    :  1;
    unsigned int    alphablend_debug    :  1;
    unsigned int    alphablend_original :  1;
    unsigned int    setpixels_debug     :  1;
    unsigned int    setpixels_original  :  1;
    unsigned int    display_upside_down :  1;
    unsigned int    do_dynamic_rotation :  1;
    unsigned int    de_ghost            :  1;
    unsigned int    reserved            : 18;
};
typedef struct mw_flags_t mw_flags_t;

#define INIT_MW_FLAGS_T() { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

struct rcs_flags_t                                  // 4 bytes
{
    unsigned int    system_startup      :  1;
    unsigned int    system_shutdown     :  1;
    unsigned int    video_startup       :  1;
    unsigned int    software_update     :  1;
    unsigned int    reserved            : 28;
};
typedef struct rcs_flags_t rcs_flags_t;

#define INIT_RCS_FLAGS_T() { 0, 0, 0, 0, 0 }

typedef u8 (*fb_apply_fx_t)(u8 data, int i);

struct bg_in_use_t
{
    unsigned long warm_restart_flag;                // BOOT_GLOBALS_BASE+0x0000
    unsigned long apollo_init_display_flag;         // BOOT_GLOBALS_BASE+0x0004
    int           progress_bar_value;               // BOOT_GLOBALS_BASE+0x0008
    unsigned long bootloader_status;                // BOOT_GLOBALS_BASE+0x000C
    unsigned long bootloader_messsage;              // BOOT_GLOBALS_BASE+0x0010
    unsigned long bootblock_version;                // BOOT_GLOBALS_BASE+0x0014
    char          free0[FREE_SIZE_0];               // BOOT_GLOBALS_BASE+0x0018
    unsigned char dirty_boot_flag;                  // BOOT_GLOBALS_BASE+0x002B
    int           user_boot_screen;                 // BOOT_GLOBALS_BASE+0x002C
    int           boot_ioc_key;                     // BOOT_GLOBALS_BASE+0x0030
    int           boot_key;                         // BOOT_GLOBALS_BASE+0x0034
    int           boot_recover;                     // BOOT_GLOBALS_BASE+0x0038
    unsigned long update_flag;                      // BOOT_GLOBALS_BASE+0x003C
    unsigned long update_data;                      // BOOT_GLOBALS_BASE+0x0040
    unsigned long saved_rtc;                        // BOOT_GLOBALS_BASE+0x0044
    unsigned long saved_rtc_checksum;               // BOOT_GLOBALS_BASE+0x0048
    char          scratch[SCRATCH_SIZE];            // BOOT_GLOBALS_BASE+0x004C
    panel_id_t    panel_id;                         // BOOT_GLOBALS_BASE+0x014C
    int           framework_started;                // BOOT_GLOBALS_BASE+0x0154
    char          serial_num[SERIAL_NUM_SIZE];      // BOOT_GLOBALS_BASE+0x0158 (32 bytes)
    sync_async_t  async_settings;                   // BOOT_GLOBALS_BASE+0x0178
    sync_async_t  sync_settings;                    // BOOT_GLOBALS_BASE+0x0198
    int           clear_dbs;                        // BOOT_GLOBALS_BASE+0x01B8
    unsigned long framebuffer_start;                // BOOT_GLOBALS_BASE+0x01BC
    int           drivemode_screen_ready;           // BOOT_GLOBALS_BASE+0x01C0
    hw_flags_t    hw_flags;                         // BOOT_GLOBALS_BASE+0x01C4
    int           framework_running;                // BOOT_GLOBALS_BASE+0x01C8
    int           screen_clear;                     // BOOT_GLOBALS_BASE+0x01CC
    bat_level_t   battery_watermarks;               // BOOT_GLOBALS_BASE+0x01D0
    int           dev_mode;                         // BOOT_GLOBALS_BASE+0x01E0
    int           update_op_amp_offset_env;         // BOOT_GLOBALS_BASE+0x01E4
    char          env_script[ENV_SCRIPT_SIZE];      // BOOT_GLOBALS_BASE+0x01E8
    sys_ioctl_t   fb_ioctl;                         // BOOT_GLOBALS_BASE+0x02E8
    unsigned long fb_init_flag;                     // BOOT_GLOBALS_BASE+0x02EC
    char          build_rev[BUILD_REV_SIZE];        // BOOT_GLOBALS_BASE+0x02F0
    int           op_amp_offset;                    // BOOT_GLOBALS_BASE+0x03EC
    unsigned long rve_value;                        // BOOT_GLOBALS_BASE+0x03F0
    unsigned long gain_value;                       // BOOT_GLOBALS_BASE+0x03F4
    pnlcd_flags_t pnlcd_flags;                      // BOOT_GLOBALS_BASE+0x03F8
    sys_ioctl_t   pnlcd_ioctl;                      // BOOT_GLOBALS_BASE+0x03FC
    mw_flags_t    mw_flags;                         // BOOT_GLOBALS_BASE+0x0400
    rcs_flags_t   rcs_flags;                        // BOOT_GLOBALS_BASE+0x0404
    fb_apply_fx_t fb_apply_fx;                      // BOOT_GLOBALS_BASE+0x0408
    int           framework_stopped;                // BOOT_GLOBALS_BASE+0x040C
    unsigned long kernel_boot_flag;                 // BOOT_GLOBALS_BASE+0x0410
    int           drivemode_online;                 // BOOT_GLOBALS_BASE+0x0414        
};
typedef struct bg_in_use_t bg_in_use_t;

struct boot_globals_t
{
    bg_in_use_t   globals;
    unsigned char reserved[BOOT_GLOBALS_SIZE - sizeof(unsigned long) - sizeof(bg_in_use_t)];
    unsigned long checksum; /* holds inverted crc32 (little endian) */
};
typedef struct boot_globals_t boot_globals_t;

extern boot_globals_t *get_boot_globals(void);

// warm_restart_flag

extern void set_warm_restart_flag(unsigned long warm_restart_flag);
extern unsigned long get_warm_restart_flag(void);

// kernel_boot_flag

extern void set_kernel_boot_flag(unsigned long kernel_boot_flag);
extern unsigned long get_kernel_boot_flag(void);

// apollo_init_display_flag

extern void set_apollo_init_display_flag(unsigned long apollo_init_display_flag);
extern unsigned long get_apollo_init_display_flag(void);

#define set_broadsheet_init_display_flag set_apollo_init_display_flag
#define get_broadsheet_init_display_flag get_apollo_init_display_flag

#define set_eink_init_display_flag set_apollo_init_display_flag
#define get_eink_init_display_flag get_apollo_init_display_flag

// dirty_boot_flag

extern void set_dirty_boot_flag(unsigned char dirty_boot_flag);
extern unsigned long get_dirty_boot_flag(void);

// user_boot_screen

extern void set_user_boot_screen(int user_boot_screen);
extern int get_user_boot_screen(void);

// progress_bar_value

extern void set_progress_bar_value(int progress_bar_value);
extern int get_progress_bar_value(void);

// update_flag

extern void set_update_flag(unsigned long update_flag);
extern unsigned long get_update_flag(void);

// update_data

extern void set_update_data(unsigned long update_data);
extern unsigned long get_update_data(void);

// scratch

extern void set_scratch(char *scratch);
extern char *get_scratch(void);

// panel_id

extern void set_panel_id(panel_id_t *panel_id_t);
extern panel_id_t *get_panel_id(void);

extern void clear_panel_id(void);

extern void set_panel_size(unsigned char size);
extern unsigned char get_panel_size(void);

// async/sync_settings

extern void set_async_settings(sync_async_t *async_settings);
extern sync_async_t *get_async_settings(void);

extern void set_sync_settings(sync_async_t *sync_settings);
extern sync_async_t *get_sync_settings(void);

extern void clear_async_sync_settings(void);

// clear_dbs

extern void set_clear_dbs(int clear_dbs);
extern int get_clear_dbs(void);

// framebuffer_start

extern void set_framebuffer_start(unsigned long framebuffer_start);
extern unsigned long get_framebuffer_start(void);

// drivemode_screen_ready

extern void set_drivemode_screen_ready(int drivemode_screen_ready);
extern int get_drivemode_screen_ready(void);

// hw_flags

extern void set_hw_flags(hw_flags_t *hw_flags);
extern hw_flags_t *get_hw_flags(void);

// framework_started, framework_running, framework_stopped

extern void set_framework_started(int framework_started);
extern int get_framework_started(void);

extern void set_framework_running(int framework_running);
extern int get_framework_running(void);

extern void set_framework_stopped(int framework_stopped);
extern int get_framework_stopped(void);

// screen_clear

extern void set_screen_clear(int screen_clear);
extern int get_screen_clear(void);

// battery_watermarks

extern void set_battery_watermarks(bat_level_t *battery_watermarks);
extern bat_level_t *get_battery_watermarks(void);

// dev_mode

extern void set_dev_mode(int dev_mode);
extern int get_dev_mode(void);

// env_script

extern void set_env_script(char *env_script);
extern char *get_env_script(void);

// fb_ioctl

extern void set_fb_ioctl(sys_ioctl_t fb_ioctl);
extern sys_ioctl_t get_fb_ioctl(void);

// fb_init_flag

extern void set_fb_init_flag(unsigned long fb_init_flag);
extern unsigned long get_fb_init_flag(void);

// op_amp_offset, update_op_amp_offset_env

extern void set_update_op_amp_offset_env(int update_op_amp_offset_env);
extern int get_update_op_amp_offset_env(void);

extern void set_op_amp_offset(int op_amp_offset);
extern int get_op_amp_offset(void);

// rve_value

extern void set_rve_value(unsigned long rve_value);
extern unsigned long get_rve_value(void);

// gain_value

extern void set_gain_value(unsigned long gain_value);
extern unsigned long get_gain_value(void);

// pnlcd_flags

extern void set_pnlcd_flags(pnlcd_flags_t *pnlcd_flags);
extern pnlcd_flags_t *get_pnlcd_flags(void);

extern int inc_pnlcd_event_count(void);
extern int dec_pnlcd_event_count(void);

// pnlcd_ioctl

extern void set_pnlcd_ioctl(sys_ioctl_t pnlcd_ioctl);
extern sys_ioctl_t get_pnlcd_ioctl(void);

// mw_flags

extern void set_mw_flags(mw_flags_t *mw_flags);
extern mw_flags_t *get_mw_flags(void);

// rcs_flags

extern void set_rcs_flags(rcs_flags_t *rcs_flags);
extern rcs_flags_t *get_rcs_flags(void);

// fb_apply_fx

extern void set_fb_apply_fx(fb_apply_fx_t fb_apply_fx);
extern fb_apply_fx_t get_fb_apply_fx(void);

// drivemode_online

extern void set_drivemode_online(int drivemode_online);
extern int get_drivemode_online(void);

#endif  // _BOOT_GLOBALS_H
