/*
 * Battery driver for max8698 PMIC
 *
 * Copyright 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Based on OLPC Battery Driver
 *
 * Copyright 2006  David Woodhouse <dwmw2@infradead.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
//#include <linux/irq.h>

#include <linux/mfd/max8698/core.h>

#include <asm/io.h>
#include <plat/regs-otg.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>

#include <mach/gpio.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach-types.h>


#include <linux/string.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include "../usb/gadget/s3c-udc.h"
struct wake_lock usbinsert_wake_lock;
void settle_system_on(void);

extern void TestLED(void);

static char *charger_exception = "None";
static char *max8698_power_supplied_to[] = {
	"battery",
};

static struct timer_list gasgauge_timeout;

struct work_struct charge_state_irq_work;
struct work_struct pmic_fault_irq_work;
struct work_struct low_battery_irq_work;

static int max8698_bat_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val);
int max8698_get_supplies();

extern void udc_stop_mode();
extern void disable_udc_stop_mode();

int irq_status = 0;
int tmp_value = 0;
static int charger_flash =0 ;
static int global_irq;
static int gpm3_state_value;
int adapter_usb_power_insert_flag = 0;
static struct delayed_work detect_ac_usb_timeout;

static int supply_state_count = 0;
static int var_limit_charger_current = 2;
static enum power_supply_property max8698_ac_props[] = {
//	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
//	POWER_SUPPLY_PROP_BATT_VOL,
};

static enum power_supply_property max8698_bat_props[] = {

	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_BATT_VOL,
	//POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_BATT_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
//	POWER_SUPPLY_PROP_HEALTH,

	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property max8698_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	//POWER_SUPPLY_PROP_BATT_VOL,
};

struct platform_device *global8698_pdev=NULL;
struct max17043_device_info *globalmax8698=NULL;
static void set_orange_flash_light(int value);
static void set_charge_full_led();
static void power0ff_green_led();

#define MAX8698_BATT_SUPPLY	 1
#define MAX8698_USB_SUPPLY	 2
#define MAX8698_LINE_SUPPLY	 4

static int usb_supply_flag = 0;

struct delayed_work usbinsert_work;
struct delayed_work charger_handler_work;
static int global_supplies=MAX8698_BATT_SUPPLY;	/* Henry Li@20101020 fix bug of "couldn't definitely detect adapter source" */
static int global_usb_plug_in=0;		/* Henry Li: 0: plug out;  1: plug in */
static void unlock_function(struct work_struct *work)
{

	wake_unlock(&usbinsert_wake_lock);
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		wake_lock(&usbinsert_wake_lock);
	}
}

int get_global_usb_plug_in(void)
{
	return global_usb_plug_in;
}
EXPORT_SYMBOL_GPL(get_global_usb_plug_in);

void adapter_usb_check_process()
{
	printk("****&&&&adapter_usb_power_insert_flag = %d\n",adapter_usb_power_insert_flag);
	if(adapter_usb_power_insert_flag == 2)
	{
		adapter_usb_power_insert_flag = 0;
		max8698_get_supplies();
		msleep(10);
	}
}

int detect_usb_ac_charge()
{
#if 0	/* Henry Li@20101020 fix bug of "couldn't definitely detect adapter source" */
	u16 val;
	int ret = 0;
	//hprt judge usb or adapter supply
	
	//printk("********************detect if not usb or ac insert********************\n");


	//if(gpio_get_value(S3C64XX_GPM(3)) == 1)
  	//wake_lock(&usbinsert_wake_lock);
	//else

	if(gpio_get_value(S3C64XX_GPM(3)) == 0)
	{
	if(wake_lock_active(&usbinsert_wake_lock)>0)
	schedule_delayed_work(&usbinsert_work,HZ*15);	
	}
	
	
	msleep(200);
	#define PORT_LINE_STATUS_MASK	(0x3 << 10)
	val = readl(S3C_UDC_OTG_HPRT) & PORT_LINE_STATUS_MASK;
  
	//printk("Enter %s, what supply(0x%x),reg_hprt=0x%x\n", __FUNCTION__, val,readl(S3C_UDC_OTG_HPRT));
	if(val && (val == PORT_LINE_STATUS_MASK))
	{ 
	  ret = MAX8698_LINE_SUPPLY;
	} 
	else if(val == 0x400)
	{
//		printk("Enter %s, USB supply(0x%x)\n", __FUNCTION__, val);
		ret = MAX8698_USB_SUPPLY;
	}
	else if(val == 0x0)
	{
	//	printk("Enter %s, battery supply(0x%x)\n", __FUNCTION__, val);
		ret = MAX8698_BATT_SUPPLY;
	}
	return ret;
#endif
	//if(gpio_get_value(S3C64XX_GPM(3)) == 1)
  	//wake_lock(&usbinsert_wake_lock);
	//else

	//if(gpio_get_value(S3C64XX_GPM(3)) == 0)
	if (global_usb_plug_in == 0)
	{
	if(wake_lock_active(&usbinsert_wake_lock)>0)
	schedule_delayed_work(&usbinsert_work,HZ*15);	
	//schedule_delayed_work(&usbinsert_work,HZ*6);	
	}

 	return global_supplies;		/* Henry Li@20101020 fix bug of "couldn't definitely detect adapter source" */
}

