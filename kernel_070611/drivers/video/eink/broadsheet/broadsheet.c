/*
 *  linux/drivers/video/eink/broadsheet/broadsheet.c
 *  -- eInk frame buffer device HAL broadsheet sw
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <plat/regs-gpio.h>
#include "../hal/einkfb_hal.h"
#include "broadsheet.h"

#if PRAGMAS
    #pragma mark Definitions/Globals
#endif

#define INIT_PWR_SAVE_MODE      0x0000

#define INIT_PLL_CFG_0_FPGA     0x0000
#define INIT_PLL_CFG_1_FPGA     0x0000
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
#define INIT_PLL_CFG_0_ASIC     0x0003
#else
#define INIT_PLL_CFG_0_ASIC     0x0004
#endif
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

#define BS_SDR_SIZE             (16*1024*1024)

#define BS_NUM_CMD_QUEUE_ELEMS  101 // n+1 -> n cmds + 1 for empty
#define BS_RECENT_CMDS_SIZE     (32*1024)

static int bs_sfm_sector_size = BS_SFM_SECTOR_SIZE_128K;
static int bs_sfm_size = BS_SFM_SIZE_128K;
static int bs_sfm_page_count = BS_SFM_PAGE_COUNT_128K;
static int bs_tst_addr = BS_TST_ADDR_128K;
static char sd[BS_SFM_SIZE_256K];
static char *rd = &sd[0];
static int sfm_cd;

static u16 bs_upd_mode = BS_UPD_MODE_INIT;
static char *bs_upd_mode_string = NULL;

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

extern unsigned short __iomem* epd_get_bsaddr();

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

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
   // Arithemetic Logic Unit Registers
   0x0080, 0x0082, 0x0084, 0x0086, 0x0088, 0x008a, 0x008c, 0x008e,
   0x0090, 0x0092, 0x0094, 0x0096, 0x0098, 0x009a, 0x009c, 0x009e,
   0x00a0, 0x00a2, 0x00a4, 0x00a6, 0x00a8, 0x00aa, 0x00ac, 0x00ae,
   0x00b0, 0x00b2, 0x00b4, 0x00b6, 0x00b8, 0x00ba, 0x00bc, 0x00be,
   0x00c0, 0x00c2, 0x00c4, 0x00c6, 0x00c8, 0x00ca, 0x00cc, 0x00ce,
   0x00d0, 0x00d2, 0x00d4, 0x00d6, 0x00d8, 0x00da, 0x00dc, 0x00de,
#endif

    // Memory Controller Configuration
    //
    0x0100, 0x0102, 0x0104, 0x0106, 0x0108, 0x010A,
    0x010C,
    
    // Host Interface Memory Access Configuration
    //
    0x0140, 0x0142, 0x0144, 0x0146, 0x0148, 0x014A,
    0x014C, 0x014E, 0x0150, 0x0152, 0x0154, 0x0156,

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    0x0158, 0x015a, 0x15e,	
#endif    
    
    // SPI Flash Memory Interface
    //
    0x0200, 0x0202, 0x0204, 0x0206, 0x0208,
    
    // I2C Thermal Sensor Interface Registers
    //
    0x0210, 0x0212, 0x0214, 0x0216,

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    0x0218, 0x021a, 0x021c, 0x021e,
#endif        
    
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

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
   //Waveform read control ergisters
   0x0260,
#endif

    // Command RAM Controller Registers
    //
    0x0290, 0x0292, 0x0294,
    
    // Command Sequence Controller Registers
    //
    0x02A0, 0x02A2,
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    0x02A4, 0x02A6, 0x02A8, 0x02AA, 0x02AC,
#endif

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
   // Multi-Function serializer registers
     0x02c0, 0x02c2, 0x02c4, 0x02c6, 0x02c8, 0x02ca, 0x02cc, 0x02ce, 
     0x02d0, 0x02d2, 0x02d4, 0x02d6, 0x02d8, 0x02da, 0x02dc, 0x02de,
     0x02e0,
#endif
   
    // Display Engine:  Display Timing Configuration
    // 
    0x0300, 0x0302, 0x0304, 0x0306, 0x0308, 0x30A,
    
    // Display Engine:  Driver Configurations
    //
    0x030C, 0x30E,
    
    // Display Engine:  Memory Region Configuration Registers
    //
    0x0310, 0x0312, 0x0314, 0x0316,
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    0x0318, 0x031a,
#endif
    
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

#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)   
    // Display Engine:  Partial Update Configuration Registers
    //
    0x0340, 0x0342, 0x0344, 0x0346, 0x0348, 0x034A,
    0x034C, 0x034E,
    
    // Display Engine:  Serial Flash Waveform Registers
    //
    0x0350, 0x0352
#endif		//!!! Pay attention to no ","
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    0x0360, 0x0362, 0x0364, 0x0366, 0x0368,
    0x0370, 0x0372, 0x0374, 0x0376, 0x0378, 0x037a,
    
    //Display Engine: Partial Update configuration registers
    0x0380, 0x0382, 0x0384, 0x0386, 0x0388, 0x038a, 0x038c, 0x038e,
    //Display Engine: Waveform registers
    0x0390, 0x0392, 0x0394,
    //Auto Waveform Mode Configuration registers
    0x03a0, 0x03a2, 0x03a4, 0x03a6, 
    //Picture in Picture(PIP) control registers
    0x03c0, 0x03c2, 0x03c4, 0x03c6, 0x03c8, 0x03ca,
    //Cursor control registers
    0x03d0, 0x03d2, 0x03d4, 0x03d6, 0x03d8, 0x03da, 0x03dc, 0x03de
#endif		//!!! Pay attention to no ","
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

//&*&*&*Modify1:EPD update mode change    
#if 0	
#define BS_DONE_UPD_MODE(m)                         \
    ((UM02 == (m)) ? BS_UPD_MODE(BS_UPD_MODE_MU) :  \
     (UM04 == (m)) ? BS_UPD_MODE(BS_UPD_MODE_GU) :  BS_UPD_MODE(BS_UPD_MODE_GC))
#else
#define BS_DONE_UPD_MODE(m)   BS_UPD_MODE(m)
#endif
//&*&*&*Modify1:EPD update mode change    
    
#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet SW Primitives
    #pragma mark -
#endif

#define BS_UPD_MODE(u) (bs_upd_modes ? bs_upd_modes[(u)] : (u))
unsigned char global_gpj5_7;	//Henry: 2010-12-09 SC8 EPD
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
    if ( EINKFB_DEBUG() || EINKFB_PERF() )
    {
        char *upd_mode_string = NULL;
        
        if ( !bs_upd_modes || (bs_upd_modes_old == bs_upd_modes) )
        {
            switch ( upd_mode )
            {
//&*&*&*Modify1:EPD update mode change       
#if 0     	
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
#endif          
                case UPDATE_GU:
                    upd_mode_string = "GU->GC4";
                break;
                
                case UPDATE_GC:
                    upd_mode_string = "GC->GC16";
                break;
                
                case UPDATE_DU:
                    upd_mode_string = "DU";
                break;
//&*&*&*Modify2:EPD update mode change    
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
            
        bs_upd_mode_string = upd_mode_string ? upd_mode_string : "??";
		
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
    
    //modify by jacky.
    //    bs_cmd_queue[bs_cmd_queue_entry++].time_stamp = FB_GET_CLOCK_TICKS();
	    bs_cmd_queue[bs_cmd_queue_entry++].time_stamp = 50000000;
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

extern void save_bs_mode_var(int bs_mode);
void bs_cmd_run_sys(void)
{
    bs_wr_one_ready();
    BS_SEND_CMD(bs_cmd_RUN_SYS);
    bs_wr_one_ready();	
    save_bs_mode_var(0);
}

void bs_cmd_stby(void)
{
    bs_wr_one_ready();
    BS_SEND_CMD(bs_cmd_STBY);
    bs_wr_one_ready();	
    save_bs_mode_var(1);
}

void bs_cmd_slp(void)
{
    bs_wr_one_ready();
    BS_SEND_CMD(bs_cmd_SLP);
    bs_wr_one_ready();	
    save_bs_mode_var(2);
}

void bs_cmd_init_sys_run(void)
{
    bs_wr_one_ready();
    BS_SEND_CMD(bs_cmd_INIT_SYS_RUN);
    bs_wr_one_ready();
    save_bs_mode_var(0);
}

void bs_cmd_init_sys_stby(void)
{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
    BS_SEND_CMD(bs_cmd_INIT_SYS_STBY);
#endif
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

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
void bs_cmd_init_wavedev(u16 location)
{
    bs_cmd_block_t bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_INIT_WAVEDEV;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_INIT_WAVEDEV;
    bs_cmd_block.args[0]  = location;

    bs_send_cmd(&bs_cmd_block);
}
#endif

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
EXPORT_SYMBOL_GPL(bs_cmd_wr_reg);
	
void bs_cmd_rd_sfm(void)
{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
    BS_SEND_CMD(bs_cmd_RD_SFM);
#else
#endif
}

void bs_cmd_wr_sfm(u8 wd)
{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
    bs_cmd_block_t bs_cmd_block = { 0 };
    u16 data = wd;

    bs_cmd_block.command  = bs_cmd_WR_SFM;
    bs_cmd_block.type     = bs_cmd_type_write;
    bs_cmd_block.num_args = BS_CMD_ARGS_WR_SFM;
    bs_cmd_block.args[0]  = data;

    bs_send_cmd(&bs_cmd_block);
#else

#endif
}

void bs_cmd_end_sfm(void)
{
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
    BS_SEND_CMD(bs_cmd_END_SFM);
#else

#endif
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

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
//Henry Li: Pen Drawing Start
void bs_cmd_set_pen_draw(u16 x_max, u16 x_min,u16  y_max, u16 y_min, u16 mode_setup)
{
    bs_cmd_block_t  bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_PEN_DRAW;
    bs_cmd_block.type     = bs_cmd_type_write;
    
    bs_cmd_block.num_args = BS_CMD_ARGS_PEN_DRAW;
    bs_cmd_block.args[0]  = x_max;
    bs_cmd_block.args[1]  = x_min;
    bs_cmd_block.args[2]  = y_max;
    bs_cmd_block.args[3]  = y_min;
    bs_cmd_block.args[4]  = mode_setup;
      
    bs_send_cmd(&bs_cmd_block);	
}

//Henry Li: Pen Menu Select Wait
void bs_cmd_set_pen_menu(u16 mode_setup)
{
    bs_cmd_block_t  bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_PEN_MENU;
    bs_cmd_block.type     = bs_cmd_type_write;
    
    bs_cmd_block.num_args = BS_CMD_ARGS_PEN_MENU;
    bs_cmd_block.args[0]  = mode_setup;
      
    bs_send_cmd(&bs_cmd_block);	   
}

void bs_pen_calibcalculate(u16 T0, 
                           u16 T1, 
                           u16 Size, 
                           u16 PosOff,
                           u16 *KVal,
                           u16 *KSign,
                           u16 *Offset, 
                           u16 *OffsetSign)
{
    int debugon = 0;
   
    u16 T1MinusT0;
    if(debugon) printk("1. Check for the proper sides\n");
    if (T1>T0) 
    {
      T1MinusT0   = (T1-T0);
      *KSign       = 0;
    }
    else 
    {
      T1MinusT0   = (T0-T1); 
      *KSign       = 1;
    }
    if(debugon) printk("Size:%d, PosOff:%d, T1MinusT0:%d, T0:%d, T1:%d\n", Size, PosOff, T1MinusT0, T0, T1);
    u32 tempval1   = ((Size-(PosOff<<1))*(65536));
    u32 tempval2   = tempval1/T1MinusT0;
    
    if(debugon) printk("tempval1:%d, tempval2:%d \n",tempval1,tempval2);
    *KVal = (u16)((T1MinusT0 == 0)? 0: ((Size-(PosOff<<1))*(65536))/T1MinusT0);
    
    
    if(debugon) printk("2. Find the offset value\n");
    if (*KSign == 1)
    {//KSign is Negative
      if(debugon) printk("PosOff:%d, T0:%d, KVal:%d", PosOff, T0, *KVal);
       *Offset     =  (u16)(PosOff+((T0 *  (*KVal) )/(65536)));
       *OffsetSign =  0;
    }
    else
    {//KSign is Positive
       if (((T0*  (*KVal))/(65536)) > PosOff)
       {//skip x1
         *Offset      = (u16)(((T0*(*KVal))/(65536))-PosOff);
         *OffsetSign  =  1;
       }
       else
       {//skip x1
         *Offset      = (u16)(PosOff-((T0*(*KVal))/(65536)));
         *OffsetSign  = 0;
       }
    } 
   
}


//Henry Li: Line Draw Command
void bs_cmd_set_pen_line(u16 pix1_x, u16 pix1_y, u16  pix2_x, u16 pix2_y, u8 pen_color, u8 pen_dest, u16 pen_width)
{

    u16 mode_setup = (pen_color << 6) |(pen_dest<< 5) |pen_width ;
    bs_cmd_block_t  bs_cmd_block = { 0 };

    bs_cmd_block.command  = bs_cmd_PEN_LINE;
    bs_cmd_block.type     = bs_cmd_type_write;
    
    bs_cmd_block.num_args = BS_CMD_ARGS_PEN_LINE;
    bs_cmd_block.args[0]  = pix1_x;
    bs_cmd_block.args[1]  = pix1_y;
    bs_cmd_block.args[2]  = pix2_x;
    bs_cmd_block.args[3]  = pix2_y;
    bs_cmd_block.args[4]  = mode_setup;
      
    bs_send_cmd(&bs_cmd_block);
}

//Henry:  calibration routine
/******************************************************/
// Input: HSize and VSize of the panel
//        If rotated 0,180 - xyswap = 0, if 90,270 - xyswap = 1
/******************************************************/
void bs_ep3_pen_calibrate(u16 HSize, u16 VSize, u16 xyswap)
{
    int debugon = 1;

/*	1. Clear screen
 //add here if you need to clear screen.
*/
     if (debugon) {
	 	printk("1. ==================Clear screen\n");
		printk("HSize =0x%x VSize=0x%x xyswap=0x%x\n",HSize, VSize, xyswap);
     	}
/* 2. Calibration starts
*/ 
    u16 Point_X, Point_Y,
        Point_XCap0, Point_YCap0,
        Point_XCap1, Point_YCap1;

    if (debugon) printk("2.1 ==================Draw 1st point\n");
    Point_X = PenDraw_CalibratePosOffsetX; Point_Y = PenDraw_CalibratePosOffsetY;
    //6": Point_X = 150; Point_Y = 100;
    //9.7": Point_X = 200; Point_Y = 150;
    bs_cmd_set_pen_line (Point_X-4, Point_Y  , Point_X+6, Point_Y  , 0, 0, 4);
    bs_cmd_set_pen_line (Point_X  , Point_Y-4, Point_X  , Point_Y+6, 0, 0, 4);
    
    bs_cmd_set_pen_menu (0x0504);  //wait for touch
    bs_wr_one_ready();  	 //wait for ready to turn off from the wait for touch command
    bs_cmd_wr_reg(0x02DA, 0xc000);  //reset tx/rx 
    
    if (debugon) printk("2.1.2 Retrieve data\n");
    if (xyswap == 1) Point_XCap0 = bs_cmd_rd_reg(0x82);
    else             Point_XCap0 = bs_cmd_rd_reg(0x80);
    if (xyswap == 1) Point_YCap0 = bs_cmd_rd_reg(0x80);
    else             Point_YCap0 = bs_cmd_rd_reg(0x82);
    if (debugon) printk("===XT0= %d, YT0= %d\n",Point_XCap0,Point_YCap0);
      
 
    if (debugon) printk("2.2 ==================Draw 2nd point\n");
      
    Point_X = HSize-PenDraw_CalibratePosOffsetX; Point_Y = VSize-PenDraw_CalibratePosOffsetY;
    //6": Point_X = 650; Point_Y = 500;
    //9.7": Point_X = 1000; Point_Y = 675;    
    bs_cmd_set_pen_line (Point_X-4, Point_Y  , Point_X+6, Point_Y  , 0, 0, 4);
    bs_cmd_set_pen_line (Point_X  , Point_Y-4, Point_X  , Point_Y+6, 0, 0, 4);
    
    bs_cmd_set_pen_menu (0x0504);  //wait for touch
    bs_wr_one_ready(); //wait for ready to turn off from the wait for touch command
    bs_cmd_wr_reg(0x02DA, 0xc000);  //reset tx/rx 
    
    if (debugon) printk("2.2.2 Retrieve data\n");
    if (xyswap == 1) Point_XCap1 = bs_cmd_rd_reg(0x82);
    else             Point_XCap1 = bs_cmd_rd_reg(0x80);
    if (xyswap == 1) Point_YCap1 = bs_cmd_rd_reg(0x80);
    else             Point_YCap1 = bs_cmd_rd_reg(0x82);
    if (debugon) printk("===XT1= %d, YT1= %d\n",Point_XCap1,Point_YCap1);
 
    if (debugon) printk("2.3 ==================calculation of calibration parameters based on the following equaltions\n");
    //  Xd = Xt * Kx + XOffset
    //  Yd = Yt * Ky + YOffset
    //where
    //  Xd/Yd   = coordinates based on display pixels
    //  Xt/Yt   = raw coordinates from touchscreen controller
    //  Kx      = scaling factor for X axis
    //  Ky      = scaling factor for Y axis
    //  XOffset = compensation for misalignment on X axis
    //  YOffset = compensation for misalignment on Y axis
    u16 Kx, KxSign, XOffset, XOffsetSign;
    u16 Ky, KySign, YOffset, YOffsetSign;
    
    bs_pen_calibcalculate (Point_XCap0,Point_XCap1,HSize, PenDraw_CalibratePosOffsetX, &Kx, &KxSign, &XOffset, &XOffsetSign); //calculate Horizontal
    bs_pen_calibcalculate (Point_YCap0,Point_YCap1,VSize, PenDraw_CalibratePosOffsetY, &Ky, &KySign, &YOffset, &YOffsetSign); //calculate Vertical
    
    bs_cmd_wr_reg (0xb0, Kx|KxSign<<15);
    bs_cmd_wr_reg (0xb2, XOffset|XOffsetSign<<15);
    bs_cmd_wr_reg (0xb4, Ky|KySign<<15);
    bs_cmd_wr_reg (0xb6, YOffset|YOffsetSign<<15);
    bs_cmd_wr_reg (0xb8, xyswap);
    
    //debugging message
    if (debugon) 
    {
      printk("HSIZE= %d, VSIZE= %d\n",HSize,VSize);
      printk("XT0= %d, YT0= %d\n",Point_XCap0,Point_YCap0);
      printk("XT1= %d, YT1= %d\n",Point_XCap1,Point_YCap1);
      printk("XYSWAP      = %d\n",xyswap);
      printk("Kx          = %d\n",Kx);
      printk("KxSign      = %d\n",KxSign);
      printk("XOffset     = %d\n",XOffset);
      printk("XOffsetSign = %d\n",XOffsetSign);
      printk("Ky          = %d\n",Ky);
      printk("KySign      = %d\n",KySign);
      printk("YOffset     = %d\n",YOffset);
      printk("YOffsetSign = %d\n",YOffsetSign);
    }
   
}


