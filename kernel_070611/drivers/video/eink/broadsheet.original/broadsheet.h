/*
 *  linux/drivers/video/eink/broadsheet/broadsheet.h --
 *  eInk frame buffer device HAL broadsheet defs
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef _BROADSHEET_H
#define _BROADSHEET_H

#define EX3_ANDROID 1

//#define EP3EP4_EVT1	1
//#define EP1EP2_EVT1	1
//ennic pan_display update mode 
#define     BS_PAN_MODE_GU  1
#define     BS_PAN_MODE_GC  2
#define     BS_PAN_MODE_SM 3


// Miscellaneous Broadsheet Definitions
//
#define BS_DFMT_2BPP            0       // Load Image Data Formats (ld_img, dfmt)
#define BS_DFMT_4BPP            2       //
#define BS_DFMT_8BPP            3       //

#define BS_ROTMODE_0            0       // Rotation is counter clockwise:  0 -> landscape upside down
#define BS_ROTMODE_90           1       //                                 1 -> portrait
#define BS_ROTMODE_180          2       //                                 2 -> landscape
#define BS_ROTMODE_270          3       //                                 3 -> portrait  upside down

#define BS_UPD_MODE_REINIT      0xFFFF  // Resync panel and buffer with flashing update.
#define BS_UPD_MODE_INIT        0       // Waveform Update Modes (upd, mode)
#define BS_UPD_MODES_OLD        0       // Old (Apollo-style) modes:
#define BS_UPD_MODE_MU          1       //  MU (monochrome update)
#define BS_UPD_MODE_GU          2       //  GU (grayscale update)
#define BS_UPD_MODE_GC          3       //  GC (grayscale clear)
#define BS_UPD_MODE_PU          4       //  PU (pen update)
#define BS_UPD_MODES_NEW        1       // New modes:
#define BS_UPD_MODE_DU          1       //  MU/PU -> DU
#define BS_UPD_MODE_GC16        2       //  GC/GU @4bpp -> GC16
#define BS_UPD_MODE_GC4         3       //  GC/GU @2bpp -> GC4

//&*&*&*Modify1:EPD update mode change    
#define UPDATE_GU  0
#define UPDATE_GC  1
#define UPDATE_DU	20
//&*&*&*Modify1:EPD update mode change    

#define BS_SDR_IMG_MSW_REG      0x0312  // Host Raw Memory Access Address bits 16-31
#define BS_SDR_IMG_LSW_REG      0x0310  // Host Raw Memory Access Address bits  0-15

#define BS_REV_CODE_REG         0x0000  // Revision Code Register
#define BS_PRD_CODE_REG         0x0002  // Product Code Register

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS_PRD_CODE             0x004d  // All Broadsheets
#else
#define BS_PRD_CODE             0x0047  // All Broadsheets
#endif
#define BS_REV_ASIC_B00         0x0000  // S1D13521 B00
#define BS_REV_ASIC_B01         0x0100  // S1D13521 B01

#define BS_ASIC_B00()           \
    ((BS_PRD_CODE == BS_CMD_RD_REG(BS_PRD_CODE_REG)) && (BS_REV_ASIC_B00 == BS_CMD_RD_REG(BS_REV_CODE_REG)))
#define BS_ASIC_B01()           \
    ((BS_PRD_CODE == BS_CMD_RD_REG(BS_PRD_CODE_REG)) && (BS_REV_ASIC_B01 == BS_CMD_RD_REG(BS_REV_CODE_REG)))

#define BS_ASIC()               \
    (BS_ASIC_B00() || BS_ASIC_B01())
#define BS_FPGA()               \
    ((BS_PRD_CODE == BS_CMD_RD_REG(BS_PRD_CODE_REG)) && !BS_ASIC())

#define BS_CMD_Q_ITERATE_ALL     (-1)   // For use with bs_iterate_cmd_queue() &...
#define BS_CMD_Q_DEBUG           5      // ...broadsheet_get_recent_commands().

#define BS_BOOTSTRAPPED()       (true == broadsheet_get_bootstrap_state())
#define BS_STILL_READY()        (true == broadsheet_get_ready_state())
#define BS_UPD_REPAIR()         (true == broadsheet_get_upd_repair_state())

#define BS_ROUNDUP32(h)         ((((unsigned long)(h)) + 31) & 0xFFFFFFE0)

#define BS_RDY_TIMEOUT          (HZ * 50)
#define BS_SFM_TIMEOUT          (HZ * 20)
#define BS_PLL_TIMEOUT          (HZ * 1)
#define BS_CMD_TIMEOUT          (HZ / 2)

#define FULL_BRINGUP_CONTROLLER FULL_BRINGUP
#define DONT_BRINGUP_CONTROLLER DONT_BRINGUP

#define FULL_BRINGUP_PANEL      FULL_BRINGUP
#define DONT_BRINGUP_PANEL      DONT_BRINGUP

#define UPD_DATA_RESTORE        true
#define UPD_DATA_NORMAL         false

// Broadsheet  800x600, 6.0-inch Panel Support (AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs60_init/bs60_init.h)
//
// 50.25Hz
//
#define BS60_INIT_HSIZE         800
#define BS60_INIT_VSIZE         600
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS60_INIT_FSLEN         4		//same
#else
#define BS60_INIT_FSLEN         4
#endif
#define BS60_INIT_FBLEN         4
#define BS60_INIT_FELEN         10
#define BS60_INIT_LSLEN         10
#define BS60_INIT_LBLEN         4
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS60_INIT_LELEN 	0x30
#define BS60_INIT_PIXCLKDIV     7
#else
#define BS60_INIT_LELEN         84
#define BS60_INIT_PIXCLKDIV     6
#endif
#define BS60_INIT_SDRV_CFG      (100 | (1 << 8) | (1 << 9))
#define BS60_INIT_GDRV_CFG      0x2
#define BS60_INIT_LUTIDXFMT     (4 | (1 << 7))

#define BS60_MM_800             121
#define BS60_MM_600              91

// Broadsheet 1200x825, 9.7-inch Panel Support (AM300_MMC_IMAGE_X03b/source/broadsheet_soft/bs97_init/bs97_init.h)
//
// 50.15Hz
//
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS97_INIT_HSIZE         1200
#define BS97_INIT_VSIZE         828	
#define BS97_INIT_VSLOP         0

#define BS97_HARD_VSIZE         (BS97_INIT_VSIZE + BS97_INIT_VSLOP)
#define BS97_SOFT_VSIZE         828
#else
#define BS97_INIT_HSIZE         1200
#define BS97_INIT_VSIZE         825
#define BS97_INIT_VSLOP         1

#define BS97_HARD_VSIZE         (BS97_INIT_VSIZE + BS97_INIT_VSLOP)
#define BS97_SOFT_VSIZE         824
#endif

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS97_INIT_FSLEN         4
#else
#define BS97_INIT_FSLEN         0
#endif
#define BS97_INIT_FBLEN         4
#define BS97_INIT_FELEN         4
#define BS97_INIT_LSLEN         4
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS97_INIT_LBLEN         4
#define BS97_INIT_LELEN         0x50
#else
#define BS97_INIT_LBLEN         10
#define BS97_INIT_LELEN         74
#endif
#define BS97_INIT_PIXCLKDIV     3
#define BS97_INIT_SDRV_CFG      (100 | (1 << 8) | (1 << 9))
#define BS97_INIT_GDRV_CFG      0x2
#define BS97_INIT_LUTIDXFMT     (4 | (1 << 7))

#define BS97_MM_1200            203
#define BS97_MM_825             139

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define BS_WFM_ADDR             0x0cd8
#else
#define BS_WFM_ADDR             0x00886     // See AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs60_init/run_bs60_init.sh.
#endif
#define BS_CMD_ADDR             0x00000     // Base of flash holds the commands (0x00000...(BS_WFM_ADDR - 1)).
#define BS_TST_ADDR_128K        0x1E000     // Test area (last 8K of 128K).
#define BS_TST_ADDR_256K        0x3E000     // Test area (last 8K of 256K).

#define BS_INIT_DISPLAY_FAST    0x74736166  // Bring the panel up without going through cycle-back-to-white process.
#define BS_INIT_DISPLAY_SLOW	0x776F6C73  // Bring the panel up by manually cycling it back to white.
#define BS_INIT_DISPLAY_ASIS    0x73697361  // Leave whatever is on the panel alone.

#define BS_INIT_DISPLAY_READ    true        // Read whatever was in the boot globals for display initialization purposes.
#define BS_INIT_DISPLAY_DONT    false       // Don't.

#define BS_DISPLAY_ASIS()       (BS_INIT_DISPLAY_ASIS == get_broadsheet_init_display_flag())

#define BS_WFM_SEL_SDRAM  0x00001  //selects WAVEFORM Storage on SDRAM
#define BS_WFM_SEL_SFM    0x000C0  //selects WAVEFORM Storage on External Serial Flash (Default)

// Broadsheet interrupt registers constants
#define BS_INTR_RAW_STATUS_REG         0x0240
#define BS_INTR_MASKED_STATUS_REG      0x0242
#define BS_INTR_CTL_REG                0x0244
#define BS_DE_INTR_RAW_STATUS_REG      0x033A
#define BS_DE_INTR_MASKED_STATUS_REG   0x033C
#define BS_DE_INTR_ENABLE_REG          0x033E
#define BS_ALL_IRQS                    0x01FB
#define BS_SDRAM_INIT_IRQ              0x01
#define BS_DISPLAY_ENGINE_IRQ          0x01 << 1
#define BS_GPIO_IRQ                    0x01 << 3
#define BS_3_WIRE_IRQ                  0x01 << 4
#define BS_PWR_MGMT_IRQ                0x01 << 5
#define BS_THERMAL_SENSOR_IRQ          0x01 << 6
#define BS_SDRAM_REFRESH_IRQ           0x01 << 7
#define BS_HOST_MEM_FIFO_IRQ           0x01 << 8
#define BS_DE_ALL_IRQS                 0x3FFF
#define BS_DE_OP_TRIG_DONE_IRQ         0x01
#define BS_DE_UPD_BUFF_RFSH_DONE_IRQ   0x01 <<  1
#define BS_DE_DISP1_FC_COMPLT_IRQ      0x01 <<  2
#define BS_DE_ONE_LUT_COMPLT_IRQ       0x01 <<  3
#define BS_DE_UPD_BUFF_CHG_IRQ         0x01 <<  4
#define BS_DE_ALL_FRMS_COMPLT_IRQ      0x01 <<  5
#define BS_DE_FIFO_UNDERFLOW_IRQ       0x01 <<  6
#define BS_DE_LUT_BSY_CONFLICT_IRQ     0x01 <<  7
#define BS_DE_OP_TRIG_ERR_IRQ          0x01 <<  8
#define BS_DE_LUT_REQ_ERR_IRQ          0x01 <<  9
#define BS_DE_TEMP_OUT_OF_RNG_IRQ      0x01 << 10
#define BS_DE_ENTRY_CNT_ERR_IRQ        0x01 << 11
#define BS_DE_FLASH_CHKSM_ERR_IRQ      0x01 << 12
#define BS_DE_BUF_UPD_INCOMPLETE_IRQ   0x01 << 13


/*
#define IS_ADS()	(GET_BOARD_PRODUCT() == 'A')
#define IS_MARIO()	(GET_BOARD_PRODUCT() == 'M')
#define IS_TURING()	(GET_BOARD_PRODUCT() == 'T')
#define IS_NELL()	(GET_BOARD_PRODUCT() == 'N')

#define IS_PROTO()	(GET_BOARD_HW_BUILD() == 'X')
#define IS_EVT()	(GET_BOARD_HW_BUILD() == 'E')
#define IS_DVT()	(GET_BOARD_HW_BUILD() == 'D')
#define IS_PVT()	(GET_BOARD_HW_BUILD() == 'P')

#define GET_BOARD_HW_VERSION() (mx31_board_id[2] - '0')

#define HAS_128M()	(GET_BOARD_MEMORY() == 'A')
#define HAS_256M()	(GET_BOARD_MEMORY() == 'B')

#define GET_BOARD_PRODUCT() (mx31_board_id[0])
#define GET_BOARD_HW_BUILD() (mx31_board_id[1])
#define GET_BOARD_MEMORY() (mx31_board_id[3])
*/

