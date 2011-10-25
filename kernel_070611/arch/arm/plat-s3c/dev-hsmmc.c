/* linux/arch/arm/plat-s3c/dev-hsmmc.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for hsmmc devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>

#include <mach/map.h>
#include <plat/sdhci.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>
#include <plat/gpio-bank-e.h>
#include <plat/regs-gpio.h>
#include <asm/io.h>

#include <plat/regs-clock.h>

static struct resource s3c_hsmmc_resource[] = {
	[0] = {
		.start = S3C_PA_HSMMC0,
		.end   = S3C_PA_HSMMC0 + S3C_SZ_HSMMC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSMMC0,
		.end   = IRQ_HSMMC0,
		.flags = IORESOURCE_IRQ,
	},
/*	[2] = {
		.start = IRQ_EINT(8),
		.end   = IRQ_EINT(8),
		.flags = IORESOURCE_IRQ,
	}
*/
};
/******************************************************
Config_ch0_extirq used to config the card detect interrupt 
add by andy cheng 2010/3/22
*******************************************************/
#if defined(CONFIG_CH0_EXTIRQ)
#define CH0_CARD_IRQ IRQ_EINT(8)
unsigned int s3c6410_setup_sdhci0_cfg_ext_cd(void)
{
    unsigned int ret;
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_UP);
	writel((readl(S3C64XX_EINT0CON0) & ~(0x7 << 16)) | (0x7 << 16),
		S3C64XX_EINT0CON0);
		printk(KERN_ALERT"gpncon=%08x,gpndat=%08x\n",S3C64XX_GPNCON,S3C64XX_GPNDAT);
		
    //setting the debounce timer
    ret=readl(S3C64XX_EINT0FLTCON0);
	writel((ret&(~0x3F))|0x3F,S3C64XX_EINT0FLTCON0);
	
	//and by andy 
     writel((readl(S3C_CLK_DIV1)& ~0xF)
			|(0xF), S3C_CLK_DIV1);
			printk("andy:div1=%08x\n",readl(S3C_CLK_DIV1));
	return 0;
}

unsigned int s3c6410_setup_sdhci0_detect_ext_cd(void)
{
	unsigned int ret;

	ret = readl(S3C64XX_GPNDAT);
	ret &= 0x0100;
	ret = !ret;


	return ret;
}
#endif
static u64 s3c_device_hsmmc_dmamask = 0xffffffffUL;

struct s3c_sdhci_platdata s3c_hsmmc0_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_SD_HIGHSPEED),
#if defined(CONFIG_CH0_EXTIRQ)
    .ext_cd      =CH0_CARD_IRQ,
	.cfg_ext_cd  =s3c6410_setup_sdhci0_cfg_ext_cd,
	.detect_ext_cd=s3c6410_setup_sdhci0_detect_ext_cd,
#endif
};

struct platform_device s3c_device_hsmmc0 = {
	.name		= "s3c-sdhci",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc_resource),
	.resource	= s3c_hsmmc_resource,
	.dev		= {
		.dma_mask		= &s3c_device_hsmmc_dmamask,
		.coherent_dma_mask	= 0xffffffffUL,
		.platform_data		= &s3c_hsmmc0_def_platdata,
	},
};

void s3c_sdhci0_set_platdata(struct s3c_sdhci_platdata *pd)
{
	struct s3c_sdhci_platdata *set = &s3c_hsmmc0_def_platdata;

	set->max_width = pd->max_width;

	if (pd->host_caps)
		set->host_caps = pd->host_caps;
	if (pd->cfg_gpio)
		set->cfg_gpio = pd->cfg_gpio;
	if (pd->cfg_card)
		set->cfg_card = pd->cfg_card;
	if (pd->cfg_ext_cd)
		set->cfg_ext_cd = pd->cfg_ext_cd;
	if (pd->detect_ext_cd)
		set->detect_ext_cd = pd->detect_ext_cd;
	if (pd->ext_cd)
		set->ext_cd = pd->ext_cd;
}