static int max8698_charger_config(int supplies)   ///Config charge current 
{
	int ret = 0;
	unsigned long cfg;
	if(supplies & MAX8698_LINE_SUPPLY)
	 {
		/********************************* 
		         Current_limit  
		             1            
		         GPK6 set 1   
		*********************************/ 
	gpio_set_value(S3C64XX_GPK(6), 1);
	s3c_gpio_cfgpin(S3C64XX_GPK(6),S3C64XX_GPK_OUTPUT(6));	
	s3c_gpio_setpull(S3C64XX_GPK(6), S3C_GPIO_PULL_UP);
	
	
    ret = 750;
    
	 }
	else
	 {
		/********************************* 
		         Current_SET1 
		             0       
		          GPK6 set 0 
		*********************************/
  gpio_set_value(S3C64XX_GPK(6), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(6),S3C64XX_GPK_OUTPUT(6));	
	s3c_gpio_setpull(S3C64XX_GPK(6), S3C_GPIO_PULL_DOWN);
	
    
    ret = 500;
	 }
	
	return ret;
}

void limit_charger_current(void)   ///with 3g module //20100121
{
	#if 0
       if ( 0 == var_limit_charger_current)
	   	return;
	if(global_supplies == MAX8698_LINE_SUPPLY)
	 {
		/********************************* 
		         Current_SET1 
		             0       
		          GPK6 set 0 
		*********************************/
  	gpio_set_value(S3C64XX_GPK(6), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(6),S3C64XX_GPK_OUTPUT(6));	
	s3c_gpio_setpull(S3C64XX_GPK(6), S3C_GPIO_PULL_DOWN);
	var_limit_charger_current = 0;	
	 }	
#endif
}
EXPORT_SYMBOL_GPL(limit_charger_current);

