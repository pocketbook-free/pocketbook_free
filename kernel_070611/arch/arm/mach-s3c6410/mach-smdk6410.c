/* linux/arch/arm/mach-s3c6410/mach-smdk6410.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio-core.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>

#include <plat/regs-rtc.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/reserved_mem.h>
#include <plat/pm.h>

#include <linux/regulator/machine.h>
#include <linux/mfd/wm8350/audio.h>
#include <linux/mfd/wm8350/core.h>
#include <linux/mfd/wm8350/pmic.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
//&*&*&*HC1_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code
//#include <plat/gpio-bank-c.h>
//#include <plat/gpio-bank-m.h>
//#include <plat/gpio-bank-f.h>
//&*&*&*HC2_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code

#include <plat/pll.h>
#include <plat/spi.h>

#include <linux/android_pmem.h>
//#include "../../../drivers/char/ioc668-dev.h"
#include <linux/ioc668-dev.h>

#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <linux/usb/ch9.h>

#include <linux/mfd/max8698/core.h>   //Added For Jay


/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	0

#ifdef USB_HOST_PORT2_EN
#define OTGH_PHY_CLK_VALUE      (0x60)  /* Serial Interface, otg_phy input clk 48Mhz Oscillator */
#else
#define OTGH_PHY_CLK_VALUE      (0x20)  /* UTMI Interface, otg_phy input clk 48Mhz Oscillator */
#endif
#endif

#define UCON S3C_UCON_DEFAULT | S3C_UCON_UCLK
#define ULCON S3C_LCON_CS8 | S3C_LCON_PNONE | S3C_LCON_STOPB
#define UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE

#ifndef CONFIG_HIGH_RES_TIMERS
extern struct sys_timer s3c_timer;
#else
extern struct sys_timer sec_timer;
#endif /* CONFIG_HIGH_RES_TIMERS */

extern void s3c64xx_reserve_bootmem(void);
#if defined (CONFIG_HW_EP3_DVT) || defined (CONFIG_HW_EP1_DVT)
    extern void hanwang_touch_power_off();
    extern void hanwang_touch_power_on();
#endif

extern int key_lock; 					 				//blazer added on 2010.08.25

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
	/* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
	/* Whatever need to be done */
}

static void cs_set(int pin, int lvl)
{
	unsigned int val;

	val = __raw_readl(S3C64XX_GPFDAT);
	val &= ~(1<<pin);

	if(lvl == CS_HIGH)
	   val |= (1<<pin);

	__raw_writel(val, S3C64XX_GPFDAT);
}

static void s3c_cs_setF13(int pin, int lvl)
{
	cs_set(13, lvl);
}

static void s3c_cs_setF14(int pin, int lvl)
{
	cs_set(14, lvl);
}

static void s3c_cs_setF15(int pin, int lvl)
{
	cs_set(15, lvl);
}

static void cs_cfg(int pin, int pull)
{
	unsigned int val;

	val = __raw_readl(S3C64XX_GPFCON);
	val &= ~(3<<(pin*2));
	val |= (1<<(pin*2)); /* Output Mode */
	__raw_writel(val, S3C64XX_GPFCON);

	val = __raw_readl(S3C64XX_GPFPUD);
	val &= ~(3<<(pin*2));

	if(pull == CS_HIGH)
	   val |= (2<<(pin*2));	/* PullUp */
	else
	   val |= (1<<(pin*2)); /* PullDown */

	__raw_writel(val, S3C64XX_GPFPUD);
}

static void s3c_cs_configF13(int pin, int mode, int pull)
{
	cs_cfg(13, pull);
}

static void s3c_cs_configF14(int pin, int mode, int pull)
{
	cs_cfg(14, pull);
}

static void s3c_cs_configF15(int pin, int mode, int pull)
{
	cs_cfg(15, pull);
}

static void s3c_cs_set(int pin, int lvl)
{
	if(lvl == CS_HIGH)
	   s3c_gpio_setpin(pin, 1);
	else
	   s3c_gpio_setpin(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
	s3c_gpio_cfgpin(pin, mode);

	if(pull == CS_HIGH)
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
	else
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S3C64XX_GPC(3),
		.cs_mode      = S3C64XX_GPC_OUTPUT(3),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S3C64XX_GPF(13),
		.cs_mode      = S3C64XX_GPF_OUTPUT(13),
		.cs_set       = s3c_cs_setF13,
		.cs_config    = s3c_cs_configF13,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S3C64XX_GPC(7),
		.cs_mode      = S3C64XX_GPC_OUTPUT(7),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S3C64XX_GPF(14),
		.cs_mode      = S3C64XX_GPF_OUTPUT(14),
		.cs_set       = s3c_cs_setF14,
		.cs_config    = s3c_cs_configF14,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[2] = {	/* Slave-2 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S3C64XX_GPF(15),
		.cs_mode      = S3C64XX_GPF_OUTPUT(15),
		.cs_set       = s3c_cs_setF15,
		.cs_config    = s3c_cs_configF15,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

static struct spi_board_info s3c_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
	[0] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 1st Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 0,
	},
	[1] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 2nd Slave */
		.bus_num	= 0,
		.irq		= IRQ_SPI0,
		.chip_select	= 1,
	},
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	[2] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 1st Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 0,
	},
	[3] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 2nd Slave */
		.bus_num	= 1,
		.irq		= IRQ_SPI1,
		.chip_select	= 1,
	},
	[4] = {
		.modalias	 = "mmc_spi", /* MMC SPI */
		.mode		 = SPI_MODE_0 | SPI_CS_HIGH,	/* CPOL=0, CPHA=0 & CS is Active High */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 3rd Slave */
		.bus_num	= 1,
		.irq		= IRQ_SPI1,
		.chip_select	= 2,
	},
#endif
};

static struct s3c_uartcfg smdk6410_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon			= S3C_UCON_DEFAULT,
		.ulcon		= S3C_ULCON_DEFAULT,
		.ufcon		= S3C_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon			= S3C_UCON_DEFAULT,
		.ulcon		= S3C_ULCON_DEFAULT,
		.ufcon		= S3C_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon			= S3C_UCON_DEFAULT,
		.ulcon		= S3C_ULCON_DEFAULT,
		.ufcon		= S3C_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon			= S3C_UCON_DEFAULT,
		.ulcon		= S3C_ULCON_DEFAULT,
		.ufcon		= S3C_UFCON_DEFAULT,
	},
};

struct map_desc smdk6410_iodesc[] = {};

