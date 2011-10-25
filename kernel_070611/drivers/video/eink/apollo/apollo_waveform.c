#define waveform_version_string_max 32
#define waveform_seperator          "_"

enum run_types
{
  bl, b, t, p, unknown_run_type, num_run_types
};
typedef enum run_types run_types;

#define skip_legacy_run_types       b

enum wf_types
{
  wx, wy, wp, wz, wq, ta, wu, unknown_wf_type, num_wf_types
};
typedef enum wf_types wf_types;

enum platforms
{
  matrix_2_0, matrix_2_1, matrix_2_3, matrix_Vizplex_110, unknown_platform, num_platforms
};
typedef enum platforms platforms;

static char waveform_version_string[waveform_version_string_max];

static char *run_type_names[num_run_types] =
{
  "BL", "B", "T", "P", "?"
};

static char *wf_type_names[num_wf_types] =
{
  "WX", "WY", "WP", "WZ", "WQ", "TA", "WU", "??"
};

static char *platform_names[num_platforms] =
{
  "20", "21", "23", "V110", "??"
};

#define has_valid_serial_number(r, s) \
  (((b == r) || (p == r)) && (0 != s))

void apollo_get_info(apollo_info_t *info) {
  if (info) {
    unsigned long  serial_number;
    unsigned short fpl_lot_bin;
    
    apollo_read_from_flash_byte(EINK_ADDR_WAVEFORM_VERSION,    &info->waveform_version);
    apollo_read_from_flash_byte(EINK_ADDR_WAVEFORM_SUBVERSION, &info->waveform_subversion);
    apollo_read_from_flash_byte(EINK_ADDR_WAVEFORM_TYPE,       &info->waveform_type);
    apollo_read_from_flash_byte(EINK_ADDR_FPL_LOT_BCD,         &info->fpl_lot_bcd);
    apollo_read_from_flash_byte(EINK_ADDR_RUN_TYPE,            &info->run_type);
    apollo_read_from_flash_byte(EINK_ADDR_FPL_PLATFORM,        &info->fpl_platform);
    apollo_read_from_flash_byte(EINK_ADDR_ADHESIVE_RUN_NUM,    &info->adhesive_run_number);
    apollo_read_from_flash_byte(EINK_ADDR_MIN_TEMP,            &info->waveform_temp_min);
    apollo_read_from_flash_byte(EINK_ADDR_MAX_TEMP,            &info->waveform_temp_max);
    apollo_read_from_flash_byte(EINK_ADDR_TEMP_STEPS,          &info->waveform_temp_inc);

    apollo_read_from_flash_short(EINK_ADDR_FPL_LOT_BIN,        &fpl_lot_bin);

    apollo_read_from_flash_long(EINK_ADDR_MFG_DATA_DEVICE,     &info->mfg_data_device);
    apollo_read_from_flash_long(EINK_ADDR_MFG_DATA_DISPLAY,    &info->mfg_data_display);
    apollo_read_from_flash_long(EINK_ADDR_SERIAL_NUMBER,       &serial_number);
    apollo_read_from_flash_long(EINK_ADDR_CHECKSUM,            &info->checksum);
    
    info->fpl_lot_bin   = ((fpl_lot_bin & 0xFF00) >> 8) | ((fpl_lot_bin & 0x00FF) << 8);
    info->serial_number = ((serial_number & 0xFF000000) >> 24) |
                          ((serial_number & 0x00FF0000) >>  8) |
                          ((serial_number & 0x0000FF00) <<  8) |
                          ((serial_number & 0x000000FF) << 24);
    
    info->last_temperature   = apollo_get_temperature();
    info->controller_version = apollo_get_version();
    
    // Translate initial legacy-looking 2.3 waveform version data into
    // a format that makes sense for the 2.3 panels.
    //
    if (LEGACY_WAVEFORM_DATA(info->run_type)) {
      if ((EINK_INIT_23_WF_VERSION     == info->waveform_version)     &&
          (EINK_INIT_23_WF_SUBVERSION  == info->waveform_subversion)  &&
          (EINK_INIT_23_WF_TYPE        == info->waveform_type)        &&
          (EINK_INIT_23_WF_FPL_LOT_BCD == info->fpl_lot_bcd)) {
            info->run_type             =  EINK_INIT_23_RUN_TYPE;
            info->fpl_platform         =  EINK_INIT_23_FPL_PLATFORM;
            info->fpl_lot_bin          =  EINK_INIT_23_FPL_LOT_BIN;
            info->adhesive_run_number  =  EINK_INIT_23_ADHESIVE_RUN_NUM;
      }
    }
  }
}

void apollo_get_waveform_version(apollo_waveform_t *waveform) {
  if (waveform) {
    apollo_info_t info; apollo_get_info(&info);
    
    waveform->version       = info.waveform_version;
    waveform->subversion    = info.waveform_subversion;
    waveform->type          = info.waveform_type;
    waveform->fpl_lot       = info.fpl_lot_bcd;
    waveform->run_type      = info.run_type;
    waveform->serial_number = info.serial_number;
  }
}

