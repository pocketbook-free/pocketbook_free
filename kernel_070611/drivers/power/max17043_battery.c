/*
 * BQ27x00 battery driver
 *
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/init.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <plat/regs-gpio.h>
#include <linux/mfd/max8698/core.h>
#include <plat/regs-clock.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#define DRIVER_VERSION			"1.0.0"

#define MAX17043_REG_VCELL		0x02
#define MAX17043_REG_SOC		  0x04
#define MAX17043_REG_MODE		  0x06 /* Relative State-of-Charge */
#define MAX17043_REG_VERSION	0x08
#define MAX17043_REG_CONFIG		0x0C
#define MAX17043_REG_COMMAND	0xFE

/*CONFIG register command*/
#define SLEEP  	0x9794  // sleep bit set to 1 ,IC get into sleep
#define WAKE	0x9714  //sleep bit set to 0 ,IC wake up
//#define ATHD	0x9714 // set  10% percent of battery as low battery alert

/*COMMAND register command */
#define POR 	0x5400 // IC power on reset
#define STR	0x4000 // IC quick start

typedef int __bitwise burn_state_t;
extern int tmp_value;
#define arch_initcall(fn)		__define_initcall("3",fn,3)

int batt_rsoc = 0;
int charge_flag = 0;
#define BURN_ON		((__force burn_state_t) 1)
//#define WIFI_POWER_ON	  ((__force burn_state_t) 1)


static int max17043_write_code_restore(u8 reg,char *wt_value,
			 struct i2c_client *client);
static int max17043_write_code_burn(u8 reg,char *wt_value,
			 struct i2c_client *client);
static int max17043_write_code(u8 reg,int wt_value,
			 struct i2c_client *client);

static struct delayed_work detect_charge_timeout;
int charge_full_flag ;

/* If the system has several batteries we need a different name for each
 * of them...
 */

//extern int s3c_adc_convert();
extern int s3c_adc_convert_temp_data();
//extern int detect_usb_ac_charge();
extern int s3c_adc_get_adc_data(int channel);
static const char * const burn_states[2] = {
	[BURN_ON]	= "burn",
//	[WIFI_POWER_ON]		= "on",
};

static DEFINE_IDR(battery_id);
static DEFINE_MUTEX(battery_mutex);

static struct max17043_device_info *batt_info=NULL;

/*
 * Common code for max17043 devices read register
 */

static int max17043_battery_read(u8 reg, int *rt_value, int b_single,
			struct max17043_device_info *di)
{
	int ret;

	ret = di->bus->read(reg, rt_value, b_single, di);
	*rt_value = be16_to_cpu(*rt_value);

	return ret;
}

/*
 * Common code for max17043 devices write register
 */
static int max17043_battery_write(u8 reg, void *rt_value,int num_regs,
			struct max17043_device_info *di)
{
	int ret;
	int bytes = num_regs * 2;

	ret = di->bus->write(reg,bytes,rt_value,di);

	return ret;
}
/*
 * Return the battery temperature in Celcius degrees
 * Or < 0 if something fails.
 */
 

