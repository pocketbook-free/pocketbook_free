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

#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

//#include "s1d13521fb.h"
#include <linux/timer.h>
//#include "s1d13521ioctl.h"

#include "s1d13521fb.h"
#include "access.h"
#include "ntx_s1d13521fb.h"
#include "Debug.h"

#define __EPSON_THERMAL_WORKARROUND__ 

extern FB_INFO_S1D13521 s1d13521fb_info;

extern EN_EPSON_ERROR_CODE WaitDPEIdle(VOID);

static ST_PVI_CONFIG	ConfigPVI={
	.CurRotateMode =(_INIT_ROTMODE << 8),
	.PastRotateMode=-1,
	.Deepth=2,
	.StartX=0,
	.StartY=0,
	.Width =S1D_DISPLAY_HEIGHT,
	.Height=S1D_DISPLAY_WIDTH,

	.Reg0x140=0,
	.Reg0x330=0,

	.ReverseGrade=0,
	.Partial=0
};

#define _PVI_IMAGE_SIZE		(S1D_DISPLAY_HEIGHT*S1D_DISPLAY_WIDTH/4)
static BYTE 	ImagePVI[S1D_DISPLAY_HEIGHT*S1D_DISPLAY_WIDTH];
static struct timer_list 	pvi_refersh_timer;

static int partial=0;

static WORD packed_bpp2_to_bbp3_dot3[] = { (0x0 << 1), (0x2 << 1), (0x5 << 1), (0x7 << 1) };
static WORD packed_bpp2_to_bbp3_dot2[] = { (0x0 << (1+4)), (0x2 << (1+4)), (0x5 << (1+4)), (0x7 << (1+4)) };
static WORD packed_bpp2_to_bbp3_dot1[] = { (0x0 << (1+4+4)), (0x2 << (1+4+4)), (0x5 << (1+4+4)), (0x7 << (1+4+4)) };
static WORD packed_bpp2_to_bbp3_dot0[] = { (0x0 << (1+4+4+4)), (0x2 << (1+4+4+4)), (0x5 << (1+4+4+4)), (0x7 << (1+4+4+4)) };
static WORD packed_bpp1_to_bbp3_dot3[] = { (0x0 << 1), (0x7 << 1) }; 
static WORD packed_bpp1_to_bbp3_dot2[] = { (0x0 << (1+4)), (0x7 << (1+4)) };
static WORD packed_bpp1_to_bbp3_dot1[] = { (0x0 << (1+4+4)), (0x7 << (1+4+4)) };
static WORD packed_bpp1_to_bbp3_dot0[] = { (0x0 << (1+4+4+4)), (0x7 << (1+4+4+4)) };

static BOOL BusReadTemperature(PWORD pTemp)
{
DWORD ii;

  BusIssueWriteReg(0x214, 1);
  for(ii=100000; ((BusIssueReadReg(0x212) & _BIT0) != 0) && (--ii != 0););
  if(pTemp != NULL)
  	*pTemp=(WORD) BusIssueReadReg(0x216);
  if((ii == 0) || ((BusIssueReadReg(0x212) & _BIT1) != 0)) 
  	{
			udelay(100);
			return FALSE;
  	}
  return TRUE;
}

static VOID BusUpdateThermal(VOID)			// adon, 2009-04-09
{
WORD temperature;
DWORD ii;

  for(ii=10; ii--;)
    if(BusReadTemperature(&temperature) != FALSE)
    	{
  			BusIssueWriteReg(0x322, temperature);
  			break;
    	}
}


static int perform_update(int WaveForm) {

  s1d13521_ioctl_cmd_params CommandParam;
  int ret;

  CommandParam.param[0] = (WaveForm << 8);

  if(ConfigPVI.Partial == 0)
  	{
	  ret=BusIssueCmd(UPD_FULL, &CommandParam, 1);
  	}
  else
  	{
  	  CommandParam.param[1]=ConfigPVI.StartX;
  	  CommandParam.param[2]=ConfigPVI.StartY;
  	  CommandParam.param[3]=ConfigPVI.Width;
  	  CommandParam.param[4]=ConfigPVI.Height;
   	  ret=BusIssueCmd(UPD_PART_AREA, &CommandParam, 5);
  	}

  return ret;

}

static int lastPartial=0, lastStartX, lastStartY, lastWidth, lastHeight;
static int isdynamic=0;