void normal_charger_current(void)   ///with 3g module //20100121
{
       if ( 1 == var_limit_charger_current)
	   	return;
	if(global_supplies == MAX8698_LINE_SUPPLY)
	 {
		/********************************* 
		         Current_limit  
		             1            
		         GPK6 set 1   
		*********************************/ 
	gpio_set_value(S3C64XX_GPK(6), 1);
	s3c_gpio_cfgpin(S3C64XX_GPK(6),S3C64XX_GPK_OUTPUT(6));	
	s3c_gpio_setpull(S3C64XX_GPK(6), S3C_GPIO_PULL_UP);
	var_limit_charger_current = 1;	
	 }	
}
EXPORT_SYMBOL_GPL(normal_charger_current);
/* Henry Li@20101020 fix bug of "couldn't definitely detect adapter source" */
/* Henry Li: detect usb/adapter (ac) while usb inserting */
extern struct clk	*otg_clock;
extern int udc_cable_status;
static int really_get_supplies_first=0;
extern bool get_sierra_module_status(void);
void really_get_supplies(void)
{
	//struct max17043_device_info *max8698 = platform_get_drvdata(global8698_pdev);
	int supplies=MAX8698_BATT_SUPPLY;
	u16 state;
	struct platform_device *pdev=global8698_pdev;
	//struct clk	*otg_clock = NULL;
	u32 uTemp;	
	u16 counter=0;


	 if ((gpio_get_value(S3C64XX_GPM(3)) == 0) || (pdev == NULL))
	 	{
	 	global_supplies = supplies;
 		return;
	 	}
	 if  (udc_cable_status ==STATUS_UDC_CONNECT)
	 	{
	 	global_supplies = MAX8698_USB_SUPPLY;
 		return;
	 	}
	int retval = 0;
	if (really_get_supplies_first == 1)
          while (udc_cable_status == STATUS_UDC_GADGET_DRIVER_INSTALL)
       	{
       	msleep(1);
		retval++;
		if (retval > 250)		// 5*50*20ms
			break;
		if (gpio_get_value(S3C64XX_GPM(3)) == 0)
			{
			global_supplies = supplies;
			return; 
			}
       	}		 

	retval = 0;
       while (udc_cable_status == STATUS_UDC_UNINSTALL)
       	{
       	msleep(1);
		retval++;
		if (retval > 50)
			break;
		if (gpio_get_value(S3C64XX_GPM(3)) == 0)
			{
			global_supplies = supplies;
			return; 
			}
       	}
		  
       if  (udc_cable_status ==STATUS_UDC_CONNECT)
	 	{
	 	global_supplies = MAX8698_USB_SUPPLY;
 		return;
	 	}
       else
	udc_cable_status=STATUS_UDC_GET_SUPPLIES;	

	int savevalue=readl(S3C_USBOTG_PHYPWR);
	if (otg_clock == NULL) {	 
	otg_clock = clk_get(&pdev->dev, "otg");
	if (IS_ERR(otg_clock)) {
		printk(KERN_INFO "failed to find otg clock source\n");
		return -ENOENT;
	}	
	clk_enable(otg_clock);
		}
	uTemp = readl(S3C_OTHERS);
	uTemp |= (1<<16);   // USB_SIG_MASK
	writel(uTemp, S3C_OTHERS);		/* 
	                                				USB signal mask to prevent unwanted leakage
	                                				(This bit must set before USB PHY is used.)  
					                             */
	
	// Initializes OTG Phy.
	writel(0x0, S3C_USBOTG_PHYPWR);			//OTG POWERUP
	writel(/*0x20*/0x2, S3C_USBOTG_PHYCLK);   //external crystal 12 MHz
	writel(0x1, S3C_USBOTG_RSTCON);			// reset must be asserted for at least 10us
	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
	//  Soft-reset OTG Core and then unreset again.
	writel(CORE_SOFT_RESET, S3C_UDC_OTG_GRSTCTL);
	counter = 0;
	do
	{
	uTemp=readl(S3C_UDC_OTG_GRSTCTL);
	counter++;
	udelay(1000);
	if (counter > 500)
		break;
	}
	while(!(uTemp & AHB_MASTER_IDLE));
	
	u32	usb_status;
/*	
		usb_status = readl(S3C_UDC_OTG_GOTGCTL);
		if (usb_status & (B_SESSION_VALID|A_SESSION_VALID))
			{
  			 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
				supplies = MAX8698_USB_SUPPLY;
			 else
				supplies = MAX8698_BATT_SUPPLY;			 				 			
			}
		else
			{
  			 if (gpio_get_value(S3C64XX_GPM(3)) == 1)			
				supplies = MAX8698_LINE_SUPPLY;								
			}
*/			
	 //printk("--2--usb_status=0x%x the supplies = %d  gpio_get_value(S3C64XX_GPM(3)=0x%x\n",usb_status, supplies, gpio_get_value(S3C64XX_GPM(3)));

	#define PORT_LINE_STATUS_MASK	(0x3 << 10)
	usb_status = readl(S3C_UDC_OTG_HPRT) & PORT_LINE_STATUS_MASK;
  
	//printk("Enter %s, what supply(0x%x),reg_hprt=0x%x\n", __FUNCTION__, usb_status,readl(S3C_UDC_OTG_HPRT));
	if(usb_status && (usb_status == PORT_LINE_STATUS_MASK))
	{ 
		 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
			  supplies = MAX8698_LINE_SUPPLY;
	} 
	else if(usb_status == 0x400)
	{
//		printk("Enter %s, USB supply(0x%x)\n", __FUNCTION__, val);
  			 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
				supplies = MAX8698_USB_SUPPLY;
	}
	else if(usb_status == 0x0)
	{
	//	printk("Enter %s, battery supply(0x%x)\n", __FUNCTION__, val);
		 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
				supplies = MAX8698_USB_SUPPLY;
		else
			supplies = MAX8698_BATT_SUPPLY;
	}
	//  Soft-reset OTG Core and then unreset again.
	writel(CORE_SOFT_RESET, S3C_UDC_OTG_GRSTCTL);
	counter = 0;
	do
	{
	uTemp=readl(S3C_UDC_OTG_GRSTCTL);
	counter++;
	udelay(1000);
	if (counter > 500)
		break;
	}
	while(!(uTemp & AHB_MASTER_IDLE));
	
	 printk("--3--usb_status=0x%x the supplies = %d  gpio_get_value(S3C64XX_GPM(3)=0x%x\n",usb_status, supplies, gpio_get_value(S3C64XX_GPM(3)));
	//writel(readl(S3C_USBOTG_PHYPWR)|(0x7<<1), S3C_USBOTG_PHYPWR);
	/* Henry@2010-12-28 --> leave savevalue as before
	    Otherwise device will crash sometimes while 3g is running and g_file_storage insert/remove and USB in */
	if (get_sierra_module_status() == false)	
		{
		writel(savevalue |(0x19) , S3C_USBOTG_PHYPWR);
		}
	else
		writel(savevalue, S3C_USBOTG_PHYPWR);
		
	//clk_disable(otg_clock);
	global_supplies = supplies;
	udc_cable_status=STATUS_UDC_NOT_INSTALL;
	really_get_supplies_first=1;	
}
void set_global_supplies(int value)
{
	global_supplies=value;
}
EXPORT_SYMBOL_GPL(set_global_supplies);
int  get_global_supplies(void)	//20100121 //4: MAX8698_LINE_SUPPLY
{
	return global_supplies;
}
EXPORT_SYMBOL_GPL(get_global_supplies);

int max8698_get_supplies()  //get charge source and adjust charge current 
{
	int supplies = 0;
	 supplies = detect_usb_ac_charge();
	 printk("the supplies = %d\n",supplies);

	return max8698_charger_config(supplies); //William 20101106
}

