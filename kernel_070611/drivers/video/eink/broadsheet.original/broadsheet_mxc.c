/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_mxc.c --
 *  eInk frame buffer device HAL broadsheet hw
 *
 *      Copyright (C) 2005-2009 BSP2
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#undef USE_BS_IRQ

#include <plat/regs-gpio.h>
#include "../hal/einkfb_hal.h"
#include "broadsheet.h"


#ifdef MXC31
#undef USE_BS_IRQ

#include <asm/arch-mxc/mx31_pins.h>
#include <asm/arch/board_id.h>
#include <asm/arch/gpio.h>
#include <asm/arch/ipu.h>

#ifdef USE_BS_IRQ
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/mach/irq.h>
#endif

extern void slcd_gpio_config(void); // From mx31ads_gpio.c/mario_gpio.c
extern void gpio_lcd_active(void);  // Ditto.

#endif


#define EPD_CMD_DELAY 25

//#include "../hal/einkfb_hal.h"
//#include "broadsheet.h"
typedef enum {
        DAT,
        CMD
} cmddata_t;

static int ipu_adc_write_cmd(cmddata_t type,uint32_t data);
static int ipu_adc_read_data(void);
unsigned short __iomem* bsaddr;
unsigned long gpcon;
unsigned long gpdata;
unsigned long sromdata;

#define HIGH_RESET_PIN	gpdata = __raw_readl(S3C64XX_GPPDAT);gpdata =(gpdata & ~(1<<9));__raw_writel(gpdata|(0x1<<9),S3C64XX_GPPDAT)
#define LOW_RESET_PIN	gpdata = __raw_readl(S3C64XX_GPPDAT);gpdata =(gpdata & ~(1<<9));__raw_writel(gpdata,S3C64XX_GPPDAT)

#define HIGH_HDC_PIN    gpdata = __raw_readl(S3C64XX_GPODAT);gpdata =(gpdata & ~(1<<2));__raw_writel(gpdata|(0x1<<2),S3C64XX_GPODAT)
#define LOW_HDC_PIN    gpdata = __raw_readl(S3C64XX_GPODAT);gpdata =(gpdata & ~(1<<2));__raw_writel(gpdata,S3C64XX_GPODAT)

#define GET_HRDY_STATUS (__raw_readl(S3C64XX_GPODAT)>>4)&(0x1<<0)


#define WR_GPIO_LINE(addr,val)   __raw_writew(val,addr);
#define RD_GPIO_LINE(addr)       __raw_readw(addr);

/*
#ifdef CONFIG_MACH_MX31ADS
#include <asm/arch-mxc/board-mx31ads.h>
uint16_t pin_addr;  // Needed for __raw_writew calls
#    define BROADSHEET_HIRQ_LINE      MX31_PIN_GPIO3_0
#    define BROADSHEET_HRDY_LINE      MX31_PIN_GPIO3_1
#    define BROADSHEET_RST_LINE       PBC_BCTRL2_LDC_RST0
#    define BROADSHEET_RESET_VAL      PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR
#    define BROADSHEET_NON_RESET_VAL  PBC_BASE_ADDRESS + PBC_BCTRL2_SET
#    define WR_GPIO_LINE(addr, val)   pin_addr=addr; __raw_writew(pin_addr, val);
#elif  CONFIG_MACH_MARIO_MX
#include <asm/arch-mxc/board-mario.h>
#    define BROADSHEET_HIRQ_LINE      MX31_PIN_SD_D_CLK
#    define BROADSHEET_HRDY_LINE      MX31_PIN_SD_D_I
#    define BROADSHEET_RST_LINE       MX31_PIN_SD_D_IO
#    define BROADSHEET_RESET_VAL      0
#    define BROADSHEET_NON_RESET_VAL  1
#    define WR_GPIO_LINE(addr, val)   mxc_set_gpio_dataout(addr, val);
#endif
#define BROADSHEET_HIRQ_IRQ           IOMUX_TO_IRQ(BROADSHEET_HIRQ_LINE)
*/

// Other Broadsheet constants
#define BROADSHEET_DISPLAY_NUMBER      DISP0
#define BROADSHEET_PIXEL_FORMAT        IPU_PIX_FMT_GENERIC
#define BROADSHEET_SCREEN_HEIGHT       600
#define BROADSHEET_SCREEN_HEIGHT_NELL  825
#define BROADSHEET_SCREEN_WIDTH        800
#define BROADSHEET_SCREEN_WIDTH_NELL   1200
#define BROADSHEET_SCREEN_SIZE         (BROADSHEET_SCREEN_WIDTH * BROADSHEET_SCREEN_HEIGHT)
#define BROADSHEET_SCREEN_SIZE_NELL    (BROADSHEET_SCREEN_WIDTH_NELL * BROADSHEET_SCREEN_HEIGHT_NELL)
#define BROADSHEET_SCREEN_LEFT_OFFSET  0
#define BROADSHEET_READY_TIMEOUT       BS_RDY_TIMEOUT

// Select which R/W timing parameters to use (slow or fast); these
// settings affect the Freescale IPU Asynchronous Parallel System 80
// Interface (Type 2) timings.
// The slow timing parameters are meant to be used for bring-up
// and debugging the functionality.  The fast timings are up to the
// Broadsheet specifications for the 16-bit Host Interface Timing, and
// are meant to maximize performance.
//
////#undef SLOW_RW_TIMING