struct platform_device sec_device_backlight = {
	.name	= "smdk-backlight",
	.id		= -1,
};

struct platform_device sec_device_battery = {
	.name	= "smdk6410-battery",
	.id		= -1,
};

#if 0
static struct s3c6410_pmem_setting pmem_setting = {
 	.pmem_start = RESERVED_PMEM_START,
	.pmem_size = RESERVED_PMEM,
	.pmem_gpu1_start = GPU1_RESERVED_PMEM_START,
	.pmem_gpu1_size = RESERVED_PMEM_GPU1,
	.pmem_render_start = RENDER_RESERVED_PMEM_START,
	.pmem_render_size = RESERVED_PMEM_RENDER,
	.pmem_stream_start = STREAM_RESERVED_PMEM_START,
	.pmem_stream_size = RESERVED_PMEM_STREAM,
	.pmem_preview_start = PREVIEW_RESERVED_PMEM_START,
	.pmem_preview_size = RESERVED_PMEM_PREVIEW,
	.pmem_picture_start = PICTURE_RESERVED_PMEM_START,
	.pmem_picture_size = RESERVED_PMEM_PICTURE,
	.pmem_jpeg_start = JPEG_RESERVED_PMEM_START,
	.pmem_jpeg_size = RESERVED_PMEM_JPEG,
  .pmem_skia_start = SKIA_RESERVED_PMEM_START,
  .pmem_skia_size = RESERVED_PMEM_SKIA,
};


#endif

//&*&*&*EH1_20100209, introduce rfkill for wifi
#ifdef CONFIG_RFKILL_WIFI_POWER
static struct platform_device wifi_rfkill_device = {
	.name	= "wifi_rfkill",
	.id	= -1,
};
#endif
//&*&*&*EH2_20100209, introduce rfkill for wifi

static struct platform_device *smdk6410_devices[] __initdata = {
#ifdef CONFIG_S3C6410_SD_CH0
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C6410_SD_CH1
	&s3c_device_hsmmc1,
#endif
//&*&*&*EH1_20100113, enable BCM4319 Wi-Fi
#ifdef CONFIG_S3C6410_SD_CH2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_WATCHDOG
    &s3c_device_wdt,
#endif
//&*&*&*EH2_20100113, enable BCM4319 Wi-Fi
//&*&*&*EH1_20100209, introduce rfkill for wifi
#ifdef CONFIG_RFKILL_WIFI_POWER
      &wifi_rfkill_device,
#endif
//&*&*&*EH2_20100209, introduce rfkill for wifi
	&s3c_device_i2c0,
	&s3c_device_i2c1,
#if defined(CONFIG_SPI_CNTRLR_0)
	&s3c_device_spi0,
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	&s3c_device_spi1,
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C	
	&s3c_device_ts,
#endif
//	&s3c_device_smc911x,
//	&s3c_device_lcd,
#ifdef CONFIG_MTD_NAND
	&s3c_device_nand,
#endif

#ifdef CONFIG_MTD_ONENAND
    &s3c_device_onenand,
#endif

#ifdef CONFIG_KEYPAD_S3c
	&s3c_device_keypad,
#endif
	&s3c_device_usb,
	&s3c_device_usbgadget,
#ifdef CONFIG_S3C_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_RTC_DRV_S3C
	&s3c_device_rtc,
#endif
#ifdef CONFIG_VIDEO_G2D
	&s3c_device_g2d,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
#endif
#if CONFIG_VIDEO_CAM 
	&s3c_device_camif,
#endif
#ifdef CONFIG_VIDEO_MFC
	&s3c_device_mfc,
#endif
#ifdef CONFIG_VIDEO_G3D
	&s3c_device_g3d,
#endif
#ifdef CONFIG_VIDEO_ROTATOR
	&s3c_device_rotator,
#endif
#ifdef CONFIG_VIDEO_JPEG
	&s3c_device_jpeg,
#endif
	&s3c_device_vpp,
#ifdef CONFIG_S3C6410_TVOUT
	&s3c_device_tvenc,
	&s3c_device_tvscaler,
#endif
	//&sec_device_backlight,
	&sec_device_battery,
	&s3c_device_gpio_keys,//add by Lucifer
	//For led 
#ifdef CONFIG_LEDS_S3C64XX
	&platform_led1,		//William add
	&platform_led2,
#endif
	
};


static int viva_max8698_init(struct max8698 *max8698)
{
		unsigned long cfg;
			int h_cfg ;
	//set gpo0 IOC to low
	if( (h_cfg = get_epd_config()) == 8 )
	{
	}
	else
	{
	 gpio_set_value(S3C64XX_GPO(0), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPO(0),S3C64XX_GPO_OUTPUT(0));
	 s3c_gpio_setpull(S3C64XX_GPO(0), S3C_GPIO_PULL_DOWN);
	 
	 gpio_set_value(S3C64XX_GPO(8), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPO(8),S3C64XX_GPO_OUTPUT(0));
	 s3c_gpio_setpull(S3C64XX_GPO(8), S3C_GPIO_PULL_DOWN);
	}
	//max8698 set1_pin setting 0 GPK4
	 gpio_set_value(S3C64XX_GPK(4), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPK(4),S3C64XX_GPK_OUTPUT(4));
	 s3c_gpio_setpull(S3C64XX_GPK(4), S3C_GPIO_PULL_DOWN);
   
  //max8698 set2_pin setting 0 GPK3
	 gpio_set_value(S3C64XX_GPK(3), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPK(3),S3C64XX_GPK_OUTPUT(3));
	 s3c_gpio_setpull(S3C64XX_GPK(3), S3C_GPIO_PULL_DOWN);
   
  //max8698 set3_pin setting 0 GPK2
	 gpio_set_value(S3C64XX_GPK(2), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPK(2),S3C64XX_GPK_OUTPUT(2));
	 s3c_gpio_setpull(S3C64XX_GPK(2), S3C_GPIO_PULL_DOWN);
	
		mdelay(50);
	
		printk("++++++++gpmcon = 0x%x+++++++++\n",__raw_readl(S3C64XX_GPMCON));
		printk("++++++++gpmcon = 0x%x+++++++++\n",__raw_readl(S3C64XX_GPMPUD));
		
		s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(9), IRQ_TYPE_LEVEL_LOW);
