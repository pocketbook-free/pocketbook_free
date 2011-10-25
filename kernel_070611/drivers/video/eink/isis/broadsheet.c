/*
 *  linux/drivers/video/eink/broadsheet/broadsheet.c
 *  -- eInk frame buffer device HAL broadsheet sw
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include "../hal/einkfb_hal.h"
#include "broadsheet.h"

#if PRAGMAS
    #pragma mark Definitions/Globals
#endif

#define INIT_PWR_SAVE_MODE      0x0000

#define INIT_PLL_CFG_0_FPGA     0x0000
#define INIT_PLL_CFG_1_FPGA     0x0000
#define INIT_PLL_CFG_0_ASIC     0x0004
#define INIT_PLL_CFG_1_ASIC     0x5949
#define INIT_PLL_CFG_2          0x0040
#define INIT_CLK_CFG            0x0000

#define INIT_SPI_FLASH_ACC_MODE 0       // access mode select
#define INIT_SPI_FLASH_RDC_MODE 0       // read command select
#define INIT_SPI_FLASH_CLK_DIV  3       // clock divider
#define INIT_SPI_FLASH_CLK_PHS  0       // clock phase select
#define INIT_SPI_FLASH_CLK_POL  0       // clock polarity select
#define INIT_SPI_FLASH_ENB      1       // enable
#define INIT_SPI_FLASH_CTL              \
    ((INIT_SPI_FLASH_ACC_MODE << 7) |   \
     (INIT_SPI_FLASH_RDC_MODE << 6) |   \
     (INIT_SPI_FLASH_CLK_DIV  << 3) |   \
     (INIT_SPI_FLASH_CLK_PHS  << 2) |   \
     (INIT_SPI_FLASH_CLK_POL  << 1) |   \
     (INIT_SPI_FLASH_ENB      << 0))
     
#define INIT_SPI_FLASH_CS_ENB   1
#define INIT_SPI_FLASH_CSC      INIT_SPI_FLASH_CS_ENB

#define SFM_READ_COMMAND        0
#define SFM_CLOCK_DIVIDE        3
#define SFM_CLOCK_PHASE         0
#define SFM_CLOCK_POLARITY      0

#define BS_SFM_WREN             0x06
#define BS_SFM_WRDI             0x04
#define BS_SFM_RDSR             0x05
#define BS_SFM_READ             0x03
#define BS_SFM_PP               0x02
#define BS_SFM_SE               0xD8
#define BS_SFM_RES              0xAB
#define BS_SFM_ESIG_M25P10      0x10    // 128K
#define BS_SFM_ESIG_M25P20      0x11    // 256K (MX25L2005 returns the same esig)

#define BS_SFM_SECTOR_COUNT     4
#define BS_SFM_SECTOR_SIZE_128K (32*1024)
#define BS_SFM_SECTOR_SIZE_256K (64*1024)
#define BS_SFM_SIZE_128K        (BS_SFM_SECTOR_SIZE_128K * BS_SFM_SECTOR_COUNT)
#define BS_SFM_SIZE_256K        (BS_SFM_SECTOR_SIZE_256K * BS_SFM_SECTOR_COUNT)
#define BS_SFM_PAGE_SIZE        256
#define BS_SFM_PAGE_COUNT_128K  (BS_SFM_SECTOR_SIZE_128K/BS_SFM_PAGE_SIZE)
#define BS_SFM_PAGE_COUNT_256K  (BS_SFM_SECTOR_SIZE_256K/BS_SFM_PAGE_SIZE)

#define BS_SFM_MAX_RETRY_COUNT  3

#define BS_SDR_SIZE             (16*1024*1024)
#define BS_SDR_SIZE_ISIS        ( 2*1024*1024)

#define BS_NUM_CMD_QUEUE_ELEMS  101 // n+1 -> n cmds + 1 for empty
#define BS_RECENT_CMDS_SIZE     (32*1024)

static int bs_sfm_sector_size = BS_SFM_SECTOR_SIZE_128K;
static int bs_sfm_size = BS_SFM_SIZE_128K;
static int bs_sfm_page_count = BS_SFM_PAGE_COUNT_128K;
static int bs_tst_addr = BS_TST_ADDR_128K;
static char sd[BS_SFM_SIZE_256K];
static char *rd = &sd[0];
static int sfm_cd;

static u16 bs_prd_code = BS_REV_CODE_UNKNOWN;
static u16 bs_rev_code = BS_PRD_CODE_UNKNOWN;
static u16 bs_upd_mode = BS_UPD_MODE_INIT;
static int bs_sdr_size = BS_SDR_SIZE;

static int wfm_fvsn = 0;
static int wfm_luts = 0;
static int wfm_mc   = 0;
static int wfm_trc  = 0;
static int wfm_eb   = 0;
static int wfm_sb   = 0;
static int wfm_wmta = 0;

static int bs_hsize = 0;
static int bs_vsize = 0;

static bs_cmd_queue_elem_t bs_cmd_queue[BS_NUM_CMD_QUEUE_ELEMS];
static bs_cmd_queue_elem_t bs_cmd_elem;

static int bs_cmd_queue_entry = 0;
static int bs_cmd_queue_exit  = 0;

static char bs_recent_commands_page[BS_RECENT_CMDS_SIZE];
static bool bs_pll_steady = false;
static bool bs_ready = false;
static bool bs_hw_up = false;

static bs_preflight_failure preflight_failure = bs_preflight_failure_none;
static int bs_bootstrap = 0;

static bool bs_upd_repair_skipped = false;
static int  bs_upd_repair_count = 0;
static int  bs_upd_repair_mode;
static u16  bs_upd_repair_x1;
static u16  bs_upd_repair_y1;
static u16  bs_upd_repair_x2;
static u16  bs_upd_repair_y2;

#ifdef MODULE
module_param_named(bs_bootstrap, bs_bootstrap, int, S_IRUGO);
MODULE_PARM_DESC(bs_bootstrap, "non-zero to bootstrap");
#endif // MODULE

static bs_power_states bs_power_state = bs_power_state_init;
static u8 *bs_ld_fb = NULL;

static u16 bs_upd_modes_old[] = 
{
    BS_UPD_MODE_INIT,
    BS_UPD_MODE_MU,
    BS_UPD_MODE_GU,
    BS_UPD_MODE_GC,
    BS_UPD_MODE_PU
};

static u16 bs_upd_modes_new_2bpp[] =
{
    BS_UPD_MODE_INIT,
    BS_UPD_MODE_DU,
    BS_UPD_MODE_GC4,
    BS_UPD_MODE_GC4,
    BS_UPD_MODE_DU
};

static u16 bs_upd_modes_new_4bpp[] =
{
    BS_UPD_MODE_INIT,
    BS_UPD_MODE_DU,
    BS_UPD_MODE_GC4,
    BS_UPD_MODE_GC16,
    BS_UPD_MODE_DU
};

static u16 *bs_upd_modes = NULL;

static u16 bs_regs[] = 
{
    // System Configuration Registers
    //
    0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000A,
    
    // Clock Configuration Registers
    //
    0x0010, 0x0012, 0x0014, 0x0016, 0x0018, 0x001A,
    
    // Component Configuration
    //
    0x0020,
    
    // Memory Controller Configuration
    //
    0x0100, 0x0102, 0x0104, 0x0106, 0x0108, 0x010A,
    0x010C,
    
    // Host Interface Memory Access Configuration
    //
    0x0140, 0x0142, 0x0144, 0x0146, 0x0148, 0x014A,
    0x014C, 0x014E, 0x0150, 0x0152, 0x0154, 0x0156,
    
    // Misc. Configuration Registers
    //
    0x015E,
    
    // SPI Flash Memory Interface
    //
    0x0200, 0x0202, 0x0204, 0x0206, 0x0208,
    
    // I2C Thermal Sensor Interface Registers
    //
    0x0210, 0x0212, 0x0214, 0x0216,
    
    // 3-Wire Chip Interface Registers
    //
    0x0220, 0x0222, 0x0224, 0x0226,
    
    // Power Pin Control Configuration
    //
    0x0230, 0x0232, 0x0234, 0x0236, 0x0238,
    
    // Interrupt Configuration Register
    //
    0x0240, 0x0242, 0x0244,
    
    // GPIO Control Registers
    //
    0x0250, 0x0252, 0x0254, 0x0256,
    
    // Waveform Read Control Register (ISIS)
    //
    0x260, 
    
    // Command RAM Controller Registers
    //
    0x0290, 0x0292, 0x0294,
    
    // Command Sequence Controller Registers
    //
    0x02A0, 0x02A2,
    
    // Display Engine:  Display Timing Configuration
    // 
    0x0300, 0x0302, 0x0304, 0x0306, 0x0308, 0x30A,
    
    // Display Engine:  Driver Configurations
    //
    0x030C, 0x30E,
    
    // Display Engine:  Memory Region Configuration Registers
    //
    0x0310, 0x0312, 0x0314, 0x0316,
    
    // Display Engine:  Component Control
    //
    0x0320, 0x0322, 0x0324, 0x0326, 0x0328, 0x032A,
    0x032C, 0x032E,
    
    // Display Engine:  Control/Trigger Registers
    //
    0x0330, 0x0332, 0x0334,
    
    // Display Engine:  Update Buffer Status Registers
    //
    0x0336, 0x0338,
    
    // Display Engine:  Interrupt Registers
    //
    0x033A, 0x033C, 0x033E,
    
    // Display Engine:  Partial Update Configuration Registers
    //
    0x0340, 0x0342, 0x0344, 0x0346, 0x0348, 0x034A,
    0x034C, 0x034E,
    
    // Display Engine:  Waveform Registers (Broadsheet = 0x035x, ISIS = 0x039x)
    //
    0x0350, 0x0352, 0x0354, 0x0356, 0x0358, 0x035A,
    0x0390, 0x0392, 0x0394, 0x0396, 0x0398, 0x039A,
    
    // Display Engine:  Auto Waveform Mode Configuration Registers
    //
    0x03A0, 0x03A2, 0x03A4, 0x03A6
};

static bs_cmd bs_poll_cmds[] =
{
    bs_cmd_RUN_SYS,
    bs_cmd_STBY,
    bs_cmd_SLP,
    bs_cmd_INIT_SYS_RUN,
    
    bs_cmd_BST_RD_SDR,
    bs_cmd_BST_WR_SDR
};

static unsigned long bs_set_ld_img_start;
static unsigned long bs_ld_img_start;
static unsigned long bs_upd_data_start;

static unsigned long bs_image_start_time;
static unsigned long bs_image_processing_time;
static unsigned long bs_image_loading_time;
static unsigned long bs_image_display_time;
static unsigned long bs_image_stop_time;

#define BS_IMAGE_TIMING_START   0
#define BS_IMAGE_TIMING_PROC    1
#define BS_IMAGE_TIMING_LOAD    2
#define BS_IMAGE_TIMING_DISP    3
#define BS_IMAGE_TIMING_STOP    4
#define BS_NUM_IMAGE_TIMINGS    (BS_IMAGE_TIMING_STOP + 1)
static unsigned long bs_image_timings[BS_NUM_IMAGE_TIMINGS];

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

#define UM02   0x00
#define UM04   0x04
#define UM16   0x06

static u8 bs_4bpp_upd_mode_table[256] = 
{
    UM02, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM02, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, UM16, 
    UM02, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM04, UM16, UM16, UM16, UM16, UM02 
};

#define BS_INIT_UPD_MODE()          UM02
#define BS_TEST_UPD_MODE(m, b)      ((EINKFB_2BPP == (b)) ? (UM04 == (m)) : (UM16 == (m)))
#define BS_FIND_UPD_MODE(m, p, b)   (BS_TEST_UPD_MODE(m, b) ? (m) : ((m) | bs_4bpp_upd_mode_table[(p)]))
#define BS_DONE_UPD_MODE(m)                         \
    ((UM02 == (m)) ? BS_UPD_MODE(BS_UPD_MODE_MU) :  \
     (UM04 == (m)) ? BS_UPD_MODE(BS_UPD_MODE_GU) :  BS_UPD_MODE(BS_UPD_MODE_GC))
    
static u8 isis_commands[] =
{
  0x00, 0x00, 0x00, 0x01, 0xff, 0x03, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00,
  0x00, 0x00, 0x16, 0x00, 0x42, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xba, 0xa0, 0xc6, 0xa0, 0xd0, 0x20, 0xd8, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x20, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xda, 0x80, 0xe2, 0x40, 0xe6, 0x40, 0xea, 0x80, 0xf2, 0x40, 0xf6, 0x20,
  0x00, 0x00, 0xf8, 0x80, 0x23, 0x81, 0x4e, 0x01, 0x00, 0x00, 0x5e, 0x21,
  0x00, 0x00, 0x9a, 0xa1, 0xb8, 0x01, 0xc4, 0x01, 0xc7, 0x41, 0xcf, 0x01,
  0x00, 0xe0, 0xd3, 0x01, 0xd6, 0x01, 0xd9, 0x01, 0xdc, 0x21, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x41, 0x00, 0x00, 0xef, 0x01,
  0xf1, 0x21, 0xf5, 0xa1, 0x01, 0x22, 0x05, 0xa2, 0x11, 0x02, 0x13, 0x42,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0x70, 0x01, 0x00, 0x16, 0x70, 0x01, 0x00, 0x0a, 0x10,
  0x01, 0x00, 0x01, 0x00, 0x16, 0x70, 0x00, 0x00, 0x06, 0x70, 0x00, 0x00,
  0x30, 0x72, 0x01, 0x00, 0x04, 0x71, 0x02, 0x00, 0x0a, 0x10, 0x00, 0x00,
  0x00, 0x42, 0x0a, 0x30, 0x0a, 0x98, 0x00, 0x04, 0x00, 0x0c, 0x0a, 0x10,
  0x00, 0x00, 0xdc, 0xc0, 0x0a, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x2c, 0x00,
  0x0a, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x2e, 0x00, 0x04, 0x71, 0x01, 0x00,
  0x0a, 0x10, 0x00, 0x02, 0x00, 0x02, 0x0a, 0x30, 0x0a, 0x90, 0x00, 0x08,
  0x00, 0x0c, 0x16, 0x70, 0x00, 0x00, 0x06, 0x78, 0x01, 0x00, 0x06, 0x70,
  0x01, 0x00, 0x16, 0x70, 0x01, 0x00, 0x0a, 0x10, 0x01, 0x00, 0x01, 0x00,
  0x16, 0x70, 0x00, 0x00, 0x06, 0x70, 0x00, 0x00, 0x30, 0x72, 0x01, 0x00,
  0x0a, 0x10, 0x00, 0x00, 0x00, 0x40, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x26, 0x00, 0x0a, 0x10, 0x00, 0x00, 0xdc, 0xc0, 0x0a, 0x00, 0x00, 0x08,
  0x00, 0x0c, 0x4d, 0x00, 0x0a, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x60, 0x00,
  0x06, 0x70, 0x00, 0x00, 0x30, 0x72, 0x02, 0x00, 0x04, 0x71, 0x01, 0x00,
  0x0a, 0x10, 0x00, 0x02, 0x00, 0x42, 0x0a, 0x30, 0x0a, 0x90, 0x00, 0x0c,
  0x00, 0x0c, 0x06, 0x70, 0x01, 0x00, 0x16, 0x70, 0x01, 0x00, 0x16, 0x70,
  0x03, 0x00, 0x0a, 0x18, 0x00, 0x00, 0x01, 0x00, 0x06, 0x70, 0x01, 0x00,
  0x10, 0x70, 0x04, 0x00, 0x12, 0x70, 0x49, 0x59, 0x14, 0x70, 0x40, 0x00,
  0x16, 0x70, 0x01, 0x00, 0x0a, 0x10, 0x01, 0x00, 0x01, 0x00, 0x16, 0x70,
  0x00, 0x00, 0x06, 0x70, 0x00, 0x00, 0x00, 0x71, 0xf1, 0x71, 0x06, 0x71,
  0x03, 0x02, 0x08, 0x71, 0x80, 0x00, 0x0a, 0x71, 0x00, 0x00, 0x18, 0x70,
  0x06, 0x00, 0x00, 0x73, 0x58, 0x02, 0x02, 0x73, 0x04, 0x00, 0x04, 0x73,
  0x04, 0x0a, 0x06, 0x73, 0x20, 0x03, 0x08, 0x73, 0x0a, 0x00, 0x0a, 0x73,
  0x16, 0x4a, 0x0c, 0x73, 0x64, 0x03, 0x0e, 0x73, 0x02, 0x00, 0x34, 0x72,
  0x00, 0x00, 0x36, 0x72, 0x00, 0x00, 0x38, 0x72, 0x00, 0x00, 0x30, 0x72,
  0x01, 0x00, 0x30, 0x12, 0x00, 0x06, 0x00, 0x06, 0x02, 0x71, 0x01, 0x00,
  0x10, 0x73, 0x80, 0xfc, 0x12, 0x73, 0x0c, 0x00, 0x14, 0x73, 0x00, 0x00,
  0x16, 0x73, 0x00, 0x00, 0x18, 0x73, 0x00, 0xa6, 0x1a, 0x73, 0x10, 0x00,
  0x2c, 0x73, 0x00, 0x04, 0x90, 0x73, 0x80, 0xfc, 0x92, 0x73, 0x0a, 0x00,
  0x30, 0x73, 0x84, 0x00, 0x1a, 0x70, 0x0a, 0x00, 0x0a, 0x10, 0x02, 0x00,
  0x02, 0x40, 0x04, 0x72, 0x99, 0x00, 0x0a, 0x30, 0x0a, 0x98, 0x00, 0x04,
  0x00, 0x0c, 0x06, 0x63, 0x01, 0x00, 0x00, 0x63, 0x02, 0x00, 0x0c, 0x63,
  0x03, 0x00, 0x0e, 0x63, 0x04, 0x00, 0x30, 0x63, 0x05, 0x00, 0x2c, 0x7b,
  0x00, 0x04, 0x02, 0x63, 0x01, 0x00, 0x04, 0x63, 0x02, 0x00, 0x08, 0x63,
  0x03, 0x00, 0x0a, 0x63, 0x04, 0x00, 0x18, 0x68, 0x05, 0x00, 0x2c, 0x33,
  0x2c, 0x83, 0x01, 0x00, 0x00, 0x03, 0x40, 0x31, 0x40, 0x89, 0x01, 0x00,
  0x00, 0x03, 0x60, 0x6a, 0x01, 0x00, 0xc0, 0x63, 0x01, 0x00, 0xc2, 0x63,
  0x02, 0x00, 0xc4, 0x63, 0x03, 0x00, 0xca, 0x6b, 0x04, 0x00, 0x18, 0x63,
  0x01, 0x00, 0x1a, 0x6b, 0x02, 0x00, 0xc6, 0x63, 0x01, 0x00, 0xc8, 0x6b,
  0x02, 0x00, 0xd0, 0x63, 0x01, 0x00, 0xd2, 0x63, 0x02, 0x00, 0xd4, 0x63,
  0x03, 0x00, 0xda, 0x6b, 0x04, 0x00, 0xd6, 0x63, 0x01, 0x00, 0xd8, 0x6b,
  0x02, 0x00, 0xdc, 0x6b, 0x01, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x10,
  0xff, 0x00, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x42, 0x71, 0x02, 0x00,
  0x40, 0x11, 0x00, 0x00, 0x00, 0x30, 0x40, 0x31, 0x40, 0x91, 0x05, 0x00,
  0x07, 0x00, 0x44, 0x61, 0x01, 0x00, 0x46, 0x61, 0x02, 0x00, 0x48, 0x61,
  0x03, 0x00, 0x4a, 0x61, 0x04, 0x00, 0x48, 0x01, 0x00, 0x00, 0xff, 0xff,
  0x18, 0x01, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x01, 0x4a, 0x01,
  0x00, 0x00, 0xff, 0xff, 0x21, 0x01, 0x42, 0x71, 0x01, 0x00, 0x40, 0x19,
  0x00, 0x20, 0x00, 0x20, 0x42, 0x79, 0x02, 0x00, 0x40, 0x01, 0x00, 0x00,
  0x00, 0x10, 0x2a, 0x01, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x42, 0x71,
  0x02, 0x00, 0x40, 0x11, 0x00, 0x00, 0x00, 0x30, 0x40, 0x31, 0x40, 0x91,
  0x01, 0x00, 0x07, 0x00, 0x44, 0x61, 0x01, 0x00, 0x46, 0x61, 0x02, 0x00,
  0x48, 0x61, 0x03, 0x00, 0x4a, 0x61, 0x04, 0x00, 0x48, 0x01, 0x00, 0x00,
  0xff, 0xff, 0x43, 0x01, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x47, 0x01,
  0x4a, 0x01, 0x00, 0x00, 0xff, 0xff, 0x4c, 0x01, 0x42, 0x71, 0x01, 0x00,
  0x40, 0x19, 0x00, 0x20, 0x00, 0x20, 0x42, 0x79, 0x02, 0x00, 0x40, 0x01,
  0x00, 0x00, 0x00, 0x10, 0x55, 0x01, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10,
  0x42, 0x71, 0x02, 0x00, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x40, 0x31,
  0x40, 0x99, 0x00, 0x80, 0x00, 0x80, 0x40, 0x01, 0x00, 0x00, 0x00, 0x10,
  0x65, 0x01, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x42, 0x71, 0x02, 0x00,
  0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x40, 0x31, 0x40, 0x91, 0x00, 0x00,
  0x07, 0x00, 0x40, 0x31, 0x40, 0x81, 0x01, 0x00, 0x78, 0x04, 0x4c, 0x71,
  0x00, 0x00, 0x4e, 0x71, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x04,
  0x83, 0x01, 0xc2, 0x33, 0x50, 0x91, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x33,
  0x52, 0x91, 0x00, 0x00, 0x00, 0x00, 0x98, 0xf1, 0x40, 0x01, 0x00, 0x01,
  0x00, 0x01, 0x90, 0x01, 0x06, 0x33, 0x50, 0x91, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x33, 0x52, 0x91, 0x00, 0x00, 0x00, 0x00, 0x98, 0xf1, 0x06, 0x33,
  0x52, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x50, 0x91, 0x00, 0x00,
  0x00, 0x00, 0x42, 0x79, 0x01, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x10,
  0xa3, 0x01, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x42, 0x71, 0x02, 0x00,
  0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x40, 0x31, 0x40, 0x91, 0x00, 0x00,
  0x07, 0x00, 0x40, 0x31, 0x40, 0x81, 0x01, 0x00, 0x78, 0x04, 0x4c, 0x61,
  0x02, 0x00, 0x4e, 0x61, 0x03, 0x00, 0x50, 0x61, 0x04, 0x00, 0x52, 0x61,
  0x05, 0x00, 0x42, 0x79, 0x01, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x10,
  0xc1, 0x01, 0x40, 0x11, 0x00, 0x00, 0x00, 0x10, 0x42, 0x71, 0x02, 0x00,
  0x40, 0x19, 0x00, 0x00, 0x00, 0x10, 0x40, 0x19, 0x00, 0x00, 0x00, 0x10,
  0x40, 0x31, 0x40, 0x91, 0x80, 0x00, 0x80, 0x00, 0x44, 0x61, 0x01, 0x00,
  0x46, 0x69, 0x02, 0x00, 0x40, 0x31, 0x40, 0x99, 0x00, 0x00, 0x80, 0x00,
  0x38, 0x1b, 0x00, 0x00, 0x01, 0x00, 0x38, 0x1b, 0x00, 0x00, 0x08, 0x00,
  0x38, 0x1b, 0x20, 0x00, 0x20, 0x00, 0x2e, 0x63, 0x01, 0x00, 0x2e, 0x03,
  0x00, 0x00, 0xff, 0xff, 0xe5, 0x01, 0x38, 0x1b, 0x40, 0x00, 0x40, 0x00,
  0x2e, 0x3b, 0x90, 0x63, 0x01, 0x00, 0x92, 0x63, 0x02, 0x00, 0x34, 0x73,
  0x01, 0x00, 0x38, 0x1b, 0x00, 0x00, 0x01, 0x00, 0x34, 0x7b, 0x05, 0x00,
  0x34, 0xab, 0x01, 0x00, 0x07, 0x00, 0xf0, 0x4f, 0x80, 0x63, 0x02, 0x00,
  0x82, 0x63, 0x03, 0x00, 0x84, 0x63, 0x04, 0x00, 0x86, 0x63, 0x05, 0x00,
  0x34, 0xab, 0x01, 0x00, 0x07, 0xa0, 0xf0, 0x4f, 0x34, 0xab, 0x01, 0x00,
  0x09, 0x00, 0xf0, 0x4f, 0x80, 0x63, 0x02, 0x00, 0x82, 0x63, 0x03, 0x00,
  0x84, 0x63, 0x04, 0x00, 0x86, 0x63, 0x05, 0x00, 0x34, 0xab, 0x01, 0x00,
  0x09, 0xa0, 0xf0, 0x4f, 0x34, 0x7b, 0x0b, 0x00, 0x10, 0x63, 0x01, 0x00,
  0x12, 0x6b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbe, 0xce,
  0x00, 0x00
};

#define SIZEOF_ISIS_COMMANDS 2186

static u8 isis_waveform[] =
{
      0x16, 0x8e, 0xc1, 0x33, 0x43, 0x0b, 0x00, 0x00, 0x1f, 0x01, 0x00, 0x00,
      0x00, 0x03, 0x01, 0x00, 0x01, 0x14, 0x16, 0x0b, 0x3c, 0x01, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0xca, 0x3a, 0x00, 0x00, 0x01,
      0x00, 0x03, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39,
      0x00, 0x30, 0x30, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x4a, 0x00,
      0x00, 0x4a, 0x4e, 0x00, 0x00, 0x4e, 0x52, 0x00, 0x00, 0x52, 0x56, 0x00,
      0x00, 0x56, 0x5a, 0x00, 0x00, 0x5a, 0x86, 0x00, 0x00, 0x86, 0x2e, 0x01,
      0x00, 0x2f, 0x97, 0x08, 0x00, 0x9f, 0x55, 0xff, 0x55, 0xff, 0x55, 0xff,
      0x55, 0xbf, 0x00, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xbf,
      0x00, 0xff, 0x00, 0xbf, 0x55, 0xff, 0x55, 0xff, 0x55, 0xff, 0x55, 0xbf,
      0x00, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xbf, 0x00, 0xff,
      0xff, 0xa2, 0x00, 0x02, 0x50, 0x00, 0x00, 0x37, 0x02, 0x00, 0x00, 0x05,
      0x50, 0x00, 0x00, 0x37, 0x56, 0x00, 0x00, 0x04, 0xfc, 0x40, 0x55, 0xfc,
      0x00, 0x37, 0x56, 0x00, 0x00, 0x04, 0x55, 0x01, 0x00, 0x37, 0xfc, 0x5a,
      0x01, 0x00, 0x10, 0x40, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0x5a,
      0x01, 0x00, 0x10, 0x40, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0x6a,
      0x05, 0x00, 0x10, 0x50, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0xaa,
      0x05, 0x00, 0x10, 0x50, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0xaa,
      0x06, 0x00, 0x10, 0x50, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0xaa,
      0x0a, 0x00, 0x10, 0x50, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0xaa,
      0x1a, 0x40, 0x21, 0x54, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xfc, 0xaa,
      0x2a, 0x45, 0x21, 0x54, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xaa, 0x01,
      0xfc, 0x8a, 0x22, 0x54, 0x55, 0x55, 0x55, 0xfc, 0x00, 0x37, 0xaa, 0x02,
      0xfc, 0x22, 0x54, 0xfc, 0x55, 0x02, 0x00, 0x37, 0xaa, 0x02, 0xfc, 0x2a,
      0x54, 0xfc, 0x55, 0x02, 0x00, 0x37, 0xaa, 0x02, 0x2a, 0x00, 0x00, 0x3f,
      0xff, 0x30, 0x00, 0x0e, 0x10, 0x00, 0x00, 0x2f, 0x02, 0x00, 0x00, 0x09,
      0x50, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x1f, 0xfc, 0x50, 0x55, 0x40,
      0x11, 0xfc, 0x00, 0x0b, 0x02, 0x00, 0x00, 0x05, 0x50, 0x00, 0x00, 0x02,
      0x50, 0x00, 0x00, 0x02, 0xfc, 0x50, 0x40, 0x01, 0x00, 0x10, 0x00, 0x00,
      0x00, 0x10, 0xfc, 0x00, 0x0a, 0xfc, 0x10, 0x40, 0x15, 0x40, 0x51, 0xfc,
      0x00, 0x07, 0x50, 0x00, 0x55, 0x01, 0x11, 0x00, 0x00, 0x0b, 0x02, 0x00,
      0x00, 0x05, 0xfc, 0x50, 0x40, 0x01, 0x00, 0x50, 0x00, 0x00, 0x00, 0x50,
      0x40, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0xfc, 0x00, 0x06, 0x10,
      0x00, 0x00, 0x02, 0xfc, 0x10, 0x40, 0x15, 0x41, 0x51, 0x40, 0x05, 0x00,
      0x10, 0xfc, 0x00, 0x03, 0x50, 0x00, 0x55, 0x02, 0xfc, 0x40, 0x01, 0x00,
      0x10, 0x10, 0x01, 0xfc, 0x00, 0x05, 0x42, 0x00, 0x00, 0x01, 0xfc, 0x10,
      0x40, 0x01, 0x00, 0x50, 0x40, 0x01, 0x00, 0x50, 0x40, 0x01, 0x00, 0x50,
      0x40, 0x01, 0x00, 0x50, 0x40, 0x01, 0x00, 0x50, 0xfc, 0x00, 0x06, 0xfc,
      0x10, 0x40, 0x01, 0x00, 0x10, 0x50, 0x15, 0x41, 0x55, 0x40, 0x05, 0x01,
      0x10, 0xfc, 0x00, 0x03, 0x50, 0x00, 0x55, 0x02, 0xfc, 0x40, 0x01, 0x00,
      0x10, 0x11, 0x01, 0xfc, 0x00, 0x05, 0x42, 0x00, 0x00, 0x01, 0xfc, 0x10,
      0x40, 0x01, 0x00, 0x50, 0x40, 0x01, 0x40, 0x51, 0x40, 0x01, 0x00, 0x50,
      0x40, 0x01, 0x00, 0x50, 0x40, 0x01, 0x00, 0x50, 0xfc, 0x00, 0x03, 0xfc,
      0x40, 0x01, 0x00, 0x10, 0x40, 0x01, 0x00, 0x10, 0x50, 0x15, 0x55, 0x55,
      0x40, 0x15, 0x41, 0x51, 0xfc, 0x00, 0x03, 0x50, 0x00, 0x55, 0x02, 0xfc,
      0x40, 0x05, 0x00, 0x10, 0x15, 0x01, 0xfc, 0x00, 0x05, 0x42, 0x00, 0x00,
      0x01, 0xfc, 0x10, 0x40, 0x05, 0x40, 0x51, 0x40, 0x01, 0x40, 0x51, 0x40,
      0x05, 0x40, 0x51, 0x40, 0x05, 0x00, 0x50, 0x40, 0x05, 0x40, 0x51, 0xfc,
      0x00, 0x03, 0xfc, 0x40, 0x01, 0x00, 0x10, 0x40, 0x01, 0x00, 0x50, 0x50,
      0x55, 0x55, 0x55, 0x40, 0x15, 0x41, 0x51, 0xfc, 0x00, 0x03, 0x50, 0x00,
      0x55, 0x02, 0xfc, 0x40, 0x05, 0x00, 0x10, 0x15, 0x01, 0xfc, 0x00, 0x05,
      0x42, 0x00, 0x00, 0x01, 0xfc, 0x10, 0x40, 0x05, 0x40, 0x51, 0x40, 0x05,
      0x40, 0x55, 0x40, 0x05, 0x40, 0x51, 0x40, 0x05, 0x00, 0x50, 0x40, 0x05,
      0x40, 0x51, 0x00, 0x00, 0x00, 0x10, 0x40, 0x05, 0x00, 0x50, 0x40, 0x01,
      0x00, 0x50, 0x50, 0x55, 0x55, 0x55, 0x50, 0x55, 0x41, 0x55, 0xfc, 0x00,
      0x03, 0x50, 0x00, 0x55, 0x02, 0xfc, 0x50, 0x15, 0x40, 0x11, 0x15, 0x01,
      0xfc, 0x00, 0x05, 0x42, 0x00, 0x00, 0x01, 0xfc, 0x10, 0x40, 0x15, 0x40,
      0x55, 0x40, 0x05, 0x40, 0x55, 0x40, 0x05, 0x40, 0x55, 0x50, 0x05, 0x40,
      0x51, 0x40, 0x15, 0x40, 0x55, 0x00, 0x00, 0x00, 0x10, 0x40, 0x05, 0x00,
      0x50, 0x40, 0x05, 0x00, 0x51, 0x50, 0x55, 0x55, 0x55, 0x50, 0x55, 0x55,
      0x55, 0xfc, 0x00, 0x03, 0x50, 0x00, 0x55, 0x02, 0xfc, 0x50, 0x15, 0x41,
      0x11, 0x15, 0x01, 0xfc, 0x00, 0x05, 0x42, 0x00, 0x00, 0x01, 0xfc, 0x10,
      0x40, 0x15, 0x41, 0x55, 0x50, 0x05, 0x40, 0x55, 0x40, 0x05, 0x41, 0x55,
      0x50, 0x05, 0x55, 0x51, 0x40, 0x15, 0x41, 0x55, 0x00, 0x00, 0x00, 0x10,
      0x40, 0x15, 0x40, 0x51, 0x40, 0x05, 0x00, 0x51, 0x50, 0x55, 0x55, 0x55,
      0x50, 0x55, 0x55, 0x55, 0x40, 0x00, 0x00, 0x00, 0x50, 0x55, 0x55, 0x55,
      0x50, 0x55, 0x41, 0x51, 0x15, 0x01, 0xfc, 0x00, 0x05, 0x52, 0x00, 0x00,
      0x01, 0xfc, 0x10, 0x50, 0x15, 0x41, 0x55, 0x50, 0x05, 0x40, 0x55, 0x50,
      0x15, 0x41, 0x55, 0x50, 0x05, 0x55, 0x51, 0x50, 0x15, 0x41, 0x55, 0x00,
      0x00, 0x00, 0x10, 0x40, 0x15, 0x41, 0x51, 0x50, 0x05, 0x40, 0x55, 0x50,
      0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x40, 0x00, 0x00, 0x00, 0x50,
      0x55, 0x55, 0x55, 0x50, 0x55, 0x51, 0x51, 0x15, 0x01, 0x05, 0xfc, 0x00,
      0x04, 0x52, 0x00, 0x00, 0x01, 0xfc, 0x10, 0x50, 0x15, 0x51, 0x55, 0x50,
      0x15, 0x40, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50, 0x05, 0x55, 0x51, 0x50,
      0x15, 0x51, 0x55, 0x40, 0x01, 0x00, 0x50, 0x50, 0x15, 0x41, 0x55, 0x50,
      0x05, 0x40, 0x55, 0x50, 0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x40,
      0x01, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55, 0x50, 0x55, 0x51, 0x55, 0x55,
      0x05, 0x05, 0xfc, 0x00, 0x04, 0xfc, 0x52, 0x01, 0x40, 0x14, 0x50, 0x15,
      0x51, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50, 0x05,
      0x55, 0x51, 0x50, 0x15, 0x51, 0x55, 0x40, 0x01, 0x00, 0x50, 0x50, 0x15,
      0x55, 0x55, 0x50, 0x15, 0x40, 0x55, 0x50, 0x55, 0x55, 0x55, 0x50, 0x55,
      0x55, 0x55, 0x40, 0x01, 0x04, 0x00, 0x54, 0x55, 0x55, 0x55, 0x50, 0xfc,
      0x55, 0x03, 0x05, 0x01, 0xfc, 0x00, 0x40, 0xfc, 0x00, 0x02, 0xfc, 0x52,
      0x01, 0x40, 0x14, 0x50, 0x15, 0x55, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50,
      0x15, 0x41, 0x55, 0x50, 0x05, 0x55, 0x51, 0x50, 0x15, 0x55, 0x55, 0x40,
      0x01, 0x00, 0x50, 0x50, 0x55, 0x55, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50,
      0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x50, 0x01, 0x04, 0x10, 0x54,
      0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x56, 0x15, 0x45, 0x50, 0x40,
      0x00, 0x00, 0x00, 0x92, 0x15, 0x40, 0x15, 0x50, 0x15, 0x55, 0x55, 0x50,
      0x15, 0x41, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50, 0x15, 0x55, 0x51, 0x50,
      0x15, 0x55, 0x55, 0x40, 0x01, 0x00, 0x50, 0x50, 0x55, 0x55, 0x55, 0x50,
      0x15, 0x41, 0x55, 0x54, 0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x50,
      0x01, 0x04, 0x10, 0x54, 0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x56,
      0x15, 0x45, 0x50, 0x40, 0x01, 0x00, 0x00, 0x92, 0x15, 0x41, 0x15, 0x50,
      0x15, 0x55, 0x55, 0x50, 0x15, 0x41, 0x55, 0x50, 0x15, 0x55, 0x55, 0x50,
      0x15, 0x55, 0x51, 0x50, 0x15, 0x55, 0x55, 0x40, 0x05, 0x40, 0x51, 0x50,
      0x55, 0x55, 0x55, 0x50, 0x15, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x54,
      0x55, 0x55, 0x55, 0x50, 0x05, 0x04, 0x10, 0x54, 0x55, 0x55, 0x55, 0x50,
      0x55, 0x55, 0x55, 0x56, 0x15, 0x45, 0x50, 0x40, 0x01, 0x00, 0x00, 0x92,
      0x15, 0x41, 0x15, 0x50, 0x55, 0x55, 0x55, 0x52, 0x15, 0x41, 0x55, 0x52,
      0x15, 0x55, 0x55, 0x52, 0x55, 0x55, 0x51, 0x54, 0x55, 0x55, 0x55, 0x40,
      0x05, 0x40, 0x51, 0x50, 0x55, 0x55, 0x55, 0x50, 0x15, 0x55, 0x55, 0x54,
      0x55, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x50, 0x05, 0x04, 0x10, 0x54,
      0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x5a, 0x15, 0x45, 0x50, 0x44,
      0x01, 0x00, 0x00, 0x92, 0x15, 0x51, 0x15, 0x50, 0x55, 0x55, 0x55, 0x92,
      0x16, 0x41, 0x55, 0x52, 0x15, 0x55, 0x55, 0x92, 0x56, 0x55, 0x51, 0x54,
      0x55, 0x55, 0x55, 0x50, 0x05, 0x40, 0x55, 0x54, 0x55, 0x55, 0x55, 0x50,
      0x15, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x50,
      0x05, 0x04, 0x50, 0x54, 0x55, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x6a,
      0x15, 0x45, 0x50, 0x54, 0x01, 0x00, 0x40, 0xa2, 0x55, 0x51, 0x15, 0x92,
      0x56, 0x55, 0x55, 0x92, 0x16, 0x41, 0xa5, 0x92, 0x16, 0x55, 0x65, 0x96,
      0x56, 0x55, 0x51, 0x54, 0x55, 0x55, 0x55, 0x51, 0x05, 0x41, 0x55, 0x54,
      0x55, 0x55, 0x55, 0x94, 0x56, 0x55, 0x55, 0xa4, 0x6a, 0x55, 0x55, 0x54,
      0x55, 0x55, 0x55, 0x50, 0x05, 0x04, 0x50, 0x64, 0x55, 0x55, 0x55, 0x54,
      0x55, 0x55, 0x55, 0x6a, 0x16, 0x45, 0x50, 0x50, 0x01, 0x00, 0x40, 0xa2,
      0x55, 0x55, 0x15, 0x92, 0x5a, 0x55, 0xa5, 0xa6, 0x1a, 0x81, 0xa6, 0x92,
      0x1a, 0x55, 0xa5, 0xa6, 0x5a, 0x55, 0x61, 0x94, 0x56, 0x55, 0x55, 0x51,
      0x15, 0x41, 0x55, 0x54, 0x55, 0x55, 0x55, 0x94, 0x56, 0x55, 0x65, 0xa4,
      0x6a, 0x96, 0x56, 0x54, 0x55, 0x55, 0x55, 0x50, 0x05, 0x04, 0x50, 0xa4,
      0xaa, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x6a, 0x16, 0x45, 0x55, 0x50,
      0x01, 0x00, 0x40, 0xa2, 0x55, 0x55, 0x25, 0xa6, 0x6a, 0x95, 0xa6, 0xa6,
      0x1a, 0x95, 0xaa, 0xa6, 0x1a, 0x95, 0xa6, 0xa6, 0x5a, 0x55, 0xa5, 0x94,
      0x5a, 0x55, 0x65, 0x51, 0x15, 0x51, 0x55, 0x54, 0x55, 0x55, 0x55, 0xa4,
      0x5a, 0x55, 0x65, 0xa4, 0xaa, 0xaa, 0xaa, 0x54, 0x55, 0x55, 0x55, 0x50,
      0x05, 0x04, 0x50, 0xa5, 0xaa, 0xaa, 0x56, 0x54, 0x55, 0x55, 0x55, 0x6a,
      0x16, 0x45, 0x55, 0x50, 0x01, 0x00, 0x40, 0xa2, 0x55, 0x55, 0x25, 0xa6,
      0x6a, 0x96, 0xaa, 0xa6, 0x6a, 0x95, 0xaa, 0xa6, 0x2a, 0x96, 0xaa, 0xa6,
      0x5a, 0xaa, 0xa6, 0xa4, 0x6a, 0x95, 0xa6, 0x52, 0x15, 0x51, 0x55, 0x94,
      0x56, 0x55, 0x55, 0xa6, 0x5a, 0x55, 0xa6, 0xa9, 0xaa, 0xaa, 0xaa, 0x94,
      0x5a, 0x55, 0x55, 0x54, 0x05, 0x04, 0x50, 0xa9, 0xaa, 0xaa, 0x6a, 0x94,
      0x56, 0x55, 0x55, 0x6a, 0x56, 0x45, 0x55, 0x50, 0x01, 0x00, 0x40, 0xa2,
      0x55, 0x55, 0x25, 0xa6, 0x6a, 0xa6, 0xaa, 0xaa, 0x6a, 0x96, 0xaa, 0xa6,
      0x6a, 0x96, 0xaa, 0xaa, 0x5a, 0xaa, 0xa6, 0xa8, 0x6a, 0x96, 0xaa, 0x52,
      0x15, 0x55, 0x55, 0x94, 0x5a, 0x55, 0x65, 0xaa, 0x6a, 0x95, 0xfc, 0xaa,
      0x04, 0xfc, 0xa5, 0x6a, 0x56, 0x55, 0x94, 0x05, 0x04, 0x50, 0xa9, 0xaa,
      0xaa, 0xaa, 0xa5, 0x5a, 0x55, 0x55, 0xaa, 0x5a, 0x5a, 0x55, 0x10, 0x05,
      0x01, 0x40, 0xa1, 0x55, 0x55, 0x25, 0xaa, 0x6a, 0xaa, 0xaa, 0xaa, 0x6a,
      0x96, 0xaa, 0xaa, 0x6a, 0x96, 0xaa, 0xaa, 0x6a, 0xaa, 0xa6, 0xaa, 0x6a,
      0xa6, 0xaa, 0x96, 0x16, 0x55, 0x65, 0xa4, 0x6a, 0x55, 0x65, 0xaa, 0x6a,
      0x96, 0xfc, 0xaa, 0x04, 0xfc, 0xa5, 0xaa, 0x96, 0x66, 0xa4, 0x06, 0x44,
      0x55, 0xfc, 0xaa, 0x03, 0xfc, 0xa5, 0x6a, 0x55, 0x65, 0xaa, 0x6a, 0x5a,
      0x55, 0x00, 0x04, 0x01, 0x50, 0xa5, 0x65, 0x55, 0x29, 0xfc, 0xaa, 0x04,
      0xfc, 0x6a, 0x96, 0xaa, 0xaa, 0x6a, 0xfc, 0xaa, 0x04, 0xfc, 0xa6, 0xaa,
      0x6a, 0xaa, 0xaa, 0x96, 0x16, 0x55, 0x65, 0xa8, 0x6a, 0x96, 0xa6, 0xaa,
      0x6a, 0xfc, 0xaa, 0x05, 0xa9, 0x00, 0xaa, 0x02, 0xfc, 0xa6, 0x06, 0x59,
      0x55, 0xfc, 0xaa, 0x03, 0xfc, 0xa5, 0xaa, 0x96, 0x66, 0xaa, 0x6a, 0x9a,
      0x55, 0x00, 0x44, 0x55, 0x50, 0xa5, 0x66, 0x95, 0x29, 0xfc, 0xaa, 0x04,
      0xfc, 0x6a, 0x96, 0xaa, 0xaa, 0x6a, 0xfc, 0xaa, 0x04, 0xa6, 0x00, 0xaa,
      0x03, 0xfc, 0xa6, 0x5a, 0x55, 0xa5, 0xa8, 0xfc, 0xaa, 0x0f, 0xfc, 0x5a,
      0x59, 0x55, 0xfc, 0xaa, 0x05, 0xfc, 0xa6, 0x66, 0xaa, 0x6a, 0x9a, 0xa5,
      0x00, 0x44, 0x55, 0x55, 0xa5, 0x6a, 0x95, 0x29, 0xfc, 0xaa, 0x08, 0x6a,
      0x00, 0xaa, 0x04, 0xa6, 0x00, 0xaa, 0x04, 0xfc, 0x5a, 0x95, 0xa6, 0xfc,
      0xaa, 0x10, 0xfc, 0x5a, 0x59, 0x65, 0xfc, 0xaa, 0x08, 0xfc, 0x6a, 0x9a,
      0xa5, 0x00, 0x54, 0x55, 0x55, 0xa5, 0x6a, 0xa5, 0x2a, 0xfc, 0xaa, 0x14,
      0xfc, 0x6a, 0x96, 0xfc, 0xaa, 0x11, 0xfc, 0x5a, 0x59, 0x65, 0xfc, 0xaa,
      0x09, 0xfc, 0x9a, 0xaa, 0x00, 0x50, 0x54, 0x15, 0xa5, 0x6a, 0xa6, 0x2a,
      0xfc, 0xaa, 0x14, 0xfc, 0x6a, 0xa6, 0xfc, 0xaa, 0x11, 0xfc, 0x5a, 0x59,
      0xa5, 0xfc, 0xaa, 0x0b, 0xfc, 0x00, 0x10, 0x40, 0x15, 0xa9, 0xaa, 0xa6,
      0x2a, 0xfc, 0xaa, 0x14, 0x6a, 0x00, 0xaa, 0x12, 0xfc, 0x5a, 0x59, 0xa5,
      0xfc, 0xaa, 0x0b, 0xfc, 0x02, 0x10, 0x00, 0x10, 0x69, 0xaa, 0xa6, 0x2a,
      0xfc, 0xaa, 0x37, 0x82, 0x00, 0x00, 0x01, 0xfc, 0x10, 0x69, 0xaa, 0xa6,
      0x2a, 0xfc, 0xaa, 0x37, 0xfc, 0x82, 0x02, 0x00, 0x00, 0x59, 0xaa, 0xaa,
      0x6a, 0xa8, 0xaa, 0xaa, 0xaa, 0xa8, 0xaa, 0xaa, 0xaa, 0xa9, 0xaa, 0xaa,
      0xaa, 0xa9, 0xfc, 0xaa, 0x2b, 0x02, 0x00, 0x00, 0x01, 0x59, 0x00, 0xaa,
      0x01, 0xfc, 0x6a, 0xa8, 0xfc, 0xaa, 0x02, 0xa8, 0x00, 0xaa, 0x33, 0x02,
      0x00, 0x00, 0x01, 0x59, 0x00, 0xaa, 0x01, 0xfc, 0x5a, 0xa8, 0xfc, 0xaa,
      0x02, 0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00, 0xaa,
      0x2b, 0xfc, 0x02, 0x00, 0x80, 0x59, 0x9a, 0xaa, 0x56, 0xfc, 0x00, 0x03,
      0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00, 0xaa, 0x0e,
      0x55, 0x03, 0xaa, 0x18, 0xfc, 0x0a, 0x00, 0x80, 0x59, 0x9a, 0x5a, 0x56,
      0xfc, 0x00, 0x07, 0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00, 0xaa, 0x2c, 0xfc,
      0x02, 0xa0, 0x59, 0x95, 0x5a, 0xfc, 0x55, 0x08, 0x54, 0x00, 0x55, 0x02,
      0x54, 0x00, 0x55, 0x02, 0xaa, 0x0b, 0x00, 0x07, 0xaa, 0x16, 0xfc, 0xa0,
      0x54, 0x55, 0x59, 0xfc, 0x55, 0x08, 0xa8, 0x00, 0xaa, 0x02, 0xa8, 0x00,
      0xaa, 0x02, 0x55, 0x03, 0x56, 0x00, 0x55, 0x02, 0xaa, 0x03, 0x00, 0x07,
      0xaa, 0x03, 0x55, 0x03, 0xaa, 0x07, 0x55, 0x03, 0xaa, 0x03, 0xfc, 0x54,
      0x55, 0x59, 0xfc, 0x55, 0x10, 0xaa, 0x03, 0xa8, 0x00, 0xaa, 0x06, 0x00,
      0x07, 0xaa, 0x17, 0x55, 0x17, 0x01, 0x00, 0x00, 0x0e, 0xaa, 0x03, 0x00,
      0x03, 0x55, 0x03, 0xaa, 0x03, 0x55, 0x03, 0xaa, 0x03, 0x55, 0x1f, 0x00,
      0x07, 0xaa, 0x03, 0x00, 0x03, 0xaa, 0x07, 0x55, 0x03, 0xaa, 0x03, 0x55,
      0x3b, 0xaa, 0x03, 0x55, 0x3b, 0xaa, 0x03, 0x55, 0x07, 0x00, 0x07, 0x55,
      0x1b, 0xaa, 0x0f, 0x00, 0x03, 0x55, 0x07, 0x00, 0x07, 0xaa, 0x1b, 0x00,
      0x0b, 0xaa, 0x03, 0x00, 0x03, 0x55, 0x03, 0x00, 0x33, 0xaa, 0x03, 0x00,
      0x43, 0xff, 0xad, 0x5a, 0x00, 0x55, 0x02, 0x00, 0x12, 0x50, 0x00, 0x00,
      0x0f, 0xfc, 0x40, 0x01, 0x00, 0x50, 0xfc, 0x00, 0x0f, 0x56, 0x00, 0x55,
      0x02, 0x5a, 0x00, 0x55, 0x02, 0x00, 0x12, 0x50, 0x00, 0x00, 0x0f, 0xfc,
      0x40, 0x01, 0x00, 0x50, 0xfc, 0x00, 0x0f, 0x56, 0x00, 0x55, 0x02, 0xfc,
      0x5a, 0x56, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0x40, 0x01, 0x00, 0x50,
      0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x05, 0x40, 0x51, 0xfc, 0x00, 0x0f, 0xfc,
      0x5a, 0x55, 0x45, 0x55, 0x5a, 0x96, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc,
      0x40, 0x01, 0x00, 0x50, 0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x05, 0x41, 0x51,
      0xfc, 0x00, 0x0f, 0xfc, 0x5a, 0x55, 0x45, 0x55, 0x5a, 0x96, 0x6a, 0x55,
      0xfc, 0x00, 0x0f, 0xfc, 0x40, 0x01, 0x40, 0x50, 0xfc, 0x00, 0x0f, 0xfc,
      0x50, 0x15, 0x41, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0x5a, 0x55, 0x00, 0x55,
      0x59, 0x96, 0xaa, 0x5a, 0xfc, 0x00, 0x0f, 0xfc, 0x40, 0x01, 0x40, 0x50,
      0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x15, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc,
      0x5a, 0x55, 0x02, 0x50, 0xa9, 0x96, 0xaa, 0x5a, 0xfc, 0x00, 0x0f, 0xfc,
      0x50, 0x05, 0x40, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x15, 0x55, 0x55,
      0xfc, 0x00, 0x0f, 0xfc, 0x5a, 0x15, 0x02, 0x50, 0xa9, 0x96, 0xaa, 0x5a,
      0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x05, 0x41, 0x55, 0xfc, 0x00, 0x0f, 0xfc,
      0x50, 0x15, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0x5a, 0x25, 0xa2, 0x50,
      0xa9, 0x56, 0xaa, 0x5a, 0xfc, 0x00, 0x0f, 0xfc, 0x50, 0x15, 0x41, 0x55,
      0xfc, 0x00, 0x0f, 0x54, 0x00, 0x55, 0x02, 0x00, 0x0f, 0xfc, 0x5a, 0x25,
      0xa2, 0x50, 0xa5, 0x56, 0x9a, 0x5a, 0xfc, 0x00, 0x0f, 0xfc, 0x52, 0x15,
      0x55, 0x55, 0xfc, 0x00, 0x0f, 0x54, 0x00, 0x55, 0x02, 0x00, 0x0f, 0xfc,
      0x4a, 0x21, 0xaa, 0x58, 0xa4, 0x55, 0x95, 0x5a, 0xfc, 0x00, 0x0f, 0xfc,
      0x52, 0x15, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0x54, 0x00, 0x55, 0x02, 0x00,
      0x0f, 0xfc, 0x6a, 0x29, 0xaa, 0x1a, 0xa4, 0x69, 0x55, 0x55, 0xfc, 0x00,
      0x0f, 0xfc, 0x92, 0x16, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0xa6, 0x56,
      0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0x6a, 0x29, 0xaa, 0x1a, 0xa4, 0x69,
      0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0xa6, 0x56, 0x55, 0xa5, 0xfc, 0x00,
      0x0f, 0xfc, 0xaa, 0x5a, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xaa, 0x01, 0xfc,
      0x88, 0x1a, 0xa4, 0x69, 0x55, 0x55, 0xfc, 0x00, 0x0f, 0xfc, 0xaa, 0x5a,
      0x95, 0xa5, 0xfc, 0x00, 0x0f, 0xfc, 0xaa, 0x6a, 0x96, 0xa6, 0xfc, 0x00,
      0x0f, 0xaa, 0x01, 0xfc, 0x08, 0x1a, 0xa4, 0x69, 0x55, 0x55, 0xfc, 0x00,
      0x0f, 0xfc, 0xaa, 0x6a, 0x96, 0xaa, 0xfc, 0x00, 0x0f, 0xfc, 0xaa, 0x6a,
      0xaa, 0xaa, 0xfc, 0x00, 0x0f, 0xfc, 0xa2, 0xaa, 0x08, 0x0a, 0x64, 0x69,
      0x55, 0x15, 0xfc, 0x00, 0x0f, 0xfc, 0xaa, 0x6a, 0xaa, 0xaa, 0xfc, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0xaa, 0x08, 0x2a, 0x54, 0x69,
      0x55, 0x25, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xfc, 0xa0, 0xaa, 0x00, 0x22, 0x54, 0x59, 0x55, 0x25, 0xfc, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0xaa,
      0x00, 0xa0, 0x54, 0x55, 0x55, 0x25, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0x8a, 0x00, 0xa0, 0x54, 0x55,
      0x55, 0x25, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xfc, 0xa0, 0x8a, 0x00, 0xa0, 0x54, 0x55, 0x55, 0x25, 0xfc, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0x8a,
      0x00, 0xa0, 0x54, 0x15, 0x55, 0x25, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0x8a, 0x00, 0xa0, 0x54, 0x15,
      0x45, 0x25, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xfc, 0xa0, 0x0a, 0x00, 0xa0, 0x50, 0x05, 0x45, 0x15, 0xfc, 0x00,
      0x0f, 0x55, 0x03, 0x00, 0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0x02,
      0x00, 0xa0, 0x50, 0x05, 0x40, 0x15, 0xfc, 0x00, 0x0f, 0xaa, 0x03, 0x00,
      0x0f, 0xaa, 0x03, 0x00, 0x0f, 0xfc, 0xa0, 0x02, 0x00, 0xa0, 0x50, 0x01,
      0x00, 0x10, 0xfc, 0x00, 0x0f, 0x55, 0x03, 0x00, 0x0f, 0x55, 0x03, 0x00,
      0x0f, 0xfc, 0x80, 0x02, 0x00, 0xa0, 0x50, 0x01, 0x00, 0x10, 0xfc, 0x00,
      0x0f, 0x55, 0x03, 0x00, 0x0f, 0x55, 0x03, 0x00, 0x0f, 0xfc, 0x80, 0x02,
      0x00, 0xa0, 0x50, 0x00, 0x00, 0x10, 0xfc, 0x00, 0x0f, 0x55, 0x03, 0x00,
      0x0f, 0x55, 0x03, 0x00, 0x0f, 0xfc, 0x80, 0x02, 0x00, 0x20, 0x50, 0x00,
      0x00, 0x10, 0xfc, 0x00, 0x23, 0xaa, 0x03, 0x00, 0x12, 0x20, 0x00, 0x00,
      0x3f, 0xff, 0x70
};

#define SIZEOF_ISIS_WAVEFORM 2883

static bool bs_isis(void);
static bool bs_asic(void);
static bool bs_fpga(void);

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet SW Primitives
    #pragma mark -
#endif

#define BS_UPD_MODE(u) (bs_upd_modes ? bs_upd_modes[(u)] : (u))

static void bs_set_upd_modes(void)
{
    // Don't re-read flash to set up the modes again unless we have to.
    //
    if ( !bs_upd_modes )
    {
        broadsheet_waveform_t waveform; broadsheet_get_waveform_version(&waveform);
    
        // Check for the old-style behavior first, then check for the new style.
        //
        if ( BS_UPD_MODES_OLD == waveform.mode_version )
            bs_upd_modes = bs_upd_modes_old;
        else
        {
            struct einkfb_info info; 
            einkfb_get_info(&info);
        
            switch ( info.bpp )
            {
                case EINKFB_2BPP:
                    bs_upd_modes = bs_upd_modes_new_2bpp;
                break;
                
                case EINKFB_4BPP:
                    bs_upd_modes = bs_upd_modes_new_4bpp;
                break;
            }
        }
    }
}

static void bs_debug_upd_mode(u16 upd_mode)
{
    if ( EINKFB_DEBUG() )
    {
        char *upd_mode_string = NULL;
        
        if ( !bs_upd_modes || (bs_upd_modes_old == bs_upd_modes) )
        {
            switch ( upd_mode )
            {
                case BS_UPD_MODE_MU:
                    upd_mode_string = "MU";
                break;
                
                case BS_UPD_MODE_PU:
                    upd_mode_string = "PU";
                break;
                
                case BS_UPD_MODE_GU:
                    upd_mode_string = "GU";
                break;
                
                case BS_UPD_MODE_GC:
                    upd_mode_string = "GC";
                break;
            }
        }
        else
        {
            switch ( upd_mode )
            {
                case BS_UPD_MODE_DU:
                    upd_mode_string = "DU";
                break;
                
                case BS_UPD_MODE_GC4:
                    upd_mode_string = "GC4";
                break;
                
                case BS_UPD_MODE_GC16:
                    upd_mode_string = "GC16";
                break;
            }
        }
        
        if ( upd_mode_string )
            einkfb_debug("upd_mode = %s\n", upd_mode_string);
    }
}

static u16 bs_upd_mode_max(u16 a, u16 b)
{
    u16 max = BS_UPD_MODE(BS_UPD_MODE_GC);
    
    if ( !bs_upd_modes || (bs_upd_modes_old == bs_upd_modes) )
    {
        switch ( a )
        {
            case BS_UPD_MODE_MU:
                max = b;
             break;
            
            case BS_UPD_MODE_PU:
                max = ((BS_UPD_MODE_GU == b) || (BS_UPD_MODE_GC == b)) ? b : a;
            break;
            
            case BS_UPD_MODE_GU:
                max = (BS_UPD_MODE_GC == b) ? b : a;
            break;
            
            case BS_UPD_MODE_GC:
                max = a;
            break;
        }
    }
    else
    {
        switch ( a )
        {
            case BS_UPD_MODE_DU:
                max = b;
            break;
            
            case BS_UPD_MODE_GC4:
                max = (BS_UPD_MODE_GC16 == b) ? b : a;
            break;

            case BS_UPD_MODE_GC16:
                max = a;
            break;
        }
    }
    
    return ( max );
}

static bool bs_reg_valid(u16 ra)
{
    bool reg_valid = false;
    int i, n;
    
    for ( i = 0, n = sizeof(bs_regs)/sizeof(u16); (i < n) && !reg_valid; i++ )
        if ( ra == bs_regs[i] )
            reg_valid = true;

    return ( reg_valid );    
}

static bool bs_poll_cmd(bs_cmd cmd)
{
    bool poll_cmd = false;
    int i, n;
    
    for ( i = 0, n = sizeof(bs_poll_cmds)/sizeof(bs_cmd); (i < n) && !poll_cmd; i++ )
        if ( cmd == bs_poll_cmds[i] )
            poll_cmd = true;

    return ( poll_cmd );    
}

static bool bs_cmd_queue_empty(void)
{
    return ( bs_cmd_queue_entry == bs_cmd_queue_exit );
}

static int bs_cmd_queue_count(void)
{
    int count = 0;
    
    if ( !bs_cmd_queue_empty() )
    {
        if ( bs_cmd_queue_entry < bs_cmd_queue_exit )
            count = (BS_NUM_CMD_QUEUE_ELEMS - bs_cmd_queue_exit) + bs_cmd_queue_entry;
       else
            count = bs_cmd_queue_entry - bs_cmd_queue_exit;
    }
    
    return ( count );
}

static bs_cmd_queue_elem_t *bs_get_queue_cmd_elem(int i)
{
    bs_cmd_queue_elem_t *result = NULL;

    if ( bs_cmd_queue_count() > i )
    {
        int elem = bs_cmd_queue_exit + i;
        
        if ( bs_cmd_queue_entry < bs_cmd_queue_exit )
            elem %= BS_NUM_CMD_QUEUE_ELEMS;
        
        EINKFB_MEMCPYK(&bs_cmd_elem, &bs_cmd_queue[elem], sizeof(bs_cmd_queue_elem_t));
        result = &bs_cmd_elem;
    }

    return ( result );
}

static void bs_enqueue_cmd(bs_cmd_block_t *bs_cmd_block)
{
    if ( bs_cmd_block )
    {
        einkfb_debug_full("bs_cmd_queue_entry = %03d\n", bs_cmd_queue_entry);
        
        // Enqueue the passed-in command.
        //
        EINKFB_MEMCPYK(&bs_cmd_queue[bs_cmd_queue_entry].bs_cmd_block, bs_cmd_block, sizeof(bs_cmd_block_t));
        bs_cmd_queue[bs_cmd_queue_entry++].time_stamp = FB_GET_CLOCK_TICKS();
    
        // Wrap back to the start if we've reached the end.
        //
        if ( BS_NUM_CMD_QUEUE_ELEMS == bs_cmd_queue_entry )
            bs_cmd_queue_entry = 0;

        // Ensure that we don't accidently go empty.
        //
        if ( bs_cmd_queue_empty() )
        {
            bs_cmd_queue_exit++;
        
            if ( BS_NUM_CMD_QUEUE_ELEMS == bs_cmd_queue_exit )
                bs_cmd_queue_exit = 0;
        }
    }
}

// static bs_cmd_queue_elem_t *bs_dequeue_cmd(void)
// {
//     bs_cmd_queue_elem_t *result = NULL;
//     
//     if ( !bs_cmd_queue_empty() )
//     {
//         einkfb_debug_full("bs_cmd_queue_exit = %03d\n", bs_cmd_queue_exit);
//         
//         // Dequeue the event.
//         //
//         EINKFB_MEMCPYK(&bs_cmd_elem, &bs_cmd_queue[bs_cmd_queue_exit++], sizeof(bs_cmd_queue_elem_t));
//         result = &bs_cmd_elem;
//         
//         // Wrap back to the start if we've reached the end.
//         //
//         if ( BS_NUM_CMD_QUEUE_ELEMS == bs_cmd_queue_exit )
//             bs_cmd_queue_exit = 0;
//     }
//     
//     return ( result );
// }

#define BS_WR_DATA(s, d) bs_wr_dat(BS_WR_DAT_DATA, s, d)
#define BS_WR_ARGS(s, d) bs_wr_dat(BS_WR_DAT_ARGS, s, d)

static int bs_send_cmd(bs_cmd_block_t *bs_cmd_block)
{
    int result = EINKFB_FAILURE;
    
    if ( (broadsheet_ignore_hw_ready() || bs_ready) && bs_cmd_block )
    {
        bs_cmd command = bs_cmd_block->command;
        bs_cmd_type type = bs_cmd_block->type;
        
        // Save the command block.
        //
        bs_enqueue_cmd(bs_cmd_block);
        
        // Send the command.
        //
        result = bs_wr_cmd(command, bs_poll_cmd(command));

        // Send the command's arguments if there are any.
        //
        if ( (EINKFB_SUCCESS == result) && bs_cmd_block->num_args )
            result = BS_WR_ARGS(bs_cmd_block->num_args, bs_cmd_block->args);
        
        // Send the subcommand if there is one.
        //
        if ( (EINKFB_SUCCESS == result) && bs_cmd_block->sub )
            result = bs_send_cmd(bs_cmd_block->sub);

        // Send/receive any data.
        //
        if ( EINKFB_SUCCESS == result )
        {
            u32 data_size = 0;
            u16 *data = NULL;
            
            // If there's any data to send/receive, say so.
            //
            if ( bs_cmd_block->data_size )
            {
                data_size = bs_cmd_block->data_size >> 1;
                data = (u16 *)bs_cmd_block->data;
            }
            else
            {
                if ( bs_cmd_type_read == type )
                {
                    data_size = BS_RD_DAT_ONE;
                    data = &bs_cmd_block->io;
                }
            }
            
            if ( data_size )
            {
                if ( bs_cmd_type_read == type )
                    result = bs_rd_dat(data_size, data);
                else
                    result = BS_WR_DATA(data_size, data);
            }
        }
        
        // The controller appears to have died!
        //
        if ( EINKFB_FAILURE == result )
        {
            // Say that we're no longer ready to accept commands.
            //
            bs_ready = false;
            
            // Dump out the most recent BS_CMD_Q_DEBUG commands.
            //
            einkfb_memset(bs_recent_commands_page, 0, BS_RECENT_CMDS_SIZE);
            
            if ( broadsheet_get_recent_commands(bs_recent_commands_page, BS_CMD_Q_DEBUG) )
                einkfb_print_crit("The last few commands sent to Broadsheet were:\n\n%s\n",
                    bs_recent_commands_page);
                    
            // Reprime the watchdog to get us reset.
            //
            broadsheet_prime_watchdog_timer(EINKFB_DELAY_TIMER);
        }
    }

    return ( result );
}

static inline int BS_SEND_CMD(bs_cmd cmd)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = cmd;
    bs_cmd_block.type     = bs_cmd_type_write;
    
    return ( bs_send_cmd(&bs_cmd_block) );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet Host Interface Command API
    #pragma mark -
    #pragma mark -- System Commands --
#endif

// See AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs_cmd/bs_cmd.cpp.

void bs_cmd_init_cmd_set(u16 arg0, u16 arg1, u16 arg2)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = bs_cmd_INIT_CMD_SET;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_INIT_CMD_SET;
    bs_cmd_block.args[0]  = arg0;
    bs_cmd_block.args[1]  = arg1;
    bs_cmd_block.args[2]  = arg2;
    
    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_init_cmd_set_isis(u32 bc, u8 *data)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command    = bs_cmd_INIT_CMD_SET;
    bs_cmd_block.type       = bs_cmd_type_write;

    bs_cmd_block.data_size  = bc;
    bs_cmd_block.data       = data;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_init_pll_stby(u16 cfg0, u16 cfg1, u16 cfg2)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = bs_cmd_INIT_PLL_STBY;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_INIT_PLL_STBY;
    bs_cmd_block.args[0]  = cfg0;
    bs_cmd_block.args[1]  = cfg1;
    bs_cmd_block.args[2]  = cfg2;
    
    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_run_sys(void)
{
    BS_SEND_CMD(bs_cmd_RUN_SYS);
}

void bs_cmd_stby(void)
{
    BS_SEND_CMD(bs_cmd_STBY);
}

void bs_cmd_slp(void)
{
    BS_SEND_CMD(bs_cmd_SLP);
}

void bs_cmd_init_sys_run(void)
{
    BS_SEND_CMD(bs_cmd_INIT_SYS_RUN);
}

void bs_cmd_init_sys_stby(void)
{
    BS_SEND_CMD(bs_cmd_INIT_SYS_STBY);
}

void bs_cmd_init_sdram(u16 cfg0, u16 cfg1, u16 cfg2, u16 cfg3)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = bs_cmd_INIT_SDRAM;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_INIT_SDRAM;
    bs_cmd_block.args[0]  = cfg0;
    bs_cmd_block.args[1]  = cfg1;
    bs_cmd_block.args[2]  = cfg2;
    bs_cmd_block.args[3]  = cfg3;
    
    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_init_dspe_cfg(u16 hsize, u16 vsize, u16 sdcfg, u16 gfcfg, u16 lutidxfmt)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = bs_cmd_INIT_DSPE_CFG;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_DSPE_CFG;
    bs_cmd_block.args[0]  = hsize;
    bs_cmd_block.args[1]  = vsize;
    bs_cmd_block.args[2]  = sdcfg;
    bs_cmd_block.args[3]  = gfcfg;
    bs_cmd_block.args[4]  = lutidxfmt;
    
    bs_send_cmd(&bs_cmd_block);
    
    bs_hsize = hsize;
    bs_vsize = vsize;
    einkfb_debug("hsize=%d vsize=%d\n", bs_hsize, bs_vsize);
}

void bs_cmd_init_dspe_tmg(u16 fs, u16 fbe, u16 ls, u16 lbe, u16 pixclkcfg)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    
    bs_cmd_block.command  = bs_cmd_INIT_DSPE_TMG;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_DSPE_TMG;
    bs_cmd_block.args[0]  = fs;
    bs_cmd_block.args[1]  = fbe;
    bs_cmd_block.args[2]  = ls;
    bs_cmd_block.args[3]  = lbe;
    bs_cmd_block.args[4]  = pixclkcfg;
    
    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_set_rotmode(u16 rotmode)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_SET_ROTMODE;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_SET_ROTMODE;
    bs_cmd_block.args[0]  = (rotmode & 0x3) << 8;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_init_waveform(u16 wfdev)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_INIT_WAVEFORMDEV;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_INIT_WFDEV;
    bs_cmd_block.args[0]  = wfdev;

    bs_send_cmd(&bs_cmd_block);
}

#if PRAGMAS
    #pragma mark -- Register and Memory Access Commands --
#endif

u16  bs_cmd_rd_reg(u16 ra)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    u16 result = 0;

    bs_cmd_block.command  = bs_cmd_RD_REG;
    bs_cmd_block.type     = bs_cmd_type_read;
    bs_cmd_block.num_args = BS_CMD_ARGS_RD_REG;
    bs_cmd_block.args[0]  = ra;

    if ( EINKFB_SUCCESS == bs_send_cmd(&bs_cmd_block) )
        result = bs_cmd_block.io;
        
    return ( result );
}

void bs_cmd_wr_reg(u16 ra, u16 wd)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_WR_REG;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_WR_REG;
    bs_cmd_block.args[0]  = ra;
    bs_cmd_block.args[1]  = wd;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_rd_sfm(void)
{
    BS_SEND_CMD(bs_cmd_RD_SFM);
}

void bs_cmd_wr_sfm(u8 wd)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    u16 data = wd;

    bs_cmd_block.command  = bs_cmd_WR_SFM;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_WR_SFM;
    bs_cmd_block.args[0]  = data;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_end_sfm(void)
{
    BS_SEND_CMD(bs_cmd_END_SFM);
}

u16 BS_CMD_RD_REG(u16 ra)
{
    u16 result = 0;
    
    if ( bs_reg_valid(ra) )
        result = bs_cmd_rd_reg(ra);
        
    return ( result );
}

void BS_CMD_WR_REG(u16 ra, u16 wd)
{
    if ( bs_reg_valid(ra) )
        bs_cmd_wr_reg(ra, wd);
}

#if PRAGMAS
    #pragma mark -- Burst Access Commands --
#endif

static void bs_cmd_bst_sdr(bs_cmd cmd, u32 ma, u32 bc, u8 *data)
{
    bs_cmd_block_t  bs_cmd_block = { 0 },
                    bs_sub_block = { 0 };

    bs_cmd_block.command    = cmd;
    bs_cmd_block.type       = (bs_cmd_BST_RD_SDR == cmd) ? bs_cmd_type_read : bs_cmd_type_write;
    bs_cmd_block.num_args   = BS_CMD_ARGS_BST_SDR;
    bs_cmd_block.args[0]    = ma & 0xFFFF;
    bs_cmd_block.args[1]    = (ma >> 16) & 0xFFFF;
    bs_cmd_block.args[2]    = bc & 0xFFFF;
    bs_cmd_block.args[3]    = (bc >> 16) & 0xFFFF;

    bs_cmd_block.data_size  = bc;
    bs_cmd_block.data       = data;
    
    bs_cmd_block.sub        = &bs_sub_block;

    bs_sub_block.command    = bs_cmd_WR_REG;
    bs_sub_block.type       = bs_cmd_type_write;
    bs_sub_block.num_args   = BS_CMD_ARGS_WR_REG_SUB;
    bs_sub_block.args[0]    = BS_CMD_BST_SDR_WR_REG;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_bst_rd_sdr(u32 ma, u32 bc, u8 *data)
{
    bs_cmd_bst_sdr(bs_cmd_BST_RD_SDR, ma, bc, data);
}

void bs_cmd_bst_wr_sdr(u32 ma, u32 bc, u8 *data)
{
    bs_cmd_bst_sdr(bs_cmd_BST_WR_SDR, ma, bc, data);
}

void bs_cmd_bst_end(void)
{
    BS_SEND_CMD(bs_cmd_BST_END);
}

#if PRAGMAS
    #pragma mark -- Image Loading Commands --
#endif

static void bs_cmd_ld_img_which(bs_cmd cmd, u16 dfmt, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd_block_t  bs_cmd_block = { 0 },
                    bs_sub_block = { 0 };
    u16 arg = (dfmt & 0x03) << 4;
    
    bs_cmd_block.command  = cmd;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.args[0]  = arg;
    
    if ( bs_cmd_LD_IMG_AREA == cmd )
    {
        bs_cmd_block.num_args = BS_CMD_ARGS_LD_IMG_AREA;
    
        bs_cmd_block.args[1]  = x;
        bs_cmd_block.args[2]  = y;
        bs_cmd_block.args[3]  = w;
        bs_cmd_block.args[4]  = h;
    }
    else
        bs_cmd_block.num_args = BS_CMD_ARGS_LD_IMG;
    
    bs_cmd_block.sub          = &bs_sub_block;
    
    bs_sub_block.command      = bs_cmd_WR_REG;
    bs_sub_block.type         = bs_cmd_type_write;
    bs_sub_block.num_args     = BS_CMD_ARGS_WR_REG_SUB;
    bs_sub_block.args[0]      = BS_CMD_LD_IMG_WR_REG;
    
    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_ld_img(u16 dfmt)
{
    bs_cmd_ld_img_which(bs_cmd_LD_IMG, dfmt, 0, 0, 0, 0);
}

void bs_cmd_ld_img_area(u16 dfmt, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd_ld_img_which(bs_cmd_LD_IMG_AREA, dfmt, x, y, w, h);
}

void bs_cmd_ld_img_end(void)
{
    BS_SEND_CMD(bs_cmd_LD_IMG_END);
}

void bs_cmd_ld_img_wait(void)
{
    BS_SEND_CMD(bs_cmd_LD_IMG_WAIT);
}

#if PRAGMAS
    #pragma mark -- Polling Commands --
#endif

void bs_cmd_wait_dspe_trg(void)
{
    BS_SEND_CMD(bs_cmd_WAIT_DSPE_TRG);
}

void bs_cmd_wait_dspe_frend(void)
{
    BS_SEND_CMD(bs_cmd_WAIT_DSPE_FREND);
}

void bs_cmd_wait_dspe_lutfree(void)
{
    BS_SEND_CMD(bs_cmd_WAIT_DSPE_LUTFREE);
}

void bs_cmd_wait_dspe_mlutfree(u16 lutmsk)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_WAIT_DSPE_MLUTFREE;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_DSPE_MLUTFREE;
    bs_cmd_block.args[0]  = lutmsk;

    bs_send_cmd(&bs_cmd_block);
}

#if PRAGMAS
    #pragma mark -- Waveform Update Commands --
#endif

void bs_cmd_rd_wfm_info(u32 ma)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_RD_WFM_INFO;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_RD_WFM_INFO;
    bs_cmd_block.args[0]  = ma & 0xFFFF;
    bs_cmd_block.args[1]  = (ma >> 16) & 0xFFFF;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_upd_init(void)
{
    BS_SEND_CMD(bs_cmd_UPD_INIT);
}

static void bs_cmd_upd(bs_cmd cmd, u16 mode, u16 lutn, u16 bdrupd, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd_block_t bs_cmd_block = { 0 };
    u16 arg = ((mode & 0xF) << 8) | ((lutn & 0xF) << 4) | ((bdrupd & 0x1) << 14);

    bs_cmd_block.command  = cmd;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.args[0]  = arg;

    if ( (bs_cmd_UPD_FULL_AREA == cmd) || (bs_cmd_UPD_PART_AREA == cmd) )
    {
        bs_cmd_block.num_args = BS_CMD_UPD_AREA_ARGS;
        
        bs_cmd_block.args[1]  = x;
        bs_cmd_block.args[2]  = y;
        bs_cmd_block.args[3]  = w;
        bs_cmd_block.args[4]  = h;
    }
    else
        bs_cmd_block.num_args = BS_CMD_UPD_ARGS;

    bs_send_cmd(&bs_cmd_block);
}

void bs_cmd_upd_full(u16 mode, u16 lutn, u16 bdrupd)
{
    bs_cmd_upd(bs_cmd_UPD_FULL, mode, lutn, bdrupd, 0, 0, 0, 0);
}

void bs_cmd_upd_full_area(u16 mode, u16 lutn, u16 bdrupd, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd_upd(bs_cmd_UPD_FULL_AREA, mode, lutn, bdrupd, x, y, w, h);
}

void bs_cmd_upd_part(u16 mode, u16 lutn, u16 bdrupd)
{
    bs_cmd_upd(bs_cmd_UPD_PART, mode, lutn, bdrupd, 0, 0, 0, 0);
}

void bs_cmd_upd_part_area(u16 mode, u16 lutn, u16 bdrupd, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd_upd(bs_cmd_UPD_PART_AREA, mode, lutn, bdrupd, x, y, w, h);
}

void bs_cmd_upd_gdrv_clr(void)
{
    BS_SEND_CMD(bs_cmd_UPD_GDRV_CLR);
}

void bs_cmd_upd_set_imgadr(u32 ma)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_UPD_SET_IMGADR;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_UPD_SET_IMGADR;
    bs_cmd_block.args[0]  = ma & 0xFFFF;
    bs_cmd_block.args[1]  = (ma >> 16) & 0xFFFF;

    bs_send_cmd(&bs_cmd_block);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark SPI Flash Interface API
    #pragma mark -
#endif

#define BSC_RD_REG(a)       bs_cmd_rd_reg(a)
#define BSC_WR_REG(a, d)    bs_cmd_wr_reg(a, d)

struct bit_ready_t
{
    int address,
        position,
        value;
};
typedef struct bit_ready_t bit_ready_t;

static bool bs_sfm_bit_ready(void *data)
{
    bit_ready_t *br = (bit_ready_t *)data;

    return ( ((BSC_RD_REG(br->address) >> br->position) & 0x1) == (br->value & 0x1) );
}

static void bs_sfm_wait_for_bit(int ra, int pos, int val)
{
    bit_ready_t br = { ra, pos, val };
    einkfb_schedule_timeout(BS_SFM_TIMEOUT, bs_sfm_bit_ready, (void *)&br);
}

bool bs_sfm_preflight(void)
{
    bool preflight = true;
    
    if ( !BS_ISIS() )
    {
        bs_sfm_start();
        
        switch ( bs_sfm_esig() )
        {
            case BS_SFM_ESIG_M25P10:
                bs_sfm_sector_size  = BS_SFM_SECTOR_SIZE_128K;
                bs_sfm_size         = BS_SFM_SIZE_128K;
                bs_sfm_page_count   = BS_SFM_PAGE_COUNT_128K;
                bs_tst_addr         = BS_TST_ADDR_128K;
            break;
            
            case BS_SFM_ESIG_M25P20:
                bs_sfm_sector_size  = BS_SFM_SECTOR_SIZE_256K;
                bs_sfm_size         = BS_SFM_SIZE_256K;
                bs_sfm_page_count   = BS_SFM_PAGE_COUNT_256K;
                bs_tst_addr         = BS_TST_ADDR_256K;
            break;
            
            default:
                einkfb_print_error("Unrecognized flash signature\n");
                preflight = false;
            break;
        }
        
        bs_sfm_end();
    }
    
    return ( preflight );
}

int bs_get_sfm_size(void)
{
    return ( bs_sfm_size );
}

void bs_sfm_start(void)
{
    int access_mode = 0, enable = 1, v;
    
    // If we're not in bootstrapping mode (where we aren't actually using
    // the display), ensure that any display transactions finish before
    // we start flashing, including any that might result from the
    // watchdog.
    //
    if ( !bs_bootstrap )
    {
        broadsheet_prime_watchdog_timer(EINKFB_EXPIRE_TIMER);
        
        bs_cmd_wait_dspe_trg();
        bs_cmd_wait_dspe_frend();
    }
    
    sfm_cd = BSC_RD_REG(0x0204);  // spi flash control reg
    BSC_WR_REG(0x0208, 0);
    BSC_WR_REG(0x0204, 0);        // disable
    
    v = (access_mode           << 7) |
        (SFM_READ_COMMAND      << 6) |
        (SFM_CLOCK_DIVIDE      << 3) |
        (SFM_CLOCK_PHASE       << 2) |
        (SFM_CLOCK_POLARITY    << 1) |
        (enable                << 0);
    BSC_WR_REG(0x0204, v);

    einkfb_debug_full("... staring sfm access\n");
    einkfb_debug_full("... esig=0x%02X\n", bs_sfm_esig());
}

void bs_sfm_end(void)
{
    einkfb_debug_full( "... ending sfm access\n");
    BSC_WR_REG(0x0204, sfm_cd);
}

void bs_sfm_wr_byte(int data)
{
    int v = (data & 0xFF) | 0x100;
    BSC_WR_REG(0x202, v);
    bs_sfm_wait_for_bit(0x206, 3, 0);
}

int bs_sfm_rd_byte(void)
{
    int v;
    
    BSC_WR_REG(0x202, 0);
    bs_sfm_wait_for_bit(0x206, 3, 0);
    v = BSC_RD_REG(0x200);
    return ( v & 0xFF );
}

int bs_sfm_esig( void )
{
    int es;
    
    BSC_WR_REG(0x208, 1);
    bs_sfm_wr_byte(BS_SFM_RES);
    bs_sfm_wr_byte(0);
    bs_sfm_wr_byte(0);
    bs_sfm_wr_byte(0);
    es = bs_sfm_rd_byte();
    BSC_WR_REG(0x208, 0);
    return ( es );
}

void bs_sfm_read(int addr, int size, char * data)
{
    int i;
    
    einkfb_debug_full( "... reading the serial flash memory (address=0x%08X, size=%d)\n", addr, size );
    
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_READ);
    bs_sfm_wr_byte(( addr >> 16 ) & 0xFF);
    bs_sfm_wr_byte(( addr >>  8 ) & 0xFF);
    bs_sfm_wr_byte(addr & 0xFF);
    
    for ( i = 0; i < size; i++ )
        data[i] = bs_sfm_rd_byte();
    
    BSC_WR_REG(0x0208, 0);
}

static void bs_sfm_write_enable(void)
{
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_WREN);
    BSC_WR_REG(0x0208, 0);
}

void bs_sfm_write_disable(void)
{
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_WRDI);
    BSC_WR_REG(0x0208, 0);
}

static int bs_sfm_read_status(void)
{
    int s;
    
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_RDSR);
    s = bs_sfm_rd_byte();
    BSC_WR_REG(0x0208, 0);
  
    return ( s );
}

static bool bs_sfs_read_ready(void *unused)
{
    return ( (bs_sfm_read_status() & 0x1) == 0 );
}

static void bs_sfm_erase(int addr)
{
    einkfb_debug_full( "... erasing sector (0x%08X)\n", addr);
  
    bs_sfm_write_enable();
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_SE);
    bs_sfm_wr_byte((addr >> 16) & 0xFF);
    bs_sfm_wr_byte((addr >>  8) & 0xFF);
    bs_sfm_wr_byte( addr & 0xFF);
    BSC_WR_REG(0x0208, 0);
  
    EINKFB_SCHEDULE_TIMEOUT(BS_SFM_TIMEOUT, bs_sfs_read_ready);
    bs_sfm_write_disable();
}

static void bs_sfm_program_page(int pa, int size, char *data)
{
    int d;
    
    bs_sfm_write_enable();
    BSC_WR_REG(0x0208, 1);
    bs_sfm_wr_byte(BS_SFM_PP);
    bs_sfm_wr_byte((pa >> 16) & 0xFF);
    bs_sfm_wr_byte((pa >>  8) & 0xFF);
    bs_sfm_wr_byte(pa & 0xFF);
    
    for ( d = 0; d < BS_SFM_PAGE_SIZE; d++ )
        bs_sfm_wr_byte(data[d]);

    BSC_WR_REG(0x0208, 0);
  
    EINKFB_SCHEDULE_TIMEOUT(BS_SFM_TIMEOUT, bs_sfs_read_ready);
    bs_sfm_write_disable();
}

static void bs_sfm_program_sector(int sa, int size, char *data)
{
    int p, y, pa = sa;

    einkfb_debug_full( "... programming sector (0x%08X)\n", sa);
  
    for ( p = 0; p < bs_sfm_page_count; p++ )
    {
        y = p * BS_SFM_PAGE_SIZE;
        bs_sfm_program_page(pa, BS_SFM_PAGE_SIZE, &data[y]);
        pa += BS_SFM_PAGE_SIZE;
    }
}

static void BS_SFM_WRITE(int addr, int size, char *data)
{
    int s1 = addr/bs_sfm_sector_size,
        s2 = (addr + size - 1)/bs_sfm_sector_size,
        x, i, s, sa, start, limit, count;

    for ( x = 0, s = s1; s <= s2; s++ )
    {
        sa    = s * bs_sfm_sector_size;
        start = 0;
        count = bs_sfm_sector_size;
        
        if ( s == s1 )
        {
            if ( addr > sa )
            {
	            start = addr - sa;
	            bs_sfm_read(sa, start, sd);
            }
        }
        
        if ( s == s2 )
        {
            limit = addr + size;
            
            if ( (sa + bs_sfm_sector_size) > limit )
            {
	            count = limit - sa;
	            bs_sfm_read(limit, (sa + bs_sfm_sector_size - limit), &sd[count]);
            }
        }
    
        bs_sfm_erase(sa);
        
        for ( i = start; i < count; i++ )
            sd[i] = data[x++];

        bs_sfm_program_sector(sa, bs_sfm_sector_size, sd);
    }
}

void bs_sfm_write(int addr, int size, char *data)
{
    int i, retry_count = 0;
    bool valid;
    
    do
    {
        einkfb_debug_full( "... writing the serial flash memory (address=0x%08X, size=%d, count=%d)\n",
            addr, size, retry_count);
        einkfb_memset(sd, 0, bs_sfm_size);
        BS_SFM_WRITE(addr, size, data);
    
        einkfb_debug_full( "... verifying the serial flash memory write\n");
        einkfb_memset(rd, 0, bs_sfm_size);
    
        bs_sfm_read(addr, size, rd);
    
        for ( i = 0, valid = true; (i < size) && valid; i++ )
        {
            if ( rd[i] != data[i] )
            {
                einkfb_debug_full( "+++++++++++++++ rd[%d]=0x%02x  data[%d]=0x%02x\n", i, rd[i], i, data[i]);
                valid = false;
                retry_count++;
            }
        }
    }  
    while ( !valid && (BS_SFM_MAX_RETRY_COUNT > retry_count) );

    einkfb_debug_full( "... writing the serial flash memory --- done\n");
    
    if ( !valid )
        einkfb_print_crit("Failed to verify write of Broadsheet Flash; retry_count = %d.\n",
            retry_count);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet Host Interface Helper API
    #pragma mark -
#endif

// See AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs_cmd/bs_cmd.h.

#define bs_cmd_bit_ready bs_sfm_bit_ready

void bs_cmd_wait_for_bit(int reg, int bitpos, int bitval)
{
    bit_ready_t br = { reg, bitpos, bitval };
    einkfb_schedule_timeout(BS_CMD_TIMEOUT, bs_cmd_bit_ready, (void *)&br);
}

static u16 bs_cmd_bpp_to_dfmt(u32 bpp)
{
    u16 result;
    
    switch ( bpp )
    {
        case EINKFB_2BPP:
            result = BS_DFMT_2BPP;
        break;
        
        case EINKFB_4BPP:
            result = BS_DFMT_4BPP;
        break;
        
        case EINKFB_8BPP:
        default:
            result = BS_DFMT_8BPP;
        break;
    }

    return ( result );
}

enum bs_ld_img_data_t
{
    bs_ld_img_data_fill,
    bs_ld_img_data_fast,
    bs_ld_img_data_slow
};
typedef enum bs_ld_img_data_t bs_ld_img_data_t; 

static u16 bs_ld_img_data(u8 *data, bool upd_area, u16 x, u16 y, u16 w, u16 h)
{
    bs_ld_img_data_t bs_ld_img_data_which = bs_ld_img_data_slow;
    u16 upd_mode, dfmt;
    int i, data_size;

    struct einkfb_info info;
    einkfb_get_info(&info);
    
    data_size = bs_ld_fb ? BPP_SIZE((w * h), EINKFB_8BPP) : BPP_SIZE((w * h), info.bpp);
    dfmt = bs_ld_fb ? bs_cmd_bpp_to_dfmt(EINKFB_8BPP) : bs_cmd_bpp_to_dfmt(EINKFB_4BPP);

    if ( upd_area )
        bs_cmd_ld_img_area(dfmt, x, y, w, h);
    else
        bs_cmd_ld_img(dfmt);
 
    bs_wr_one_ready();
    
    // Say which image-loading type we are:  Simple fill or fast (no upd_mode overrides
    // and we're using ISIS's auto-waveform upd_mode selection capabilities).  Otherwise,
    // we just use the default (slow) image-loading methodology.
    //
    if ( bs_ld_fb )
        bs_ld_img_data_which = bs_ld_img_data_fill;
    else
    {
        u16 override_upd_mode = (u16)broadsheet_get_override_upd_mode();
        
        if ( (BS_UPD_MODE_INIT == override_upd_mode) && BS_ISIS() )
            bs_ld_img_data_which = bs_ld_img_data_fast;
    }
    
    bs_ld_img_start = jiffies;
 
    switch ( bs_ld_img_data_which )
    {
        case bs_ld_img_data_fill:
        {
            u16 controller_data = BS_ISIS() ? *((u16 *)data) : ~*((u16 *)data);
            data_size >>= 1;
            
            for ( i = 0; i < data_size; i++ )
            {
                bs_wr_one(controller_data);
                EINKFB_SCHEDULE_BLIT(((i+1) << 1));
            }
    
            upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
        }
        break;
        
        case bs_ld_img_data_fast:
        {
            u16 *controller_data = (u16 *)data;
            data_size >>= 1;
            
            for ( i = 0; i < data_size; i++ )
            {
                bs_wr_one(controller_data[i]);
                EINKFB_SCHEDULE_BLIT(((i+1) << 1));
            }
            
            upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
        }
        break;
        
        case bs_ld_img_data_slow:
        {
            u8 einkfb_white = BS_ISIS() ? EINKFB_WHITE : ~EINKFB_WHITE,
               *controller_data = broadsheet_get_scratchfb();
            int j, k, l = 0, m = 0, n = 0;
            unsigned char s, pixels[2];
    
            upd_mode = BS_INIT_UPD_MODE();
    
            // Determine the hardware slop factors for non-area updates.
            //
            if ( !upd_area )
            {
                switch ( broadsheet_get_orientation() )
                {
                    case EINKFB_ORIENT_PORTRAIT:
                        if ( (BS97_INIT_VSIZE == bs_vsize) && (BS97_SOFT_VSIZE == w) )
                            m = BS97_HARD_VSIZE - BS97_SOFT_VSIZE;
                    break;
                    
                    case EINKFB_ORIENT_LANDSCAPE:
                        if ( (BS97_INIT_VSIZE == bs_vsize) && (BS97_SOFT_VSIZE == h) )
                            n = (BS97_HARD_VSIZE - BS97_SOFT_VSIZE) * w;
                    break;
                }
            }
            
            // Make BPP-specific adjustments.
            //
            w = BPP_SIZE(w, info.bpp);
            
            switch ( info.bpp )
            {
                case EINKFB_4BPP:
                    l = 1;
                break;
                
                case EINKFB_2BPP:
                    l = 2;
                break;
            }
            
            for ( i = j = 0; i < data_size; )
            {
                // First, get the source pixel(s).
                //
                s = data[i];
                
                // Next, if they're not already at 4bpp, get them there.
                //
                switch ( info.bpp )
                {
                    case EINKFB_4BPP:
                        pixels[0] = s;
                    break;
                    
                    case EINKFB_2BPP:
                        pixels[0] = STRETCH_HI_NYBBLE(s, info.bpp);
                        pixels[1] = STRETCH_LO_NYBBLE(s, info.bpp);
                    break;
                }
        
                // Now, get the 4bpp pixels to their destination in the right
                // format.
                //
                for ( k = 0; k < l; k++ )
                {
                    // Get pixels.
                    //
                    s = pixels[k];
                    
                    // Accumulate the update mode. 
                    //
                    upd_mode = BS_FIND_UPD_MODE(upd_mode, s, info.bpp);
                    
                    // Invert and nybble-swap the pixels going out to the controller on Broadsheet
                    // but not on ISIS.
                    //
                    controller_data[j++] = BS_ISIS() ? s : bs_4bpp_nybble_swap_table_inverted[s];
    
                    if ( 0 == (j % 2) )
                        bs_wr_one(*((u16 *)&controller_data[j-2]));
                }
        
                // Finally, determine what's next.
                //
                if ( 0 == (++i % w) )
                {
                    // Handle the horizontal hardware slop factor when necessary.
                    //
                    for ( k = 0; k < m; k++ )
                    {
                        controller_data[j++] = einkfb_white;
                        
                        if ( 0 == (j % 2) )
                            bs_wr_one(*((u16 *)&controller_data[j-2]));
                    }
                }
            
                EINKFB_SCHEDULE_BLIT(i+1);
            }
            
            // Handle the vertical slop factor when necessary.
            //
            for ( k = 0; k < n; k++ )
            {
                controller_data[j++] = einkfb_white;
                
                if ( 0 == (j % 2) )
                    bs_wr_one(*((u16 *)&controller_data[j-2]));
            }
            
            upd_mode = BS_DONE_UPD_MODE(upd_mode);
        }
        break;
    }

    bs_cmd_ld_img_end();

    bs_debug_upd_mode(upd_mode);
    
    return ( upd_mode );
}

static u16 bs_cmd_orientation_to_rotmode(bool orientation, bool upside_down)
{
    u16 result = BS_ROTMODE_180;
    
    if ( EINKFB_ORIENT_PORTRAIT == orientation )
        result = BS_ROTMODE_90;
        
    // The ADS (FPGA) platform is always upside down with respect to everything else.
    // So, fix that here.
    //
    if ( IS_ADS() )
    {
        switch ( result )
        {
            case BS_ROTMODE_180:
                result = BS_ROTMODE_0;
            break;
            
            case BS_ROTMODE_90:
                result = BS_ROTMODE_270;
            break;
        }
    }
    
    // Flip everything upside down if we're supposed to.
    //
    if ( upside_down )
    {
        switch ( result )
        {
            case BS_ROTMODE_0:
                result = BS_ROTMODE_180;
            break;
            
            case BS_ROTMODE_90:
                result = BS_ROTMODE_270;
            break;
            
            case BS_ROTMODE_180:
                result = BS_ROTMODE_0;
            break;
            
            case BS_ROTMODE_270:
                result = BS_ROTMODE_90;
            break;
        }
    }
        
    return ( result );
}

static bool bs_disp_ready(void *unused)
{
    int vsize = bs_cmd_rd_reg(0x300),
        hsize = bs_cmd_rd_reg(0x306);

    return ( (hsize == bs_hsize) && (vsize == bs_vsize) );
}

#define BS_DISP_READY() bs_disp_ready(NULL)

void bs_cmd_print_disp_timings(void)
{
    int vsize = bs_cmd_rd_reg(0x300),
        vsync = bs_cmd_rd_reg(0x302),
        vblen = bs_cmd_rd_reg(0x304),
        velen = (vblen >> 8) & 0xFF,
        hsize = bs_cmd_rd_reg(0x306),
        hsync = bs_cmd_rd_reg(0x308),
        hblen = bs_cmd_rd_reg(0x30A),
        helen = (hblen >> 8) & 0xFF;

    vblen &= 0xFF;
    hblen &= 0xFF;

    if ( !BS_DISP_READY() )
    {
        EINKFB_SCHEDULE_TIMEOUT(BS_CMD_TIMEOUT, bs_disp_ready);
        
        vsize = bs_cmd_rd_reg(0x300);
        hsize = bs_cmd_rd_reg(0x306);
    }

    einkfb_debug("disp_timings: vsize=%d vsync=%d vblen=%d velen=%d\n", vsize, vsync, vblen, velen);
    einkfb_debug("disp_timings: hsize=%d hsync=%d hblen=%d helen=%d\n", hsize, hsync, hblen, helen);
}

void bs_cmd_set_wfm(int addr)
{
    bs_cmd_rd_wfm_info(addr);
    bs_cmd_wait_dspe_trg();
}

void bs_cmd_get_wfm_info(void)
{
  u16   addr_base = BS_ISIS() ? 0x390 : 0x350,
        a = bs_cmd_rd_reg(addr_base+0x4),
        b = bs_cmd_rd_reg(addr_base+0x6),
        c = bs_cmd_rd_reg(addr_base+0x8),
        d = bs_cmd_rd_reg(addr_base+0xC),
        e = bs_cmd_rd_reg(addr_base+0xE);

  wfm_fvsn  = a & 0xFF;
  wfm_luts  = (a >> 8) & 0xFF;
  wfm_trc   = (b >> 8) & 0xFF;
  wfm_mc    = b & 0xFF;
  wfm_sb    = (c >> 8) & 0xFF;
  wfm_eb    = c & 0xFF;
  wfm_wmta  = d | (e << 16);
}

void bs_cmd_print_wfm_info(void)
{
    if ( EINKFB_DEBUG() )
    {
        bs_cmd_get_wfm_info();

        einkfb_print_info("wfm: fvsn=%d luts=%d mc=%d trc=%d eb=0x%02x sb=0x%02x wmta=%d\n",
            wfm_fvsn, wfm_luts, wfm_mc, wfm_trc, wfm_eb, wfm_sb, wfm_wmta);
    }
}

void bs_cmd_clear_gd(void)
{
    bs_cmd_upd_gdrv_clr();
    bs_cmd_wait_dspe_trg();
    bs_cmd_wait_dspe_frend();
}

u32 bs_cmd_get_sdr_img_base(void)
{
    return ( ((bs_cmd_rd_reg(BS_SDR_IMG_MSW_REG) & 0xFFFF) << 16) | (bs_cmd_rd_reg(BS_SDR_IMG_LSW_REG) & 0xFFFF) );
}

static void bs_cmd_sdr(bs_cmd cmd, u32 ma, u32 bc, u8 *data)
{
    if ( bs_cmd_BST_RD_SDR == cmd )
        bs_cmd_bst_rd_sdr(ma, bc, data);
    else
        bs_cmd_bst_wr_sdr(ma, bc, data);
    
    bs_cmd_bst_end();
}

void bs_cmd_rd_sdr(u32 ma, u32 bc, u8 *data)
{
    bs_cmd_sdr(bs_cmd_BST_RD_SDR, ma, bc, data);
}

void bs_cmd_wr_sdr(u32 ma, u32 bc, u8 *data)
{
    bs_cmd_sdr(bs_cmd_BST_WR_SDR, ma, bc, data);
}

int bs_cmd_get_lut_auto_sel_mode(void) 
{
    int v = bs_cmd_rd_reg(0x330);
    return ( (v >> 7) & 0x1 );
}

void bs_cmd_set_lut_auto_sel_mode(int v)
{
    int d = bs_cmd_rd_reg(0x330);
    
    if ( v & 0x1 )
        d |= 0x80;
    else
        d &= ~0x80;
    
    bs_cmd_wr_reg(0x330, d);
}

int bs_cmd_get_wf_auto_sel_mode(void) 
{
    int v = bs_cmd_rd_reg(0x330);
    return ( (v >> 6) & 0x1 );
}

void bs_cmd_set_wf_auto_sel_mode(int v)
{
    int d = bs_cmd_rd_reg(0x330);
    
    if ( v & 0x1 )
        d |= 0x40;
    else
        d &= ~0x40;
    
    bs_cmd_wr_reg(0x330, d);
}

// Lab126
//
#define UPD_MODE_INIT(u)                                    \
    ((BS_UPD_MODE_INIT   == (u))   ||                       \
     (BS_UPD_MODE_REINIT == (u)))

#define IMAGE_TIMING_STRT_TYPE  0
#define IMAGE_TIMING_PROC_TYPE  1
#define IMAGE_TIMING_LOAD_TYPE  2
#define IMAGE_TIMING_DISP_TYPE  3
#define IMAGE_TIMING_STOP_TYPE  4

#define IMAGE_TIMING_STRT_NAME  "strt"
#define IMAGE_TIMING_PROC_NAME  "proc"
#define IMAGE_TIMING_LOAD_NAME  "load"
#define IMAGE_TIMING_DISP_NAME  "disp"

#define EINKFB_PRINT_IMAGE_TIMING_REL(t, n)                 \
    einkfb_print_perf("id=image_timing,time=%ld,type=relative:%s\n", t, n)

#define EINKFB_PRINT_IMAGE_TIMING_ABS(t)                    \
    einkfb_print_perf("id=image_timing,time=%ld,type=absolute\n", t)

static void einkfb_print_image_timing(unsigned long time, int which)
{
    char *name = NULL;
    
    switch ( which )
    {
        case IMAGE_TIMING_STRT_TYPE:
            name = IMAGE_TIMING_STRT_NAME;
        goto relative_common;
        
        case IMAGE_TIMING_PROC_TYPE:
            name =  IMAGE_TIMING_PROC_NAME;
        goto relative_common;
        
        case IMAGE_TIMING_LOAD_TYPE:
            name =  IMAGE_TIMING_LOAD_NAME;
        goto relative_common;

        case IMAGE_TIMING_DISP_TYPE:
            name =  IMAGE_TIMING_DISP_NAME;
        /* goto relative_common; */
            
        relative_common:
            EINKFB_PRINT_IMAGE_TIMING_REL(time, name);
        break;
        
        case IMAGE_TIMING_STOP_TYPE:
            EINKFB_PRINT_IMAGE_TIMING_ABS(time);
        break;
    }
}

