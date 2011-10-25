/*
   (c) Copyright 2001-2009  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/ioctl.h>


#include <systems/fbdev/fbdev.h>
//#include <linux/h3600_ts.h>

#include <directfb.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/input.h>

#include <misc/conf.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/messages.h>
#include <direct/thread.h>

#include <core/input_driver.h>

#include <core/windows.h>

#include <core/system.h>

#include <inkview.h>
#include <inkinternal.h>


DFB_INPUT_DRIVER( zytronic )

typedef struct
{
    CoreInputDevice  *device;
    DirectThread *thread;

    int           fd;
} H3600TSData;

struct tsdata
{
    int x;
    int y;
};

#define H(x) (unsigned char)((x)>>8)
#define L(x) (unsigned char)((x)&0xff)

typedef struct eink_dfb_cmd_s
{
    int owner;
    int command;
    int nwrite;
    int nread;
//    unsigned char data[2*1600*1600];
    unsigned char data[1600*1200];
    unsigned char empty[4096];
} eink_dfb_cmd;

static eink_dfb_cmd cmd_refresh = {0};

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *		so make sure, you call it before using the other
 *		functions!
 */
u_int32_t crc_tab[256];

FBDev *dfb_fbdev = NULL;
int isBooklandMode;
int useFullUpdate = 0;

/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */
u_int32_t chksum_crc32 (unsigned char *block, unsigned int length)
{
    register unsigned long crc;
    unsigned long i;

    crc = 0xFFFFFFFF;
    for (i = 0; i < length; i++)
    {
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
    }
    return (crc ^ 0xFFFFFFFF);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */

void chksum_crc32gentab ()
{
    unsigned long crc, poly;
    int i, j;

    poly = 0xEDB88320L;
    for (i = 0; i < 256; i++)
    {
        crc = i;
        for (j = 8; j > 0; j--)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ poly;
            }
            else
            {
                crc >>= 1;
            }
        }
        crc_tab[i] = crc;
    }
}


static int screen_locked = 0;
static int update_needed = 0;
static int updFullCounter = 0;

static void update_screen()
{
    int dataOffset;
    if (screen_locked)
    {
        update_needed = 1;
        return;
    }
    update_needed = 0;

    static uint32_t oldHash = 0;
    uint32_t currentHash;
    if ( !(ivstate.uiflags & IN_KEYBOARD) &&
            (NULL != dfb_fbdev) &&
            (dfb_fbdev->fd > 0))
    {
        currentHash = chksum_crc32(dfb_fbdev->framebuffer_base, 2 * dfb_fbdev->shared->current_mode.xres * dfb_fbdev->shared->current_mode.yres);
        if (oldHash != currentHash)
        {
            unsigned char* buffer_ptr = NULL;
            unsigned char* display_buffer_ptr = NULL;
            long i = 0;
            long j = 0;

            oldHash = currentHash;
            cmd_refresh.owner = 0;
            cmd_refresh.command = 0;
            cmd_refresh.nwrite = 0;
            cmd_refresh.nread = 0;

            //set fast refresh
            //ioctl( dfb_fbdev->fd, 901, (unsigned long)1 );

	    if (useFullUpdate && updFullCounter >= useFullUpdate)
            {
		// FullUpdate
                updFullCounter = 0;
		dataOffset = 0;

		// enable refresh
                cmd_refresh.nwrite = 0;
                cmd_refresh.command = 0xf9;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );
	    }
	    else
	    {
		// PartialUpdate
		updFullCounter++;
		dataOffset = 8;

		// disable refresh
                cmd_refresh.nwrite = 0;
                cmd_refresh.command = 0xfa;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

		// set update region
            	cmd_refresh.data[0] = H(0);
            	cmd_refresh.data[1] = L(0);
            	cmd_refresh.data[2] = H(0);
            	cmd_refresh.data[3] = L(0);
            	cmd_refresh.data[4] = H(dfb_fbdev->shared->current_mode.xres-1);
            	cmd_refresh.data[5] = L(dfb_fbdev->shared->current_mode.xres-1);
            	cmd_refresh.data[6] = H(dfb_fbdev->shared->current_mode.yres-1);
            	cmd_refresh.data[7] = L(dfb_fbdev->shared->current_mode.yres-1);
	    }
            cmd_refresh.nwrite = (dfb_fbdev->shared->current_mode.xres*dfb_fbdev->shared->current_mode.yres)/2 + dataOffset;

            for (i = 0; i < dfb_fbdev->shared->current_mode.yres; i++)
            {
                display_buffer_ptr = cmd_refresh.data + dataOffset + (dfb_fbdev->shared->current_mode.xres * i)/2;
                buffer_ptr = dfb_fbdev->framebuffer_base + dfb_fbdev->shared->current_mode.xres * i;
                for (j = 0; j < dfb_fbdev->shared->current_mode.xres; j+=2)
                {
                    unsigned char firstSrc = *(buffer_ptr+j);
                    unsigned char secondSrc = *(buffer_ptr+j+1);
                    unsigned int firstPix = ((firstSrc & 0x3)*11 + ((firstSrc >> 2) & 0x7)*59 + ((firstSrc >> 5) & 0x7)*30 + 4)/44;
                    unsigned int secondPix = ((secondSrc & 0x3)*11 + ((secondSrc >> 2) & 0x7)*59 + ((secondSrc >> 5) & 0x7)*30 + 4)/44;
                    *display_buffer_ptr = ((firstPix & 0x0f) << 4) | (secondPix & 0x0f);
                    display_buffer_ptr++;
                }
            }
	    if (!dataOffset)
            {
		// FullUpdate
		cmd_refresh.command = 0xa0;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

                cmd_refresh.nwrite = 0;
                cmd_refresh.command = 0xa1;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

                cmd_refresh.command = 0xfc;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

                cmd_refresh.command = 0xa2;
                ioctl( dfb_fbdev->fd, 1, &cmd_refresh );
            }
            else
            {
		// PartialUpdate
	    	cmd_refresh.command = 0xb0;
            	ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

            	cmd_refresh.nwrite = 0;
            	cmd_refresh.command = 0xa1;
            	ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

	    	cmd_refresh.command = 0xb2;
            	ioctl( dfb_fbdev->fd, 1, &cmd_refresh );
	    }
        }
    }
}

