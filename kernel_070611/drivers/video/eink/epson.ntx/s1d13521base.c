//-----------------------------------------------------------------------------
//
// linux/drivers/video/epson/s1d13521fb.c -- frame buffer driver for Epson
// S1D13521 series of LCD controllers.
//
// Copyright(c) Seiko Epson Corporation 2000-2008.
// All rights reserved.
//
// This file is subject to the terms and conditions of the GNU General Public
// License. See the file COPYING in the main directory of this archive for
// more details.
//
//----------------------------------------------------------------------------

#define S1D13521FB_VERSION              "S1D13521FB: $Revision: 0 $"
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>

#ifdef CONFIG_FB_EPSON_PCI
    #include <linux/pci.h>
#endif

#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

//========================================================================
#include <linux/platform_device.h>

#include <plat/regs-gpio.h>

//------------------------------------------------------------------------
#include "s1d13521fb.h"
#include "access.h"
//#include "s1d13521ioctl.h"
#include <linux/timer.h>

//========================================================================
#define TimeOutValue    	0x1000000
#define TAckTimeOutValue 	100

#undef VIDEO_REFRESH_PERIOD

// This is the refresh period for updating the display for RAM based LCDs
// and/or indirect interfaces.  It is set to (1 second / X).  This value can
// modified by the end-user.  Be aware that decreasing the refresh period
// increases CPU usage as well.
// There are HZ (typically 100) jiffies per second
#if CONFIG_FB_EPSON_VIRTUAL_FRAMEBUFFER_FREQ != 0
#define VIDEO_REFRESH_PERIOD   (HZ/CONFIG_FB_EPSON_VIRTUAL_FRAMEBUFFER_FREQ)
#endif

// This section only reports the configuration settings
#ifdef CONFIG_FB_EPSON_SHOW_SETTINGS
        #warning ###################################################################
        #warning Epson S1D13521 Frame Buffer Configuration:

        #ifdef CONFIG_FB_EPSON_PCI
                #warning Using PCI interface
        #else
                #warning Not using PCI interface
        #endif

        #ifdef CONFIG_FB_EPSON_GPIO_GUMSTIX
                #warning Gumstix/Broadsheet port using GPIO pins
        #elif  CONFIG_FB_EPSON_NTX_EBOOK_2440
                #warning Netronix eBook S3C2440 platform with GPIO
        #else
                #warning Not a Gumstix/Broadsheet port
        #endif

        #ifdef CONFIG_FB_EPSON_PROC
                #warning Adding /proc functions
        #else
                #warning No /proc functions
        #endif

        #ifdef CONFIG_FB_EPSON_DEBUG_PRINTK
                #warning Enable debugging printk() calls
        #else
                #warning No debugging printk() calls
        #endif

        #ifdef CONFIG_FB_EPSON_BLACK_AND_WHITE
                #warning Virtual Framebuffer Black And White
        #else
//                #warning Virtual Framebuffer 16 Shades of Gray
                #warning Virtual Framebuffer 8/16 Shades of Gray
        #endif

        #ifdef VIDEO_REFRESH_PERIOD
                #warning Timer video refresh ENABLED.
        #else
                #warning Timer video refresh DISABLED.
        #endif

        #ifdef CONFIG_FB_EPSON_HRDY_OK
                #warning Assuming HRDY signal present
        #else
                #warning Assuming HRDY signal NOT present.
        #endif
        #warning ###################################################################
#endif

//-----------------------------------------------------------------------------
//
// Local Definitions
//
//---------------------------------------------------------------------------

#define S1D_MMAP_PHYSICAL_REG_SIZE      sizeof(S1D13XXXFB_IND_STRUCT)
#define VFB_SIZE_BYTES ((S1D_DISPLAY_WIDTH*S1D_DISPLAY_HEIGHT*S1D_DISPLAY_BPP)/8)
PVOID				pFrameData;

//-----------------------------------------------------------------------------
//
// Function Prototypes
//
//-----------------------------------------------------------------------------
int  __devinit s1d13521fb_init(void);
int  __devinit s1d13521fb_setup(char *options, int *ints);
void __exit s1d13521fb_exit(void);