/*********************************************************************/
// Setup the UART Baud Rate
// Input: Your OSC clock in MHz, Your desired Baudrate in Bps (usually 19200)
// Assumption: OSC clock around 24Mhz - 26.6Mhz
/*********************************************************************/
void bs_pen_uart_config(u32 OSCMhz, u32 baudrateoutBps)
{
int debugon=0;
/* 1. Setup the UART interface */
bs_cmd_wr_reg(0x02C0, 0x0001);

/* 2. Set the baud rates to 19200...  */
/* 2.1 Set UART transfer properties */
/*REG[0x02CC]
    bit1-0: 11        8bit data
    bit2   : 0        1bit stop
    bit3   : 0        no parity
    bit14 : 1         enable UART  
*/
bs_cmd_wr_reg(0x02CC, 0x4003);

/* 2.2 Setup the Baudrate */
//Baud rate = ActualSystemClock/(Baudrate x 16) = Integer . Factional 
//Baud rate = ((OSCMhz x 5)/2)/(Baudrate x 16)  = Integer . Factional
//
int    systemclock = OSCMhz * 5;
/*
double Baudrate    = (systemclock*1000000)/(baudrateoutBps<<4);
u16    BaudInt     = (u16)Baudrate;
u16    BaudFrc     = (u16)((Baudrate- BaudInt)*64);
*/
        systemclock = systemclock/2;		//Henry: Why divide by 2 ???
u32    Baudrate    = (systemclock*1000000)/(baudrateoutBps<<4);
u16    BaudInt     = (u16) (Baudrate & 0xffff);
Baudrate = systemclock*1000000 - BaudInt * (baudrateoutBps<<4);
u16    BaudFrc     = (u16) ( (Baudrate*64)/(baudrateoutBps<<4) );

bs_cmd_wr_reg(0x02D0, BaudInt);
bs_cmd_wr_reg(0x02D2, BaudFrc);  //011101 b

/* 2.3 Reset UART Tx and Rx Fifo */
bs_cmd_wr_reg(0x02DA, 0xc000);  //reset tx/rx 

/* 3. START WACOM Pen Input Writing ASCII "1" */
//bs_cmd_wr_reg(0x02D4, 0x0031);   


/*Henry Li: We can get "UART receive Read Data register 0x02d6" in HIRQ interrupt routine
in case of GPIO5 connect to Touch interrupt, like value = bs_cmd_rd_reg(0x02d6) */
 if (debugon) printk ("UART Config completed\n");
}