int max17043_battery_temperature()
{
	#if 1
	int ret;
	int temp = 0;
	int temp1;
	int data;
	int i = 0;

	//temp = s3c_adc_convert();
	temp = s3c_adc_convert_temp_data();
	//temp= s3c_adc_get_adc_data(0);
	temp = temp*33000/4096;
 // printk("*******temp adc reg value0 = 0x%x**********\n",temp&0xffff);
  
  while(max17043_temp_map[i].V_bat_ntc >= temp)
  {
  	i++;
  }
//  printk("*******temp i= %d**********\n",i);
//  printk("*******max17043_temp_map[i].V_bat_ntc= %d**********\n",max17043_temp_map[i].V_bat_ntc);
  temp = max17043_temp_map[i].T_temp_ntc;

	return temp;
	#endif
	
	#if 0
  int adc_data = 0;
	int battery_voltage = 0;
	int adc_convert_vol = 0;
	int battery_convert_vol = 0;

	adc_convert_vol= s3c_adc_get_adc_data(0);

	return adc_convert_vol;
	#endif
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
static int max17043_battery_voltage(struct max17043_device_info *di)
{
	int ret;
	int volt = 0;

	ret = max17043_battery_read(MAX17043_REG_VCELL, &volt, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	}

	return volt;
}
EXPORT_SYMBOL(max17043_battery_voltage);
/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
static int max17043_battery_current(struct max17043_device_info *di)
{
	int ret;
	int curr = 0;
	int flags = 0;

/*
	ret = max17043_battery_read(BQ27x00_REG_AI, &curr, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading current\n");
		return 0;
	}
	ret = bq27x00_read(BQ27x00_REG_FLAGS, &flags, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error reading flags\n");
		return 0;
	}
	if ((flags & (1 << 7)) != 0) {
		dev_dbg(di->dev, "negative current!\n");
		return -curr;
	}
	*/
	return curr;
}


/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
int max17043_battery_rsoc(struct max17043_device_info *di)
{
	int ret;
	int rsoc = 0;
	float capacity;
  int quick_state_value=STR;
//  struct i2c_client	*client = di->client;
  
  max17043_battery_write(MAX17043_REG_COMMAND,&quick_state_value,1,batt_info);
  
//  max17043_write_code(MAX17043_REG_MODE,STR,client);
  mdelay(30);
	ret = max17043_battery_read(MAX17043_REG_SOC, &rsoc, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading relative State-of-Charge\n");
		return ret;
	}
	
  capacity = 1.0*(rsoc &0xff) + 1.0*(rsoc >>8)/256;
//	batt_capacity = (rsoc >> 8) + (rsoc & 0x0f)*1/256;
//  printk("battery capacity = %f\n",capacity);
  
	rsoc = rsoc &0xff;
	return rsoc;
}

int voltage_value_change(int reg_value)
{
	int value_high_bit;
	int value_low_bit;
	int value;
	
	
	value_high_bit = (reg_value & 0xff)<<4;
	value_low_bit = (reg_value & 0xff00)>>12;

	value = (value_high_bit + value_low_bit);

   value = value * 125;

	return value;
}

int max17043_battery_get_property(enum power_supply_property psp,
										union power_supply_propval *val)
{
	struct max17043_device_info *di;
	int vol_reg_value;
	int tmp;
	
	if(batt_info==NULL)
	{
		return 0;
	}
		
	di=batt_info;
	
	switch (psp) {
	case POWER_SUPPLY_PROP_BATT_VOL:
	case POWER_SUPPLY_PROP_PRESENT:
	#if 1
		vol_reg_value = max17043_battery_voltage(di);
   val->intval = voltage_value_change(vol_reg_value);
		
		if (psp == POWER_SUPPLY_PROP_PRESENT)
			val->intval = val->intval <= 0 ? 0 : 1;
	#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = max17043_battery_current(di);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		tmp= max17043_battery_rsoc(di);
		if(tmp > 100)
		 val->intval = 100;
		else
		 val->intval = tmp;
		break;
#if 1
	case POWER_SUPPLY_PROP_BATT_TEMP:
	if(tmp_value == 0x10)
		val->intval = 1;
	else
		val->intval = 0;
		break;
#endif
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_MCNAIR;
		break;
	
	default:
		return -EINVAL;
	}

	return 0;
}

EXPORT_SYMBOL(max17043_battery_get_property);

/*
 * max17043 specific code
 */

static int max17043_read(u8 reg, int *rt_value, int b_single,
			struct max17043_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int err;

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		if (!b_single)
			msg->len = 2;
		else
			msg->len = 1;

		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			if (!b_single)
				*rt_value = get_unaligned_be16(data);
			else
				*rt_value = data[0];

			return 0;
		}
	}
	return err;
}