/*
#ifdef SLOW_RW_TIMING // Slow timing for bring-up or debugging
#    define BROADSHEET_READ_CYCLE_TIME     1900  // nsec
#    define BROADSHEET_READ_UP_TIME         170  // nsec
#    define BROADSHEET_READ_DOWN_TIME      1040  // nsec
#    define BROADSHEET_READ_LATCH_TIME     1900  // nsec
#    define BROADSHEET_PIXEL_CLK        5000000
#    define BROADSHEET_WRITE_CYCLE_TIME    1230  // nsec
#    define BROADSHEET_WRITE_UP_TIME        170  // nsec
#    define BROADSHEET_WRITE_DOWN_TIME      680  // nsec
#else // Fast timing, according to Broadsheet specs 
// NOTE: these timings assume a Broadsheet System Clock at the 
//       maximum speed of 66MHz (15.15 nsec period) 
#    define BROADSHEET_READ_CYCLE_TIME      110  // nsec 
#    define BROADSHEET_READ_UP_TIME           1  // nsec 
#    define BROADSHEET_READ_DOWN_TIME       100  // nsec 
#    define BROADSHEET_READ_LATCH_TIME      110  // nsec 
#    define BROADSHEET_PIXEL_CLK        5000000 
#    define BROADSHEET_WRITE_CYCLE_TIME      83  // nsec 
#    define BROADSHEET_WRITE_UP_TIME          1  // nsec 
#    define BROADSHEET_WRITE_DOWN_TIME       72  // nsec 
#endif
*/

#if PRAGMAS
    #pragma mark Definitions/Globals
#endif

//static uint16_t   broadsheet_screen_height = BROADSHEET_SCREEN_HEIGHT;
//static uint16_t   broadsheet_screen_width  = BROADSHEET_SCREEN_WIDTH;
//static uint32_t   broadsheet_screen_size   = BROADSHEET_SCREEN_SIZE;

#if PRAGMAS
    #pragma mark -
    #pragma mark Broadsheet HW Primitives
    #pragma mark -
#endif

/*
 * Wait until the HRDY line of the Broadsheet controller is asserted.
 */

static bool bs_hardware_ready(void *unused)
{
//    return ( !broadsheet_force_hw_not_ready() && mxc_get_gpio_datain(BROADSHEET_HRDY_LINE) );
    return ( !broadsheet_force_hw_not_ready() && GET_HRDY_STATUS );
}

static int wait_for_ready(void)
{
    return ( EINKFB_SCHEDULE_TIMEOUT(BROADSHEET_READY_TIMEOUT, bs_hardware_ready) );
}

#define BS_READY()      (broadsheet_ignore_hw_ready() || (EINKFB_SUCCESS == wait_for_ready()))
#define USE_WR_BUF(s)   (BS_CMD_ARGS_MAX < (s))
#define USE_RD_BUF(s)   (BS_RD_DAT_ONE != (s))
#define BS_CMD          true
#define BS_DAT          false
#define BS_WR           true
#define BS_RD           false



extern void ep3_reinit(void);

void epd_write_epson_reg(unsigned short command, unsigned short addr, unsigned short data)
{
	 
    unsigned short value = 0;	
    u32 rd_reg=0;				//Testing 0917
    udelay(EPD_CMD_DELAY);
    
    while(!value)
	{
	 value = GET_HRDY_STATUS;	
	  udelay(1);
	  rd_reg++;
	  if (rd_reg > 3000000)
		break;	  
	}

	if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
		//return;
		}

    ipu_adc_write_cmd(CMD,command);
  //epd_write_command(command); 
  
  udelay(1); 
	__raw_writew(addr,bsaddr);
  udelay(1); 
	__raw_writew(data,bsaddr);
  udelay(1); 
}


void epd_send_command(unsigned short command, unsigned short data)
{
    unsigned short value = 0;	
    u32 rd_reg=0;				//Testing 0917
    udelay(EPD_CMD_DELAY);
    
    while(!value)
	{
 	  value = GET_HRDY_STATUS;	
	  udelay(1);
	  rd_reg++;
	  if (rd_reg > 3000000)
		break;	  
	}
	if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
		//return;
		}
	
	LOW_HDC_PIN;
	
	__raw_writew(command,bsaddr);
	
    udelay(1);
	HIGH_HDC_PIN;
	 	
	__raw_writew(data,bsaddr);
	
}
void epd_write_command(unsigned short command)
{

    unsigned short value = 0;	
    u32 rd_reg=0;				//Testing 0917
    udelay(EPD_CMD_DELAY);
    
    while(!value)
		{
			//if(command == 0x29)
				//msleep(100);

		value = GET_HRDY_STATUS;	
		udelay(1);
		rd_reg++;
		if (rd_reg > 3000000)
			break;	  
		}
			
		if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
			//return;
		}
		LOW_HDC_PIN;
	
		__raw_writew(command,bsaddr);
    
		udelay(1);
		HIGH_HDC_PIN;

}	

void epd_write_data(unsigned short data)
{
	__raw_writew(data,bsaddr);
  udelay(EPD_CMD_DELAY);
}