static inline int getpixel(int x, int y)
{

    unsigned int pix = *((unsigned char *)(dfb_fbdev->framebuffer_base + dfb_fbdev->shared->current_mode.xres * y + x));
    return (((pix & 0x3)*11 + ((pix >> 2) & 0x7)*59 + ((pix >> 5) & 0x7)*30 + 4)/44) & 0x0f;

}

static unsigned char cursor_arrow[] =
{

    0xff, 0xf7, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xf0, 0x0f, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xf0, 0x00, 0xf7, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xf0, 0x00, 0x0f, 0x77, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0x00, 0x00, 0xf7, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0x00, 0x00, 0x0f, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0x00, 0x00, 0x00, 0xf7, 0x77, 0x77, 0x77,
    0x77, 0xf0, 0x00, 0x00, 0x0f, 0x77, 0x77, 0x77,
    0x77, 0xf0, 0x00, 0x00, 0x00, 0xf7, 0x77, 0x77,
    0x77, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0x77, 0x77,
    0x77, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xf7, 0x77,
    0x77, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x77,
    0x77, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf7,
    0x77, 0x77, 0xf0, 0x00, 0x00, 0xff, 0xff, 0x77,
    0x77, 0x77, 0xf0, 0x0f, 0xff, 0x77, 0x77, 0x77,
    0x77, 0x77, 0xf0, 0xf7, 0x77, 0x77, 0x77, 0x77,

};

static unsigned char cursor_shade[] =
{

    0xff, 0xf7, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xff, 0xff, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xff, 0xff, 0xf7, 0x77, 0x77, 0x77, 0x77, 0x77,
    0xff, 0xff, 0xff, 0x77, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0xff, 0xff, 0xf7, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0xff, 0xff, 0xff, 0x77, 0x77, 0x77, 0x77,
    0x7f, 0xff, 0xff, 0xff, 0xf7, 0x77, 0x77, 0x77,
    0x77, 0xff, 0xff, 0xff, 0xff, 0x77, 0x77, 0x77,
    0x77, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x77, 0x77,
    0x77, 0xff, 0xff, 0xff, 0xff, 0xff, 0x77, 0x77,
    0x77, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x77,
    0x77, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x77,
    0x77, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x77, 0x77, 0xff, 0xff, 0xff, 0xff, 0xff, 0x77,
    0x77, 0x77, 0xff, 0xff, 0xff, 0x77, 0x77, 0x77,
    0x77, 0x77, 0xff, 0xf7, 0x77, 0x77, 0x77, 0x77,

};