//	set_irq_type(IRQ_EINT(0), IRQ_TYPE_EDGE_FALLING);
//		set_irq_type(IRQ_EINT(9),IRQ_TYPE_EDGE_BOTH);
/*
	blazer modify 20100906
*/
#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
		s3c_gpio_setpull(S3C64XX_GPN(3), S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(S3C64XX_GPN(3), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(3), IRQ_TYPE_LEVEL_LOW);
		//	set_irq_type(IRQ_EINT(0), IRQ_TYPE_EDGE_FALLING);
		//	set_irq_type(IRQ_EINT(9),IRQ_TYPE_EDGE_BOTH);
#endif

#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)||defined(CONFIG_HW_EP4_DVT)
		s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(9), IRQ_TYPE_LEVEL_LOW);
		//	set_irq_type(IRQ_EINT(0), IRQ_TYPE_EDGE_FALLING);
		//	set_irq_type(IRQ_EINT(9),IRQ_TYPE_EDGE_BOTH);
#endif


//gpk7 setting 0 , enable max8677 charge_en,
	 gpio_set_value(S3C64XX_GPK(7), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPK(7),S3C64XX_GPK_OUTPUT(7));
	 s3c_gpio_setpull(S3C64XX_GPK(7), S3C_GPIO_PULL_DOWN);
	 
	//gpk2 setting 1 
	 gpio_set_value(S3C64XX_GPE(2), 1);
	 s3c_gpio_cfgpin(S3C64XX_GPE(2),S3C64XX_GPE_OUTPUT(2));
	 s3c_gpio_setpull(S3C64XX_GPE(2), S3C_GPIO_PULL_UP);
		
//charge LED off
/*
	gpio_set_value(S3C64XX_GPK(0), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));	
	s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);
	*/
	gpio_set_value(S3C64XX_GPK(1), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_DOWN);
	
	//set gpn1 for powerkey to wakeup
		s3c_gpio_cfgpin(S3C64XX_GPN(1), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(1), IRQ_TYPE_EDGE_BOTH);
		s3c_gpio_setpull(S3C64XX_GPN(1), S3C_GPIO_PULL_UP);
	#if 0	 //William cancel
	//set gpn5 for ioc I2C_REQ	to wakeup
	s3c_gpio_cfgpin(S3C64XX_GPN(5), S3C_GPIO_SFN(2)); 
	s3c_gpio_setpull(S3C64XX_GPN(5), S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_FALLING);	
			mdelay(50);
	#endif
	//set gpl10 pageupkey to wakeup
		s3c_gpio_cfgpin(S3C64XX_GPL(10), S3C_GPIO_SFN(3));
		set_irq_type(IRQ_EINT(18), IRQ_TYPE_EDGE_BOTH);
		s3c_gpio_setpull(S3C64XX_GPL(10), S3C_GPIO_PULL_UP);
		
	printk("+++++++S3C64XX_GPLCON1:0x%x,S3C64XX_GPL10=%d,\n",__raw_readl(S3C64XX_GPLCON1),gpio_get_value(S3C64XX_GPL(10)));
	mdelay(50);

	return 0;
}

static int max17043_irq_init(struct max17043_device_info *max17043)
{
	printk("&&&&&&&&&&&enter max17043_irq_init^^^^^^^^^^\n");
	//set gpm3 for i2c irq handle
		s3c_gpio_cfgpin(S3C64XX_GPM(3), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S3C64XX_GPM(3), S3C_GPIO_PULL_NONE);
		set_irq_type(IRQ_EINT(26),IRQ_TYPE_EDGE_BOTH);
		mdelay(50);
		
		//set gpn0 for PMIC_FAULT handle
		s3c_gpio_cfgpin(S3C64XX_GPN(0), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(0), IRQ_TYPE_EDGE_BOTH);
		s3c_gpio_setpull(S3C64XX_GPN(0), S3C_GPIO_PULL_UP);
			mdelay(50);
/*
		blazer modify 20100906
*/
		
#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
		//set gpn9 for low_battery warnning
		s3c_gpio_cfgpin(S3C64XX_GPN(3), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(3), IRQ_TYPE_LEVEL_LOW);
		s3c_gpio_setpull(S3C64XX_GPN(3), S3C_GPIO_PULL_UP);
			mdelay(50);
#endif

#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)||defined(CONFIG_HW_EP4_DVT)
		//set gpn9 for low_battery warnning
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(2));
		set_irq_type(IRQ_EINT(9), IRQ_TYPE_LEVEL_LOW);
		s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_UP);
			mdelay(50);
#endif

			
		//set gpl13 for charge_sta(sleep wakeup when charge full)
		s3c_gpio_cfgpin(S3C64XX_GPL(13), S3C_GPIO_SFN(3));
	//	set_irq_type(IRQ_EINT(21), IRQ_TYPE_LEVEL_HIGH);
		set_irq_type(IRQ_EINT(21), IRQ_TYPE_EDGE_BOTH);
	//	s3c_gpio_setpull(S3C64XX_GPL(13), S3C_GPIO_PULL_UP);
		s3c_gpio_setpull(S3C64XX_GPL(13), S3C_GPIO_PULL_NONE);
			mdelay(50);
		printk("@@@@@@@@@@@@@@@gpl13 = 0x%x\n",gpio_get_value(S3C64XX_GPL(13)));		//William 20111106
		return 0;
}

static struct max8698_platform_data __initdata viva_max8698_pdata = {
	.init = viva_max8698_init,
};

static struct max17043_platform_data __initdata max17043_pdata = {
	.init = max17043_irq_init,
	.pmic_fault_irq = IRQ_EINT(0),
/*
	blazer modify 20100906
*/
	
#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)	
	.low_battery_irq = IRQ_EINT(3),
#endif

#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)||defined(CONFIG_HW_EP4_DVT)	
	.low_battery_irq = IRQ_EINT(9),
#endif
	.charge_state_irq = IRQ_EINT(21),
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	
	{ 
		I2C_BOARD_INFO("max8698",0x66),
		.platform_data=&viva_max8698_pdata,	
  //  .irq = IRQ_EINT(26),//IOMUX_TO_IRQ(MX31_PIN_GPIO1_3),
	},  //Added by Hill 2010-04-08
	/*
		blazer modify 2010.07.20
	*/
	{
      I2C_BOARD_INFO("adxl345", 0x53),
      .irq = IRQ_EINT(7),
  },
  {
		I2C_BOARD_INFO("ioc", 0x33),
		.irq = IRQ_EINT(5),
	//	.platform_data = &pic16_pdata,
	},
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	
		{ I2C_BOARD_INFO("max17043", 0x36), 
		.platform_data= &max17043_pdata ,
		.irq = IRQ_EINT(26),//IOMUX_TO_IRQ(MX31_PIN_GPIO1_3),
	},
};

