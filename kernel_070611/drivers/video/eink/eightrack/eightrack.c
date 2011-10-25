/*
** eightrack controller driver routine for Fiona.  (C) 2007 Lab126
*/

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
#include <linux/timer.h>
#include <linux/module.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/segment.h>
#include <asm/hardware.h>
#include <asm/io.h>

#include <asm/arch/fiona.h>
#include <asm/arch/fpow.h>
#include <asm/delay.h>
#include <asm/arch/pxa-regs.h>


// Event keycodes and handler routine are here:
#include <asm/arch/fiona_eventmap.h>

#include <asm/arch/boot_globals.h>

#include <linux/init.h>

#include "eightrack.h"

static void eightrack_power_up_timer_timeout(unsigned long timer_data);
static void eightrack_display_timer_timeout(unsigned long display_timer_data);
static irqreturn_t eightrack_irq_ERR(int irq, void *data, struct pt_regs *r);
static irqreturn_t eightrack_irq_RDY(int irq, void *data, struct pt_regs *r);

//typedef unsigned int u32;
typedef u32 * volatile reg_addr_t;

// Global variables

static char *gpio_base_map = NULL;
static char *lcd_base_map = NULL;

// Size of command field for 8Track in frame buffer
static const int CMD_SIZE = 33;

static eightrack_state_t eightrack_state = ERROR;

//Global variables
static int _frame_width; // frame width = # 2-byte pixels
static int _frame_height; // frame height = # lines
static int _frame_size; // frame size in words
static ushort * _fb_mp;   // fb memory map base address
static int _image_start_line; // Line number for image display data
static int _image_height;  // Height of display image, in lines
static int _frame_count;    // Frame count for current waveform
 
// The following includes initialize the waveform arrays
// NOTE: for supporting different panels we need to make this dynamic
#include "waveforms/23P01001_60_TB0304_GC_25C.h"
#include "waveforms/23P01001_60_TB0304_GU_25C.h"
#include "waveforms/23P01001_60_TB0304_INIT_25C.h"
#include "waveforms/23P01001_60_TB0304_MU_25C.h"


static reg_addr_t reg_EIGHT_TRACK_ERR_DR = NULL;
static reg_addr_t reg_EIGHT_TRACK_ERR_FE = NULL;
static reg_addr_t reg_EIGHT_TRACK_ERR_RE = NULL;
static reg_addr_t reg_EIGHT_TRACK_ERR_ED = NULL;
static reg_addr_t reg_EIGHT_TRACK_ERR_AF = NULL;
static reg_addr_t reg_EIGHT_TRACK_ERR_LR = NULL;

static reg_addr_t reg_EIGHT_TRACK_RST_DR = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_FE = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_ED = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_AF = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_LR = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_SR = NULL;
static reg_addr_t reg_EIGHT_TRACK_RST_CR = NULL;

static reg_addr_t reg_EIGHT_TRACK_STBY_DR = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_FE = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_ED = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_AF = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_LR = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_SR = NULL;
static reg_addr_t reg_EIGHT_TRACK_STBY_CR = NULL;

static reg_addr_t reg_EIGHT_TRACK_RDY_DR = NULL;
static reg_addr_t reg_EIGHT_TRACK_RDY_FE = NULL;
static reg_addr_t reg_EIGHT_TRACK_RDY_RE = NULL;
static reg_addr_t reg_EIGHT_TRACK_RDY_ED = NULL;
static reg_addr_t reg_EIGHT_TRACK_RDY_AF = NULL;
static reg_addr_t reg_EIGHT_TRACK_RDY_LR = NULL;

static reg_addr_t reg_PPL_ADDR = NULL;
static reg_addr_t reg_LPP_ADDR = NULL;

// Timer used to space out the assertion of RDY_L and STBY_L on power up
static struct timer_list eightrack_power_up_timer;

// Timer used to recover missing RDY transitions during DISPLAY command
static struct timer_list eightrack_display_timer;

#define POWER_UP_TIMEOUT_DEFAULT 100 // Time is in milliseconds
static int POWER_UP_timeout = POWER_UP_TIMEOUT_DEFAULT;