static void bs_cmd_ld_img_upd_data_which(bs_cmd cmd, fx_type update_mode, u8 *data, u16 x, u16 y, u16 w, u16 h)
{
    if ( data )
    {
        bool    upd_area = (bs_cmd_LD_IMG_AREA == cmd),
                wait_dspe_frend = false;
        u16     upd_mode = bs_upd_mode;
        struct  einkfb_info info;

        // Set up to process and load the image data.
        //
        bs_set_ld_img_start = jiffies;
                        
        if ( upd_area )
        {
            // On area-updates, stall the LUT pipeline only on flashing updates.
            //
            if ( UPDATE_AREA_FULL(update_mode) )
                bs_cmd_wait_dspe_frend();
        }
        else
        {
            // Always stall the LUT pipeline on full-screen updates.
            //
            bs_cmd_wait_dspe_frend();
        }

        // Load the image data into the controller, determining what the non-flashing
        // upd_mode would be.
        //
        bs_upd_mode = bs_ld_img_data(data, upd_area, x, y, w, h);

        // Set up to display the just-loaded image data.
        //
        if ( upd_area )
        {
            if ( !UPDATE_AREA_FULL(update_mode) )
            {
                // On non-flashing updates, start accumulating the area that will need
                // repair when we're not stalling the LUT pipeline (i.e., Broadsheet
                // will be hitting the panel faster than the panel can respond).
                //
                if ( 0 == bs_upd_repair_count )
                {
                    bs_upd_repair_x1 = x;
                    bs_upd_repair_y1 = y;
                    bs_upd_repair_x2 = x + w;
                    bs_upd_repair_y2 = y + h;
                
                    bs_upd_repair_mode = bs_upd_mode;
                    bs_upd_repair_skipped = false;
                }
                else
                {
                    bs_upd_repair_x1   = min(x, bs_upd_repair_x1);
                    bs_upd_repair_y1   = min(y, bs_upd_repair_y1);
                    bs_upd_repair_x2   = max((u16)(x + w), bs_upd_repair_x2);
                    bs_upd_repair_y2   = max((u16)(y + h), bs_upd_repair_y2);
                    
                    // This update mode doesn't take into account the accumulated
                    // area.  We'll address that in the repair.
                    //
                    bs_upd_repair_mode = bs_upd_mode_max(bs_upd_mode, bs_upd_repair_mode);
                }
                
                // Indicate that we'll be needing a repair.
                //
                bs_upd_repair_count++;
            }
        }
        else
        {
            // Don't do the update repair since, in effect, we'll be repairing it now.
            //
            bs_upd_repair_count = 0;
        }
        
        broadsheet_prime_watchdog_timer(bs_upd_repair_count ? EINKFB_DELAY_TIMER : EINKFB_EXPIRE_TIMER);
                
        // Update the display in the specified way (upd_mode || bs_upd_mode).
        //
        bs_upd_data_start = jiffies;
        
        if ( UPD_MODE_INIT(upd_mode) )
        {
            bs_cmd_wait_dspe_trg();
            bs_cmd_wait_dspe_frend();
            
            switch ( upd_mode )
            {
                case BS_UPD_MODE_REINIT:
                    bs_cmd_upd_part(BS_UPD_MODE(BS_UPD_MODE_GC), 0, 0);
                break;
                
                case BS_UPD_MODE_INIT:
                    bs_cmd_upd_full(BS_UPD_MODE_INIT, 0, 0);
                break;
            }

            wait_dspe_frend = true;
        }
        else
        {
            u16 override_upd_mode = (u16)broadsheet_get_override_upd_mode();
            
            if ( BS_UPD_MODE_INIT != override_upd_mode )
            {
                bs_upd_repair_mode = bs_upd_mode = BS_UPD_MODE(override_upd_mode);
                
                if ( BS_ISIS() )
                    bs_cmd_set_wf_auto_sel_mode(0);
                
                einkfb_debug("overriding upd_mode\n");
                bs_debug_upd_mode(bs_upd_mode);
            }
            else
            {
                if ( BS_ISIS() )
                    bs_cmd_set_wf_auto_sel_mode(1);
            }
            
            if ( upd_area )
            {
                if ( UPDATE_AREA_FULL(update_mode) )
                {
                    bs_cmd_upd_full_area(bs_upd_mode, 0, 0, x, y, w, h);
                    wait_dspe_frend = true;
                }
                else
                    bs_cmd_upd_part_area(bs_upd_repair_mode, 0, 0,
                        bs_upd_repair_x1, bs_upd_repair_y1,
                        (bs_upd_repair_x2 - bs_upd_repair_x1),
                        (bs_upd_repair_y2 - bs_upd_repair_y1));
            }
            else
            {
                if ( UPDATE_FULL(update_mode) )
                {
                    bs_cmd_upd_full(bs_upd_mode, 0, 0);
                    wait_dspe_frend = true;
                }
                else
                    bs_cmd_upd_part(bs_upd_mode, 0, 0);
            }
            
            einkfb_debug("actual upd_mode\n");
            bs_debug_upd_mode(BS_LAST_WF_USED(BS_CMD_RD_REG(BS_UPD_BUF_CFG_REG)));
        }

        // Done.
        //
        bs_cmd_wait_dspe_trg();
        
        // Stall the LUT pipeline in all full-refresh situations (i.e., prevent any potential
        // partial-refresh parallel updates from stomping on the full-refresh we just
        // initiated).
        //
        if ( wait_dspe_frend )
            bs_cmd_wait_dspe_frend();

        bs_upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
        
        // Say how long it took to perform various image-related operations.
        //
        bs_image_stop_time = jiffies;
        einkfb_get_info(&info);

        bs_image_start_time      = jiffies_to_msecs(bs_set_ld_img_start - info.jif_on);
        bs_image_processing_time = jiffies_to_msecs(bs_ld_img_start     - bs_set_ld_img_start);
        bs_image_loading_time    = jiffies_to_msecs(bs_upd_data_start   - bs_ld_img_start);
        bs_image_display_time    = jiffies_to_msecs(bs_image_stop_time  - bs_upd_data_start);
        bs_image_stop_time       = jiffies_to_msecs(bs_image_stop_time  - info.jif_on);

        if ( EINKFB_PERF() )
        {
            einkfb_print_image_timing(bs_image_start_time,      IMAGE_TIMING_STRT_TYPE);
            einkfb_print_image_timing(bs_image_processing_time, IMAGE_TIMING_PROC_TYPE);
            einkfb_print_image_timing(bs_image_loading_time,    IMAGE_TIMING_LOAD_TYPE);
            einkfb_print_image_timing(bs_image_display_time,    IMAGE_TIMING_DISP_TYPE);
            einkfb_print_image_timing(bs_image_stop_time,       IMAGE_TIMING_STOP_TYPE);
        }
    }
}

