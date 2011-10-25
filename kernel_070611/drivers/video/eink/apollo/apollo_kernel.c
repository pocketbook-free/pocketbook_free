#include "apollo.h"

// Define to report power state transition timings
#undef REPORT_POWER_TRANSITION_TIMING

extern void __eink_fb_power_clear_screen(void);
extern int fiona_sys_getpid(void);

extern FPOW_POWER_MODE g_eink_power_mode;
extern int flash_fail_count;

wait_queue_head_t apollo_ack_wq;
int g_current_apollo_power_mode = APOLLO_NORMAL_POWER_MODE;

// Local Prototypes
int apollo_set_power_state(int target_mode);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23)
static void eink_irq_hdl(int irq, void *data, struct pt_regs* regs) {
#else
irqreturn_t eink_irq_hdl(int irq, void *data, struct pt_regs* regs) {
#endif
  wake_up(&apollo_ack_wq);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,23)
        return IRQ_HANDLED;
#endif
}

void apollo_init_irq(void) {
  int rqstatus;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
  set_irq_type(IRQ_GPIO(83), IRQT_FALLING);
#else
  set_GPIO_IRQ_edge(83, GPIO_FALLING_EDGE);
  printk("SET FALLING EDGE 2.4 style done\n");
#endif

  rqstatus = request_irq(IRQ_GPIO(83), eink_irq_hdl, SA_INTERRUPT, "einkfb", NULL);
  do_debug("rqstatus %d irq %d\n", rqstatus, IRQ_GPIO(83));

  disable_irq(IRQ_GPIO(83)); /* initially disable irq.  we will use it 
                                only for longer waits to avoid prolonged 
                                spinning */
  do_debug("DISABLE_IRQ done\n");
}

void apollo_shutdown_irq(void) {
  free_irq(IRQ_GPIO(83), NULL);
}

static int g_ack_clients = 0;
static int g_ack_waiting_pid = 0;

int wait_for_ack_irq(int which) {
  int pid = fiona_sys_getpid();
  
  g_ack_clients++;
  if (g_ack_clients > 1)  {
    /* Someone is already waiting for an EINK controller acknowledgement.
     * We should never have two processes waiting here for acks, as that
     * indicates two separate processes presumably writing two separate
     * things to hardware, each thinking it owns it.... 
     * For now, this is a hack fix to avoid the effect when we hit this bug.
     * Real solution is to fix semaphore issue with driver and setmode. -njv
     */
    printk("BUG !!! Eink semaphore issue in wait_for_ack_irq !!\n");
    printk("BUG !!! Waiting pid: %d, this pid: %d\n",g_ack_waiting_pid,pid); 
    g_ack_clients--;
    return 1;
  }

  g_ack_waiting_pid = pid;
  enable_irq(IRQ_GPIO(83));
  wait_event_interruptible_timeout(apollo_ack_wq, (apollo_get_ack() == which), MAX_TIMEOUT);
  disable_irq(IRQ_GPIO(83));
  g_ack_clients--;
  g_ack_waiting_pid = 0;
  return 1;
}

int apollo_init_kernel(int full) {
  unsigned long bpp = eink_fb_get_bpp();
  
  if (full) {
    g_current_apollo_power_mode = APOLLO_NORMAL_POWER_MODE;
    apollo_power_up();
    apollo_init(bpp);

  } else {
    apollo_status_t status = { 0 };

    g_current_apollo_power_mode = APOLLO_SLEEP_POWER_MODE;
    apollo_comm_init();

    // If we're not in sleep mode or the right bit depth, then say that we need to be
    // fully re-initialized.
    //
    if (apollo_get_status(&status) && ((0 == status.operation_mode) || (bpp != (status.bpp + 1)))) {

      // Something's wrong, so dump out all of the status info to help us figure why.
      //
      printk("apollo_init_kernel: operation mode = 0x%02X\n", status.operation_mode);
      printk("apollo_init_kernel:  screen status = 0x%02X\n", status.screen);
      printk("apollo_init_kernel:   auto refresh = 0x%02X\n", status.auto_refresh);
      printk("apollo_init_kernel:            bpp = 0x%02X\n", status.bpp);

      full = 1;
      apollo_init_kernel(full);
    }
  }
  
  return full;
}

