/*
 * tsec.c
 * Freescale Three Speed Ethernet Controller driver
 *
 * This software may be used and distributed according to the
 * terms of the GNU Public License, Version 2, incorporated
 * herein by reference.
 *
 * Copyright 2004 Freescale Semiconductor.
 * (C) Copyright 2003, Motorola, Inc.
 * author Andy Fleming
 *
 */

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>

#include <plat/regs-gpio.h>

#include "epson.h"
#include "access.h"
#include "Debug.h"

#define TRUE 1
#define FALSE 0

#define BS60_INIT_HSIZE         800
#define BS60_INIT_VSIZE         600
#define BS60_INIT_FSLEN         4
#define BS60_INIT_FBLEN         4
#define BS60_INIT_FELEN         10
#define BS60_INIT_LSLEN         10
#define BS60_INIT_LBLEN         4
/* #define BS60_INIT_LELEN         84
#define BS60_INIT_PIXCLKDIV     6 */

#define BS60_INIT_LELEN         0x30
#define BS60_INIT_PIXCLKDIV     7
#define BS60_INIT_SDRV_CFG      (100 | (1 << 8) | (1 << 9))
#define BS60_INIT_GDRV_CFG      0x2
#define BS60_INIT_LUTIDXFMT     (4 | (1 << 7))

#define HIGH_RESET_PIN  {gpdata = __raw_readl(S3C64XX_GPPDAT);gpdata =(gpdata & ~(1<<9));__raw_writel(gpdata|(0x1<<9),S3C64XX_GPPDAT);}
#define LOW_RESET_PIN   {gpdata = __raw_readl(S3C64XX_GPPDAT);gpdata =(gpdata & ~(1<<9));__raw_writel(gpdata,S3C64XX_GPPDAT);}

#define HIGH_HDC_PIN    {gpdata = __raw_readl(S3C64XX_GPODAT);gpdata =(gpdata & ~(1<<2));__raw_writel(gpdata|(0x1<<2),S3C64XX_GPODAT);}
#define LOW_HDC_PIN     {gpdata = __raw_readl(S3C64XX_GPODAT);gpdata =(gpdata & ~(1<<2));__raw_writel(gpdata,S3C64XX_GPODAT);}

volatile unsigned short __iomem* bsaddr;
unsigned long gpcon;
unsigned long gpdata;
unsigned long sromdata;

#define epd_read_data()	__raw_readw(bsaddr)
#define epd_write_data(v) { __raw_writew(v,bsaddr); udelay(10); }

static inline void epd_write_command(WORD cmd) {

	udelay(10);
	LOW_HDC_PIN;
	udelay(3);
	__raw_writew(cmd,bsaddr);
	udelay(3);
	HIGH_HDC_PIN;
	
}

static WORD epd_read_reg(WORD Index)
{
  WORD r;

  DDPRINTF("RD_REG(%x) ", (int)Index);
  epd_write_command(RD_REG);			// write out command
  epd_write_data(Index);				// write out register index
  r = epd_read_data();				// fetch data
  DDPRINTF("= %x\n", (int)r);
  return r;
}

static void epd_write_reg(WORD Index, WORD Value) {

  DDPRINTF("WR_REG(%x, %x)\n", (int)Index, (int)Value);
  epd_write_command(WR_REG);
  epd_write_data(Index);
  epd_write_data(Value);


}

#if 0

static BOOL epd_wait_for_hrdy(VOID)
{
  DWORD cnt;

  for(cnt=0x10000; cnt--;) {
    if((epd_read_reg(_EPSON_REG_STATUS) & 0x20) == 0) {
    	return 0;
    }
  }
  DDPRINTF("epd: host is busy\n");
  return -1;
}

#else

static BOOL epd_wait_for_hrdy(VOID)
{
  int i;

  for (i=0; i<100000; i++) {
        if (((__raw_readl(S3C64XX_GPPDAT))&(0x1<<2)) != 0) return 1;
  }
  DDPRINTF("epd: host is busy\n");
  return 0;

}

#endif

BOOL epd_package(WORD Command, PWORD pParam, DWORD len)
{
  int i;

  DDPRINTF("EPD_PACKAGE cmd=%x ", Command);
  for (i=0; i<len; i++) DDPRINTF("par%d=%x ", i+1, (unsigned) pParam[i]);

  epd_write_command(Command);		// write out
  if(pParam != NULL)
  	{
  	  for(; len-- != 0; pParam++)
  	    {
  	      epd_write_data(*pParam);		// write out
  	  	}
  	}
  DDPRINTF(" ok\n");
  return TRUE;
}