// Module parameters
#define DEBUG_EIGHT_TRACK_DEFAULT 0
static int debug_eightrack = DEBUG_EIGHT_TRACK_DEFAULT;
module_param(debug_eightrack, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(debug, "Enable debugging; 0=off, any other value=on (default is off)");


u32 read_reg(reg_addr_t reg_addr, int len, int pos)
{
   u32 value_read;

   value_read = readl(reg_addr);
   if (debug_eightrack > 1) {
      printk("<1>[read_reg] Full value read = %x\n", value_read);
   }
   value_read = value_read >> pos;
   value_read &= ~(0xffffffff << len);
   if (debug_eightrack > 1) {
      printk("<1>[read_reg] len = %d; pos = %d\n", len, pos);
      printk("<1>[read_reg] Masked value read = %x\n", value_read);
   }
   return (value_read);
}


void write_reg(reg_addr_t reg_addr, int len, int pos, u32 val, u32 mask)
{
   u32 reg_value;
   u32 masked_val;

   reg_value = readl(reg_addr);  // Read register
   masked_val = val &  ~(0xffffffff << len);  // If value sent is too big, trim it
   if (debug_eightrack > 1) {
      printk("<1>[write_reg] reg_value = %x\n", reg_value);
   }
   reg_value |= (masked_val << pos); // Put value in its proper position
   reg_value &= mask;  // Mask out reserved bits that need to be written as 0 
   if (debug_eightrack > 1) {
      printk("<1>[write_reg] reg_value = %x\n", reg_value);
   }
   writel(reg_value, reg_addr);  // Write the new value into the register
   if (debug_eightrack > 1) {
      printk("<1>[write_reg] reg_addr = %x; re-read val = %x\n", (unsigned int)reg_addr, read_reg(reg_addr, len, pos));
   }
}

//
// Initialize the global variables with the frame buffer parameters.
// These parameters are in the "info" structure being passed.  The
// structure is defined in include/linux/fb.h
//

int init_fb_vars( struct fb_info *info )
{
  _frame_width  = info->var.xres;
  _frame_height = info->var.yres;
  _frame_size =  _frame_width * _frame_height * 2;  // 2 pixels per word
  _image_start_line = 1 + (( MAX_WFD_SIZE+1 ) / _frame_width );
  if ( ( ( MAX_WFD_SIZE+1 ) % _frame_width ) != 0 ) _image_start_line += 1;
  _image_height = _frame_height - _image_start_line;
  if ( _image_height < 0 ) {
    printk("<1>[init_fb] ***  ERROR: _frame_height (%d) < isl (%d)\n", _frame_height, _image_start_line);
    return ( 1 );
  }
  _fb_mp = (ushort *)info->screen_base; // Base address of frame buffer


   if (debug_eightrack != 0) {
      printk("<1>[init_fb] Frame buffer information:\n");
      printk("<1>          width = %d, height = %d\n", _frame_width, _frame_height);
      printk("<1>          total size = %d * 2 bytes\n", _frame_size);
      printk("<1>          image height = %d\n", _image_height);
      printk("<1>          base address = 0x%08X\n", (int)_fb_mp);
   }

   // Return the offset -in bytes- from the beginning of the
   // frame buffer to the starting point of the image data.

   return ( _image_start_line*_frame_width*2 );
}

void start_power_up_timer(void)
{
   if (debug_eightrack != 0) {
      printk("<1>[start_power_up_timer] Starting the timer...\n");
   }
   // Convert from ms to jiffies; in Fiona HZ=100, so each jiffy is 10ms
   eightrack_power_up_timer.expires = jiffies + ((POWER_UP_timeout * HZ) / 1000);
   add_timer(&eightrack_power_up_timer);
   eightrack_power_up_timer.data = 1; // Indicate that timer is running
}


void start_display_timer(int timeout_ms)
{
   if (debug_eightrack != 0) {
      printk("<1>[start_display_timer] Starting the timer for %d ms...\n", timeout_ms);
   }
   // Convert from ms to jiffies; in Fiona HZ=100, so each jiffy is 10ms
   eightrack_display_timer.expires = jiffies + ((timeout_ms * HZ) / 1000);
   add_timer(&eightrack_display_timer);
   eightrack_display_timer.data = 1; // Indicate that timer is running
}


void stop_display_timer(void)
{
   if (debug_eightrack != 0) {
      printk("<1>[stop_display_timer] Stopping the timer...\n");
   }
   del_timer(&eightrack_display_timer); // Stop the timer
   eightrack_display_timer.data = 0;    // Indicate that timer is not running
}


//
// Initialization routine.  This MUST be called first!
//

int eightrack_init( struct fb_info *info )
{
   int rqstatus;
   int error = 0;
   int image_offset;

   printk("<1>[eightrack_init] Starting...\n");

   // Initialize the power-up timer structure
   init_timer(&eightrack_power_up_timer);  // Add timer to kernel list of timers
   eightrack_power_up_timer.data = 0;  // Timer currently not running
   eightrack_power_up_timer.function = eightrack_power_up_timer_timeout;

   // Initialize the display timer structure
   init_timer(&eightrack_display_timer);  // Add timer to kernel list of timers
   eightrack_display_timer.data = 0;  // Timer currently not running
   eightrack_display_timer.function = eightrack_display_timer_timeout;


   // Map the LCD controller registers 
   lcd_base_map = (char *)ioremap_nocache(LCD_BASE_ADDR, LCD_MAP_SIZE);
   if (debug_eightrack != 0) {
      printk("<1>[eightrack_init] lcd_base_map = 0x%x\n", (unsigned int)gpio_base_map);
   }
   reg_PPL_ADDR = (reg_addr_t) (lcd_base_map + (PPL_ADDR & LCD_MAP_MASK));
   reg_LPP_ADDR = (reg_addr_t) (lcd_base_map + (LPP_ADDR & LCD_MAP_MASK));

   // Map the GPIO registers for the eightrack signals
   gpio_base_map = (char *)ioremap_nocache(GPIO_BASE_ADDR, GPIO_MAP_SIZE);
   if (debug_eightrack != 0) {
      printk("<1>[eightrack_init] gpio_base_map = 0x%x\n", (unsigned int)gpio_base_map);
   }

   reg_EIGHT_TRACK_ERR_DR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_DR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_ERR_FE = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_FE_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_ERR_RE = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_RE_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_ERR_ED = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_ED_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_ERR_AF = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_AF_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_ERR_LR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_ERR_LR_ADDR & GPIO_MAP_MASK));

   reg_EIGHT_TRACK_RST_DR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_DR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RST_ED = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_ED_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RST_AF = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_AF_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RST_LR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_LR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RST_SR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_SR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RST_CR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RST_CR_ADDR & GPIO_MAP_MASK));

   reg_EIGHT_TRACK_STBY_DR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_DR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_FE = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_FE_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_ED = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_ED_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_AF = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_AF_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_LR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_LR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_SR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_SR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_STBY_CR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_STBY_CR_ADDR & GPIO_MAP_MASK));

   reg_EIGHT_TRACK_RDY_DR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_DR_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RDY_FE = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_FE_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RDY_RE = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_RE_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RDY_ED = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_ED_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RDY_AF = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_AF_ADDR & GPIO_MAP_MASK));
   reg_EIGHT_TRACK_RDY_LR = (reg_addr_t) (gpio_base_map + (EIGHT_TRACK_RDY_LR_ADDR & GPIO_MAP_MASK));


   // Set IRQ for eightrack ERR GPIO line; trigger on rising edge
   set_irq_type(EIGHT_TRACK_ERR_IRQ, IRQT_RISING);
   rqstatus = request_irq(EIGHT_TRACK_ERR_IRQ, eightrack_irq_ERR, SA_INTERRUPT, "eightrack", NULL);
   if (rqstatus != 0) {
      printk("<1>[eightrack_init] ***  ERROR: UP IRQ line = %d; request status = %d\n", EIGHT_TRACK_ERR_IRQ, rqstatus);
      error = 1;
   }

   // Set IRQ for eightrack RDY GPIO line; trigger on rising and falling edges
   set_irq_type(EIGHT_TRACK_RDY_IRQ, IRQT_FALLING | IRQT_RISING);
   rqstatus = request_irq(EIGHT_TRACK_RDY_IRQ, eightrack_irq_RDY, SA_INTERRUPT, "eightrack", NULL);
   if (rqstatus != 0) {
      printk("<1>[eightrack_init] ***  ERROR: PUSH IRQ line = %d; request status = %d\n", EIGHT_TRACK_RDY_IRQ, rqstatus);
      error = 1;
   }

   // Set the eightrack GPIO lines 

   // ERR line is an input, rising-edge detection
   write_reg(reg_EIGHT_TRACK_ERR_AF, EIGHT_TRACK_ERR_AF_LEN, EIGHT_TRACK_ERR_AF_POS, 0, EIGHT_TRACK_ERR_GAFR_MASK);  // No alternate function
   write_reg(reg_EIGHT_TRACK_ERR_DR, EIGHT_TRACK_ERR_DR_LEN, EIGHT_TRACK_ERR_DR_POS, 0, EIGHT_TRACK_ERR_REG_MASK);  // Direction = input
   write_reg(reg_EIGHT_TRACK_ERR_RE, EIGHT_TRACK_ERR_RE_LEN, EIGHT_TRACK_ERR_RE_POS, 1, EIGHT_TRACK_ERR_REG_MASK);  // Enable rising-edge

   // RST_L line is an output
   write_reg(reg_EIGHT_TRACK_RST_AF, EIGHT_TRACK_RST_AF_LEN, EIGHT_TRACK_RST_AF_POS, 0, EIGHT_TRACK_RST_GAFR_MASK);  // No alternate function
   write_reg(reg_EIGHT_TRACK_RST_CR, EIGHT_TRACK_RST_CR_LEN, EIGHT_TRACK_RST_CR_POS, 1, EIGHT_TRACK_RST_REG_MASK);  // Clear bit (Assert RST_L)
   write_reg(reg_EIGHT_TRACK_RST_DR, EIGHT_TRACK_RST_DR_LEN, EIGHT_TRACK_RST_DR_POS, 1, EIGHT_TRACK_RST_REG_MASK);  // Direction = output

   // STBY_L line is an output
   write_reg(reg_EIGHT_TRACK_STBY_AF, EIGHT_TRACK_STBY_AF_LEN, EIGHT_TRACK_STBY_AF_POS, 0, EIGHT_TRACK_STBY_GAFR_MASK);  // No alternate function
   write_reg(reg_EIGHT_TRACK_STBY_CR, EIGHT_TRACK_STBY_CR_LEN, EIGHT_TRACK_STBY_CR_POS, 1, EIGHT_TRACK_STBY_REG_MASK);  // Clear bit (Assert STBY_L)
   write_reg(reg_EIGHT_TRACK_STBY_DR, EIGHT_TRACK_STBY_DR_LEN, EIGHT_TRACK_STBY_DR_POS, 1, EIGHT_TRACK_STBY_REG_MASK);  // Direction = output


   // RDY line is an input, rising-edge and falling-edge detection
   write_reg(reg_EIGHT_TRACK_RDY_AF, EIGHT_TRACK_RDY_AF_LEN, EIGHT_TRACK_RDY_AF_POS, 0, EIGHT_TRACK_RDY_GAFR_MASK);  // No alternate function
   write_reg(reg_EIGHT_TRACK_RDY_DR, EIGHT_TRACK_RDY_DR_LEN, EIGHT_TRACK_RDY_DR_POS, 0, EIGHT_TRACK_RDY_REG_MASK);  // Direction = input
   write_reg(reg_EIGHT_TRACK_RDY_FE, EIGHT_TRACK_RDY_FE_LEN, EIGHT_TRACK_RDY_FE_POS, 1, EIGHT_TRACK_RDY_REG_MASK);  // Enable falling-edge interrupt
   write_reg(reg_EIGHT_TRACK_RDY_RE, EIGHT_TRACK_RDY_RE_LEN, EIGHT_TRACK_RDY_RE_POS, 1, EIGHT_TRACK_RDY_REG_MASK);  // Enable rising-edge interrupt

   printk("<1>[eightrack_init] GPIOs and IRQs have been set up\n");

   eightrack_state = RESET;  // Initialize state machine

   image_offset = init_fb_vars(info); // Calculate all frame buffer parameters

   // Return error code if any errors were found during initialization
   if (error != 0) {
      return ( -1 );
   }

   // Start the power-up timer.  When expired we will clear RST_L
   start_power_up_timer( );

   // Return the offset -in bytes- from the beginning of the
   // frame buffer to the starting point of the image data.
   // This offset is used by the calling program to move the
   // images into the frame buffer before calling send_fb_to_eightrack

   return ( image_offset );
}