void apollo_done_kernel(int full) {
  int apollo_init_display_speed = APOLLO_INIT_DISPLAY_SLOW;
  
  if (full) {
    if (0 == apollo_set_power_state(APOLLO_STANDBY_OFF_POWER_MODE)) {
      apollo_init_display_speed = APOLLO_INIT_DISPLAY_FAST;
      apollo_power_down(full);
    }
  } else {
    if (0 == apollo_set_power_state(APOLLO_SLEEP_POWER_MODE)) {
      apollo_init_display_speed = APOLLO_INIT_DISPLAY_FAST;
      apollo_comm_shutdown();
    }
  }
  
  apollo_set_init_display_speed(apollo_init_display_speed);
}

int needs_to_enable_temp_sensor(void) {
	if (get_fiona_board_revision() >= FIONA_BOARD_DVT1)
		return 1;
	else
		return 0;
}

int WriteToFlash (unsigned long Address, char Data) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command  = APOLLO_FLASH_WRITE;
  apollo_command.type     = APOLLO_WRITE_ONLY;
  apollo_command.num_args = 4;
  apollo_command.args[0]  = (u8)((Address & 0x00FF0000) >> 16);
  apollo_command.args[1]  = (u8)((Address & 0x0000FF00) >> 8);
  apollo_command.args[2]  = (u8)((Address & 0x000000FF));
  apollo_command.args[3]  = (u8)Data;
  
  apollo_send_command(&apollo_command);
  
  return 1;
}

int ReadFromRam (unsigned long Address, unsigned long *Data) {
  unsigned char b0, b1, b2, b3;

  Address = Address >> 2;

  apollo_write_register(0x16, (unsigned char)(Address & 0xff));
  apollo_write_register(0x15, (unsigned char)((Address >> 8) & 0xff));
  apollo_write_register(0x14, (unsigned char)((Address >> 16) & 0xff));
  apollo_write_register(0x17, 1);

  apollo_read_register(0x1d, &b0);
  apollo_read_register(0x1c, &b1);
  apollo_read_register(0x1b, &b2);
  apollo_read_register(0x1a, &b3);

  *Data = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;

  return 1;
}

int WriteToRam(unsigned long Address, unsigned long Data) {

  Address = Address >> 2;

  apollo_write_register(0x16, (unsigned char)(Address & 0xff));
  apollo_write_register(0x15, (unsigned char)((Address >> 8) & 0xff));
  apollo_write_register(0x14, (unsigned char)((Address >> 16) & 0xff));

  apollo_write_register(0x1d, (unsigned char)(Data & 0xff));
  apollo_write_register(0x1c, (unsigned char)((Data >> 8) & 0xff));
  apollo_write_register(0x1b, (unsigned char)((Data >> 16) & 0xff));
  apollo_write_register(0x1a, (unsigned char)((Data >> 24) & 0xff));

  apollo_write_register(0x17, 2);

  return 1;
}

int ProgramRam(unsigned long start_addr, unsigned char *buffer, unsigned long blen) {
  unsigned long i;
  unsigned long *lptr;
  lptr = (unsigned long *) buffer;
  for(i = start_addr; i < start_addr + blen; i+=4) {
    WriteToRam(i, *lptr);
    lptr++;
  }
  return 1;
}

int VerifyByte (unsigned long Address, char *Data) {
  int Check;
  char Toggle;

  Check = ReadFromFlash (Address, &Toggle);

  if ((Check) && (*Data != Toggle))
      Check = 0;

  *Data = Toggle;
  return(Check);
}

unsigned long SectorSize (unsigned long Sector) {
  if (Sector < 63)
    return(64*1024);
  else
    return(8*1024);
}

unsigned long SectorAddress (unsigned long Sector) {;
  switch (Sector) {
  case 8:
    return(0x78000);
  case 9:
    return(0x7a000);
  case 10:
    return(0x7c000);
  default:
    return(Sector << 16);
  }

}