void bs_cmd_ld_img_upd_data(fx_type update_mode, bool restore)
{
    struct einkfb_info info;
    u8 *buffer;
    
    einkfb_get_info(&info);
    
    if ( restore )
        buffer = info.vfb;
    else
        buffer = info.start;
    
    bs_cmd_ld_img_upd_data_which(bs_cmd_LD_IMG, update_mode, buffer, 0, 0, info.xres, info.yres);
}

void BS_CMD_LD_IMG_UPD_DATA(fx_type update_mode)
{
    bs_cmd_ld_img_upd_data(update_mode, UPD_DATA_NORMAL);
}

void bs_cmd_ld_img_area_upd_data(u8 *data, fx_type update_mode, u16 x, u16 y, u16 w, u16 h)
{
    bs_cmd cmd = bs_ld_fb ? bs_cmd_LD_IMG : bs_cmd_LD_IMG_AREA;
    bs_cmd_ld_img_upd_data_which(cmd, update_mode, data, x, y, w, h);
}

static u16 bs_cmd_upd_repair_get_upd_mode(int xstart, int xend, int ystart, int yend)
{
    u16 override_upd_mode = (u16)broadsheet_get_override_upd_mode(),
        upd_mode = BS_INIT_UPD_MODE();

    if ( BS_UPD_MODE_INIT != override_upd_mode )
    {
        upd_mode = BS_UPD_MODE(override_upd_mode);
        
        if ( BS_ISIS() )
            bs_cmd_set_wf_auto_sel_mode(0);
    
        einkfb_debug("overriding upd_mode\n");
        bs_debug_upd_mode(upd_mode);
    }
    else
    {
        // On ISIS, we're using its auto-waveform update selection capability.
        //
        if ( BS_ISIS() )
        {
            upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
            bs_cmd_set_wf_auto_sel_mode(1);
        }
        else
        {
            int x, y, rowbytes, bytes;
            u8 pixels;
        
            struct einkfb_info info;
            einkfb_get_info(&info);
            
            // Make bpp-related adjustments.
            //
            xstart   = BPP_SIZE(xstart,    info.bpp);
            xend     = BPP_SIZE(xend,	   info.bpp);
            rowbytes = BPP_SIZE(info.xres, info.bpp);
        
            // Check EINKFB_MEMCPY_MIN bytes at a time before yielding.
            //
            for (bytes = 0, y = ystart; y < yend; y++ )
            {
                for ( x = xstart; x < xend; x++ )
                {
                    pixels = info.vfb[(rowbytes * y) + x];
                    
                    // We only support 2bpp and 4bpp, but we always transmit the buffer out to
                    // Broadsheet in 4bpp mode.
                    // 
                    if ( EINKFB_2BPP == info.bpp )
                    {
                        upd_mode = BS_FIND_UPD_MODE(upd_mode, STRETCH_HI_NYBBLE(pixels, EINKFB_2BPP), EINKFB_2BPP);
                        upd_mode = BS_FIND_UPD_MODE(upd_mode, STRETCH_LO_NYBBLE(pixels, EINKFB_2BPP), EINKFB_2BPP);
                    }
                    else
                        upd_mode = BS_FIND_UPD_MODE(upd_mode, pixels, EINKFB_4BPP);
        
                    if ( BS_TEST_UPD_MODE(upd_mode, info.bpp) )
                        goto done;
        
                    EINKFB_SCHEDULE_BLIT(++bytes);
                }
            }
        
            done:
                upd_mode = BS_DONE_UPD_MODE(upd_mode);
                bs_debug_upd_mode(upd_mode);
        }
    }

    return ( upd_mode );
}