//
// Exit routine.  This MUST be called when unloading the module.
//

void eightrack_exit(void)
{
   // Release all IRQs previously reserved
   free_irq(EIGHT_TRACK_ERR_IRQ, NULL);
   free_irq(EIGHT_TRACK_RDY_IRQ, NULL);

   // Stop power-up timer if running
   if (eightrack_power_up_timer.data != 0) {
      del_timer(&eightrack_power_up_timer);
      eightrack_power_up_timer.data = 0;
   }

   // Stop display timer if running
   if (eightrack_display_timer.data != 0) {
      del_timer(&eightrack_display_timer);
      eightrack_display_timer.data = 0;
   }

   printk("<1>[eightrack_exit] IRQs released and timer stopped.  Lehitraot!\n");
}



//
// command_eightrack sends the appropriate command sequence to the eightrack
// controller.
//

void command_eightrack(int command)
{
   ushort cmd_0 = 0;
   ushort checksum = 0;
   int cmd_ptr = 1;
   int i;
   int ready_line;

   // Do not modify frame buffer until eightrack controller is done with
   // any previous operation!  Exception: on power-up, the eightrack
   // controller is held reset so the ready line is low (ie busy).
   ready_line = read_reg(reg_EIGHT_TRACK_RDY_LR, EIGHT_TRACK_RDY_LR_LEN, EIGHT_TRACK_RDY_LR_POS);
   if ((command != CMD_POWERUP) && (ready_line == 0)) {
      printk("<1>[command_eightrack] ***  ERROR: attempted to send command while RDY=0; operation aborted.\n");
      return;
   }

   switch (command) {
   case CMD_POWERUP:
      if (debug_eightrack != 0) {
         printk("<1>[command_eightrack] About to send CMD_POWERUP...\n");
      }
      cmd_0 = 0xABC0 | ((_fb_mp[0] + 1) & 0xf);
      _fb_mp[cmd_ptr++] = 2048;  // Delay timing for PWR1
      _fb_mp[cmd_ptr++] = 2048;  // Delay timing for PWR2
      _fb_mp[cmd_ptr++] = 2048;  // Delay timing for PWR3
   break;
   case CMD_CONFIG:
      if (debug_eightrack != 0) {
         printk("<1>[command_eightrack] About to send CMD_CONFIG...\n");
      }
      cmd_0 = 0xCC10 | ((_fb_mp[0] + 1) & 0xf);
      if (_frame_width == EINK_6_INCH_FW) {
         // Note: parameters not listed in comments are zero
         _fb_mp[cmd_ptr++] = 0x021F;  // SDOSZ=2; SDLEW=31
         _fb_mp[cmd_ptr++] = 0x002A;  // GDSPL=42
         _fb_mp[cmd_ptr++] = 0x0012;  // GDSPW=18
         _fb_mp[cmd_ptr++] = 0x0257;  // VDLC=599
         _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
         _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
      }
      else if (_frame_width == EINK_9P7_INCH_FW) {
         // Note: parameters not listed in comments are zero
         _fb_mp[cmd_ptr++] = 0x0711;  // SDOSZ=7; SDLEW=17
         //_fb_mp[cmd_ptr++] = 0x0312;  // SDSHR=1; GDRL=1; GDSPL=18
         _fb_mp[cmd_ptr++] = 0x0311;  // SDSHR=1; GDRL=1; GDSPL=17
         _fb_mp[cmd_ptr++] = 0x003B;  // GDSPW=59
         _fb_mp[cmd_ptr++] = 0x0338;  // VDLC=824
         _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
         _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
      }
      else {
         printk("<1>[command_eightrack] ***  ERROR: Unrecognized display frame width; cannot issue correct CONFIG command to 8Track controller!\n");
      }
   break;
   case CMD_INIT:
      if (debug_eightrack != 0) {
         printk("<1>[command_eightrack] About to send CMD_INIT...\n");
      }
      cmd_0 = 0xCC20 | ((_fb_mp[0] + 1) & 0xf);
      _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
      _fb_mp[cmd_ptr++] = 0x0000;  // All parameters zero here
   break;
   case CMD_REFRESH:
      if (debug_eightrack != 0) {
         printk("<1>[command_eightrack] About to send CMD_REFRESH...\n");
      }
      cmd_0 = 0xCC30 | ((_fb_mp[0] + 1) & 0xf);
   break;
   case CMD_DISPLAY:
      if (debug_eightrack != 0) {
         printk("<1>[command_eightrack] About to send CMD_DISPLAY...\n");
      }
      cmd_0 = 0xCC40 | ((_fb_mp[0] + 1) & 0xf);
      _fb_mp[cmd_ptr++] = ( ( _frame_count & 0xff ) << 8 ); // Frame count for waveform
      // Start the RDY watchdog timer so we don't get stuck waiting
      start_display_timer(_frame_count * MSEC_PER_FRAME * 2);
   break;
   } // switch
   while (cmd_ptr < CMD_SIZE) {
      _fb_mp[cmd_ptr++] = 0;  // Padding for unused command parameters
   }
   checksum = cmd_0;
   for ( i = 1; i < ( CMD_SIZE-1 ); i++ ) checksum += _fb_mp[i];
   _fb_mp[CMD_SIZE-1] = checksum;

   // Fill up the rest of the line with the padding bytes
   for ( i = CMD_SIZE; i < _frame_width; i++ ) _fb_mp[i] = 0xCAFE;

   // To initiate the execution of the command at the eightrack controller,
   // we copy the first word now; the controller will go BUSY until done
   _fb_mp[0] = cmd_0;

   if (debug_eightrack != 0) {
      printk("<1>[command_eightrack] Frame buffer command contents:\n");
      for (i=0; i<_frame_width; i+=8) {
         if ( _fb_mp[i] != 0xCAFE ) {
            /* Only print if not "padding" bytes */
            printk("<1>%04X %04X %04X %04X %04X %04X %04X %04X\n",
                _fb_mp[i], _fb_mp[i+1], _fb_mp[i+2], _fb_mp[i+3], _fb_mp[i+4],
                _fb_mp[i+5], _fb_mp[i+6], _fb_mp[i+7]);
         }
      }
   }
}