void bs_ep3_penwrite_mode(bool mode, unsigned char drawdata[10] )
{
    u16 x_max, x_min, y_max, y_min, mode_setup;
    int debugon = 0;

    x_max = (drawdata[0] << 8) | drawdata[1] ;
    x_min = (drawdata[2] << 8) | drawdata[3] ;
    y_max = (drawdata[4] << 8) | drawdata[5] ;
    y_min = (drawdata[6] << 8) | drawdata[7] ;
    mode_setup = (drawdata[8] << 8) | drawdata[9] ;	
    if (debugon) printk("mode=0x%x x_max=0x%x x_min=0x%x y_max=0x%x y_min=0x%x mode_setup=0x%x\n",mode, x_max, x_min, y_max, y_min, mode_setup);
    if (mode) {
		bs_pen_uart_config(26, 19200);
    	}
    else{
		bs_cmd_wr_reg(0x0146, 0x0016);  
		bs_cmd_wr_reg(0x0144, 0xd000);  

	    int reg_value=0;
	    int delay_counter=0;
   
	    while (1)
	   	{
	       reg_value = bs_cmd_rd_reg(0x2de);  
	   	while ( 0x0000 != reg_value & 0x1f00)
	   		{
	        	if (debugon) 
					printk("%s [2de]=0x%x [2D6]=0x%x \n", __FUNCTION__, reg_value, bs_cmd_rd_reg(0x2d6));
			else
					bs_cmd_rd_reg(0x2d6);
			msleep(20);	
			delay_counter++;
			if (delay_counter > 5*10 )
				break;
		       reg_value = bs_cmd_rd_reg(0x2de);  		   
	   		}
		msleep(20);
	      	if (debugon) 
				printk("%s [2de]=0x%x [2d6]=0x%x \n", __FUNCTION__, reg_value, bs_cmd_rd_reg(0x2d6));	
		else
				bs_cmd_rd_reg(0x2d6);
	       reg_value = bs_cmd_rd_reg(0x2de);  
	       if  ( 0x0000 == (reg_value & 0x1f00))
	       	break;
		if (delay_counter > 5*10 )
			break;	   
	   	}    
		
		bs_cmd_set_pen_draw(x_max, x_min, y_max, y_min, mode_setup);		
	}		
}