enum bs_flash_select
{
    bs_flash_waveform,
    bs_flash_commands,
    bs_flash_test
};
typedef enum bs_flash_select bs_flash_select;

enum bs_power_states
{
    bs_power_state_off_screen_clear,    // einkfb_power_level_off
    bs_power_state_off,                 // einkfb_power_level_sleep
 
    bs_power_state_run,                 // einkfb_power_level_on
    bs_power_state_standby,
    bs_power_state_sleep,               // einkfb_power_level_standby
    
    bs_power_state_init
};
typedef enum bs_power_states bs_power_states;

enum bs_preflight_failure
{
    bs_preflight_failure_hw   = 1 << 0,  // Hardware isn't responding at all.
    bs_preflight_failure_id   = 1 << 1,  // ID bits aren't right.
    bs_preflight_failure_bus  = 1 << 2,  // Bits on bus aren't toggling correctly.
    bs_preflight_failure_cmd  = 1 << 3,  // Commands area of flash isn't valid.
    bs_preflight_failure_wf   = 1 << 4,  // Waveform area of flash isn't valid.
    bs_preflight_failure_flid = 1 << 5,  // Flash id isn't recognized.
    
    bs_preflight_failure_none = 0
};
typedef enum bs_preflight_failure bs_preflight_failure;