//
// Put the requested waveform data in the frame buffer
// Returns 0 if successful, non-zero if an error occurred
//

int put_waveform_in_fb(int waveform_type)
{
   unsigned char * waveform_data;
   int waveform_size;
   int fb_index;
   int wf_count;

   switch ( waveform_type ) {
      case INIT_WAVEFORM:
         waveform_data = &waveform_data_INIT[0];
         waveform_size = sizeof(waveform_data_INIT);
      break;

      case MU_WAVEFORM:
         waveform_data = &waveform_data_MU[0];
         waveform_size = sizeof(waveform_data_MU);
      break;

      case GU_WAVEFORM:
         waveform_data = &waveform_data_GU[0];
         waveform_size = sizeof(waveform_data_GU);
      break;

      case GC_WAVEFORM:
         waveform_data = &waveform_data_GC[0];
         waveform_size = sizeof(waveform_data_GC);
      break;

      default:
         printk("<1>[put_waveform_in_fb] ***  ERROR: Invalid waveform type requested\n");
         return(1);
   }

   // Prepare to move the waveform data into the frame buffer

   _frame_count = waveform_size >> 6;  // Frame count depends on waveform size
   fb_index = _frame_width;  // Starting point of waveform data is on second line of FB

   // First move the data from the appropriate array into the FB
   for ( wf_count = 0; wf_count < waveform_size; wf_count+=2) {
      _fb_mp[fb_index++] = (ushort)( (waveform_data[wf_count] & 0x00ff) | 
                          ((waveform_data[wf_count+1] << 8) & 0xff00) );
   }

   // Fill the rest of the maximum waveform area with zeros
   while (wf_count < MAX_WFD_SIZE*2) {
      _fb_mp[fb_index++] = 0;
      wf_count+=2;
   }

   // Finally, add padding words if needed to complete the last line
   while (fb_index < (_image_start_line * _frame_width)) {
      _fb_mp[fb_index++] = 0xDEAD;
   }

   return( 0 );
}