void bs_cmd_upd_repair(void)
{
    // Give any non-composite updates an opportunity to become composite.
    //
    if ( (1 < bs_upd_repair_count) || ((0 != bs_upd_repair_count) && bs_upd_repair_skipped) )
    {
        u16 w = bs_upd_repair_x2 - bs_upd_repair_x1,
            h = bs_upd_repair_y2 - bs_upd_repair_y1,
            upd_mode = bs_upd_repair_mode;
            
        einkfb_debug("Repairing %d parallel-update(s):\n", bs_upd_repair_count);
        einkfb_debug(" x1 = %d\n", bs_upd_repair_x1);
        einkfb_debug(" y1 = %d\n", bs_upd_repair_y1);
        einkfb_debug(" x2 = %d\n", bs_upd_repair_x2);
        einkfb_debug(" y2 = %d\n", bs_upd_repair_y2);
    
        // Determine the upd_mode for the area we're about to repair if the
        // repair is composite.
        //
        if ( 1 < bs_upd_repair_count )
            upd_mode = bs_cmd_upd_repair_get_upd_mode(bs_upd_repair_x1, bs_upd_repair_x2,
                bs_upd_repair_y1, bs_upd_repair_y2);
    
        // Perform the repair.
        //
        bs_cmd_wait_dspe_frend();
    
        bs_cmd_upd_part_area(upd_mode, 0, 0, bs_upd_repair_x1, bs_upd_repair_y1,
            w, h);
             
        bs_cmd_wait_dspe_trg();
        
        einkfb_debug("actual upd_mode\n");
        bs_debug_upd_mode(BS_LAST_WF_USED(BS_CMD_RD_REG(BS_UPD_BUF_CFG_REG)));
        
        // Done.
        //
        bs_upd_repair_skipped = false;
        bs_upd_repair_count = 0;
    }
    else
    {
        // Prime the timer again and note that we skipped this repair.
        //
        broadsheet_prime_watchdog_timer(EINKFB_DELAY_TIMER);
        bs_upd_repair_skipped = true;
    }
}