//&*&*&*JJ1_1212ADD add EPD handwriting function.
void epd_send_command_du(unsigned short command, unsigned short data)
{
    u32 rd_reg=0;				//Testing 0917
  	udelay(1);
    unsigned short value = 0;	    
    
    while(!value)			
    	{
		value = GET_HRDY_STATUS;
	 	 udelay(1);
	 	 rd_reg++;
	 	if (rd_reg > 3000000)
			break;	  
	}
	if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
		//return;
		}
  	
		LOW_HDC_PIN;
		
		__raw_writew(command,bsaddr);
		
  	udelay(1);
		HIGH_HDC_PIN;
		 	
		__raw_writew(data,bsaddr);
	
}
void epd_write_command_du(unsigned short command)
{
	
    udelay(1);
    unsigned short value = 0;	    
    u32 rd_reg=0;				//Testing 0917
    
    while(!value)			
    	{
		value = GET_HRDY_STATUS;
	  udelay(1);
	  rd_reg++;
	  if (rd_reg > 3000000)
		break;	  
	}
	if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
		//return;
		}
			
		LOW_HDC_PIN;
	
		__raw_writew(command,bsaddr);
    
		udelay(1);
		HIGH_HDC_PIN;

}	

void epd_write_data_du(unsigned short data)
{
		__raw_writew(data,bsaddr);	
  	udelay(1);
}

//&*&*&*JJ2_1212ADD add EPD handwriting function.
unsigned short epd_read_epson_reg(unsigned short command, unsigned short addr)
{
	
	volatile unsigned short reg;
	unsigned short value = 0;	
	u32 rd_reg=0;				//Testing 0917
		
	udelay(EPD_CMD_DELAY);
	
	//while(!GET_HRDY_STATUS)	
	while(!value)
	{
		value = GET_HRDY_STATUS;	
	  udelay(1);
	  rd_reg++;
	  if (rd_reg > 3000000)
		break;	  
	}
	if (rd_reg > 3000000)		// reinit EPD controller	//Testing 0917
		{
		ep3_reinit();
		//return;
		}

	epd_write_command(command);
  udelay(EPD_CMD_DELAY);
	__raw_writew(addr,bsaddr);
  udelay(EPD_CMD_DELAY);
	reg = __raw_readw(bsaddr);
  udelay(EPD_CMD_DELAY);
	
	return reg;
}

void epd_write_buffer(u32 data_size, u16 *data)
{	
	int i;
    for ( i = 0; i < data_size; i++)
    {
        ipu_adc_write_cmd(DAT, (uint32_t)data[i]);
    }
  
}


static int bs_wr_which(bool which, uint32_t data)
{
//    display_port_t disp = BROADSHEET_DISPLAY_NUMBER;
    cmddata_t type = (BS_CMD == which) ? CMD : DAT;
    int result = EINKFB_FAILURE;
    
    if ( BS_READY() )
//        result = ipu_adc_write_cmd(disp, type, data, 0, 0);
        result = ipu_adc_write_cmd(type, data);
    else
        result = EINKFB_FAILURE;
    
    return ( result );
}

static int ipu_adc_write_cmd(cmddata_t type,uint32_t data)
{
	int result=EINKFB_SUCCESS;
	if(type==CMD)
	{
		udelay(1);
		LOW_HDC_PIN;
	}

	WR_GPIO_LINE(bsaddr,data)

	udelay(1);

		HIGH_HDC_PIN;

	return result;
}


int bs_wr_cmd(bs_cmd cmd, bool poll)
{
    int result;
    
    einkfb_debug_full("command = 0x%04X, poll = %d\n", cmd, poll);
    result = bs_wr_which(BS_CMD, (uint32_t)cmd);
    
    if ( (EINKFB_SUCCESS == result) && poll )
    {
        if ( !BS_READY() )
           result = EINKFB_FAILURE; 
    }

    return ( result );
}

// Scheduled loop for doing IO on framebuffer-sized buffers.
//
static int bs_io_buf(u32 data_size, u16 *data, bool which)
{
//    display_port_t disp = BROADSHEET_DISPLAY_NUMBER;
    int result = EINKFB_FAILURE;

    einkfb_debug_full("size    = %d\n", data_size);

    if ( BS_READY() )
    {
        int     i = 0, j, length = (EINKFB_MEMCPY_MIN >> 1), num_loops = data_size/length,
                remainder = data_size % length;
        bool    done = false;
        
        if ( 0 != num_loops )
            einkfb_debug("num_loops @ %d bytes = %d, remainder = %d\n",
                (length << 1), num_loops, (remainder << 1));
        
        result = EINKFB_SUCCESS;
        
        // Read/write EINKFB_MEMCPY_MIN bytes (hence, divide by 2) at a time.  While
        // there are still bytes to read/write, yield the CPU.
        //
        do
        {
            if ( 0 >= num_loops )
                length = remainder;

            for ( j = 0; j < length; j++)
            {
                if ( BS_WR == which )
//                    ipu_adc_write_cmd(disp, DAT, (uint32_t)data[i + j], 0, 0);
                    ipu_adc_write_cmd(DAT, (uint32_t)data[i + j]);
                else
//                    data[i + j] = (u16)(ipu_adc_read_data(disp) & 0x0000FFFF);
                    data[i + j] = (u16)(ipu_adc_read_data() & 0x0000FFFF);
            }
                
            i += length;
            
            if ( i < data_size )
            {
                EINKFB_SCHEDULE();
                num_loops--;
            }
            else
                done = true;
        }
        while ( !done );
    }

    return ( result );
}