//
// The following routine handles interrupts generated by the eightrack 
// power-up timer, for proper sequencing of the RST_L and STBY_L signals
//

static void eightrack_power_up_timer_timeout(unsigned long timer_data)
{
   switch ( eightrack_state ) {
      case RESET:
         if (debug_eightrack != 0) {
            printk("<1>[eightrack_power_up_timer_timeout] RESET state\n");
         }
         eightrack_state = STANDBY;  // New state
         command_eightrack(CMD_POWERUP); // Put "power up" command in frame buffer
         if (debug_eightrack != 0) {
            printk("<1>[eightrack_power_up_timer_timeout] Now clearing RST line\n");
         }
         write_reg(reg_EIGHT_TRACK_RST_SR, EIGHT_TRACK_RST_SR_LEN, EIGHT_TRACK_RST_SR_POS, 1, EIGHT_TRACK_RST_REG_MASK);  // Set bit (RST_L deasserted)
         // Start the power-up timer.  When expired we will clear RST_L
         start_power_up_timer( );
         break;
         
      case STANDBY:
         if (debug_eightrack != 0) {
            printk("<1>[eightrack_power_up_timer_timeout] STANDBY state; now clearing STBY_L line\n");
         }
         write_reg(reg_EIGHT_TRACK_STBY_SR, EIGHT_TRACK_STBY_SR_LEN, EIGHT_TRACK_STBY_SR_POS, 1, EIGHT_TRACK_STBY_REG_MASK);  // Set bit (STBY_L deasserted)
         // We do not start the power-up timer.
         eightrack_power_up_timer.data = 0;
         // When RDY is asserted we will get a RDY interrupt
         eightrack_state = WAITING_FOR_POWERUP_READY;
         break;

      default :
         printk("<1>[eightrack_power_up_timer_timeout] ***  ERROR: unexpected state: %d\n", eightrack_state);
   }
}


