/*
 * Author: Liam Girdwood, Mark Brown
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <plat/regs-otg.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

#include <linux/mfd/max8698/core.h>


extern void max8698_init_charger(struct max17043_device_info *max8698);

static void max17043_irq_call_handler(struct max17043_device_info *max17043, int irq)
{
	mutex_lock(&max17043->irq_mutex);

	if (max17043->irq[irq].handler)
		max17043->irq[irq].handler(max17043, irq, max17043->irq[irq].data);
	else {
		dev_err(max17043->dev, "irq %d nobody cared. now masked.\n",
			irq);
	}

	mutex_unlock(&max17043->irq_mutex);
}

int max17043_register_irq(struct max17043_device_info *max17043, int irq,
			void (*handler) (struct max17043_device_info *, int, void *),
			void *data)
{
	if (irq < 0 || irq > MAX8698_NUM_IRQ || !handler)
		return -EINVAL;

	if (max17043->irq[irq].handler)
		return -EBUSY;

	mutex_lock(&max17043->irq_mutex);
	max17043->irq[irq].handler = handler;
	max17043->irq[irq].data = data;
	mutex_unlock(&max17043->irq_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(max17043_register_irq);

/*
 * max8698_irq_worker actually handles the interrupts.  Since all
 * interrupts are clear on read the IRQ line will be reasserted and
 * the physical IRQ will be handled again if another interrupt is
 * asserted while we run - in the normal course of events this is a
 * rare occurrence so we save I2C/SPI reads.
 */
 
static void max17043_irq_worker(struct work_struct *work)
{
	struct max17043_device_info *max17043 = container_of(work, struct max17043_device_info, irq_work);
	
	max17043_irq_call_handler(max17043,MAX8698_IRQ_DETECT_POWER);
		enable_irq(max17043->chip_irq);
}

static irqreturn_t max17043_irq(int irq, void *data)
{
	struct max17043_device_info *max17043 = data;

	disable_irq_nosync(irq);
	schedule_work(&max17043->irq_work);

	return IRQ_HANDLED;
}

int max17043_free_irq(struct max17043_device_info *max17043, int irq)
{
	if (irq < 0 || irq > MAX8698_NUM_IRQ)
		return -EINVAL;

	mutex_lock(&max17043->irq_mutex);
	max17043->irq[irq].handler = NULL;
	mutex_unlock(&max17043->irq_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(max17043_free_irq);

static void max17043_client_dev_register(struct max17043_device_info *max17043,
				       const char *name,
				       struct platform_device **pdev)
{
	int ret;

printk(KERN_INFO "######*****%s start ..\n",__FUNCTION__);
	*pdev = platform_device_alloc(name, -1);
	if (pdev == NULL) {
		dev_err(max17043->dev, "Failed to allocate %s\n", name);
		return;
	}

	(*pdev)->dev.parent = max17043->dev;
	platform_set_drvdata(*pdev, max17043);
	ret = platform_device_add(*pdev);
	if (ret != 0) {
		dev_err(max17043->dev, "Failed to register %s: %d\n", name, ret);
		platform_device_put(*pdev);
		*pdev = NULL;
	}
}

#if 1
int max17043_power_charger_init(struct max17043_device_info *max17043,int irq,struct max17043_platform_data *pdata)
{
	int ret,err;
	int i = 0;
	printk(KERN_INFO "######*****%s start ..\n",__FUNCTION__);
	
	if (pdata && pdata->init) {
		ret = pdata->init(max17043);
		if (ret != 0) {
			dev_err(max17043->dev, "Platform init() failed: %d\n",
				ret);
			goto err;
		}
	}

	mutex_init(&max17043->irq_mutex);
	
	max8698_init_charger(max17043);
	
	INIT_WORK(&max17043->irq_work, max17043_irq_worker);
	if (irq)
	{
		
		ret = request_irq(irq, max17043_irq, 0,//IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				  "max8698_irq", max17043);
		if (ret != 0) 
		{
			dev_err(max17043->dev, "Failed to request IRQ: %d\n",
				ret);
			goto err;
		}
		
		charge_state_irq_init(max17043,pdata);

	}
	else 
	{
		dev_err(max17043->dev, "No IRQ configured\n");
		goto err;
	}
	max17043->chip_irq = irq;
	
	printk("######*****enter register power ..$$$$$$$$$\n");
	  max17043_client_dev_register(max17043, "max8698-power",
				   &(max17043->power.pdev));
				   
	

		return 0;
err:
  return ret;
	
}
#endif

void max17043_device_exit(struct max17043_device_info *max17043)
{
	platform_device_unregister(max17043->power.pdev);
		free_irq(max17043->chip_irq, NULL);
}
EXPORT_SYMBOL_GPL(max17043_device_exit);