static int ipu_adc_read_data(void)
{

	return RD_GPIO_LINE(bsaddr);
}

bool bs_wr_one_ready(void)
{
    bool ready = false;
    
    if ( BS_READY() )
        ready = true;
    
    return ( ready );
}

void bs_wr_one(u16 data)
{
//    ipu_adc_write_cmd((display_port_t)BROADSHEET_DISPLAY_NUMBER, DAT, (uint32_t)data, 0, 0);
ipu_adc_write_cmd(DAT, (uint32_t)data);
}

/*Henry Li: write data to 13522 directly */
void ep3_bs_wr_one(u16 data)
{
	__raw_writew(data ,bsaddr);
}
int bs_wr_dat(bool which, u32 data_size, u16 *data)
{
    int result;
    
    if ( USE_WR_BUF(data_size) )
        result = bs_io_buf(data_size, data, BS_WR);
    else
    {
        int i; result = EINKFB_SUCCESS;
        
        for ( i = 0; (i < data_size) && (EINKFB_SUCCESS == result); i++ )
        {
            if ( BS_WR_DAT_DATA == which )
            {
                if ( EINKFB_DEBUG() && (9 > i) )
                    einkfb_debug_full("data[%d] = 0x%04X\n", i, data[i]);
            }
            else
                einkfb_debug_full("args[%d] = 0x%04X\n", i, data[i]);
            
            result = bs_wr_which(BS_DAT, (uint32_t)data[i]);
        }
    }
    
    return ( result );
}

static int bs_rd_one(u16 *data)
{
//    display_port_t disp = BROADSHEET_DISPLAY_NUMBER;
    int result = EINKFB_SUCCESS;
    
    if ( BS_READY() )
//        *data = (u16)(ipu_adc_read_data(disp) & 0x0000FFFF);
        *data = (u16)(ipu_adc_read_data() & 0x0000FFFF);
    else
        result = EINKFB_FAILURE;
    
    if ( EINKFB_SUCCESS == result )
        einkfb_debug_full("data    = 0x%04X\n", *data);
    
    return ( result );
}

int bs_rd_dat(u32 data_size, u16 *data)
{
    int result = EINKFB_FAILURE;

    // For single-word reads, don't use the buffer call.
    //
    if ( !USE_RD_BUF(data_size) )
         result = bs_rd_one(data);
    else
    {
        einkfb_debug_full("size    = %d\n", data_size);

        if ( BS_READY() )
            result = bs_io_buf(data_size, data, BS_RD);
    }
    
    return ( result );
}

/*
#ifdef USE_BS_IRQ

static irqreturn_t bs_irq_handler (int irq, void *data, struct pt_regs *r)
{
    u16 irq_status;

    // Check Broadsheet general IRQs

    irq_status = bs_cmd_rd_reg(BS_INTR_RAW_STATUS_REG);
    einkfb_debug("BS_INTR_RAW_STATUS_REG = 0x%04X\n", irq_status);

    irq_status = bs_cmd_rd_reg(BS_DE_INTR_RAW_STATUS_REG);
    einkfb_debug("BS_DE_INTR_RAW_STATUS_REG = 0x%04X\n", irq_status);

    // Clear all interrupt flags
    bs_cmd_wr_reg(BS_INTR_RAW_STATUS_REG, BS_ALL_IRQS);
    bs_cmd_wr_reg(BS_DE_INTR_RAW_STATUS_REG, BS_DE_ALL_IRQS);

   return IRQ_HANDLED;
}
#endif
*/

/*
** Initialize the Broadsheet hardware interface and reset the
** Broadsheet controller.
*/

bool bs_hw_init(void)
{

	bool result = true;
	bsaddr = ioremap(0x10000000, 4);

	//printk(KERN_ALERT "hello broadsheet!\n");

//reset broadsheet

	// GPC7  EINK_3.3V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 28);
		__raw_writel(gpcon | (0x1 << 28), S3C64XX_GPCCON);		
	//GPC7 EINK_3.3V_EN set low
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata & ~(1<<7));
		__raw_writel(gpdata,S3C64XX_GPCDAT);

	//GPC6 EINK_1.8V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 24);
		__raw_writel(gpcon | (0x1 << 24), S3C64XX_GPCCON);
	
	//GPC6 EINK_1.8V_EN set low
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata & ~(1<<6));
		__raw_writel(gpdata,S3C64XX_GPCDAT);
	
	//GPP2 OSC_EN config output
		gpcon = __raw_readl(S3C64XX_GPPCON);
		gpcon  = (gpcon & ~(3<<4));
		 __raw_writel(gpcon|(0x1<<4), S3C64XX_GPPCON);
	//GPP2 OSC_EN set high
		gpdata = __raw_readl(S3C64XX_GPPDAT);
		gpdata =(gpdata & ~(1<<2));
		__raw_writel(gpdata|(0x1<<2),S3C64XX_GPPDAT);
		mdelay(10);

	//GPP9 HnRST config output
	gpcon = __raw_readl(S3C64XX_GPPCON);
	gpcon  = (gpcon & ~(3<<18));
	__raw_writel(gpcon|(0x1<<18), S3C64XX_GPPCON);
	
      HIGH_RESET_PIN;
	mdelay(10);
      LOW_RESET_PIN;
      mdelay(100); 
      HIGH_RESET_PIN;


