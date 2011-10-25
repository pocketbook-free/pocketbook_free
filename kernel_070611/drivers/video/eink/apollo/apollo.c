#include "apollo.h"
#include "apollo_waveform.c"

static char *apollo_base_map  = NULL;

static reg_addr_t reg_GPSR1   = NULL;
static reg_addr_t reg_GPSR2   = NULL;

static reg_addr_t reg_GPCR1   = NULL;
static reg_addr_t reg_GPCR2   = NULL;

static reg_addr_t reg_GPLR1   = NULL;
static reg_addr_t reg_GPLR2   = NULL;

static reg_addr_t reg_GPDR1   = NULL;
static reg_addr_t reg_GPDR2   = NULL;

static int apollo_init_comm   = 0;
static int apollo_standby     = 0; // Workaround for PVI-6001A.
static u8  apollo_orientation = APOLLO_ORIENTATION_270;

#ifndef APOLLO_UNIVERSAL_BUILD
static void apollo_preprocess_command(apollo_preprocess_command_t *apollo_preprocess_command,
  apollo_command_t *apollo_command)
{
  // Do nothing.
}

static void apollo_postprocess_command(apollo_command_t *apollo_command)
{
  // Do nothing.
}

static apollo_controller_preprocess_command_t apollo_controller_preprocess_command    = 
  apollo_preprocess_command;
  
static apollo_controller_postprocess_command_t apollo_controller_postprocess_command  =
  apollo_postprocess_command;
  
#else
  
static apollo_controller_preprocess_command_t apollo_controller_preprocess_command    = 
  universal_preprocess_command;
  
static apollo_controller_postprocess_command_t apollo_controller_postprocess_command  =
  universal_postprocess_command;
  
#endif // APOLLO_UNIVERSAL_BUILD

int apollo_comm_init(void) {
  if (0 == apollo_init_comm) {
    init_waitqueue_head(&apollo_ack_wq);
    
    apollo_base_map = (char *)ioremap_nocache(BASE_ADDR, MAP_SIZE);
    
    reg_GPSR1 = (reg_addr_t) (apollo_base_map + (GPSR1_ADDR & MAP_MASK));
    reg_GPSR2 = (reg_addr_t) (apollo_base_map + (GPSR2_ADDR & MAP_MASK));
    
    reg_GPCR1 = (reg_addr_t) (apollo_base_map + (GPCR1_ADDR & MAP_MASK));
    reg_GPCR2 = (reg_addr_t) (apollo_base_map + (GPCR2_ADDR & MAP_MASK));
    
    reg_GPLR1 = (reg_addr_t) (apollo_base_map + (GPLR1_ADDR & MAP_MASK));
    reg_GPLR2 = (reg_addr_t) (apollo_base_map + (GPLR2_ADDR & MAP_MASK));
    
    reg_GPDR1 = (reg_addr_t) (apollo_base_map + (GPDR1_ADDR & MAP_MASK));
    reg_GPDR2 = (reg_addr_t) (apollo_base_map + (GPDR2_ADDR & MAP_MASK));
    
    apollo_init_irq();
    
    apollo_init_comm = 1;
  }

  return 0;
}

int apollo_comm_shutdown(void) {
  if (1 == apollo_init_comm) {
    apollo_shutdown_irq();
    
    iounmap(apollo_base_map);
    apollo_base_map = NULL;
    
    reg_GPSR1 = NULL;
    reg_GPSR2 = NULL;
    
    reg_GPCR1 = NULL;
    reg_GPCR2 = NULL;
    
    reg_GPLR1 = NULL;
    reg_GPLR2 = NULL;
    
    reg_GPDR1 = NULL;
    reg_GPDR2 = NULL;
    
    apollo_init_comm = 0;
  }

  return 0;
}

u32 apollo_get_ack(void) {
  return((apollo_read_reg(reg_GPLR2) & 0x00080000) >> 19);
}

int wait_for_ack(int which) {
  register int i=0;

  for(i = 0; i < FAST_TIMEOUT; i++) {
      if(apollo_get_ack() == which) {
        return (1);
      } 
  }

  return wait_for_ack_irq(which);
}