#ifdef CONFIG_TOUCHSCREEN_S3C	
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};
#endif
static struct s3c_adc_mach_info s3c_adc_platform __initdata= {
	/* Support 12-bit resolution */
	.delay	= 	0xff,
	.presc 	= 	49,
	.resolution = 	12,
};

static void __init smdk6410_map_io(void)
{
	s3c_device_nand.name = "s3c6410-nand";

	s3c64xx_init_io(smdk6410_iodesc, ARRAY_SIZE(smdk6410_iodesc));
	s3c64xx_gpiolib_init();
	s3c_init_clocks(XTAL_FREQ);
	s3c_init_uarts(smdk6410_uartcfgs, ARRAY_SIZE(smdk6410_uartcfgs));
}

static void __init smdk6410_smc911x_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S3C64XX_SROM_BW);
	tmp &= ~(S3C64XX_SROM_BW_WAIT_ENABLE1_MASK | S3C64XX_SROM_BW_WAIT_ENABLE1_MASK |
		S3C64XX_SROM_BW_DATA_WIDTH1_MASK);
	tmp |= S3C64XX_SROM_BW_BYTE_ENABLE1_ENABLE | S3C64XX_SROM_BW_WAIT_ENABLE1_ENABLE |
		S3C64XX_SROM_BW_DATA_WIDTH1_16BIT;

	__raw_writel(tmp, S3C64XX_SROM_BW);

	__raw_writel(S3C64XX_SROM_BCn_TACS(0) | S3C64XX_SROM_BCn_TCOS(4) |
			S3C64XX_SROM_BCn_TACC(13) | S3C64XX_SROM_BCn_TCOH(1) |
			S3C64XX_SROM_BCn_TCAH(4) | S3C64XX_SROM_BCn_TACP(6) |
			S3C64XX_SROM_BCn_PMC_NORMAL, S3C64XX_SROM_BC1);
}

static void __init smdk6410_fixup (struct machine_desc *desc, struct tag *tags,
	      char **cmdline, struct meminfo *mi)
{
	/*
	 * Bank start addresses are not present in the information
	 * passed in from the boot loader.  We could potentially
	 * detect them, but instead we hard-code them.
	 */
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = PHYS_UNRESERVED_SIZE;
	mi->bank[0].node = 0;
}

static void smdk6410_set_qos(void)
{
	u32 reg;

	/* AXI sfr */
	reg = (u32) ioremap((unsigned long) 0x7e003000, SZ_4K);

	/* QoS override: FIMD min. latency */
	writel(0x2, S3C_VA_SYS + 0x128);

	/* AXI QoS */
	writel(0x7, reg + 0x460);	/* (8 - MFC ch.) */
	writel(0x7ff7, reg + 0x464);

	/* Bus cacheable */
	writel(0x8ff, S3C_VA_SYS + 0x838);
}
//&*&*&*HC1_20081223, Add power off function
    /* Henry@10-02: Fix bug of "After long key press => power off,  the system can't wake up anymore" */
void turn_LDO9_off(void);
void kernel_restart(char *cmd);
static void ep_power_off(void)
{
   unsigned int tmp,i;

/*
	tmp = readw(S3C64XX_GPECON);
    tmp &= ~0xF<<8;
	tmp |= 0x1<<8;
    writew(tmp, S3C64XX_GPECON);

	tmp = readw(S3C64XX_GPEDAT);
    tmp &= ~0x1<<2;
	tmp |= 0x0<<2;
    writew(tmp, S3C64XX_GPEDAT);
*/ 
    /* Henry@10-02: Fix bug of "After long key press => power off,  the system can't wake up anymore" */
  tmp=0;
  for (i=0; i < 3; i++)
  {
   while (!gpio_get_value(S3C64XX_GPN(1)))	
   	{
	msleep(1);
	tmp++;
	if (tmp > 10000)
		break;
   	}
   msleep(1);
  }
/*  
  if(gpio_get_value(S3C64XX_GPM(3)) == 0)		//No usb plug in	
	turn_LDO9_off();	 

*/
    /* Henry@10-00: Guarantee poweroff safety */
if ((gpio_get_value(S3C64XX_GPM(3)) == 1) || (gpio_get_value(S3C64XX_GPN(1)) == 0))	// usb plug in or PWR_KEY
			kernel_restart(NULL);
   else
   	{
   gpio_set_value(S3C64XX_GPL(2), 0);
	 s3c_gpio_cfgpin(S3C64XX_GPL(2),S3C64XX_GPL_OUTPUT(2));
	 s3c_gpio_setpull(S3C64XX_GPL(2), S3C_GPIO_PULL_DOWN);	
   	}  	
}
//&*&*&*HC2_20081223, Add power off function

static void __init smdk6410_machine_init(void)
{
 	unsigned char val;
 	int h_cfg ;
        printk("PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\n");
      
	s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;
	s3c_device_onenand.dev.platform_data = &s3c_onenand_data;

	smdk6410_smc911x_set();
	if( (h_cfg = get_epd_config()) == 8 )
	{
		mdelay(1) ;
	}
	else
	{
		ioc_poweron(); //William add 20101027
		ioc_reset();
    msleep(10);
  }
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
#ifdef CONFIG_TOUCHSCREEN_S3C	
	s3c_ts_set_platdata(&s3c_ts_platform);   //added for touch  20100510
#endif
	s3c_adc_set_platdata(&s3c_adc_platform);
	
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

#if defined(CONFIG_SPI_CNTRLR_0)
	s3c_spi_set_slaves(BUSNUM(0), ARRAY_SIZE(s3c_slv_pdata_0), s3c_slv_pdata_0);
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	s3c_spi_set_slaves(BUSNUM(1), ARRAY_SIZE(s3c_slv_pdata_1), s3c_slv_pdata_1);
#endif
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));

	platform_add_devices(smdk6410_devices, ARRAY_SIZE(smdk6410_devices));
	//s3c6410_add_mem_devices (&pmem_setting);

	s3c6410_pm_init();

	smdk6410_set_qos();
//&*&*&*HC1_20081223, Add power off function
	pm_power_off = ep_power_off;
