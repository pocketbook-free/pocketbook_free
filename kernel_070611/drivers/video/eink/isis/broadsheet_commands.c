/*
 *  linux/drivers/video/eink/broadsheet/broadsheet_commands.c --
 *  eInk frame buffer device HAL broadsheet commands code
 *
 *      Copyright (C) 2005-2008 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#define COMMANDS_VERSION_STRING_MAX     32
#define COMMANDS_VERSION_UNKNOWN_CHAR   '?'
#define COMMANDS_VERSION_SEPERATOR      "_"

#define COMMANDS_TYPE_0             0x0000
#define COMMANDS_TYPE_1             0x6273

enum commands_types
{
    commands_type_0, commands_type_1, commands_type_unknown, num_commands_types
};
typedef enum commands_types commands_types;

static char commands_version_string[COMMANDS_VERSION_STRING_MAX];

static char *commands_types_names[num_commands_types] =
{
    "T", "P", "?"
};

void broadsheet_get_commands_info(broadsheet_commands_info_t *info)
{
    if ( info )
    {
        bs_flash_select saved_flash_select = broadsheet_get_flash_select();
        broadsheet_set_flash_select(bs_flash_commands);
        
        if ( broadsheet_supports_flash() )
        {
            unsigned short type;
            
            broadsheet_read_from_flash_byte(EINK_ADDR_COMMANDS_VERS_MAJOR,  &info->vers_major);
            broadsheet_read_from_flash_byte(EINK_ADDR_COMMANDS_VERS_MINOR,  &info->vers_minor);
            broadsheet_read_from_flash_short(EINK_ADDR_COMMANDS_TYPE,       &type);
            broadsheet_read_from_flash_long(EINK_ADDR_COMMANDS_CHECKSUM,    &info->checksum);
        
            info->type  = ((type & 0xFF00) >> 8) | ((type & 0x00FF) << 8);
            info->which = EINK_COMMANDS_BROADSHEET;
        }
        else
        {
            unsigned short code_size;
            unsigned long  version;
            int file_size;
            
            broadsheet_read_from_flash_long(EINK_ADDR_COMMANDS_VERSION,     &version);
            broadsheet_read_from_flash_short(EINK_ADDR_COMMANDS_CODE_SIZE,  &code_size);
            
            version     = ((version & 0xFF000000) >> 24) |
                          ((version & 0x00FF0000) >>  8) |
                          ((version & 0x0000FF00) <<  8) |
                          ((version & 0x000000FF) << 24);

            file_size   = ((code_size + 1) << 1) + EINK_COMMANDS_FIXED_CODE_SIZE;

            broadsheet_read_from_flash_long((file_size - 4),                &info->checksum);
            
            info->which = EINK_COMMANDS_ISIS;
            info->version = version;
        }

        broadsheet_set_flash_select(saved_flash_select);
    }
}

char *broadsheet_get_commands_version_string(void)
{
    char temp_string[COMMANDS_VERSION_STRING_MAX];
    broadsheet_commands_info_t info;
    commands_types commands_type;
    
    broadsheet_get_commands_info(&info);
    commands_version_string[0] = '\0';

    // Build a commands version string of the form:
    //
    //  Broadsheet:
    //
    //      <TYPE>_<VERS_MAJOR.VERS_MINOR> (C/S = <CHECKSUM>).
    //
    //  ISIS:
    //
    //      V<VERSION> (C/S = <CHECKSUM>).
    //
    
    if ( EINK_COMMANDS_BROADSHEET == info.which )
    {
        // TYPE
        //
        switch ( info.type )
        {
            case COMMANDS_TYPE_0:
                commands_type = commands_type_0;
            break;
            
            case COMMANDS_TYPE_1:
                commands_type = commands_type_1;
            break;
            
            default:
                commands_type = commands_type_unknown;
            break;
        }
        strcat(commands_version_string, commands_types_names[commands_type]);
        strcat(commands_version_string, COMMANDS_VERSION_SEPERATOR);
        
        // VERSION
        //
        sprintf(temp_string, "%02X.%02X", info.vers_major, info.vers_minor);
        strcat(commands_version_string, temp_string);
    }
    else
    {
        // VERSION
        //
        sprintf(temp_string, "V%04lX", (info.version % 0x10000));
        strcat(commands_version_string, temp_string);
    }
    
    // CHECKSUM
    //
    if ( info.checksum )
    {
        sprintf(temp_string, " (C/S %08lX)", info.checksum);
        strcat(commands_version_string, temp_string);
    }
    
    return ( commands_version_string );
}

bool broadsheet_commands_valid(void)
{
    char *commands_version = broadsheet_get_commands_version_string();
    bool result = true;
    
    if ( strchr(commands_version, COMMANDS_VERSION_UNKNOWN_CHAR) )
    {
        einkfb_print_error("Unrecognized values in commands header\n");
        einkfb_debug("commands_version = %s\n", commands_version);
        result = false;
    }
        
    return ( result );
}
