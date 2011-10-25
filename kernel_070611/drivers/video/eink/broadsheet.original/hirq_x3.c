/*
 * hirq_ep3.c
 */

#include "../hal/einkfb_hal.h"
#include "broadsheet.h"
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/irqs.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-bank-q.h>
#include <plat/gpio-cfg.h>

extern void epd_set_HIRQ(unsigned short cmd);


struct x3_hirq_info
{	
	int x3_hirq_status;
};

static struct x3_hirq_info *x3_hirq_info_data;


int x3_hirq_status(void)
{
	unsigned int	hp_conectd_n = 0;

	/* set GPO(3) as input to get x3_hirq_info_data status*/
	s3c_gpio_cfgpin(S3C64XX_GPO(3),S3C64XX_GPO_INPUT(3));

	hp_conectd_n = gpio_get_value(S3C64XX_GPO(3));
	
	/* set GPO(3) as EXT_INTTERRUPT7(3) for x3_hirq_info_data hotplug */
	s3c_gpio_cfgpin(S3C64XX_GPO(3),S3C64XX_GPO3_EINT_G7_3);

	printk("[EPD_interrupt]%d\n", hp_conectd_n);
	if(hp_conectd_n)
		epd_set_HIRQ(0xFF);	
	
	return (hp_conectd_n);
}

void x3_hirq_set_config(void)
{
	/* set GPO(3) as EXT_INTTERRUPT7(3) for x3_hirq_info_data hotplug */
	s3c_gpio_cfgpin(S3C64XX_GPO(3),S3C64XX_GPO3_EINT_G7_3);
	s3c_gpio_setpull(S3C64XX_GPO(3), S3C_GPIO_PULL_DOWN);	
		//Rising edge triggered
	writel((readl(S3C64XX_EINT78CON) & (~(0x7 << 0)))| (0x4 << 0) , S3C64XX_EINT78CON);			
}

void x3_hirq_detect(struct work_struct *work)
{	
		printk("%d\n", x3_hirq_status());	
}


static int x3_hirq_probe(struct platform_device *pdev)
{
	int ret;
	
	printk("[EP3 HIRQ] (x3_hirq) driver +++\n");
	
	x3_hirq_info_data = kzalloc(sizeof(struct x3_hirq_info), GFP_KERNEL);
	if (!x3_hirq_info_data)
		return -ENOMEM;
		
	epd_set_HIRQ(0);	
	
	x3_hirq_set_config();	
	
	ret = request_irq(IRQ_EINT_GROUP(7, 3), x3_hirq_status, 0,
			 "x3_hirq detect", NULL);
	if (ret < 0) {
		printk("fail to claim x3_hirq irq , ret = %d\n", ret);
		return -EIO;
	}	

	printk("[EP3HIRQ]  (x3_hirq) driver ---\n");	
	return 0;		
		
input_err:
	free_irq(IRQ_EINT_GROUP(7, 3), 0);
	printk(KERN_ERR "x3_hirq: Failed to register driver\n");
	return ret;
	
}

static int x3_hirq_remove(struct platform_device *pdev)
{
	flush_scheduled_work();
	
	free_irq(IRQ_EINT_GROUP(7, 3), 0);

	return 0;
}

static struct platform_device x3_hirq_device = {
	.name		= "EPD_HIRQ",
};

static struct platform_driver x3_hirq_driver = {
	.probe		= x3_hirq_probe,
	.remove		= x3_hirq_remove,
	.driver		= {
		.name		= "EPD_HIRQ",
		.owner		= THIS_MODULE,
	},
};

static int __init x3_hirq_init(void)
{
	int ret;

	ret = platform_driver_register(&x3_hirq_driver);
	
	if (ret)
		return ret;
		
	return platform_device_register(&x3_hirq_device);
}

static void __exit x3_hirq_exit(void)
{
	platform_device_unregister(&x3_hirq_device);
	platform_driver_unregister(&x3_hirq_driver);
}

//late_initcall(x3_hirq_init);

//module_init(x3_hirq_init);
module_exit(x3_hirq_exit);

MODULE_AUTHOR("Foxconn");
MODULE_DESCRIPTION("driver for eintgroup7[3]");
MODULE_LICENSE("GPL");