static int  s1d13521fb_set_par(struct fb_info *info);
static void s1d13521fb_fillrect(struct fb_info *p, const struct fb_fillrect *rect);
static void s1d13521fb_copyarea(struct fb_info *p, const struct fb_copyarea *area);
static void s1d13521fb_imageblit(struct fb_info *p, const struct fb_image *image);
static int  s1d13521fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int  s1d13521fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info);
static int  s1d13521fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
static int  s1d13521fb_blank(int blank_mode, struct fb_info *info);
static int  s1d13521fb_mmap(struct fb_info *info, struct vm_area_struct *vma);
static int  s1d13521fb_set_virtual_framebuffer(void);
#ifdef VIDEO_REFRESH_PERIOD
static void s1d13521fb_refresh_display(unsigned long dummy);
#endif

//===================================================================================
static int s1d13521fb_probe(struct platform_device *dev);
static int s1d13521fb_remove(struct platform_device *dev);
static int s1d13521fb_suspend(struct platform_device *dev, pm_message_t state);
static int s1d13521fb_resume(struct platform_device *dev);
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Globals
//
//-----------------------------------------------------------------------------

char *s1d13521fb_version  = S1D13521FB_VERSION;
static int gDisplayChange = 0;          // Display update needed flag

static struct fb_ops s1d13521fb_ops =
{
        .owner          = THIS_MODULE,
        .fb_set_par     = s1d13521fb_set_par,
        .fb_check_var   = s1d13521fb_check_var,
        .fb_setcolreg   = s1d13521fb_setcolreg,
        .fb_fillrect    = s1d13521fb_fillrect,
        .fb_copyarea    = s1d13521fb_copyarea,
        .fb_imageblit   = s1d13521fb_imageblit,
//      .fb_cursor      = soft_cursor,
        .fb_mmap        = s1d13521fb_mmap,
        .fb_ioctl       = s1d13521fb_ioctl,
};

struct fb_fix_screeninfo s1d13521fb_fix =
{
        .id             = "s1d13521",
        .type           = FB_TYPE_PACKED_PIXELS,
        .type_aux       = 0,
        .xpanstep       = 0,
        .ypanstep       = 0,
        .ywrapstep      = 0,
        .smem_len       = S1D_DISPLAY_WIDTH*S1D_DISPLAY_HEIGHT*S1D_DISPLAY_BPP,
        .line_length    = S1D_DISPLAY_WIDTH*S1D_DISPLAY_BPP/8,
        .accel          = FB_ACCEL_NONE,
};

struct fb_info   s1d13521_fb;
FB_INFO_S1D13521 s1d13521fb_info;

#ifdef VIDEO_REFRESH_PERIOD
  struct timer_list s1d13521_timer;
#endif

static int suspended = 0;

///////////////////////////////////////////////////////////////////////////////
//void __exit s1d13521fb_exit(void)
static int s1d13521fb_remove(struct platform_device *dev)
{
#ifdef CONFIG_FB_EPSON_PROC
        s1d13521proc_terminate();
#endif
#ifdef VIDEO_REFRESH_PERIOD
        del_timer(&s1d13521_timer);
#endif
        unregister_framebuffer(&s1d13521_fb);
        pvi_Deinit();
  	return 0;
}

//-----------------------------------------------------------------------------
// Parse user specified options (`video=s1d13521:')
// Example:
// video=s1d13521:noaccel
//-----------------------------------------------------------------------------
int __init s1d13521fb_setup(char *options, int *ints)
{
        return 0;
}