// Broadsheet Primitives Support
//
#define BS_RD_DAT_ONE           1       // One 16-bit read.
#define BS_WR_DAT_ONE           1       // One 16-bit write.

#define BS_WR_DAT_DATA          true    // For use with...
#define BS_WR_DAT_ARGS          false   // ...bs_wr_dat().

// Broadsheet Host Interface Commands per Epson/eInk S1D13521 Hardware
// Fuctional Specification (Rev. 0.06)
//
#define BS_CMD_ARGS_MAX         5

#define BS_CMD_BST_SDR_WR_REG   0x0154  // Host Memory Access Port Register
#define BS_CMD_ARGS_BST_SDR     4

#define BS_CMD_LD_IMG_WR_REG    0x0154  // Host Memory Access Port Register
#define BS_CMD_ARGS_WR_REG_SUB  1

#define BS_CMD_UPD_ARGS         1
#define BS_CMD_UPD_AREA_ARGS    5

#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP3_DVT)  || defined(CONFIG_HW_EP4_DVT)
#error not an ep3!!!
#define XRES                            828	//832
#define YRES                            1200
#else
#define XRES                            600
#define YRES                            800
#endif
enum bs_cmd
{
    // System Commands
    //
    bs_cmd_INIT_CMD_SET         = 0x0000,   BS_CMD_ARGS_INIT_CMD_SET    = 3,
    bs_cmd_INIT_PLL_STBY        = 0x0001,   BS_CMD_ARGS_INIT_PLL_STBY   = 3,
    bs_cmd_RUN_SYS              = 0x0002,
    bs_cmd_STBY                 = 0x0004,
    bs_cmd_SLP                  = 0x0005,
    bs_cmd_INIT_SYS_RUN         = 0x0006,
    bs_cmd_INIT_SYS_STBY        = 0x0007,
    bs_cmd_INIT_SDRAM           = 0x0008,   BS_CMD_ARGS_INIT_SDRAM      = 4,
    bs_cmd_INIT_DSPE_CFG        = 0x0009,   BS_CMD_ARGS_DSPE_CFG        = 5,
    bs_cmd_INIT_DSPE_TMG        = 0x000A,   BS_CMD_ARGS_DSPE_TMG        = 5,
    bs_cmd_SET_ROTMODE          = 0x000B,   BS_CMD_ARGS_SET_ROTMODE     = 1,

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    bs_cmd_INIT_WAVEDEV          = 0x000C,   BS_CMD_ARGS_INIT_WAVEDEV     = 1,
#endif