static EN_EPSON_ERROR_CODE PutImage_PVI2EPSON(PBYTE pData, UINT16 WaveForm)
{
int length, cc;
s1d13521_ioctl_cmd_params CommandParam;
PBYTE   pSource;
PUINT16 pDest;
BYTE data;
EN_EPSON_ERROR_CODE ret;

  if(BusWaitForHRDY() != _EPSON_ERROR_SUCCESS)
  	return _EPSON_ERROR_NOT_READY;

  ConfigPVI.Reg0x330=BusIssueReadReg(0x330);
  ret=BusIssueWriteReg(0x330, 0x84);
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}
  if(ConfigPVI.PastRotateMode != ConfigPVI.CurRotateMode)
  	{
	  	CommandParam.param[0]=ConfigPVI.CurRotateMode;
   	  ret=BusIssueCmd(INIT_ROTMODE,  &CommandParam, 1);
	  	ConfigPVI.PastRotateMode=ConfigPVI.CurRotateMode;
  	}
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}
  CommandParam.param[0] = (0x2 << 4);
  if(ConfigPVI.Partial == 0)
  	{
#if (defined(__5_INCH__) || defined(__6_INCH__))
   	  ret=BusIssueCmd(LD_IMG, &CommandParam, 1);
      length=((S1D_DISPLAY_WIDTH*S1D_DISPLAY_HEIGHT) >> 2); /* !!!!! */
#elif defined(__9D7_INCH__)
  	  CommandParam.param[1]=(1200-800)/2;
  	  CommandParam.param[2]=(826-600)/2;
  	  CommandParam.param[3]=800;
  	  CommandParam.param[4]=600;
   	  ret=BusIssueCmd(LD_IMG_AREA, &CommandParam, 5);
      length=(800*600) >> 2;
#else
			#error "unknow panel"
#endif
  	}
  else
  	{
  	  CommandParam.param[1]=ConfigPVI.StartX;
  	  CommandParam.param[2]=ConfigPVI.StartY;
  	  CommandParam.param[3]=ConfigPVI.Width;
  	  CommandParam.param[4]=ConfigPVI.Height;
   	  ret=BusIssueCmd(LD_IMG_AREA, &CommandParam, 5);
      length=(ConfigPVI.Width*ConfigPVI.Height) >> 2; /* !!!!! */
  	}
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}
  pSource=(PBYTE) pData;
  pDest=(PUINT16) s1d13521fb_info.VirtualFramebufferAddr;
  if(ConfigPVI.Deepth == 2) {
    for(cc=length; cc--; pDest++, pSource++)
      {
        data=*pSource ^ ConfigPVI.ReverseGrade;
  	    *pDest=(packed_bpp2_to_bbp3_dot0[(data >> 0) & 3]
  			  | packed_bpp2_to_bbp3_dot1[(data >> 2) & 3]
  			  | packed_bpp2_to_bbp3_dot2[(data >> 4) & 3]
  			  | packed_bpp2_to_bbp3_dot3[(data >> 6) & 3]);
      }
  } else if(ConfigPVI.Deepth == 4) {
    for(cc=length; cc--; pDest++, pSource+=2)
      {
	data=*pSource ^ ConfigPVI.ReverseGrade;
	*pDest = ((data >> 4) & 0xf) | ((data << 4) & 0xf0);
	data=*(pSource+1) ^ ConfigPVI.ReverseGrade;
	*pDest |= ((data << 4) & 0xf00) | ((data << 12) & 0xf000);
      }
  } else {
    for(cc=length >> 1; cc--; pDest++, pSource++)
      {
        data=*pSource ^ ConfigPVI.ReverseGrade;
  	    *pDest=(packed_bpp1_to_bbp3_dot0[(data >> 0) & 1]
  			  | packed_bpp1_to_bbp3_dot1[(data >> 1) & 1]
  			  | packed_bpp1_to_bbp3_dot2[(data >> 2) & 1]
  			  | packed_bpp1_to_bbp3_dot3[(data >> 3) & 1]);
  		pDest++;
  	    *pDest=(packed_bpp1_to_bbp3_dot0[(data >> 4) & 1]
  			  | packed_bpp1_to_bbp3_dot1[(data >> 5) & 1]
  			  | packed_bpp1_to_bbp3_dot2[(data >> 6) & 1]
  			  | packed_bpp1_to_bbp3_dot3[(data >> 7) & 1]);
      }
  }

  ret=BusIssueWriteRegBuf(0x154, (PUINT16) s1d13521fb_info.VirtualFramebufferAddr, length);
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}

  ret=BusIssueCmd(LD_IMG_END, &CommandParam, 0);
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}

  //ret=WaitDPEIdle();
  //if(ret != _EPSON_ERROR_SUCCESS)
  //	{
  //	  goto _PutImage_PVI2EPSON_Fail;
  //	}

  BusIssueCmd(WAIT_DSPE_LUTFREE,&CommandParam,0);
  if (ConfigPVI.Partial == 0 || lastPartial == 0) {
	// last or current update is full - waiting for update
	BusIssueCmd(WAIT_DSPE_FREND,&CommandParam,0);
  } else {
	// last and current update are partial - check for overlap
	if (
	      (
		(lastStartX <= ConfigPVI.StartX && lastStartX+lastWidth > ConfigPVI.StartX) ||
		(ConfigPVI.StartX <= lastStartX && ConfigPVI.StartX+ConfigPVI.Width > lastStartX)
	      )
		&&
	      (
		(lastStartY <= ConfigPVI.StartY && lastStartY+lastHeight > ConfigPVI.StartY) ||
		(ConfigPVI.StartY <= lastStartY && ConfigPVI.StartY+ConfigPVI.Height > lastStartY)
	      )
	)
	{
		if (! isdynamic) BusIssueCmd(WAIT_DSPE_FREND,&CommandParam,0);
	}
  }

  isdynamic = 0;

  ret = perform_update(WaveForm);
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}

  BusIssueCmd(WAIT_DSPE_TRG,&CommandParam,0);
  //if (ConfigPVI.Partial == 0) BusIssueCmd(WAIT_DSPE_FREND,&CommandParam,0);

  BusUpdateThermal();			// adon, 2009-04-09
  ret=BusIssueWriteReg(0x330, ConfigPVI.Reg0x330);
  if(ret != _EPSON_ERROR_SUCCESS)
  	{
  	  goto _PutImage_PVI2EPSON_Fail;
  	}
_PutImage_PVI2EPSON_Fail:

  lastPartial = ConfigPVI.Partial;
  lastStartX = ConfigPVI.StartX;
  lastStartY = ConfigPVI.StartY;
  lastWidth = ConfigPVI.Width;
  lastHeight = ConfigPVI.Height;

  return ret;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_ioctl_NewImage(PTDisplayCommand puDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  memcpy(ImagePVI, puDisplayCommand->Data, (S1D_DISPLAY_HEIGHT*S1D_DISPLAY_WIDTH)/(8/ConfigPVI.Deepth));
  ConfigPVI.Partial = 0;
  return 0;
}

int pvi_ioctl_StopNewImage(PTDisplayCommand puDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  return 0;
}