int EraseSector (unsigned long Sector) {
  unsigned long Address;
  int   Busy;
  int   Check;
  char  Toggle;
  char  SaveToggle;

  Address = SectorAddress (Sector);
  Check   = WriteToFlash (0x000AAA, (char)0xAA);
  Check   = (Check && WriteToFlash (0x000555, (char)0x55));
  Check   = (Check && WriteToFlash (0x000AAA, (char)0x80));
  Check   = (Check && WriteToFlash (0x000AAA, (char)0xAA));
  Check   = (Check && WriteToFlash (0x000555, (char)0x55));
  Check   = (Check && WriteToFlash (Address, (char)0x30));
  Check   = (Check && ReadFromFlash (Address, &SaveToggle));

  Busy = 1;

  while ((Check) && (Busy)) {
    Check = (Check && ReadFromFlash (Address, &Toggle));

    if (Check) { 
      Busy = (((Toggle & 0x40) != (SaveToggle & 0x40))); 
      SaveToggle = Toggle;
      if (Busy && ((Toggle & 0x20) != 0x00)) {
  Check = (Check && ReadFromFlash (Address, &Toggle));
  if (Check) {
    Busy = (((Toggle & 0x40) != (SaveToggle & 0x40)) && ((Toggle & 0x80) == 0x00));
    SaveToggle = Toggle;
    if (Busy) 
      Check = 0;
        }
      }
    }
  }
  return(Check);
}

int ProgramByte (unsigned long Address, unsigned char Data) {
  int Busy;
  int Check;
  char Toggle;
  char SaveToggle;

  Check = WriteToFlash (0x000AAA, (char)0xAA);
  Check = (Check && WriteToFlash(0x000555, (char)0x55));
  Check = (Check && WriteToFlash(0x000AAA, (char)0xA0));
  Check = (Check && WriteToFlash(Address, Data));
  Check = (Check && ReadFromFlash(Address, &SaveToggle));

  Busy = 1;

  while ((Check) && (Busy)) {
    Check = (Check && ReadFromFlash (Address, &Toggle));
    if (Check) {     
      Busy = ((Toggle & 0x40) != (SaveToggle & 0x40));
      SaveToggle = Toggle;
      if ((Busy) && ((Toggle & 0x20) != 0x00)) {
  Check = (Check && ReadFromFlash (Address, &Toggle));
  if (Check) {
    Busy = ((Toggle & 0x40) != (SaveToggle & 0x40));
    SaveToggle = Toggle;
    if (Busy)    
      Check = 0;  
        }
      }
    }
  } 
  return(Check);
}

int ProgramFlash(unsigned long start_addr, unsigned char *buffer, unsigned long blen) {
  static unsigned long nextSector = 0;
  static unsigned long nextSectorAddress = 0;

  unsigned long bptr = 0;
  unsigned long curraddr = 0;

  if(start_addr == 0) {
    printk("resetting sector & address\n");
    nextSector = 0;
    nextSectorAddress = 0;
  }
  printk("programming chunk size %d at %d\n", (int) blen, (int) start_addr);
  for(bptr = 0, curraddr = start_addr; bptr < blen; bptr++, curraddr++) {

    if(curraddr >= nextSectorAddress) {
      printk("erase sector %d\n", (int) nextSector);
      EraseSector(nextSector);
      nextSector++;
      nextSectorAddress = SectorAddress(nextSector);
    }
    
    if(buffer[bptr] != 0xFF) {
      unsigned char data = buffer[bptr];
      int retries = 0;
    
      while(!(ProgramByte(curraddr, data) && VerifyByte(curraddr, &data)) && (APOLLO_FLASH_MAX_RETRIES > retries++)) {
        printk("failed to flash byte at 0x%08lX; write = 0x%02X, read = 0x%02X\n", curraddr, buffer[bptr], data);
        data = buffer[bptr];
      }
      
      if (retries) {
        flash_fail_count++;
      }
    }
  } 
  return 1;
}

/**************************************************************************
 * Power Management Routines
 **************************************************************************/

