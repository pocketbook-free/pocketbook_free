/*
 *  epson.h
 *
 *  Driver for the epson EINK panel controller
 *
 *  This software may be used and distributed according to the
 *  terms of the GNU Public License, Version 2, incorporated
 *  herein by reference.
 *
 */

#ifndef __EPSON_H__
  #define __EPSON_H__

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  #include "DataType.h"
  #include "s1d13521.h"
  #include "s1d13521ioctl.h"

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//  #define __2BPP__
  #define __3BPP__
  //#define __4BPP__
  //#define __BYTE_BPP__

  #define _MAX_PARAMS_PER_COMMAND			5

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  #define _PANEL_WIDTH               		600
  #define _PANEL_HEIGHT              		800
  #define _BASE_IMAGE_BUFFER				0x30000000
  #define _BASE_PACKED_IMAGE				0x30100000
  #define _BASE_ERASE_IMAGE					0x30200000

  #define _TOTAL_PARTIAL_IMAGE_AREA			1

  #define _PANEL_TOTAL_PEXIL           	(_PANEL_WIDTH*_PANEL_HEIGHT)

  #if    defined(__2BPP__) && !defined(__3BPP__) && !defined(__4BPP__) && !defined(__BYTE_BPP__)

    #define _PEXILE_PER_BYTE				4
    #define _CMD_LD_IMG_DATA_FORMAT			(0x00 << 4)
    #define _CMD_LD_IMG_AREA_DATA_FORMAT	(0x00 << 4)

    #define _ERASE_COLOR_WHITE				(0xffff)
    #define _ERASE_COLOR_BLACK				(0x0000)

  #elif !defined(__2BPP__) &&  defined(__3BPP__) && !defined(__4BPP__) && !defined(__BYTE_BPP__)

    #define _PEXILE_PER_BYTE				2
    #define _CMD_LD_IMG_DATA_FORMAT			(0x01 << 4)
    #define _CMD_LD_IMG_AREA_DATA_FORMAT	(0x01 << 4)

    #define _ERASE_COLOR_WHITE				(0xeeee)
    #define _ERASE_COLOR_BLACK				(0x0000)

  #elif  defined(__2BPP__) && !defined(__3BPP__) &&  defined(__4BPP__) && !defined(__BYTE_BPP__)

    #define _PEXILE_PER_BYTE				2
    #define _CMD_LD_IMG_DATA_FORMAT			(0x02 << 4)
    #define _CMD_LD_IMG_AREA_DATA_FORMAT	(0x02 << 4)

    #define _ERASE_COLOR_WHITE				(0xffff)
    #define _ERASE_COLOR_BLACK				(0x0000)

  #elif  defined(__2BPP__) && !defined(__3BPP__) && !defined(__4BPP__) &&  defined(__BYTE_BPP__)

    #define _PEXILE_PER_BYTE				1
    #define _CMD_LD_IMG_DATA_FORMAT			(0x03 << 4)
    #define _CMD_LD_IMG_AREA_DATA_FORMAT	(0x03 << 4)

    #define _ERASE_COLOR_WHITE				(0xffff)
    #define _ERASE_COLOR_BLACK				(0x0000)

  #else
    #error "Specified the bpp, please!!!!s"
  #endif

  #define _PEXILE_PER_TRANSFER				(_PEXILE_PER_BYTE << 1)
  #define _FRAME_BUFFER_SIZE				(_PANEL_WIDTH*_PANEL_HEIGHT)

  #define _PACKED_FRAME_TRANSFERS			(_PANEL_TOTAL_PEXIL/_PEXILE_PER_TRANSFER)
  #define _PACKED_FRAME_WORD_TRANSFERS		(_PACKED_FRAME_TRANSFERS)
  #define _PACKED_FRAME_DWORD_TRANSFERS		(_PACKED_FRAME_TRANSFERS >> 1)

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  typedef enum {
  	_WAVE_FORM_INIT=0,
  	_WAVE_FORM_DU,
  	_WAVE_FORM_GU,
  	_WAVE_FORM_DC
  } E_EPSON_WAVE_FORM_MODE;

  typedef enum {
  	_ROATE_NONE=0,
  	_ROATE_90,
  	_ROATE_180,
  	_ROATE_270
  } E_EPSON_ROTATE_DEGREE;

  typedef enum {
  	_PEXIL_BPP_2BITS=0,
  	_PEXIL_BPP_3BITS,
  	_PEXIL_BPP_4BITS,
  	_PEXIL_BPP_5BITS
  } E_EPSON_PEXIL_WIDTH;

  typedef enum {
  	_PEXIL_MODE_PACKED=0,
  	_PEXIL_MODE_PEXIL_PER_BYTE
  } E_EPSON_PEXIL_DATA_FORMAT;

  typedef enum {
  	_UPDATE_COMMAND_FULL=0,
  	_UPDATE_COMMAND_PARTIAL
  } E_EPSON_UPDATE_COMMAND;

  typedef enum {
  	_PIPE_0=0,
  	_PIPE_1,
  	_PIPE_2,
  	_PIPE_3,
  	_PIPE_4,
  	_PIPE_5,
  	_PIPE_6,
  	_PIPE_7,
  	_PIPE_8,
  	_PIPE_9,
  	_PIPE_10,
  	_PIPE_11,
  	_PIPE_12,
  	_PIPE_13,
  	_PIPE_14,
  	_PIPE_15
  } E_EPSON_PIPE_NO;

  #pragma pack(1)
  typedef struct _TAG_ST_IMAGE_AREA {
  	DWORD StartX;
  	DWORD StartY;
  	DWORD Width;
  	DWORD Height;
  } ST_IMAGE_AREA, *PST_IMAGE_AREA;

  typedef struct _TAG_ST_PARTIAL_IMAGE_CTB {

  	PVOID 						pBuf;
  	DWORD						BaseLTU;
  	ST_IMAGE_AREA				Area;

  	BOOL  						f_BorderDetect;
  	E_EPSON_PIPE_NO				PipeNo;
  	E_EPSON_ROTATE_DEGREE		RotateDegree;
  	E_EPSON_PEXIL_DATA_FORMAT	Format;
  	E_EPSON_PEXIL_WIDTH			PexilWidth;
  	E_EPSON_WAVE_FORM_MODE		WaveForm;

  	E_EPSON_UPDATE_COMMAND		UpdateCommand;

  } ST_PARTIAL_IMAGE_CTB, *PST_PARTIAL_IMAGE_CTB;

  #pragma pack()

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  #define _EPSON_DISPLAY_WIDTH              600
  #define _EPSON_DISPLAY_HEIGHT             800
  #define _EPSON_DISPLAY_BPP                8
  #define _EPSON_DISPLAY_SCANLINE_BYTES     600

  #define _EPSON_FB_SIZE_BYTES 				((_EPSON_DISPLAY_WIDTH*_EPSON_DISPLAY_HEIGHT*_EPSON_DISPLAY_BPP)/8)

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  ////////////////////////////////////////////////////////
  //
  //
  //
  //

  ////////////////////////////////////////////////////////
  //
  //
  //
  //
  #define _CMD_UPD_FULL_BOADER_UPDATE_ENABLE		0x1
  #define _CMD_UPD_FULL_BOADER_UPDATE_DISABLE		0x0
  #define _BOADER_UPDATE_ENABLE						0x1
  #define _BOADER_UPDATE_DISABLE					0x0
  #define BULD_CMD_UPD_FULL_BOARDER(x)				(((x) & 1) << 14)

  #define _CMD_UPD_FULL_WAVEFORM_INIT				(0x0)
  #define _CMD_UPD_FULL_WAVEFORM_DU					(0x1)
  #define _CMD_UPD_FULL_WAVEFORM_GU					(0x2)
  #define _CMD_UPD_FULL_WAVEFORM_GC					(0x3)
  #define BULD_CMD_UPD_FULL_WAVEFORM(x)				(((x) & 3) << 8)

  #define BUILD_CMD_UPD_FULL_SEL_LTU(x)				(((x) & 0xf) << 4)

  #define BULD_CMD_UPD_FULL(p1, p2, p3)				(BULD_CMD_UPD_FULL_BOARDER(p1) | BULD_CMD_UPD_FULL_WAVEFORM(p2) | (BUILD_CMD_UPD_FULL_SEL_LTU(p3)))

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  #define _EPSON_REG_STATUS				0x0a
  #define _EPSON_REG_WRITE_IMAGE		0x154

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //
  //
  //
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  #define _EPSON_PACKAGE_WITH_PRT			0x80000000
  #pragma pack(1)

  struct ST_TAG_EPSON_PACKAGE {

  	DWORD DataLength;
  	WORD  Command;
  	union {
  	  WORD  wMSG[6];
  	  PWORD pMSG;
	} Data;
  };

  typedef struct ST_TAG_EPSON_PACKAGE ST_EPSON_PACKAGE;
  typedef struct ST_TAG_EPSON_PACKAGE *PST_EPSON__PACKAGE;

  #pragma pack()

  extern ST_EPSON_PACKAGE 				epson_cmd_INIT_SYS_RUN;
  extern ST_EPSON_PACKAGE 				epson_cmd_INIT_DSPE_CFG;
  extern ST_EPSON_PACKAGE 				epson_cmd_INIT_DSPE_TMG;
  extern ST_EPSON_PACKAGE 				epson_cmd_RD_WFM_INFO;
  extern ST_EPSON_PACKAGE 				epson_cmd_UPD_GDRV_CLR;
  extern ST_EPSON_PACKAGE 				epson_cmd_WAIT_DSPE_TRG;
  extern ST_EPSON_PACKAGE 				epson_cmd_INIT_ROTMODE;
  extern ST_EPSON_PACKAGE 				epson_cmd_WAIT_DSPE_FREND;
  extern ST_EPSON_PACKAGE 				epson_cmd_UPD_FULL;
  extern ST_EPSON_PACKAGE 				epson_cmd_LD_IMG_END;
  extern ST_EPSON_PACKAGE 				epson_cmd_LD_IMG;
  extern ST_EPSON_PACKAGE 				epson_cmd_WR_REG_0x154;

#endif /* __EPSON_H__ */