int pvi_ioctl_DisplayImage(PTDisplayCommand puDisplayCommand)
{
//	printk("pvi_ioctl_DisplayImage()\n");
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;

  PutImage_PVI2EPSON(ImagePVI, WF_MODE_GC);
//  PutImage_PVI2EPSON(ImagePVI, FALSE, WF_MODE_GU);
  return 0;
}

int pvi_ioctl_PartialImage(PTDisplayCommand puDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  ConfigPVI.StartX=(puDisplayCommand->Data  [0] << 8)+puDisplayCommand->Data[1];
  ConfigPVI.StartY=(puDisplayCommand->Data  [2] << 8)+puDisplayCommand->Data[3];
  ConfigPVI.Width =((puDisplayCommand->Data [4] << 8)+puDisplayCommand->Data[5])+1-ConfigPVI.StartX;
  ConfigPVI.Height=((puDisplayCommand->Data [6] << 8)+puDisplayCommand->Data[7])+1-ConfigPVI.StartY;
  /////////////////////////////////////////////////////////
  // align pexil to modula 4
  memcpy(ImagePVI, &puDisplayCommand->Data[8], (ConfigPVI.Height*((ConfigPVI.Width+3) & ~3))/(8/ConfigPVI.Deepth));
  ConfigPVI.Partial = 1;
  return 0;
}

int pvi_ioctl_DisplayPartial(PTDisplayCommand puDisplayCommand)
{
//	printk("pvi_ioctl_DisplayPartial()\n");
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
#if 0
	BusIssueWriteReg(0x360, 0x1001);
  BusIssueWriteReg(0x364, 0x1001);
  PutImage_PVI2EPSON(ImagePVI, TRUE, WF_MODE_MU);
#else
  PutImage_PVI2EPSON(ImagePVI, WF_MODE_MU);
#endif
  return 0;
}

int pvi_ioctl_DisplayPartialGU(PTDisplayCommand puDisplayCommand)
{
//	printk("pvi_ioctl_DisplayPartial()\n");
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
	BusIssueWriteReg(0x360, 0x1001);
  BusIssueWriteReg(0x364, 0x1001);
  PutImage_PVI2EPSON(ImagePVI, WF_MODE_GU);
  return 0;
}

int pvi_ioctl_EraseDisplay(PTDisplayCommand pDisplayCommand)
{
BYTE Color=0xff;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  switch(pDisplayCommand->Data[0]) {
  	case 0:
  		Color=0x00;
  		break;
  	case 1:
  		Color=0xff;
  		break;
  	case 2:
  		Color=0x55;
  		break;
  	case 3:
  		Color=0xaa;
  		break;
  };
  memset(ImagePVI, Color, (S1D_DISPLAY_HEIGHT*S1D_DISPLAY_WIDTH)/(8/ConfigPVI.Deepth));
  ConfigPVI.Partial = 0;
  PutImage_PVI2EPSON(ImagePVI, WF_MODE_GC);
  return 0;
}

////////////////////////////////////////////////////////
//
// i do not know how to support
//
int pvi_ioctl_RestoreImage(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_ioctl_SetDepth(PTDisplayCommand puDisplayCommand)
{
int RetCode=0;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  ConfigPVI.Deepth=puDisplayCommand->Data[0];
  if (ConfigPVI.Deepth == 0) ConfigPVI.Deepth = 1;
  return RetCode;
}

int pvi_ioctl_Rotate(PTDisplayCommand puDisplayCommand)
{
//  if(ConfigPVI.fNormalMode == FALSE)
//  	return -EFAULT;


 switch(puDisplayCommand->Data[0]) {
  	case 1:
  		ConfigPVI.CurRotateMode=3<<8;
  		break;
  	case 2:
  		ConfigPVI.CurRotateMode=2<<8;
  		break;
  	case 3:
  		ConfigPVI.CurRotateMode=1<<8;
  		break;
  	case 0:
  		ConfigPVI.CurRotateMode=0<<8;
  		break;
  };
  return 0;
}

int pvi_ioctl_Positive(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  ConfigPVI.ReverseGrade=0x0000;
  return 0;
}

int pvi_ioctl_Negative(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  ConfigPVI.ReverseGrade=0xffff;
  return 0;
}

int pvi_ioctl_Init(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
//  BusIssueInitDisplay();
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
static UINT16 SPI_RegCTRLData;

static VOID SPI_FlashSwitchToHost(VOID)
{
  BusIssueWriteReg(_EPSON_REG_SPI_CS_CONTROL, _EPSON_REG_SPI_CS_CONTROL_ENABLE);
  SPI_RegCTRLData=BusIssueReadReg(_EPSON_REG_SPI_CONTROL);
  BusIssueWriteReg(_EPSON_REG_SPI_CONTROL, (SPI_RegCTRLData & ~_BIT7));
  BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
}

static VOID SPI_FlashSwitchToDPE(VOID)
{
  BusIssueWriteReg(_EPSON_REG_SPI_CONTROL, SPI_RegCTRLData);
  BusIssueWriteReg(_EPSON_REG_SPI_CS_CONTROL, 0);
}

static BOOL SPI_FlashWaitReady(VOID)
{
UINT32 loop=1000;

  for(; loop--;)
    if((BusIssueReadReg(_EPSON_REG_SPI_STATUS) & _EPSON_REG_SPI_STATUS_BUSY) == 0)
      return TRUE;
  return FALSE;
}

static BOOL SPI_FlashWriteData(BYTE Data)
{
  BusIssueWriteReg(_EPSON_REG_SPI_WRITE_DATA, (Data | _EPSON_REG_SPI_WRITE_DATA_ENABLE));
  return SPI_FlashWaitReady();
}

static BOOL SPI_FlashWriteDummy(VOID)
{
  BusIssueWriteReg(_EPSON_REG_SPI_WRITE_DATA, 0x00);
  return SPI_FlashWaitReady();
}

static BOOL spi_FlashEnableWrite(VOID)
{
BOOL ret;

  SPI_FlashSwitchToHost();
  ret=SPI_FlashWriteData(0x06);
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL spi_FlashDisableWrite(VOID)
{
BOOL ret;

  SPI_FlashSwitchToHost();
  ret=SPI_FlashWriteData(0x04);
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL spi_FlashUnProtect(UINT32 Address)
{
BOOL ret=FALSE;

  spi_FlashEnableWrite();
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x39) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) (Address >> 16)) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) (Address >> 8)) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) Address) == FALSE)
  	goto _spi_FlashUnProtectFail;
  ret=TRUE;