static long long timeinms()
{

    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((long long)(tv.tv_sec) * 1000LL) + ((long long)(tv.tv_usec) / 1000LL);

}

void directfb_sendevent(H3600TSData *data, DFBInputEvent *event)
{
    DFBInputEvent evt;
    if (event)
    {
        memcpy(&evt, event, sizeof(DFBInputEvent));
        dfb_input_dispatch( data->device, &evt );
    }
}

static inline int dither(int x, int y, int v)
{

    return (v > ((((x & 1) + (y & 1)) << 2) + 3)) ? 0xf : 0;

}

static void showcursor(int x, int y, int shade)
{

    unsigned char tempcursor[16*16/2], *ptr;
    int xx, yy, pix;

    memcpy(tempcursor, cursor_arrow, 16*16/2);
    for (yy=0; yy<16; yy++)
    {
        for (xx=0; xx<16; xx+=2)
        {
            pix = getpixel(x+xx, y+yy);
            ptr = tempcursor + yy * 8 + xx / 2;
            if ((*ptr & 0xf0) == 0x70)
            {
                *ptr &= 0x0f;
                *ptr |= (pix << 4);
            }
            else if (shade)
            {
                *ptr &= 0x0f;
                *ptr |= (dither(x, y, pix) << 4);
            }
            pix = getpixel(x+xx+1, y+yy);
            if ((*ptr & 0x0f) == 0x07)
            {
                *ptr &= 0xf0;
                *ptr |= pix;
            }
            else if (shade)
            {
                *ptr &= 0xf0;
                *ptr |= dither(x, y, pix);
            }
        }
    }

    cmd_refresh.owner = 0;

    cmd_refresh.data[0] = H(x);
    cmd_refresh.data[1] = L(x);
    cmd_refresh.data[2] = H(y);
    cmd_refresh.data[3] = L(y);
    cmd_refresh.data[4] = H(x+15);
    cmd_refresh.data[5] = L(x+15);
    cmd_refresh.data[6] = H(y+15);
    cmd_refresh.data[7] = L(y+15);
    memcpy(cmd_refresh.data+8, tempcursor, (16 * 16) / 2);
    cmd_refresh.command = 0xb0;
    cmd_refresh.nwrite = (16 * 16) / 2 + 8;
    cmd_refresh.nread = 0;
    ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

    cmd_refresh.command = 0xb1;
    cmd_refresh.nwrite = 0;
    cmd_refresh.nread = 0;
    ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

}

#define TRAILLEN 4
static int trailX[TRAILLEN], trailY[TRAILLEN];
static long long trailWait = 0;

static void show_cursor_trail(int x, int y)
{

    int i;

    screen_locked = 1;

    for (i=TRAILLEN-1; i>0; i--)
    {
        trailX[i] = trailX[i-1];
        trailY[i] = trailY[i-1];
    }
    trailX[0] = x;
    trailY[0] = y;

    ioctl( dfb_fbdev->fd, 901, 1 );

    showcursor(x, y, 0);
    if (trailX[TRAILLEN-1] >= 0)
    {
        showcursor(trailX[TRAILLEN-1], trailY[TRAILLEN-1], 1);
        trailX[TRAILLEN-1] = trailY[TRAILLEN-1] = -1;
    }
    trailWait = timeinms();

}

static void cancel_cursor_trail()
{

    ioctl( dfb_fbdev->fd, 901, 0 );
    trailWait = timeinms();

}