static int max8698_batt_status()
{
	u16 state = 0;
//	printk("#######irq_status = %d,gpm3_state_value=%d#######\n",irq_status,gpm3_state_value);

  if(gpio_get_value(S3C64XX_GPM(3)) == 1)
  {
   state = gpio_get_value(S3C64XX_GPL(13));
   
//   printk("****GPL13 = 0x%x\n",__raw_readl(S3C64XX_GPLCON1));

	 switch (state)
	 {
	  case CHG_STATUS_OFF:
		 return POWER_SUPPLY_STATUS_DISCHARGING;

	  case CHG_STATUS_CHARGE:
		 return POWER_SUPPLY_STATUS_CHARGING;
   }
  } 
  else
  {
	 return POWER_SUPPLY_STATUS_DISCHARGING;
	}
}

static void max8698_gasgauge_timeout(unsigned long arg)
{
	
	struct power_supply *battery = (struct power_supply *)arg;
	int tmp;
	charger_flash++;
	
	if (really_get_supplies_first == 0)
		{
		 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
		 	{
			global_usb_plug_in = 1;
			unlock_function(&usbinsert_work);
		 	}
		 else
			global_usb_plug_in = 0;	 			
		really_get_supplies();	
		really_get_supplies();	
		really_get_supplies();	
//		max8698_get_supplies();
		max8698_charger_config(global_supplies);
		}
		//printk(">>>>>>>>>>>max8698_gasgauge_timeout,gpkcon0=0x%x,gpkdat=0x%x,gpk6=%d******\n",readl(S3C64XX_GPKCON),readl(S3C64XX_GPKDAT),gpio_get_value(S3C64XX_GPK(6)));

//	tmp = __raw_readl(S3C_INFORM3)&0x10;
	
	if((gpio_get_value(S3C64XX_GPM(3)) == 1)&&(tmp_value != 0x10))
	{/**when usb in and orange led flash close green led**/
		    power0ff_green_led();
		set_orange_flash_light(charger_flash);
		mod_timer(&gasgauge_timeout, jiffies+HZ *1);
	}
	else if((gpio_get_value(S3C64XX_GPM(3)) == 1)&&(tmp_value ==0x10))
	{/**when usb in and orange led light close green led**/
	        power0ff_green_led();
		//	del_timer_sync(&gasgauge_timeout);
			set_charge_full_led();
	//		tmp_value = 0;
			mod_timer(&gasgauge_timeout, jiffies+HZ *3);
	}
	else
	{
		mod_timer(&gasgauge_timeout, jiffies+HZ *3);
	}
}

static ssize_t charger_current_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct max8698 *max8698 = dev_get_drvdata(dev);
	struct max8698_charger_policy *policy = max8698->power.policy;
	unsigned long fast_limit_mA;

	fast_limit_mA = simple_strtoul(buf, NULL, 0);
	
	return 0;

}

static ssize_t charger_current_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max8698 *max8698 = dev_get_drvdata(dev);
	u16 val;
	int fast_limit_mA = 0;

//	val = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2);
//	fast_limit_mA = (val & 0xf) * 50;

	return sprintf(buf, "%d\n", fast_limit_mA);
}

static DEVICE_ATTR(charger_current, S_IRUGO | S_IWUSR, charger_current_show, charger_current_set);

static ssize_t charger_exception_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%s\n", charger_exception);
	charger_exception = "None";
	return ret;
}

static DEVICE_ATTR(charger_exception, 0444, charger_exception_show, NULL);

static ssize_t charger_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	
//	struct max8698 *max8698 = dev_get_drvdata(dev);
	char *charge = "Charging";
	int state;
 
  state = gpio_get_value(S3C64XX_GPN(0));


	return sprintf(buf, "%s\n", charge);
}

static ssize_t charger_state_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
//	struct max8698 *max8698 = dev_get_drvdata(dev);
	int len;
	u16 val;
		return -EINVAL;
}

static DEVICE_ATTR(charger_state, 0644, charger_state_show, charger_state_set);


static ssize_t charger_charger_debug_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
//	struct max8698 *max8698 = dev_get_drvdata(dev);
	
	
	return n;
}

static ssize_t charger_charger_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("chager debug command list:\n");
	printk("debug_on - show charger infomation per 30sec\n");
	printk("debug_off - disable show charger info\n");
	printk("led_off - disable led's light\n");
	return 1;
}
static DEVICE_ATTR(charger_debug, 0666, charger_charger_debug_show, charger_charger_debug_set);

/*********************************************************************
 *		AC Power
 *********************************************************************/
static int max8698_ac_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max17043_device_info *max8698 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;
	
//	printk(KERN_INFO "****%s start ....*****\n",__FUNCTION__);
//	really_get_supplies();

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			//	if(detect_usb_ac_charge() == MAX8698_LINE_SUPPLY)
			if(global_supplies == MAX8698_LINE_SUPPLY)
          val->intval = 1;
         else
         val->intval = 0; 
			break;
		default:
			ret = -EINVAL;
			break;	
	}
	return ret;
}

static int max8698_bat_check_health()
{
	u16 reg;
	union power_supply_propval val = {0,};

   max17043_battery_get_property(POWER_SUPPLY_PROP_BATT_TEMP,&val);
	if(val.intval > 46)
	{
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	}
	else if(val.intval < 0)
	{
		return POWER_SUPPLY_HEALTH_COLD;
	}
	else
	{
		return POWER_SUPPLY_HEALTH_GOOD; 
	}
	
}