static void bs_ld_value(u8 v, u16 hsize, u16 vsize, fx_type update_mode)
{    
    // Adjust for hardware slop.
    //
    if ( BS97_INIT_VSIZE == bs_vsize )
        vsize = BS97_HARD_VSIZE;
    
    bs_ld_fb = broadsheet_get_scratchfb();
    einkfb_memset(bs_ld_fb, v, sizeof(u16));
    
    bs_cmd_ld_img_area_upd_data(bs_ld_fb, update_mode, 0, 0, hsize, vsize);
    bs_ld_fb = NULL;
}

unsigned long *bs_get_img_timings(int *num_timings)
{
    unsigned long *timings = NULL;
    
    if ( num_timings )
    {
        bs_image_timings[BS_IMAGE_TIMING_START] = bs_image_start_time;
        bs_image_timings[BS_IMAGE_TIMING_PROC]  = bs_image_processing_time;
        bs_image_timings[BS_IMAGE_TIMING_LOAD]  = bs_image_loading_time;
        bs_image_timings[BS_IMAGE_TIMING_DISP]  = bs_image_display_time;
        bs_image_timings[BS_IMAGE_TIMING_STOP]  = bs_image_stop_time;
   
        *num_timings = BS_NUM_IMAGE_TIMINGS;
        timings = bs_image_timings;
    }
    
    return ( timings );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet  800x600, 6.0-inch Panel API
    #pragma mark -
#endif

// See AM300_MMC_IMAGE_X03a/source/broadsheet_soft/bs60_init/bs60_init.cpp.

void bs60_init(int wa, bool full)
{
    u16 bs60_init_lutidxfmt = BS60_INIT_LUTIDXFMT;
    
    einkfb_debug("initializing broadsheet for 6-inch panel\n");
    
    if ( BS_ISIS() )
        bs60_init_lutidxfmt |= (BS60_INIT_PIX_INVRT | BS60_INIT_AUTO_WF);
    
    bs_cmd_init_dspe_cfg(BS60_INIT_HSIZE, BS60_INIT_VSIZE, 
            BS60_INIT_SDRV_CFG, BS60_INIT_GDRV_CFG, bs60_init_lutidxfmt);
    bs_cmd_init_dspe_tmg(BS60_INIT_FSLEN,
            (BS60_INIT_FELEN << 8) | BS60_INIT_FBLEN,
            BS60_INIT_LSLEN,
            (BS60_INIT_LELEN << 8) | BS60_INIT_LBLEN,
            BS60_INIT_PIXCLKDIV);
    bs_cmd_print_disp_timings();
    
    bs_cmd_set_wfm(wa);
    bs_cmd_print_wfm_info();
    
    einkfb_debug("display engine initialized with waveform 0x%X\n", wa);

    bs_cmd_clear_gd();

    bs_cmd_wr_reg(0x01A, 4); // i2c clock divider
    bs_cmd_wr_reg(0x320, 0); // temp auto read on
    
    if ( full )
    {
        bs60_flash();
        bs60_black();
        bs60_white();
    }
    else
    {
        if ( BS_UPD_MODE_INIT == bs_upd_mode )
        {
            bs_upd_mode = BS_UPD_MODE_REINIT;
            bs60_white();
        }
    }
}

void bs60_ld_value(u8 v)
{
    bs_ld_value(v, BS60_INIT_HSIZE, BS60_INIT_VSIZE, fx_update_full);
}

void bs60_flash(void)
{
    bs_upd_mode = BS_UPD_MODE_INIT;
    bs60_ld_value(EINKFB_WHITE);
}

void bs60_white(void)
{
    einkfb_debug("displaying white\n");
    bs60_ld_value(EINKFB_WHITE);
}

void bs60_black(void)
{
    einkfb_debug("displaying black\n");
    bs60_ld_value(EINKFB_BLACK);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet 1200x825, 9.7-inch Panel API
    #pragma mark -
#endif

// See AM300_MMC_IMAGE_X03b/source/broadsheet_soft/bs97_init/bs97_init.cpp.

void bs97_init(int wa, bool full)
{
    int iba = BS97_INIT_HSIZE * BS97_HARD_VSIZE * 2;
    u16 bs97_init_lutidxfmt = BS97_INIT_LUTIDXFMT;
    
    einkfb_debug("initializing broadsheet for 9.7-inch panel\n");

    if ( BS_ISIS() )
        bs97_init_lutidxfmt |= (BS97_INIT_PIX_INVRT | BS97_AUTO_WF);
    
    bs_cmd_init_dspe_cfg(BS97_INIT_HSIZE, BS97_INIT_VSIZE, 
            BS97_INIT_SDRV_CFG, BS97_INIT_GDRV_CFG, bs97_init_lutidxfmt);
    bs_cmd_init_dspe_tmg(BS97_INIT_FSLEN,
			(BS97_INIT_FELEN << 8) | BS97_INIT_FBLEN,
			BS97_INIT_LSLEN,
			(BS97_INIT_LELEN << 8) | BS97_INIT_LBLEN,
			BS97_INIT_PIXCLKDIV);

    bs_cmd_wr_reg(0x310, (iba & 0xFFFF));
    bs_cmd_wr_reg(0x312, ((iba >> 16) & 0xFFFF));
    
    bs_cmd_print_disp_timings();
    
    bs_cmd_set_wfm(wa);
    bs_cmd_print_wfm_info();
    
    einkfb_debug("display engine initialized with waveform 0x%X\n", wa);
    
    bs_cmd_clear_gd();
    
    bs_cmd_wr_reg(0x01A, 4); // i2c clock divider
    bs_cmd_wr_reg(0x320, 0); // temp auto read on

    if ( full )
    {
        bs97_flash();
        bs97_black();
        bs97_white();
    }
    else
    {
        if ( BS_UPD_MODE_INIT == bs_upd_mode )
        {
            bs_upd_mode = BS_UPD_MODE_REINIT;
            bs97_white();
        }
    }
}

void bs97_ld_value(u8 v)
{
    bs_ld_value(v, BS97_INIT_HSIZE, BS97_INIT_VSIZE, fx_update_full);
}

void bs97_flash(void)
{
    bs_upd_mode = BS_UPD_MODE_INIT;
    bs97_ld_value(EINKFB_WHITE);
}

void bs97_white(void)
{
    einkfb_debug("displaying white\n");
    bs97_ld_value(EINKFB_WHITE);
}

void bs97_black(void)
{
    einkfb_debug("displaying black\n");
    bs97_ld_value(EINKFB_BLACK);
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet API
    #pragma mark -
#endif

static bool bs_pll_ready(void *unused)
{
    return ( (BSC_RD_REG(0x00A) & 0x1) != 0 );
}

static void bs_init_pll(void)
{
    u16 init_pll_cfg_0, init_pll_cfg_1;
    int v;
    
    if ( BS_ASIC() )
    {
        init_pll_cfg_0 = INIT_PLL_CFG_0_ASIC;
        init_pll_cfg_1 = INIT_PLL_CFG_1_ASIC;
        
        BSC_WR_REG(0x006, INIT_PWR_SAVE_MODE);
    }
    else
    {
        init_pll_cfg_0 = INIT_PLL_CFG_0_FPGA;
        init_pll_cfg_1 = INIT_PLL_CFG_1_FPGA;
        
        bs_hw_test();
    }
    
    BSC_WR_REG(0x010, init_pll_cfg_0);
    BSC_WR_REG(0x012, init_pll_cfg_1);
    BSC_WR_REG(0x014, INIT_PLL_CFG_2);
    
    if ( BS_FPGA() )
        BSC_WR_REG(0x006, INIT_PWR_SAVE_MODE);
    
    BSC_WR_REG(0x016, INIT_CLK_CFG);

    if ( EINKFB_SUCCESS == EINKFB_SCHEDULE_TIMEOUT(BS_PLL_TIMEOUT, bs_pll_ready) )
        bs_pll_steady = true;
    else
        bs_pll_steady = false;
    
    if ( BS_ASIC() )
    {
        v = BSC_RD_REG(0x006);
        BSC_WR_REG(0x006, v & ~0x1);
    }
}

static void bs_init_spi(void)
{
    BSC_WR_REG(0x204, INIT_SPI_FLASH_CTL);
    BSC_WR_REG(0x208, INIT_SPI_FLASH_CSC);
}

static void bs_bootstrap_init(void)
{
    broadsheet_set_ignore_hw_ready(true);
    
    bs_init_pll();
    
    if ( BS_ASIC() )
        BSC_WR_REG(0x106, 0x0203);
    
    bs_init_spi();
    
    if ( BS_ASIC() )
        bs_hw_test();
}

static void bs_sys_init(void)
{
    if ( BS_ISIS() )
    {
        bs_cmd_init_pll_stby(INIT_PLL_CFG_0_ASIC, INIT_PLL_CFG_1_ASIC, INIT_PLL_CFG_2);
        bs_cmd_init_cmd_set_isis(SIZEOF_ISIS_COMMANDS, isis_commands);
        
        bs_sdr_size = BS_SDR_SIZE_ISIS;
    }
    
    bs_cmd_init_sys_run();
    
    if ( BS_ISIS() )
    {
        bs_cmd_bst_wr_sdr(BS_WFM_ADDR_ISIS, SIZEOF_ISIS_WAVEFORM, isis_waveform);
        bs_cmd_init_waveform(BS_WFM_ADDR_SDRAM);
    }
    
    if ( BS_ASIC() )
    {
        bs_cmd_wr_reg(0x106, 0x0203);    
        bs_hw_test();
    }
}

static unsigned char bs_get_fpl_size(void)
{
    unsigned char fpl_size = EINK_FPL_SIZE_60;
    
    if ( !bs_bootstrap )
    {
        if ( (0 == bs_hsize) || (0 == bs_vsize) )
        {
            broadsheet_waveform_info_t waveform_info;
            broadsheet_get_waveform_info(&waveform_info);
            
            fpl_size = waveform_info.fpl_size;
        }
        else
        {
            if ( BS97_INIT_HSIZE == bs_hsize )
                fpl_size = EINK_FPL_SIZE_97;
        }
    }
    
    return ( fpl_size );
}

static void bs_get_resolution(unsigned char fpl_size, bool orientation, u32 *xres_hw, u32 *xres_sw, u32 *yres_hw, u32 *yres_sw)
{
    switch ( fpl_size )
    {
        // Portrait  ->  824 x 1200 software,  826 x 1200 hardware
        // Landscape -> 1200 x  824 software, 1200 x  826 hardware
        //
        case EINK_FPL_SIZE_97:
            *xres_hw = BS97_INIT_HSIZE; 
            *yres_hw = BS97_HARD_VSIZE;
            
            *xres_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_SOFT_VSIZE : BS97_INIT_HSIZE;
            *yres_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_INIT_HSIZE : BS97_SOFT_VSIZE;
        break;
        
        // Portrait  ->  600 x  800 software/hardware
        // Landscape ->  800 x  600 software/hardware
        //
        case EINK_FPL_SIZE_60:
        default:
            *xres_hw = BS60_INIT_HSIZE;
            *yres_hw = BS60_INIT_VSIZE;
            
            *xres_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_INIT_VSIZE : BS60_INIT_HSIZE;
            *yres_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_INIT_HSIZE : BS60_INIT_VSIZE;
        break;
    }

    // Broadsheet actually aligns the hardware horizontal resolution to a 32-bit boundary.
    //
    *xres_hw = BS_ROUNDUP32(*xres_hw);
}

#define BS_ASIC_B00()   ((BS_PRD_CODE == bs_prd_code) && (BS_REV_ASIC_B00 == bs_rev_code))
#define BS_ASIC_B01()   ((BS_PRD_CODE == bs_prd_code) && (BS_REV_ASIC_B01 == bs_rev_code))

static bool bs_isis(void)
{
    if ( (BS_PRD_CODE_UNKNOWN == bs_prd_code) || (BS_REV_CODE_UNKNOWN == bs_rev_code) )
    {
        bs_prd_code = BS_CMD_RD_REG(BS_PRD_CODE_REG);
        bs_rev_code = BS_CMD_RD_REG(BS_REV_CODE_REG);
    }
    
    return ( BS_PRD_CODE_ISIS == bs_prd_code );
}

static bool bs_asic(void)
{
    if ( (BS_PRD_CODE_UNKNOWN == bs_prd_code) || (BS_REV_CODE_UNKNOWN == bs_rev_code) )
    {
        bs_prd_code = BS_CMD_RD_REG(BS_PRD_CODE_REG);
        bs_rev_code = BS_CMD_RD_REG(BS_REV_CODE_REG);
    }
    
    return ( BS_ASIC_B00() || BS_ASIC_B01() || bs_isis() );
}

static bool bs_fpga(void)
{
    if ( (BS_PRD_CODE_UNKNOWN == bs_prd_code) || (BS_REV_CODE_UNKNOWN == bs_rev_code) )
    {
        bs_prd_code = BS_CMD_RD_REG(BS_PRD_CODE_REG);
        bs_rev_code = BS_CMD_RD_REG(BS_REV_CODE_REG);
    }
    
    return ( (BS_PRD_CODE == bs_prd_code) && !bs_asic() );
}

#define BS_SW_INIT_CONTROLLER(f) bs_sw_init_controller(f, EINKFB_ORIENT_PORTRAIT, NULL, NULL, NULL, NULL)

void bs_sw_init_controller(bool full, bool orientation, u32 *xres_hw, u32 *xres_sw, u32 *yres_hw, u32 *yres_sw)
{
    unsigned char fpl_size = 0;

    full = bs_bootstrap || full;
    
    // Perform general Broadsheet initialization.
    //
    if ( full )
    {
        // Take the hardware down first if it's still up.
        //
        if ( bs_hw_up )
            bs_sw_done(DONT_DEALLOC);
        
        // Attempt to bring the hardware (back) up.
        // 
        if ( bs_hw_init() )
        {
            // Say that the hardware is up and that
            // we're ready to accept commands.
            //
            bs_hw_up = bs_ready = true;
            
            // Perform FPGA- vs. ASIC-specific initialization if we're not
            // bootstrapping the device.
            //
            if ( !bs_bootstrap )
            {
                if ( BS_FPGA() )
                    bs_hw_test();
                    
                if ( BS_ASIC() )
                    bs_cmd_wr_reg(0x006, 0x0000);
            }
        }
        else
            bs_sw_done(DONT_DEALLOC);
    }
    
    // Under special circumstances (as when Broadsheet's flash is blank), we need
    // to bootstrap Broadsheet.  And, in that case, we just return the default,
    // 6-inch panel resolution if requested.
    //
    if ( bs_bootstrap )
    {
        if ( bs_hw_up )
            bs_bootstrap_init();
        
        if ( xres_hw && xres_sw && yres_sw && yres_hw )
            fpl_size = EINK_FPL_SIZE_60;
    }
    else
    {
        // Return panel-specific resolutions by reading the panel size from the
        // waveform header if requested.
        //
        bs_sys_init();
        
        if ( xres_hw && xres_sw && yres_sw && yres_hw )
            fpl_size = bs_get_fpl_size();
    }

    if ( fpl_size )
        bs_get_resolution(fpl_size, orientation, xres_hw, xres_sw, yres_hw, yres_sw);
}

#define BROADSHEET_GET_ORIENTATION() broadsheet_get_orientation(), broadsheet_get_upside_down()

bool bs_sw_init_panel(bool full)
{
    // Don't go through the panel initialization path if
    // we're in bootstrap mode.
    //
    if ( bs_bootstrap )
        full = false;
    else
    {
        u32 bs_wfm_addr = BS_ISIS() ? BS_WFM_ADDR_ISIS : BS_WFM_ADDR;
        
        // Otherwise, go through panel-specific initialization.
        //
        switch ( bs_get_fpl_size() )
        {
            case EINK_FPL_SIZE_97:
                bs97_init(bs_wfm_addr, full);
            break;
            
            case EINK_FPL_SIZE_60:
            default:
                bs60_init(bs_wfm_addr, full);
            break;
        }
        
        // Put us into the right orientation.
        //
        bs_cmd_set_rotmode(bs_cmd_orientation_to_rotmode(BROADSHEET_GET_ORIENTATION()));
        
        // Determine which update modes to use.
        //
        bs_set_upd_modes();
        
        // On ISIS, get the pixels in the right order in hardware, and
        // set up the auto-waveform update selection in hardware.
        //
        if ( BS_ISIS() )
        {
            bs_cmd_wr_reg(BS_PIX_CNFG_REG, BS_PIX_REVSWAP);
            
            bs_cmd_wr_reg(BS_AUTO_WF_REG_DU,   BS_AUTO_WF_MODE_DU);
            bs_cmd_wr_reg(BS_AUTO_WF_REG_GC4,  BS_AUTO_WF_MODE_GC4);
            bs_cmd_wr_reg(BS_AUTO_WF_REG_GC16, BS_AUTO_WF_MODE_GC16);
        }
        
        // Remember that the framebuffer has been initialized.
        //
        set_fb_init_flag(BOOT_GLOBALS_FB_INIT_FLAG);
    }

    // Say that we're now in the run state.
    //
    bs_power_state = bs_power_state_run;
    
    return ( full );
}

#define BS_SW_INIT(f) bs_sw_init(f, f)

bool bs_sw_init(bool controller_full, bool panel_full)
{
    BS_SW_INIT_CONTROLLER(controller_full);
    return ( bs_sw_init_panel(panel_full) );
}

void bs_sw_done(bool dealloc)
{
    // Take the hardware all the way down, but only deallocate when
    // necessary.
    //
    bs_hw_done(dealloc);
    
    // Say that hardware is down and that we're
    // no longer ready to accept commands.
    //
    bs_hw_up = bs_ready = bs_pll_steady = false;
    
    // Note that we're in the off state.
    //
    bs_power_state = bs_power_state_off;
}

bs_preflight_failure bs_get_preflight_failure(void)
{
    return ( preflight_failure );
}

static bool BS_PREFLIGHT_PASSES(void)
{
    preflight_failure = bs_preflight_failure_none;          // Assume no failures.
    
    if ( !(bs_hw_up && bs_ready && bs_pll_steady) )         // Is the hardware responding?
        preflight_failure |= bs_preflight_failure_hw;
        
    if ( !preflight_failure )
    {
        if ( !(BS_ASIC() || BS_FPGA()) )                    // Do the ID bits look good?
            preflight_failure |= bs_preflight_failure_id;
    }

    if ( !preflight_failure )
    {
        if ( !bs_hw_test() )                                // Is the bus okay?
            preflight_failure |= bs_preflight_failure_bus;
    }
        
    if ( !preflight_failure )                               // Do we recognize the Flash?
    {
        if ( !bs_sfm_preflight() )
            preflight_failure |= bs_preflight_failure_flid;
    }
    
    if ( !preflight_failure )
    {
        if ( !broadsheet_commands_valid() )                 // Is the commands area okay?
            preflight_failure |= bs_preflight_failure_cmd;
    }
        
    if ( !preflight_failure )
    {
        if ( !broadsheet_waveform_valid() )                 // Is the waveform okay?
            preflight_failure |= bs_preflight_failure_wf;
    }
        
    return ( (bs_preflight_failure_none == preflight_failure) ? true : false );
}

bool bs_preflight_passes(void)
{
    // Preserve bootstrapping state.
    //
    int saved_bs_bootstrap = bs_bootstrap;
    bool result = false;
    
    // For preflighting, force us into bootstrapping mode.
    //
    bs_bootstrap = true;
    BS_SW_INIT_CONTROLLER(FULL_BRINGUP_CONTROLLER);
    
    // If all is well, clean up.
    //
    if ( BS_PREFLIGHT_PASSES() )
    {
        // Before taking us down, save/set how the display should be
        // initialized.
        //
        broadsheet_preflight_init_display_flag(BS_INIT_DISPLAY_READ);
        
        // Finish up by pretending we were never here.
        //
        bs_sw_done(DONT_DEALLOC);
        
        broadsheet_set_ignore_hw_ready(false);
        bs_power_state = bs_power_state_init;
        
        result = true;
    }
    else
    {
        // Note that we can't ascertain directly how to initialize
        // the display.
        //
        broadsheet_preflight_init_display_flag(BS_INIT_DISPLAY_DONT);
    }
   
    // Restore bootstrapping state.
    //
    bs_bootstrap = saved_bs_bootstrap;

    return ( result );
}

bool broadsheet_get_bootstrap_state(void)
{
    return ( bs_bootstrap ? true : false );
}

void broadsheet_set_bootstrap_state(bool bootstrap_state)
{
    bs_bootstrap = bootstrap_state ? 1 : 0;
}

bool broadsheet_get_ready_state(void)
{
    return ( bs_ready );
}

bool broadsheet_get_upd_repair_state(void)
{
    return ( (0 == bs_upd_repair_count) ? true : false );
}

void broadsheet_clear_screen(fx_type update_mode)
{
    bs_ld_value(EINKFB_WHITE, bs_hsize, bs_vsize, update_mode);
}

bs_power_states broadsheet_get_power_state(void)
{
    return ( bs_power_state );
}

void broadsheet_set_power_state(bs_power_states power_state)
{
    if ( bs_power_state != power_state )
    {
        switch ( power_state )
        {            
            // run
            //
            case bs_power_state_run:
                switch ( bs_power_state )
                {
                    // Initial state.  Do nothing as bs_sw_init() will be called "naturally."
                    //
                    case bs_power_state_init:
                    break;

                    // off_screen_clear -> run
                    //
                    case bs_power_state_off_screen_clear:
                        BS_SW_INIT(FULL_BRINGUP);
                    break;
                    
                    // off -> run
                    //
                    case bs_power_state_off:
                        bs_sw_init(FULL_BRINGUP_CONTROLLER, DONT_BRINGUP_PANEL);
                        bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
                    break;

                    // sleep/standby -> run
                    //
                    default:
                        bs_cmd_run_sys();
                        bs_power_state = bs_power_state_run;
                    break;
                }
            break;
            
            // off
            //
            case bs_power_state_off:
                broadsheet_set_power_state(bs_power_state_sleep);
                bs_sw_done(DONT_DEALLOC);
            break;
            
            // off -> off_screen_clear
            //
            case bs_power_state_off_screen_clear:
                broadsheet_set_power_state(bs_power_state_off);
                bs_power_state = bs_power_state_off_screen_clear;
            break;
            
            //  run -> standby (not currently in use)
            //
            case bs_power_state_standby:
                broadsheet_set_power_state(bs_power_state_run);

                bs_cmd_stby();
                bs_power_state = bs_power_state_standby;
            break;
            
            // run -> sleep
            //
            case bs_power_state_sleep:
                broadsheet_set_power_state(bs_power_state_run);
                
                bs_cmd_slp();
                bs_power_state = bs_power_state_sleep;
            break;
            
            // Prevent the compiler from complaining.
            //
            default:
            break;
        }
    }
}

void bs_iterate_cmd_queue(bs_cmd_queue_iterator_t bs_cmd_queue_iterator, int max_elems)
{
    if ( bs_cmd_queue_iterator )
    {
        bs_cmd_queue_elem_t *bs_cmd_queue_elem;
        int i, count = bs_cmd_queue_count();
        
        if ( (count < max_elems) || (0 > max_elems) )
            i = 0;
        else
            i = count - max_elems;

        for ( ; (NULL != (bs_cmd_queue_elem = bs_get_queue_cmd_elem(i))) && (i < count); i++ )
            (*bs_cmd_queue_iterator)(bs_cmd_queue_elem);
    }
}

int bs_read_temperature(void)
{
    int raw_temp, temp;

    bs_cmd_wr_reg(0x214, 0x0001);           // Trigger a temperature-sensor read.
    bs_cmd_wait_for_bit(0x212, 0, 0);       // Wait for the sensor to be idle.
    
    raw_temp = (int)bs_cmd_rd_reg(0x216);   // Read the result.
    
    // The temperature sensor is actually returning a signed, 8-bit
    // value from -25C to 80C.  But we go ahead and use the entire
    // 8-bit range:  0x00..0x7F ->  0C to  127C
    //               0xFF..0x80 -> -1C to -128C
    //
    if ( IN_RANGE(raw_temp, 0x0000, 0x007F) )
        temp = raw_temp;
    else
    {
        raw_temp &= 0x00FF;
        temp = raw_temp - 0x0100;
    }
    
    return ( temp );    
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet Read/Write SDRAM API
    #pragma mark -
#endif

int broadsheet_get_ram_size(void)
{
    return ( bs_sdr_size );
}

static unsigned long broadsheet_get_ram_base(void)
{
    return ( (unsigned long)(broadsheet_get_ram_select() * broadsheet_get_ram_select_size()) );
}

int broadsheet_read_from_ram(unsigned long addr, unsigned char *data, unsigned long size)
{
    unsigned long start = broadsheet_get_ram_base() + addr;
    int result = 0;

    if ( bs_sdr_size >= (start + size) )
    { 
        bs_cmd_rd_sdr(start, size, data);
        result = 1;
    }
    else
        einkfb_print_warn("Attempting to read off the end of memory, start = %ld, length %ld\n",
            start, size);

    return ( result );
}

int broadsheet_read_from_ram_byte(unsigned long addr, unsigned char *data)
{
    unsigned long byte_addr = addr & 0xFFFFFFFE;
    unsigned char byte_array[2] = { 0 };
    int result = 0;
    
    if ( broadsheet_read_from_ram(byte_addr, byte_array, sizeof(byte_array)) )
    {
        *data  = (1 == (addr & 1)) ? byte_array[1] : byte_array[0];
        result = 1;
    }
    
    return ( result );
}

int broadsheet_read_from_ram_short(unsigned long addr, unsigned short *data)
{
    return ( broadsheet_read_from_ram(addr, (unsigned char *)data, sizeof(unsigned short)) );
}

int broadsheet_read_from_ram_long(unsigned long addr, unsigned long *data)
{
    return ( broadsheet_read_from_ram(addr, (unsigned char *)data, sizeof(unsigned long)) );
}

int broadsheet_program_ram(unsigned long start_addr, unsigned char *buffer, unsigned long blen)
{
   unsigned long start = broadsheet_get_ram_base() + start_addr;
   int result = 0;

    if ( bs_sdr_size >= (start + blen) )
    { 
        bs_cmd_wr_sdr(start, blen, buffer);
        result = 1;
    }
    else
        einkfb_print_warn("Attempting to write off the end of memory, start = %ld, length %ld\n",
            start, blen);

    return ( result );
}

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet Waveform/Commands Flashing API
    #pragma mark -
#endif

#include "broadsheet_waveform.c"
#include "broadsheet_commands.c"

static unsigned char   *buffer_byte  = NULL;
static unsigned short  *buffer_short = NULL;
static unsigned long   *buffer_long  = NULL;

static u8 *isis_get_flash_base(void)
{
    u8 *result;
    
    switch ( broadsheet_get_flash_select() )
    {
        case bs_flash_waveform:
            result = isis_waveform;
        break;

        case bs_flash_commands:
            result = isis_commands;
        break;
        
        case bs_flash_test:
        default:
            result = NULL;
        break;
    }
    
    return ( result );
}

static int isis_read_from_flash_byte(unsigned long addr, unsigned char *data)
{
    if ( data && buffer_byte )
        *data = buffer_byte[addr];
    
    return ( NULL != buffer_byte );
}

static int isis_read_from_flash_short(unsigned long addr, unsigned short *data)
{
    if ( data && buffer_short && buffer_byte )
    {
        // Read 16 bits if we're 16-bit aligned.
        //
        if ( addr == ((addr >> 1) << 1) )
            *data = buffer_short[addr >> 1];
        else
            *data = (buffer_byte[addr + 0] << 0) |
                    (buffer_byte[addr + 1] << 8);
    }
    
    return ( NULL != buffer_short );
}

static int isis_read_from_flash_long(unsigned long addr, unsigned long *data)
{
    if ( data && buffer_long && buffer_byte )
    {
        // Read 32 bits if we're 32-bit aligned.
        //
        if ( addr == ((addr >> 2) << 2) )
            *data = buffer_long[addr >> 2];
        else
            *data = (buffer_byte[addr + 0] <<  0) |
                    (buffer_byte[addr + 1] <<  8) |
                    (buffer_byte[addr + 2] << 16) |
                    (buffer_byte[addr + 3] << 24);
    }
    
    return ( NULL != buffer_long );
}

static int isis_read_from_flash(unsigned long addr, unsigned char *data, unsigned long size)
{
    u8 *buffer = isis_get_flash_base();
    int result = 0;
    
    if ( buffer )
    {
        buffer_byte  = buffer;
        buffer_short = (unsigned short *)buffer;
        buffer_long  = (unsigned long *)buffer;
        
        switch ( size )
        {
            case sizeof(unsigned char):
                result = isis_read_from_flash_byte(addr, data);
            break;
            
            case sizeof(unsigned short):
                result = isis_read_from_flash_short(addr, (unsigned short *)data);
            break;
            
            case sizeof(unsigned long):
                result = isis_read_from_flash_long(addr, (unsigned long *)data);
            break;
        }

        buffer_byte  = NULL;
        buffer_short = NULL;
        buffer_long  = NULL;
    }
    
    return ( result );
}

static unsigned long broadsheet_get_flash_base(void)
{
    unsigned long result;
    
    switch ( broadsheet_get_flash_select() )
    {
        case bs_flash_waveform:
            result = BS_WFM_ADDR;
        break;

        case bs_flash_commands:
            result = BS_CMD_ADDR;
        break;
        
        case bs_flash_test:
        default:
            result = bs_tst_addr;
        break;
    }
    
    return ( result );
}

int broadsheet_read_from_flash(unsigned long addr, unsigned char *data, unsigned long size)
{
    unsigned long start = broadsheet_get_flash_base() + addr;
    int result = 0;

    if ( bs_sfm_size >= (start + size) )
    { 
        bs_sfm_start();
        bs_sfm_read(start, size, data);
        bs_sfm_end();
        
        result = 1;
    }
    else
        einkfb_print_warn("Attempting to read off the end of flash, start = %ld, length %ld\n",
            start, size);

    return ( result );
}

#define BROADSHEET_READ_FROM_FLASH(a, d, s) \
    (BS_ISIS() ? isis_read_from_flash((a), (d), (s)) : broadsheet_read_from_flash((a), (d), (s)))

bool broadsheet_supports_flash(void)
{
    return ( BS_ISIS() ? false : true );
}

int broadsheet_read_from_flash_byte(unsigned long addr, unsigned char *data)
{
    return ( BROADSHEET_READ_FROM_FLASH(addr, data, sizeof(unsigned char)) );
}

int broadsheet_read_from_flash_short(unsigned long addr, unsigned short *data)
{
    return ( BROADSHEET_READ_FROM_FLASH(addr, (unsigned char *)data, sizeof(unsigned short)) );
}

int broadsheet_read_from_flash_long(unsigned long addr, unsigned long *data)
{
    return ( BROADSHEET_READ_FROM_FLASH(addr, (unsigned char *)data, sizeof(unsigned long)) );
}

int broadsheet_program_flash(unsigned long start_addr, unsigned char *buffer, unsigned long blen)
{
    unsigned long flash_base = broadsheet_get_flash_base(),
                  start = flash_base + start_addr;
    int result = 0;

    if ( bs_sfm_size >= (start + blen) )
    { 
        bs_sfm_start();
        bs_sfm_write(start, blen, buffer);
        bs_sfm_end();
        
        switch ( flash_base )
        {
            // Soft reset after flashing the waveform.
            //
            case BS_WFM_ADDR:
                bs_sw_init(DONT_BRINGUP_CONTROLLER, DONT_BRINGUP_PANEL);
                if ( !bs_bootstrap )
                    bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
            break;

            // Hard reset after flashing the commands (allow
            // the test area of flash to act like commands
            // when in bootstrap mode).
            //
            case BS_TST_ADDR_128K:
            case BS_TST_ADDR_256K:
                if ( !bs_bootstrap )
                    break;

            case BS_CMD_ADDR:
                bs_sw_init(FULL_BRINGUP_CONTROLLER, DONT_BRINGUP_PANEL);
                if ( !bs_bootstrap )
                    bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
            break;
        }

        result = 1;
    }
    else
        einkfb_print_warn("Attempting to write off the end of flash, start = %ld, length %ld\n",
            start, blen);

    return ( result );
}

unsigned long broadsheet_get_init_display_flag(void)
{
    unsigned long init_display_flag = get_broadsheet_init_display_flag();
    
    // Keep the values we recognize, and force everything else to the slow path.
    //
    switch ( init_display_flag )
    {
        case BS_INIT_DISPLAY_FAST:
        case BS_INIT_DISPLAY_SLOW:
        case BS_INIT_DISPLAY_ASIS:
        break;
        
        default:
            init_display_flag = BS_INIT_DISPLAY_SLOW;
        break;
    }
    
    return ( init_display_flag );
}

void broadsheet_set_init_display_flag(unsigned long init_display_flag)
{
    // Keep the values we recognize, and force everything else to the slow path.
    //
    switch ( init_display_flag )
    {
        case BS_INIT_DISPLAY_FAST:
        case BS_INIT_DISPLAY_SLOW:
        case BS_INIT_DISPLAY_ASIS:
        break;
        
        default:
            init_display_flag = BS_INIT_DISPLAY_SLOW;
        break;
    }
    
    set_broadsheet_init_display_flag(init_display_flag);
}

void broadsheet_preflight_init_display_flag(bool read)
{
    unsigned long init_display_flag = BS_INIT_DISPLAY_SLOW;
    
    // Read the initialization flag from the boot globals if we should.
    //
    if ( read )
        init_display_flag = broadsheet_get_init_display_flag();
    
    switch ( init_display_flag )
    {
        case BS_INIT_DISPLAY_FAST:
            set_fb_init_flag(BOOT_GLOBALS_FB_INIT_FLAG);
        break;
        
        case BS_INIT_DISPLAY_ASIS:
        case BS_INIT_DISPLAY_SLOW:
        default:
            set_fb_init_flag(0);
        break;
    }

    broadsheet_set_init_display_flag(init_display_flag);
}
