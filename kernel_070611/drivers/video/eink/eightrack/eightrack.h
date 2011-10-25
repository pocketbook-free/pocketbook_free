// Register operations definitions
//#define write_reg(reg, val) writel((val), (reg))
//#define read_reg(reg, len, pos) ( readl((reg)) >> pos ) & ~( 0xffffffff << len )

// GPIO registers
#define GPIO_BASE_ADDR 0x40E00000
#define GPIO_MAP_SIZE  0x1000
#define GPIO_MAP_MASK  0x0FFF

#define GPLR0_ADDR 0x40E00000
#define GPLR1_ADDR 0x40E00004
#define GPLR2_ADDR 0x40E00008
#define GPDR0_ADDR 0x40E0000C
#define GPDR1_ADDR 0x40E00010
#define GPDR2_ADDR 0x40E00014
#define GPSR0_ADDR 0x40E00018
#define GPSR1_ADDR 0x40E0001C
#define GPSR2_ADDR 0x40E00020
#define GPCR0_ADDR 0x40E00024
#define GPCR1_ADDR 0x40E00028
#define GPCR2_ADDR 0x40E0002C
#define GRER0_ADDR 0x40E00030
#define GRER1_ADDR 0x40E00034
#define GRER2_ADDR 0x40E00038
#define GFER0_ADDR 0x40E0003C
#define GFER1_ADDR 0x40E00040
#define GFER2_ADDR 0x40E00044
#define GEDR0_ADDR 0x40E00048
#define GEDR1_ADDR 0x40E0004C
#define GEDR2_ADDR 0x40E00050
#define GAFR0_L_ADDR 0x40E00054
#define GAFR0_U_ADDR 0x40E00058
#define GAFR1_L_ADDR 0x40E0005C
#define GAFR1_U_ADDR 0x40E00060
#define GAFR2_L_ADDR 0x40E00064
#define GAFR2_U_ADDR 0x40E00068

// 8Track ERR line:  GPIO 37
#define EIGHT_TRACK_ERR_IRQ       IRQ_GPIO(37)
#define EIGHT_TRACK_ERR_REG_MASK  0xffffffff
#define EIGHT_TRACK_ERR_GAFR_MASK 0xffffffff
#define EIGHT_TRACK_ERR_DR_ADDR   GPDR1_ADDR
#define EIGHT_TRACK_ERR_DR_LEN    1
#define EIGHT_TRACK_ERR_DR_POS    5
#define EIGHT_TRACK_ERR_FE_ADDR   GFER1_ADDR
#define EIGHT_TRACK_ERR_FE_LEN    1
#define EIGHT_TRACK_ERR_FE_POS    5
#define EIGHT_TRACK_ERR_RE_ADDR   GRER1_ADDR
#define EIGHT_TRACK_ERR_RE_LEN    1
#define EIGHT_TRACK_ERR_RE_POS    5
#define EIGHT_TRACK_ERR_ED_ADDR   GEDR1_ADDR
#define EIGHT_TRACK_ERR_ED_LEN    1
#define EIGHT_TRACK_ERR_ED_POS    5
#define EIGHT_TRACK_ERR_AF_ADDR   GAFR1_L_ADDR
#define EIGHT_TRACK_ERR_AF_LEN    2
#define EIGHT_TRACK_ERR_AF_POS    10
#define EIGHT_TRACK_ERR_LR_ADDR   GPLR1_ADDR
#define EIGHT_TRACK_ERR_LR_LEN    1
#define EIGHT_TRACK_ERR_LR_POS    5

// 8Track RST line:  GPIO 51
#define EIGHT_TRACK_RST_IRQ       IRQ_GPIO(51)
#define EIGHT_TRACK_RST_REG_MASK  0xffffffff
#define EIGHT_TRACK_RST_GAFR_MASK 0xffffffff
#define EIGHT_TRACK_RST_DR_ADDR   GPDR1_ADDR
#define EIGHT_TRACK_RST_DR_LEN    1
#define EIGHT_TRACK_RST_DR_POS    19
#define EIGHT_TRACK_RST_FE_ADDR   GFER1_ADDR
#define EIGHT_TRACK_RST_FE_LEN    1
#define EIGHT_TRACK_RST_FE_POS    19
#define EIGHT_TRACK_RST_RE_ADDR   GRER1_ADDR
#define EIGHT_TRACK_RST_RE_LEN    1
#define EIGHT_TRACK_RST_RE_POS    19
#define EIGHT_TRACK_RST_ED_ADDR   GEDR1_ADDR
#define EIGHT_TRACK_RST_ED_LEN    1
#define EIGHT_TRACK_RST_ED_POS    19
#define EIGHT_TRACK_RST_AF_ADDR   GAFR1_U_ADDR
#define EIGHT_TRACK_RST_AF_LEN    2
#define EIGHT_TRACK_RST_AF_POS    6
#define EIGHT_TRACK_RST_LR_ADDR   GPLR1_ADDR
#define EIGHT_TRACK_RST_LR_LEN    1
#define EIGHT_TRACK_RST_LR_POS    19
#define EIGHT_TRACK_RST_SR_ADDR   GPSR1_ADDR
#define EIGHT_TRACK_RST_SR_LEN    1
#define EIGHT_TRACK_RST_SR_POS    19
#define EIGHT_TRACK_RST_CR_ADDR   GPCR1_ADDR
#define EIGHT_TRACK_RST_CR_LEN    1
#define EIGHT_TRACK_RST_CR_POS    19