_spi_FlashUnProtectFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL spi_FlashProtect(UINT32 Address)
{
BOOL ret=FALSE;

  spi_FlashEnableWrite();
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x36) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) (Address >> 16)) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) (Address >> 8)) == FALSE)
  	goto _spi_FlashUnProtectFail;
  if(SPI_FlashWriteData((BYTE) Address) == FALSE)
  	goto _spi_FlashUnProtectFail;
  ret=TRUE;
_spi_FlashUnProtectFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

static BYTE spi_GetStatus(VOID)
{
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x05) == FALSE)
  	goto  _spi_GetStatusFial;
  SPI_FlashWriteDummy();
_spi_GetStatusFial:
  SPI_FlashSwitchToDPE();
  return (BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
}

static BOOL spi_FlashEraseChip(VOID)
{
  BusIssueWriteReg(_EPSON_REG_SPI_CS_CONTROL, _EPSON_REG_SPI_CS_CONTROL_ENABLE);
  BusIssueWriteReg(_EPSON_REG_SPI_WRITE_DATA, (0x02 | _EPSON_REG_SPI_WRITE_DATA_ENABLE));
  if(SPI_FlashWaitReady() == FALSE)
  	return FALSE;
  BusIssueWriteReg(_EPSON_REG_SPI_CS_CONTROL, 0);
  return FALSE;
}

static BOOL spi_FlashEraseSector(PTDisplayCommand pDisplayCommand)
{
UINT32 Address;

  Address=((pDisplayCommand->Data[0] << 16) |
           (pDisplayCommand->Data[1] << 8)  |
            pDisplayCommand->Data[2]);

  spi_FlashEnableWrite();
  spi_FlashUnProtect(0x00000);
  spi_FlashUnProtect(0x10000);
  spi_FlashUnProtect(0x20000);
  spi_FlashUnProtect(0x30000);

  spi_FlashEnableWrite();
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0xd8) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[0] & 3) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(0) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(0) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteDummy() == FALSE)
  	goto  _spi_FlashWriteByteFail;
  SPI_FlashSwitchToDPE();

  spi_FlashProtect(0x00000);
  spi_FlashProtect(0x10000);
  spi_FlashProtect(0x20000);
  spi_FlashProtect(0x30000);
  spi_FlashDisableWrite();

_spi_FlashWriteByteFail:
  SPI_FlashSwitchToDPE();
  return TRUE;
}

static BOOL spi_FlashWriteByte(PTDisplayCommand pDisplayCommand)
{
UINT32 Address;
BYTE status;

  Address=((pDisplayCommand->Data[0] << 16) |
           (pDisplayCommand->Data[1] << 8)  |
            pDisplayCommand->Data[2]);

  spi_FlashEnableWrite();
  spi_FlashUnProtect(0x00000);
  spi_FlashUnProtect(0x10000);
  spi_FlashUnProtect(0x20000);
  spi_FlashUnProtect(0x30000);

  spi_FlashEnableWrite();
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x02) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[0]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[1]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[2]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[3]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  SPI_FlashSwitchToDPE();


  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x03) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[0]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[1]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[2]) == FALSE)
  	goto  _spi_FlashWriteByteFail;
  if(SPI_FlashWriteDummy() == FALSE)
  	goto  _spi_FlashWriteByteFail;
  SPI_FlashSwitchToDPE();
  status=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);

  spi_FlashProtect(0x00000);
  spi_FlashProtect(0x10000);
  spi_FlashProtect(0x20000);
  spi_FlashProtect(0x30000);
  spi_FlashDisableWrite();

  SPI_FlashSwitchToDPE();
  return TRUE;

_spi_FlashWriteByteFail:
  SPI_FlashSwitchToDPE();
  return FALSE;
}