u32 apollo_read_write(int which, u8 wdata) {
  u32 gpsr1, gpsr2, gpcr1, gpcr2, gpdr1, gpdr2, gplr1, gplr2, rdata;

  gpsr1 = gpsr2 = gpcr1 = gpcr2 = gpdr1 = gpdr2 = gplr1 = gplr2 = rdata = 0;

  if(apollo_base_map == NULL) {
    printk("apollo_base_map = NULL\n");
    return rdata;
  }

  /* set H_CD high for commands, low for data and for reads */
  switch (which) {
    case APOLLO_WRITE_COMMAND:
      apollo_write_reg(reg_GPSR2, 1 << 3);
      break;
      
    case APOLLO_WRITE_DATA:
    case APOLLO_READ:
      apollo_write_reg(reg_GPCR2, 1 << 3);
      break;
  }
  
  /* prepare H_D[7:0] for reads and writes   */
  /* set H_RW high for reads, low for writes */
  switch (which) {
    case APOLLO_WRITE_COMMAND:
    case APOLLO_WRITE_DATA:
      gpsr1 = wdata << 26;
      gpsr2 = wdata >> 6;
      gpcr1 = (0x000000ff & ~wdata) << 26;
      gpcr2 = (0x000000ff & ~wdata) >> 6;
      
      apollo_write_reg(reg_GPCR2, 1 << 5);
      break;

    case APOLLO_READ:
      gpdr1 = apollo_read_reg(reg_GPDR1);
      gpdr2 = apollo_read_reg(reg_GPDR2);
      apollo_write_reg(reg_GPDR1, (gpdr1 & 0x03ffffff)); /*clear upper 6 bits*/
      apollo_write_reg(reg_GPDR2, (gpdr2 & 0xfffffffc)); /*clear lower 2 bits*/  
      
      apollo_write_reg(reg_GPSR2, 1 << 5);
      break;
  }

  /* write H_D[7:0] */
  switch (which) {
    case APOLLO_WRITE_COMMAND:
    case APOLLO_WRITE_DATA:
      apollo_write_reg(reg_GPSR1, gpsr1);
      apollo_write_reg(reg_GPSR2, gpsr2);
      apollo_write_reg(reg_GPCR1, gpcr1);
      apollo_write_reg(reg_GPCR2, gpcr2);
      apollo_write_reg(reg_GPCR2, 1 << 2);
      break;
  }
  
  /* set H_DS low, wait for H_ACK to go low */
  apollo_write_reg(reg_GPCR2, 1 << 2);
  wait_for_ack(APOLLO_ACK);
  
  /* read H_D[7:0]  */
  switch (which) {
    case APOLLO_READ:
      gplr1 = apollo_read_reg(reg_GPLR1);
      gplr2 = apollo_read_reg(reg_GPLR2);

      gplr1 = (gplr1 & 0xfc000000) >> 26;
      gplr2 = (gplr2 & 0x00000003) << 6;
      rdata = gplr2 | gplr1;
      break;
  }
  
  /* set H_DS high */
  apollo_write_reg(reg_GPSR2, 1 << 2);
  
  /* unprepare H_D[7:0] for reads */
  /* set H_RW low */
  switch (which) {
    case APOLLO_READ:
      gpdr1 = apollo_read_reg(reg_GPDR1);
      gpdr2 = apollo_read_reg(reg_GPDR2);
      apollo_write_reg(reg_GPDR1, (gpdr1 | 0xfc000000)); /*set upper 6 bits*/
      apollo_write_reg(reg_GPDR2, (gpdr2 | 0x00000003)); /*set lower 2 bits*/  
      
      apollo_write_reg(reg_GPCR2, 1 << 5);
      break;
  }

  /* wait for H_ACK to go high */
  wait_for_ack(APOLLO_NACK);

  if (!FB_INITED()) {
    udelay(APOLLO_SEND_DELAY);
  }

  return rdata;
}

unsigned char apollo_get_version(void) {
  apollo_write_command(APOLLO_GET_VERSION_CMD);
  return apollo_read();
}

