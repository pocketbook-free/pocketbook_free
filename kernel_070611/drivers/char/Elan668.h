/*
***********************************************
 * NAME	    : Elan668.h
***********************************************
*/

#ifndef __ELAN668_H__
#define __ELAN668_H__

#include <linux/io.h>

#include <plat/gpio-bank-o.h>
#include <plat/gpio-bank-p.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>


//	extern int get_epd_config() ;
#define IOC_VDD_ON 		0 //for gpio
#define IOC_VDD_OFF		1 //for gpio

#define IOC_RST_LOW   0 //for ioc pin
#define IOC_RST_HIGH  1 //for ioc pin

#define EPD_SC8_TYPE		8   
#define EPE_SC4_TYPE		4   



#define Elan_Set_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) | 0x00000800 , S3C64XX_GPLDAT)	//GPL11=H
#define Elan_Clr_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) & 0xfffff7ff , S3C64XX_GPLDAT) //GPL11=L
#define Elan_Read_KIN_Status	((__raw_readl(S3C64XX_GPLDAT) >> 11) & 0x01)														//Read GPL11 Status

#define Elan_Set_VDD					ioc_vdd_control(IOC_VDD_ON)
#define Elan_Clr_VDD					ioc_vdd_control(IOC_VDD_OFF)
//#define Elan_Set_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) //GPO0=L
//#define Elan_Clr_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	//GPO0=H
//#define Elan_Read_VDD_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)															//Read GPO0 Status
#define Elan_Read_VDD_Status	ioc_read_vdd()

#define Elan_Set_RST					ioc_rst_control(IOC_RST_HIGH)
#define Elan_Clr_RST					ioc_rst_control(IOC_RST_LOW)
//#define Elan_Set_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) //GPO8=L
//#define Elan_Clr_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) //GPO8=H
//#define Elan_Read_RST_Status	((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01)															//Read GPO8 Status
#define Elan_Read_RST_Status	ioc_read_rst()

#define Elan_Set_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000001 , S3C64XX_GPPDAT) //GPP0=H
#define Elan_Clr_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xfffffffe , S3C64XX_GPPDAT) //GPP0=L
#define Elan_Read_DAT_Status	((__raw_readl(S3C64XX_GPPDAT) >> 0) & 0x01)															//Read GPP0 Status

#define Elan_Set_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000080 , S3C64XX_GPPDAT) //GPP7=H
#define Elan_Clr_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xffffff7f , S3C64XX_GPPDAT) //GPP7=L
#define Elan_Read_CLK_Status	((__raw_readl(S3C64XX_GPPDAT) >> 7) & 0x01)															//Read GPP7 Status

#define Set_I2C_CLK						__raw_writel(__raw_readl(S3C64XX_GPBDAT) | 0x00000020 , S3C64XX_GPBDAT) //GPB5=H
#define Clr_I2C_CLK						__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffdf , S3C64XX_GPBDAT) //GPB5=L
#define Read_I2C_CLK_Status		((__raw_readl(S3C64XX_GPBDAT) >> 5) & 0x01)	

#define Set_I2C_DAT						__raw_writel(__raw_readl(S3C64XX_GPBDAT) | 0x00000040 , S3C64XX_GPBDAT) //GPB6=H
#define Clr_I2C_DAT						__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffbf , S3C64XX_GPBDAT) //GPB6=L
#define Read_I2C_DAT_Status		((__raw_readl(S3C64XX_GPBDAT) >> 6) & 0x01)	


//SC4
//#define Elan_Set_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) | 0x00000800 , S3C64XX_GPLDAT)	//GPL11=H
//#define Elan_Clr_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) & 0xfffff7ff , S3C64XX_GPLDAT) //GPL11=L
//#define Elan_Read_KIN_Status	((__raw_readl(S3C64XX_GPLDAT) >> 11) & 0x01)														//Read GPL11 Status
//
//#define Elan_Set_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) //GPO0=L
//#define Elan_Clr_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	//GPO0=H
//#define Elan_Read_VDD_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)															//Read GPO0 Status
//
//#define Elan_Set_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) //GPO8=L
//#define Elan_Clr_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) //GPO8=H
//#define Elan_Read_RST_Status	((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01)															//Read GPO8 Status
//
//#define Elan_Set_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000001 , S3C64XX_GPPDAT) //GPP0=H
//#define Elan_Clr_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xfffffffe , S3C64XX_GPPDAT) //GPP0=L
//#define Elan_Read_DAT_Status	((__raw_readl(S3C64XX_GPPDAT) >> 0) & 0x01)															//Read GPP0 Status
//
//#define Elan_Set_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000080 , S3C64XX_GPPDAT) //GPP7=H
//#define Elan_Clr_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xffffff7f , S3C64XX_GPPDAT) //GPP7=L
//#define Elan_Read_CLK_Status	((__raw_readl(S3C64XX_GPPDAT) >> 7) & 0x01)															//Read GPP7 Status


//SC8
//#define Elan_Set_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) | 0x00000800 , S3C64XX_GPLDAT)	//GPL11=H
//		#define Elan_Clr_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) & 0xfffff7ff , S3C64XX_GPLDAT) //GPL11=L
//		#define Elan_Read_KIN_Status	((__raw_readl(S3C64XX_GPLDAT) >> 11) & 0x01)														//Read GPL11 Status
//		
//		//#define Elan_Set_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) //GPO0=L
//		#define Elan_Set_VDD						__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) //GPO8=L
//		//#define Elan_Clr_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	//GPO0=H
//		#define Elan_Clr_VDD						__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) //GPO8=H
//		//#define Elan_Read_VDD_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)															//Read GPO0 Status
//		#define Elan_Read_VDD_Status		((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01)	
//		
//		
//		#define Elan_Set_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) //GPO0=L
//		#define Elan_Clr_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	//GPO0=H
//		#define Elan_Read_RST_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)														//Read GPO0 Status
//		
//		#define Elan_Set_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000001 , S3C64XX_GPPDAT) //GPP0=H
//		#define Elan_Clr_DAT					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xfffffffe , S3C64XX_GPPDAT) //GPP0=L
//		#define Elan_Read_DAT_Status	((__raw_readl(S3C64XX_GPPDAT) >> 0) & 0x01)															//Read GPP0 Status
//		
//		#define Elan_Set_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) | 0x00000080 , S3C64XX_GPPDAT) //GPP7=H
//		#define Elan_Clr_CLK					__raw_writel(__raw_readl(S3C64XX_GPPDAT) & 0xffffff7f , S3C64XX_GPPDAT) //GPP7=L
//		#define Elan_Read_CLK_Status	((__raw_readl(S3C64XX_GPPDAT) >> 7) & 0x01)															//Read GPP7 Status
//		
//		#define Set_I2C_CLK						__raw_writel(__raw_readl(S3C64XX_GPBDAT) | 0x00000020 , S3C64XX_GPBDAT) //GPB5=H
//		#define Clr_I2C_CLK						__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffdf , S3C64XX_GPBDAT) //GPB5=L
//		#define Read_I2C_CLK_Status		((__raw_readl(S3C64XX_GPBDAT) >> 5) & 0x01)	
//		
//		#define Set_I2C_DAT						__raw_writel(__raw_readl(S3C64XX_GPBDAT) | 0x00000040 , S3C64XX_GPBDAT) //GPB6=H
//		#define Clr_I2C_DAT						__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffbf , S3C64XX_GPBDAT) //GPB6=L
//		#define Read_I2C_DAT_Status		((__raw_readl(S3C64XX_GPBDAT) >> 6) & 0x01)	

#endif