//&*&*&*HC2_20081223, Add power off function
#if 0
	     val=__raw_readl(S3C64XX_GPPCON);
        val&=~(0x3<<2*7);
        __raw_writel(val|0x2<<2*7,S3C64XX_GPPCON);
        val=__raw_readl(S3C64XX_GPPCON);
        val&=~(0x3<<2*0);
        __raw_writel(val|0x2<<2*0,S3C64XX_GPPCON);
        val=__raw_readl(S3C64XX_MEM0CONSTOP)|0x3<<26;
        __raw_writel(val,S3C64XX_MEM0CONSTOP);
#endif
}

MACHINE_START(SMDK6410, "SMDK6410")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,
	.fixup		= smdk6410_fixup,
	.init_irq	= s3c6410_init_irq,
	.map_io		= smdk6410_map_io,
	.init_machine	= smdk6410_machine_init,
#ifndef CONFIG_HIGH_RES_TIMERS
	.timer		= &s3c64xx_timer,
#else
	.timer		= &sec_timer,
#endif /* CONFIG_HIGH_RES_TIMERS */

MACHINE_END

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{

	writel(readl(S3C_OTHERS)|S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
	writel(0x0, S3C_USBOTG_PHYPWR);		/* Power up */
    writel(/*OTGH_PHY_CLK_VALUE*/2, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);

	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(readl(S3C_OTHERS)&~S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) 
{
	struct clk *otg_clk;

 switch (S3C_USB_CLKSRC) {
	case 0: /* epll clk */
	
		writel((readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK)
			|S3C_CLKSRC_EPLL_CLKSEL|S3C_CLKSRC_UHOST_EPLL,
			S3C_CLK_SRC);

			udelay(50);
		/* USB host colock divider ratio is 2 */
		writel((readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK)
			|(10<<20), S3C_CLK_DIV1);
		break;
	case 1: /* oscillator 48M clk */
		otg_clk = clk_get(NULL, "otg");
		clk_enable(otg_clk);
		writel(readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK, S3C_CLK_SRC);
		otg_phy_init();

		/* USB host colock divider ratio is 1 */
		writel(readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK, S3C_CLK_DIV1);
		break;
	default:
		printk(KERN_INFO "Unknown USB Host Clock Source\n");
		BUG();
		break;
	}

	writel(readl(S3C_HCLK_GATE)|S3C_CLKCON_HCLK_UHOST|S3C_CLKCON_HCLK_SECUR,
		S3C_HCLK_GATE);
	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_UHOST, S3C_SCLK_GATE);

}

EXPORT_SYMBOL(usb_host_clk_en);
#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C_RTCCON) & (S3C_RTCCON_TICEN | S3C_RTCCON_RTCEN );
        writew(tmp, base + S3C_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C_RTCCON);
		writew(tmp & ~ (S3C_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C_RTCCON) & S3C_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp|S3C_RTCCON_RTCEN, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp& ~S3C_RTCCON_CNTSEL, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp & ~S3C_RTCCON_CLKRST, base+S3C_RTCCON);
		}
	}
}
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C64XX_GPK(8 + rows);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S3C64XX_GPK(8); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S3C64XX_GPL(0 + columns);

	/* Set all the necessary GPL pins to special-function 0 */
	for (gpio = S3C64XX_GPL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
//		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

#define inform3_val (*(volatile unsigned *)(S3C_INFORM3))

//&*&*&*HC1_20100119, modify SLEEP GPIO configuration
//&*&*&*HC1_20091229, Add for setting GPIOs in order to reduce current leakage in Sleep state 
void s3c_config_sleep_gpio(void)
{	
	unsigned long gpioslp;
    u32 cfg;
//=======================================================================

	printk("s3c inform 1 = 0x%x\n", inform3_val);
	__raw_writel(__raw_readl(S3C_INFORM1)|0x80000000, S3C_INFORM1);
	
	s3c_gpio_slp_setpull_updown(S3C64XX_GPE(2), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPE(2), 1);	
	
	// set I2C0 as input in sleep mode	
	// I2C_SCL(GPB5),  I2C_SDA(GPB6)		
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(5), 1);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(6), 1);
	
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(5), 2);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(6), 2);	
	
	// set I2C1 as input in sleep mode	
	// I2C_SCL1(GPB2),  I2C_SDA1(GPB3)		
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(2), 1);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(3), 1);
	
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(2), 2);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(3), 2);	
//=======================================================================

// set 2SD_CLK, 2SD_CMD, 2SD_D1 ~ 2SD_D4 to keep previous state in Sleep	 
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(0), 2);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(1), 2);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(2), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(3), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(4), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(5), 2);	

	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(0), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(1), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(2), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(3), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(4), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(5), 0);	

/*************************************************************************
 * set gpio for wifi sleep mode 
*************************************************************************/

	 s3c_gpio_cfgpin(S3C64XX_GPK(9),S3C_GPIO_INPUT);
	 s3c_gpio_cfgpin(S3C64XX_GPK(13),S3C_GPIO_INPUT);
	 s3c_gpio_cfgpin(S3C64XX_GPK(14),S3C_GPIO_INPUT);
	 s3c_gpio_cfgpin(S3C64XX_GPM(5),S3C_GPIO_INPUT);
	 	s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
		s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);
/*
	// set I2S input,pull up/down disable
*/	 
#if 1
     //set gpa4-7 in sleep mode
  s3c_gpio_slp_cfgpin(S3C64XX_GPA(0), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(1), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(2), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(3), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(4), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(5), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(6), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPA(7), 2);
   	
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(0), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(1), 2);
	
	// set I2C1 as input in sleep mode	
	// I2C_SCL1(GPB2),  I2C_SDA1(GPB3)		
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(2), 1);
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(3), 1);
	
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(2), 2);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(3), 2);	
	
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(5), 1);
	s3c_gpio_slp_cfgpin(S3C64XX_GPB(6), 1);
	
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(5), 2);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPB(6), 2);
	
	s3c_gpio_slp_cfgpin(S3C64XX_GPC(4), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPC(5), 2);


	s3c_gpio_slp_cfgpin(S3C64XX_GPC(6), 1);
	s3c_gpio_slp_cfgpin(S3C64XX_GPC(7), 1);

	s3c_gpio_slp_setpull_updown(S3C64XX_GPC(6), 2);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPC(7), 2);

	//=======================================================================


	// set I2S input,pull up/down disable
	s3c_gpio_slp_cfgpin(S3C64XX_GPD(0), 2);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPD(1), 2);	
	s3c_gpio_slp_cfgpin(S3C64XX_GPD(2), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPD(3), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPD(4), 2);

	s3c_gpio_slp_setpull_updown(S3C64XX_GPD(0), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPD(1), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPD(2), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPD(3), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPD(4), 0);