//-----------------------------------------
#endif

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
    einkfb_schedule_timeout_interruptible(BS_SFM_TIMEOUT, bs_sfm_bit_ready, (void *)&br);
}

bool bs_sfm_preflight(void)
{
    bool preflight = true;
    
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
        
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)        bs_cmd_wait_dspe_trg();
        bs_cmd_wait_dspe_frend();
#endif
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
    //return ( es );
    
    return BS_SFM_ESIG_M25P20;
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
  
    EINKFB_SCHEDULE_TIMEOUT_INTERRUPTIBLE(BS_SFM_TIMEOUT, bs_sfs_read_ready);
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
  
    EINKFB_SCHEDULE_TIMEOUT_INTERRUPTIBLE(BS_SFM_TIMEOUT, bs_sfs_read_ready);
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
    bool valid;
    int i;
    
    einkfb_debug_full( "... writing the serial flash memory (address=0x%08X, size=%d)\n", addr, size);
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
        }
    }

    einkfb_debug_full( "... writing the serial flash memory --- done\n");
    
    if ( !valid )
        einkfb_print_crit("Failed to verify write of Broadsheet Flash.\n");
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
    einkfb_schedule_timeout_interruptible(BS_CMD_TIMEOUT, bs_cmd_bit_ready, (void *)&br);
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