    // Register and Memory Access Commands
    //
    bs_cmd_RD_REG               = 0x0010,   BS_CMD_ARGS_RD_REG          = 1,
    bs_cmd_WR_REG               = 0x0011,   BS_CMD_ARGS_WR_REG          = 2,
    bs_cmd_RD_SFM               = 0x0012,
    bs_cmd_WR_SFM               = 0x0013,   BS_CMD_ARGS_WR_SFM          = 1,
    bs_cmd_END_SFM              = 0x0014,
    
    // Burst Access Commands
    //
    bs_cmd_BST_RD_SDR           = 0x001C,   BS_CMD_ARGS_BST_RD_SDR      = BS_CMD_ARGS_BST_SDR,
    bs_cmd_BST_WR_SDR           = 0x001D,   BS_CMD_ARGS_BST_WR_SDR      = BS_CMD_ARGS_BST_SDR,
    bs_cmd_BST_END              = 0x001E,

    // Image Loading Commands
    //
    bs_cmd_LD_IMG               = 0x0020,   BS_CMD_ARGS_LD_IMG          = 1,
    bs_cmd_LD_IMG_AREA          = 0x0022,   BS_CMD_ARGS_LD_IMG_AREA     = 5,
    bs_cmd_LD_IMG_END           = 0x0023,
    bs_cmd_LD_IMG_WAIT          = 0x0024,
    bs_cmd_LD_IMG_SETADR        = 0x0025,   BS_CMD_ARGS_LD_IMG_SETADR   = 2,
    bs_cmd_LD_IMG_DSPEADR       = 0x0026,
    
