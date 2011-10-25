/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_waveform.c --
 *  eInk frame buffer device HAL broadsheet waveform code
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#define waveform_version_string_max 64
#define waveform_unknown_char       '?'
#define waveform_seperator          "_"


enum run_types
{
    bl, b, t, p, q, ax4, cx5, dx6, ex7, fx8, gx9, hxa, ixb, jxc, kxd, lxe, mxf, nx10, unknown_run_type, num_run_types
};
typedef enum run_types run_types;

#define skip_legacy_run_types       b

enum wf_types
{
    wx, wy, wp, wz, wq, ta, wu, tb, td, wv, wt, te, xx, xy, we, wd, 

    xz, ve,   unknown_wf_typex12, unknown_wf_typex13, unknown_wf_typex14, WJ, WK, WL, 
    unknown_wf_typex18, unknown_wf_typex19, unknown_wf_typex1a, WM,  unknown_wf_typex1c, WN,  unknown_wf_typex1e, WO, 

    unknown_wf_typex20, WP,  unknown_wf_typex22, WR,  unknown_wf_typex24, WS,  unknown_wf_typex26, WA, 
    unknown_wf_typex28, WB,  unknown_wf_typex2a, WC,  unknown_wf_typex2c, WF,  unknown_wf_type,
    num_wf_types
};
typedef enum wf_types wf_types;

enum platforms
{
    matrix_2_0_unsupported, matrix_2_1_unsupported, matrix_Vizplex_100, matrix_Vizplex_110,
    matrix_2_4_Eink_Vizplex_110A, matrix_2_5_unsupported, matrix_2_6_Eink_Vizplex_220, 
    matrix_2_7_Eink_Vizplex_250, unknown_platform, num_platforms
};
typedef enum platforms platforms;

enum panel_sizes
{
    five_oh, six_oh, eight_oh, nine_seven, unknown_panel_size, num_panel_sizes,
    five_inch = 50, six_inch = 60, eight_inch = 80, nine_inch = 97
};
typedef enum panel_sizes panel_sizes;

enum mfg_codes
{
   undefined_mfg_code, pvi, lgd, ED060SC4H1, ED060SC4H2, ED097OC1H2, ED060SC5, ED097OC1,
   ED050SC3, ED050SC3H1, ED050SU2, ED060SC5H1, ED060SC5H2, ED097OC1H1, EF097OC3, EF097OC2H1,
   EF097OC2H2, ED060SC4, ED080XC1, ED071OC1, unknown_mfg_code, num_mfg_codes
};
typedef enum mfg_codes mfg_codes;

static char waveform_version_string[waveform_version_string_max];

static char *run_type_names[num_run_types] =
{
    "?", "B", "T", "P", "Q", "A", "C", "D", "E", "F", "G", "H", "I", "J", "K","L", "M", "N", "?"
};

static char *wf_type_names[num_wf_types] =
{
    "WX", "WY", "WP", "WZ", "WQ", "TA", "WU", "TB", 
    "TD",  "WV",  "WT", "TE",  "??",  "??", "WE", "WD", 
     "??",  "VE",   "??",  "??",  "??",  "WJ",  "WK", "WL",
    "??",   "??",   "??",  "WM" , "??", "WN", "??",  "WO",
    "??",  "WP",  "??",  "WR" ,  "??", "WS", "??", "WA",
    "??", "WB",  "??", "WC" ,  "??", "WF", "??"
};

static char *platform_names[num_platforms] =
{
    "????", "????", "V100", "V110",  "V100A", "????",  "V220", "V250", "????"
};

static char *panel_size_names[num_panel_sizes] =
{
    "50", "60", "80", "97", "??"
};

static char *mfg_code_names[num_mfg_codes] =
{
    "???", "PVI", "LGD", "4H1", "4H2", "1H2", "SC5", "OC1",
   "SC3", "3H1", "SU2", "5H1", "5H2", "1H1", "OC3", "2H1",
   "2H2", "SC4", "XC1", "OC1", "???"
};

#define IN_RANGE(n, m, M) (((n) >= (m)) && ((n) <= (M)))

/*
#define has_valid_serial_number(r, s) \
    (((b == r) || (p == r)) && (0 != s))
*/    
#define has_valid_serial_number(r, s) \
    (0 != s)

#define has_valid_mfg_code(c) \
    (IN_RANGE(c, pvi, lgd))

#define BS_CHECKSUM(c1, c2) (((c2) << 16) | (c1))

