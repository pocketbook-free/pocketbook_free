/* linux/arch/arm/mach-s3c6410/setup-sdhci.c 
 *
 * Copyright 2008 Simtec Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C6410 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>

#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

#include <mach/hardware.h>
#include <linux/irq.h>

/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */

char *s3c6410_hsmmc_clksrcs[4] = {
	[0] = "hsmmc",
	[1] = "hsmmc",
	[2] = "hsmmc",
	/* [3] = "48m", - note not succesfully used yet */
//	[0] = NULL,
//	[1] = NULL,//"hsmmc"
//	[2] = MOUT_EPLL_CLK,
	/* [3] = "48m", - note not succesfully used yet */
};

char *s3c6410_hsmmc1_clksrcs[4] = {			//William add for change frequence	
	[0] = "hsmmc",
	[1] = "hsmmc",
	[2] = "hsmmc",
	/* [3] = "48m", - note not succesfully used yet */
//	[0] = NULL, //MOUT_EPLL_CLK, //"epll",	
//	[1] = NULL, //MOUT_EPLL_CLK, //"epll",	
//	[2] = MOUT_EPLL_CLK, //"epll",		// according to spec. CONTROL2 register bit[5:4] 00 or 01 is HCLK; 10 is EPLL, 11 is external clock source 
};

char *s3c6410_hsmmc2_clksrcs[4] = {
	[0] = "hsmmc",
	[1] = "hsmmc",
	[2] = "hsmmc",
	/* [3] = "48m", - note not succesfully used yet */
};

void s3c6410_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C64XX_GPG(2 + width);

	/* Set all the necessary GPG pins to special-function 0 */
	for (gpio = S3C64XX_GPG(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

    // Card Detect
#if 0
	s3c_gpio_setpull(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S3C64XX_GPG(6), S3C_GPIO_SFN(2));
#endif

}

void s3c6410_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;
	int cfg;
	width=4;
	end = S3C64XX_GPH(2 + width);

	/* Set all the necessary GPG pins to special-function 0 */
	for (gpio = S3C64XX_GPH(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
	s3c_gpio_cfgpin(S3C64XX_GPL(3), S3C_GPIO_SFN(1));
	s3c_gpio_setpull(S3C64XX_GPL(3), S3C_GPIO_PULL_NONE);
	gpio_set_value(S3C64XX_GPL(3), 0);			//Power on 
	s3c_gpio_cfgpin(S3C64XX_GPG(6), S3C_GPIO_SFN(0));
	s3c_gpio_setpull(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
	s3c_gpio_slp_setpull_updown(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
	
	s3c_gpio_slp_cfgpin(S3C64XX_GPG(6), 2);   // can not use S3C_GPIO_SFN(2) tp set. have to use 0,1,2,3
	//printk("S3C64XX_GPGCONSLP is 0x%08x\n", readl(S3C64XX_GPGCONSLP));
	//printk("S3C64XX_GPGPUDSLP is 0x%08x\n", readl(S3C64XX_GPGPUDSLP));	
	//William add 20100819
#if defined(CONFIG_HW_EP1_EVT) || defined(CONFIG_HW_EP2_EVT) ||  \
	defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT) ||  \
	defined(CONFIG_HW_EP1_EVT2) || defined(CONFIG_HW_EP2_EVT2) ||  \
	defined(CONFIG_HW_EP3_EVT2) || defined(CONFIG_HW_EP4_EVT2)
	
	
	s3c_gpio_cfgpin(S3C64XX_GPG(6), S3C_GPIO_SFN(7));
	s3c_gpio_setpull(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_EINT_GROUP(5, 6), IRQ_TYPE_EDGE_BOTH);
	cfg = __raw_readl(S3C64XX_EINT56FLTCON);
	cfg &= ~ 0xff;
	cfg |= 0xff;
	__raw_writel(cfg, S3C64XX_EINT56FLTCON);

#elif   defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP2_DVT) || \
	 	defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)

		s3c_gpio_cfgpin(S3C64XX_GPL(14), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S3C64XX_GPL(14), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_BOTH);
		//blazer adds 2010.09.10
		cfg = __raw_readl(S3C64XX_EINT0FLTCON2);
		cfg &= ~0xff000000;				//Filter for EINT22,23
		cfg |=  0xbf000000;				//Filter selection for EINT22,23: 0=delay filter  1=digital filter(clock count)
		__raw_writel(cfg, S3C64XX_EINT0FLTCON2);
	
#endif

}

void s3c6410_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

    //printk(">>> s3c6410_setup_sdhci2_cfg_gpio, width= %d\n", width);

    width = 4;
    end = S3C64XX_GPH(6+width);

    /* Set all the necessary GPH pins to special-function 3 */
//&*&*&*EH1_20100113, enable BCM4319 Wi-Fi
    for (gpio = S3C64XX_GPH(6); gpio < end; gpio++)
    {
        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
    }

	//WF_PWR_DNn
// s3c_gpio_cfgpin(S3C64XX_GPN(3), S3C_GPIO_SFN(0));
//	s3c_gpio_setpull(S3C64XX_GPN(3), S3C_GPIO_PULL_NONE);
  
    // WF_SD_CMD
	s3c_gpio_cfgpin(S3C64XX_GPC(4), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S3C64XX_GPC(4), S3C_GPIO_PULL_NONE);

    // WF_SD_CLK
	s3c_gpio_cfgpin(S3C64XX_GPC(5), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S3C64XX_GPC(5), S3C_GPIO_PULL_NONE);
//&*&*&*EH2_20100113, enable BCM4319 Wi-Fi

    //printk("<<< s3c6410_setup_sdhci2_cfg_gpio\n");
}

void s3c6410_setup_sdhci0_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2 = 0, ctrl3 = 0;

	/* don't need to alter anything acording to card-type */

	writel(S3C64XX_SDHCI_CONTROL4_DRIVE_9mA, r + S3C64XX_SDHCI_CONTROL4);

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;		//Slect HCLK as Base clock source 133MHz
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		S3C_SDHCI_CTRL2_DFCNT_NONE |
		S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios->clock < 25 * 1000000)
	{
		ctrl3 = (S3C_SDHCI_CTRL3_FCSEL3 |
			 S3C_SDHCI_CTRL3_FCSEL2 |
			 S3C_SDHCI_CTRL3_FCSEL1 |
			 S3C_SDHCI_CTRL3_FCSEL0);
	}
	else
	{
		ctrl3 = S3C_SDHCI_CTRL3_FCSEL0;
	}
	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

