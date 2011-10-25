#include "apollo.h"

static int ack_timeout_count = 0;

int wait_for_ack_irq(int which) {
    g_ack_timed_out = (MAX_TIMEOUTS <= ack_timeout_count) ? true : false;

    if (g_ack_timed_out) {
        puts("wait_for_ack_irq: too many timeouts, rebooting...\n");
        do_reboot();
    
    } else {
        unsigned long start_time;
    
        reset_timer(); start_time = get_timer(0);
    
        while (!g_ack_timed_out && (which != apollo_get_ack())) {
            if (MAX_TIMEOUT < get_timer(start_time)) {
                printf("wait_for_ack_irq: acks timed out = %d\n", ++ack_timeout_count);
                g_ack_timed_out = true;
            }
        }
    }

    return 1;
}

int needs_to_enable_temp_sensor(void) {
#ifdef NEEDS_TO_ENABLE_TEMP_SENSOR
    return 1;
#else
    return 0;
#endif
}

// 1bpp -> 2bpp
//
// 0 0 0 0 -> 00 00 00 00   0x0 -> 0x00
// 0 0 0 1 -> 00 00 00 11   0x1 -> 0x03
// 0 0 1 0 -> 00 00 11 00   0x2 -> 0x0C
// 0 0 1 1 -> 00 00 11 11   0x3 -> 0x0F
// 0 1 0 0 -> 00 11 00 00   0x4 -> 0x30
// 0 1 0 1 -> 00 11 00 11   0x5 -> 0x33
// 0 1 1 0 -> 00 11 11 00   0x6 -> 0x3C
// 0 1 1 1 -> 00 11 11 11   0x7 -> 0x3F
// 1 0 0 0 -> 11 00 00 00   0x8 -> 0xC0
// 1 0 0 1 -> 11 00 00 11   0x9 -> 0xC3
// 1 0 1 0 -> 11 00 11 00   0xA -> 0xCC
// 1 0 1 1 -> 11 00 11 11   0xB -> 0xCF
// 1 1 0 0 -> 11 11 00 00   0xC -> 0xF0
// 1 1 0 1 -> 11 11 00 11   0xD -> 0xF3
// 1 1 1 0 -> 11 11 11 00   0xE -> 0xFC
// 1 1 1 1 -> 11 11 11 11   0xF -> 0xFF
//
static u8 stretch_nybble_table_1_to_2bpp[16] =
{
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

// 2bpp -> 4bpp
//
// 00 00 -> 0000 0000       0x0 -> 0x00
// 00 01 -> 0000 0101       0x1 -> 0x05
// 00 10 -> 0000 1010       0x2 -> 0x0A
// 00 11 -> 0000 1111       0x3 -> 0x0F
// 01 00 -> 0101 0000       0x4 -> 0x50
// 01 01 -> 0101 0101       0x5 -> 0x55
// 01 10 -> 0101 1010       0x6 -> 0x5A
// 01 11 -> 0101 1111       0x7 -> 0x5F
// 10 00 -> 1010 0000       0x8 -> 0xA0
// 10 01 -> 1010 0101       0x9 -> 0xA5
// 10 10 -> 1010 1010       0xA -> 0xAA
// 10 11 -> 1010 1111       0xB -> 0xAF
// 11 00 -> 1111 0000       0xC -> 0xF0
// 11 01 -> 1111 0101       0xD -> 0xF5
// 11 10 -> 1111 1010       0xE -> 0xFA
// 11 11 -> 1111 1111       0xF -> 0xFF
//
static u8 stretch_nybble_table_2_to_4bpp[16] =
{
    0x00, 0x05, 0x0A, 0x0F, 0x50, 0x55, 0x5A, 0x5F,
    0xA0, 0xA5, 0xAA, 0xAF, 0xF0, 0xF5, 0xFA, 0xFF
};

unsigned char stretch_nybble(unsigned char nybble, unsigned long bpp)
{
    unsigned char *which_nybble_table = NULL, result = nybble;

    switch ( nybble )
    {
        // Special-case the table endpoints since they're always the same.
        //
        case 0x00:
            result = APOLLO_WHITE;
        break;
        
        case 0x0F:
            result = APOLLO_BLACK;
        break;
        
        // Handle everything else on a bit-per-pixel basis.
        //
        default:
            switch ( bpp )
            {
                case APOLLO_DEPTH_1BPP:
                    which_nybble_table = stretch_nybble_table_1_to_2bpp;
                break;
                
                case APOLLO_DEPTH_2BPP:
                    which_nybble_table = stretch_nybble_table_2_to_4bpp;
                break;
            }
            
            if ( which_nybble_table )
                result = which_nybble_table[nybble];
        break;
    }

    return ( result );
}