//setup HD/C signalconfig
	gpcon = __raw_readl(S3C64XX_GPOCON);
	gpcon =(gpcon & ~(3<<4));
	__raw_writel(gpcon|(0x1<<4),S3C64XX_GPOCON);

	gpdata = __raw_readl(S3C64XX_GPODAT);
        gpdata =(gpdata & ~(1<<2));
        __raw_writel(gpdata|(0x1<<2),S3C64XX_GPODAT);

	//GPO3 HIRQ config input
		gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon  = (gpcon & ~(3<<6));
		__raw_writel(gpcon, S3C64XX_GPOCON);

//now HRDY  setup to input
	gpcon = __raw_readl(S3C64XX_GPOCON);
        gpcon  = (gpcon & ~(3<<8));
        __raw_writel(gpcon, S3C64XX_GPOCON);


	sromdata = __raw_readl(S3C64XX_VA_SROMC);
        sromdata &=~(0xF<<0);
        sromdata |= (1<<3) | (1<<2) | (1<<0);
        __raw_writel(sromdata, S3C64XX_VA_SROMC);
		
	//SROM_BC0 config
		//__raw_writel((0x1<<28)|(0xf<<24)|(0x1c<<16)|(0x1<<12)|(0x6<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);
    __raw_writel((0x0<<28)|(0x0<<24)|(0xA<<16)|(0x1<<12)|(0x0<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);

	//printk(KERN_ALERT "Product id is %x\n",__raw_readw(bsaddr));
	//printk(KERN_ALERT "The new bs driver\n");
#ifdef MXC31
	for(i=0;i<100;i++)
	{
	mdelay(500);
	__raw_writew(0xffff,bsaddr);
	//printk(KERN_ALERT "write the %dth times data\n",i);
	}
#endif

#ifdef USE_BS_IRQ
    int  rqstatus;
#endif
#ifdef MXC31
    ipu_adc_sig_cfg_t sig = { 0, 0, 0, 0, 0, 0, 0, 0,
            IPU_ADC_BURST_WCS,
            IPU_ADC_IFC_MODE_SYS80_TYPE2,
            16, 0, 0, IPU_ADC_SER_NO_RW
    };

    // Init DI interface
    if ( IS_NELL() || IS_MARIO() || IS_ADS() )
    {
        broadsheet_screen_height = BROADSHEET_SCREEN_HEIGHT_NELL;
        broadsheet_screen_width  = BROADSHEET_SCREEN_WIDTH_NELL;

        broadsheet_screen_size   = BROADSHEET_SCREEN_SIZE_NELL;
    }

    ipu_adc_init_panel(BROADSHEET_DISPLAY_NUMBER,
                       broadsheet_screen_width,
                       broadsheet_screen_height,
                       BROADSHEET_PIXEL_FORMAT, broadsheet_screen_size, sig, XY, 0, VsyncInternal);

    // Set IPU timing for read cycles
    ipu_adc_init_ifc_timing(BROADSHEET_DISPLAY_NUMBER, true,
                            BROADSHEET_READ_CYCLE_TIME,
                            BROADSHEET_READ_UP_TIME,
                            BROADSHEET_READ_DOWN_TIME,
                            BROADSHEET_READ_LATCH_TIME,
                            BROADSHEET_PIXEL_CLK);
    // Set IPU timing for write cycles
    ipu_adc_init_ifc_timing(BROADSHEET_DISPLAY_NUMBER, false,
                            BROADSHEET_WRITE_CYCLE_TIME,
                            BROADSHEET_WRITE_UP_TIME,
                            BROADSHEET_WRITE_DOWN_TIME,
                            0, 0);

    ipu_adc_set_update_mode(ADC_SYS1, IPU_ADC_REFRESH_NONE, 0, 0, 0);

    gpio_lcd_active();
    slcd_gpio_config();

#ifdef CONFIG_MACH_MX31ADS
    // Reset the level translator for the two GPIO inputs (HIRQ and HRDY)
    pin_addr = PBC_BCTRL2_LDCIO_EN;
    __raw_writew(pin_addr, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);
#endif

#ifdef USE_BS_IRQ
    // Set up IRQ for for Broadsheet HIRQ line
    disable_irq(BROADSHEET_HIRQ_IRQ);
    set_irq_type(BROADSHEET_HIRQ_IRQ, IRQF_TRIGGER_RISING);
    rqstatus = request_irq(BROADSHEET_HIRQ_IRQ, (irq_handler_t) bs_irq_handler, 0, "eink_fb_hal_broads", NULL);
    if (rqstatus != 0) {
        einkfb_print_crit("Failed IRQ request for Broadsheet HIRQ line; request status = %d\n", rqstatus);
        result = false;
    }
#endif

     // Set up GPIO pins
    if (mxc_request_gpio(BROADSHEET_HIRQ_LINE)) {
        einkfb_print_crit("Could not obtain GPIO pin for HIRQ\n");
        result = false;
    }
    else {
        // Set HIRQ pin as input
        mxc_set_gpio_direction(BROADSHEET_HIRQ_LINE, 1);
    }
    if (mxc_request_gpio(BROADSHEET_HRDY_LINE)) {
        einkfb_print_crit("Could not obtain GPIO pin for HRDY\n");
        result = false;
    }
    else {
        // Set HRDY pin as input
        mxc_set_gpio_direction(BROADSHEET_HRDY_LINE, 1);
    }
#ifdef CONFIG_MACH_MARIO_MX
    if (mxc_request_gpio(BROADSHEET_RST_LINE)) {
        einkfb_print_crit("Could not obtain GPIO pin for RST\n");
        result = false;
    }
    else {
        // Set RST pin as output and initialize to zero (it's active LOW)
        mxc_set_gpio_direction(BROADSHEET_RST_LINE, 0);
        mxc_set_gpio_dataout(BROADSHEET_RST_LINE, 0);
    }
#endif

#ifdef CONFIG_MACH_MX31ADS
    // Enable the level translator for the two GPIO inputs (HIRQ and HRDY)
    mdelay(100);    // Pause 100 ms to allow level translator to settle
    pin_addr = PBC_BCTRL2_LDCIO_EN;
    __raw_writew(pin_addr, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);
#endif
    // Reset Broadsheet
    einkfb_debug("Sending RST signal to Broadsheet...\n");
    LOW_RESET_PIN;	//WR_GPIO_LINE(BROADSHEET_RST_LINE, BROADSHEET_RESET_VAL);     // Assert RST
    mdelay(100);    // Pause 100 ms during reset
    HIGH_RESET_PIN;
 //WR_GPIO_LINE(BROADSHEET_RST_LINE, BROADSHEET_NON_RESET_VAL); // Clear RST
    mdelay(400);    // Pause 400 ms to allow Broasheet time to come up
    einkfb_debug("Broadsheet reset done.\n");
//#ifdef TEST_BROADSHEET
//    test_broadsheet(disp);
//#endif
#endif
#ifdef USE_BS_IRQ
    // Set up Broadsheet for interrupt generation (enable all conditions)
    bs_cmd_wr_reg(BS_INTR_CTL_REG, BS_ALL_IRQS);
    bs_cmd_wr_reg(BS_INTR_RAW_STATUS_REG, BS_ALL_IRQS);

    // Enable all Broadsheet display engine interrupts
    bs_cmd_wr_reg(BS_DE_INTR_ENABLE_REG, BS_DE_ALL_IRQS);
    bs_cmd_wr_reg(BS_DE_INTR_RAW_STATUS_REG, BS_DE_ALL_IRQS);
#endif
    //ipu_adc_write_cmd(CMD,0x11);
    //ipu_adc_write_cmd(DAT,0x030a);
    //ipu_adc_write_cmd(DAT,0x0123);

/*
    	ipu_adc_write_cmd(CMD,0x10);
        __raw_writew(0x02,bsaddr);

        udelay(1);

        printk(KERN_ALERT "Product id re-get is %x\n",__raw_readw(bsaddr));
        //printk(KERN_ALERT "The new bs driver\n");
*/

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    u16 rd_reg;
    unsigned short value = 0;	
    rd_reg = bs_cmd_rd_reg(0x00a) & ~(1 << 12);
    bs_cmd_wr_reg(0x00a, (value | (1 << 12)));		//REG[000Ah] bit 12 = 1b

 //Henry Li 0927 fro saving time in resume 	
    mdelay(4);
    
    bs_cmd_init_sys_run();
    udelay(EPD_CMD_DELAY);
    rd_reg=0;
    while(!value)
	{
		value = GET_HRDY_STATUS;
   	       udelay(50);
		rd_reg++;
		if (rd_reg > 60000)
			break;
	}
    //rd_reg = bs_cmd_rd_reg(0x0204) | (1 << 7);  // spi flash control reg: Display Engine access mode is selected.
       rd_reg = 0x99;
    bs_cmd_wr_reg(0x0204, rd_reg);
/* //Henry Li 0927 fro saving time in resume 	
    udelay(EPD_CMD_DELAY);
    printk("reg[0x0204] is 0x%x\n", rd_reg);
*/    
#endif	
/* //Henry Li 0927 fro saving time in resume 	
    ipu_adc_write_cmd(CMD,0x10);
    ipu_adc_write_cmd(DAT,0x02);
    printk(KERN_ALERT "register 0x0002 content is 0x%4x\n",ipu_adc_read_data());
*/
     einkfb_debug("GPIOs and IRQ set; Broadsheet has been reset\n");
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT) 
/* //Henry Li 0927 fro saving time in resume 	
    mdelay(1);
*/    
    bs_cmd_wr_reg(0x300, BS97_INIT_VSIZE);	//Frame Data Length Register
    											//Henry: as instruction code init it as 825