    // Polling Commands
    //
    bs_cmd_WAIT_DSPE_TRG        = 0x0028,
    bs_cmd_WAIT_DSPE_FREND      = 0x0029,
    bs_cmd_WAIT_DSPE_LUTFREE    = 0x002A,
    bs_cmd_WAIT_DSPE_MLUTFREE   = 0x002B,   BS_CMD_ARGS_DSPE_MLUTFREE   = 1,
    
    // Waveform Update Commands
    //
    bs_cmd_RD_WFM_INFO          = 0x0030,   BS_CMD_ARGS_RD_WFM_INFO     = 2,
    bs_cmd_UPD_INIT             = 0x0032,
    bs_cmd_UPD_FULL             = 0x0033,   BS_CMD_ARGS_UPD_FULL        = BS_CMD_UPD_ARGS,
    bs_cmd_UPD_FULL_AREA        = 0x0034,   BS_CMD_ARGS_UPD_FULL_AREA   = BS_CMD_UPD_AREA_ARGS,
    bs_cmd_UPD_PART             = 0x0035,   BS_CMD_ARGS_UPD_PART        = BS_CMD_UPD_ARGS,
    bs_cmd_UPD_PART_AREA        = 0x0036,   BS_CMD_ARGS_UPD_PART_AREA   = BS_CMD_UPD_AREA_ARGS,
    bs_cmd_UPD_GDRV_CLR         = 0x0037,
    bs_cmd_UPD_SET_IMGADR       = 0x0038,   BS_CMD_ARGS_UPD_SET_IMGADR  = 2
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    ,
    bs_cmd_PEN_DRAW          = 0x003a,   BS_CMD_ARGS_PEN_DRAW     = 5,
    bs_cmd_PEN_MENU           = 0x003b,   BS_CMD_ARGS_PEN_MENU     = 1,
    bs_cmd_PEN_LINE             = 0x003c,   BS_CMD_ARGS_PEN_LINE     = 5    
#endif
    
};
typedef enum bs_cmd bs_cmd;

enum bs_cmd_type
{
    bs_cmd_type_write,
    bs_cmd_type_read
};
typedef enum bs_cmd_type bs_cmd_type;


#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
//EPSON IC - CURSOR Setup
#define BS_CSR_TRANSPARENCY_ENABLE   0x0100
#define BS_CSR_TRANSPARENCY_DISABLE  0x0000
#define BS_CSR_TRANSPARENCY_KEYVAL   0xF000
#define BS_CSR_PIXOUTCTRL_REPLACE    0x0000
#define BS_CSR_PIXOUTCTRL_XOR        0x0040
#define BS_CSR_PIXOUTCTRL_XORNOT     0x0080
#define BS_CSR_PIXOUTCTRL_INVERT     0x00c0
#define BS_CSR_ENABLE                0x0001
#define BS_CSR_DISABLE               0x0000

//EPSON IC - PIP Setup
//EPSON IC - CURSOR Setup
#define BS_PIP_TRANSPARENCY_ENABLE   0x0100
#define BS_PIP_TRANSPARENCY_DISABLE  0x0000
#define BS_PIP_TRANSPARENCY_KEYVAL   0xF000
#define BS_PIP_PIXOUTCTRL_REPLACE    0x0000
#define BS_PIP_PIXOUTCTRL_XOR        0x0040
#define BS_PIP_PIXOUTCTRL_XORNOT     0x0080
#define BS_PIP_PIXOUTCTRL_INVERT     0x00c0