void broadsheet_get_waveform_info(broadsheet_waveform_info_t *info)
{
    if ( info )
    {
        bs_flash_select saved_flash_select = broadsheet_get_flash_select();
        unsigned char checksum1, checksum2;

        broadsheet_set_flash_select(bs_flash_waveform);

        broadsheet_read_from_flash_byte(EINK_ADDR_WAVEFORM_VERSION,    &info->waveform_version);
        broadsheet_read_from_flash_byte(EINK_ADDR_WAVEFORM_SUBVERSION, &info->waveform_subversion);
        broadsheet_read_from_flash_byte(EINK_ADDR_WAVEFORM_TYPE,       &info->waveform_type);
        broadsheet_read_from_flash_byte(EINK_ADDR_RUN_TYPE,            &info->run_type);
        broadsheet_read_from_flash_byte(EINK_ADDR_FPL_PLATFORM,        &info->fpl_platform);
        broadsheet_read_from_flash_byte(EINK_ADDR_FPL_SIZE,            &info->fpl_size);
        broadsheet_read_from_flash_byte(EINK_ADDR_ADHESIVE_RUN_NUM,    &info->adhesive_run_number);
        broadsheet_read_from_flash_byte(EINK_ADDR_MODE_VERSION,        &info->mode_version);
        broadsheet_read_from_flash_byte(EINK_ADDR_MFG_CODE,            &info->mfg_code);
        broadsheet_read_from_flash_byte(EINK_ADDR_CHECKSUM1,           &checksum1);
        broadsheet_read_from_flash_byte(EINK_ADDR_CHECKSUM2,           &checksum2);
        
        broadsheet_read_from_flash_short(EINK_ADDR_FPL_LOT,            &info->fpl_lot);
        
        broadsheet_read_from_flash_long(EINK_ADDR_CHECKSUM,            &info->filesize);
        broadsheet_read_from_flash_long(EINK_ADDR_FILESIZE,            &info->checksum);
        broadsheet_read_from_flash_long(EINK_ADDR_SERIAL_NUMBER,       &info->serial_number);
        
        broadsheet_set_flash_select(saved_flash_select);
        
        if ( 0 == info->filesize )
            info->checksum = BS_CHECKSUM(checksum1, checksum2);
    
	    einkfb_debug(   "\n"
                        " Waveform version:  0x%02X\n"
                        "       subversion:  0x%02X\n"
                        "             type:  0x%02X\n"
                        "         run type:  0x%02X\n"
                        "     mode version:  0x%02X\n"
                        "\n"
                        "     FPL platform:  0x%02X\n"
                        "              lot:  0x%04X\n"
                        "             size:  0x%02X\n"
                        " adhesive run no.:  0x%02X\n"
                        "\n"
                        "        File size:  0x%08lX\n"
                        "         Mfg code:  0x%02X\n"
                        "       Serial no.:  0x%08lX\n"
                        "         Checksum:  0x%08lX\n",
                        
                        info->waveform_version,
                        info->waveform_subversion,
                        info->waveform_type,
                        info->run_type,
                        info->mode_version,
                        
                        info->fpl_platform,
                        info->fpl_lot,
                        info->fpl_size,
                        info->adhesive_run_number,
                        
                        info->filesize,
                        info->mfg_code,
                        info->serial_number,
                        info->checksum);
    }
}

void broadsheet_get_waveform_version(broadsheet_waveform_t *waveform)
{
    if ( waveform )
    {
        broadsheet_waveform_info_t info; broadsheet_get_waveform_info(&info);
        
        waveform->version       = info.waveform_version;
        waveform->subversion    = info.waveform_subversion;
        waveform->type          = info.waveform_type;
        waveform->run_type      = info.run_type;
        waveform->mode_version  = (matrix_Vizplex_110 != info.fpl_platform) ? 0 : info.mode_version;
        waveform->mfg_code      = info.mfg_code;
        waveform->serial_number = info.serial_number;
        
        waveform->parse_wf_hex  = info.filesize ? true : false;
    }
}

void broadsheet_get_fpl_version(broadsheet_fpl_t *fpl)
{
    if ( fpl )
    {
        broadsheet_waveform_info_t info; broadsheet_get_waveform_info(&info);
        
        fpl->platform            = info.fpl_platform;
        fpl->size                = info.fpl_size;
        fpl->adhesive_run_number = (matrix_Vizplex_110 == info.fpl_platform) ? 0 : info.adhesive_run_number;
        fpl->lot                 = info.fpl_lot;
    }
}