/*For mode burn reading*/
static int max17043_read_code_burn(u8 reg, char *rt_value,
			struct i2c_client *client)
{
//	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[4];
	int err;
	int i;
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		msg->len = 4;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		printk("data[0] is %x and data[1] is %x,and data[2] is %x, and data[3] is %x, \n",data[0],data[1],data[2],data[3]);
		if (err >= 0) {		
		//	*rt_value = get_unaligned_be16(data);
		//printk(" get_unaligned_be16(data) is %x \n",*rt_value);
			for(i = 0;i<4;i++)
			{
				rt_value[i]=data[i];
			}
		
			return 0;
		}
	}
	return err;
}

static int max17043_write(u8 reg, int bytes,void *src,struct max17043_device_info *di)
{
	
	struct i2c_client *client = di->client;
	u8 msg[3];
	int ret;

	if (!client->adapter)
		return -ENODEV;
	
	msg[0] = reg;
	memcpy(&msg[1], src, bytes);
	ret = i2c_master_send(client, msg, bytes + 1);
	if (ret < 0)
		return ret;
	if (ret != bytes + 1)
		return -EIO;
	return 0;
	
}
/*For normal data writing*/
static int max17043_write_code(u8 reg,int wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[3];
	int err;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = data;

	data[0] = reg;
	data[1] = wt_value >> 8;
	data[2] = wt_value & 0xFF;
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

/*
 ************************ Max17043 Loading a Custom Model********************************
 */

/*1.Unlock Model Access*/
static int max17043_ulock_model(struct i2c_client *client)
{
	int ret ;
	ret = max17043_write_code(0x3E,0x4A57,client);
	if (ret) {
		printk( "writing unlock  Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*2.Read Original RCOMP and OCV Register*/
static int max17043_read_rccmp_ocv(struct i2c_client *client,char *rt_value)
{
	int ret;
	char rsoc[4];
	int i;
	ret = max17043_read_code_burn(0x0C,rsoc,client);
	if (ret) {
		printk( "reading rccmp and ocv  Register\n");
		return ret;
	}
	for(i=0;i<4;i++)
	{
		rt_value[i] = rsoc[i];
	}
	printk("rt_value[0] is %x and rt_value[1] is %x,and rt_value[2] is %x, and rt_value[3] is %x, \n",\
							    rt_value[0],rt_value[1],rt_value[2],rt_value[3]);
	return ret;
}

/*3.Write OCV Register*/
static int max17043_write_ocv(struct i2c_client *client)
{	
	int ret ;
//	ret = max17043_write_code(0x0E,0xDA30,client);
//	ret = max17043_write_code(0x0E,0xD9B0,client);
	ret = max17043_write_code(0x0E,0xDA80,client);
	if (ret) {
		printk( "writing OCV Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*4.Write RCCOMP Register to a Maximum value of 0xFF00h*/
static int max17043_write_rccomp(struct i2c_client *client)
{	
	int ret;
	ret = max17043_write_code(0x0C,0xFF00,client);
	if (ret) {
		printk( "writing rccomp Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*5.Write the Model,once the model is unlocked,
 *the host software must write the 64 byte model to the max17044.
 *the model is located between memory locations 0x40h and 0x7Fh.
 */
static int max17043_write_model(struct i2c_client *client)
{	
	int ret;
//	int i;

/*			  
	char data0[] = {0x8C,0x80,0xB9,0x70,0xB9,0xB0,0xB9,0xF0,
			  0xBC,0x30,0xBC,0xA0,0xBC,0xE0,0xBD,0x00};

	char data1[] = {0xBD,0x40,0xBD,0x80,0xBE,0xE0,0xC1,0x60,
			  0xC2,0x30,0xC4,0x70,0xC8,0x50,0xCF,0xB0};

	char data2[] = {0x00,0xA0,0x2B,0xE0,0x7D,0x00,0x00,0x20,
			  0x3D,0x20,0x6C,0xF0,0x80,0x00,0x59,0xF0};

	char data3[] = {0x4D,0xF0,0x07,0xF0,0x07,0x80,0x3C,0x60,
			  0x07,0xF0,0x0C,0x30,0x0A,0x00,0x0A,0x00};
*/			  
	char data0[] = {0xAB,0xF0,0xAF,0xB0,0xB6,0xA0,0xB6,0xD0,
			  0xB7,0xC0,0xBA,0x00,0xBB,0x10,0xBB,0xE0};

	char data1[] = {0xBC,0xC0,0xBD,0xB0,0xBE,0xF0,0xC0,0x00,
			  0xC2,0x00,0xC6,0x30,0xCB,0x10,0xD0,0x80};

	char data2[] = {0x01,0xF0,0x00,0x20,0xFF,0xF0,0x09,0x00,
			  0x14,0xE0,0x3D,0xE0,0x05,0xD0,0x5B,0xE0};

	char data3[] = {0X42,0xF0,0x2B,0xF0,0x2E,0xE0,0x1C,0xF0,
				0x1B,0x10,0x16,0x10,0x0F,0x10,0x0F,0x10};


#if 0
	for(i=0;i<16;i++)
	{	
		printk( "*********get into max17044 write model*************");
		printk( "data0[%d] is %x\n",i,data0[i]);
	}
#endif			
	ret = max17043_write_code_burn(0x40,data0,client);
	if (ret) {
		printk( "data0 writing mode,and ret is %d\n",ret);

	}

	ret = max17043_write_code_burn(0x50,data1,client);
	if (ret) {
		printk( "data1 writing mode,and ret is %d\n",ret);

	}

	ret = max17043_write_code_burn(0x60,data2,client);
	if (ret) {
		printk( "data2 writing mode,and ret is %d\n",ret);

	}

	ret = max17043_write_code_burn(0x70,data3,client);
	if (ret) {
		printk( "data3 writing mode,and ret is %d\n",ret);
	}

	return ret;
}

/*10.Restore RCOMP and OCV */
static int max17043_restore(struct i2c_client *client, char *val)
{	
	int ret;
	int i;
	char data[4] ;
	for(i=0;i<4;i++)
	{
		data[i] = val[i];
		printk( "restore data[%d] is %x\n",i,data[i]);
	}
	ret = max17043_write_code_restore(0x0C,data,client);
	if (ret) {
//		printk( "restore writing mode,and ret is %d\n",ret);
		return ret;
	}

	return ret;
}

/*lock Model Access*/
static int max17043_lock_model(struct i2c_client *client)
{
	int ret ;
	ret = max17043_write_code(0x3E,0x0000,client);
	if (ret) {
//		printk( "writing lock  Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}


/*burn mode sequence */
static void max17043_burn(struct i2c_client *client)
{	
	printk("enter burn\n");
	char rt_value[4];
	max17043_ulock_model(client);			//1.unlock model access
	max17043_read_rccmp_ocv(client, rt_value);	//2.read rcomp and ocv register
	max17043_write_ocv(client);			//3.write OCV register
	max17043_write_rccomp(client);			//4.write rcomp to a maximum value of 0xff00
	max17043_write_model(client);			//5.write the model
	mdelay(150);					//6.delay at least 200ms
	max17043_write_ocv(client);			//7.write ocv register
	mdelay(400);					//8. this delay must between 150ms and 600ms
	/*9.read soc register and compare to expected result
	 *if SOC1 >= 0xF9 and SOC1 <= 0xFB then 
	 *mode was loaded successful else was not loaded successful
	 */
	max17043_restore(client, rt_value);		//10.restore rcomp and ocv
	max17043_lock_model(client);			//11.lock model access
}

/*For mode burn writing*/
static int max17043_write_code_burn(u8 reg,char *wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[17];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 17;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<17;i++)
	{
		data[i]=wt_value[i-1];
	}
#if 0	
	for(i=0;i<17;i++)
	{	
		printk( "*********get into max17043 write code burn*************");
		printk( "data[%d] is %x\n",i,data[i]);
	}
#endif
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

/*For mode burn restore*/
static int max17043_write_code_restore(u8 reg,char *wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[5];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 5;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<5;i++)
	{
		data[i]=wt_value[i-1];
		printk( "*****get into write code restore and data[%d] is %x\n",i,data[i]);
	}	
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

/*
 *sysfs for burn the model 
 */
static ssize_t max17043_store_burn(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);		
	unsigned long val = simple_strtoul(buf, NULL, 10);
	burn_state_t state = BURN_ON;
	const char * const *s;
	char *p;
	int len = 0;
	
	p = memchr(buf, '\n', count);
	len = p ? p - buf : count;
	
	for (s = &burn_states[state]; state < 2; s++, state++) 
		{
		if (*s && len == strlen(*s) && !strncmp(buf, *s, len))
			break;
		}
	
	if (val < 0 || val > 1)
		return -EINVAL;	
		
//if (state == BURN_ON)
//	max17043_burn(client);

	return count;
}

static DEVICE_ATTR(burn, S_IWUSR | S_IRUGO,
		   NULL ,max17043_store_burn);

/*
 * SysFS support
 */
static int max17043_set_reset(int val)
{	
	return 0;
}

static int max17043_set_reset_chip(int val)
{	
	int ret;
	int por = POR;
	int str = STR;
	if(0 == val){                                                   // max170444 power on reset
		ret = max17043_battery_write(MAX17043_REG_COMMAND,&por,1,batt_info);
		mdelay(110);
		if (ret) {
		printk( "writing power on reset Register,and ret is %d\n",ret);
		return ret;
		}
	}else if(1 == val){                                                // max17044 quick start
		ret = max17043_battery_write(MAX17043_REG_MODE,&str,1,batt_info);
		if (ret) {
		printk( "writing quick start Register,and ret is %d\n",ret);
		return ret;
		}
	}
	return ret;
}

static ssize_t max17043_store_reset(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);		
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	if (val < 0 || val > 1)
		return -EINVAL;	
	ret = max17043_set_reset(val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(reset, S_IWUSR | S_IRUGO,
		   NULL ,max17043_store_reset);
		   
static int max17043_set_sleep(int val)
{	
	int ret;
	int sleep = SLEEP;
	int wake = WAKE;
	if(0 == val){                                                   // max17043 power on reset
		ret = max17043_battery_write(MAX17043_REG_CONFIG,&sleep,1,batt_info);
		mdelay(110);
		if (ret) {
		printk( "writing power on reset Register,and ret is %d\n",ret);
		return ret;
		}
	}else if(1 == val){                                                // max17043 quick start
		ret = max17043_battery_write(MAX17043_REG_CONFIG,&wake,1,batt_info);
		mdelay(440);
		if (ret) {
		printk( "writing quick start Register,and ret is %d\n",ret);
		return ret;
		}
	}
	return ret;
}

static ssize_t max17043_store_sleep(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);	
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	if (val < 0 || val > 1)
		return -EINVAL;	
	ret = max17043_set_sleep(val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(sleep, S_IWUSR | S_IRUGO,
		    NULL ,max17043_store_sleep);
		    
static struct attribute *max17043_attributes[] = {
	&dev_attr_burn.attr,
	&dev_attr_reset.attr,
	&dev_attr_sleep.attr,
	NULL
};

static const struct attribute_group max17043_attr_group = {
	.attrs = max17043_attributes,
};

static void battery_capacity_detect_timeout(struct work_struct *work)
{
		int batt_capacity = 0;
		int vol_value;
		int temp;
		int cfg;
		
		max17043_power_charger_init(batt_info,batt_info->client->irq,batt_info->client->dev.platform_data);
		batt_capacity = max17043_battery_rsoc(batt_info);
		
		vol_value = max17043_battery_voltage(batt_info);
    vol_value = voltage_value_change(vol_value);
		
	//	if(batt_capacity >= 95)
		if((batt_capacity >= 95)&&(gpio_get_value(S3C64XX_GPM(3)) == 1)&&(gpio_get_value(S3C64XX_GPL(13))==1))
		//  __raw_writel(__raw_readl(S3C_INFORM3)|0x10, S3C_INFORM3);
			tmp_value = 0x10;
		else
			tmp_value = 0x0;
		  
//		printk("****S3C64XX_GPLCON1:0x%x,S3C64XX_GPL10=%d,\n",__raw_readl(S3C64XX_GPLCON1),gpio_get_value(S3C64XX_GPL(13)));

//	schedule_delayed_work(&detect_charge_timeout, HZ * 1);
}

static int max17043_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	char *name;
	struct max17043_device_info *di;
	struct max17043_access_methods *bus;
	int num;
	int retval = 0;
	int value;
	int ret;
	
	printk(KERN_INFO "max17043 :%s start ....\n",__FUNCTION__);

	/* Get new ID for the new battery device */
	retval = idr_pre_get(&battery_id, GFP_KERNEL);
	if (retval == 0)
		return -ENOMEM;
	mutex_lock(&battery_mutex);
	retval = idr_get_new(&battery_id, client, &num);
	mutex_unlock(&battery_mutex);
	if (retval < 0)
		return retval;

//	name = kasprintf(GFP_KERNEL, "bq27200-%d", num);
	name = kasprintf(GFP_KERNEL, "%s", "battery");
	if (!name) {
		dev_err(&client->dev, "failed to allocate device name\n");
		retval = -ENOMEM;
		goto batt_failed_1;
	}

//	di = (struct max17043_device_info *)kzalloc(sizeof(struct max17043_device_info), GFP_KERNEL);
	di = kzalloc(sizeof(struct max17043_device_info), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}
	di->id = num;

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate access method "
					"data\n");
		retval = -ENOMEM;
		goto batt_failed_3;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	di->bat.name = name;
	bus->read = &max17043_read;
	bus->write = &max17043_write;
	di->bus = bus;
	di->client = client;

//	max17043_power_charger_init(di,di->client->irq,di->client->dev.platform_data);
//	charge_state_irq_init(client->dev.platform_data);
//	charge_state_irq_init(di);

	if (retval) {
		dev_err(&client->dev, "failed to register battery\n");
		goto batt_failed_4;
	}
	
	batt_info=di;

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);
	
	
  max17043_battery_read(0x0c, &value,0, di);
  printk("*********0x0c = 0x%x*******\n",value);
  
  value = (value & 0) |0x1c97;
  max17043_battery_write(0x0c,&value,1,di);
	printk("*********write 0x0c = 0x%x*******\n",value);
  
  max17043_battery_read(0x0c,&value,0, di);
  printk("*********0x0c = 0x%x*******\n",value);
  
  charge_full_flag = 0;
  
  ret = sysfs_create_group(&client->dev.kobj, &max17043_attr_group);
	if (ret)
		goto batt_failed_4;
  
	max17043_burn(client);
	msleep(1500);
	max17043_set_reset_chip(0);
	mdelay(100);
	
	INIT_DELAYED_WORK(&detect_charge_timeout, battery_capacity_detect_timeout);
	schedule_delayed_work(&detect_charge_timeout, HZ * 1);
		
	return 0;
	

batt_failed_4:
	kfree(bus);
batt_failed_3:
	kfree(di);
batt_failed_2:
	kfree(name);
batt_failed_1:
	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, num);
	mutex_unlock(&battery_mutex);

	return retval;
}

static int max17043_battery_remove(struct i2c_client *client)
{
	struct max17043_device_info *di = i2c_get_clientdata(client);

	power_supply_unregister(&di->bat);

	kfree(di->bat.name);

	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, di->id);
	mutex_unlock(&battery_mutex);
	cancel_delayed_work_sync(&detect_charge_timeout);

	kfree(di);

	return 0;
}

/*
 * Module stuff
 */

static const struct i2c_device_id max17043_id[] = {
	{ "max17043", 0 },
	{},
};

static struct i2c_driver max17043_battery_driver = {
	.driver = {
		.name = "max17043",
	},
	.probe = max17043_battery_probe,
	.remove = max17043_battery_remove,
	.id_table = max17043_id,
};

static int __init max17043_battery_init(void)
{
	int ret;
	
//	 __raw_writel(0x0, S3C_INFORM3);

	ret = i2c_add_driver(&max17043_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register max17043 driver\n");

	return ret;
}
module_init(max17043_battery_init);

static void __exit max17043_battery_exit(void)
{
	i2c_del_driver(&max17043_battery_driver);
}
module_exit(max17043_battery_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("max17043 monitor driver");
MODULE_LICENSE("GPL");