//EPSON IC - Host
#define BS_HST_DEST_PIPSEL           0x0400
#define BS_HST_DEST_MAINSEL          0x0000
#define BS_HST_4BPPMATCH_ENABLE      0x0040
#define BS_HST_4BPPMATCH_DISABLE     0x0000
#define BS_HST_TRANSPARENCY_ENABLE   0x0008
#define BS_HST_TRANSPARENCY_DISABLE  0x0000


//EPSON IC - Auto Waveform Setup
#define BS_AUTOWAV_ON                0x0001
#define BS_AUTOWAV_OFF               0x0000
#define BS_AUTOWAV_ZEROLVLON         0x0001
#define BS_AUTOWAV_ZEROLVLOFF        0x0000
#define BS_AUTOWAV_PIXMSKON          0x000F

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT) 
//EPSON IC - Pen Drawing Calibration Offset
#define PenDraw_CalibratePosOffsetX 120
#define PenDraw_CalibratePosOffsetY 80
#elif defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define PenDraw_CalibratePosOffsetX 80
#define PenDraw_CalibratePosOffsetY 60
#endif
#endif

struct bs_cmd_block_t
{
    bs_cmd          command;               // bs_cmd_XXXX
    bs_cmd_type     type;                  // read/write
    
    u32             num_args;              // [io = ]command(args[0], ..., args[BS_CMD_ARGS_MAX - 1])
    u16             args[BS_CMD_ARGS_MAX], //
                    io;                    // 

    u32             data_size;             // data[0..data_size-1]
    u8              *data;                 //
    
    struct
    bs_cmd_block_t  *sub;                  // subcommand(args[0], ..., args[BS_CMD_ARGS_MAX - 1])
};
typedef struct bs_cmd_block_t bs_cmd_block_t;

struct bs_cmd_queue_elem_t
{
    bs_cmd_block_t  bs_cmd_block;
    unsigned long   time_stamp;
};
typedef struct bs_cmd_queue_elem_t bs_cmd_queue_elem_t;

typedef void (*bs_cmd_queue_iterator_t)(bs_cmd_queue_elem_t *bs_cmd_queue_elem);

struct bs_resolution_t
{
    u32 x_hw, x_sw, x_mm,
        y_hw, y_sw, y_mm;
};
typedef struct bs_resolution_t bs_resolution_t;

// Broadsheet Host Interface Commands API (AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs_cmd/bs_cmd.h)
//
// System Commands
//
extern void bs_cmd_init_cmd_set(u16 arg0, u16 arg1, u16 arg2);
extern void bs_cmd_init_pll_stby(u16 cfg0, u16 cfg1, u16 cfg2);
extern void bs_cmd_run_sys(void);
extern void bs_cmd_stby(void);
extern void bs_cmd_slp(void);
extern void bs_cmd_init_sys_run(void);
extern void bs_cmd_init_sys_stby(void);
extern void bs_cmd_init_sdram(u16 cfg0, u16 cfg1, u16 cfg2, u16 cfg3);
extern void bs_cmd_init_dspe_cfg(u16 hsize, u16 vsize, u16 sdcfg, u16 gfcfg, u16 lutidxfmt);
extern void bs_cmd_init_dspe_tmg(u16 fs, u16 fbe, u16 ls, u16 lbe, u16 pixclkcfg);
extern void bs_cmd_set_rotmode(u16 rotmode);

// Register and Memory Access Commands
//
extern u16  bs_cmd_rd_reg(u16 ra);
extern void bs_cmd_wr_reg(u16 ra, u16 wd);
extern void bs_cmd_rd_sfm(void);
extern void bs_cmd_wr_sfm(u8 wd);
extern void bs_cmd_end_sfm(void);

extern u16  BS_CMD_RD_REG(u16 ra);          // Validates ra.
extern void BS_CMD_WR_REG(u16 ra, u16 wd);  // Validates ra.

// Burst Access Commands
//
extern void bs_cmd_bst_rd_sdr(u32 ma, u32 bc, u8 *data);
extern void bs_cmd_bst_wr_sdr(u32 ma, u32 bc, u8 *data);
extern void bs_cmd_bst_end(void);

// Image Loading Commands
//
extern void bs_cmd_ld_img(u16 dfmt);
extern void bs_cmd_ld_img_area(u16 dfmt, u16 x, u16 y, u16 w, u16 h);
extern void bs_cmd_ld_img_end(void);
extern void bs_cmd_ld_img_wait(void);