char *broadsheet_get_waveform_version_string(void)
{
    char temp_string[waveform_version_string_max];
    broadsheet_waveform_t waveform;
    broadsheet_fpl_t fpl;
    
    panel_sizes panel_size;
    run_types run_type;
    
    int valid_serial_number, valid_mfg_code;

    // Get the waveform version info and clear the waveform
    // version string.
    //
    broadsheet_get_waveform_version(&waveform);
    broadsheet_get_fpl_version(&fpl);
    
    waveform_version_string[0] = '\0';
    run_type = waveform.run_type;
    panel_size = fpl.size;
    
    // Build up a waveform version string in the following way:
    //
    //      <FPL PLATFORM>_<RUN TYPE><FPL LOT NUMBER>_<FPL SIZE>_
    //      <WF TYPE><WF VERSION><WF SUBVERSION> (MFG CODE, S/N XXX)
    //
    // FPL PLATFORM
    //

    switch ( fpl.platform )
    {
        case matrix_Vizplex_100:
        case matrix_Vizplex_110:
        case matrix_2_4_Eink_Vizplex_110A:
        case matrix_2_6_Eink_Vizplex_220:
        case matrix_2_7_Eink_Vizplex_250:			
            strcat(waveform_version_string, platform_names[fpl.platform]);
        break;
        
        case unknown_platform:
        default:
            strcat(waveform_version_string, platform_names[unknown_platform]);
        break;
    }
    
    if ( matrix_Vizplex_110 == fpl.platform )
        strcat(waveform_version_string, waveform_seperator);

    // RUN TYPE
    //
    run_type += skip_legacy_run_types;
    
    switch ( run_type )
    {
        case b:
        case t:
        case p:
        case q:
        case ax4:
        case cx5:
        case dx6:
        case ex7:
        case fx8:
        case gx9:
        case hxa:
        case ixb:
        case jxc:
        case kxd:
        case lxe:
        case mxf:
        case nx10:
            strcat(waveform_version_string, run_type_names[run_type]);
        break;
        
        case unknown_run_type:
        case bl:
        default:
            strcat(waveform_version_string, run_type_names[unknown_run_type]);
        break;
    }

    // FPL LOT NUMBER
    //
    sprintf(temp_string, "%03d", (fpl.lot % 1000));
    strcat(waveform_version_string, temp_string);

    // ADHESIVE RUN NUMBER
    //
    if ( matrix_Vizplex_110 != fpl.platform )
    {
        sprintf(temp_string, "%02d", (fpl.adhesive_run_number % 100));
        strcat(waveform_version_string, temp_string);
    }

    strcat(waveform_version_string, waveform_seperator);

    // FPL SIZE
    //
    switch ( fpl.size )
    {
        case five_inch:
            panel_size = five_oh;
        break;
        
        case six_inch:
            panel_size = six_oh;
        break;
        
        case eight_inch:
            panel_size = eight_oh;
        break;
        
        case nine_inch:
            panel_size = nine_seven;
        break;
        
        default:
            panel_size = unknown_panel_size;
        break;
    }

    switch ( panel_size )
    {
        case five_oh:
        case six_oh:
        case eight_oh:
        case nine_seven:
            strcat(waveform_version_string, panel_size_names[panel_size]);
        break;
    
        case unknown_panel_size:
        default:
            strcat(waveform_version_string, panel_size_names[unknown_panel_size]);
        break;
    }
    strcat(waveform_version_string, waveform_seperator);

    // WF TYPE
    //
    switch ( waveform.type )
    {
        case wx:
        case wy:
        case wp:
        case wz:
        case wq:
        case ta:
        case wu:
        case tb:
        case td:
        case wv:
        case wt:
        case te:
        case we:
        case ve:

        case WJ:
        case WK:
        case WL:
        case WM:
        case WN:
        case WO:
        case WP:
        case WR:
        case WS:
        case WA:
        case WB:
        case WC:
        case WF:		
            strcat(waveform_version_string, wf_type_names[waveform.type]);
        break;
      
        case unknown_wf_type:
        default:
            strcat(waveform_version_string, wf_type_names[unknown_wf_type]);
        break;
    }
    
    // WF VERSION
    //
    if ( waveform.parse_wf_hex )
        sprintf(temp_string, "%02X", waveform.version);
    else
        sprintf(temp_string, "%02d", (waveform.version % 100));
        
    strcat(waveform_version_string, temp_string);
    
    // WF SUBVERSION
    //
    if ( waveform.parse_wf_hex )
        sprintf(temp_string, "%02X", waveform.subversion);
    else
        sprintf(temp_string, "%02d", (waveform.subversion % 100));
    
    strcat(waveform_version_string, temp_string);
    
    // MFG CODE, SERIAL NUMBER
    //
    valid_serial_number = has_valid_serial_number(run_type, waveform.serial_number);
    valid_mfg_code = has_valid_mfg_code(waveform.mfg_code);
    temp_string[0] = '\0';

    if ( valid_mfg_code && valid_serial_number )
        sprintf(temp_string, " (%s, S/N %ld)", mfg_code_names[waveform.mfg_code], waveform.serial_number);
    else if ( valid_mfg_code )
            sprintf(temp_string, " (%s)", mfg_code_names[waveform.mfg_code]);
        else if ( valid_serial_number )
                sprintf(temp_string, " (S/N %ld)", waveform.serial_number);

    if ( strlen(temp_string) )
        strcat(waveform_version_string, temp_string);

    // All done.
    //
    return ( waveform_version_string );
}

bool broadsheet_waveform_valid(void)
{
    char *waveform_version = broadsheet_get_waveform_version_string();
    bool result = true;
    
    if ( strchr(waveform_version, waveform_unknown_char) )
    {
        einkfb_print_error("Unrecognized values in waveform header\n");
        einkfb_debug("waveform_version = %s\n", waveform_version);
        result = false;
    }
    printk("Henry: waveform_version = %s\n", waveform_version);        
    return ( result );
}