//-----------------------------------------------------------------------------
//
// Fill in the 'var' and 'fix' structure.
//
//-----------------------------------------------------------------------------
static int s1d13521fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
        var->xres               = S1D_DISPLAY_WIDTH;
        var->yres               = S1D_DISPLAY_HEIGHT;
        var->xres_virtual       = var->xres;
        var->yres_virtual       = var->yres;
        var->xoffset            = var->yoffset = 0;
        var->bits_per_pixel     = S1D_DISPLAY_BPP;
        var->grayscale          = 1;
        var->nonstd             = 0;                    /* != 0 Non standard pixel format */
        var->activate           = FB_ACTIVATE_NOW;      /* see FB_ACTIVATE_*             */
        var->height             = -1;                   /* height of picture in mm       */
        var->width              = -1;                   /* width of picture in mm        */
        var->accel_flags        = 0;                    /* acceleration flags (hints     */
        var->pixclock           = S1D_DISPLAY_PCLK;
        var->right_margin       = 0;
        var->lower_margin       = 0;
        var->hsync_len          = 0;
        var->vsync_len          = 0;
        var->left_margin        = 0;
        var->upper_margin       = 0;
        var->sync               = 0;
        var->vmode              = FB_VMODE_NONINTERLACED;
        var->red.msb_right      = var->green.msb_right = var->blue.msb_right = 0;
        var->transp.offset      = var->transp.length = var->transp.msb_right = 0;

        s1d13521fb_fix.visual = FB_VISUAL_TRUECOLOR;

        switch (info->var.bits_per_pixel)
        {
                case 1:
                case 2:
                case 4:
                case 5:
                case 8:
                        var->red.offset  = var->green.offset = var->blue.offset = 0;
                        var->red.length  = var->green.length = var->blue.length = S1D_DISPLAY_BPP;
                        break;

                default:
                        printk(KERN_WARNING "%dbpp is not supported.\n",
                                        info->var.bits_per_pixel);
                        return -EINVAL;
        }

        return 0;
}


//----------------------------------------------------------------------------
// Set a single color register. The values supplied have a 16 bit
// magnitude.
// Return != 0 for invalid regno.
//
// We get called even if we specified that we don't have a programmable palette
// or in direct/true color modes!
//----------------------------------------------------------------------------
static int s1d13521fb_setcolreg(unsigned regno, unsigned red, unsigned green,
                                unsigned blue, unsigned transp, struct fb_info *info)
{
        // Make the first 16 LUT entries available to the console
        if (info->var.bits_per_pixel == 8 && s1d13521fb_fix.visual == FB_VISUAL_TRUECOLOR)
        {
                if (regno < 16)
                {
                        // G= 30%R + 59%G + 11%B
                        unsigned gray = (red*30 + green*59 + blue*11)/100;
                        gray = (gray>>8) & 0xFF;

#ifdef CONFIG_FB_EPSON_BLACK_AND_WHITE
                        if (gray != 0)
                                gray = 0xFF;
#endif

                        // invert: black on white
                        gray = 0xFF-gray;
                        ((u32*)info->pseudo_palette)[regno] = gray;

                }
        }
        else
                return 1;

        return 0;
}

//-----------------------------------------------------------------------------
//
// Set the hardware.
//
//-----------------------------------------------------------------------------
static int s1d13521fb_set_par(struct fb_info *info)
{
        info->fix = s1d13521fb_fix;
        info->fix.mmio_start = (unsigned long)virt_to_phys((void*)s1d13521fb_info.RegAddr);
        info->fix.mmio_len   = s1d13521fb_info.RegAddrMappedSize; //S1D_MMAP_PHYSICAL_REG_SIZE;
        info->fix.smem_start = virt_to_phys((void*)s1d13521fb_info.VirtualFramebufferAddr);
        info->screen_base    = (unsigned char*)s1d13521fb_info.VirtualFramebufferAddr;
        info->screen_size    = (S1D_DISPLAY_WIDTH*S1D_DISPLAY_HEIGHT*S1D_DISPLAY_BPP)/8;
        return 0;
}

//----------------------------------------------------------------------------
//
// PRIVATE FUNCTION:
// Remaps virtual framebuffer from virtual memory space to a physical space.
//
//      If no virtual framebuffer is used, the default fb_mmap routine should
//      be fine, unless there is a concern over its use of
//              io_remap_pfn_range versus remap_pfn_range
//
//----------------------------------------------------------------------------
static int s1d13521fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
        unsigned long off;
        unsigned long start;
        u32 len;

        off = vma->vm_pgoff << PAGE_SHIFT;

        // frame buffer memory
        start = info->fix.smem_start;
        len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);

        if (off >= len)
        {
                // memory mapped io
                off -= len;

                if (info->var.accel_flags)
                        return -EINVAL;

                start = info->fix.mmio_start;
                len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
        }

        start &= PAGE_MASK;

        if ((vma->vm_end - vma->vm_start + off) > len)
                return -EINVAL;

        off += start;
        vma->vm_pgoff = off >> PAGE_SHIFT;

        // This is an IO map - tell maydump to skip this VMA
        vma->vm_flags |= VM_RESERVED;

        if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                             vma->vm_end - vma->vm_start, vma->vm_page_prot))
                return -EAGAIN;

        return 0;
}