int pvi_ioctl_WriteToFlash(PTDisplayCommand pDisplayCommand)
{
  int i;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  switch(ConfigPVI.StepSPI) {
  	case _STEP_NOR_BYPASS:
  	DDPRINTF("_STEP_NOR_BYPASS ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x0a) && (pDisplayCommand->Data[2] == 0xaa) && (pDisplayCommand->Data[3] == 0xaa))
  	  	ConfigPVI.StepSPI=_STEP_NOR_WRITE_AA_INTO_555;
  	  break;

  	case _STEP_NOR_WRITE_AA_INTO_555:
  	DDPRINTF("_STEP_NOR_WRITE_AA_INTO_555 ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x05) && (pDisplayCommand->Data[2] == 0x55) && (pDisplayCommand->Data[3] == 0x55))
  	  	ConfigPVI.StepSPI=_STEP_NOR_WRITE_55_INTO_2AA;
  	  else
  	  	ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_WRITE_55_INTO_2AA:
  	DDPRINTF("_STEP_NOR_WRITE_55_INTO_2AA ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x0a) && (pDisplayCommand->Data[2] == 0xaa))
	  	{
	  	  if(pDisplayCommand->Data[3] == 0x90)
  	  		ConfigPVI.StepSPI=_STEP_NOR_CMD_STATUS;
	  	  else if(pDisplayCommand->Data[3] == 0xa0)
  	  		ConfigPVI.StepSPI=_STEP_NOR_CMD_PROGRAM;
	  	  else if(pDisplayCommand->Data[3] == 0x80)
  	  		ConfigPVI.StepSPI=_STEP_NOR_CMD_ERASE;
	  	  else
  	  		ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
	  	}
  	  else
  	  	ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_CMD_PROGRAM:
  	DDPRINTF("_STEP_NOR_CMD_PROGRAM ");
	  spi_FlashWriteByte(pDisplayCommand);
  	  ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_CMD_ERASE:
  	DDPRINTF("_STEP_NOR_CMD_ERASE ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x0a) && (pDisplayCommand->Data[2] == 0xaa) && (pDisplayCommand->Data[3] == 0xaa))
  	  	ConfigPVI.StepSPI=_STEP_NOR_CMD_ERASE_WRITE_AA_INTO_555;
  	  else
  	  	ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_CMD_ERASE_WRITE_AA_INTO_555:
  	DDPRINTF("_STEP_NOR_CMD_ERASE_WRITE_AA_INTO_555 ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x05) && (pDisplayCommand->Data[2] == 0x55) && (pDisplayCommand->Data[3] == 0x55))
  	  	ConfigPVI.StepSPI=_STEP_NOR_CMD_ERASE_WRITE_55_INTO_2AA;
  	  else
  	  	ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_CMD_ERASE_WRITE_55_INTO_2AA:
  	DDPRINTF("_STEP_NOR_CMD_ERASE_WRITE_55_INTO_2AA ");
	  if((pDisplayCommand->Data[0] == 0x00) && (pDisplayCommand->Data[1] == 0x0a) && (pDisplayCommand->Data[2] == 0xaa) && (pDisplayCommand->Data[3] == 0x10))
  	  	spi_FlashEraseChip();
	  else if(pDisplayCommand->Data[3] == 0x30)
  	  	spi_FlashEraseSector(pDisplayCommand);
  	  ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

  	case _STEP_NOR_CMD_STATUS:
  	DDPRINTF("_STEP_NOR_CMD_STATUS ");
	  if(pDisplayCommand->Data[3] == 0xf0)
  	  	ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  	  break;

	default:
  	DDPRINTF("_STEP_UNDEFINE ");
	  ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  };
  return 0;
}

int pvi_ioctl_ReadFromFlash(PTDisplayCommand pDisplayCommand)
{
int RetCode=-EFAULT;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  if(ConfigPVI.StepSPI == _STEP_NOR_CMD_STATUS)
  	{
  	  RetCode=0;
  	  if((pDisplayCommand->Data[2] & 3) == 0)
  		pDisplayCommand->Data[0]=0x3e;
  	  else if((pDisplayCommand->Data[2] & 3) == 1)
  		pDisplayCommand->Data[0]=0xc2;
  	  else
  		pDisplayCommand->Data[0]=0;
  	  return RetCode;
  	}
  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x03) == FALSE)
  	goto  _pvi_ioctl_ReadFromFlashFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[0]) == FALSE)
  	goto  _pvi_ioctl_ReadFromFlashFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[1]) == FALSE)
  	goto  _pvi_ioctl_ReadFromFlashFail;
  if(SPI_FlashWriteData(pDisplayCommand->Data[2]) == FALSE)
  	goto  _pvi_ioctl_ReadFromFlashFail;
  if(SPI_FlashWriteDummy() == FALSE)
  	goto  _pvi_ioctl_ReadFromFlashFail;
  pDisplayCommand->Data[0]=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
  RetCode=0;
_pvi_ioctl_ReadFromFlashFail:
  SPI_FlashSwitchToDPE();
  return RetCode;
}

int pvi_ioctl_ForcedRefresh(PTDisplayCommand pDisplayCommand)
{
#ifdef __TURN_ON_FORCE_DISPLAY__
s1d13521_ioctl_cmd_params CommandParam;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  CommandParam.param[0]=((WF_MODE_GC << 8) | 0x4000);
  BusIssueCmd(UPD_FULL, &CommandParam, 1);
#endif
  return 0;
}

static void pvi_DoTimerForcedRefresh(ULONG dummy)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return;
  pvi_ioctl_ForcedRefresh(NULL);
  pvi_refersh_timer.expires = jiffies+HZ*ConfigPVI.AutoRefreshTimer;
  add_timer(&pvi_refersh_timer);
}

int pvi_ioctl_AutoRefreshOn(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  if((ConfigPVI.AutoRefreshTimer != 0) && (ConfigPVI.fAutoRefreshMode == FALSE))
  	{
      pvi_refersh_timer.expires = jiffies+HZ*ConfigPVI.AutoRefreshTimer;
      add_timer(&pvi_refersh_timer);
      ConfigPVI.fAutoRefreshMode=TRUE;
  	}
  return 0;
}