//=======================================================================

  __raw_writel(0xaaaaa,S3C64XX_GPECONSLP);
  __raw_writel(0,S3C64XX_GPEPUDSLP);
  
 //======================================================================
 //GPF didn't use
//====================================================================== 

	s3c_gpio_slp_cfgpin(S3C64XX_GPH(0), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(1), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(2), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(3), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(4), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(5), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(6), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(7), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(8), 2);
	s3c_gpio_slp_cfgpin(S3C64XX_GPH(9), 2);

	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(0), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(1), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(2), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(3), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(4), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(5), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(6), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(7), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(8), 0);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPH(9), 0);

#endif
	
  //Set GPF0~15 GPIO in Sleep. Input, Pull Up/Down disable 
  //GPF0--GPF4 as key
	__raw_writel(0xaaaaaaaa,  S3C64XX_GPFCONSLP);	
	__raw_writel(0x0,  S3C64XX_GPFPUDSLP);

  //Set GPO0~15 GPIO in Sleep. Input, Pull Up/Down disable
  //GPo0~XM0CSn2,GPO2~HDC,GPO3~HIRQ,GPO4~HRDY,GPO6~FWE as "touch Flash ROM Compulsory Rewrite "
  //GPO8~IOCRST  
     s3c_gpio_cfgpin(S3C64XX_GPO(12), 1);
     s3c_gpio_cfgpin(S3C64XX_GPO(13), 1);
//__raw_writel(0xaaaaaaa8,  S3C64XX_GPOCONSLP);
	__raw_writel(0xaaa8aaa8,  S3C64XX_GPOCONSLP);//Jack added to solve SC8 on 2011/11/5
	__raw_writel(0x01,  S3C64XX_GPOPUDSLP);
	
//		s3c_gpio_slp_cfgpin(S3C64XX_GPF(12), 1);	
//		s3c_gpio_slp_setpull_updown(S3C64XX_GPF(12), 2);
	
//	//Jack added ON 2010/09/16 output low, pull down enable
//	s3c_gpio_slp_cfgpin(S3C64XX_GPO(8), 0);	
//	s3c_gpio_slp_cfgpin(S3C64XX_GPO(0), 0);
//	
//	s3c_gpio_slp_setpull_updown(S3C64XX_GPO(8), 1);
//	s3c_gpio_slp_setpull_updown(S3C64XX_GPO(0), 1);	

  //Set GPP0~15 GPIO in Sleep. Input, Pull Up/Down disable
	__raw_writel(0xaaaaaaaa,  S3C64XX_GPPCONSLP);
	__raw_writel(0x0,  S3C64XX_GPPPUDSLP);
  //********************************************************

		s3c_gpio_cfgpin(S3C64XX_GPK(5), S3C64XX_GPK_OUTPUT(5));
		s3c_gpio_setpull(S3C64XX_GPK(5), S3C_GPIO_PULL_UP);
		
		s3c_gpio_cfgpin(S3C64XX_GPL(3), S3C64XX_GPL_OUTPUT(3));
		s3c_gpio_setpull(S3C64XX_GPL(3), S3C_GPIO_PULL_UP);	
		gpio_set_value(S3C64XX_GPL(3), 1);
		
#if defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)

		s3c_gpio_slp_cfgpin(S3C64XX_GPJ(5), 3);
		s3c_gpio_slp_cfgpin(S3C64XX_GPJ(6), 3);
		s3c_gpio_slp_cfgpin(S3C64XX_GPJ(7), 3);
		
		s3c_gpio_slp_setpull_updown(S3C64XX_GPJ(5), 1);
		s3c_gpio_slp_setpull_updown(S3C64XX_GPJ(6), 1);
		s3c_gpio_slp_setpull_updown(S3C64XX_GPJ(7), 1);
		
#endif

    //power off touch
    #if defined (CONFIG_HW_EP3_DVT) || defined (CONFIG_HW_EP1_DVT)
        hanwang_touch_power_off();
    #endif

/*  blazer cancels 2010.08.24
  //set GPN9 EPD_PWR_EN off
		gpio_set_value(S3C64XX_GPN(9), 0);		
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C64XX_GPN_OUTPUT(9));	
		s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_DOWN);

  //set GPN6 WIFI_PWR_EN off
		gpio_set_value(S3C64XX_GPN(6), 0);		
		s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C64XX_GPN_OUTPUT(6));	
		s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_DOWN);
*/	
	//set MEM1_CSn, MEM1_WEn, MEM1_RASn, MEM1_CASn as high in Sleep
//	gpioslp = __raw_readl(S3C64XX_MEM1CONSLP);
//	__raw_writel(gpioslp |(0x01<<12) |(0x01<<14) |(0x01<<16) |(0x01<<18),  S3C64XX_MEM1CONSLP);
	
	gpioslp = __raw_readl(S3C64XX_MEM0CONSLP0);
	gpioslp &= (0x3<<12) | (0x3<<14) | (0x3<<22);
	__raw_writel(gpioslp |(0x01<<12) |(0x01<<14) |(0x01<<22),  S3C64XX_MEM0CONSLP0);	
  
  //MEM0_OEn , MEM0_FWEn, MEM0_FREn, MEM0_RP_RnB
	gpioslp = __raw_readl(S3C64XX_MEM0CONSLP1);
	gpioslp &= (0x3<<0) | (0x3<<6) | (0x3<<8) | (0x3<<12);
	__raw_writel(gpioslp |(0x01<<0) |(0x01<<6) |(0x01<<8) |(0x01<<12),  S3C64XX_MEM0CONSLP1);	
//=========================================================================
//
//	// set Charge_Fail(GPN0) as output high	
//	s3c_gpio_cfgpin(S3C64XX_GPN(0), S3C64XX_GPN_INPUT(0));
//	s3c_gpio_setpull(S3C64XX_GPN(0), S3C_GPIO_PULL_NONE);

//  //Set Codec_En to Output High	
//	s3c_gpio_slp_cfgpin(S3C64XX_GPF(12), 1);		
//	s3c_gpio_slp_setpull_updown(S3C64XX_GPF(12), 2);		
//=========================================================================
//&*&*&*HC2_20100205, modify sleep configuration for DVT 