//----------------------------------------------------------------------------
// PRIVATE FUNCTION:
// Allocates virtual framebuffer.
//----------------------------------------------------------------------------
static int s1d13521fb_set_virtual_framebuffer(void)
{
        u32 order = 0;
        u32 size = VFB_SIZE_BYTES;
        u32 addr;

        while (size > (PAGE_SIZE * (1 << order)))
                order++;

        s1d13521fb_info.VirtualFramebufferAddr = __get_free_pages(GFP_KERNEL, order);

        if (s1d13521fb_info.VirtualFramebufferAddr == 0)
        {
  		printk("%s(): Could not allocate memory for virtual display buffer.\n", __FUNCTION__);
                return 1;
        }

        for (addr = s1d13521fb_info.VirtualFramebufferAddr; addr < (s1d13521fb_info.VirtualFramebufferAddr+size); addr += PAGE_SIZE)
                SetPageReserved(virt_to_page(addr));

        return 0;
}

//----------------------------------------------------------------------------
// s1d13521fb_do_refresh_display(void): unconditionally refresh display:
// This function updates the display
//----------------------------------------------------------------------------
#define VFB_GC_COUNT    60
static int gGCCount = VFB_GC_COUNT;

//#define VFB_IDLE_COUNT  120
//static int gIdleCount = 0;

void BusIssueDoRefreshDisplay(unsigned cmd,unsigned mode)
{
#if 0
        // Copy virtual framebuffer to display framebuffer.
        s1d13521_ioctl_cmd_params cmd_params;
        u32     size16  = VFB_SIZE_BYTES/4;
        u16 *   pSource = (u16*) s1d13521fb_info.VirtualFramebufferAddr;

        unsigned reg330 = BusIssueReadReg(0x330);
        BusIssueWriteReg(0x330,0x84);              // LUT AutoSelect + P4N

        // Copy virtual framebuffer to hardware via indirect interface burst mode write
        BusIssueCmd(WAIT_DSPE_TRG,&cmd_params,0);
        cmd_params.param[0] = (0x3<<4);
        BusIssueCmd(LD_IMG,&cmd_params,1);

        cmd_params.param[0] = 0x154;
        BusIssueCmd(WR_REG,&cmd_params,1);
        BusIssueWriteBuf(pSource,size16);

        BusIssueCmd(LD_IMG_END,&cmd_params,0);

        cmd_params.param[0] = (mode<<8);
        BusIssueCmd(cmd,&cmd_params,1);              // update all pixels

        BusIssueCmd(WAIT_DSPE_TRG,&cmd_params,0);
        BusIssueCmd(WAIT_DSPE_FREND,&cmd_params,0);
        BusIssueWriteReg(0x330,reg330);            // restore the original reg330
#endif
}

//----------------------------------------------------------------------------
// PRIVATE FUNCTION:
// This function updates the display
//----------------------------------------------------------------------------
#ifdef VIDEO_REFRESH_PERIOD
static void s1d13521fb_refresh_display(unsigned long dummy)
{
        if (gDisplayChange != 0 && s1d13521fb_info.blank_mode == VESA_NO_BLANKING)
        {
                unsigned mode,cmd;

                // once in a while do an accurate update...
                if (--gGCCount != 0)
                {
#ifdef CONFIG_FB_EPSON_BLACK_AND_WHITE
                        mode = WF_MODE_PU;
#else
                        mode = WF_MODE_GU;
#endif
                        cmd = UPD_PART;
                }
                else
                {
                        mode = WF_MODE_GC;
                        cmd = UPD_FULL;
                        gGCCount = VFB_GC_COUNT;
                }

//                BusIssueDoRefreshDisplay(cmd,mode);
                gDisplayChange = 0;
//                gIdleCount = 0;
        }
//        else
//        {
//                // force a full update once in a while
//                if (gIdleCount++ >= VFB_IDLE_COUNT)
//                        gDisplayChange = 1;
//        }

        // Reset the timer
        s1d13521_timer.expires = jiffies+VIDEO_REFRESH_PERIOD;
        add_timer(&s1d13521_timer);
}
#endif