void apollo_get_fpl_version(apollo_fpl_t *fpl) {
  if (fpl) {
    fpl->overridden = 0; apollo_override_fpl(fpl);
    
    if (!fpl->overridden) {
      apollo_info_t info; apollo_get_info(&info);
        
      if (LEGACY_WAVEFORM_DATA(info.run_type)) {
        fpl->platform            = matrix_2_1;
        fpl->lot                 = (((info.fpl_lot_bcd & 0xF0) >> 4) * 10) + (info.fpl_lot_bcd & 0x0F);
        fpl->adhesive_run_number = 0;
      } else {
        fpl->platform            = info.fpl_platform;
        fpl->lot                 = info.fpl_lot_bin;
        fpl->adhesive_run_number = (matrix_Vizplex_110 == fpl->platform) ? 0 : info.adhesive_run_number;
      }
    }
  }
}

char *apollo_get_waveform_version_string(void) {
  char temp_string[waveform_version_string_max];
  apollo_waveform_t waveform;
  run_types run_type;
  apollo_fpl_t fpl;
  
  // Get the waveform version info and clear the waveform
  // version string.
  //
  apollo_get_waveform_version(&waveform);
  apollo_get_fpl_version(&fpl);
  
  waveform_version_string[0] = '\0';
  run_type = waveform.run_type;

  // Build up a waveform version string of one of two forms:
  //
  //    BL<FPL LOT>_<WF TYPE><WF VERSION>_<WF SUBVERSION>
  //
  // or
  //
  //    <RUN TYPE><FPL_LOT>_<ADHESIVE RUN NUMBER>_<FPL PLATFORM>_
  //    <WF TYPE><WF VERSION><WF SUBVERSION>
  //
  if (LEGACY_WAVEFORM_DATA(run_type)) {
    // BL
    strcat(waveform_version_string, run_type_names[bl]);
    
    // FPL LOT
    //
    sprintf(temp_string, "%02X", waveform.fpl_lot);
    strcat(waveform_version_string, temp_string);
  
  } else {
    run_type += skip_legacy_run_types;
  
    // RUN TYPE
    //
    switch (run_type) {
      case b:
      case t:
      case p:
        strcat(waveform_version_string, run_type_names[run_type]);
        break;
        
      case unknown_run_type:
      case bl:
      default:
        strcat(waveform_version_string, run_type_names[unknown_run_type]);
        break;
    }
    
    // FPL LOT, ADHESIVE RUN NUMBER, FPL PLATFORM
    //
    apollo_get_panel_id_str_from_fpl(temp_string, &fpl);
    strcat(waveform_version_string, temp_string);
  }
  strcat(waveform_version_string, waveform_seperator);

  // WF TYPE
  //
  switch (waveform.type) {
    case wx:
    case wy:
    case wp:
    case wz:
    case wq:
    case ta:
    case wu:
      strcat(waveform_version_string, wf_type_names[waveform.type]);
      break;
      
    case unknown_wf_type:
    default:
      strcat(waveform_version_string, wf_type_names[unknown_wf_type]);
      break;
  }
  
  // WF VERSION
  //
  sprintf(temp_string, "%02X", waveform.version);
  strcat(waveform_version_string, temp_string);
  
  if (LEGACY_WAVEFORM_DATA(waveform.run_type)) {
    strcat(waveform_version_string, waveform_seperator);
  }
  
  // WF SUBVERSION
  //
  sprintf(temp_string, "%02X", waveform.subversion);
  strcat(waveform_version_string, temp_string);
  
  // SERIAL NUMBER
  //
  if (has_valid_serial_number(run_type, waveform.serial_number)) {
    sprintf(temp_string, " (S/N %ld)", waveform.serial_number);
    strcat(waveform_version_string, temp_string);
  }
  
  // All done.
  //
  return waveform_version_string;
}

unsigned short apollo_get_fpl_platform_from_str(char *str) {
  unsigned short result = unknown_platform;
  
  if (str) {
    int i;
    
    for (i = 0; (i < num_platforms) && (unknown_platform == result); i++) {
      if (0 == strcmp(str, platform_names[i])) {
        result = i;
      }
    }
  }
  
  return result;
}

void apollo_get_panel_id_str_from_fpl(char *str, apollo_fpl_t *fpl) {
  if (fpl && str) {
    char temp_string[waveform_version_string_max];

    // FPL LOT
    //
    sprintf(str, "%03d%s", fpl->lot, waveform_seperator);
    
    // ADHESIVE RUN NUMBER
    //
    sprintf(temp_string, "%02d%s", fpl->adhesive_run_number, waveform_seperator);
    strcat(str, temp_string);
    
    // FPL PLATFORM
    //
    switch (fpl->platform) {
      case matrix_2_0:
      case matrix_2_1:
      case matrix_2_3:
      case matrix_Vizplex_110:
        strcat(str, platform_names[fpl->platform]);
        break;
        
      case unknown_platform:
      default:
        strcat(str, platform_names[unknown_platform]);
        break;
    }
  }
}
