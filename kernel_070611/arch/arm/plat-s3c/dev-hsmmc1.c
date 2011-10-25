/* linux/arch/arm/plat-s3c/dev-hsmmc1.c 
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for hsmmc device 1
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
static struct resource s3c_hsmmc1_resource[] = {
	[0] = {
		.start = S3C_PA_HSMMC1,
		.end   = S3C_PA_HSMMC1 + S3C_SZ_HSMMC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSMMC1,
		.end   = IRQ_HSMMC1,
		.flags = IORESOURCE_IRQ,
	},
#if defined(CONFIG_HW_EP1_EVT) || defined(CONFIG_HW_EP2_EVT) ||  \
	defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT) ||  \
	defined(CONFIG_HW_EP1_EVT2) || defined(CONFIG_HW_EP2_EVT2) ||  \
	defined(CONFIG_HW_EP3_EVT2) || defined(CONFIG_HW_EP4_EVT2)	
	[2] = {
		.start = IRQ_EINT_GROUP(5, 6),
		.end   = IRQ_EINT_GROUP(5, 6),
		.flags = IORESOURCE_IRQ,			//William add 20100819
	},
#elif 	defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP2_DVT) || \
	 	defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)
	 	
	[2] = {
		.start = IRQ_EINT(22),
		.end   = IRQ_EINT(22),
		.flags = IORESOURCE_IRQ,			//William add 20100819
	},
		
#endif
};


int detect_ext_card(void)
{	
	
#if 	defined(CONFIG_HW_EP1_EVT) || defined(CONFIG_HW_EP2_EVT) ||  \
		defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT) ||  \
		defined(CONFIG_HW_EP1_EVT2) || defined(CONFIG_HW_EP2_EVT2) ||  \
		defined(CONFIG_HW_EP3_EVT2) || defined(CONFIG_HW_EP4_EVT2)	

	return gpio_get_value(S3C64XX_GPG(6));	

#elif 	defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP2_DVT) || \
	 	defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)

	return gpio_get_value(S3C64XX_GPL(14));

#endif

}

unsigned int power_pin(int onoff)
{
	if(onoff)		
	{	
		gpio_set_value(S3C64XX_GPL(3), 0);		//PowerOn 
	}
	else
	{
		gpio_set_value(S3C64XX_GPL(3), 1);
	}
	return gpio_get_value(S3C64XX_GPL(3));

}
unsigned int power_pin_status(void)
{
	return gpio_get_value(S3C64XX_GPL(3));
}

static u64 s3c_device_hsmmc1_dmamask = 0xffffffffUL;

struct s3c_sdhci_platdata s3c_hsmmc1_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_SD_HIGHSPEED),

#if defined(CONFIG_HW_EP1_EVT) || defined(CONFIG_HW_EP2_EVT) ||  \
	defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT) ||  \
	defined(CONFIG_HW_EP1_EVT2) || defined(CONFIG_HW_EP2_EVT2) ||  \
	defined(CONFIG_HW_EP3_EVT2) || defined(CONFIG_HW_EP4_EVT2)	
				
	.ext_cd = IRQ_EINT_GROUP(5, 6),
    
#elif defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP2_DVT) || \
	 defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)
	 
	.ext_cd = IRQ_EINT(22),
#endif
	.detect_ext_cd = detect_ext_card,
	.power_pin = power_pin,		//William add 20100911
	.power_pin_status = power_pin_status,
};

struct platform_device s3c_device_hsmmc1 = {
	.name		= "s3c-sdhci",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc1_resource),
	.resource	= s3c_hsmmc1_resource,
	.dev		= {
		.dma_mask		= &s3c_device_hsmmc1_dmamask,
		.coherent_dma_mask	= 0xffffffffUL,
		.platform_data		= &s3c_hsmmc1_def_platdata,
	},
};



void s3c_sdhci1_set_platdata(struct s3c_sdhci_platdata *pd)
{
	struct s3c_sdhci_platdata *set = &s3c_hsmmc1_def_platdata;

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