static int s1d13521fb_suspend(struct platform_device *dev, pm_message_t state)
{
//	printk("[Arron] s1d13521fb_suspend()\n");
	pvi_GoToSleep();
	return 0;
}

static int s1d13521fb_resume(struct platform_device *dev)
{
//	printk("[Arron] s1d13521fb_resume()\n");
	pvi_GoToNormal();
	return 0;
}

//-----------------------------------------------------------------------------
// Initialize the chip and the frame buffer driver.
//-----------------------------------------------------------------------------
//int s1d13521fb_init(void)
static int s1d13521fb_probe(struct platform_device *dev)
{
        s1d13521fb_set_par(&s1d13521_fb);

        // Allocate the virtual framebuffer
        if (s1d13521fb_set_virtual_framebuffer())
        {
                printk(KERN_WARNING "s1d13521fb_init: _get_free_pages() failed.\n");
                return -EINVAL;
        }

        //-------------------------------------------------------------------------
        // Set the controller registers and initialize the display
        //-------------------------------------------------------------------------

	BusIssueInitHW();
	pvi_Init();

        //-------------------------------------------------------------------------
        // Initialize Hardware Display Blank
        //-------------------------------------------------------------------------

        s1d13521fb_info.blank_mode = VESA_NO_BLANKING;
        s1d13521fb_ops.fb_blank = s1d13521fb_blank;

        // Set flags for controller supported features

        s1d13521_fb.flags = FBINFO_FLAG_DEFAULT;

        // Set the pseudo palette

        s1d13521_fb.pseudo_palette = s1d13521fb_info.pseudo_palette;

        s1d13521fb_check_var(&s1d13521_fb.var, &s1d13521_fb);
        s1d13521_fb.fbops = &s1d13521fb_ops;
        s1d13521_fb.node = -1;

        if (register_framebuffer(&s1d13521_fb) < 0)
        {
                printk(KERN_ERR "s1d13521fb_init(): Could not register frame buffer with kernel.\n");
                return -EINVAL;
        }

        printk(KERN_INFO "fb%d: %s frame buffer device\n", s1d13521_fb.node, s1d13521_fb.fix.id);

#ifdef CONFIG_FB_EPSON_PROC
        s1d13521proc_init();
#endif

	pFrameData=(PVOID) s1d13521fb_info.VirtualFramebufferAddr;
        return 0;
}
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// PRIVATE FUNCTION:
// Set registers to initial values
//----------------------------------------------------------------------------
void BusIssueInitRegisters(void)
{
        unsigned i, cmd,j,numparams;
        s1d13521_ioctl_cmd_params cmd_params;
//===========================================================================================
// Arron

#if 0
	writel(0x9AAAAAAA,S3C2410_GPCCON);
	int temp = readl(S3C2410_GPCCON);
	printk("[Arron] BusIssueInitRegisters() :: S3C2410_GPCCON = 0x%X\n",temp);
#endif
//-------------------------------------------------------------------------------------------

        S1D_INSTANTIATE_REGISTERS(static, InitCmdArray);

        i = 0;

        while (i < sizeof(InitCmdArray)/sizeof(InitCmdArray[0]))
        {
                cmd = InitCmdArray[i++];
                numparams = InitCmdArray[i++];

                for (j = 0; j < numparams; j++)
                        cmd_params.param[j] = InitCmdArray[i++];

                BusIssueCmd(cmd,&cmd_params,numparams);
        }
}

//----------------------------------------------------------------------------


void suspend_display(void) {

	if (suspended) return;
	suspended = 1;
	printk("-E");
	pvi_GoToSleep();

}


static void resume_display(void) {

	if (! suspended) return;
	suspended = 0;
	printk("+E");
	pvi_GoToNormal();

}

int s1d13521fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
        void __user *argp = (void __user *)arg;
        struct s1d13521_ioctl_hwc ioctl_hwc;
        s1d13521_ioctl_cmd_params cmd_params;
	int c, ret;
//				PTDisplayCommand pDispCommand;

