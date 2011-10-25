//-----------------------------------------------------------------------------
//      
// linux/drivers/video/epson/s1d13521proc.c -- proc handling for frame buffer 
// driver for Epson S1D13521 LCD controller.
// 
// Copyright(c) Seiko Epson Corporation 2000-2008.
// All rights reserved.
//
// This file is subject to the terms and conditions of the GNU General Public
// License. See the file COPYING in the main directory of this archive for
// more details.
//
//---------------------------------------------------------------------------- 

#ifdef CONFIG_FB_EPSON_PROC

#include <linux/version.h>
#include <linux/proc_fs.h>
#include "s1d13521fb.h"
#include "access.h"

// Definitions for "type" in proc_read_fb() and proc_write_fb()
#define PROC_INFO       0
#define PROC_REG        1
#define PROC_FRAME      2
#define PROC_INIT       3

//-----------------------------------------------------------------------------
// Local Function Prototypes
//-----------------------------------------------------------------------------
static int  proc_read_fb(char *page, char **start, off_t off, int count, int *eof, void *data);
static int  proc_write_fb(struct file *file, const char *buffer, unsigned long count, void *data);
static long s1d13521proc_htol(const char *szAscii);
static int  s1d13521proc_iswhitechar(int c);

//-----------------------------------------------------------------------------
// Local Globals
//-----------------------------------------------------------------------------
static struct proc_dir_entry *s1d13521fb_dir;
static struct proc_dir_entry *info_file;
static struct proc_dir_entry *reg_file;
static struct proc_dir_entry *ft_file;
static struct proc_dir_entry *init_file;
static unsigned long ProcRegIndex = 0;
static unsigned long ProcRegVal = 0;

#ifdef CONFIG_FB_EPSON_GPIO_GUMSTIX
  extern void init_gpio(void);
#endif

#if (defined(CONFIG_FB_EPSON_GPIO_GUMSTIX) || defined(CONFIG_FB_EPSON_NTX_EBOOK_2440))
  extern int dump_gpio(char *buf);
#endif