//&*&*&*EH1_20100113, enable BCM4319 Wi-Fi
#define SDHCI_HOST_CONTROL      0x28

void s3c6410_setup_sdhci2_cfg_card(struct platform_device *dev,
                    void __iomem *r,
                    struct mmc_ios *ios,
                    struct mmc_card *card)
{
    u32 ctrl2 = 0, ctrl3 = 0;
    u32 status;

    //printk(">>> s3c6410_setup_sdhci2_cfg_card\n");

    /* don't need to alter anything acording to card-type */

    writel(S3C64XX_SDHCI_CONTROL4_DRIVE_9mA, r + S3C64XX_SDHCI_CONTROL4);

    ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
    ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
    ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
              S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
              S3C_SDHCI_CTRL2_DFCNT_64SDCLK |
              S3C_SDHCI_CTRL2_ENCLKOUTHOLD | 
//&*&*& SJ1_20100205, set EPLL clk source 
              (0x2<<4)
//&*&*& SJ2_20100205, set EPLL clk source 
              );

    if (ios->clock < 25 * 1000000)
    {
        ctrl3 = (S3C_SDHCI_CTRL3_FCSEL3 |
             S3C_SDHCI_CTRL3_FCSEL2 |
             S3C_SDHCI_CTRL3_FCSEL1 |
             S3C_SDHCI_CTRL3_FCSEL0);
    }
    else
    {
        ctrl3 = (S3C_SDHCI_CTRL3_FCSEL1 | S3C_SDHCI_CTRL3_FCSEL0);
    }

#if 1
    status = readl(r+ SDHCI_HOST_CONTROL);

    writel((status|0xc0), r + SDHCI_HOST_CONTROL);
#endif 

    writel(ctrl2, r + S3C_SDHCI_CONTROL2);
    writel(ctrl3, r + S3C_SDHCI_CONTROL3);

//    printk("<<< s3c6410_setup_sdhci2_cfg_card:%x\n",ctrl2);
//	printk("tony clkcon0:%x\n",readl(r+0x2c));
}
//&*&*&*EH2_20100113, enable BCM4319 Wi-Fi