/*Blazer cancels 2010.09.02

  //Set EXTT4 GPIO config (GPN4) 設置GPN4為中斷模式，並且全能 PULL_UP
  //GPN4　 設置後 Home Key 才能中斷喚醒板子。

   cfg = __raw_readl(S3C64XX_GPNCON);
   cfg  = (cfg & ~(0x3<<8));
   __raw_writel(cfg |(0x2<<8), S3C64XX_GPNCON);
   set_irq_type(IRQ_EINT(4), IRQ_TYPE_EDGE_BOTH);
   s3c_gpio_setpull(S3C64XX_GPN(4), S3C_GPIO_PULL_UP);		
   printk(KERN_ALERT "EINT4...\n");
   udelay(50);
*/
}
EXPORT_SYMBOL(s3c_config_sleep_gpio);

void s3c_config_wakeup_gpio(void)
{
    
    u32 gpiocfg;    

    //GPK1,2,5;    1,2 OUTPUT 0;    5 output 1
    gpiocfg=__raw_readl(S3C64XX_GPKCON);
    gpiocfg &= (0xf<<4)|(0xf<<8)|(0xf<<20);
		__raw_writel(gpiocfg|(0x1<<4)|(0x1<<8)|(0x1<<20), S3C64XX_GPKCON);
				
    gpiocfg=__raw_readl(S3C64XX_GPKDAT);
    gpiocfg &= (0x1<<1)|(0x1<<2)|(0x1<5);
		__raw_writel(gpiocfg|(0x0<<1)|(0x0<<2)|(0x1<<5), S3C64XX_GPKDAT);
		
	#if 0		//William cancel	 
		 //Jack added for ioc I2C_REQ
		 s3c_gpio_cfgpin(S3C64XX_GPN(5), S3C_GPIO_SFN(2));  
	s3c_gpio_setpull(S3C64XX_GPN(5), S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_FALLING);
	#endif
#if defined (CONFIG_HW_EP3_DVT) || defined (CONFIG_HW_EP1_DVT)
    if (gpio_get_value(S3C64XX_GPN(4)) == 1)
        hanwang_touch_power_off();
    else
        hanwang_touch_power_on();
#endif

/*	blazer cancels 2010.08.24	
    //set GPN9 EPD_PWR_EN on
		gpio_set_value(S3C64XX_GPN(9), 0);
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C64XX_GPN_OUTPUT(9));
		s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_DOWN);
		
    //set GPN6 WIFI_PWR_EN on
		gpio_set_value(S3C64XX_GPN(6), 1);
		s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C64XX_GPN_OUTPUT(6));
		s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_DOWN);	
*/	
}
EXPORT_SYMBOL(s3c_config_wakeup_gpio);
//&*&*&*HC2_20091229, Add for setting GPIOs in order to reduce current leakage in Sleep state 

//&*&*&*HC1_20091208, for suspend/resume function
void s3c_config_wakeup_source(void)
{
//	if (WAKEUP_TD_MODEM != 0)
//	{
//		/*WAKEUP_AP*/
//#ifdef CONFIG_EX3_HARDWARE_DVT		
//		/*(GPL8 - EINT16)*/
//		s3c_gpio_cfgpin(S3C64XX_GPL(8), S3C_GPIO_SFN(3));
//		set_irq_type(IRQ_EINT(16), IRQ_TYPE_EDGE_BOTH);
//		__raw_writel(1UL << (IRQ_EINT(16) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
//		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(16) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
//#else
//		/*(GPL14 - EINT22)*/
//		s3c_gpio_cfgpin(S3C64XX_GPL(14), S3C_GPIO_SFN(3));
//		set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_BOTH);
//		__raw_writel(1UL << (IRQ_EINT(22) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
//		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(22) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
//#endif
//	}	
//
//	__raw_writel((0x0fffffff &~ VIVA_WAKEUP_SOURCE), S3C_EINT_MASK);
//
//	// disable keyboard interrupt
//	__raw_writel(__raw_readl(S3C_PWR_CFG) | (0x1<<8), S3C_PWR_CFG);
//#endif
////&*&*&*HC2_20100201, Modify wakeup source
//#endif
/*
	Wake_up source   Blazer 
*/
//config the usb insertion/removal wakeup   blazer added on 2010.08.26
		//gpm3->USB_IN wakeup
        //__raw_writel(__raw_readl(S3C_EINT_MASK) & (~(0x1<<26)), S3C_EINT_MASK);
		/*
		  * set INT20(GPL12==>Key_Back),INT21(GPL13==>CHARGE_STA) as BOTHEDGE
		*/
		__raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 20)) |
                (S3C64XX_EXTINT_BOTHEDGE << 20), S3C64XX_EINT0CON1);
        /*
		 * Enable INT26(GPM3==> USB5V_IN) interrupt
		*/        
		__raw_writel(1UL << (IRQ_EINT(26) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(26) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		
#if defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_DVT)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)

//config the SD card insertion/removal wakeup   blazer added on 2010.09.01
		//gpl14->SD_DET wakeup
		__raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(22) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(22) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
#endif

#if defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
//config G-Sensor wakeup   blazer added on 2010.09.04
		//gpn7->G_INT1 wakeup
       // __raw_writel(__raw_readl(S3C_PWR_CFG) & (~(0x1<<15)), S3C_PWR_CFG);		//MMC1_WAKEUP_MASK

		__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(7) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(7) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
#endif

#if defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
//config ioc I2C_REQ wakeup   Jack added on 2010/10/17
		printk("~~~~~~~~~~~~~config ioc I2C_REQ wakeup~~~~~~~~~~~~~~\n") ;
		/*
		 * Set INT4(GPN4==>PEN_OUT), INT5(GPN5==>I2C_REQ) as FALLEDGE type
		*/
		__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 8)) |
                (S3C64XX_EXTINT_FALLEDGE << 8), S3C64XX_EINT0CON0);
        /*
		 * Enable INT5 interrupt
		*/        
		__raw_writel(1UL << (IRQ_EINT(5) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(5) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
#endif

//config the keypad wakeup     blazer added on 2010.08.25
        __raw_writel(__raw_readl(S3C_PWR_CFG) & (~(0x1<<8)), S3C_PWR_CFG);		//KEY_WAKEUP_MASK
     
		udelay(50);
		
		//gpn1->power key  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 0)) |
                (S3C64XX_EXTINT_BOTHEDGE << 0), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(1) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(1) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		

		//gpl8->menu key  wakeup 
		printk("~~~~~~~~~~~~~gpl8->menu key  wakeup~~~~~~~~~~~~~\n") ;
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 0)) |
				(S3C64XX_EXTINT_BOTHEDGE << 0), S3C64XX_EINT0CON1);
						
		__raw_writel(1UL << (IRQ_EINT(16) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(16) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpl9->home key  wakeup 
		printk("~~~~~~~~~~~~~gpl9->home key  wakeup~~~~~~~~~~~~~\n") ;
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 0)) |
				(S3C64XX_EXTINT_BOTHEDGE << 0), S3C64XX_EINT0CON1);
						
		__raw_writel(1UL << (IRQ_EINT(17) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(17) - IRQ_EINT(0))), S3C64XX_EINT0MASK);


		//gpl10->page up wakeup 
		printk("~~~~~~~~~~~~~gpl10->page up wakeup~~~~~~~~~~~~~\n") ;
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 4)) |
                (S3C64XX_EXTINT_BOTHEDGE << 4), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(18) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(18) - IRQ_EINT(0))), S3C64XX_EINT0MASK);


		//gpl11->page down wakeup 
		printk("~~~~~~~~~~~~~gpl11->page down wakeup~~~~~~~~~~~~~\n") ;
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 4)) |
                (S3C64XX_EXTINT_BOTHEDGE << 4), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(19) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(19) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpl12->back key wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 8)) |
                (S3C64XX_EXTINT_BOTHEDGE << 8), S3C64XX_EINT0CON1);
       printk("~~~~~~~~~~~~~gpl12->back key wakeup~~~~~~~~~~~~~\n") ;
                
		__raw_writel(1UL << (IRQ_EINT(20) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(20) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpm4->pen det wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 20)) |
                (S3C64XX_EXTINT_BOTHEDGE << 20), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(27) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(27) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		

		//gpn4->pen out wakeup 
 		__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 8)) |
				(S3C64XX_EXTINT_BOTHEDGE << 8), S3C64XX_EINT0CON0);
		
		__raw_writel(1UL << (IRQ_EINT(4) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(4) - IRQ_EINT(0))), S3C64XX_EINT0MASK);