/* //Henry Li 0927 fro saving time in resume 	
    mdelay(1);
*/    
#endif
    return ( result );
}

bool bs_quickly_poweroff(void)
{

	bool result = true;

	// GPC7  EINK_3.3V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 28);
		__raw_writel(gpcon | (0x1 << 28), S3C64XX_GPCCON);		
	//GPC7 EINK_3.3V_EN set low  ==> high
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata | (1<<7));
		__raw_writel(gpdata,S3C64XX_GPCDAT);

/*
	//GPC6 EINK_1.8V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 24);
		__raw_writel(gpcon | (0x1 << 24), S3C64XX_GPCCON);
	
	//GPC6 EINK_1.8V_EN set low  ==> high
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata | (1<<6));	
		__raw_writel(gpdata,S3C64XX_GPCDAT);
*/
	
	//GPP2 OSC_EN config output
		gpcon = __raw_readl(S3C64XX_GPPCON);
		gpcon  = (gpcon & ~(3<<4));
		 __raw_writel(gpcon|(0x1<<4), S3C64XX_GPPCON);
	//GPP2 OSC_EN set high  ==> low
		gpdata = __raw_readl(S3C64XX_GPPDAT);
		gpdata =(gpdata & ~(1<<2));
		__raw_writel(gpdata,S3C64XX_GPPDAT);
		//mdelay(10);		//Work well 10ms

}
EXPORT_SYMBOL_GPL(bs_quickly_poweroff);
bool bs_quickly_hw_init(void)
{

	bool result = true;
	if (bsaddr == NULL)
		{
		bsaddr = ioremap(0x10000000, 4);
		}
	if ((unsigned int)bsaddr  < 0xa0000000)
		{
		iounmap(bsaddr);
		bsaddr = ioremap(0x10000000, 4);
		}
//reset broadsheet

	// GPC7  EINK_3.3V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 28);
		__raw_writel(gpcon | (0x1 << 28), S3C64XX_GPCCON);		
	//GPC7 EINK_3.3V_EN set low
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata & ~(1<<7));
		__raw_writel(gpdata,S3C64XX_GPCDAT);

	//GPC6 EINK_1.8V_EN config output
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon = gpcon & ~(0xF << 24);
		__raw_writel(gpcon | (0x1 << 24), S3C64XX_GPCCON);
	
	//GPC6 EINK_1.8V_EN set low
		gpdata = __raw_readl(S3C64XX_GPCDAT);
		gpdata =(gpdata & ~(1<<6));
		__raw_writel(gpdata,S3C64XX_GPCDAT);
	
	//GPP2 OSC_EN config output
		gpcon = __raw_readl(S3C64XX_GPPCON);
		gpcon  = (gpcon & ~(3<<4));
		 __raw_writel(gpcon|(0x1<<4), S3C64XX_GPPCON);
	//GPP2 OSC_EN set high
		gpdata = __raw_readl(S3C64XX_GPPDAT);
		gpdata =(gpdata & ~(1<<2));
		__raw_writel(gpdata|(0x1<<2),S3C64XX_GPPDAT);
		mdelay(10);

	//GPP9 HnRST config output
      HIGH_RESET_PIN;	
	gpcon = __raw_readl(S3C64XX_GPPCON);
	gpcon  = (gpcon & ~(3<<18));
	__raw_writel(gpcon|(0x1<<18), S3C64XX_GPPCON);

      HIGH_RESET_PIN;
	udelay(100);
      LOW_RESET_PIN;
	udelay(100);
      HIGH_RESET_PIN;