//        dbg_info("%s(): cmd=%04Xh ^^^^^^^\n", __FUNCTION__,cmd);

        switch (cmd)
        {
          // zero parameter commands:

          case S1D13521_RUN_SYS:
          case S1D13521_STBY:
          case S1D13521_SLP:
          case S1D13521_INIT_SYS_RUN:
          case S1D13521_INIT_SYS_STBY:
          case S1D13521_RD_SFM:
          case S1D13521_END_SFM:
          case S1D13521_BST_END_SDR:
          case S1D13521_LD_IMG_END:
          case S1D13521_LD_IMG_WAIT:
          case S1D13521_LD_IMG_DSPEADR:
          case S1D13521_WAIT_DSPE_TRG:
          case S1D13521_WAIT_DSPE_FREND:
          case S1D13521_WAIT_DSPE_LUTFREE:
          case S1D13521_UPD_INIT:
          case S1D13521_UPD_GDRV_CLR:
            BusIssueCmd(cmd,&cmd_params,0);
            break;

          // one parameter commands
          case S1D13521_INIT_ROTMODE:
          case S1D13521_WR_SFM:
          case S1D13521_LD_IMG:
          case S1D13521_WAIT_DSPE_MLUTFREE:
          case S1D13521_UPD_FULL:
          case S1D13521_UPD_PART:
            if (copy_from_user(&cmd_params, argp, 1*sizeof(u16)))
                return -EFAULT;

            BusIssueCmd(cmd,&cmd_params,1);
            break;

          // two parameter commandss
          case S1D13521_WR_REG:
          case S1D13521_LD_IMG_SETADR:
          case S1D13521_RD_WFM_INFO:
          case S1D13521_UPD_SET_IMGADR:
            if (copy_from_user(&cmd_params, argp, 2*sizeof(u16)))
                return -EFAULT;

            if (cmd == S1D13521_WR_REG && cmd_params.param[0] == 0x154)
                BusIssueCmd(cmd,&cmd_params,1);
            else
                BusIssueCmd(cmd,&cmd_params,2);

            break;

          // three parameter commands
          case S1D13521_INIT_CMD_SET:
          case S1D13521_INIT_PLL_STANDBY:
            if (copy_from_user(&cmd_params, argp, 3*sizeof(u16)))
                return -EFAULT;

            BusIssueCmd(cmd,&cmd_params,3);
            break;

          // four parameter commands
          case S1D13521_INIT_SDRAM:
          case S1D13521_BST_RD_SDR:
          case S1D13521_BST_WR_SDR:
            if (copy_from_user(&cmd_params, argp, 4*sizeof(u16)))
                return -EFAULT;

            BusIssueCmd(cmd,&cmd_params,4);
            break;

          // five parameter commands
          case S1D13521_INIT_DSPE_CFG:
          case S1D13521_INIT_DSPE_TMG:
          case S1D13521_LD_IMG_AREA:
          case S1D13521_UPD_FULL_AREA:
          case S1D13521_UPD_PART_AREA:
            if (copy_from_user(&cmd_params, argp, 5*sizeof(u16)))
                return -EFAULT;

            BusIssueCmd(cmd,&cmd_params,5);
            break;

          case S1D13521_RD_REG:
            if (copy_from_user(&cmd_params, argp, 2*sizeof(u16)))
                return -EFAULT;
            cmd_params.param[1] = BusIssueReadReg(cmd_params.param[0]);
            return copy_to_user(argp, &cmd_params, 2*sizeof(u16)) ? -EFAULT : 0;

          case S1D13521_REGREAD:
            if (copy_from_user(&ioctl_hwc, argp, sizeof(ioctl_hwc)))
                return -EFAULT;

            ioctl_hwc.value = BusIssueReadReg(ioctl_hwc.addr);
            return copy_to_user(argp, &ioctl_hwc, sizeof(ioctl_hwc)) ? -EFAULT : 0;

          case S1D13521_REGWRITE:
            if (copy_from_user(&ioctl_hwc, argp, sizeof(ioctl_hwc)))
                return -EFAULT;

            BusIssueWriteReg(ioctl_hwc.addr, ioctl_hwc.value);
            return 0;

          case S1D13521_FLASH:
	    BusIssueFlashOperation((PS1D13532_FLASH_PACKAGE) argp);
            return 0;

          case S1D13521_RESET:
	    BusIssueReset();
            return 0;

#if 1
          case S1D13521_MEMBURSTWRITE:
            {
            u8 buffer[2048];
            unsigned user_buflen,copysize,copysize16;
            u16 *ptr16;
            u8* user_buffer;

            if (copy_from_user(&ioctl_hwc, argp, sizeof(ioctl_hwc)))
                return -EFAULT;

            // ioctl_hwc.value = number of bytes in the user buffer
            // ioctl_hwc.buffer = pointer to user buffer

            user_buflen = ioctl_hwc.value;
            user_buffer = (u8*) ioctl_hwc.buffer;

            while (user_buflen != 0)
            {
                copysize = user_buflen;

                if (user_buflen > sizeof(buffer))
                        copysize = sizeof(buffer);

                if (copy_from_user(buffer,user_buffer,copysize))
                        return -EFAULT;

                copysize16 = (copysize + 1)/2;
                ptr16 = (u16*) buffer;
                BusIssueWriteBuf(ptr16,copysize16);
                user_buflen -= copysize;
                user_buffer += copysize;
            }

            return 0;
            }

          case S1D13521_MEMBURSTREAD:
            {
            u8 buffer[2048];
            unsigned user_buflen,copysize,copysize16;
            u16 *ptr16;
            u8* user_buffer;

            if (copy_from_user(&ioctl_hwc, argp, sizeof(ioctl_hwc)))
                return -EFAULT;

            // ioctl_hwc.value = size in bytes of the user buffer, number of bytes to copy
            // ioctl_hwc.buffer = pointer to user buffer

            user_buflen = ioctl_hwc.value;
            user_buffer = (u8*) ioctl_hwc.buffer;

            while (user_buflen != 0)
            {
                copysize = user_buflen;

                if (user_buflen > sizeof(buffer))
                        copysize = sizeof(buffer);

                copysize16 = (copysize + 1)/2;
                ptr16 = (u16*) buffer;
                BusIssueReadBuf(ptr16,copysize16);

                if (copy_to_user(user_buffer,buffer,copysize))
                        return -EFAULT;

                user_buflen -= copysize;
                user_buffer += copysize;
            }

            return 0;
            }
#endif

	/* pvi ioctls */

	case CMD_SendCommand:
		resume_display();
		c = ((TDisplayCommand *)arg)->Command;
                if ((c >= 0xa0 && c <= 0xa5) ||
                    (c >= 0xb0 && c <= 0xb2) ||
                    (c == 0xaa) ||
                    (c == 0x21) ||
                    (c >= 0x10 && c <= 0x11) ||
                    (c >= 0xf0 && c <= 0xf3) ||
                    (c == 0xf5) ||
                    (c >= 0xf7 && c <= 0xfd) ||
                    (c >= 0xe0 && c <= 0xe2) ||
                    (c == 0xee) ||
                    capable(CAP_SYS_ADMIN)
                ) {
		    ret = pvi_SwitchCommand((PTDisplayCommand) arg);
                } else {
		    printk("cmd %i: EACCES\n", c);
                    ret = -EACCES;
                }
		return ret;
		break;

	case CMD_Dynamic:
		pvi_SwitchDynamic(arg);
		break;

	case CMD_WakeUp:
		resume_display();
		break;

	case CMD_Suspend:
		suspend_display();
		break;

	case CMD_Reset:
		BusIssueReset();
		pvi_Init();
		suspended = 0;
		break;

	default:
		printk("unknown command %i\n", cmd);
		break;
		return -EINVAL;

        }

        return 0;
}