void apollo_send_command(apollo_command_t *apollo_command)
{
  if (apollo_command) {
    apollo_preprocess_command_t apollo_preprocess_command = { 0 };
    int i, n;

    // Do any controller-specific pre-command processing.
    //
    (*apollo_controller_preprocess_command)(&apollo_preprocess_command, apollo_command);

    // Send command.
    //
    apollo_write_command(apollo_command->command);
    do_debug("command = 0x%02X\n", apollo_command->command);

    // Delay if necessary.
    //
    if (APOLLO_WRITE_NEEDS_DELAY == apollo_command->type) {
      udelay(apollo_command->io);
    }

    // Send args if there are any or we aren't skipping them.
    //
    if (!apollo_preprocess_command.skip_args && apollo_command->num_args) {
      n = apollo_command->num_args;
      
      for (i = 0; i < n; i++) {
        apollo_write_data(apollo_command->args[i]);
        do_debug("data[%d] = 0x%02X\n", i, apollo_command->args[i]);
      }
    }

    // Send (image) data if there is any, preceded by a header if there
    // is one.
    //
    if (apollo_preprocess_command.header_size) {
      n = apollo_preprocess_command.header_size;

      for (i = 0; i < n; i++) {
        apollo_write_data(apollo_preprocess_command.header[i]);
      }
    }
    
    if (apollo_command->buffer_size && apollo_command->buffer) {
      u8 data, white = 0;

      n = apollo_command->buffer_size;
      
      for (i = 0; i < n; i++) {
        white |= apollo_command->buffer[i];
        
        if (apollo_preprocess_command.transform_data) {
          data = (*apollo_preprocess_command.transform_data)(apollo_command->buffer, i,
            apollo_preprocess_command.xres, apollo_preprocess_command.yres);
        } else {
          data = apollo_command->buffer[i];
        }

        apollo_write_data(data);
      }
      
      // Return whether the buffer was cleared to white or not.
      //
      apollo_command->io = (0 == white) ? 1 : 0;

      // If data was image data, say that we're now done sending it.
      //
      if (IS_LOAD_IMAGE_CMD(apollo_command->command)) {
        apollo_write_command(APOLLO_STOP_IMGLD_CMD);
        do_debug("command = 0x%02X\n", APOLLO_STOP_IMGLD_CMD);
      }
    }
    
    // Return results if we're supposed to.
    //
    if (APOLLO_READ == apollo_command->type) {
      apollo_command->io = apollo_read();
      do_debug("result  = 0x%02X\n", apollo_command->io);
    }
    
    // Do any controller-specific post-command processing.
    //
    (*apollo_controller_postprocess_command)(apollo_command);
  }
}

u8 apollo_simple_send_command(u8 command, u8 type) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command = command;
  apollo_command.type = type;
  apollo_send_command(&apollo_command);
 
  return apollo_command.io;
}

int apollo_get_status(apollo_status_t *status) {
  u8 local_status = apollo_simple_get(APOLLO_GET_STATUS_CMD);
  int status_valid = 0;
  
  if (status) {
    memcpy(status, &local_status, sizeof(apollo_status_t));
    status_valid = 1;
  }
  
  return status_valid;
}

unsigned char apollo_get_temperature(void) {
  return apollo_simple_get(APOLLO_GET_TEMPERATURE_CMD);
}

unsigned char apollo_get_init_display_speed(void) {
  unsigned long init_display_flag = get_apollo_init_display_flag();
  unsigned char init_display_speed;
  
  switch (init_display_flag) {
    case INIT_DISPLAY_FAST:
      init_display_speed = APOLLO_INIT_DISPLAY_FAST;
      break;
      
    case INIT_DISPLAY_SKIP:
      init_display_speed = APOLLO_INIT_DISPLAY_SKIP;
      break;
    
    case INIT_DISPLAY_WAKE:
      init_display_speed = APOLLO_INIT_DISPLAY_WAKE;
      break;
    
    case INIT_DISPLAY_SLOW:
    default:
      init_display_speed = APOLLO_INIT_DISPLAY_SLOW;
      break;
  }

  return init_display_speed;
}

void apollo_set_init_display_speed(unsigned char init_display_speed) {
  unsigned long init_display_flag;
  
  switch (init_display_speed) {
    case APOLLO_INIT_DISPLAY_FAST:
      init_display_flag = INIT_DISPLAY_FAST;
      break;
      
    case APOLLO_INIT_DISPLAY_SKIP:
      init_display_flag = INIT_DISPLAY_SKIP;
      break;
      
    case APOLLO_INIT_DISPLAY_WAKE:
      init_display_flag = INIT_DISPLAY_WAKE;
      break;
      
    case APOLLO_INIT_DISPLAY_SLOW:
    default:
      init_display_flag = INIT_DISPLAY_SLOW;
      break;
  }
  
  set_apollo_init_display_flag(init_display_flag);
}

void apollo_init_display(unsigned char init_display_speed) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command  = APOLLO_INIT_DISPLAY_CMD;
  apollo_command.type     = APOLLO_WRITE_ONLY;
  apollo_command.num_args = 1;
  apollo_command.args[0]  = init_display_speed;
  
  apollo_send_command(&apollo_command);
}