//
// The following routine handles interrupts generated by the eightrack 
// DISPLAY command timer, to make sure we are not stuck waiting for RDY.
//

static void eightrack_display_timer_timeout(unsigned long timer_data)
{
   int ready_line;

   // Perform actions depending on current state of RDY signal

   ready_line = read_reg(reg_EIGHT_TRACK_RDY_LR, EIGHT_TRACK_RDY_LR_LEN, EIGHT_TRACK_RDY_LR_POS);
   printk("<1>[eightrack_display_timer_timeout] TIMEOUT!  RDY signal = %d\n", ready_line);
   if ( (ready_line == 1) && (eightrack_state == WAITING_FOR_DISPLAY_READY) ) {
      eightrack_state = READY;
      printk("<1>[eightrack_display_timer_timeout] Forcing state to READY - looks like we missed the rising edge...");
   }
   else {
      printk("<1>[eightrack_display_timer_timeout] ***  ERROR - What do I do now???");
   }
}


//
// The following routine handles interrupts generated by the eightrack ERR
// signal.
//

static irqreturn_t eightrack_irq_ERR (int irq, void *data, struct pt_regs *r)
{
   printk("<1>[eightrack_irq_ERR] ***  ERROR: Got eightrack ERR signal\n");
   return IRQ_HANDLED;
}