static u16 bs_ld_img_data(u8 *data, bool upd_area, u16 x, u16 y, u16 w, u16 h)
{
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
    
    bs_ld_img_start = jiffies;
 
    if ( bs_ld_fb )
    {
        u16 controller_data = ~*((u16 *)data);
        data_size >>= 1;
        
        for ( i = 0; i < data_size; i++ )
        {
            ep3_bs_wr_one(controller_data);
            EINKFB_SCHEDULE_BLIT(i << 1);
        }

        upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
    }
    else
    {
        u8 *controller_data = broadsheet_get_scratchfb();
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
                
                // Invert and nybble-swap the pixels going out to the controller.
                //
                controller_data[j++] = bs_4bpp_nybble_swap_table_inverted[s];

                if ( 0 == (j % 2) )
                    ep3_bs_wr_one(*((u16 *)&controller_data[j-2]));
            }
    
            // Finally, determine what's next.
            //
            if ( 0 == (++i % w) )
            {
                // Handle the horizontal hardware slop factor when necessary.
                //
                for ( k = 0; k < m; k++ )
                {
                    controller_data[j++] = ~EINKFB_WHITE;
                    
                    if ( 0 == (j % 2) )
                        ep3_bs_wr_one(*((u16 *)&controller_data[j-2]));
                }
            }
        
            EINKFB_SCHEDULE_BLIT(i);
        }
        
        // Handle the vertical slop factor when necessary.
        //
        for ( k = 0; k < n; k++ )
        {
            controller_data[j++] = ~EINKFB_WHITE;
            
            if ( 0 == (j % 2) )
                ep3_bs_wr_one(*((u16 *)&controller_data[j-2]));
        }
        
        upd_mode = BS_DONE_UPD_MODE(upd_mode);
    }

    bs_cmd_ld_img_end();

    bs_debug_upd_mode(upd_mode);
    
    return ( upd_mode );
}

extern unsigned char broadsheet_ep3_get_picture_mode(void);

/* Henry Li: send data[] to controller in different pixel mode and rotation mode */
const u8 gray_table_1bpp_4bpp[4] = {0x00, 0xF0, 0x0F, 0xFF};
const u8 gray_table_2bpp_4bpp[16] =
{
    0x00, 0x50, 0xA0, 0xF0, 0x05, 0x55, 0xA5, 0xF5,
    0x0A, 0x5A, 0xAA, 0xFA, 0x0F, 0x5F, 0xAF, 0xFF
};

/*
const u8 gray_table_1bpp_4bpp[4] = {0xFF, 0x0F, 0xF0, 0x00};
const u8 gray_table_2bpp_4bpp[16] =
{
    0xFF, 0xAF, 0x5F, 0x0F, 0xFA, 0xAA, 0x5A, 0x0A,
    0xF5, 0xA5, 0x55, 0x05, 0xF0, 0xA0, 0x50, 0x00
};
*/

static void load_img_data_4bpp(struct einkfb_info info, u32 *data, int data_size, u32 *controller_data) {

    unsigned char picture_mode = broadsheet_ep3_get_picture_mode();
    unsigned short __iomem *baseaddr = epd_get_bsaddr();
    u32 v;
    u32 inv = (picture_mode == 0) ? 0 : 0xffffffff;
    int i;

	for (i=0; i<data_size; i++) {

		v = data[i] ^ inv;
		controller_data[i] = v = ((v << 4) & 0xf0f0f0f0) | ((v >> 4) & 0x0f0f0f0f);
		 __raw_writew(v, baseaddr);
		 __raw_writew(v >> 16, baseaddr);

	}
	EINKFB_SCHEDULE();

}


void bs_only_load_img_data(u8 *data, bool upd_area, u16 x, u16 y, u16 w, u16 h)
{
    int i, data_size;

    struct einkfb_info info;
    unsigned char picture_mode = broadsheet_ep3_get_picture_mode();
    einkfb_get_info(&info);
    //info.bpp = EINKFB_4BPP;
    data_size = BPP_SIZE((w * h), info.bpp);
   
        u8 *controller_data = broadsheet_get_scratchfb();
        int j, k, l = 0, m = 0, n = 0;
        unsigned char s, pixels[4];
	 unsigned char twobit_shift=0;

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

	// DmitryZ - fast routine for 4bpp
	if (info.bpp == 4) {
		// !! no slop used !!
		load_img_data_4bpp(info, (u32 *)data, data_size/4, (u32 *)controller_data); 
		return;
	}

        switch ( info.bpp )
        {
            case EINKFB_4BPP:
                l = 1;
            break;
            
            case EINKFB_2BPP:
                l = 2;
            break;

            case EINKFB_1BPP:
                l = 4;
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
		      twobit_shift = (s & 0xf0) >> 4;					
                    pixels[0] = gray_table_2bpp_4bpp[twobit_shift];
		      twobit_shift = s & 0xf;					
                    pixels[1] = gray_table_2bpp_4bpp[twobit_shift];
                break;

                case EINKFB_1BPP:
		      twobit_shift = (s & 0xc) >> 2;
                    pixels[0] = gray_table_1bpp_4bpp[twobit_shift];
		      twobit_shift = s & 0x3;
                    pixels[1] = gray_table_1bpp_4bpp[twobit_shift];
		      twobit_shift = (s & 0xc0) >> 6;
                    pixels[2] = gray_table_1bpp_4bpp[twobit_shift];
		      twobit_shift = (s & 0x30) >> 4;
                    pixels[3] = gray_table_1bpp_4bpp[twobit_shift];
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
                                
                // Invert and nybble-swap the pixels going out to the controller.
                //
		 if ( 1 ==  picture_mode)	//bs_picture_inverted_mode
		 //if (0 ==  picture_mode)	// DmitryZ
                 controller_data[j++] = 0xff - s;
		 else
                 controller_data[j++] = s;		 

                if ( 0 == (j % 2) )
			ep3_bs_wr_one(*((u16 *)&controller_data[j-2]) );
            }
    
            // Finally, determine what's next.
            //
            if ( 0 == (++i % w) )
            {
                // Handle the horizontal hardware slop factor when necessary.
                //
                for ( k = 0; k < m; k++ )
                {
		      //if ( 1 ==  picture_mode)	//bs_picture_inverted_mode
       	      if (0 ==  picture_mode)
                      controller_data[j++] = ~EINKFB_WHITE;
		      else
                       controller_data[j++] = EINKFB_WHITE;
                    
                    if ( 0 == (j % 2) )
                        ep3_bs_wr_one(*((u16 *)&controller_data[j-2]));					
                }
            }
        
            EINKFB_SCHEDULE_BLIT(i);
        }
        
        // Handle the vertical slop factor when necessary.
        //
        for ( k = 0; k < n; k++ )
        {
	     //if ( 1 ==  picture_mode)	//bs_picture_inverted_mode
	     if (0 ==  picture_mode)
                controller_data[j++] = ~EINKFB_WHITE;
	     else
            controller_data[j++] = EINKFB_WHITE;
            
            if ( 0 == (j % 2) )
                ep3_bs_wr_one(*((u16 *)&controller_data[j-2]));
        }
}
EXPORT_SYMBOL_GPL(bs_only_load_img_data);
static u16 bs_cmd_orientation_to_rotmode(bool orientation, bool upside_down)
{
    u16 result = BS_ROTMODE_180;
    
    if ( EINKFB_ORIENT_PORTRAIT == orientation )
        result = BS_ROTMODE_90;
        
    // The ADS (FPGA) platform is always upside down with respect to everything else.
    // So, fix that here.
    //
    //if ( IS_ADS() )

    if ( BS_FPGA() )    
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
        EINKFB_SCHEDULE_TIMEOUT_INTERRUPTIBLE(BS_CMD_TIMEOUT, bs_disp_ready);
        
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
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)
  u16   a = bs_cmd_rd_reg(0x354),
        b = bs_cmd_rd_reg(0x356),
        c = bs_cmd_rd_reg(0x358),
        d = bs_cmd_rd_reg(0x35C),
        e = bs_cmd_rd_reg(0x35E);

  wfm_fvsn  = a & 0xFF;
  wfm_luts  = (a >> 8) & 0xFF;
  wfm_trc   = (b >> 8) & 0xFF;
  wfm_mc    = b & 0xFF;
  wfm_sb    = (c >> 8) & 0xFF;
  wfm_eb    = c & 0xFF;
  wfm_wmta  = d | (e << 16);