static u32 apollo_load_data(unsigned char cmd, unsigned char *buffer, int bufsize,
  u16 x1, u16 y1, u16 x2, u16 y2) {
  apollo_command_t apollo_command = { 0 };
  
  if (APOLLO_LOAD_PARTIAL_IMAGE_CMD == cmd) {
      apollo_command.num_args = 8;
      
      apollo_command.args[0]  = (x1 >> 8) & 0xff;
      apollo_command.args[1]  = x1 & 0xff;
      
      apollo_command.args[2]  = (y1 >> 8) & 0xff;
      apollo_command.args[3]  = y1 & 0xff;
      
      apollo_command.args[4]  = (x2 >> 8) & 0xff;
      apollo_command.args[5]  = x2 & 0xff;
      
      apollo_command.args[6]  = (y2 >> 8) & 0xff;
      apollo_command.args[7]  = y2 & 0xff;
  }
  
  apollo_command.command      = cmd;
  apollo_command.type         = APOLLO_WRITE_ONLY;
  apollo_command.buffer_size  = bufsize;
  apollo_command.buffer       = buffer;
  
  apollo_send_command(&apollo_command);

  return apollo_command.io;
}

u32 apollo_load_image_data(unsigned char *buffer, int bufsize) {
  return apollo_load_data(APOLLO_LOAD_IMAGE_CMD, buffer, bufsize, 0, 0, 0, 0);
}

u32 apollo_load_partial_data(unsigned char *buffer, int bufsize, 
  u16 x1, u16 y1, u16 x2, u16 y2) {
  return apollo_load_data(APOLLO_LOAD_PARTIAL_IMAGE_CMD, buffer, bufsize, x1, y1, --x2, --y2);
}

void apollo_display(int full) {
  if (full) {
    apollo_simple_set(APOLLO_AUTO_REFRESH_CMD);         // Workaround for PVI-6001A.
    apollo_simple_set(APOLLO_MANUAL_REFRESH_CMD);
  }

  apollo_simple_set(APOLLO_DISP_IMAGE_CMD);
  
  if (full) {
    apollo_simple_set(APOLLO_CANCEL_AUTO_REFRESH_CMD);  // Workaround for PVI-6001A.
  }
}

void apollo_display_partial(void) {
  apollo_simple_set(APOLLO_DISP_PARTIAL_IMAGE_CMD);
}

void apollo_restore(void) {
  apollo_simple_set(APOLLO_RESTORE_IMAGE_CMD);
}

void apollo_erase_screen(unsigned char color) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command  = APOLLO_ERASE_SCRN_CMD;
  apollo_command.type     = APOLLO_WRITE_ONLY;
  apollo_command.num_args = 1;
  apollo_command.args[0]  = color;
  
  apollo_send_command(&apollo_command);
}

void apollo_clear_screen(int full) {
  if (full) {
	  apollo_erase_screen(APOLLO_ERASE_SCREEN_BLACK);
	}
	
	apollo_erase_screen(APOLLO_ERASE_SCREEN_WHITE);
}

void apollo_temp_sensor_init(void) {
  if (needs_to_enable_temp_sensor()) {
    pxa_gpio_mode(GPIO_FIONA_TEMP_SENSOR_MD);
    pxa_gpio_mode(GPIO_FIONA_APOLLO_COMMON_MD);
    pxa_gpio_mode(GPIO_FIONA_APOLLO_POWER_MD);
  }
}