int pvi_ioctl_AutoRefreshOff(PTDisplayCommand pDisplayCommand)
{
  del_timer(&pvi_refersh_timer);
  ConfigPVI.fAutoRefreshMode=FALSE;
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// set_refresh_timer
//
int pvi_ioctl_SetRefresh(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  ConfigPVI.AutoRefreshTimer=pDisplayCommand->Data[0];
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// get_refresh_timer
//
int pvi_ioctl_GetRefresh(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  pDisplayCommand->Data[0]=ConfigPVI.AutoRefreshTimer;
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/////////////////////////////////////////////////////////////////////////
//
// i do not know what is the value, give me the correct value
//
int pvi_ioctl_ControllerVersion(PTDisplayCommand puDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  puDisplayCommand->Data[0]=0x06;
  return 0;
}

/////////////////////////////////////////////////////////////////////////
//
// i do not know what is the value, give me the correct value
//
int pvi_ioctl_SoftwareVersion(PTDisplayCommand puDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  puDisplayCommand->Data[0]=0x06;
  return 0;
}

int pvi_ioctl_DisplaySize(PTDisplayCommand puDisplayCommand)
{
  puDisplayCommand->Data[0]=0x22;
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_ioctl_GetStatus(PTDisplayCommand puDisplayCommand)
{
  puDisplayCommand->Data[0]=0x06;
  if(ConfigPVI.fNormalMode == FALSE)
    puDisplayCommand->Data[0]|=_BIT0;
  if(ConfigPVI.Deepth != 0)
    puDisplayCommand->Data[0]|=_BIT4;
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_ioctl_Temperature(PTDisplayCommand pDisplayCommand)
{
UINT32 loop=100;

  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  BusIssueWriteReg(_EPSON_REG_THERMAL_SENSOR_TRIGGER, _EPSON_REG_THERMAL_SENSOR_TRIGGER_READ);
  for(; loop--;)
    if((BusIssueReadReg(_EPSON_REG_THERMAL_SENSOR_STATUS) & _EPSON_REG_THERMAL_SENSOR_STATUS_BUSY) == 0)
      break;;
  if(loop == 0)
  	return -EFAULT;
  pDisplayCommand->Data[0]=BusIssueReadReg(_EPSON_REG_THERMAL_SENSOR_DATA);
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/////////////////////////////////////////////////////////////////////////
//
// i do not know what is the value
//
int pvi_ioctl_WriteRegister(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  return 0;
}

/////////////////////////////////////////////////////////////////////////
//
// i do not know what is the value
//
int pvi_ioctl_ReadRegister(PTDisplayCommand pDisplayCommand)
{
  if(ConfigPVI.fNormalMode == FALSE)
  	return -EFAULT;
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_ioctl_Reset(PTDisplayCommand puDisplayCommand)
{
  ConfigPVI.CurRotateMode=(_INIT_ROTMODE << 8);
  ConfigPVI.PastRotateMode=-1;
  ConfigPVI.ReverseGrade=0;
  ConfigPVI.Deepth=2;
  ConfigPVI.fNormalMode=FALSE;
  ConfigPVI.AutoRefreshTimer=100;
  ConfigPVI.StepSPI=_STEP_NOR_BYPASS;
  if(ConfigPVI.fAutoRefreshMode != FALSE)
    del_timer(&pvi_refersh_timer);
//  printk("%s] 0x0a=0x%04x\n", __FUNCTION__, BusIssueReadReg(0x0a));
  return 0;
}

int pvi_ioctl_GoToNormal(PTDisplayCommand pDisplayCommand)
{
	BusIssueWriteRegX(0x0a, BusIssueReadReg(0x0a) | 0x1000);
//	printk("pvi_ioctl_GoToNormal\n");
	BusIssueWriteReg(0x16, 0);
//	printk("pvi_ioctl_GoToNormal\n");
  pvi_ioctl_AutoRefreshOn(NULL);
  ConfigPVI.fNormalMode=TRUE;
  BusIssueCmd(RUN_SYS, NULL, 0);
  return 0;
}
//=====================================================================
// Arron
int pvi_GoToNormal(void)
{

/*

unsigned int ii;

	for(ii=100; ii--;)
	  if(BusWaitForHRDY() == _EPSON_ERROR_SUCCESS)
	  	break;

	BusIssueWriteRegX(0x0a, 0x1000);
//	printk("BusIssueWriteRegX 0x00a=0x1503\n");

	for(ii=100; ii--;)
	  if(BusWaitForHRDY() == _EPSON_ERROR_SUCCESS)
	  	break;
  BusIssueCmdX(RUN_SYS);

	for(ii=10; ii--;)
	  if(BusWaitForHRDY() == _EPSON_ERROR_SUCCESS)
	  	break;
  pvi_ioctl_AutoRefreshOn(NULL);
  ConfigPVI.fNormalMode=TRUE;
*/
  return 0;
}
EXPORT_SYMBOL(pvi_GoToNormal);


int pvi_GoToSleep(void)
{
/*
  ConfigPVI.fNormalMode=FALSE;
  if(ConfigPVI.fAutoRefreshMode != FALSE)
    del_timer(&pvi_refersh_timer);
  BusIssueCmd(SLP, NULL, 0);
*/
  return 0;
}

EXPORT_SYMBOL(pvi_GoToSleep);
//---------------------------------------------------------------------

int pvi_ioctl_GoToSleep(PTDisplayCommand pDisplayCommand)
{
  ConfigPVI.fNormalMode=FALSE;
  if(ConfigPVI.fAutoRefreshMode != FALSE)
    del_timer(&pvi_refersh_timer);
  BusIssueCmd(SLP, NULL, 0);
  return 0;
}

int pvi_ioctl_GoToStandBy(PTDisplayCommand pDisplayCommand)
{
  ConfigPVI.fNormalMode=FALSE;
  BusIssueCmd(STBY, NULL, 0);
  return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_Init(VOID)
{
  ConfigPVI.fAutoRefreshMode=FALSE;
  init_timer(&pvi_refersh_timer);
  pvi_refersh_timer.function=pvi_DoTimerForcedRefresh;
  pvi_ioctl_Reset(NULL);
  ConfigPVI.fNormalMode=TRUE;
  return 0;
}

void pvi_Deinit(VOID)
{
  if(ConfigPVI.fAutoRefreshMode != FALSE)
    del_timer(&pvi_refersh_timer);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int pvi_SwitchCommand(PTDisplayCommand pDisplayCommand)
{
int ReturnCode=0;

  lock_kernel();
  switch(pDisplayCommand->Command)  {
		case dc_NewImage:
	    DDPRINTF("dc_NewImage\n");
      ReturnCode=pvi_ioctl_NewImage(pDisplayCommand);
//=============================================================================
// Arron
			partial = 0;
//------------------------------------------------------------------------------
			break;

		case dc_StopNewImage:
	    DDPRINTF("dc_StopNewImage\n");
      ReturnCode=pvi_ioctl_StopNewImage(pDisplayCommand);
			break;

		case dc_DisplayImage:
//==============================================================================
		if(partial==0)
		{
	    		DDPRINTF("dc_DisplayImage\n");
      			ReturnCode=pvi_ioctl_DisplayImage(pDisplayCommand);
		}else{
	    		DDPRINTF("dc_DisplayPartial dc_DisplayImage\n");
      			ReturnCode=pvi_ioctl_DisplayPartial(pDisplayCommand);
		}
		//kay add battery detect
                //s3c2410_adc_read_once();
//------------------------------------------------------------------------------
			break;

		case dc_PartialImage:
	    DDPRINTF("dc_PartialImage\n");
		  ReturnCode=pvi_ioctl_PartialImage(pDisplayCommand);
//=============================================================================
// Arron
			partial = 1;
//------------------------------------------------------------------------------
			break;

		case dc_DisplayPartial:
	    DDPRINTF("dc_DisplayPartial dc_DisplayPartial\n");
      ReturnCode=pvi_ioctl_DisplayPartial(pDisplayCommand);
			break;

		case dc_DisplayPartialGU:
	    DDPRINTF("dc_DisplayPartial dc_DisplayPartial\n");
      ReturnCode=pvi_ioctl_DisplayPartialGU(pDisplayCommand);
			break;

		case dc_Reset:
	    DDPRINTF("dc_Reset\n");
      ReturnCode=pvi_ioctl_Reset(pDisplayCommand);
			break;

		case dc_SetDepth:
	    DDPRINTF("dc_SetDepth\n");
      ReturnCode=pvi_ioctl_SetDepth(pDisplayCommand);
			break;

		case dc_EraseDisplay:
	    DDPRINTF("dc_EraseDisplay\n");
      ReturnCode=pvi_ioctl_EraseDisplay(pDisplayCommand);
			break;

		case dc_Rotate:
	    DDPRINTF("dc_Rotate\n");
      ReturnCode=pvi_ioctl_Rotate(pDisplayCommand);
			break;

		case dc_Positive:
	    DDPRINTF("dc_Positive\n");
      ReturnCode=pvi_ioctl_Positive(pDisplayCommand);
			break;

		case dc_Negative:
	    DDPRINTF("dc_Negative\n");
      ReturnCode=pvi_ioctl_Negative(pDisplayCommand);
			break;

		case dc_GoToNormal:
	    DDPRINTF("dc_GoToNormal\n");
      ReturnCode=pvi_ioctl_GoToNormal(pDisplayCommand);
			break;

		case dc_GoToSleep:
	    DDPRINTF("dc_GoToSleep\n");
      ReturnCode=pvi_ioctl_GoToSleep(pDisplayCommand);
			break;

		case dc_GoToStandBy:
	    DDPRINTF("dc_GoToStandBy\n");
      ReturnCode=pvi_ioctl_GoToStandBy(pDisplayCommand);
			break;

		case dc_WriteToFlash:
	    DDPRINTF("dc_WriteToFlash\n");
      ReturnCode=pvi_ioctl_WriteToFlash(pDisplayCommand);
			break;

		case dc_ReadFromFlash:
	    DDPRINTF("dc_ReadFromFlash\n");
      ReturnCode=pvi_ioctl_ReadFromFlash(pDisplayCommand);
			break;

		case dc_Init:
	    DDPRINTF("dc_Init+\n");
      ReturnCode=pvi_ioctl_Init(pDisplayCommand);
	    DDPRINTF("dc_Init-\n");
			break;

		case dc_AutoRefreshOn:
	    DDPRINTF("dc_AutoRefreshOn\n");
      ReturnCode=pvi_ioctl_AutoRefreshOn(pDisplayCommand);
			break;

		case dc_AutoRefreshOff:
	    DDPRINTF("dc_AutoRefreshOff\n");
      ReturnCode=pvi_ioctl_AutoRefreshOff(pDisplayCommand);
			break;

		case dc_SetRefresh:
	    DDPRINTF("dc_SetRefresh\n");
      ReturnCode=pvi_ioctl_SetRefresh(pDisplayCommand);
			break;

		case dc_ForcedRefresh:
	    DDPRINTF("dc_ForcedRefresh\n");
      ReturnCode=pvi_ioctl_ForcedRefresh(pDisplayCommand);
			break;

		case dc_GetRefresh:
	    DDPRINTF("dc_GetRefresh\n");
      ReturnCode=pvi_ioctl_GetRefresh(pDisplayCommand);
			break;

		case dc_RestoreImage:
	    DDPRINTF("dc_RestoreImage\n");
      ReturnCode=pvi_ioctl_RestoreImage(pDisplayCommand);
			break;

		case dc_ControllerVersion:
	    DDPRINTF("dc_ControllerVersion\n");
      ReturnCode=pvi_ioctl_ControllerVersion(pDisplayCommand);
			break;

		case dc_SoftwareVersion:
	    DDPRINTF("dc_SoftwareVersion\n");
      ReturnCode=pvi_ioctl_SoftwareVersion(pDisplayCommand);
			break;

		case dc_DisplaySize:
	    DDPRINTF("dc_DisplaySize\n");
      ReturnCode=pvi_ioctl_DisplaySize(pDisplayCommand);
			break;

		case dc_GetStatus:
	    DDPRINTF("dc_GetStatus\n");
      ReturnCode=pvi_ioctl_GetStatus(pDisplayCommand);
			break;

		case dc_Temperature:
	    DDPRINTF("dc_Temperature\n");
      ReturnCode=pvi_ioctl_Temperature(pDisplayCommand);
			break;

		case dc_WriteRegister:
	    DDPRINTF("dc_WriteRegister\n");
      ReturnCode=pvi_ioctl_WriteRegister(pDisplayCommand);
			break;

		case dc_ReadRegister:
	    DDPRINTF("dc_ReadRegister\n");
      ReturnCode=pvi_ioctl_ReadRegister(pDisplayCommand);
			break;

	  default:
	    DDPRINTF("unknown command\n");
	    ReturnCode=-EFAULT;
  }
  unlock_kernel();
  return ReturnCode;
}

void pvi_SwitchDynamic(int n) {

	isdynamic = n;

}

static BOOL Flash_GetID(PBYTE pCodeID)
{
BOOL ret=FALSE;

  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x9f) == FALSE)
  	goto _flash_GetIDFail;
  SPI_FlashWriteDummy();
  pCodeID[0]=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
  SPI_FlashWriteDummy();
  pCodeID[1]=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
  SPI_FlashWriteDummy();
  pCodeID[2]=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
  SPI_FlashWriteDummy();
  pCodeID[3]=(BYTE) BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);

  ret=TRUE;
_flash_GetIDFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL Flash_Erase(DWORD StartAddr, DWORD Length)
{
BOOL ret=FALSE;
DWORD Sectors, Loops;
WORD Status;

	if(((StartAddr & 0xfff) != 0) || ((Length & 0xfff) != 0))
		return FALSE;
  Sectors=Length/0x1000;

  for(; Sectors--; StartAddr+=0x1000)
    {
  		if(spi_FlashUnProtect(StartAddr & ~0xffff) == FALSE)
  			{
  				goto _Flash_EraseFail;
  			}
  		if(spi_FlashEnableWrite() == FALSE)
  			{
  				goto _Flash_EraseFail;
  			}
  		SPI_FlashSwitchToHost();
  		if(SPI_FlashWriteData(0x20) == FALSE)
  		  goto  _Flash_EraseFail;
  		if(SPI_FlashWriteData(((StartAddr >> 16) & 0xff)) == FALSE)
  			goto  _Flash_EraseFail;
			if(SPI_FlashWriteData(((StartAddr >> 8) & 0xff)) == FALSE)
  			goto  _Flash_EraseFail;
			if(SPI_FlashWriteData((StartAddr & 0xff)) == FALSE)
  			goto  _Flash_EraseFail;
			SPI_FlashSwitchToDPE();
			for(Loops=10000; Loops--;)
  			{
					Status=spi_GetStatus();
					if((Status & 1) == 0)
						break;
  			}
    }
  ret=TRUE;
_Flash_EraseFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL Flash_Write(DWORD FlashAddress, DWORD Length, PBYTE pData)
{
BOOL ret=FALSE;
DWORD Loops;
WORD Status;

  for(; (Length-- != 0);)
    {
  		if(spi_FlashEnableWrite() == FALSE)
  			{
  				goto _Flash_WriteFail;
  			}
  		SPI_FlashSwitchToHost();
  		if(SPI_FlashWriteData(0x02) == FALSE)
  			goto  _Flash_WriteFail;
  		if(SPI_FlashWriteData(((FlashAddress >> 16) & 0xff)) == FALSE)
  			goto  _Flash_WriteFail;
  		if(SPI_FlashWriteData(((FlashAddress >> 8) & 0xff)) == FALSE)
  			goto  _Flash_WriteFail;
  		if(SPI_FlashWriteData((FlashAddress & 0xff)) == FALSE)
  			goto  _Flash_WriteFail;
    		if(SPI_FlashWriteData((*pData)) == FALSE)
  			goto  _Flash_WriteFail;

  		SPI_FlashSwitchToDPE();
  		FlashAddress++;
  		pData++;
  		for(Loops=10000; Loops--;)
  		  {
  			  Status=spi_GetStatus();
  			  if((Status & 1) == 0)
  			  	break;
  		  }
    }
  ret=TRUE;
_Flash_WriteFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

static BOOL Flash_Read(DWORD FlashAddress, DWORD Length, PBYTE pData)
{
BOOL ret=FALSE;

  SPI_FlashSwitchToHost();
  if(SPI_FlashWriteData(0x03) == FALSE)
  	goto  _Flash_ReadFail;
  if(SPI_FlashWriteData(((FlashAddress >> 16) & 0xff)) == FALSE)
  	goto  _Flash_ReadFail;
  if(SPI_FlashWriteData(((FlashAddress >> 8) & 0xff)) == FALSE)
  	goto  _Flash_ReadFail;
  if(SPI_FlashWriteData((FlashAddress & 0xff)) == FALSE)
  	goto  _Flash_ReadFail;

	for(; Length-- != 0; pData++)
	  {
  	  SPI_FlashWriteDummy();
	    *pData=BusIssueReadReg(_EPSON_REG_SPI_READ_DATA);
	  }
  ret=TRUE;
_Flash_ReadFail:
  SPI_FlashSwitchToDPE();
  return ret;
}

int BusIssueFlashOperation(PS1D13532_FLASH_PACKAGE pFlashControl)
{
int RetCode=-1;

	switch(pFlashControl->Command) {
		case _FLASH_CMD_GET_INFO:
			if(Flash_GetID(pFlashControl->Buf) == FALSE)
				break;
			RetCode=4;
			break;

		case _FLASH_CMD_ERASE:
			if(Flash_Erase(pFlashControl->StartAddr, pFlashControl->DataLength) == FALSE)
				break;
			RetCode=0;
			break;

		case _FLASH_CMD_WRITE:
			if(Flash_Write(pFlashControl->StartAddr, pFlashControl->DataLength, pFlashControl->Buf) == FALSE)
				break;
			RetCode=0;
			break;

		case _FLASH_CMD_READ:
			if(Flash_Read(pFlashControl->StartAddr, pFlashControl->DataLength, pFlashControl->Buf) == FALSE)
				break;
			break;

		default:
			return FALSE;
	};
	return RetCode;
}