char *
apollo_get_power_state_str(int state)
{
    switch (state)  {
        case APOLLO_NORMAL_POWER_MODE:
            return("NORMAL_MODE");
        case APOLLO_SLEEP_POWER_MODE:
            return("SLEEP_MODE");
        case APOLLO_STANDBY_POWER_MODE:
        	return("STANDBY_MODE");
        case APOLLO_STANDBY_OFF_POWER_MODE:
            return("STANDBY_OFF_MODE");
        default:
            return("INVALID MODE");
    }
}

#define EINK_FPOW_MODE_OFF() (FPOW_MODE_OFF == g_eink_power_mode)

void
verify_apollo_state(void)
{
    int not_okay = 0;
    
    switch ( g_current_apollo_power_mode )
    {
    	case APOLLO_NORMAL_POWER_MODE:
    	case APOLLO_SLEEP_POWER_MODE:
    	case APOLLO_STANDBY_POWER_MODE:
    		if ( EINK_FPOW_MODE_OFF() )
    			not_okay = 1;
    	break;

    	case APOLLO_STANDBY_OFF_POWER_MODE:
    		if ( !EINK_FPOW_MODE_OFF() )
    			not_okay = 1;
    	break;
    	
    	default:
    		printk("verify_apollo_state(): Unknown power mode state %d\n", g_current_apollo_power_mode);
    	break;
    }

	if ( not_okay )
		powdebug("\n\nERROR !!  Apollo Mode: %s,  FPOW EINK Mode: %s\n", apollo_get_power_state_str(g_current_apollo_power_mode), fpow_mode_to_str(g_eink_power_mode));
}

// Normal:	GPIOs for normal (full on) Apollo operation.
//
static void apollo_normal_gpios(void)
{
	if (needs_to_enable_temp_sensor()) {
		fiona_pgsr_clear(GPIO_FIONA_TEMP_SENSOR);
	}

	fiona_pgsr_set(GPIO_FIONA_APOLLO_COMMON);
	fiona_pgsr_set(GPIO_FIONA_APOLLO_POWER);
}

// Sleep:	Normal GPIOs.
//
#define apollo_sleep_gpios()	apollo_normal_gpios()

// Wake:	Normal GPIOs.
//
#define apollo_wake_gpios()		apollo_normal_gpios()

// Standby:	Normal GPIOs.
//
#define apollo_standby_gpios()	apollo_normal_gpios()

// Standby-off:	GPIOs for standby operation and no power to Apollo.
//
static void apollo_standby_off_gpios(void)
{
	if (needs_to_enable_temp_sensor()) {
		fiona_pgsr_set(GPIO_FIONA_TEMP_SENSOR);
	}
	
	fiona_pgsr_clear(GPIO_FIONA_APOLLO_COMMON);
	fiona_pgsr_clear(GPIO_FIONA_APOLLO_POWER);
}

/*********************************************************
 * Routine:     apollo_wake
 * Purpose:     Initialize the Apollo chip after coming from
 *              sleep power mode.
 * Paramters:   bpp - screen bits-per-pixel depth
 * Returns:     nothing
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *********************************************************/
static void
apollo_wake(
    void)
{
    apollo_status_t status;
    extpowdebug("apollo_wake() re-initializing Apollo...\n");

    apollo_get_status(&status);
    if (1 == status.operation_mode)
      printk("apollo_wake(): Apollo still in sleep mode!!!!\n");
    extpowdebug("Apollo Status: 0x%02x\n",(int)(*((unsigned char*)&status)));

 	apollo_init(eink_fb_get_bpp());
	apollo_wake_gpios();

  extpowdebug("apollo_wake() exiting...\n");
}

/*********************************************************
 * Routine:     apollo_wake_on_activity
 * Purpose:     Wakes the Apollo chip after coming from
 *              standby or sleep power mode.
 * Paramters:   none
 * Returns:     1 - Error - couldn't wake the Apollo
 *              0 - Woke up the Apollo
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *********************************************************/
int
apollo_wake_on_activity()
{
    int retVal = 0;

	if (g_current_apollo_power_mode != APOLLO_NORMAL_POWER_MODE) {
        extpowdebug("apollo_wake_on_activity() calling apollo_set_power_state(NORMAL_MODE)\n");
        retVal = apollo_set_power_state(APOLLO_NORMAL_POWER_MODE);
    }
    
    return retVal;
}

