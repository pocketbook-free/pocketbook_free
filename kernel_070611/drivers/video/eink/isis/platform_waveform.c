/*
 * eInk display framebuffer driver
 * 
 * Copyright (C)2005-2008 Lab126, Inc.  All rights reserved.
 */

#include "broadsheet_waveform.c"
#include "broadsheet_commands.c"

unsigned long get_embedded_waveform_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
    {
        unsigned long *long_buffer = (unsigned long *)buffer,
                       filesize = long_buffer[EINK_ADDR_FILESIZE >> 2];
        
        if ( filesize )
            checksum = (buffer[EINK_ADDR_CHECKSUM + 0] <<  0) |
                       (buffer[EINK_ADDR_CHECKSUM + 1] <<  8) |
                       (buffer[EINK_ADDR_CHECKSUM + 2] << 16) |
                       (buffer[EINK_ADDR_CHECKSUM + 3] << 24);
        else
            checksum = BS_CHECKSUM(buffer[EINK_ADDR_CHECKSUM1], buffer[EINK_ADDR_CHECKSUM2]);
    }
    
    return ( checksum );
}

unsigned long get_computed_waveform_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
    {
        unsigned long *long_buffer = (unsigned long *)buffer,
                       filesize = long_buffer[EINK_ADDR_FILESIZE >> 2];
        
        if ( filesize )
        {
            unsigned long saved_embedded_checksum;
            
            // Save the buffer's embedded checksum and then set it zero.
            //
            saved_embedded_checksum = long_buffer[EINK_ADDR_CHECKSUM >> 2];
            long_buffer[EINK_ADDR_CHECKSUM >> 2] = 0;
            
            // Compute the checkum over the entire buffer, including
            // the zeroed-out embedded checksum area, and then restore
            // the embedded checksum.
            //
            checksum = crc32((unsigned char *)buffer, filesize);
            long_buffer[EINK_ADDR_CHECKSUM >> 2] = saved_embedded_checksum;
        }
        else
        {
            unsigned char checksum1, checksum2;
            int start, length;
    
            // Checksum bytes 0..(EINK_ADDR_CHECKSUM1 - 1).
            //
            start     = 0;
            length    = EINK_ADDR_CHECKSUM1;
            checksum1 = crc8(&buffer[start], length);
            
            // Checksum bytes (EINK_ADDR_CHECKSUM1 + 1)..(EINK_ADDR_CHECKSUM2 - 1).
            //
            start     = EINK_ADDR_CHECKSUM1 + 1;
            length    = EINK_ADDR_CHECKSUM2 - start;
            checksum2 = crc8(&buffer[start], length);
            
            checksum  = BS_CHECKSUM(checksum1, checksum2);
        }
    }
    
    return ( checksum );
}

unsigned long get_embedded_commands_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
        checksum = (buffer[EINK_ADDR_COMMANDS_CHECKSUM + 0] <<  0) |
                   (buffer[EINK_ADDR_COMMANDS_CHECKSUM + 1] <<  8) |
                   (buffer[EINK_ADDR_COMMANDS_CHECKSUM + 2] << 16) |
                   (buffer[EINK_ADDR_COMMANDS_CHECKSUM + 3] << 24);
        
    return ( checksum );
}

unsigned long get_computed_commands_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
        checksum = crc32((unsigned char *)buffer, (EINK_COMMANDS_FILESIZE - 4));
    
    return ( checksum );
}