static void epd_hw_reset(void) {

  DDPRINTF("RESET_EPD\n");
  HIGH_RESET_PIN;
  mdelay(10);
  LOW_RESET_PIN;
  mdelay(30);
  HIGH_RESET_PIN;

}

int epd_init_registers(void) {

  s1d13521_ioctl_cmd_params CommandParam;
  int ret = -1;
  int iba;

  BusWaitForHRDY();
  DDPRINTF("Write 0xa\n");
  BusIssueWriteRegX(0xa, (BusIssueReadReg(0xa) | 0x1000));
  BusIssueWriteReg(0x12, 0x5949);
  BusIssueWriteReg(0x14, 0x0040);

  DDPRINTF("INIT_SYS_RUN\n");
  if(BusIssueCmd(INIT_SYS_RUN, &CommandParam, 0) == -1)
		goto _BusIssueResetFail;
  mdelay(2);

  CommandParam.param[0]=BS60_INIT_HSIZE;
  CommandParam.param[1]=BS60_INIT_VSIZE;
  CommandParam.param[2]=BS60_INIT_SDRV_CFG;
  CommandParam.param[3]=BS60_INIT_GDRV_CFG;
  CommandParam.param[4]=BS60_INIT_LUTIDXFMT;

  DDPRINTF("INIT_DSPE_CFG\n");
  if(BusIssueCmd(INIT_DSPE_CFG, &CommandParam, 5) == -1)
		goto _BusIssueResetFail;

  CommandParam.param[0]=BS60_INIT_FSLEN;
  CommandParam.param[1]=((BS60_INIT_FELEN<<8) | BS60_INIT_FBLEN);
  CommandParam.param[2]=BS60_INIT_LSLEN;
  CommandParam.param[3]=((BS60_INIT_LELEN<<8) | BS60_INIT_LBLEN);
  CommandParam.param[4]=BS60_INIT_PIXCLKDIV;
  DDPRINTF("INIT_DSPE_TMG\n");
  if(BusIssueCmd(INIT_DSPE_TMG, &CommandParam, 5) == -1)
		goto _BusIssueResetFail;

  iba = BS60_INIT_HSIZE * BS60_INIT_VSIZE * 2;
  BusIssueWriteReg(0x310, (iba & 0xFFFF));
  BusIssueWriteReg(0x312, ((iba >> 16) & 0xFFFF));

  //CommandParamiparam[0]=_WAVE_FORM_BASE;
  CommandParam.param[0]=0x0cd8;
  CommandParam.param[1]=0;
  DDPRINTF("RD_WFM_INFO\n");
  if(BusIssueCmd(RD_WFM_INFO, &CommandParam, 2) == -1)
		goto _BusIssueResetFail;


  DDPRINTF("UPD_GDRV_CLR\n");
  if(BusIssueCmd(UPD_GDRV_CLR, &CommandParam, 0) == -1)
		goto _BusIssueResetFail;

  DDPRINTF("INIT_ROTMODE\n");
  CommandParam.param[0]=(_INIT_ROTMODE << 8);
  if(BusIssueCmd(INIT_ROTMODE, &CommandParam, 2) == -1)
		goto _BusIssueResetFail;


		
//#ifdef __EPSON_THERMAL_WORKARROUND__ 			// adon, 2009-0409
//  BusIssueWriteReg(0x320, 0x0001);
//  BusIssueWriteReg(0x322, 0x001a);
//  #warning @@@@workarround for thermal reading
//#endif

//#if (defined(__8_INCH__) || defined(__9D7_INCH__))
//  BusIssueWriteReg(0x310, 0x0000);
//  BusIssueWriteReg(0x312, 0x0020);
//#endif

  ret=0;

_BusIssueResetFail:
	return ret;
}