/*****************************************************************************
 * Routine:     execute_wake_up_mode_cmd
 * Purpose:     Send a wakeup command to Apollo chip.
 * Paramters:   None
 * Returns:     1 - SUCCESS
 *              0 - FAILED
 * Assumptions: Apollo is in StandBy mode
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 * Implementation Details:
 *  The following is the wakeup sequence for the Apollo chip:
 *      1) H-WUP is set to "high"
 *      2) H_DS is set to "Low"  Controller makes H_ACK active "High"
 *      3) Host waits til mode switch is done and H_ACK is "Low"
 *      4) Host sets H_DS back to "High".  H_ACK is set to high impedence ("Z")
 *****************************************************************************/
#define BAIL_OUT    (1000*1000) // 1 second
static int
execute_wake_up_mode_cmd(void)
{
	int success = 1;
	int loop_count = 0;

	// 0) Power up and wake
	apollo_power_up();
	apollo_wake_gpios();

    // 1) H-WUP is set to "high"
    GPSR(GPIO70_FIONA_APOLLO_H_WUP) = GPIO_bit(GPIO70_FIONA_APOLLO_H_WUP);

    // t(SETUP) min 20 nsec
    ndelay(20);

    // 2) H_DS is set to "Low"  Controller makes H_ACK active "High"
    GPCR(GPIO66_FIONA_APOLLO_H_DS) = GPIO_bit(GPIO66_FIONA_APOLLO_H_DS);

    // t(ACK_Z2H) max 10 nsec
    ndelay(10);

    /****
     Note from eink folks:
     1) DS goes low -> ACK goes from high impedance to driven high
     2) ACK goes from driven high to driven low
     3) ACK remains driven low until DS returns high, at which point it 
     goes back to high impedance
     
     In both cases, the delay from falling edge of DS to rising edge of ACK 
     is about 6nS.
     
     With the regular clock running, ACK remains high for about 70nS before 
     being driven low.
     
     In standby mode, ACK remains high for about 520uS before being driven 
     low. This is for the first part of the wakeup sequence where you put 
     WUP high and give a DS pulse.
     
     In both cases, ACK remains driven low until DS is brought high again, 
     at which point it goes back to high-Z.
    ***/

#ifdef LOOK_FOR_LOW_TO_HIGH_ACK_TRANSITION
    // Since the line is pulled up, we could never see the transition from
    // 0->1, so we're just going to ignore that transition and wait for it
    // to drop back down low again. -njv

    // 2b) Wait for ACK to go high
    loop_count = 0;
    while (((GPLR(GPIO83_FIONA_APOLLO_H_ACK) & (GPIO_bit(GPIO83_FIONA_APOLLO_H_ACK))) == 0) && (loop_count < BAIL_OUT)) {
        // Should we give up time here??
        loop_count++;
        udelay(1);
    }
    if (loop_count >= BAIL_OUT)
        printk("\nTimed out waiting for H_ACK to go HIGH !!! (0x%x)\n",
                  (GPLR(GPIO83_FIONA_APOLLO_H_ACK) &  \
                  (GPIO_bit(GPIO83_FIONA_APOLLO_H_ACK))));
#endif

    // 3) Host waits til mode switch is done and H_ACK is "Low" (~ 8 ms)
    loop_count = 0;
    while (((GPLR(GPIO83_FIONA_APOLLO_H_ACK) & 
            (GPIO_bit(GPIO83_FIONA_APOLLO_H_ACK))) != 0) && 
                    (loop_count < BAIL_OUT)) {
        // Should we give up time here??
        udelay(1);
        loop_count++;
    }

    if (loop_count >= BAIL_OUT)
        printk("\nTimed out waiting for H_ACK to go LOW !!! (0x%x)\n",
                  (GPLR(GPIO83_FIONA_APOLLO_H_ACK) &  \
                  (GPIO_bit(GPIO83_FIONA_APOLLO_H_ACK))));

    // t(WUP_HL) min 35 nsec
    ndelay(35);

    // 3b) H-WUP is set to "low"
    GPCR(GPIO70_FIONA_APOLLO_H_WUP) = GPIO_bit(GPIO70_FIONA_APOLLO_H_WUP);

    // for a good time
    ndelay(10);

    // 4) Host sets H_DS back to "High".  H_ACK is set to high
    loop_count = 0;
    GPSR(GPIO66_FIONA_APOLLO_H_DS) = GPIO_bit(GPIO66_FIONA_APOLLO_H_DS);

#ifdef WAIT_FOR_FINAL_HACK_HIGH
    while (((GPLR(GPIO83_FIONA_APOLLO_H_ACK) & (GPIO_bit(GPIO83_FIONA_APOLLO_H_ACK))) == 0) && (loop_count < BAIL_OUT)) {
        // Should we give up time here??
        udelay(1);
        loop_count++;
    }

    if (loop_count >= BAIL_OUT)
        printk("\nTimed out waiting for H_ACK to go back high !!!\n");
#endif

    extpowdebug("apollo<<LO-level wake\n");
    return(success);
}