static int max8698_bat_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max17043_device_info *max8698 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;
	int batt_supply;
	
//	printk(KERN_INFO "*****%s start ....******\n",__FUNCTION__);

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = max8698_batt_status();
			break;
		case POWER_SUPPLY_PROP_ONLINE:
					if(gpio_get_value(S3C64XX_GPM(3)) == 0)
				 val->intval = 1;
				else
				 val->intval = 0;
			break;
		default:
			ret = max17043_battery_get_property(psp,val);
			break;
	}
	
	return ret;
}

static int max8698_usb_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max17043_device_info *max8698 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;
	
	msleep(100);
	
	if((gpio_get_value(S3C64XX_GPM(3)) == 1) && (supply_state_count == 4))
	{
		supply_state_count = 0;
		usb_supply_flag = 1;
	}
	else if(gpio_get_value(S3C64XX_GPM(3)) == 0)
		supply_state_count = 0;
	else
		supply_state_count++;
		

	//printk(KERN_INFO "*****%s start ....global_supplies = %d....,usb_supply= %d....******\n",__FUNCTION__,global_supplies,usb_supply_flag);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
				if((gpio_get_value(S3C64XX_GPM(3)) == 1)&&(global_supplies == MAX8698_USB_SUPPLY)&& (usb_supply_flag == 1))
          val->intval = 1;
         else
         {
         	val->intval = 0;
         	usb_supply_flag = 0;
         }
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static void set_orange_flash_light(int value)
{
	unsigned long cfg;
   if(value % 2 == 1)
  {
  	
  gpio_set_value(S3C64XX_GPK(1), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_DOWN);
	
  }
  else
  {
  gpio_set_value(S3C64XX_GPK(1), 1);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_UP);
	
  }
  if(value == 100)
    charger_flash = 0;
  
}

static void set_charge_full_led()
{
	del_timer_sync(&gasgauge_timeout);
	/*charge flash led power off->orange*/
	gpio_set_value(S3C64XX_GPK(1), 1);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_UP);
	
}

static void power0ff_green_led()
{
		gpio_set_value(S3C64XX_GPK(0), 0);
		s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
		s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);
}
static void powerOn_green_led()
{
		gpio_set_value(S3C64XX_GPK(0), 1);
		s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(1));
		s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_UP);
}

static void set_charge_led_poweroff()
{
	gpio_set_value(S3C64XX_GPK(1), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_DOWN);
/*	
	gpio_set_value(S3C64XX_GPK(0), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));	
	s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);
	*/
}

void sleep_led()
{
	gpio_set_value(S3C64XX_GPK(1), 1);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_UP);
	
	msleep(200);
	
	gpio_set_value(S3C64XX_GPK(1), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_DOWN);
	msleep(200);
	
	gpio_set_value(S3C64XX_GPK(1), 1);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_UP);
	
	msleep(200);
	
	gpio_set_value(S3C64XX_GPK(1), 0);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_DOWN);
	
	msleep(200);
	gpio_set_value(S3C64XX_GPK(1), 1);		
	s3c_gpio_cfgpin(S3C64XX_GPK(1),S3C64XX_GPK_OUTPUT(1));	
	s3c_gpio_setpull(S3C64XX_GPK(1), S3C_GPIO_PULL_UP);
	
}

