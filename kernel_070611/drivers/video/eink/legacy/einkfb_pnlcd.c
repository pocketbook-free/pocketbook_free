/*
 *  linux/drivers/video/eink/legacy/einkfb_pnlcd.c -- eInk pnlcd emulator
 *
 *      Copyright (C) 2005-2007 Lab126
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

#include "einkfb_pnlcd.h"

#if PRAGMAS
	#pragma mark Definitions/Globals
#endif

FPOW_POWER_MODE g_pnlcd_power_mode = FPOW_MODE_ON;
int g_ioc_driver_present = 1;

static int IOC_Ioctl(struct inode *inNode, struct file *inFile, unsigned int inCommand, unsigned long inArg);
struct file_operations ioc_fops = {
	ioctl:	IOC_Ioctl
};

#if PRAGMAS
	#pragma mark -
	#pragma mark Utilities
	#pragma mark -
#endif

int fiona_eink_ioctl_stub(unsigned int cmd, unsigned long arg)
{
	return ( 0 );
}

int pnlcd_awake(void)
{
	return ( 1 );
}

////////////////////////////////////////////////////////
//
// Segment On/Off Functions
//
////////////////////////////////////////////////////////

static char pnlcd_segments0[PN_LCD_SEGMENT_COUNT]; // Saved PNLCD state.
static char pnlcd_segments1[PN_LCD_SEGMENT_COUNT]; // Working copy.

static char *pnlcd_segments = pnlcd_segments0;     // Point to saved state.
static int  force_issue_cmd = 0;                   // Set to "flush" cache.

static int
IOC_IssuePNLCDCmd(char *cmd, int cmdSize, char *response, int responseSize)
{
    int  i, issue_cmd = force_issue_cmd, skip = 1, result = responseSize;
    char startSeg, endSeg, segment = SEGMENT_ON;
    
    // Make a copy of the current PNLCD segment state.
    //
    memcpy(pnlcd_segments1, pnlcd_segments0, PN_LCD_SEGMENT_COUNT);
    
    // Update the copy of the PNLCD segment state.
    //
    switch ( cmd[0] )
    {
        // Supported commands.
        //
        case IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_OFF:
            segment = SEGMENT_OFF;
        case IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_ON:
            skip = 2;
        goto sequential_common;
       
        case IOC_CMD_PNLCD_SEQUENTIAL_OFF:
            segment = SEGMENT_OFF;
        case IOC_CMD_PNLCD_SEQUENTIAL_ON:
        sequential_common:      
            startSeg = cmd[1];
            endSeg   = cmd[2] + 1;

            for (i = startSeg; i < endSeg; i += skip)
                pnlcd_segments1[i] = segment;
        break;

        case IOC_CMD_PNLCD_MISC_OFF:
            segment = SEGMENT_OFF;
        case IOC_CMD_PNLCD_MISC_ON:
            startSeg = 2;
            endSeg   = cmd[1] + 2;

            for (i = startSeg; i < endSeg; i++)
                pnlcd_segments1[(int)cmd[i]] = segment;
        break;

        // Not-yet supported commands (and not used by framework).
        //
        case IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_OFF:
        case IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_ON:
        case IOC_CMD_PNLCD_MISC_PAIR_OFF:
        case IOC_CMD_PNLCD_MISC_PAIR_ON:
        default:
            issue_cmd = 1;
        break;
    }
    
    // If the PNLCD state changed, then update the PNLCD segment state and
    // issue the appropriate command.
    //
    if ( issue_cmd || (0 != memcmp(pnlcd_segments1, pnlcd_segments0, PN_LCD_SEGMENT_COUNT)) )
        memcpy(pnlcd_segments0, pnlcd_segments1, PN_LCD_SEGMENT_COUNT);

    return ( result );
}

static int
IOC_Sequential_Segment(unsigned short on, char startSeg, char endSeg, int vertical)
{
    int     err = 0;
    char    the_cmd[3];
    char    result[IOC_CMD_PNLCD_LIGHT_RESPONSE_BYTES];

    NDEBUG("@@@@ IOC_Sequential_Segment(on=%02x, start=%02x, end=%02x, vert=%d)\n",on,startSeg,endSeg,vertical);

    // Range-check values to assure segment < 200
    if (startSeg > MAX_PN_LCD_SEGMENT) {
            NDEBUG("@@@@ IOC_Sequential_Segment(start=%02x out of range)\n",startSeg);
            err = startSeg;
            goto exit;
    }
    else if (endSeg > MAX_PN_LCD_SEGMENT)  {
            NDEBUG("@@@@ IOC_Sequential_Segment(end=%02x out of range)\n",endSeg);
            err = endSeg;
            goto exit;
    }

    // Turn on sequential segments
    if (vertical)
        the_cmd[0] = (on ? IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_ON : IOC_CMD_PNLCD_VERTICAL_SEQUENTIAL_OFF);
    else
        the_cmd[0] = (on ? IOC_CMD_PNLCD_SEQUENTIAL_ON : IOC_CMD_PNLCD_SEQUENTIAL_OFF);
    the_cmd[1] = startSeg;         // The starting segment number to turn on
    the_cmd[2] = endSeg;           // The ending segment number to turn on
    err = IOC_IssuePNLCDCmd(the_cmd,3,result,IOC_CMD_PNLCD_LIGHT_RESPONSE_BYTES);
    if (err == 1)
        err = 0;  // no errors...
exit:
    return(err);  // Return Error - not implemented yet
}


static int
IOC_Multi_Segment(unsigned short on, char num_segs, char *segment_array)
{
    int     sample_count = 0;
    int     x = 0;
    int     err = 0;
    char    the_cmd[256];
    char    *cur_byte = the_cmd;
    char    result = 0;

    NDEBUG("@@@@ IOC_Multi_Segment(on=%02x, numSegs=%02x, array=%08x)\n",on,num_segs,(int)segment_array);

    sample_count = num_segs;     // Count is given in pairs

    // Make sure we don't overflow firmware buffers
    if (num_segs > (MAX_COMMAND_BYTES-3))  {
        err = num_segs;
        goto exit;
    }
    for (x=0;x<num_segs;x++) {
        if (segment_array[x] > MAX_PN_LCD_SEGMENT) {
            err = segment_array[x];
            goto exit;
        }
    }

    // Turn on multiple segments
    *cur_byte++ = (on ? IOC_CMD_PNLCD_MISC_ON : IOC_CMD_PNLCD_MISC_OFF);
    *cur_byte++ = num_segs;         // The number of segments to turn on or off
    for (x=0;x<num_segs;x++)
        *cur_byte++ = segment_array[x];  // The next segment number to turn on
    err = IOC_IssuePNLCDCmd(the_cmd,(sample_count+2),&result,IOC_CMD_PNLCD_LIGHT_RESPONSE_BYTES);
    if (err == 1)
        err = 0;  // no errors...

exit:
    return(err);  
}

static int
IOC_Multi_Segment_Pairs(unsigned short on, char num_pairs, char *paired_segment_array, int vertical)
{
    int     sample_count = 0;
    int     x = 0;
    int     err = 0;
    char    the_cmd[256];
    char    *cur_byte = the_cmd;
    char    result = 0;

    NDEBUG("@@@@ IOC_Multi_Segment_Pairs(on=%02x, numPairs=%02x, array=%08x)",on,num_pairs,(int)paired_segment_array);

    sample_count = (num_pairs*2);     // Count is given in pairs

    // Range-check values to assure segment < 200
    if (sample_count > (MAX_COMMAND_BYTES-3))  {
        err = sample_count;
        goto exit;
    }
    for (x=0;x<sample_count;x++) {
        if (paired_segment_array[x] > MAX_PN_LCD_SEGMENT) {
            err = paired_segment_array[x];
            goto exit;
        }
    }

    // Build command to turn on/off multiple segments
    if (vertical)
        *cur_byte++ = (on ? IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_ON : IOC_CMD_PNLCD_MISC_VERTICAL_PAIR_OFF);
    else
        *cur_byte++ = (on ? IOC_CMD_PNLCD_MISC_PAIR_ON : IOC_CMD_PNLCD_MISC_PAIR_OFF);
    *cur_byte++ = sample_count;        // The number of segments to turn on or off
    for (x=0;x<sample_count;x++)
        *cur_byte++ = paired_segment_array[x];  // The next segment number to turn on

    err = IOC_IssuePNLCDCmd(the_cmd,(sample_count+2),&result,IOC_CMD_PNLCD_LIGHT_RESPONSE_BYTES);
    if (err == 1)
        err = 0;  // no errors...

exit:
    return(err);  
}

static void set_which_pnlcd_segments(char which, char *segments)
{
    char start, count;
    int i;
    
    // First, do all the segments, if any, that are transitioning.
    //
    for (i = start = which, count = 0; i < (PN_LCD_SEGMENT_COUNT - 2); i += 2)
    {
        if ( segments[i] != segments[i+2] )
        {
            IOC_Sequential_Segment((unsigned short)segments[(int)start], start, (start + (count * 2)), VERT_SEQ);
            start = i+2; count = 0;
        }
        else
            count++;
    }
    
    // Now, do any "left overs."
    //
    IOC_Sequential_Segment((unsigned short)segments[(int)start], start, (start + (count * 2)), VERT_SEQ);
}

void set_pnlcd_segments(char *segments)
{
    set_which_pnlcd_segments(0, segments); // Evens (right column).
    set_which_pnlcd_segments(1, segments); // Odds  (left  column).
}

void restore_pnlcd_segments(void)
{
    force_issue_cmd = 1; set_pnlcd_segments(pnlcd_segments);
    force_issue_cmd = 0;
}

char *get_pnlcd_segments(void)
{
    return(pnlcd_segments);
}

int
IOC_Enable_PNLCD(unsigned short enable)
{
    // Just clear the PNLCD on DISABLE to eliminate the "sparkle" problem.
    if (enable!=ENABLE) {
        IOC_Sequential_Segment(SEGMENT_OFF, FIRST_PNLCD_SEGMENT, LAST_PNLCD_SEGMENT, SEQ);
    } 
    return(0);
}

static int
IOC_Ioctl(struct inode *inNode, struct file *inFile,
            unsigned int inCommand, unsigned long inArg)
{
    IOC_ENABLE_REC              enableRec;
    int                         retVal = 0;
    int                         x = 0;

    // Get user data
    switch(inCommand)  {
        case IOC_PNLCD_VERT_SEQ_IOCTL:
        case IOC_PNLCD_SEQ_IOCTL:
        {   
            PNLCD_SEQ_SEGMENTS_REC      pnlcdSeqRec;

            NDEBUG("IOC_PNLCD%sSEQ_IOCTL case seen. \n",(inCommand == IOC_PNLCD_SEQ_IOCTL)?"_":"_VERT_");

            // If we were called from inside, don't copy from user space
            if (inNode == (struct inode *)0xDEADBEEF)
                memcpy(&pnlcdSeqRec,(void*)inArg,sizeof(pnlcdSeqRec));
            else 
            if (copy_from_user(&pnlcdSeqRec,(void *)inArg, sizeof(pnlcdSeqRec)))  {
                NDEBUG("IOC_IOCTL() failed copy_from_user !! Kernel Space ??? \n");
                goto errOut;
            }

            // Turn on or off the requested sequential segment range
            NDEBUG("IOC_IOCTL() calling IOC_Seq_Segment() \n");
            retVal = IOC_Sequential_Segment(pnlcdSeqRec.enable, pnlcdSeqRec.start_seg, pnlcdSeqRec.end_seg,(inCommand == IOC_PNLCD_SEQ_IOCTL)?0:1);
            NDEBUG("IOC_IOCTL() back from calling IOC_Seq_Segment(), retVal = %d \n",retVal);
            break;
        }

        case IOC_PNLCD_MISC_IOCTL:
        {
            PNLCD_MISC_SEGMENTS_REC     pnlcdMiscRec;
            char                        segments[PN_LCD_SEGMENT_COUNT];
            int                         user_segments[PN_LCD_SEGMENT_COUNT];
            
            NDEBUG("IOC_PNLCD_MISC_IOCTL case seen. \n");

            // If we were called from inside, don't copy from user space
            if (inNode == (struct inode *) 0xDEADBEEF) {
                memcpy(&pnlcdMiscRec,(void*)inArg,sizeof(pnlcdMiscRec));
                memcpy(user_segments,pnlcdMiscRec.seg_array,MIN((pnlcdMiscRec.num_segs*sizeof(*pnlcdMiscRec.seg_array)),sizeof(user_segments)));
            }
            else  {
                // Copy main parameter block
                if (copy_from_user(&pnlcdMiscRec,(void *)inArg, sizeof(pnlcdMiscRec))) {
                    NDEBUG("IOC_IOCTL() failed to copy user data\n");
                    goto errOut;
                }

                // Copy segment array pointer
                if (copy_from_user(user_segments,(void *)pnlcdMiscRec.seg_array, MIN((pnlcdMiscRec.num_segs*sizeof(*pnlcdMiscRec.seg_array)),sizeof(user_segments)))) {
                    NDEBUG("IOC_IOCTL() failed to copy user segment_array data\n");
                    goto errOut;
                }
            }

            // Now load up our character array pointer
            for (x=0; x<pnlcdMiscRec.num_segs; x++)
                segments[x] = (char) user_segments[x];

            // Turn on or off the requested segments
            retVal = IOC_Multi_Segment(pnlcdMiscRec.enable, pnlcdMiscRec.num_segs, segments);
            break;
        }

        case IOC_PNLCD_VERT_PAIRS_IOCTL:
        case IOC_PNLCD_PAIRS_IOCTL:
        {
            PNLCD_SEGMENT_PAIRS_REC     pnlcdPairRec;
            char                        segments[PN_LCD_SEGMENT_COUNT];
            int                         user_segments[PN_LCD_SEGMENT_COUNT];

            NDEBUG("IOC_PNLCD%sPAIRS_IOCTL case seen. \n",(inCommand == IOC_PNLCD_PAIRS_IOCTL)?"_":"_VERT_");

            // If we were called from inside, don't copy from user space
            if (inNode == (struct inode *) 0xDEADBEEF) {
                memcpy(&pnlcdPairRec,(void*)inArg,sizeof(pnlcdPairRec));
                memcpy(user_segments,pnlcdPairRec.seg_pair_array,MIN((pnlcdPairRec.num_pairs*2*sizeof(*pnlcdPairRec.seg_pair_array)),sizeof(user_segments)));
            }
            else  {
                // Copy main parameter block
                if (copy_from_user(&pnlcdPairRec,(void *)inArg, sizeof(pnlcdPairRec)))
                    goto errOut;

                // Copy segment array pointer
                if (copy_from_user(user_segments,(void *)pnlcdPairRec.seg_pair_array, MIN((pnlcdPairRec.num_pairs*2*sizeof(*pnlcdPairRec.seg_pair_array)),sizeof(user_segments))))
                    goto errOut;
            }

            // Now load up our character array pointer
            for (x=0; x<(pnlcdPairRec.num_pairs*2); x++)
                segments[x] = (char) user_segments[x];

            // Turn on or off the requested segments
            retVal = IOC_Multi_Segment_Pairs(pnlcdPairRec.enable, pnlcdPairRec.num_pairs, segments,(inCommand == IOC_PNLCD_PAIRS_IOCTL)?0:1);
            break;
        }

        case IOC_PNLCD_ENABLE_IOCTL:
            NDEBUG("IOC_PNLCD_ENABLE_IOCTL case seen. \n");

            // Copy main parameter block
            // If we were called from inside, don't copy from user space
            if (inNode == (struct inode *)0xDEADBEEF)
                memcpy(&enableRec,(void*)inArg,sizeof(enableRec));
            else  {
                if (copy_from_user(&enableRec,(void *)inArg, sizeof(enableRec)))
                    goto errOut;
            }

            // Enable or disable the pn-lcd 
            retVal = IOC_Enable_PNLCD(enableRec.enable);
            break;

        default:
            printk("Unknown IOCTL case %x seen. \n",inCommand);
            goto errOut;
    }
    return(retVal);

errOut:
    return(-EFAULT);
}