int BusIssueInitHW(void) {

	DDPRINTF("INIT_HW\n");

        bsaddr = ioremap(0x10000000, 4);
	DDPRINTF("bsaddr=%x\n", (unsigned)bsaddr);

        // GPC7  EINK_3.3V_EN config output
        gpcon = __raw_readl(S3C64XX_GPCCON);
        gpcon = gpcon & ~(0xF << 28);
        __raw_writel(gpcon | (0x1 << 28), S3C64XX_GPCCON);

        //GPC7 EINK_3.3V_EN set low
        gpdata = __raw_readl(S3C64XX_GPCDAT);
        gpdata =(gpdata & ~(1<<7));
        __raw_writel(gpdata,S3C64XX_GPCDAT);

        //GPC6 EINK_1.8V_EN config output
        gpcon = __raw_readl(S3C64XX_GPCCON);
        gpcon = gpcon & ~(0xF << 24);
        __raw_writel(gpcon | (0x1 << 24), S3C64XX_GPCCON);

        //GPC6 EINK_1.8V_EN set low
        gpdata = __raw_readl(S3C64XX_GPCDAT);
        gpdata =(gpdata & ~(1<<6));
        __raw_writel(gpdata,S3C64XX_GPCDAT);

        //GPP2 OSC_EN config output
        gpcon = __raw_readl(S3C64XX_GPPCON);
        gpcon  = (gpcon & ~(3<<4));
         __raw_writel(gpcon|(0x1<<4), S3C64XX_GPPCON);
        //GPP2 OSC_EN set high
        gpdata = __raw_readl(S3C64XX_GPPDAT);
        gpdata =(gpdata & ~(1<<2));
        __raw_writel(gpdata|(0x1<<2),S3C64XX_GPPDAT);
        mdelay(10);

        //GPP9 HnRST config output
        gpcon = __raw_readl(S3C64XX_GPPCON);
        gpcon  = (gpcon & ~(3<<18));
        __raw_writel(gpcon|(0x1<<18), S3C64XX_GPPCON);

	epd_hw_reset();

	//setup HD/C signalconfig
        gpcon = __raw_readl(S3C64XX_GPOCON);
        gpcon =(gpcon & ~(3<<4));
        __raw_writel(gpcon|(0x1<<4),S3C64XX_GPOCON);

        gpdata = __raw_readl(S3C64XX_GPODAT);
        gpdata =(gpdata & ~(1<<2));
        __raw_writel(gpdata|(0x1<<2),S3C64XX_GPODAT);

        //GPO3 HIRQ config input
                gpcon = __raw_readl(S3C64XX_GPOCON);
                gpcon  = (gpcon & ~(3<<6));
                __raw_writel(gpcon, S3C64XX_GPOCON);

	//now HRDY  setup to input
        gpcon = __raw_readl(S3C64XX_GPOCON);
        gpcon  = (gpcon & ~(3<<8));
        __raw_writel(gpcon, S3C64XX_GPOCON);


        sromdata = __raw_readl(S3C64XX_VA_SROMC);
        sromdata &=~(0xF<<0);
        sromdata |= (1<<3) | (1<<2) | (1<<0);
        __raw_writel(sromdata, S3C64XX_VA_SROMC);

        //SROM_BC0 config
        //__raw_writel((0x1<<28)|(0xf<<24)|(0x1c<<16)|(0x1<<12)|(0x6<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);
	__raw_writel((0x0<<28)|(0x0<<24)|(0xA<<16)|(0x1<<12)|(0x0<<8)|(0x2<<4)|(0x0<<0), S3C64XX_VA_SROMC+4);

	epd_init_registers();


	

	return 0;

}


int BusIssueReset(void)
{

  epd_hw_reset();

  return epd_init_registers();

}

int BusWaitForHRDY() {

	return epd_wait_for_hrdy() ? 0 : -1;

}

int BusIssueCmd(unsigned ioctlcmd,s1d13521_ioctl_cmd_params *params,int numparams) {

	WORD param[16];
	int i;

	if (BusWaitForHRDY() != 0) return -1;
	for (i=0; i<numparams; i++) param[i] = params->param[i];
	return (epd_package(ioctlcmd, param, numparams) == TRUE) ? 0 : -1;

}

int BusIssueCmdX(unsigned ioctlcmd) {

	epd_write_command(ioctlcmd);
	return 0;

}

u16 BusIssueReadReg(u16 Index) {

	return epd_read_reg(Index);

}

int BusIssueWriteReg(u16 Index, u16 Value) {

	if (BusWaitForHRDY() != 0) return -1;
	epd_write_reg(Index, Value);
	return 0;

}

int BusIssueWriteRegX(u16 Index, u16 Value) {

	epd_write_reg(Index, Value);
	return 0;

}

int BusIssueWriteRegBuf(u16 Index, PUINT16 pData, UINT32 Length) {

	if (BusWaitForHRDY() != 0) return -1;
	epd_write_command(WR_REG);
	epd_write_data(Index);
	while (Length-- > 0) epd_write_data(*(pData++)); 
	return 0;

}

void BusIssueReadBuf(u16 *ptr16, unsigned copysize16) {

	while (copysize16-- > 0) *(ptr16++) = epd_read_data();

}

void BusIssueWriteBuf(u16 *ptr16, unsigned copysize16) {

	if (BusWaitForHRDY() != 0) return;
	while (copysize16-- > 0) epd_write_data(*(ptr16++));

}