static void charger_handler_work_function(struct work_struct *work);
static void max8698_charger_handler(struct max17043_device_info *max8698, int irq, void *data)
{
#if 0
	struct max17043_power *power = &max8698->power;
	unsigned long cfg;
	int supplies;
	u16 state;
	int charge_state_value;

	printk(KERN_INFO "*****%s ,irq:%d ..\n",__FUNCTION__,irq);
	
	switch (irq) 
	{
		case MAX8698_IRQ_DETECT_POWER:
			dev_info(max8698->dev, "*******irq enter....*******\n");
					power0ff_green_led();
					msleep(200);
	//			printk("******detect value = %d******\n",detect_usb_ac_charge());
				 state = gpio_get_value(S3C64XX_GPM(3));
				 charge_state_value = gpio_get_value(S3C64XX_GPL(13));	

	//			 printk("*****the state is %d******\n",state);
	//		if((state == 1)&&(charge_state_value == 0))
			if(state == 1)
			{
				printk("*****the state is %d******\n",state);
				
					mod_timer(&gasgauge_timeout, jiffies+ HZ * 1);
					if(adapter_usb_power_insert_flag == 0)
				 		{
				 			//disable_udc_stop_mode();
							msleep(2);
							udc_stop_mode();
				 			max8698_get_supplies();
							disable_udc_stop_mode();
				 		}
				 	else
				 	//  adapter_usb_check_process();
					{
					 disable_udc_stop_mode();
				 	 schedule_delayed_work(&detect_ac_usb_timeout, HZ * 2);
                    }
				 	irq_status = 1;
			}
			else if(state == 0)
			{
				printk("*****the state is %d******\n",state);
				msleep(200);
			  	max8698_get_supplies();
			  	del_timer_sync(&gasgauge_timeout);
			  	set_charge_led_poweroff();

			  	irq_status = 0;
			}
			break;
	}
#else

	 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
		global_usb_plug_in = 1;
	 else
		global_usb_plug_in = 0;	 	
       global_irq = irq;
       globalmax8698 = max8698;  
	 //if (gpio_get_value(S3C64XX_GPM(3)) == 1)
	 if (global_usb_plug_in == 1)
	 	{
	 		wake_lock(&usbinsert_wake_lock);
		      writel(0xffffffff, S3C_UDC_OTG_GINTSTS);
	 	cancel_delayed_work(&charger_handler_work);
		schedule_delayed_work(&charger_handler_work,HZ*3/2);
//		schedule_delayed_work(&charger_handler_work,HZ*1);
		if (udc_cable_status != STATUS_UDC_CONNECT)
  	       	udc_cable_status=STATUS_UDC_GET_SUPPLIES;	
	 	}
	 else
	 	{
	      charger_handler_work_function(&charger_handler_work);
/* Konstantin required:  Green LED should be lit when USB cable removed -- Henry*/
		powerOn_green_led();
		      writel(0xffffffff, S3C_UDC_OTG_GINTSTS);
	 	}
#endif
}
/* Henry Li: detect usb/adapter (ac) while usb inserting */
static void charger_handler_work_function(struct work_struct *work)
{
	struct max17043_device_info *max8698;		// = platform_get_drvdata(global8698_pdev);
	u16 state;
	u32 uTemp;	
	int charge_state_value;

	if (globalmax8698 != NULL)
		max8698 = globalmax8698;
	else if ( global8698_pdev != NULL)
		max8698 = platform_get_drvdata(global8698_pdev);
	else
		{
		printk("--charger_handler_work_function-- Abnormal Exit \n");
		return;
		}

	really_get_supplies();
	if ( MAX8698_USB_SUPPLY ==  global_supplies )
		really_get_supplies();
	if ( MAX8698_USB_SUPPLY ==  global_supplies )
		really_get_supplies();

	switch (global_irq) 
	{
		case MAX8698_IRQ_DETECT_POWER:
			dev_info(max8698->dev, "*******irq enter....*******\n");
		//			power0ff_green_led();
					msleep(200);
	//			printk("******detect value = %d******\n",detect_usb_ac_charge());
				 state = gpio_get_value(S3C64XX_GPM(3));
				 charge_state_value = gpio_get_value(S3C64XX_GPL(13));	

				 //printk("*****the state is %d charge_state_value=%d******\n",state, charge_state_value);
	
		//	if((state == 1)&&(charge_state_value == 0))
		if(state == 1)
			{
				printk("*****the state is %d******\n",state);
				
				max8698_get_supplies();
					mod_timer(&gasgauge_timeout, jiffies+ HZ * 1);

				 	irq_status = 1;
			}

			else if(state == 0)
			{
				printk("*****the state is %d******\n",state);
				msleep(200);
			  	max8698_get_supplies();
			  	del_timer_sync(&gasgauge_timeout);  //if not usb plug-in, delete gas gauge timer
			  	set_charge_led_poweroff();

			  	irq_status = 0;
			}
			break;
	}
		cancel_delayed_work(&charger_handler_work);
}


void max8698_init_charger(struct max17043_device_info *max8698)
{
	unsigned long cfg;
	int supplies;
	
//	printk(KERN_INFO "<<<<$$$<<<$<<<$<<$>>%s start ..\n",__FUNCTION__);
	INIT_DELAYED_WORK(&charger_handler_work,charger_handler_work_function);		//William add 20101106	
  supplies = max8698_get_supplies();
  if(supplies)
  {
	  if(supplies == 500)
	  {
		 printk("can use 500 mA charge!\n");
	  }
	  else if(supplies == 750)
	  {
	 	 printk("can use 750 mA charge!\n");
	  }
  }
 //init BAT_NTC 
 
 gpm3_state_value = gpio_get_value(S3C64XX_GPM(3));
 
// printk("$$$$>>>>>>>>>>>gpm3_state_value = %d<<<<<<<<<<<<<<<<\n",gpm3_state_value);
	
	if(gpm3_state_value == 1)
	 irq_status = 1;
	
	init_timer(&gasgauge_timeout);
	gasgauge_timeout.function = max8698_gasgauge_timeout;
//	gasgauge_timeout.data = (unsigned long)battery;

 max17043_register_irq(max8698, MAX8698_IRQ_DETECT_POWER,
			max8698_charger_handler, NULL);
			
 //&*&*&*AL1_20100206, check chager supply after bootup system
	 mod_timer(&gasgauge_timeout, jiffies+5*HZ);
//&*&*&*AL2_20100206, check chager supply after bootup system

}
#if 1
static void charge_state_worker(struct work_strut *work)
{
	struct max17043_device_info *max17043 = container_of(work, struct max17043_device_info, irq_work);
//	struct max17043_platform_data *pdata = max17043->client->dev.platform_data;
	int charge_state_value;
	int gpm3_value;
	
	msleep(200);
	charge_state_value = gpio_get_value(S3C64XX_GPL(13));	
	gpm3_value = gpio_get_value(S3C64XX_GPM(3));
		printk("$$$$gpl13 = %d,gpm3_value = %d\n",charge_state_value,gpm3_value);

}

