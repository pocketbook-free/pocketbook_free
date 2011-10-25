/*
Elan Initial S3C6410 for 668 
*/

//#include <s3c6410.h> 
//#include <Elan668.h>
#include <asm/gpio.h>
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include "Elan668.h"
//#include "ioc668-dev.h"
//#include <linux/ioc668-dev.h>
//#include "Elan_Initial_6410vs668.h"

extern unsigned short fw_a_area[8453] ;
extern unsigned short fw_b_area[8453] ;

extern int get_epd_config() ;

unsigned short *EEPROM_SRC_Addr, *ROM_SRC_Addr, *CODEOPTION_SRC_Addr, *CHECKSUM_SRC_Addr;
unsigned short *EEPROM_DES_Addr, *ROM_DES_Addr, *CODEOPTION_DES_Addr, *CHECKSUM_DES_Addr;

void Elan_Initial_6410vs668(void)
{
//	printk("~~~~~~~~~~~~~~~~Elan_Initial_6410vs668~~~~~~~~~~~~~~~~\n") ;
		
//	fw_a_area[0] = 0x001B;
//	fw_a_area[15] = 0x002B;
//	fw_a_area[16] = 0x003C;
//	fw_a_area[31] = 0x004D;
//	fw_a_area[240] = 0x00CC;
//	fw_a_area[255] = 0x00DD;
//
//	fw_a_area[256] = 0x12AB;
//	fw_a_area[271] = 0x03CD;
//	fw_a_area[272] = 0x14EF;
//	fw_a_area[287] = 0x0555;
//	fw_a_area[8432] = 0x1222;
//	fw_a_area[8447] = 0x1555;
//	
//	fw_a_area[8448] = 0x7C58;
//	fw_a_area[8449] = 0x706A;
//	fw_a_area[8450] = 0x078B;

	EEPROM_SRC_Addr = &fw_a_area[0];				//EEPROM_SRC_Addr			= EEPROM Source Addr			 				(256*16bit  & 0x00ff) 
	ROM_SRC_Addr = &fw_a_area[256];					//ROM_SRC_Addr 				= ROM Source Addr				 					(8192*16bit & 0x7fff) 
	CODEOPTION_SRC_Addr = &fw_a_area[8448];	//CODEOPTION_SRC_Addr = CODEOPTION Source Addr	 				(3*16bit    & 0xffff) 
	CHECKSUM_SRC_Addr = &fw_a_area[8451];		//CHECKSUM_SRC_Addr   = ROM CHECKSUM Source Add					(1*16bit		& 0xffff) 
																					//CHECKSUM_SRC_Addr+1 = EEPROM CHECKSUM Source Addr			(1*16bit		& 0xffff) 

	EEPROM_DES_Addr = &fw_b_area[0];				//EEPROM_DES_Addr			= EEPROM Destination Addr			 		(256*16bit  & 0x00ff)
	ROM_DES_Addr = &fw_b_area[256];					//ROM_DES_Addr 				= ROM Destination Addr				 		(8192*16bit & 0x7fff) 
	CODEOPTION_DES_Addr = &fw_b_area[8448];	//CODEOPTION_DES_Addr = CODEOPTION Destination Addr 		(3*16bit    & 0xffff) 
	CHECKSUM_DES_Addr = &fw_b_area[8451];		//CHECKSUM_DES_Addr   = ROM CHECKSUM Destination Addr		(1*16bit    & 0xffff)
																					//CHECKSUM_DES_Addr+1 = EEPROM CHECKSUM Destination Addr(1*16bit    & 0xffff)
}


void Elan_Define_668_KIN_AS_OUTPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPL(11), S3C_GPIO_SFN(1));
	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_KIN_AS_INPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPL(11), S3C_GPIO_SFN(0));
	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_VDD_AS_OUTPUT_PIN(void)
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
			s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_UP);
		}
		else
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
			s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_UP);
		}
	
//	s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_VDD_AS_INPUT_PIN(void)
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(0));
			s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_UP);
		}
		else
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(0));
			s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_UP);
		}
	
//	s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_RST_AS_OUTPUT_PIN(void)
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
			s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_UP);
		}
		else
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
			s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_UP);
		}
	
//	s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_RST_AS_INPUT_PIN(void)
{
	int epd_type = get_epd_config();
		if(epd_type == EPD_SC8_TYPE)
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(0));
			s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_UP);
		}
		else
		{
			s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(0));
			s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_UP);
		}
	
//	s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_DAT_AS_OUTPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPP(0), S3C_GPIO_SFN(1));
	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_DAT_AS_INPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPP(0), S3C_GPIO_SFN(0));
	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_CLK_AS_OUTPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPP(7), S3C_GPIO_SFN(1));
	s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_NONE);
}

void Elan_Define_668_CLK_AS_INPUT_PIN(void)
{
	s3c_gpio_cfgpin(S3C64XX_GPP(7), S3C_GPIO_SFN(0));
	s3c_gpio_setpull(S3C64XX_GPN(7), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C64XX_GPN(7), S3C_GPIO_PULL_DOWN);
//	s3c_gpio_setpull(S3C64XX_GPN(7), S3C_GPIO_PULL_NONE);
}

//delay function
void Elan_Define_Delay1us(void)
{
	udelay(1) ;
}

void Elan_Define_Delay5us(void)
{
	volatile int	Elan_5us;
	for(Elan_5us=0;Elan_5us<5;Elan_5us++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay10us(void)
{
	volatile int	Elan_10us;
	for(Elan_10us=0;Elan_10us<10;Elan_10us++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay50us(void)
{
	volatile int	Elan_50us;
	for(Elan_50us=0;Elan_50us<50;Elan_50us++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay100us(void)
{
	volatile int	Elan_100us;
	for(Elan_100us=0;Elan_100us<100;Elan_100us++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay500us(void)
{
	volatile int	Elan_500us;
	for(Elan_500us=0;Elan_500us<470;Elan_500us++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay1ms(void)
{
	volatile int	Elan_1ms;
	for(Elan_1ms=0;Elan_1ms<950;Elan_1ms++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay5ms(void)
{
	volatile int	Elan_5ms;
	for(Elan_5ms=0;Elan_5ms<4730;Elan_5ms++)
	Elan_Define_Delay1us();
}

void Elan_Define_Delay100ms(void)
{
	volatile int	Elan_100ms;
	for(Elan_100ms=0;Elan_100ms<100;Elan_100ms++)
	Elan_Define_Delay1ms();
}

void Elan_Define_Delay500ms(void)
{
	volatile int	Elan_500ms;
	for(Elan_500ms=0;Elan_500ms<500;Elan_500ms++)
	Elan_Define_Delay1ms();
}
