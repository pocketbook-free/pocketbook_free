/*
 *   --  Generic I2C driver for max8698 PMIC
 *
 * Copyright 2007, 2008 Microelectronics PLC.
 *
 * Author: Liam Girdwood
 *         linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/mfd/max8698/core.h>

#ifdef CONFIG_STOP_MODE_SUPPORT
#include <linux/earlysuspend.h>
#endif

#define core_initcall(fn)		__define_initcall("1",fn,1)

//extern int pm_policy_flag;
extern int adapter_usb_power_insert_flag;
extern void udc_stop_mode();
extern void disable_udc_stop_mode();
struct max8698 *global_max8698=NULL;		//Henry: 1002: turn off device with SYS_3.3V OFF
static int max8698_i2c_read_device(struct max8698 *max8698, char reg,
				  int bytes, void *dest)
{
	int ret;

	ret = i2c_master_send(max8698->i2c_client, &reg, 1);
	if (ret < 0)
		return ret;
	ret = i2c_master_recv(max8698->i2c_client, dest, bytes);
	if (ret < 0)
		return ret;
	if (ret != bytes)
		return -EIO;
		
	return 0;
}

static int max8698_i2c_write_device(struct max8698 *max8698, char reg,
				   int bytes, void *src)
{
	/* we add 1 byte for device register */
	u8 msg[(MAX8698_MAX_REGISTER << 1) + 1];
//  u8 msg[2];
	int ret;

	if (bytes > ((MAX8698_MAX_REGISTER << 1) + 1))
		return -EINVAL;

	msg[0] = reg;
	memcpy(&msg[1], src, bytes);
	ret = i2c_master_send(max8698->i2c_client, msg, bytes + 1);
	if (ret < 0)
		return ret;
	if (ret != bytes + 1)
		return -EIO;
	return 0;
}
#ifdef CONFIG_STOP_MODE_SUPPORT
static struct i2c_client *p_i2c_client = NULL;

extern bool get_sierra_module_status(void);
static void max8698_early_suspend(struct early_suspend *h)
{
#if 0	// Henry Li: cancel as lead to OTG unstable
	disable_udc_stop_mode();
#endif
        if (p_i2c_client)
        {
		struct max8698 *max8698 = i2c_get_clientdata(p_i2c_client);
		int reg_value, ret;

		/* Disable LDO3, OTG_1.2V */
	/* Henry@2010-10-6 --> leave bit8 of STOP_CFG as 1 (TOP_LOGIC active mode)
	    Otherwise device will crash at STOP mode while 3g is running */
		if (get_sierra_module_status() == false)	// 1105		
		{
		reg_value = max8698_reg_read(max8698, 0x00);
		max8698_reg_write(max8698, 0x00, (reg_value & ~(0x1 << 3)));
		}

		/* Disable LDO8, OTG_3.3V */
		/* Disable LDO7, LCD_3.3V */
		reg_value = max8698_reg_read(max8698, 0x01);
		if (get_sierra_module_status() == false)	// 1105		
			{
		max8698_reg_write(max8698, 0x01, (reg_value & ~(0x1 << 5) & ~(0x1 << 6)));
			}
		else
			max8698_reg_write(max8698, 0x01, (reg_value  & ~(0x1 << 6)));
		adapter_usb_power_insert_flag = 1;
        }
}

static void max8698_late_resume(struct early_suspend *h)
{
        if (p_i2c_client)
        {
		struct max8698 *max8698 = i2c_get_clientdata(p_i2c_client);
		int reg_value, ret;

		/* Disable LDO3, OTG_1.2V */
		reg_value = max8698_reg_read(max8698, 0x00);
		max8698_reg_write(max8698, 0x00, (reg_value | (0x1 << 3)));

		/* Enable LDO8, OTG_3.3V */
		/* Enable LDO7, LCD_3.3V */
		reg_value = max8698_reg_read(max8698, 0x01);
		max8698_reg_write(max8698, 0x01, (reg_value | (0x1 << 5) | (0x1 << 6)));
#if 0	// 1105	// Henry Li: cancel as lead to OTG unstable
		udc_stop_mode();
#endif
		adapter_usb_power_insert_flag = 2;
        }
}

struct early_suspend max8698_early_suspend_desc = {
        .level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
        .suspend = max8698_early_suspend,
        .resume = max8698_late_resume,
};
#endif

static int max8698_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct max8698 *max8698;
	int ret = 0;
	int data;


printk(KERN_INFO "######*****%s start ..\n",__FUNCTION__);
	max8698 = kzalloc(sizeof(struct max8698), GFP_KERNEL);
	if (max8698 == NULL) {
		kfree(i2c);
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, max8698);
	max8698->dev = &i2c->dev;
	max8698->i2c_client = i2c;
	max8698->read_dev = max8698_i2c_read_device;
	max8698->write_dev = max8698_i2c_write_device;

   ret = max8698_device_init(max8698,i2c->dev.platform_data);
 // ret = max8698_device_init(max8698,i2c->irq,i2c->dev.platform_data);