// 8Track STBY line:  GPIO 82
#define EIGHT_TRACK_STBY_IRQ       IRQ_GPIO(82)
#define EIGHT_TRACK_STBY_REG_MASK  0x001fffff
#define EIGHT_TRACK_STBY_GAFR_MASK 0x000003ff
#define EIGHT_TRACK_STBY_DR_ADDR   GPDR2_ADDR
#define EIGHT_TRACK_STBY_DR_LEN    1
#define EIGHT_TRACK_STBY_DR_POS    18
#define EIGHT_TRACK_STBY_FE_ADDR   GFER2_ADDR
#define EIGHT_TRACK_STBY_FE_LEN    1
#define EIGHT_TRACK_STBY_FE_POS    18
#define EIGHT_TRACK_STBY_RE_ADDR   GRER2_ADDR
#define EIGHT_TRACK_STBY_RE_LEN    1
#define EIGHT_TRACK_STBY_RE_POS    18
#define EIGHT_TRACK_STBY_ED_ADDR   GEDR2_ADDR
#define EIGHT_TRACK_STBY_ED_LEN    1
#define EIGHT_TRACK_STBY_ED_POS    18
#define EIGHT_TRACK_STBY_AF_ADDR   GAFR2_U_ADDR
#define EIGHT_TRACK_STBY_AF_LEN    2
#define EIGHT_TRACK_STBY_AF_POS    4
#define EIGHT_TRACK_STBY_LR_ADDR   GPLR2_ADDR
#define EIGHT_TRACK_STBY_LR_LEN    1
#define EIGHT_TRACK_STBY_LR_POS    18
#define EIGHT_TRACK_STBY_SR_ADDR   GPSR2_ADDR
#define EIGHT_TRACK_STBY_SR_LEN    1
#define EIGHT_TRACK_STBY_SR_POS    18
#define EIGHT_TRACK_STBY_CR_ADDR   GPCR2_ADDR
#define EIGHT_TRACK_STBY_CR_LEN    1
#define EIGHT_TRACK_STBY_CR_POS    18

// 8Track RDY line:  GPIO 84
#define EIGHT_TRACK_RDY_IRQ       IRQ_GPIO(84)
#define EIGHT_TRACK_RDY_REG_MASK  0x001fffff
#define EIGHT_TRACK_RDY_GAFR_MASK 0x000003ff
#define EIGHT_TRACK_RDY_DR_ADDR   GPDR2_ADDR
#define EIGHT_TRACK_RDY_DR_LEN    1
#define EIGHT_TRACK_RDY_DR_POS    20
#define EIGHT_TRACK_RDY_FE_ADDR   GFER2_ADDR
#define EIGHT_TRACK_RDY_FE_LEN    1
#define EIGHT_TRACK_RDY_FE_POS    20
#define EIGHT_TRACK_RDY_RE_ADDR   GRER2_ADDR
#define EIGHT_TRACK_RDY_RE_LEN    1
#define EIGHT_TRACK_RDY_RE_POS    20
#define EIGHT_TRACK_RDY_ED_ADDR   GEDR2_ADDR
#define EIGHT_TRACK_RDY_ED_LEN    1
#define EIGHT_TRACK_RDY_ED_POS    20
#define EIGHT_TRACK_RDY_AF_ADDR   GAFR2_U_ADDR
#define EIGHT_TRACK_RDY_AF_LEN    2
#define EIGHT_TRACK_RDY_AF_POS    8
#define EIGHT_TRACK_RDY_LR_ADDR   GPLR2_ADDR
#define EIGHT_TRACK_RDY_LR_LEN    1
#define EIGHT_TRACK_RDY_LR_POS    20


// LCD controller registers
#define LCD_BASE_ADDR 0x44000000
#define LCD_MAP_SIZE  0x1000
#define LCD_MAP_MASK  0x0FFF

#define LCCR0_ADDR 0x44E00000
#define LCCR1_ADDR 0x44E00004
#define LCCR2_ADDR 0x44E00008
#define LCCR3_ADDR 0x44E0000C

// PPL - LCD controller pixels per line
#define PPL_ADDR   LCCR1_ADDR
#define PPL_LEN    10
#define PPL_POS    0

// LPP - LCD controller lines per panel
#define LPP_ADDR   LCCR2_ADDR
#define LPP_LEN    10
#define LPP_POS    0

// Command list for 8Track controller
enum cmd_type {
  CMD_POWERUP = 0,
  CMD_CONFIG,
  CMD_INIT,
  CMD_REFRESH,
  CMD_DISPLAY,
  CMD_UNKNOWN
};

// Waveform types for 8Track controller
#define INIT_WAVEFORM 0
#define MU_WAVEFORM   1
#define GU_WAVEFORM   2
#define GC_WAVEFORM   3

#define NUM_OF_WAVEFORMS 4  // Depends on the number of waveform types above
#define MAX_WFD_SIZE 8192   // Maximum waveform data size, in words

#define EINK_6_INCH_FW   416  // Frame buffer width for 6" eInk panel
#define EINK_9P7_INCH_FW 600  // Frame buffer width for 9.7" eInk panel

#define MSEC_PER_FRAME 20  // Frame is refreshed 50 times per second

// States for the 8Track controller state machine
enum eightrack_state_t {
   RESET,
   STANDBY,
   WAITING_FOR_READY,
   READY,
   WAITING_FOR_BUSY,
   BUSY,
   WAITING_FOR_POWERUP_READY,
   WAITING_FOR_CONFIG_BUSY,
   WAITING_FOR_CONFIG_READY,
   WAITING_FOR_INIT_BUSY,
   WAITING_FOR_INIT_READY,
   WAITING_FOR_DISPLAY_BUSY,
   WAITING_FOR_DISPLAY_READY,
   ERROR
   };
typedef enum eightrack_state_t eightrack_state_t;

// External function definitions
extern int eightrack_init(struct fb_info *info);
extern void eightrack_exit(void);
extern int is_eightrack_ready(void);
extern int send_fb_to_eightrack(int waveform_type);