static void check_cursor_trail(H3600TSData *data, int force)
{

    DFBInputEvent evt;
    int i;

    if (trailWait == 0) return;
    memset(&evt, 0, sizeof(DFBInputEvent));
    if (force || timeinms() > trailWait + 500)
    {
        evt.type    = DIET_AXISMOTION;
        evt.flags   = DIEF_AXISABS;
        evt.axis    = DIAI_X;
        evt.axisabs = trailX[0];
        directfb_sendevent(data, &evt);

        usleep(50);
        evt.type    = DIET_AXISMOTION;
        evt.flags   = DIEF_AXISABS;
        evt.axis    = DIAI_Y;
        evt.axisabs = trailY[0];
        directfb_sendevent(data, &evt);

        for (i=0; i<TRAILLEN; i++) trailX[i] = trailY[i] = -1;
        trailWait = 0;

        screen_locked = 0;
        if (update_needed) update_screen();

    }
    else if ( timeinms() > trailWait + 300)
    {
        for (i=TRAILLEN-1; i>0; i--)
        {
            if (trailX[i] >= 0)
            {
                showcursor(trailX[i], trailY[i], 1);
                trailX[i] = trailY[i] = -1;
            }
        }
    }


}

static int checkKeyBoardEvent(int *lastState, int *keyState, int *keyCode, int *keyRepeatCount)
{
    static long long timeWait = 0;

    int result = 0;
    int evt_key_type = 0, evt_key_par1 = 0, evt_key_par2 = 0, mod = 0;
    long long currentTime = timeinms();

    if (hw_nextevent_key(&evt_key_type, &mod, &evt_key_par1, &evt_key_par2))
    {
        if (evt_key_type == EVT_KEYDOWN)
        {
            *lastState = evt_key_par1;
            *keyCode = evt_key_par1;
            *keyState = EVT_KEYDOWN;
            timeWait = currentTime;
            *keyRepeatCount = 0;
            result = 1;
        }
        else if (evt_key_type == EVT_KEYUP)
        {
            *keyCode = evt_key_par1;
            *keyState = EVT_KEYUP;
            *lastState = 0;
            result = 1;
        }
    }
    else if (*lastState != 0)
    {
        if (currentTime - timeWait > 0)
        {
            timeWait = currentTime + 100;
            *keyState = EVT_KEYREPEAT;
            *keyCode = *lastState;
            *keyRepeatCount = *keyRepeatCount + 1;
            result = 1;
        }
    }
    return result;
}

void ResetAccelerate(int *accelerate)
{
    *accelerate = 0;
}

int GetAccelerate(int *accelerate)
{
    *accelerate = *accelerate + 1;
    if (*accelerate == 1) return 8;
    if (*accelerate == 2) return 0;
    if (*accelerate == 3) return 0;
    if (*accelerate >= 4 && *accelerate <= 8) return 16;
    if (*accelerate >= 9 && *accelerate <= 14) return 32;
    return 48;
}

int hw_nextevent_tsink(int *laststate, int *type, int *par1, int *par2)
{
	int result = 0, i_type, i_par1, i_par2, mod;
	if (hw_nextevent_mon(&i_type, &mod, &i_par1, &i_par2))
	{
		// create event filter. because we have some problems with touch keyboard
		if ((i_type == EVT_POINTERDOWN && laststate != EVT_POINTERDOWN) ||
		    (i_type == EVT_POINTERUP && laststate != EVT_POINTERUP) ||
		    (i_type == EVT_POINTERMOVE && (laststate == EVT_POINTERDOWN || laststate == EVT_POINTERMOVE)))
		{
			result = 1;
		}
		if (result)
		{
			*type = i_type;
			*par1 = i_par1;
			*par2 = i_par2;
			*laststate = i_type;
		}
	}
	return result;
}

void make_tab_navi(H3600TSData *data, DFBInputEvent *evt, int *flag, int direction)
{
	if (data && evt && flag)
	{
		if (direction)
		{
                            evt->key_id = DIKI_TAB;
		}
		else
		{
                            if ( evt->type == DIET_KEYPRESS )
                            {
                                evt->key_id = DIKI_SHIFT_L;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_TAB;
                            }
                            else //evt.type == DIET_KEYRELEASE;
                            {
                                evt->key_id = DIKI_TAB;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_SHIFT_L;
                            }
		}
		*flag = 1;
	}
}