//setup HD/C signalconfig
	gpcon = __raw_readl(S3C64XX_GPOCON);
	gpcon =(gpcon & ~(3<<4));
	__raw_writel(gpcon|(0x1<<4),S3C64XX_GPOCON);

	gpdata = __raw_readl(S3C64XX_GPODAT);
        gpdata =(gpdata & ~(1<<2));
        __raw_writel(gpdata|(0x1<<2),S3C64XX_GPODAT);

	//GPO3 HIRQ config input
		gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon  = (gpcon & ~(3<<6));
		__raw_writel(gpcon, S3C64XX_GPOCON);

//now HRDY  setup to input
	gpcon = __raw_readl(S3C64XX_GPOCON);
        gpcon  = (gpcon & ~(3<<8));
        __raw_writel(gpcon, S3C64XX_GPOCON);


	sromdata = __raw_readl(S3C64XX_VA_SROMC);
        sromdata &=~(0xF<<0);
        sromdata |= (1<<3) | (1<<2) | (1<<0);
        __raw_writel(sromdata, S3C64XX_VA_SROMC);
		
	//SROM_BC0 config
		//__raw_writel((0x1<<28)|(0xf<<24)|(0x1c<<16)|(0x1<<12)|(0x6<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);
    __raw_writel((0x0<<28)|(0x0<<24)|(0xA<<16)|(0x1<<12)|(0x0<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);

	//printk(KERN_ALERT "Product id is %x\n",__raw_readw(bsaddr));
	//printk(KERN_ALERT "The new bs driver\n");

    //ipu_adc_write_cmd(CMD,0x11);
    //ipu_adc_write_cmd(DAT,0x030a);
    //ipu_adc_write_cmd(DAT,0x0123);

/*
    	ipu_adc_write_cmd(CMD,0x10);
        __raw_writew(0x02,bsaddr);

        udelay(1);

        printk(KERN_ALERT "Product id re-get is %x\n",__raw_readw(bsaddr));
        //printk(KERN_ALERT "The new bs driver\n");

    ipu_adc_write_cmd(CMD,0x10);
    ipu_adc_write_cmd(DAT,0x02);
    printk(KERN_ALERT "register 0x0002 content is 0x%4x\n",ipu_adc_read_data());
*/

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)  || defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)			
    u16 rd_reg;
    unsigned short value = 0;	
    rd_reg = bs_cmd_rd_reg(0x00a) & ~(1 << 12);
    bs_cmd_wr_reg(0x00a, (value | (1 << 12)));		//REG[000Ah] bit 12 = 1b

 //Henry Li 0927 fro saving time in resume 	
    mdelay(4);
    
    bs_cmd_init_sys_run();
    udelay(EPD_CMD_DELAY);
    rd_reg=0;
    while(!value)
	{
		value = GET_HRDY_STATUS;
   	       udelay(50);
		rd_reg++;
		if (rd_reg > 60000)
			break;
	}
    //rd_reg = bs_cmd_rd_reg(0x0204) | (1 << 7);  // spi flash control reg: Display Engine access mode is selected.
       rd_reg = 0x99;
    bs_cmd_wr_reg(0x0204, rd_reg);
 //Henry Li 0927 fro saving time in resume 	