//
// The following routine handles interrupts generated by the eightrack ERR
// signal.
//

static irqreturn_t eightrack_irq_RDY (int irq, void *data, struct pt_regs *r)
{
   int ready_line;
   int reset_line;

   // Perform actions depending on current state and RDY transition

   ready_line = read_reg(reg_EIGHT_TRACK_RDY_LR, EIGHT_TRACK_RDY_LR_LEN, EIGHT_TRACK_RDY_LR_POS);
   reset_line = read_reg(reg_EIGHT_TRACK_RST_LR, EIGHT_TRACK_RST_LR_LEN, EIGHT_TRACK_RST_LR_POS);
   if (debug_eightrack != 0) {
      printk("<1>[eightrack_irq_RDY] RDY signal = %d\n", ready_line);
      printk("<1>[eightrack_irq_RDY] RST signal = %d\n", reset_line);
   }

   //
   // If we got the interrupt because of a reset, ignore it
   //
   if (reset_line == 0) {
      if (debug_eightrack != 0) {
         printk("<1>[eightrack_irq_RDY] Got RDY because of RST_L being asserted.  This is OK.\n");
      }
      return IRQ_HANDLED;
   }

   switch ( eightrack_state ) {
      case WAITING_FOR_POWERUP_READY:
         if (ready_line != 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] Power up sequence complete; now issuing CONFIG command.\n");
            }
            eightrack_state = WAITING_FOR_CONFIG_BUSY;
            command_eightrack(CMD_CONFIG);
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY falling edge while WAITING_FOR_POWERUP_READY.\n");
         }
      break;

      case WAITING_FOR_CONFIG_BUSY:
         if (ready_line == 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] RDY went busy for CONFIG; now waiting for RDY.\n");
            }
            eightrack_state = WAITING_FOR_CONFIG_READY;
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY rising edge while WAITING_FOR_CONFIG_BUSY.\n");
         }
      break;

      case WAITING_FOR_CONFIG_READY:
         if (ready_line != 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] CONFIG command complete; now issuing INIT command.\n");
            }
            eightrack_state = WAITING_FOR_INIT_BUSY;
            command_eightrack(CMD_INIT);
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY falling edge while WAITING_FOR_CONFIG_READY.\n");
         }
      break;

      case WAITING_FOR_INIT_BUSY:
         if (ready_line == 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] RDY went busy for INIT; now waiting for RDY.\n");
            }
            eightrack_state = WAITING_FOR_INIT_READY;
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY rising edge while WAITING_FOR_INIT_BUSY.\n");
         }
      break;

      case WAITING_FOR_INIT_READY:
         if (ready_line != 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] INIT command complete; now going to RDY state.\n");
            }
            eightrack_state = READY;
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY falling edge while WAITING_FOR_INIT_READY.\n");
         }
      break;

      case WAITING_FOR_DISPLAY_BUSY:
         if (ready_line == 0) {
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] RDY went busy for DISPLAY; now waiting for RDY.\n");
            }
            eightrack_state = WAITING_FOR_DISPLAY_READY;
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY rising edge while WAITING_FOR_DISPLAY_BUSY.\n");
         }
      break;

      case WAITING_FOR_DISPLAY_READY:
         if (ready_line != 0) {
            stop_display_timer( ); // Stop the RDY line watchdog timer
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] DISPLAY command complete; now going to RDY state.\n");
            }
            eightrack_state = READY;
         }
         else {
            printk("<1>[eightrack_irq_RDY] ERROR: got RDY falling edge while WAITING_FOR_DISPLAY_READY.\n");
         }
      break;

      case WAITING_FOR_READY:
         if (ready_line != 0) {
            eightrack_state = READY;
            if (debug_eightrack != 0) {
               printk("<1>[eightrack_irq_RDY] Going to READY state\n");
            }
         }
         else {
            printk("<1>[eightrack_irq_RDY] ***  ERROR: RDY deasserted when waiting for READY\n");
         }
      break;

      case READY:
         if (ready_line != 0) {
            printk("<1>[eightrack_irq_RDY] ***  ERROR: RDY asserted when in READY state\n");
         }
         else {
            printk("<1>[eightrack_irq_RDY] ***  ERROR: RDY deasserted when in READY state\n");
         }
      break;

      default :
         printk("<1>[eightrack_irq_RDY] ***  ERROR: unexpected state: %d\n", eightrack_state);
   }

   return IRQ_HANDLED;
}

//
// This routine is used for polling.  It returns 1 if the eightrack driver state
// machine is in READY mode, indicating that it is OK for the calling routine
// to modify the frame buffer with data for a new image.  A zero return
// value indicates that we are not READY and the frame buffer should NOT
// be modified.
//

int is_eightrack_ready(void)
{
   if (eightrack_state == READY) {
      return ( 1 );
   }
   else {
      return ( 0 );
   }
}


//
// Send the image already contained in the frame buffer to the eightrack
// controller for display in the eInk display.  The parameter passed
// determines the type of waveform to send with the frame buffer.
// The routine returns 0 to indicate that the display command has been
// successfully started.  Completion of the command will take place when
// the RDY signal is asserted; this will cause an interrupt that will be
// serviced by eightrack_irq_ready.
//

int send_fb_to_eightrack(int waveform_type)
{
   eightrack_state = WAITING_FOR_DISPLAY_BUSY;
   put_waveform_in_fb(waveform_type);
   command_eightrack(CMD_DISPLAY);
   return ( 0 );
}

