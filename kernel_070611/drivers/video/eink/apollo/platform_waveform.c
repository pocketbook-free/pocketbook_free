/*
 * eInk display framebuffer driver
 * 
 * Copyright (C)2005-2008 Lab126, Inc.  All rights reserved.
 */

unsigned char apollo_get_temperature(void)
{
    return ( 0 );
}

unsigned char apollo_get_version(void)
{
    return ( 0 );
}

void apollo_override_fpl(apollo_fpl_t *fpl)
{
    if ( fpl )
        fpl->overridden = 0;
}

#include "apollo_waveform.c"

unsigned long get_embedded_waveform_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
    {
        // Perform little-endian-to-big-endian conversion.
        //
        checksum = (buffer[EINK_ADDR_CHECKSUM + 0] << 24) |
                   (buffer[EINK_ADDR_CHECKSUM + 1] << 16) |
                   (buffer[EINK_ADDR_CHECKSUM + 2] <<  8) |
                   (buffer[EINK_ADDR_CHECKSUM + 3] <<  0);
    }
        
    return ( checksum );
}

unsigned long get_computed_waveform_checksum(unsigned char *buffer)
{
    unsigned long checksum = 0;
    
    if ( buffer )
    {
        unsigned long saved_embedded_checksum;
        buffer_long = (unsigned long *)buffer;
        
        // Save the buffer's embedded checksum and then set it zero.
        //
        saved_embedded_checksum = buffer_long[EINK_ADDR_CHECKSUM >> 2];
        buffer_long[EINK_ADDR_CHECKSUM >> 2] = 0;
        
        // Compute the checkum over the entire buffer, including
        // the zeroed-out embedded checksum area, and then restore
        // the embedded checksum.
        //
        checksum = crc32((unsigned char *)buffer, EINK_WAVEFORM_FILESIZE);
        buffer_long[EINK_ADDR_CHECKSUM >> 2] = saved_embedded_checksum;
        buffer_long = NULL;
    }
    
    return ( checksum );
}