static void low_battery_worker(struct work_strut *work)
{
	struct max17043_device_info *max17043 = container_of(work, struct max17043_device_info, irq_work);
//	struct max17043_platform_data *pdata = max17043->client->dev.platform_data;
	int low_battery_value;
	
	low_battery_value = gpio_get_value(S3C64XX_GPN(9));
	
}
static void pmic_fault_worker(struct work_strut *work)
{
	struct max17043_device_info *max17043 = container_of(work, struct max17043_device_info, irq_work);
//	struct max17043_platform_data *pdata = max17043->client->dev.platform_data;
	int pmic_fault_value;
	
	pmic_fault_value = gpio_get_value(S3C64XX_GPN(0));
	printk("gpn0 (pmic_fault_value)= %d\n",pmic_fault_value);
}

static irqreturn_t charge_state_irq_handle(int irq, void *data)
{
//	struct max17043_device_info *max17043 = data;
	struct max17043_platform_data *pdata = data;

	printk( "*****irq:%d ..\n",irq);
	disable_irq_nosync(irq);
	schedule_work(&charge_state_irq_work);

	return IRQ_HANDLED;
}

static irqreturn_t low_battery_irq_handle(int irq, void *data)
{
//	struct max17043_device_info *max17043 = data;

	disable_irq_nosync(irq);
	schedule_work(&low_battery_irq_work);

	return IRQ_HANDLED;
}
static irqreturn_t pmic_fault_irq_handle(int irq, void *data)
{
//	struct max17043_device_info *max17043 = data;

	disable_irq_nosync(irq);
	schedule_work(&pmic_fault_irq_work);

	return IRQ_HANDLED;
}