//----------------------------------------------------------------------------
// Blank the display
//
// >>> Modify powerup and powerdown sequences as required.          <<<
// >>> This routine will need to be extensively modified to support <<<
// >>> any powerdown modes required.                                <<<
//
//----------------------------------------------------------------------------
int s1d13521fb_blank(int blank_mode, struct fb_info *info)
{

        // If nothing has changed, just return
        if (s1d13521fb_info.blank_mode == blank_mode)
        {
                return 0;
        }

        // Older versions of Linux Framebuffers used VESA modes, these are now remapped to FB_ modes
        switch (blank_mode)
        {
                // Disable display blanking and get out of powerdown mode.
                case FB_BLANK_UNBLANK:
                        // If the last mode was powerdown, then it is necessary to power up.
                        if (s1d13521fb_info.blank_mode == FB_BLANK_POWERDOWN)
                        {
                        }

                        break;

                // Panels don't use VSYNC or HSYNC, but the intent here is just to blank the display
                case FB_BLANK_NORMAL:
                case FB_BLANK_VSYNC_SUSPEND:
                case FB_BLANK_HSYNC_SUSPEND:
                        break;

                case FB_BLANK_POWERDOWN:
                        // When powering down, the controller needs to get into an idle state
                        // then the power save register bits 0/1 are the key values to play with
                        // if sleep mode is used, it disables the PLL and will require 10 msec to
                        // come back on.
                        // Also, when powering down, the linux refresh timer should be stopped and
                        // restarted when coming out of powerdown.

                        // If the last mode wasn't powerdown, then powerdown here.
                        if (s1d13521fb_info.blank_mode != FB_BLANK_POWERDOWN)
                        {
                        }

                        break;

                default:
                        return -EINVAL;         // Invalid argument
        }

        s1d13521fb_info.blank_mode = blank_mode;
        return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#if 0
struct fb_fillrect {
        __u32 dx;       /* screen-relative */
        __u32 dy;
        __u32 width;
        __u32 height;
        __u32 color;
        __u32 rop;
};
#endif

void s1d13521fb_fillrect(struct fb_info *p, const struct fb_fillrect *rect)
{
        cfb_fillrect(p,rect);
        gDisplayChange = 1;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#if 0
struct fb_copyarea {
        __u32 dx;
        __u32 dy;
        __u32 width;
        __u32 height;
        __u32 sx;
        __u32 sy;
};
#endif

void s1d13521fb_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
        cfb_copyarea(p,area);
        gDisplayChange = 1;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#if 0
struct fb_image {
        __u32 dx;               /* Where to place image */
        __u32 dy;
        __u32 width;            /* Size of image */
        __u32 height;
        __u32 fg_color;         /* Only used when a mono bitmap */
        __u32 bg_color;
        __u8  depth;            /* Depth of the image */
        const char *data;       /* Pointer to image data */
        struct fb_cmap cmap;    /* color map info */
#endif

void s1d13521fb_imageblit(struct fb_info *p, const struct fb_image *image)
{
        cfb_imageblit(p, image);
        gDisplayChange = 1;
}

//==============================================================================================
static struct platform_device *s1d13521fb_device;

static struct platform_driver s1d13521fb_driver = {
	.probe		= s1d13521fb_probe,
	.remove		= s1d13521fb_remove,
	.suspend	= s1d13521fb_suspend,
	.resume		= s1d13521fb_resume,
	.driver = {
		.name	= "s1d13521fb",
		.owner	= THIS_MODULE,
	},
};

int __init s1d13521fb_init(void)
{
	int ret = 0;

	printk("[Arron] s1d13521fb_init()\n");
//----------------------------------------------------------------------------------------------------------------

	ret = platform_driver_register(&s1d13521fb_driver);
	if (!ret) {
		//EPRINTF("__device alloc\n");
		printk("__device alloc\n");
		s1d13521fb_device = platform_device_alloc("s1d13521fb", 0);

		if (s1d13521fb_device){
		//	EPRINTF("__device add\n");
			printk("__device add\n");
			ret = platform_device_add(s1d13521fb_device);
		}
		else
			ret = -ENOMEM;
		if (ret) {
		//	EPRINTF("__why close it\n");
			printk("__why close it\n");
			platform_device_put(s1d13521fb_device);
			platform_driver_unregister(&s1d13521fb_driver);
		}
	}

//------------------------------------------------------------------------
	return ret;
}

void __exit s1d13521fb_exit(void)
{
	platform_device_unregister(s1d13521fb_device);
	platform_driver_unregister(&s1d13521fb_driver);
}
//---------------------------------------------------------------------------------------------------
module_init(s1d13521fb_init);
module_exit(s1d13521fb_exit);

MODULE_AUTHOR("Epson Research and Development");
MODULE_DESCRIPTION("framebuffer driver for Epson s1d13521 controller");
MODULE_LICENSE("GPL");