// Polling Commands
//
extern void bs_cmd_wait_dspe_trg(void);
extern void bs_cmd_wait_dspe_frend(void);
extern void bs_cmd_wait_dspe_lutfree(void);
extern void bs_cmd_wait_dspe_mlutfree(u16 lutmsk);

// Waveform Update Commands
//
extern void bs_cmd_rd_wfm_info(u32 ma);
extern void bs_cmd_upd_init(void);
extern void bs_cmd_upd_full(u16 mode, u16 lutn, u16 bdrupd);
extern void bs_cmd_upd_full_area(u16 mode, u16 lutn, u16 bdrupd, u16 x, u16 y, u16 w, u16 h);
extern void bs_cmd_upd_part(u16 mode, u16 lutn, u16 bdrupd);
extern void bs_cmd_upd_part_area(u16 mode, u16 lutn, u16 bdrupd, u16 x, u16 y, u16 w, u16 h);
extern void bs_cmd_upd_gdrv_clr(void);
extern void bs_cmd_upd_set_imgadr(u32 ma);

// SPI Flash Interface API (AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs_sfm.h)
//
extern bool bs_sfm_preflight(void);
extern int bs_get_sfm_size(void);

extern void bs_sfm_start(void);
extern void bs_sfm_end(void);

extern void bs_sfm_wr_byte(int data);
extern int  bs_sfm_rd_byte(void);
extern int  bs_sfm_esig(void);

extern void bs_sfm_read(int addr, int size, char *data);
extern void bs_sfm_write(int addr, int size, char *data);

// Broadsheet Host Interface Helper API (AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs_cmd/bs_cmd.h)
//
extern void bs_cmd_wait_for_bit(int reg, int bitpos, int bitval);
extern void bs_cmd_print_disp_timings(void);

extern void bs_cmd_set_wfm(int addr);
extern void bs_cmd_get_wfm_info(void);
extern void bs_cmd_print_wfm_info(void);
extern void bs_cmd_clear_gd(void);

extern void bs_cmd_rd_sdr(u32 ma, u32 bc, u8 *data);
extern void bs_cmd_wr_sdr(u32 ma, u32 bc, u8 *data);

extern int  bs_cmd_get_lut_auto_sel_mode(void);
extern void bs_cmd_set_lut_auto_sel_mode(int v);

// 
//
extern u32  bs_cmd_get_sdr_img_base(void);

extern void BS_CMD_LD_IMG_UPD_DATA(fx_type update_mode);
extern void bs_cmd_ld_img_upd_data(fx_type update_mode, bool restore);
extern void bs_cmd_ld_img_area_upd_data(u8 *data, fx_type update_mode, u16 x, u16 y, u16 w, u16 h);

extern void bs_cmd_upd_repair(void);

extern unsigned long *bs_get_img_timings(int *num_timings);

// Broadsheet  800x600, 6.0-inch Panel API (AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs60_init/bs60_init.h)
//
extern void bs60_init(int wfmaddr, bool full);

extern void bs60_ld_value(u8 v);
extern void bs60_flash(void);
extern void bs60_white(void );
extern void bs60_black(void);

// Broadsheet 1200x825, 9.7-inch Panel API (AM300_MMC_IMAGE_X03b/source/broadsheet_soft/bs97_init/bs97_init.h)
//
extern void bs97_init(int wfmaddr, bool full);

extern void bs97_ld_value(u8 v);
extern void bs97_flash(void);
extern void bs97_white(void);
extern void bs97_black(void);

// Broadsheet API (broadsheet.c)
//
extern void bs_sw_init_controller(bool full, bool orientation, bs_resolution_t *res);
extern bool bs_sw_init_panel(bool full);

extern bool bs_sw_init(bool controller_full, bool panel_full);
extern void bs_sw_done(void);

extern bs_preflight_failure bs_get_preflight_failure(void);
extern bool bs_preflight_passes(void);

extern bool broadsheet_get_bootstrap_state(void);
extern void broadsheet_set_bootstrap_state(bool bootstrap_state);

extern bool broadsheet_get_ready_state(void);
extern bool broadsheet_get_upd_repair_state(void);

extern void broadsheet_clear_screen(fx_type update_mode);
extern bs_power_states broadsheet_get_power_state(void);
extern void broadsheet_set_power_state(bs_power_states power_state);