void apollo_power_up(void) {
  if (needs_to_enable_temp_sensor()) {
    GPCR(GPIO_FIONA_TEMP_SENSOR) = GPIO_bit(GPIO_FIONA_TEMP_SENSOR);
  }
    
  GPSR(GPIO_FIONA_APOLLO_COMMON) = GPIO_bit(GPIO_FIONA_APOLLO_COMMON);
  GPSR(GPIO_FIONA_APOLLO_POWER)	 = GPIO_bit(GPIO_FIONA_APOLLO_POWER);

  apollo_temp_sensor_init();
  
  GPSR(68) = GPIO_bit(68);
  GPCR(69) = GPIO_bit(69);
  GPCR(70) = GPIO_bit(70);
  
  // Workaround for PVI-6001A.
  //
  GPDR(75) &= ~GPIO_bit(75);
  GPSR(75) = GPIO_bit(75);
  
  GPDR(83) &= ~GPIO_bit(83);
  GPSR(83) = GPIO_bit(83);

  GAFR(58) &= ~GAFR_bit(58);
  GAFR(59) &= ~GAFR_bit(59);
  GAFR(60) &= ~GAFR_bit(60);
  GAFR(61) &= ~GAFR_bit(61);
  GAFR(62) &= ~GAFR_bit(62);
  GAFR(63) &= ~GAFR_bit(63);
  GAFR(64) &= ~GAFR_bit(64);
  GAFR(65) &= ~GAFR_bit(65);
  GAFR(66) &= ~GAFR_bit(66);
  GAFR(67) &= ~GAFR_bit(67);
  GAFR(68) &= ~GAFR_bit(68);
  GAFR(69) &= ~GAFR_bit(69);
  GAFR(70) &= ~GAFR_bit(70);

  GPDR(58) |= GPIO_bit(58);
  GPDR(59) |= GPIO_bit(59);
  GPDR(60) |= GPIO_bit(60);
  GPDR(61) |= GPIO_bit(61);
  GPDR(62) |= GPIO_bit(62);
  GPDR(63) |= GPIO_bit(63);
  GPDR(64) |= GPIO_bit(64);
  GPDR(65) |= GPIO_bit(65);
  GPDR(66) |= GPIO_bit(66);
  GPDR(67) |= GPIO_bit(67);
  GPDR(68) |= GPIO_bit(68);
  GPDR(69) |= GPIO_bit(69);
  GPDR(70) |= GPIO_bit(70);
  
  apollo_comm_init();

  // Workaround for PVI-6001A.
  //
  if (0 == ((apollo_read_reg(reg_GPLR2) & 0x00000800) >> 11)) {
    panel_id_t *panel_id = get_panel_id();
    panel_id->apollo_is_pvi = 1;
  }
}

void apollo_power_down(int full) {
  if (full) {
    apollo_comm_shutdown();
  }
  
  if (needs_to_enable_temp_sensor()) {
    GPSR(GPIO_FIONA_TEMP_SENSOR) = GPIO_bit(GPIO_FIONA_TEMP_SENSOR);
  }

  GPCR(GPIO_FIONA_APOLLO_COMMON) = GPIO_bit(GPIO_FIONA_APOLLO_COMMON);
  GPCR(GPIO_FIONA_APOLLO_POWER)  = GPIO_bit(GPIO_FIONA_APOLLO_POWER);
}

void apollo_init(unsigned long bpp) {
  unsigned char init_display_speed = apollo_get_init_display_speed();
  apollo_command_t apollo_command = { 0 };
  
  // Workaround for PVI-6001A.
  //
  if (apollo_standby) {
    switch (init_display_speed) {
      case APOLLO_INIT_DISPLAY_SKIP:
      case APOLLO_INIT_DISPLAY_WAKE:
        init_display_speed = APOLLO_INIT_DISPLAY_FAST;
        break;
    }
  }
  
  // Waking temporarily from kernel?  Yes, skip everything.
  //
  if (APOLLO_INIT_DISPLAY_SKIP != init_display_speed) {
    
    // Waking from bootloader?  Yes, just wake.
    //
    if (APOLLO_INIT_DISPLAY_WAKE == init_display_speed) {
      apollo_simple_set(APOLLO_NORMAL_MODE_CMD);
    
    } else {

      // Otherwise, go through normal init sequence.
      //
      apollo_command.command  = APOLLO_RESET_CMD;
      apollo_command.type     = APOLLO_WRITE_NEEDS_DELAY;
      apollo_command.io       = APOLLO_RESET_CMD_DELAY;
      apollo_send_command(&apollo_command);

      apollo_write_register(APOLLO_OFF_DELAY_REGISTER, APOLLO_OFF_DELAY_DEFAULT);
      apollo_write_register(APOLLO_T1_DELAY_REGISTER,  APOLLO_T1_DELAY_DEFAULT);

      // Workarounds for PVI-6001A.
      //
      switch (init_display_speed) {
        case APOLLO_INIT_DISPLAY_FAST:
          if (apollo_standby && get_screen_clear()) {
            goto init_display;
          }
          break;
        
        init_display:
        case APOLLO_INIT_DISPLAY_SLOW:
          apollo_init_display(init_display_speed);
          break;
      }
      apollo_standby = 0;
      
      apollo_simple_set(APOLLO_CANCEL_AUTO_REFRESH_CMD);

      // Back to normal init sequence.
      //
      apollo_set_orientation(APOLLO_ORIENTATION_270);
    
      apollo_command.command  = APOLLO_SET_DEPTH_CMD;
      apollo_command.type     = APOLLO_WRITE_ONLY;
      apollo_command.num_args = 1;
      switch (bpp) {
        case EINK_DEPTH_1BPP:
          apollo_command.args[0] = APOLLO_DEPTH_1BPP;
          break;
    
        case EINK_DEPTH_2BPP:
        default:
          apollo_command.args[0] = APOLLO_DEPTH_2BPP;
          break;
      }
      apollo_send_command(&apollo_command);
      
      apollo_simple_set(INVERT_VIDEO() ? APOLLO_POSITIVE_PICTURE_CMD : APOLLO_NEGATIVE_PICTURE_CMD);

      // Say that we're init'ed.
      //
      set_fb_init_flag(BOOT_GLOBALS_FB_INIT_FLAG);
    }
  }
}