/* 
    udelay(EPD_CMD_DELAY);
    printk("reg[0x0204] is 0x%x\n", rd_reg);
*/    
#endif	
 //Henry Li 0927 fro saving time in resume 	
/* 
    ipu_adc_write_cmd(CMD,0x10);
    ipu_adc_write_cmd(DAT,0x02);
    printk(KERN_ALERT "register 0x0002 content is 0x%4x\n",ipu_adc_read_data());
*/
     einkfb_debug("GPIOs and IRQ set; Broadsheet has been reset\n");
#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT) 
/* //Henry Li 0927 fro saving time in resume 	
    mdelay(1);
*/    
    bs_cmd_wr_reg(0x300, BS97_INIT_VSIZE);	//Frame Data Length Register
    											//Henry: as instruction code init it as 825
 //Henry Li 0927 fro saving time in resume 	
    mdelay(1);
    
#endif
    return ( result );
}


static bool bs_cmd_ck_reg(u16 ra, u16 rd)
{
    u16 rd_reg = bs_cmd_rd_reg(ra);
    bool result = true;

    if ( rd_reg != rd )
    {
        einkfb_print_crit("REG[0x%04X] is 0x%04X but should be 0x%04X\n",
            ra, rd_reg, rd);
            
        result = false;
    }
    
    return ( result );
}

bool bs_hw_test(void)
{
    bool result = true;

#if defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)
    bs_cmd_wr_reg(0x0304, 0x0404); // frame begin/end length reg
    bs_cmd_wr_reg(0x030A, 0x5004); // line  begin/end length reg
    result &= bs_cmd_ck_reg(0x0304, 0x0404);
    result &= bs_cmd_ck_reg(0x030A, 0x5004 );
    
    bs_cmd_wr_reg(0x0304, 0x0404);
    bs_cmd_wr_reg(0x030A, 0x5004);
    result &= bs_cmd_ck_reg(0x0304, 0x0404);
    result &= bs_cmd_ck_reg(0x030A, 0x5004);
#elif defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)
    bs_cmd_wr_reg(0x0304, 0x0a04); // frame begin/end length reg
/*    
    bs_cmd_wr_reg(0x030A, 0x3004); // line  begin/end length reg
*/    
    result &= bs_cmd_ck_reg(0x0304, 0x0a04);
/*    
    result &= bs_cmd_ck_reg(0x030A, 0x3004 );
*/    
    
    bs_cmd_wr_reg(0x0304, 0x0a04);
/*    
    bs_cmd_wr_reg(0x030A, 0x3004);
*/    
    result &= bs_cmd_ck_reg(0x0304, 0x0a04);
/*    
    result &= bs_cmd_ck_reg(0x030A, 0x3004);
*/    
#else
    bs_cmd_wr_reg(0x0304, 0x0123); // frame begin/end length reg
    bs_cmd_wr_reg(0x030A, 0x4567); // line  begin/end length reg
    result &= bs_cmd_ck_reg(0x0304, 0x0123);
    result &= bs_cmd_ck_reg(0x030A, 0x4567 );
    
    bs_cmd_wr_reg(0x0304, 0xFEDC);
    bs_cmd_wr_reg(0x030A, 0xBA98);
    result &= bs_cmd_ck_reg(0x0304, 0xFEDC);
    result &= bs_cmd_ck_reg(0x030A, 0xBA98);
#endif
    return ( result );
}

void bs_hw_done()//bool dealloc
{
    
    if ( !(broadsheet_ignore_hw_ready() || broadsheet_force_hw_not_ready()) )
    {
        // Wait until any pending operations are done before shutting down.
        //
        einkfb_debug("Waiting for HRDY before shutting down...\n");
        wait_for_ready();
    }
    
#ifdef MXC31
#ifdef CONFIG_MACH_MX31ADS
    // Reset the level translator for the two GPIO inputs (HIRQ and HRDY)
    pin_addr = PBC_BCTRL2_LDCIO_EN;
    __raw_writew(pin_addr, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);
    mdelay(100);    // Pause 100 ms to allow level translator to settle
#elif  CONFIG_MACH_MARIO_MX
    mxc_free_gpio(BROADSHEET_RST_LINE);
#endif

#ifdef USE_BS_IRQ
    disable_irq(BROADSHEET_HIRQ_IRQ);
    free_irq(BROADSHEET_HIRQ_IRQ, NULL);
#endif

    mxc_free_gpio(BROADSHEET_HIRQ_LINE);
    mxc_free_gpio(BROADSHEET_HRDY_LINE);
#endif
    einkfb_debug("Released Broadsheet GPIO pins and IRQs\n");
}