#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)

		//gpn12->K5 wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 24)) |
                (S3C64XX_EXTINT_BOTHEDGE << 24), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(12) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(12) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpn6->K6  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(6) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(6) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpn8->K7 wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 16)) |
                (S3C64XX_EXTINT_BOTHEDGE << 16), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(8) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(8) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpn9->K8  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 16)) |
                (S3C64XX_EXTINT_BOTHEDGE << 16), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(9) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(9) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpn10->K9 wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 20)) |
                (S3C64XX_EXTINT_BOTHEDGE << 20), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(10) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(10) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		

		//config low battery wakeup   blazer added on 2010.08.26
		//gpn3->low battery wake up
		//__raw_writel((__raw_readl(S3C_PWR_CFG) | (0x1<<3) )| (0x1<<7), S3C_PWR_CFG);

		__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 4)) |
                (S3C64XX_EXTINT_BOTHEDGE << 4), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(3) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(3) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		if(key_lock==1)
		{
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<3)&~(1<<22)&~(1<<26)), S3C_EINT_MASK);
		}
		else
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<3)&~(1<<4)&~(1<<5)&~(1<<6)&~(1<<7)&~(1<<8)&~(1<<9)&~(1<<10)&~(1<<12)&~(1<<16)&~(1<<17)&~(1<<18)&~(1<<19)&~(1<<20)&~(1<<22)&~(1<<26)&~(1<<27)), S3C_EINT_MASK);
    
#endif

#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)||defined(CONFIG_HW_EP4_DVT)

		//gpn12->5way_key wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 24)) |
				(S3C64XX_EXTINT_BOTHEDGE << 24), S3C64XX_EINT0CON0);
		
		__raw_writel(1UL << (IRQ_EINT(12) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(12) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

//config low battery wakeup   blazer added on 2010.08.26
		//gpn9->low battery wake up
		__raw_writel((__raw_readl(S3C_PWR_CFG) | (0x1<<3) )| (0x1<<7), S3C_PWR_CFG);

		__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 16)) |
                (S3C64XX_EXTINT_BOTHEDGE << 16), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(9) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(9) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
#endif
		
#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)

		//gpn6->volume up  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(6) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(6) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		
		//gpn7->volume down  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(7) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(7) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		
		//gpm2->size up  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 16)) |
                (S3C64XX_EXTINT_BOTHEDGE << 16), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(25) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(25) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpl14->size down  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(22) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(22) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		

		if(key_lock==1)
		{
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<9)&~(1<<26)), S3C_EINT_MASK);
		}
		else
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<4)&~(1<<5)&~(1<<6)&~(1<<7)&~(1<<9)&~(1<<12)&~(1<<16)&~(1<<17)&~(1<<18)&~(1<<19)&~(1<<20)&~(1<<22)&~(1<<25)&~(1<<26)&~(1<<27)), S3C_EINT_MASK);		
#endif		
#if defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)

		//gpn6->volume up  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 12)) |
                (S3C64XX_EXTINT_BOTHEDGE << 12), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(6) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(6) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		
		//gpn3->volume down  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 4)) |
                (S3C64XX_EXTINT_BOTHEDGE << 4), S3C64XX_EINT0CON0);
                
		__raw_writel(1UL << (IRQ_EINT(3) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(3) - IRQ_EINT(0))), S3C64XX_EINT0MASK);
		
		//gpm2->size up  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 16)) |
                (S3C64XX_EXTINT_BOTHEDGE << 16), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(25) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(25) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		//gpl10->size down  wakeup 
		 __raw_writel((__raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 20)) |
                (S3C64XX_EXTINT_BOTHEDGE << 20), S3C64XX_EINT0CON1);
                
		__raw_writel(1UL << (IRQ_EINT(10) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
		__raw_writel(__raw_readl(S3C64XX_EINT0MASK)&~(1UL << (IRQ_EINT(10) - IRQ_EINT(0))), S3C64XX_EINT0MASK);

		if(key_lock==1)
		{
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<9)&~(1<<22)&~(1<<26)), S3C_EINT_MASK);
		}
		else
			__raw_writel((0x0fffffff&~(1<<1)&~(1<<3)&~(1<<4)&~(1<<5)&~(1<<6)&~(1<<7)&~(1<<9)&~(1<<10)&~(1<<12)&~(1<<16)&~(1<<17)&~(1<<18)&~(1<<19)&~(1<<20)&~(1<<22)&~(1<<25)&~(1<<26)&~(1<<27)), S3C_EINT_MASK);		
    
#endif			
}
EXPORT_SYMBOL(s3c_config_wakeup_source);
//&*&*&*HC2_20091208, for suspend/resume function