#endif  
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

// 
//
#define UPD_MODE_INIT(u)                                    \
    ((BS_UPD_MODE_INIT   == (u))   ||                       \
     (BS_UPD_MODE_REINIT == (u)))

#define IMAGE_TIMING_STRT_TYPE  0
#define IMAGE_TIMING_PROC_TYPE  1
#define IMAGE_TIMING_LOAD_TYPE  2
#define IMAGE_TIMING_DISP_TYPE  3
#define IMAGE_TIMING_STOP_TYPE  4

#define IMAGE_TIMING            "image_timing"
#define IMAGE_TIMING_STRT_NAME  "strt"
#define IMAGE_TIMING_PROC_NAME  "proc"
#define IMAGE_TIMING_LOAD_NAME  "load"
#define IMAGE_TIMING_DISP_NAME  "disp"

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
            EINKFB_PRINT_PERF_REL(IMAGE_TIMING, time, name);
        break;
        
        case IMAGE_TIMING_STOP_TYPE:
            EINKFB_PRINT_PERF_ABS(IMAGE_TIMING, time, bs_upd_mode_string);
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
{
                bs_cmd_wait_dspe_frend();
}				
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
                    bs_cmd_upd_init();
                    bs_cmd_wait_dspe_trg();
                    
                    bs_cmd_upd_full(BS_UPD_MODE(BS_UPD_MODE_GC), 0, 0);
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

                einkfb_debug("overriding upd_mode\n");
                bs_debug_upd_mode(bs_upd_mode);
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
        }

        // Done.
        //
        bs_cmd_wait_dspe_trg();
        
        // Stall the LUT pipeline in all full-refresh situations (i.e., prevent any potential
        // partial-refresh parallel updates from stomping on the full-refresh we just
        // initiated).
        //
#if defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)  || defined(CONFIG_HW_EP1_EVT)  || defined(CONFIG_HW_EP2_EVT)

        if ( wait_dspe_frend )
            bs_cmd_wait_dspe_frend();
#else
		mdelay(1000);
#endif
        
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
            bs_debug_upd_mode(bs_upd_mode);
            
            einkfb_print_image_timing(bs_image_start_time,      IMAGE_TIMING_STRT_TYPE);
            einkfb_print_image_timing(bs_image_processing_time, IMAGE_TIMING_PROC_TYPE);
            einkfb_print_image_timing(bs_image_loading_time,    IMAGE_TIMING_LOAD_TYPE);
            einkfb_print_image_timing(bs_image_display_time,    IMAGE_TIMING_DISP_TYPE);
            einkfb_print_image_timing(bs_image_stop_time,       IMAGE_TIMING_STOP_TYPE);
        }
        
        bs_upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
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
    
        einkfb_debug("overriding upd_mode\n");
        bs_debug_upd_mode(upd_mode);
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
    einkfb_debug("initializing broadsheet for 6-inch panel\n");
    
    bs_cmd_init_dspe_cfg(BS60_INIT_HSIZE, BS60_INIT_VSIZE, 
            BS60_INIT_SDRV_CFG, BS60_INIT_GDRV_CFG, BS60_INIT_LUTIDXFMT);
	unsigned long gpdata, gpa3;
	gpdata = __raw_readl(S3C64XX_GPJDAT) & (7 << 5);
	gpa3 = ((__raw_readl(S3C64XX_GPADAT) & 0x008) >> 3) & 0x01 ; //xyd:2011.04.21 SC8 EPD
	if(gpa3 == 0x01){
		if ((gpdata == (0x6 << 5)) || (gpdata == (0x2 << 5)) || (gpdata == (0x4 << 5)))
		{
			printk("xuyuda: sc8: %s\n", __FUNCTION__);
				//Henry: 2010-12-09 SC8 EPD
		   bs_cmd_init_dspe_tmg(BS60_INIT_FSLEN,
	            (0xd << 8) | BS60_INIT_FBLEN,
	            BS60_INIT_LSLEN,
	            (0x51 << 8) | BS60_INIT_LBLEN,
	            3);
		}
	}else if(gpa3 == 0x00){
		if ((gpdata == (0x7 << 5)) || (gpdata == (0x3 << 5)) || (gpdata == (0x0 << 5))){
			printk("xuyuda: sc4: %s\n", __FUNCTION__);
		   bs_cmd_init_dspe_tmg(BS60_INIT_FSLEN,
	            (BS60_INIT_FELEN << 8) | BS60_INIT_FBLEN,
	            BS60_INIT_LSLEN,
	            (BS60_INIT_LELEN << 8) | BS60_INIT_LBLEN,
	            BS60_INIT_PIXCLKDIV);
		}
	}
    bs_cmd_print_disp_timings();
    
    bs_cmd_set_wfm(wa);
    bs_cmd_print_wfm_info();
    
    einkfb_debug("display engine initialized with waveform 0x%X\n", wa);

    bs_cmd_clear_gd();

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x01A, 0xa); // i2c clock divider	//Henry: Check latter???
#else
    bs_cmd_wr_reg(0x01A, 4); // i2c clock divider