void apollo_write_register(unsigned char regAddress, unsigned char data) {
  apollo_command_t apollo_command = { 0 };

  apollo_command.command  = APOLLO_WRITE_REGISTER;
  apollo_command.type     = APOLLO_WRITE_ONLY;
  apollo_command.num_args = 2;
  apollo_command.args[0]  = regAddress;
  apollo_command.args[1]  = data;
  
  apollo_send_command(&apollo_command);
}

void apollo_read_register(unsigned char regAddress, unsigned char *data) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command  = APOLLO_READ_REGISTER;
  apollo_command.type     = APOLLO_READ;
  apollo_command.num_args = 1;
  apollo_command.args[0]  = regAddress;
  
  apollo_send_command(&apollo_command);
  *data = apollo_command.io;
}

int apollo_read_from_flash_byte(unsigned long addr, unsigned char *data) {
  apollo_command_t apollo_command = { 0 };
  
  apollo_command.command  = APOLLO_FLASH_READ;
  apollo_command.type     = APOLLO_READ;
  apollo_command.num_args = 3;
  apollo_command.args[0]  = (u8)((addr & 0x00FF0000) >> 16);
  apollo_command.args[1]  = (u8)((addr & 0x0000FF00) >> 8);
  apollo_command.args[2]  = (u8)((addr & 0x000000FF));
  
  apollo_send_command(&apollo_command);
  *data = apollo_command.io;
  
  return 1;
}

int apollo_read_from_flash_short(unsigned long addr, unsigned short *data) {
  unsigned char byte;
  int i;
  
  for (i = 0, *data = 0; i < sizeof(unsigned short); i++) {
    apollo_read_from_flash_byte((addr + i), &byte);
    *data |= (byte << (i * 8));
  }
  
  return 1;
}

int apollo_read_from_flash_long(unsigned long addr, unsigned long *data) {
  unsigned char byte;
  int i;

  for (i = 0, *data = 0; i < sizeof(unsigned long); i++) {
    apollo_read_from_flash_byte((addr + i), &byte);
    *data |= (byte << (i * 8));
  }
  
  return 1;
}

void apollo_get_temperature_info(apollo_temperature_t *temperature) {
  if (temperature) {
    apollo_info_t info; apollo_get_info(&info);
    
    temperature->min = info.waveform_temp_min;
    temperature->max = info.waveform_temp_max;
    temperature->inc = info.waveform_temp_inc;
    temperature->cur = apollo_get_temperature();
    
    temperature->valid = !LEGACY_WAVEFORM_DATA(info.waveform_temp_min);
  }
}

void apollo_override_fpl(apollo_fpl_t *fpl) {
  panel_id_t *panel_id = get_panel_id();

  if ((panel_id && panel_id->override) && fpl) {
    fpl->platform            = panel_id->platform;
    fpl->lot                 = panel_id->lot;
    fpl->adhesive_run_number = panel_id->adhesive_run_number;
    
    fpl->overridden          = 1;
  }
}

u8 apollo_get_orientation(void) {
    return apollo_orientation;
}

void apollo_set_orientation(u8 orientation) {
  int set_orientation = 0;
  
  switch(orientation) {
    case APOLLO_ORIENTATION_0:
    case APOLLO_ORIENTATION_90:
    case APOLLO_ORIENTATION_180:
    case APOLLO_ORIENTATION_270:
      set_orientation = 1;
      break;
  }

  if (set_orientation) {
    apollo_command_t apollo_command = { 0 };
    
    apollo_command.command  = APOLLO_SET_ORIENTATION_CMD;
    apollo_command.type     = APOLLO_WRITE_ONLY;
    apollo_command.num_args = 1;
    apollo_command.args[0]  = orientation;
    apollo_send_command(&apollo_command);
    
    apollo_orientation = orientation;
  }
}