int charge_state_irq_init(struct max17043_device_info *max17043,struct max17043_platform_data *pdata)
{
	//struct max17043_platform_data *pdata = max17043->client->dev.platform_data;
	int ret;
	
	printk("gpl13 = 0x%x,pdata->charge_state_irq=%d\n",gpio_get_value(S3C64XX_GPL(13)),pdata->charge_state_irq);
//	mutex_init(&pdata->irq_mutex);
	INIT_WORK(&charge_state_irq_work,charge_state_worker);
	
	INIT_WORK(&low_battery_irq_work,low_battery_worker);
	
	INIT_WORK(&pmic_fault_irq_work,pmic_fault_worker);
	
			//charge state irq	
	/* Henry Li: guanratee GPM3 ext int. both edge trigger */
	 int value= __raw_readl(S3C64XX_EINT0CON1) & ~(0x7 << 20);	
        value = value | (S3C64XX_EXTINT_BOTHEDGE << 20);
	  __raw_writel(value, S3C64XX_EINT0CON1);
			
		ret = request_irq(pdata->charge_state_irq, charge_state_irq_handle, 0,//IRQF_DISABLED |IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				  "charge_state", max17043);
		if (ret != 0) 
		{
			printk("Failed to request IRQ: %d\n",ret);
			goto err;
		}
		
		//low_battery irq 
		ret = request_irq(pdata->low_battery_irq, low_battery_irq_handle, 0,//IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				  "low_battery", max17043);
		if (ret != 0) 
		{
			printk("Failed to request IRQ: %d\n",ret);
			goto err;
		}
		ret = request_irq(pdata->pmic_fault_irq, pmic_fault_irq_handle, 0,//IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				  "pmic_fault", max17043);
		if (ret != 0) 
		{
			printk("Failed to request IRQ: %d\n",ret);
			goto err;
		}
		
		return 0;
	err:
  return ret;
}
#endif

static __devinit int max8698_power_probe(struct platform_device *pdev)
{
	struct max17043_device_info *max17043 = platform_get_drvdata(pdev);
	struct max17043_power *power = &max17043->power;
//	struct max17043_platform_data *pdata = max17043->client->dev.platform_data;

	struct power_supply *usb = &power->usb;
	struct power_supply *battery = &power->battery;
	struct power_supply *ac = &power->ac;
	
	int ret;

	printk(KERN_INFO "*****%s jay start ..\n",__FUNCTION__);
	global8698_pdev = pdev;	
	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->properties = max8698_ac_props;
	ac->num_properties = ARRAY_SIZE(max8698_ac_props);
	ac->get_property = max8698_ac_get_prop;
	ret = power_supply_register(&pdev->dev, ac);
	if (ret)
		return ret;

	battery->name = "battery";
	battery->type = POWER_SUPPLY_TYPE_BATTERY;		//William 20101105
	battery->properties = max8698_bat_props;
	battery->num_properties = ARRAY_SIZE(max8698_bat_props);
	battery->get_property = max8698_bat_get_property;
//	battery->external_power_changed = set_charge_led;
	battery->use_for_apm = 1;
	battery->num_supplicants = ARRAY_SIZE(max8698_power_supplied_to);
	battery->supplied_to = max8698_power_supplied_to;
	ret = power_supply_register(&pdev->dev, battery);
	if (ret)
		goto battery_failed;
		
	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = max8698_usb_props;
	usb->num_properties = ARRAY_SIZE(max8698_usb_props);
	usb->get_property = max8698_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto usb_failed;

	ret = device_create_file(&pdev->dev, &dev_attr_charger_exception);
	if (ret < 0)
		dev_warn(max17043->dev, "failed to add charge exception sysfs: %d\n", ret);

	ret = device_create_file(&pdev->dev, &dev_attr_charger_state);
	if (ret < 0)
		dev_warn(max17043->dev, "failed to add charge state sysfs: %d\n", ret);

	ret = device_create_file(&pdev->dev, &dev_attr_charger_current);
	if (ret < 0)
		dev_warn(max17043->dev, "failed to add charge current sysfs: %d\n", ret);
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
	ret = device_create_file(&pdev->dev, &dev_attr_charger_debug);
	if (ret < 0)
		dev_warn(max17043->dev, "failed to add charge debug sysfs: %d\n", ret);
//&*&*&*AL2_20100206, Add debug mode for trace charger issue
	ret = 0;
	
		really_get_supplies();
	
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		wake_lock(&usbinsert_wake_lock);
	}

/*
	gpm3_state_value = gpio_get_value(S3C64XX_GPM(3));
	
	if(gpm3_state_value == 1)
	 irq_status = 1;
	
	init_timer(&gasgauge_timeout);
	gasgauge_timeout.function = max8698_gasgauge_timeout;
//	gasgauge_timeout.data = (unsigned long)battery;

	max8698_init_charger(max17043);
	
 //&*&*&*AL1_20100206, check chager supply after bootup system
	 mod_timer(&gasgauge_timeout, jiffies+5*HZ);
//&*&*&*AL2_20100206, check chager supply after bootup system

	printk("*****%s start ..\n",__FUNCTION__);
*/	

//	 if (gpio_get_value(S3C64XX_GPM(3)) == 1)
//		schedule_delayed_work(&charger_handler_work,1*HZ);
	really_get_supplies_first=0;
	return ret;

usb_failed:
	power_supply_unregister(battery);
battery_failed:
	power_supply_unregister(ac);
	return ret;
}

static __devexit int max8698_power_remove(struct platform_device *pdev)
{
	struct max17043_device_info *max8698 = platform_get_drvdata(pdev);
	struct max17043_power *power = &max8698->power;

	device_remove_file(&pdev->dev, &dev_attr_charger_state);
	device_remove_file(&pdev->dev, &dev_attr_charger_exception);
	device_remove_file(&pdev->dev, &dev_attr_charger_current);
	power_supply_unregister(&power->battery);
	power_supply_unregister(&power->ac);
	power_supply_unregister(&power->usb);
	
	del_timer_sync(&gasgauge_timeout);
	return 0;
}

static int max8698_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(">>>>>>>>>>>>>>>max8698_power_resume gpm3 = %d\n",gpio_get_value(S3C64XX_GPM(3)));
	set_charge_led_poweroff();
	return 0;
}	
static int max8698_power_resume(struct platform_device *pdev)
{
	int supplies;
	
//	printk(">>>>>>>>>>>>>>>max8698_power_resume gpm3 = %d\n",gpio_get_value(S3C64XX_GPM(3)));
	powerOn_green_led();
	
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		
		//wake_lock_timeout(&usbinsert_wake_lock,HZ*10);
		power0ff_green_led();
		wake_lock(&usbinsert_wake_lock);
		
		really_get_supplies_first = 0;
		mod_timer(&gasgauge_timeout, jiffies+HZ *1);
		irq_status = 1;
	}
	return 0;
}
EXPORT_SYMBOL(usbinsert_wake_lock);

static struct platform_driver max8698_power_driver = {
	.probe = max8698_power_probe,
	.remove = __devexit_p(max8698_power_remove),
	.suspend = max8698_power_suspend,
	.resume = max8698_power_resume,
	.driver = {
		.name = "max8698-power",
	},
};

static int __init max8698_power_init(void)
{
	wake_lock_init(&usbinsert_wake_lock, WAKE_LOCK_SUSPEND, "usbinsert_delayed_work");
	INIT_DELAYED_WORK(&usbinsert_work,unlock_function);
	//INIT_DELAYED_WORK(&charger_handler_work,charger_handler_work_function);  //William 20101106 cancel 

	return platform_driver_register(&max8698_power_driver);
}
module_init(max8698_power_init);

static void __exit max8698_power_exit(void)
{
	platform_driver_unregister(&max8698_power_driver);
}
module_exit(max8698_power_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Power supply driver for MAX8698");
MODULE_ALIAS("platform:max8698-power");