#endif    
    bs_cmd_wr_reg(0x320, 0); // temp auto read on
    
    if ( full )
    {
//        bs60_flash();
//        bs60_black();
//        bs60_white();
         bs_upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
    }
    else
    {
        if ( BS_UPD_MODE_INIT == bs_upd_mode )
        {
            bs_upd_mode = BS_UPD_MODE_REINIT;
            bs60_white();
        }
    }
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x140, 0x120); 
    udelay(25);
#endif    		
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
extern void LIST_REG(void);

void bs97_init(int wa, bool full)
{
    int iba = BS97_INIT_HSIZE * BS97_HARD_VSIZE * 2;
    
    einkfb_debug("initializing broadsheet for 9.7-inch panel\n");

    bs_cmd_init_dspe_cfg(BS97_INIT_HSIZE, BS97_INIT_VSIZE, 
            BS97_INIT_SDRV_CFG, BS97_INIT_GDRV_CFG, BS97_INIT_LUTIDXFMT);
    bs_cmd_init_dspe_tmg(BS97_INIT_FSLEN,
			(BS97_INIT_FELEN << 8) | BS97_INIT_FBLEN,
			BS97_INIT_LSLEN,
			(BS97_INIT_LELEN << 8) | BS97_INIT_LBLEN,
			BS97_INIT_PIXCLKDIV);

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
#else
    bs_cmd_wr_reg(0x310, (iba & 0xFFFF));
    bs_cmd_wr_reg(0x312, ((iba >> 16) & 0xFFFF));
#endif

    bs_cmd_print_disp_timings();

    bs_cmd_set_wfm(wa);
    
    bs_cmd_print_wfm_info();
    
    einkfb_debug("display engine initialized with waveform 0x%X\n", wa);
    
    bs_cmd_clear_gd();

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x01A, 0xa); // i2c clock divider	//Henry: Check latter???
#else
    bs_cmd_wr_reg(0x01A, 4); // i2c clock divider
#endif
    bs_cmd_wr_reg(0x320, 0); // temp auto read on

    if ( full )
    {
        //bs97_flash();	//Henry 0801
        //bs97_black(); //binchong: improve the boot time 
        //bs97_white();
         bs_upd_mode = BS_UPD_MODE(BS_UPD_MODE_GC);
    }
    else
    {
        if ( BS_UPD_MODE_INIT == bs_upd_mode )
        {
            bs_upd_mode = BS_UPD_MODE_REINIT;
            bs97_white();
        }
    }

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x140, 0x120); 
    udelay(25);
#endif    	
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

static unsigned char bs_get_fpl_size(void);
static bool resume_after_suspend=false;
void reduce_ghosting(bool state)
{
/*
        switch ( bs_get_fpl_size() )
        {
            case EINK_FPL_SIZE_97:
	        bs97_black();
	        bs97_white();
            break;
            
            case EINK_FPL_SIZE_60:
            default:
		 bs60_black();
	        bs60_white();
            break;
        }
*/        
	resume_after_suspend=state;
}
bool get_resume_after_suspend(void)
{
	return resume_after_suspend;
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

    if ( EINKFB_SUCCESS == EINKFB_SCHEDULE_TIMEOUT_INTERRUPTIBLE(BS_PLL_TIMEOUT, bs_pll_ready) )
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
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)		
        BSC_WR_REG(0x106, 0x0194);
#else
        BSC_WR_REG(0x106, 0x0203);
#endif    
    bs_init_spi();
    
    if ( BS_ASIC() )
        bs_hw_test();

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
	bs_cmd_init_wavedev(BS_WFM_SEL_SFM);
#endif
	
}

static void bs_sys_init(void)
{
    bs_cmd_init_sys_run();
    
    if ( BS_ASIC() )
    {
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)		    
        bs_cmd_wr_reg(0x106, 0x0194);    
#else
        bs_cmd_wr_reg(0x106, 0x0203);    
#endif
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

static void bs_get_resolution(unsigned char fpl_size, bool orientation, bs_resolution_t *res)
{
    switch ( fpl_size )
    {
        // Portrait  ->  824 x 1200 software,  826 x 1200 hardware
        // Landscape -> 1200 x  824 software, 1200 x  826 hardware
        //
        case EINK_FPL_SIZE_97:
            res->x_hw = BS97_INIT_HSIZE; 
            res->y_hw = BS97_HARD_VSIZE;
            
            res->x_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_SOFT_VSIZE : BS97_INIT_HSIZE;
            res->y_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_INIT_HSIZE : BS97_SOFT_VSIZE;
            
            res->x_mm = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_MM_825  : BS97_MM_1200;
            res->y_mm = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS97_MM_1200 : BS97_MM_825;
        break;
        
        // Portrait  ->  600 x  800 software/hardware
        // Landscape ->  800 x  600 software/hardware
        //
        case EINK_FPL_SIZE_60:
        default:
            res->x_hw = BS60_INIT_HSIZE;
            res->y_hw = BS60_INIT_VSIZE;
            
            res->x_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_INIT_VSIZE : BS60_INIT_HSIZE;
            res->y_sw = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_INIT_HSIZE : BS60_INIT_VSIZE;
        
            res->x_mm = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_MM_600 : BS60_MM_800;
            res->y_mm = (EINKFB_ORIENT_PORTRAIT == orientation) ? BS60_MM_800 : BS60_MM_600;
        break;
    }

    // Broadsheet actually aligns the hardware horizontal resolution to a 32-bit boundary.
    //
    res->x_hw = BS_ROUNDUP32(res->x_hw);
}

#define BS_SW_INIT_CONTROLLER(f) bs_sw_init_controller(f, EINKFB_ORIENT_PORTRAIT, NULL)

void bs_sw_init_controller(bool full, bool orientation, bs_resolution_t *res)
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
            bs_sw_done();
        
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
            bs_sw_done();
    }
    
    // Under special circumstances (as when Broadsheet's flash is blank), we need
    // to bootstrap Broadsheet.  And, in that case, we just return the default,
    // 6-inch panel resolution if requested.
    //
    if ( bs_bootstrap )
    {
        if ( bs_hw_up )
            bs_bootstrap_init();
        
        if ( res )
            fpl_size = EINK_FPL_SIZE_60;
    }
    else
    {
        // Return panel-specific resolutions by reading the panel size from the
        // waveform header if requested.
        //
        bs_sys_init();
        
        if ( res )
            fpl_size = bs_get_fpl_size();
    }

    if ( fpl_size )
        bs_get_resolution(fpl_size, orientation, res);
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
        // Otherwise, go through panel-specific initialization.
        //
        switch ( bs_get_fpl_size() )
        {
            case EINK_FPL_SIZE_97:
                bs97_init(BS_WFM_ADDR, full);
            break;
            
            case EINK_FPL_SIZE_60:
            default:
                bs60_init(BS_WFM_ADDR, full);
            break;
        }
        
        // Put us into the right orientation.
        //
		//mdelay(100);		//Henry 0801 temporary
		//bs_cmd_set_rotmode(BS_ROTMODE_90);		
        bs_cmd_set_rotmode(bs_cmd_orientation_to_rotmode(BROADSHEET_GET_ORIENTATION()));
        
        // Determine which update modes to use.
        //
        bs_set_upd_modes();
        
        // Remember that the framebuffer has been initialized.
        //
        set_fb_init_flag(BOOT_GLOBALS_FB_INIT_FLAG);
    }

    // Say that we're now in the run state.
    //
    bs_power_state = bs_power_state_run;

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    u16 rd_reg;
       rd_reg = 0x99;
	bs_cmd_wr_reg(0x0204, rd_reg);
	udelay(25);