/*****************************************************************************
 * Routine:     execute_standby_mode_cmd
 * Purpose:     Put the Apollo chip in Standby Power mode.
 * Paramters:   target mode
 * Returns:     None
 * Assumptions: Apollo is NOT in StandBy mode
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *****************************************************************************/
static int
execute_standby_mode_cmd(int target_mode)        
{
    apollo_command_t apollo_command = { 0 };
    
    apollo_command.command  = APOLLO_STANDBY_MODE_CMD;
    apollo_command.type     = APOLLO_WRITE_NEEDS_DELAY;
    apollo_command.io       = APOLLO_STANDBY_CMD_DELAY;
    apollo_send_command(&apollo_command);
    
    if ( APOLLO_STANDBY_OFF_POWER_MODE == target_mode )
    {
    	apollo_power_down(APOLLO_POWER_DOWN_SLEEP);
    	apollo_standby_off_gpios();
   	}
    else
    	apollo_standby_gpios();
    
    extpowdebug("apollo>>LO-level sleep\n");
    
    return(target_mode);
}

/*****************************************************************************
 * Routine:     execute_sleep_mode_cmd
 * Purpose:     Put the Apollo chip in Sleep Power mode.
 * Paramters:   None
 * Returns:     None
 * Assumptions: Apollo is NOT in StandBy mode
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *****************************************************************************/
static void
execute_sleep_mode_cmd(void)
{
    apollo_simple_set(APOLLO_SLEEP_MODE_CMD);
    apollo_sleep_gpios();
    
    extpowdebug("apollo>>HI-level sleep\n");
}

/*****************************************************************************
 * Routine:     execute_normal_mode_cmd
 * Purpose:     Put the Apollo chip in Normal Power mode.
 * Paramters:   None
 * Returns:     None
 * Assumptions: Apollo is NOT in StandBy mode
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *****************************************************************************/
static void
execute_normal_mode_cmd(void)
{
    apollo_simple_set(APOLLO_NORMAL_MODE_CMD);
    apollo_wake();
    
    extpowdebug("apollo<<HI-level wake\n");
 }

/**************************************************************************/

/*****************************************************************************
 * Routine:     apollo_set_power_state
 * Purpose:     Transition the Apollo chip to a requested power mode.
 * Paramters:   NORMAL_POWER_MODE         - Fast clock, all commands
 *              SLEEP_POWER_MODE          - Slow clock, limited commands
 *              STANDBY_OFF_POWER_MODE	  - No clocks, no commands, no power
 *              STANDBY_POWER_MODE        - No clocks, no commands
 * Returns:     1 - Failed to transition to proper power mode
 *              0 - Transitioned to requested power mode
 * Author:      Nick Vaccaro <nvaccaro@lab126.com>
 *****************************************************************************/
int
apollo_get_power_state(void)
{
	return ( g_current_apollo_power_mode );
}

