#include "apollo.h"

static void universal_preprocess_command(apollo_preprocess_command_t *apollo_preprocess_command,
  apollo_command_t *apollo_command);

static void universal_postprocess_command(apollo_command_t *apollo_command);

#define APOLLO_UNIVERSAL_BUILD
#include "apollo.c"

extern int apollo_standby; // Workaround for PVI-6001A.
static apollo_t apollo_type = apollo_unknown;

static u8 apollo_nops[] = {
  STEPHANIE_DISP_IMAGE_CMD,
  STEPHANIE_DRAW_LINE_CMD,
  STEPHANIE_GET_STATUS_CMD,
  
  STEPHANIE_POWER_ON_CMD,
  STEPHANIE_POWER_OFF_CMD,

  STEPHANIE_SOFTWARE_VERSION_CMD,
  STEPHANIE_DISPLAY_SIZE_CMD,
  
  APOLLO_NOP
};

static u8 stephanie_nops[] = {
  APOLLO_FLASH_WRITE,
  APOLLO_FLASH_READ,
  
  APOLLO_WRITE_REGISTER,
  APOLLO_READ_REGISTER,
  
  APOLLO_NORMAL_MODE_CMD,
  APOLLO_SLEEP_MODE_CMD,
  APOLLO_STANDBY_MODE_CMD,
  APOLLO_SET_DEPTH_CMD,
  APOLLO_SET_ORIENTATION_CMD,
  APOLLO_POSITIVE_PICTURE_CMD,
  APOLLO_NEGATIVE_PICTURE_CMD,
  APOLLO_AUTO_REFRESH_CMD,
  APOLLO_CANCEL_AUTO_REFRESH_CMD,
  APOLLO_SET_REFRESH_TIMER_CMD,
  APOLLO_MANUAL_REFRESH_CMD,
  APOLLO_READ_REFRESH_TIMER_CMD,
  
  APOLLO_NOP
};

static u8 stephanie_no_args[] = {
  APOLLO_ERASE_SCRN_CMD,
  APOLLO_INIT_DISPLAY_CMD,
  
  APOLLO_NOP
};

#define STEPHANIE_BUFF_SIZE ((STEPHANIE_X_RES * STEPHANIE_Y_RES * EINK_DEPTH_4BPP)/8)
#define STEPHANIE_REGS_SIZE 1072

static u8 stephanie_regs[STEPHANIE_REGS_SIZE] = { 0 };
static u8 stephanie_buff[STEPHANIE_BUFF_SIZE] = { 0 };

static void stephanie_get_status(stephanie_status_t *status) {
  u8 local_status;
  
  apollo_write_command(APOLLO_GET_STATUS_CMD);
  local_status = apollo_read();

  memcpy(status, &local_status, sizeof(stephanie_status_t));
}

static void stephanie_wait_command_complete(void) {
  stephanie_status_t status;
  int wait = 1;
  
  while (wait) {
    stephanie_get_status(&status);
    
    if (!status.sys_busy) {
      wait = 0;
    }
  }
}

#define HI_NYBBLE(i, x, y)  ((((x) * 2) * (i % ((y) / 2))) + (i / (y)))
#define LO_NYBBLE(i, x, y)  (HI_NYBBLE(i, x, y) + x)

#define SWITCH_NYBBLE(i, y) (0 == ((i % ((y) / 2))) ? 1 : 0)

static int stephanie_transform_which_nybble = 0;

static u8 stephanie_transform_data(u8 *buffer, int i, int xres, int yres) {
  int rowbytes = (xres * EINK_DEPTH_4BPP)/8;
  u8 hi_nybble, lo_nybble, data;

  // Swap which nybble we're grabbing on a row-by-row basis.
  //
  if (0 == i) {
    stephanie_transform_which_nybble = 1;
  } else if (SWITCH_NYBBLE(i, yres)) {
    stephanie_transform_which_nybble = !stephanie_transform_which_nybble;
  }

  // Stephanie is natively 800x600@4bpp, and we must rotate the buffer
  // from its 600x800@4bpp geometry.
  //
  hi_nybble = buffer[HI_NYBBLE(i, rowbytes, yres)];
  lo_nybble = buffer[LO_NYBBLE(i, rowbytes, yres)];
  
  if (stephanie_transform_which_nybble) {
    data = (0xF0 & hi_nybble) | ((0xF0 & lo_nybble) >> 4);
  } else {
    data = ((0x0F & hi_nybble) << 4) | (0x0F & lo_nybble);
  }
  
  // Stephanie is inverted from Apollo.
  //
  if (!INVERT_VIDEO()) {
    data = ~data;
  }

  return data;
}

static void stephanie_stretch_buffer(apollo_command_t *apollo_command) {
  unsigned long bpp = eink_fb_get_bpp();

  // Don't stretch if we're already at 4bpp.
  //
  if (EINK_DEPTH_4BPP != bpp) {
    int i, j, n = apollo_command->buffer_size;
    u8 *src = apollo_command->buffer,
       *dst = stephanie_buff,
       a, b;
    
    // Stretch the 1bpp & 2bpp buffers to 4bpp for Stephanie.
    //
    for (i = 0, j = 0; i < n; i++) {
      switch (bpp) {
        case EINK_DEPTH_1BPP:
          a = STRETCH_HI_NYBBLE(src[i], EINK_DEPTH_1BPP);
          b = STRETCH_LO_NYBBLE(src[i], EINK_DEPTH_1BPP);
          
          dst[j++] = STRETCH_HI_NYBBLE(a, EINK_DEPTH_2BPP);
          dst[j++] = STRETCH_LO_NYBBLE(a, EINK_DEPTH_2BPP);
          dst[j++] = STRETCH_HI_NYBBLE(b, EINK_DEPTH_2BPP);
          dst[j++] = STRETCH_LO_NYBBLE(b, EINK_DEPTH_2BPP);
          break;
        
        case EINK_DEPTH_2BPP:
          dst[j++] = STRETCH_HI_NYBBLE(src[i], EINK_DEPTH_2BPP);
          dst[j++] = STRETCH_LO_NYBBLE(src[i], EINK_DEPTH_2BPP);
          break;
      }
    }
    
    // Replace src buffer with dst.
    //
    apollo_command->buffer_size = j;
    apollo_command->buffer = dst;
  }
}

