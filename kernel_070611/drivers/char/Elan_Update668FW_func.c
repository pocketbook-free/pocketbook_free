/*
Elan Update 668 FirmWare Functions
*/ 

//#include <s3c6410.h> 
#include <linux/delay.h>
#include "Elan668.h"
#include "Elan_Initial_6410vs668.h"
//#include "Elan_Update668FW_func.h"

#define Elan_Define_ICnum				0x668
#define Elan_CODEOPTION_INIT		0x000
#define Elan_CODEOPTION_WR			0x001
#define Elan_CODEOPTION_END			0x002


extern unsigned short *EEPROM_SRC_Addr, *ROM_SRC_Addr, *CODEOPTION_SRC_Addr, *CHECKSUM_SRC_Addr;
extern unsigned short *EEPROM_DES_Addr, *ROM_DES_Addr, *CODEOPTION_DES_Addr, *CHECKSUM_DES_Addr;

extern unsigned short fw_b_area[8453] ;
extern unsigned short B2_CODEOPTION[3] ;

extern int get_epd_config() ;

//Global Variable --------------------------------------------------------------------------------------------
	unsigned short 	Elan_Count[3];
	unsigned short 	Elan_Temp[2];
	unsigned short 	Elan_Prgbuffer[16];
	unsigned short 	Elan_UpdateResult=0;
	



unsigned int ioc_vdd_control(int ctrl)
{
		int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
			{
					if(ctrl == IOC_VDD_ON)	
					{
						__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) ; //GPO8=L
					}	
					else if(ctrl == IOC_VDD_OFF)
					{
						__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) ; //GPO8=H
					}
			}
		else
		{
			if(ctrl == IOC_VDD_ON)
			{
					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) ; //GPO0=L
			}	
			else if(ctrl == IOC_VDD_OFF)
			{
				__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	; //GPO0=H
			}	
		}
		
}

unsigned int ioc_rst_control(int ctrl)
{
		int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
			{
					if(ctrl == IOC_RST_LOW)	
					{
						__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT) ;	//GPO0=H
						//__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) //GPO8=L
					}	
					else if(ctrl == IOC_RST_HIGH)
					{
						__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT)  ;//GPO0=L
						//__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) //GPO8=H
					}
			}
		else
		{
			if(ctrl == IOC_RST_LOW)
			{
					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) ; //GPO8=H
			}	
			else if(ctrl == IOC_RST_HIGH)
			{
				__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) ; //GPO8=L
			}	
		}
		
}

unsigned int ioc_read_vdd()
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01) ;
		}
		else
		{
			((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)	;														//Read GPO0 Status
		}
}

unsigned int ioc_read_rst()
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)	;													//Read GPO0 Status
		}
		else
		{
			((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01)	;														//Read GPO8 Status
		}
}

	// int epd_type = -2 ;
	// epd_type = get_epd_config() ;
	
	//	if(epd_type == 8)
	//		#define SC8_EPD
	
			
//	 #if defined SC8_EPD
//	 //{
//	 	#define Elan_Set_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) | 0x00000800 , S3C64XX_GPLDAT)	//GPL11=H
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
//		#define Elan_Read_RST_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)														//Read GPO8 Status
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
//	// }

//	 #else
//	 	#define Elan_Set_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) | 0x00000800 , S3C64XX_GPLDAT)	//GPL11=H
//		#define Elan_Clr_KIN					__raw_writel(__raw_readl(S3C64XX_GPLDAT) & 0xfffff7ff , S3C64XX_GPLDAT) //GPL11=L
//		#define Elan_Read_KIN_Status	((__raw_readl(S3C64XX_GPLDAT) >> 11) & 0x01)														//Read GPL11 Status
//		
//		#define Elan_Set_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) //GPO0=L
//		#define Elan_Clr_VDD					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT)	//GPO0=H
//		#define Elan_Read_VDD_Status	((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01)															//Read GPO0 Status
//		
//		#define Elan_Set_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) //GPO8=L
//		#define Elan_Clr_RST					__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) //GPO8=H
//		#define Elan_Read_RST_Status	((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01)															//Read GPO8 Status
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
//	 #endif //}
	
	
	


//Functions ---------------------------------------------------------------------------------------------------
void Elan_Clr_AllPin(void)
{
	Elan_Clr_DAT;
	Elan_Clr_CLK;
	Elan_Clr_RST;
	Elan_Clr_VDD;
}

//------------------------------------------------------------------------------------------------------------
void Elan_Clr_AllPin_noRSTpin(void)
{
	Elan_Clr_DAT;
	Elan_Clr_CLK;
	Elan_Clr_VDD;
}

//------------------------------------------------------------------------------------------------------------
void Elan_OUTPUT_1CLK(void)
{
	Elan_Define_Delay1us();
	Elan_Set_CLK;
	Elan_Define_Delay1us();
	Elan_Clr_CLK;
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_Password(unsigned short Elan_Password)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_Password_noRSTpin(unsigned short Elan_Password1)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_CommandCode(unsigned short Elan_CommandCode)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_15bitDATA(unsigned short Elan_15bitDATA)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_8bitDATA(unsigned short Elan_8bitDATA)
{
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_Read_15bitDATA(void)
{
	return 0;
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_Read_8bitDATA(void)
{
	return 0;
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_EEPROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_ROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_CODEOPTION(void)
{
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_BC_EEPROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_BC_ROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_EEPROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_ROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_CODEOPTION(unsigned short Elan_WrCODEOPTION)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_CODEOPTION_LVR01(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_EEPROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_ROM(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_CODEOPTION(unsigned short Elan_ReCODEOPTION)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_ALL(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_ALL(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_ALL(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_CODEOPTION_Only(void) // mode 0x6
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_CODEOPTION_Only_noRSTpin(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Ask_668_Status(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Update_EEPROM_only(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Update_ROM_only(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_LVR01_only(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_LVR01_only_noRSTpin(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_LVR01_only_noRSTpin_RWR(void)
{
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_LVR01_only_RWR(void) // mode 0x10
{
}

//Main Function ----------------------------------------------------------------------------------------------
unsigned short Elan_Update668FW_func(unsigned short Elan_FUNCTIONS)
{

}