extern void bs_iterate_cmd_queue(bs_cmd_queue_iterator_t bs_cmd_queue_iterator, int max_elems);
extern int bs_read_temperature(void);

// Broadsheet HW Primitives (broadsheet_mxc.c)
//
extern int  bs_wr_cmd(bs_cmd cmd, bool poll);
extern int  bs_wr_dat(bool which, u32 data_size, u16 *data);
extern int  bs_rd_dat(u32 data_size, u16 *data);

extern bool bs_wr_one_ready(void);
extern void bs_wr_one(u16 data);
extern void ep3_bs_wr_one(u16 data);

extern bool bs_hw_init(void); // Similar to bsc.init_gpio() from bs_chip.cpp.
extern bool bs_hw_test(void); // Similar to bsc.test_gpio() from bs_chip.cpp.

extern void bs_hw_done(void);

// Broadsheet eInk HAL API (broadsheet_hal.c)
//
extern unsigned char *broadsheet_get_scratchfb(void);
extern unsigned long  broadsheet_get_scratchfb_size(void);

void broadsheet_prime_watchdog_timer(bool delay_timer);

extern bs_flash_select broadsheet_get_flash_select(void);
extern void broadsheet_set_flash_select(bs_flash_select flash_select);

extern int  broadsheet_get_ram_select_size(void);
extern int  broadsheet_get_ram_select(void);
extern void broadsheet_set_ram_select(int ram_select);

extern bool broadsheet_get_orientation(void);
extern bool broadsheet_get_upside_down(void);
extern int  broadsheet_get_override_upd_mode(void);

extern bool broadsheet_ignore_hw_ready(void);
extern void broadsheet_set_ignore_hw_ready(bool value);
extern bool broadsheet_force_hw_not_ready(void);

extern int broadsheet_get_recent_commands(char *page, int max_commands);

// Broadsheet Read/Write SDRAM API (broadsheet.c)
//
extern int broadsheet_read_from_ram(unsigned long addr, unsigned char *data, unsigned long size);
extern int broadsheet_read_from_ram_byte(unsigned long addr, unsigned char *data);
extern int broadsheet_read_from_ram_short(unsigned long addr, unsigned short *data);
extern int broadsheet_read_from_ram_long(unsigned long addr, unsigned long *data);

extern int broadsheet_program_ram(unsigned long start_addr, unsigned char *buffer, unsigned long blen);

// Broadsheet Waveform/Commands Flashing API (broadsheet.c, broadsheet_waveform.c, broadsheet_commands.c)
//
#include "broadsheet_waveform.h"
#include "broadsheet_commands.h"

extern int broadsheet_read_from_flash(unsigned long addr, unsigned char *data, unsigned long size);
extern int broadsheet_read_from_flash_byte(unsigned long addr, unsigned char *data);
extern int broadsheet_read_from_flash_short(unsigned long addr, unsigned short *data);
extern int broadsheet_read_from_flash_long(unsigned long addr, unsigned long *data);

extern int broadsheet_program_flash(unsigned long start_addr, unsigned char *buffer, unsigned long blen);

// Like the waveform and commands, we have display initialization flag we keep in flash.
//
extern unsigned long broadsheet_get_init_display_flag(void);
extern void broadsheet_set_init_display_flag(unsigned long init_display_flag);

extern void broadsheet_preflight_init_display_flag(bool read);

//ennic added  for add interface to control sd13521
void ex3_bs_cmd_ld_img_area_upd_data(char *data,fx_type update_mode, u16 bdrupd, u16  x, u16 y, u16 w, u16 h);
//extern void broadsheet_mxc_pan_display(bool read);
extern void epd_send_command(unsigned short command, unsigned short data);
extern void epd_write_command(unsigned short command);
extern void epd_write_epson_reg(unsigned short command, unsigned short addr, unsigned short data);
extern void epd_write_buffer(u32 data_size, u16 *data);
extern	unsigned short epd_read_epson_reg(unsigned short command, unsigned short addr);
extern void epd_write_data(unsigned short data);
extern unsigned short epd_read_epson_reg(unsigned short command, unsigned short addr);
extern void epd_send_command_du(unsigned short command, unsigned short data);
extern void epd_write_command_du(unsigned short command);
extern void epd_write_data_du(unsigned short data);
#endif // _BROADSHEET_H