#endif
    return ( full );
}


extern bool bs_quickly_hw_init(void);
bool quick_resume_hook(void)		//Henry: For PB quick resume 0927
{
	bool full=true;
	bs_bootstrap = false;	
	//bs2_sw_init_controller(full, EINKFB_ORIENT_PORTRAIT, NULL);
	
	//bs_hw_init();
	bs_quickly_hw_init();
       bs_hw_up = bs_ready = true;		//important !!!
       bs_cmd_init_sys_run();
	
	bs_sw_init_panel(full);
	bs_bootstrap = false;
	bs_hw_up = true;
	bs_ready = true;
	bs_power_state = bs_power_state_run;	

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    u16 rd_reg;
    unsigned char dump[10];

    bs_ep3_penwrite_mode(1, dump);
    rd_reg = bs_cmd_rd_reg(0x2cc) & ~(1 << 6);
    bs_cmd_wr_reg(0x2cc, (rd_reg & ~(1 << 6)));	//do not transmit BREAK condition
#endif	
	printk("Leave %s\n", __FUNCTION__);
	return (false);
}

#define BS_SW_INIT(f) bs_sw_init(f, f)

bool bs_sw_init(bool controller_full, bool panel_full)
{
    BS_SW_INIT_CONTROLLER(controller_full);

#if  defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)	
	unsigned long gpdata,gpa3; //xyd:2011.04.21 SC8 EPD
	gpa3 = ((__raw_readl(S3C64XX_GPADAT) & 0x008) >> 3) & 0x01 ; //xyd:2011.04.21 SC8 EPD
	gpdata = __raw_readl(S3C64XX_GPJDAT) & (7 << 5);
	if (gpa3 == 0x01){
		global_gpj5_7= gpdata & 0xff;		//Henry: 2010-12-09 SC8 EPD
	}
#endif
	
    return ( bs_sw_init_panel(panel_full) );
}

unsigned char get_EPD_type(void)
{
	return global_gpj5_7;
}
void set_EPD_type(unsigned char value)
{
	global_gpj5_7 = value;
}

void bs_sw_done(void)
{
    // Take the hardware all the way down.
    //
    bs_hw_done();
    
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
        bs_sw_done();
        
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

void ep3_reinit(void)
{
	BS_SW_INIT(FULL_BRINGUP);
}

extern void epd_show_black(unsigned short cmd); //100114 binchong: improve the system suspend and resume time

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
//100114 binchong: improve the system suspend and resume time
                        //bs_cmd_ld_img_upd_data(fx_update_partial, UPD_DATA_RESTORE);
                        //epd_show_black(0);   // disable to improve the resume time              
                        bs_power_state = bs_power_state_run; 
//100114 binchong: improve the system suspend and resume time
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
//100114 binchong: improve the system suspend and resume time
            	//bs97_black();
                //broadsheet_set_power_state(bs_power_state_sleep);
                bs_sw_done();
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
                //broadsheet_set_power_state(bs_power_state_run);
//100114 binchong: improve the system suspend and resume time
                bs_cmd_stby();
                bs_power_state = bs_power_state_standby;
            break;
            
            // run -> sleep
            //
            case bs_power_state_sleep:
//100114 binchong: improve the system suspend and resume time

                //broadsheet_set_power_state(bs_power_state_run);
               
//#ifdef X3_EPD_POWER
                bs_cmd_slp();
                bs_power_state = bs_power_state_sleep;
//#endif                
//100114 binchong: improve the system suspend and resume time

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

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x214, 0x0101);           // Trigger a temperature-sensor read.
#else
    bs_cmd_wr_reg(0x214, 0x0001);           // Trigger a temperature-sensor read.
#endif    
    bs_cmd_wait_for_bit(0x212, 0, 0);       // Wait for the sensor to be idle.
    
    raw_temp = (int)bs_cmd_rd_reg(0x216);   // Read the result.

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    bs_cmd_wr_reg(0x322, raw_temp);           // Henry: 2010-10-05  ==> REG[0322h] Temperature Value Register
#endif
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

static unsigned long broadsheet_get_ram_base(void)
{
    return ( (unsigned long)(broadsheet_get_ram_select() * broadsheet_get_ram_select_size()) );
}

int broadsheet_read_from_ram(unsigned long addr, unsigned char *data, unsigned long size)
{
    unsigned long start = broadsheet_get_ram_base() + addr;
    int result = 0;

    if ( BS_SDR_SIZE >= (start + size) )
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

    if ( BS_SDR_SIZE >= (start + blen) )
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

int broadsheet_read_from_flash_byte(unsigned long addr, unsigned char *data)
{
    static unsigned char cache_bytes[16];
    static unsigned long cache_addr = 0xffffffff;
    int ret = 0;

    unsigned long base = (addr & 0xfffffff0);
    if (base == cache_addr) {
	*data = cache_bytes[addr-base];
	return 1;
    }
    ret = broadsheet_read_from_flash(base, cache_bytes, 16);
    if (ret) {
	cache_addr = base;
	*data = cache_bytes[addr-base];
    } else {
	printk("Error reading flash address %x !!!\n", base);
	cache_addr = 0xffffffff;
    }
    return ret;
}

int broadsheet_read_from_flash_short(unsigned long addr, unsigned short *data)
{
    return ( broadsheet_read_from_flash(addr, (unsigned char *)data, sizeof(unsigned short)) );
}

int broadsheet_read_from_flash_long(unsigned long addr, unsigned long *data)
{
    return ( broadsheet_read_from_flash(addr, (unsigned char *)data, sizeof(unsigned long)) );
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
