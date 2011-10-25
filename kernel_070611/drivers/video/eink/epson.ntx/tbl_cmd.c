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
//#include <common.h>
//#include <s3c2440.h>
//#include <s3c24x0.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>

#include "epson.h"

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
ST_EPSON_PACKAGE epson_cmd_INIT_SYS_RUN = {
	.DataLength  = 0,
	.Command	 = INIT_SYS_RUN,		// 0x06
};

ST_EPSON_PACKAGE epson_cmd_INIT_DSPE_CFG = {
	.DataLength  = 5,
	.Command		 = INIT_DSPE_CFG,	// 0x09
	.Data 		 = {
	  .wMSG= {
 	   BS60_INIT_HSIZE,
 	   BS60_INIT_VSIZE,
 	   BS60_INIT_SDRV_CFG,
 	   BS60_INIT_GDRV_CFG,
 	   BS60_INIT_LUTIDXFMT,
	  },
	}
};

ST_EPSON_PACKAGE epson_cmd_INIT_DSPE_TMG = {
	.DataLength  = 5,
	.Command	 = INIT_DSPE_TMG,		// 0x0A
	.Data 		 = {
	  .wMSG= {
		BS60_INIT_FSLEN,
       (BS60_INIT_FELEN<<8)|BS60_INIT_FBLEN,
        BS60_INIT_LSLEN,
       (BS60_INIT_LELEN<<8)|BS60_INIT_LBLEN,
        BS60_INIT_PIXCLKDIV}
    }
};

ST_EPSON_PACKAGE epson_cmd_RD_WFM_INFO  = {
	.DataLength  = 2,
	.Command	 = RD_WFM_INFO,			// 0x30
	.Data 		 = {
	  .wMSG= {0x0886}
	}
};


ST_EPSON_PACKAGE epson_cmd_UPD_GDRV_CLR = {
	.DataLength  = 0,
	.Command	 = UPD_GDRV_CLR,		// 0x37
};

ST_EPSON_PACKAGE epson_cmd_WAIT_DSPE_TRG= {
	.DataLength  = 0,
	.Command	 = WAIT_DSPE_TRG,
};

ST_EPSON_PACKAGE epson_cmd_INIT_ROTMODE = {
	.DataLength  = 1,
	.Command	 = INIT_ROTMODE,		// 0x0b
	.Data 		 = {
	  .wMSG=  { (BS60_INIT_ROTMODE << 8) }
	}
};

ST_EPSON_PACKAGE epson_cmd_WAIT_DSPE_FREND= {
	.DataLength  = 0,
	.Command	 = WAIT_DSPE_FREND,
};

ST_EPSON_PACKAGE epson_cmd_UPD_FULL= {
	.DataLength  = 1,
	.Command	 = UPD_FULL,
	.Data 		 = {
	  .wMSG=  { ((WF_MODE_INIT << 8) | 0x4000) }
	}
};

ST_EPSON_PACKAGE epson_cmd_UPD_FULL_2= {
	.DataLength  = 1,
	.Command	 = UPD_FULL,
	.Data 		 = {
	  .wMSG=  { (WF_MODE_GC << 8) }
	}
};

ST_EPSON_PACKAGE epson_cmd_LD_IMG_END= {
	.DataLength  = 0,
	.Command	 = LD_IMG_END,
};

ST_EPSON_PACKAGE epson_cmd_LD_IMG= {
	.DataLength  = 1,
	.Command	 = LD_IMG,
	.Data 		 = {
	  .wMSG=  { (0x3 << 4) }
	}
};

ST_EPSON_PACKAGE epson_cmd_WR_REG_0x154= {
	.DataLength  = 1,
	.Command	 = WR_REG,
	.Data 		 = {
	  .wMSG=  { (0x154) }
	}
};