void make_page_navi(H3600TSData *data, DFBInputEvent *evt, int direction)
{
	if (data && evt)
	{
		if (direction)
		{
                            if ( evt->type == DIET_KEYPRESS )
                            {
                                evt->key_id = DIKI_ALT_L;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_RIGHT;
                            }
                            else //evt.type == DIET_KEYRELEASE;
                            {
                                evt->key_id = DIKI_RIGHT;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_ALT_L;
                            }
		}
		else
		{
                            if ( evt->type == DIET_KEYPRESS )
                            {
                                evt->key_id = DIKI_ALT_L;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_LEFT;
                            }
                            else //evt.type == DIET_KEYRELEASE;
                            {
                                evt->key_id = DIKI_LEFT;
                                directfb_sendevent(data, evt);
                                evt->key_id = DIKI_ALT_L;
                            }
		}
	}
}

static void *
h3600tsEventThread( DirectThread *thread, void *driver_data )
{
    H3600TSData *data = (H3600TSData*) driver_data;

    DFBInputEvent evt;
    struct tsdata ts_event;
    int readlen;
    int isTouchUsed;
    unsigned short old_pressure = 0;
    int old_x;
    int old_y;
    int lastState = 0;
    int lastStateTS = 0;
    int mouse_button = 0;
    int evt_key_type = 0;
    int evt_key_par1 = 0;
    int evt_key_par2 = 0;
    int repeatCounter = 0;
    int isTabNavigation = 1;
    int cursor_step;
    int accelerate;
    unsigned long ticker = 0;

    chksum_crc32gentab();
    dfb_fbdev = dfb_system_data();

    OpenScreen();

    //disable refresh();
    cmd_refresh.command = 0xfa;
    ioctl( dfb_fbdev->fd, 1, &cmd_refresh );

    //set depth
    cmd_refresh.command = 0xf3;
    cmd_refresh.nwrite = 1;
    cmd_refresh.data[0] = 0x04;
    ioctl( dfb_fbdev->fd, 1, &cmd_refresh );
    cmd_refresh.nwrite = 0;

    //set orientation
    SetOrientation(0);
    ClearScreen();
    ShowHourglass();
    FullUpdate();
    /*
        cmd_refresh.command = 0xf5;
        cmd_refresh.nwrite = 1;
        cmd_refresh.data[0] = 0x02;
        ioctl( dfb_fbdev->fd, 1, &cmd_refresh );
        cmd_refresh.nwrite = 0;
    */
    if (!strcmp("ibseReader", GetDefaultUserAgent()))
	useFullUpdate = ReadInt(GetGlobalConfig(), "invertupdate", 1);
 
    isTouchUsed = hw_touchpanel_ready();
    if (isBooklandMode)
    {
	old_x = dfb_fbdev->shared->current_mode.xres;
	old_y = dfb_fbdev->shared->current_mode.yres;
    }
    else
    {
        old_x = dfb_fbdev->shared->current_mode.xres >> 1;
        old_y = dfb_fbdev->shared->current_mode.yres >> 1;
    }

    memset(&evt, 0, sizeof(DFBInputEvent));

    evt.type    = DIET_AXISMOTION;
    evt.flags   = DIEF_AXISABS;
    evt.axis    = DIAI_X;
    evt.axisabs = old_x;
    directfb_sendevent(data, &evt);

    usleep(50);
    evt.type    = DIET_AXISMOTION;
    evt.flags   = DIEF_AXISABS;
    evt.axis    = DIAI_Y;
    evt.axisabs = old_y;
    directfb_sendevent(data, &evt);

    while (true)
    {

        FILE *fff = fopen("/tmp/midorialive", "w");
        fclose(fff);

        direct_thread_testcancel( thread );

        memset(&evt, 0, sizeof(DFBInputEvent));

        if (isTouchUsed &&
	    hw_nextevent_tsink(&lastStateTS, &evt_key_type, &evt_key_par1, &evt_key_par2) &&
	    (evt_key_type != EVT_POINTERUP))
        {
            ts_event.x = ivstate.tsc.a / 10000 + (evt_key_par1 * ivstate.tsc.b) / 10000 + (evt_key_par2 * ivstate.tsc.c) / 10000;
            ts_event.y = ivstate.tsc.d / 10000 + (evt_key_par1 * ivstate.tsc.e) / 10000 + (evt_key_par2 * ivstate.tsc.f) / 10000;

            if ( !(ivstate.uiflags & IN_KEYBOARD) )
            {
                evt.type    = DIET_AXISMOTION;
                evt.flags   = DIEF_AXISABS;
                evt.axis    = DIAI_X;
                evt.axisabs = ts_event.x;

                directfb_sendevent(data, &evt);

                evt.type    = DIET_AXISMOTION;
                evt.flags   = DIEF_AXISABS;
                evt.axis    = DIAI_Y;
                evt.axisabs = ts_event.y;

                directfb_sendevent(data, &evt);

                if (0 == old_pressure)
                {
                    memset(&evt, 0, sizeof(DFBInputEvent));
                    evt.type   = DIET_BUTTONPRESS;
                    evt.flags  = DIEF_NONE;
                    evt.button = DIBI_LEFT;

                    directfb_sendevent(data, &evt);
                    usleep(50);
                    old_pressure = 1;
                }
            }
            else
            {
                if (0 == old_pressure)
                {
                    iv_handler handler = GetEventHandler();
                    if (handler)
                    {
                        old_x = ts_event.x;
                        old_y = ts_event.y;
                        (*handler)(EVT_POINTERDOWN, ts_event.x, ts_event.y);
                    }
                    old_pressure = 1;
                }
            }
        }
        else
        {
            if (1 == old_pressure)
            {
                if ( !(ivstate.uiflags & IN_KEYBOARD) )
                {
                    evt.type   = DIET_BUTTONRELEASE;
                    evt.flags  = DIEF_NONE;
                    evt.button = DIBI_LEFT;

                    directfb_sendevent(data, &evt);
                    usleep(50);
                }
                else
                {
                    iv_handler handler = GetEventHandler();
                    if (handler)
                    {
                        (*handler)(EVT_POINTERUP, old_x, old_y);
                    }
                }
                old_pressure = 0;
            }
        }


        // keyboard events
        if (checkKeyBoardEvent(&lastState, &evt_key_type, &evt_key_par1, &repeatCounter))
        {
            if (ivstate.uiflags & IN_KEYBOARD)
            {
                iv_handler handler = GetEventHandler();
                // don't send KEYREPEAT, because text enter would be unstable
                if (handler && evt_key_type != EVT_KEYREPEAT)
                {
                    //(*handler)(evt_key_type, evt_key_par1, repeatCounter);
                    (*handler)(evt_key_type, evt_key_par1, 0);
                }
            }
            else
            {
                memset(&evt, 0, sizeof(DFBInputEvent));

                // dirty hack

                if (evt_key_par1 == KEY_OK || evt_key_par1 == KEY_BACK || evt_key_par1 == KEY_HOME || evt_key_par1 == KEY_NEXT || evt_key_par1 == KEY_PREV || isTouchUsed != 0 ||
		    (isBooklandMode && (evt_key_par1 == KEY_UP || evt_key_par1 == KEY_DOWN || evt_key_par1 == KEY_LEFT || evt_key_par1 == KEY_RIGHT)))
                {
                    check_cursor_trail(data, 1);

                    // handle key back and ok button
                    if (evt_key_type == EVT_KEYDOWN)
                    {
                        evt.type = DIET_KEYPRESS;
                    }
                    else if (evt_key_type == EVT_KEYUP)
                    {
                        evt.type = DIET_KEYRELEASE;
                    }
                    if (evt.type != 0)
                    {
                        evt.flags = DIEF_KEYID;
                        evt.key_id = 0;
                        mouse_button = 0;
                        switch (evt_key_par1)
                        {
                        case KEY_UP:
			    if (isBooklandMode) make_tab_navi(data, &evt, &isTabNavigation, 0);
			    else evt.key_id = DIKI_PAGE_UP;
                            break;
                        case KEY_LEFT:
		            if (isBooklandMode) 
			    {
				if (isTabNavigation) evt.key_id = DIKI_UP;
				else make_tab_navi(data, &evt, &isTabNavigation, 0);
			    }
                            else evt.key_id = DIKI_LEFT;
                            break;
                        case KEY_DOWN:
			    if (isBooklandMode) make_tab_navi(data, &evt, &isTabNavigation, 1);
                            else evt.key_id = DIKI_PAGE_DOWN;
                            break;
                        case KEY_RIGHT:
                            if (isBooklandMode)
			    {
				if (isTabNavigation) evt.key_id = DIKI_DOWN;
				else make_tab_navi(data, &evt, &isTabNavigation, 1);
			    }
                            else evt.key_id = DIKI_RIGHT;
                            break;
                        case KEY_NEXT:
			    if (isBooklandMode) make_page_navi(data, &evt, 1);
			    else make_tab_navi(data, &evt, &isTabNavigation, 1);
                            break;
                        case KEY_PREV:
			    if (isBooklandMode) make_page_navi(data, &evt, 0);
			    else make_tab_navi(data, &evt, &isTabNavigation, 0);
                            break;
                        case KEY_MENU:
                            break;
                        case KEY_HOME:
                            if ( evt.type == DIET_KEYPRESS )
                            {
                                evt.key_id = DIKI_ALT_L;
                                directfb_sendevent(data, &evt);
                                evt.key_id = DIKI_HOME;
                            }
                            else //evt.type == DIET_KEYRELEASE;
                            {
                                evt.key_id = DIKI_HOME;
                                directfb_sendevent(data, &evt);
                                evt.key_id = DIKI_ALT_L;
                            }
                            break;
                        case KEY_OK:
                            if (isTabNavigation == 0)
                            {
                                evt.flags  = DIEF_NONE;
                                evt.button = DIBI_LEFT;
                                if ( evt.type == DIET_KEYPRESS )
                                {
                                    evt.type   = DIET_BUTTONPRESS;
                                }
                                else //evt.type == DIET_KEYRELEASE;
                                {
                                    evt.type   = DIET_BUTTONRELEASE;
                                }
                                mouse_button = 1;
                            }
                            else
                            {
                                evt.key_id = DIKI_ENTER;
                            }
                            break;
                        case KEY_BACK:
                            if ( evt.type == DIET_KEYPRESS )
                            {
                                evt.key_id = DIKI_CONTROL_L;
                                directfb_sendevent(data, &evt);
                                evt.key_id = DIKI_Q;
                            }
                            else //evt.type == DIET_KEYRELEASE;
                            {
                                evt.key_id = DIKI_Q;
                                directfb_sendevent(data, &evt);
                                evt.key_id = DIKI_CONTROL_L;
                            }
                            break;
                        default:
                            break;
                        }
                        if (evt.key_id != 0 ||
                                mouse_button != 0)
                        {
                            usleep(50);
                            directfb_sendevent(data, &evt);
                        }
                    }
                }
                else
                {
                    // disable cursor navigation in bookland mode
                    if (!isBooklandMode)
                    {
                        // cursor navigation support if no touchpanel
                        switch (evt_key_type)
                        {
                        case EVT_KEYDOWN:
                            ResetAccelerate(&accelerate);
                        case EVT_KEYREPEAT:

                            evt.type = DIET_KEYPRESS;
                            evt.flags = DIEF_KEYID;
                            evt.key_id = 0;

                            cursor_step = GetAccelerate(&accelerate);
                            switch (evt_key_par1)
                            {
                            case KEY_UP:
                                if (old_y > 0)
                                {
                                    if (old_y < cursor_step) old_y = 0;
                                    else old_y -= cursor_step;
                                }
                                else
                                {
                                    evt.key_id = DIKI_PAGE_UP;
                                }
                                isTabNavigation = 0;
                                break;
                            case KEY_DOWN:
                                if (old_y < dfb_fbdev->shared->current_mode.yres)
                                {
                                    if (dfb_fbdev->shared->current_mode.yres - old_y < cursor_step) old_y = dfb_fbdev->shared->current_mode.yres;
                                    else old_y += cursor_step;
                                }
                                else
                                {
                                    evt.key_id = DIKI_PAGE_DOWN;
                                }
                                isTabNavigation = 0;
                                break;
                            case KEY_LEFT:
                                if (old_x > 0)
                                {
                                    if (old_x < cursor_step) old_x = 0;
                                    else old_x -= cursor_step;
                                }
                                else
                                {
                                    evt.key_id = DIKI_LEFT;
                                }
                                isTabNavigation = 0;
                                break;
                            case KEY_RIGHT:
                                if (old_x < dfb_fbdev->shared->current_mode.xres)
                                {
                                    if (dfb_fbdev->shared->current_mode.xres - old_x < cursor_step) old_x = dfb_fbdev->shared->current_mode.xres;
                                    else old_x += cursor_step;
                                }
                                else
                                {
                                    evt.key_id = DIKI_RIGHT;
                                }
                                isTabNavigation = 0;
                                break;
                            default:
                                break;
                            }

                            if (evt.key_id != 0)
                            {
                                directfb_sendevent(data, &evt);
                                usleep(50);
                                evt.key_symbol = 0;
                                evt.key_code = 0;
                                evt.type = DIET_KEYRELEASE;
                                directfb_sendevent(data, &evt);
                            }
                            else
                            {
                                old_x = (old_x + 2) & ~3;
                                old_y = (old_y + 2) & ~3;
                                show_cursor_trail(old_x, old_y);

                            }
                            break;
                        case EVT_KEYUP:
                            cancel_cursor_trail();

                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        check_cursor_trail(data, 0);

        if ( ticker > 10 )
        {
            update_screen();
            ticker = 0;
        }
        ticker++;

        usleep(50);
    }

    if (readlen <= 0)
        D_PERROR ("H3600 Touchscreen thread died\n");

    return NULL;
}


/* exported symbols */

static int driver_get_available( void )
{
    int fd;
    return 1;
}

static void
driver_get_info( InputDriverInfo *info )
{
    /* fill driver info structure */
    snprintf( info->name,
              DFB_INPUT_DRIVER_INFO_NAME_LENGTH, "H3600 Touchscreen Driver" );

    snprintf( info->vendor,
              DFB_INPUT_DRIVER_INFO_VENDOR_LENGTH, "directfb.org" );

    info->version.major = 0;
    info->version.minor = 2;
}

static DFBResult
driver_open_device( CoreInputDevice      *device,
                    unsigned int      number,
                    InputDeviceInfo  *info,
                    void            **driver_data )
{
    int          fd;
    H3600TSData *data;

    /* fill device info structure */
    snprintf( info->desc.name,
              DFB_INPUT_DEVICE_DESC_NAME_LENGTH, "H3600 Touchscreen" );

    snprintf( info->desc.vendor,
              DFB_INPUT_DEVICE_DESC_VENDOR_LENGTH, "Unknown" );

    //info->prefered_id     = DIDID_ANY; //DIDID_MOUSE;
    info->prefered_id     = DIDID_KEYBOARD;

    info->desc.type       = DIDTF_MOUSE | DIDTF_REMOTE | DIDTF_KEYBOARD;
    info->desc.caps       = DICAPS_AXES | DICAPS_BUTTONS | DICAPS_KEYS;
    info->desc.max_axis   = DIAI_Y;
    info->desc.max_button = DIBI_LEFT;

    /* allocate and fill private data */
    data = D_CALLOC( 1, sizeof(H3600TSData) );
    if (!data)
    {
        close( fd );
        return D_OOM();
    }

    data->fd     = fd;
    data->device = device;

    char *booklandenv = getenv("BOOKLANDMODE");
    if (booklandenv && strcmp(booklandenv, "on") == 0)
        isBooklandMode = 1;
    else
        isBooklandMode = 0;

    /* start input thread */
    data->thread = direct_thread_create( DTT_INPUT, h3600tsEventThread, data, "H3600 TS Input" );

    /* set private data pointer */
    *driver_data = data;

    return DFB_OK;
}

/*
 * Fetch one entry from the device's keymap if supported.
 */
static DFBResult
driver_get_keymap_entry( CoreInputDevice               *device,
                         void                      *driver_data,
                         DFBInputDeviceKeymapEntry *entry )
{
    return DFB_UNSUPPORTED;
}

static void
driver_close_device( void *driver_data )
{
    H3600TSData *data = (H3600TSData*) driver_data;

    /* stop input thread */
    direct_thread_cancel( data->thread );
    direct_thread_join( data->thread );
    direct_thread_destroy( data->thread );

    /* close device */
    if (close( data->fd ) < 0)
        D_PERROR( "DirectFB/H3600: Error closing `%s'!\n", dfb_config->h3600_device ? : "/dev/ts" );

    /* free private data */
    D_FREE( data );
}