//	max8698_i2c_read_device(max8698,0x08,1,&data);
	printk("i2c->irq = %d&&&&&&\n",i2c->irq);
#ifdef CONFIG_STOP_MODE_SUPPORT
        p_i2c_client = i2c;
        register_early_suspend(&max8698_early_suspend_desc);
#endif
	global_max8698 = max8698;	//Henry: 1002: turn off device with SYS_3.3V OFF
	if (ret < 0)
		goto err;

	return ret;

err:
	kfree(max8698);
	return ret;
}

static int max8698_power_suspend(struct i2c_client *i2c,pm_message_t mesg)
{
	struct max8698 *max8698 = i2c_get_clientdata(i2c);
	int reg_value,ret;
	
	printk("[%s]: max8698 suspend \n", __FUNCTION__);
	
		// Henry Li: cancel as lead to OTG unstable
	if (get_sierra_module_status() == false)
	disable_udc_stop_mode();
	reg_value = max8698_reg_read(max8698,0x00);
	if (reg_value < 0) 
	{
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
//	ret = max8698_reg_write(max8698,0x00,reg_value&0xF2);
	ret = max8698_reg_write(max8698,0x00,reg_value&0x32);
	if (ret < 0) 
	{
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
	reg_value = max8698_reg_read(max8698,0x01);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}

  //EPD_PWR_OFF(0);   //Added for kernel cann't sleep, EPD_PWR_EN and LDO7 must shutdown at the same time.
	ret = max8698_reg_write(max8698,0x01,reg_value&0x10);		//off LDO7
	//ret = max8698_reg_write(max8698,0x01,reg_value&0x90);
	//ret = max8698_reg_write(max8698,0x01,0x1F);	//LDO7 on is sleep
  	
	if (ret < 0) 
  {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
/*********************************************************
	pull low sys_3.3v to sys_3.0v
*********************************************************/
	reg_value = max8698_reg_read(max8698,0x0e);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}

	 ret = max8698_reg_write(max8698,0x0e,0x0e);		//off LDO9
  	
	if (ret < 0) 
  {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	adapter_usb_power_insert_flag = 1;
	
	return 0;
}

static int max8698_power_resume(struct i2c_client *i2c)
{
	struct max8698 *max8698 = i2c_get_clientdata(i2c);
	int reg_value,ret;
	
		printk("[%s]: max8698 Resume \n", __FUNCTION__);
	
	//enable buck and ldo
	reg_value = max8698_reg_read(max8698,0x0);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
	ret = max8698_reg_write(max8698,0x0,reg_value|0xFE);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
	reg_value = max8698_reg_read(max8698,0x1);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
	
	reg_value = 0x00;
	ret = max8698_reg_write(max8698,0x1,reg_value|0xF0);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
/*********************************************************
	pull high sys_3.0v to sys_3.3v
*********************************************************/
	reg_value = max8698_reg_read(max8698,0x0e);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}

	 ret = max8698_reg_write(max8698,0x0e,0x11);		//off LDO9
  	
	if (ret < 0) 
  {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}

	udc_stop_mode();
	adapter_usb_power_insert_flag = 2;
//	mdelay(100);
//	pm_policy_flag = 0;
		
	return 0;
}

/* HenryLi 2010-10-2: Turn OFF SYS_3.3V */
/* Henry@10-02: Fix bug of "After long key press => power off,  the system can't wake up anymore" */
void turn_LDO9_off(void)
{
	int reg_value;
	struct max8698 *max8698;

    max8698 = global_max8698;

    if (max8698 != NULL)
     {
	reg_value = max8698_reg_read(max8698,0x01);	
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read register01: %d\n", reg_value);
		return;
	}
	max8698_reg_write(max8698,0x01,reg_value & 0xef);		// Henry: Bit4 ELDO9 0: Turn LDO9 off	
     }
}
EXPORT_SYMBOL_GPL(turn_LDO9_off);
	
static int max8698_i2c_remove(struct i2c_client *i2c)
{
	struct max8698 *max8698 = i2c_get_clientdata(i2c);

#ifdef CONFIG_STOP_MODE_SUPPORT
        p_i2c_client = NULL;
        unregister_early_suspend(&max8698_early_suspend_desc);
#endif

	max8698_device_exit(max8698);
	kfree(max8698);

	return 0;
}

static const struct i2c_device_id max8698_i2c_id[] = {
       { "max8698", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, max8698_i2c_id);


static struct i2c_driver max8698_i2c_driver = {
	.driver = {
		   .name = "max8698",
		   .owner = THIS_MODULE,
	},
	.probe = max8698_i2c_probe,
	.suspend = max8698_power_suspend,
	.resume = max8698_power_resume,
	.remove = max8698_i2c_remove,
	.id_table = max8698_i2c_id,
};

static int __init max8698_i2c_init(void)
{
	return i2c_add_driver(&max8698_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max8698_i2c_init);

static void __exit max8698_i2c_exit(void)
{
	i2c_del_driver(&max8698_i2c_driver);
}
module_exit(max8698_i2c_exit);

MODULE_DESCRIPTION("I2C support for the max8698 PMIC");
MODULE_LICENSE("GPL");