//-----------------------------------------------------------------------------
// /proc setup
//-----------------------------------------------------------------------------
int s1d13521proc_init(void)
{
        // First setup a subdirectory for s1d13521fb
        s1d13521fb_dir = proc_mkdir("s1d13521fb", NULL);

        if (!s1d13521fb_dir)
                return -ENOMEM;

        s1d13521fb_dir->owner = THIS_MODULE;
        info_file = create_proc_read_entry("info", 0444, s1d13521fb_dir, proc_read_fb, (void*)PROC_INFO);

        if (info_file == NULL)
                return -ENOMEM;

        ft_file = create_proc_entry("ft", 0644, s1d13521fb_dir);

        if (ft_file == NULL)
                return -ENOMEM;

        ft_file->data = (void *)PROC_FRAME;
        ft_file->read_proc = proc_read_fb;
        ft_file->write_proc = proc_write_fb;
        ft_file->owner = THIS_MODULE;

        init_file = create_proc_entry("init", 0644, s1d13521fb_dir);

        if (init_file == NULL)
                return -ENOMEM;

        init_file->data = (void *)PROC_INIT;
        init_file->read_proc = proc_read_fb;
        init_file->write_proc = proc_write_fb;
        init_file->owner = THIS_MODULE;

        reg_file = create_proc_entry("regio", 0644, s1d13521fb_dir);

        if (reg_file == NULL)
                return -ENOMEM;

        reg_file->data = (void *)PROC_REG;
        reg_file->read_proc = proc_read_fb;
        reg_file->write_proc = proc_write_fb;
        reg_file->owner = THIS_MODULE;

        return 0;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void s1d13521proc_terminate(void)
{       
        remove_proc_entry("ft",s1d13521fb_dir);
        remove_proc_entry("regio",s1d13521fb_dir);
        remove_proc_entry("info",s1d13521fb_dir);
        //remove_proc_entry("s1d13521fb",&proc_root);
}

//-----------------------------------------------------------------------------
// /proc read function
//-----------------------------------------------------------------------------
static int proc_read_fb(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len;

        switch ((u32)data)
        {
          default:
                len = sprintf(page, "s1d13521fb driver\n");             
                break;

          case PROC_INFO:
                len = sprintf(page, "%s\n"
#ifdef CONFIG_FB_EPSON_VIRTUAL_FRAMEBUFFER
                        "Virtual Framebuffer Frequency: %dHz\n\n"
#endif  
                        "Syntax when writing to reg:  [index=hex] [val=hex]\n"
                        "To read a register, only set index=hex and then read from reg.\n"
                        "For example, to read register 0xAB:\n"
                        "   echo index=AB > /proc/s1d13521fb/regio\n"
                        "   cat /proc/s1d13521fb/regio\n\n",
                        s1d13521fb_version
#ifdef CONFIG_FB_EPSON_VIRTUAL_FRAMEBUFFER
                        ,CONFIG_FB_EPSON_VIRTUAL_FRAMEBUFFER_FREQ
#endif
                        );
                break;

          case PROC_REG:
                len = sprintf(page, "Register I/O: REG[%0lXh]=%0lXh\n\n",
                        ProcRegIndex, (unsigned long)BusIssueReadReg(ProcRegIndex));
                break;

          case PROC_FRAME:
                len = sprintf(page,"BusIssueDoRefreshDisplay(UPD_FULL,WF_MODE_GC)\n\n");

                BusIssueDoRefreshDisplay(UPD_FULL,WF_MODE_GC);
                break;

          case PROC_INIT:
                len = 0; // sprintf(page,"init the hw...\n\n");
//              BusIssueInitRegisters();
//              BusIssueInitDisplay();
                break;
        }

        return len;
}

//-----------------------------------------------------------------------------
// proc write function
//
// echo index=AB > /proc/s1d13521fb/regio
//-----------------------------------------------------------------------------
static int proc_write_fb(struct file *file, const char *buffer, unsigned long count, void *data)
{
        int GotRegVal;
        int len = count;
        char *buf = (char *)buffer;

        #define SKIP_OVER_WHITESPACE(str,count)                         \
        {                                                               \
                while ((count > 0) && !s1d13521proc_iswhitechar(*(str)))\
                {                                                       \
                        (str)++;                                        \
                        count--;                                        \
                }                                                       \
                                                                        \
                while ((count > 0) && s1d13521proc_iswhitechar(*(str))) \
                {                                                       \
                        (str)++;                                        \
                        count--;                                        \
                }                                                       \
        }

        switch ((int)data)
        {
          case PROC_FRAME:
                BusIssueDoRefreshDisplay(UPD_FULL,WF_MODE_GC);
                break;

          case PROC_REG:
                GotRegVal = FALSE;

                while (count > 0)
                {
                        if (!strncmp(buf,"index=",6))
                                ProcRegIndex = s1d13521proc_htol(buf+6);
                        else if (!strncmp(buf,"val=",4))
                        {
                                ProcRegVal = s1d13521proc_htol(buf+4);
                                GotRegVal = TRUE;
                        }
                        else
                        {
                                count = 0;
                                break;
                        }

                        SKIP_OVER_WHITESPACE(buf,count);
                }

                if (GotRegVal)
                        BusIssueWriteReg(ProcRegIndex,ProcRegVal);
                break;
        }

        return len;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int s1d13521proc_iswhitechar(int c)
{
        return ((c == ' ') || (c >= 0x09 && c <= 0x0d)); 
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int s1d13521proc_toupper(int c)
{
        if (c >= 'a' && c <= 'z')
                c += 'A'-'a';

        return c;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int s1d13521proc_isxdigit(int c)
{
        if ((c >= '0' && c <= '9') || 
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))
                return 1;

        return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static long s1d13521proc_htol(const char *szAscii)
{
        long lTmp;
        char ch;

        lTmp = 0;

        // skip whitespace
        while (s1d13521proc_iswhitechar(*szAscii))
                szAscii++;

        while (!s1d13521proc_iswhitechar((int) *szAscii) && ('\0' != *szAscii))
        {
                ch = (char)s1d13521proc_toupper(*szAscii);

                if (!s1d13521proc_isxdigit((int) ch))
                        return 0;

                if ((ch >= 'A') && (ch <= 'F'))
                        lTmp = lTmp * 16 + 10 + (ch - 'A');
                else
                        lTmp = lTmp * 16 + (ch - '0');

                szAscii++;
        }

        return lTmp;
}


#endif // CONFIG_FB_EPSON_PROC
