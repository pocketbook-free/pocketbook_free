/*
 * max8698-core.c  --  Device access for max8698
 *
 *
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

#include <linux/mfd/max8698/core.h>


static int max8698_read(struct max8698 *max8698,u8 reg,int num_regs,u8 *dest)
{
	int ret = 0;
	int bytes = num_regs ;
	
	 ret = max8698->read_dev(max8698,reg,bytes,(char*)dest);
	 
	 return ret;
}

u8 max8698_reg_read(struct max8698 *max8698,int reg)
{
	u8 data;
	int err;
	err = max8698_read(max8698,reg,1,&data);
	if(err)
	{
	  dev_err(max8698->dev, "read from reg R%d failed\n", reg);
	  return err;
	} 
	  return data;
}

static int max8698_write(struct max8698 *max8698,u8 reg, int num_regs, u8 *src)
{
	int bytes = num_regs;
	
	return max8698->write_dev(max8698, reg, bytes, (char *)src);
}


int max8698_reg_write(struct max8698 *max8698,int reg,u8 val)
{
	int ret;
	u8 data = val;
	ret = max8698_write(max8698,reg,1,&data);
	if(ret)
	  dev_err(max8698->dev, "write to reg R%d failed\n", reg);
	  
	return ret;
}


//int max8698_device_init(struct max8698 *max8698,int irq,struct max8698_platform_data *pdata)
int max8698_device_init(struct max8698 *max8698,struct max8698_platform_data *pdata)
{
	int ret,reg_value;
	
	
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
	
	//setting buck1 to 1.1v
	reg_value = max8698_reg_read(max8698,0x4);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
	reg_value = reg_value & (0xF0);
	ret = max8698_reg_write(max8698,0x4,reg_value|0x7);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
		//setting buck2 to 1.2v
	reg_value = max8698_reg_read(max8698,0x6);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
	reg_value = reg_value & (0xF0);
	ret = max8698_reg_write(max8698,0x6,reg_value|0x9);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
		//setting buck3 to 1.8v
	reg_value = max8698_reg_read(max8698,0x7);
	if (reg_value < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", reg_value);
		return reg_value;
	}
	reg_value = reg_value & (0xF0);
	ret = max8698_reg_write(max8698,0x7,reg_value|0x2);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
	//ldo2,ldo3 voltage setting 1.2v
	
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x08,reg_value|0x88);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	//ldo4 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x09,reg_value|0x11);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	//ldo5 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x0a,reg_value|0x11);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
#if 0
	//ldo6 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x0b,reg_value|0x11);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
#endif
	//ldo7 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x0c,reg_value|0x11);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
	//ldo8 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x0d,reg_value|0x30);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	//ldo9 voltage setting 3.3v
	reg_value = 0x0;
	ret = max8698_reg_write(max8698,0x0e,reg_value|0x11);
	if (ret < 0) {
		dev_err(max8698->dev, "Failed to read ID: %d\n", ret);
		return ret;
	}
	
if (pdata && pdata->init) {
		ret = pdata->init(max8698);
		if (ret != 0) {
			dev_err(max8698->dev, "Platform init() failed: %d\n",
				ret);
			goto err;
		}
	}
		return 0;
err:
  return ret;
}
EXPORT_SYMBOL_GPL(max8698_device_init);


void max8698_device_exit(struct max8698 *max8698)
{
	platform_device_unregister(max8698->power.pdev);
//		free_irq(max8698->chip_irq, NULL);
}
EXPORT_SYMBOL_GPL(max8698_device_exit);