static int item_inlist(u8 item, u8 *list) {
  int result = 0;
  
  if (list) {
    while (*list && !result) {
      if (item == *list) {
        result = 1;
      } else {
        list++;
      }
    }
  }

  return result;
}

static void universal_preprocess_command(apollo_preprocess_command_t *apollo_preprocess_command,
  apollo_command_t *apollo_command)
{
  u8 *nops_list = NULL;
  
  if (apollo_unknown == apollo_type) {
    apollo_type = (apollo_t)apollo_get_version();
  }
  
  // Get NOP filter based on the type of Apollo we have.
  //
  switch (apollo_type) {
    case apollo_stephanie:
      nops_list = stephanie_nops;
      break;
      
    default:
      nops_list = apollo_nops;
      break;
  }
  
  // Filter NOP commands out.
  //
  if (nops_list && !item_inlist(apollo_command->command, nops_list)) {
    switch (apollo_type) {
      case apollo_stephanie:
        if (apollo_command->num_args && item_inlist(apollo_command->command, stephanie_no_args)) {
          apollo_preprocess_command->skip_args = 1;
        }

        if (IS_LOAD_IMAGE_CMD(apollo_command->command)) {
          u16 x1, x2, y1, y2;
          switch (apollo_command->command) {
            
            // In the full-screen case, need to send the header.
            //
            case APOLLO_LOAD_IMAGE_CMD:
              apollo_preprocess_command->header_size = STEPHANIE_REGS_SIZE;
              apollo_preprocess_command->header = stephanie_regs;

              apollo_preprocess_command->xres = APOLLO_X_RES;
              apollo_preprocess_command->yres = APOLLO_Y_RES;
              break;
            
            // Need to save the original resolution in the partial-screen case for the
            // rotation, and need to flip the coordinates for the rotation.
            //
            case APOLLO_LOAD_PARTIAL_IMAGE_CMD:
              x1 = ((apollo_command->args[0] & 0x00FF) << 8) | (apollo_command->args[1] & 0x00FF);
              y1 = ((apollo_command->args[2] & 0x00FF) << 8) | (apollo_command->args[3] & 0x00FF);
              x2 = ((apollo_command->args[4] & 0x00FF) << 8) | (apollo_command->args[5] & 0x00FF);
              y2 = ((apollo_command->args[6] & 0x00FF) << 8) | (apollo_command->args[7] & 0x00FF);
              
              apollo_preprocess_command->xres = (x2 - x1) + 1;
              apollo_preprocess_command->yres = (y2 - y1) + 1;
              
              apollo_command->args[0] = (y1 >> 8) & 0xFF;
              apollo_command->args[1] = y1 & 0xFF;
              apollo_command->args[2] = (x1 >> 8) & 0xFF;
              apollo_command->args[3] = x1 & 0xFF;
              apollo_command->args[4] = (y2 >> 8) & 0xFF;
              apollo_command->args[5] = y2 & 0xFF;
              apollo_command->args[6] = (x2 >> 8) & 0xFF;
              apollo_command->args[7] = x2 & 0xFF;
              break;
          }
          
          apollo_preprocess_command->transform_data = stephanie_transform_data;
          stephanie_stretch_buffer(apollo_command);
        }
        break;
        
      default:
        break;
    }
  }
}

static void universal_postprocess_command(apollo_command_t *apollo_command)
{
  switch (apollo_type) {
    case apollo_stephanie:
      stephanie_wait_command_complete();
      
      // Stephanie returns the temperature in 0.25C increments.
      //
      if (APOLLO_GET_TEMPERATURE_CMD == apollo_command->command) {
        apollo_command->io /= 4;
      }
      
      if (APOLLO_GET_STATUS_CMD == apollo_command->command) {
        apollo_status_t apollo_status = { 0 };
        stephanie_status_t stephanie_status;
        
        // Get Stephanie's status.
        //
        memcpy(&stephanie_status, &apollo_command->io, sizeof(stephanie_status_t));
        
        // For compatibility with Apollo, say we're in "normal" mode if we're not busy; otherwise,
        // say we're sleeping.
        //
        if (stephanie_status.cmd_ready && stephanie_status.cmd_finished && !stephanie_status.sys_busy) {
          apollo_status.operation_mode = 0;
        } else {
          apollo_status.operation_mode = 1;
        }
        
        // For compatibility with Apollo, say we're in 2bpp if we're not in 1bpp.
        //
        if (EINK_DEPTH_1BPP == eink_fb_get_bpp()) {
          apollo_status.bpp = 0;
        } else {
          apollo_status.bpp = 1;
        }
        
        // Return status for Apollo.
        //
        memcpy(&apollo_command->io, &apollo_status, sizeof(apollo_status_t));
      }
      break;
    
    // Workaround for PVI-6001A.
    //
    case apollo_apollo:
        if (APOLLO_STANDBY_MODE_CMD == apollo_command->command) {
          apollo_standby = 1;
        }
      break;
    
    default:
      break;
  }
}