int
apollo_set_power_state(int target_mode)
{
    int failure = 0;
    int rc = 0;
#ifdef REPORT_POWER_TRANSITION_TIMING
    int start_state = g_current_apollo_power_mode;
    int start = OSCR;
    int end;
#endif

    if (g_current_apollo_power_mode != target_mode) {
      powdebug("apollo_set_power_state(%s-->%s)\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
    }

    // Keep transitioning until we're in the requested power mode
    while (g_current_apollo_power_mode != target_mode)  {

        switch(g_current_apollo_power_mode)  {
    
        case APOLLO_NORMAL_POWER_MODE:
            if (IS_APOLLO_STANDBY_MODE(target_mode)) {
                // Transition directly to Standby mode
                extpowdebug("apollo_set_power_state(%s-->%s) calling execute_standby_mode_cmd\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                g_current_apollo_power_mode = execute_standby_mode_cmd(target_mode);        
                continue;
            }
            else if (APOLLO_SLEEP_POWER_MODE == target_mode)  {
                // Transition directly to Sleep mode
                extpowdebug("apollo_set_power_state(%s-->%s) calling execute_sleep_mode_cmd\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                execute_sleep_mode_cmd();        
                g_current_apollo_power_mode = APOLLO_SLEEP_POWER_MODE;
                continue;
            }
            else {
                failure = 1;    // Invalid state
                extpowdebug("apollo_set_power_state(%s-->%s) INVALID\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                goto exit;
            }
            break;

        case APOLLO_SLEEP_POWER_MODE:
            if (IS_APOLLO_STANDBY_MODE(target_mode)) {
                // Transition directly to Standby mode
                extpowdebug("apollo_set_power_state(%s-->%s) calling execute_standby_mode_cmd\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                g_current_apollo_power_mode = execute_standby_mode_cmd(target_mode);        
                continue;
            }
            else if (APOLLO_NORMAL_POWER_MODE == target_mode)  {
                // Transition directly to Normal mode
                extpowdebug("apollo_set_power_state(%s-->%s) calling execute_normal_mode_cmd\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                execute_normal_mode_cmd();        
                g_current_apollo_power_mode = APOLLO_NORMAL_POWER_MODE;
                continue;
            }
            else {
                failure = 1;    // Invalid state
                extpowdebug("apollo_set_power_state(%s-->%s) INVALID\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                goto exit;
            }
            break;

        case APOLLO_STANDBY_OFF_POWER_MODE:
        case APOLLO_STANDBY_POWER_MODE:
            if ((APOLLO_NORMAL_POWER_MODE == target_mode) ||
                (APOLLO_SLEEP_POWER_MODE == target_mode)  ||
                (IS_APOLLO_STANDBY_MODE(target_mode))) {
                // To get to any other mode, we must transition
                // thru Sleep mode first 
                extpowdebug("apollo_set_power_state(%s-->%s) calling execute_wake_up_mode_cmd\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
                rc = execute_wake_up_mode_cmd();
                if (rc) {
                    extpowdebug("apollo_set_power_state(%s-->%s) - execute_wake_up_mode_cmd() returned %d\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode),rc);
                    execute_sleep_mode_cmd();
                    g_current_apollo_power_mode = APOLLO_SLEEP_POWER_MODE;
                    continue;
                }
            }
            // FIXME - what do we do if we can't transition out of this state?
            failure = 1;    // Invalid state
            extpowdebug("apollo_set_power_state(%s-->%s) INVALID\n",apollo_get_power_state_str(g_current_apollo_power_mode),apollo_get_power_state_str(target_mode));
            goto exit;

        default:
            printk("apollo_set_power_state(): Unknown power mode state %d\n",g_current_apollo_power_mode);
            failure = 1;
            goto exit;
        }
    }

#ifdef REPORT_POWER_TRANSITION_TIMING
    end = OSCR;

    printk("apollo_set_power_state(): %s to %s in %d uSec\n", \
        apollo_get_power_state_str(start_state), \
        apollo_get_power_state_str(target_mode),(end-start)/4);
#endif
exit:
    extpowdebug("apollo_set_power_state() returned %d (cur state %s)\n",failure,apollo_get_power_state_str(g_current_apollo_power_mode));

    return(failure);
}
