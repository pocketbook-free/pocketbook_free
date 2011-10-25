/*
 * Battery driver for wm8350 PMIC
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
#include <linux/mfd/wm8350/supply.h>
#include <linux/mfd/wm8350/core.h>
#include <linux/mfd/wm8350/comparator.h>

#include <asm/io.h>
#include <plat/regs-otg.h>
#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-m.h>

//&*&*&*HC1_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code
//#include <plat/regs-oldstyle-gpio.h> // ghcstop
//&*&*&*HC2_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code


#include <linux/string.h>
#include "bq27x00_battery.h"

static char *charger_exception = "None";
static char *wm8350_power_supplied_to[] = {
	"battery",
};

static struct timer_list gasgauge_timeout;
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
static int enable_debug = 0;
//&*&*&*AL2_20100206, Add debug mode for trace charger issue

static int wm8350_read_battery_uvolts(struct wm8350 *wm8350)
{
	return wm8350_read_auxadc(wm8350, WM8350_AUXADC_BATT, 0, 0)
		* WM8350_AUX_COEFF;
}

static int wm8350_read_line_uvolts(struct wm8350 *wm8350)
{
	return wm8350_read_auxadc(wm8350, WM8350_AUXADC_LINE, 0, 0)
		* WM8350_AUX_COEFF;
}

static int wm8350_read_usb_uvolts(struct wm8350 *wm8350)
{
	return wm8350_read_auxadc(wm8350, WM8350_AUXADC_USB, 0, 0)
		* WM8350_AUX_COEFF;
}

#define WM8350_BATT_SUPPLY	1
#define WM8350_USB_SUPPLY	2
#define WM8350_LINE_SUPPLY	4

static inline int wm8350_charge_time_min(struct wm8350 *wm8350, int min)
{

	if (!wm8350->power.rev_g_coeff)
		return (((min - 30) / 15) & 0xf) << 8;
	else
		//return (((min - 30) / 30) & 0xf) << 8;
		return (((min - 60) / 30) & 0xf) << 8;
}

static int detect_usb_ac_charge()
{
	u16 val;
	int ret = 0;
	//Enable or Disable Charge_EN
	#define PORT_LINE_STATUS_MASK	(0x3 << 10)
	val = readl(S3C_UDC_OTG_HPRT) & PORT_LINE_STATUS_MASK;
	printk("Enter %s, what supply(0x%x)\n", __FUNCTION__, val);
	if(val && (val == PORT_LINE_STATUS_MASK)) {
		//Enable Charge_EN
#ifdef CONFIG_EX3_HARDWARE_DVT
		gpio_set_value(S3C64XX_GPL(7), 1);
#else
		gpio_set_value(S3C64XX_GPM(1), 1);
#endif
		printk("Enter %s, Adapter supply(0x%x)\n", __FUNCTION__, val);
		ret = WM8350_LINE_SUPPLY;
	} 
	else if(val == 0x400){
		//Disable Charge_EN
#ifdef CONFIG_EX3_HARDWARE_DVT
		gpio_set_value(S3C64XX_GPL(7), 0);
#else
		gpio_set_value(S3C64XX_GPM(1), 0);
#endif
		printk("Enter %s, USB supply(0x%x)\n", __FUNCTION__, val);
		ret = WM8350_USB_SUPPLY;
	}
	else if(val == 0x0){
#ifdef CONFIG_EX3_HARDWARE_DVT
		gpio_set_value(S3C64XX_GPL(7), 0);
#else	
		gpio_set_value(S3C64XX_GPM(1), 0);
#endif
		printk("Enter %s, battery supply(0x%x)\n", __FUNCTION__, val);
		ret = WM8350_BATT_SUPPLY;
	}
	return ret;
}

static int wm8350_get_supplies(struct wm8350 *wm8350)
{
	u16 sm, ov, co, chrg, chrh1, pm, chrh3;
	int supplies = 0;

	sm = wm8350_reg_read(wm8350, WM8350_STATE_MACHINE_STATUS);
	ov = wm8350_reg_read(wm8350, WM8350_MISC_OVERRIDES);
	co = wm8350_reg_read(wm8350, WM8350_COMPARATOR_OVERRIDES);
	chrg = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2);
	chrh1 = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1);
	pm = wm8350_reg_read(wm8350, WM8350_POWER_MGMT_5);
	chrh3 = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_3);
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
	if(enable_debug)
	{
		printk("Enter %s,  WM8350_STATE_MACHINE_STATUS(0xE9) state, reg is 0x%08x\n", __FUNCTION__, sm);
		//printk("Enter %s,  WM8350_MISC_OVERRIDES(0xE3) state, reg is 0x%08x\n", __FUNCTION__, ov);
		//printk("Enter %s,  WM8350_COMPARATOR_OVERRIDES(0xE7) state, reg is 0x%08x\n", __FUNCTION__, co);
		printk("Enter %s,  WM8350_BATTERY_CHARGER_CONTROL_2(0xA9), reg is 0x%08x\n", __FUNCTION__, chrg);
		printk("Enter %s,  WM8350_BATTERY_CHARGER_CONTROL_1(0xA8), reg is 0x%08x\n", __FUNCTION__, chrh1);
		//printk("Enter %s,  WM8350_POWER_MGMT_5(0x0C), reg is 0x%08x\n", __FUNCTION__, pm);
		printk("Enter %s,  WM8350_BATTERY_CHARGER_CONTROL_3(0xAA), reg is 0x%08x\n", __FUNCTION__, chrh3);
	}
//&*&*&*AL2_20100206, Add debug mode for trace charger issue

	/* USB_SM */
	sm = (sm & WM8350_USB_SM_MASK) >> WM8350_USB_SM_SHIFT;

	/* CHG_ISEL */
	chrg &= WM8350_CHG_ISEL_MASK;

	/* If the USB state machine is active then we're using that with or
	 * without battery, otherwise check for wall supply */
	if (((sm == WM8350_USB_SM_100_SLV) ||
				(sm == WM8350_USB_SM_500_SLV) ||
				(sm == WM8350_USB_SM_STDBY_SLV))
			&& !(ov & WM8350_USB_LIMIT_OVRDE))
		supplies = WM8350_USB_SUPPLY;
	else if (((sm == WM8350_USB_SM_100_SLV) ||
				(sm == WM8350_USB_SM_500_SLV) ||
				(sm == WM8350_USB_SM_STDBY_SLV))
			&& (ov & WM8350_USB_LIMIT_OVRDE) && (chrg == 0))
		supplies = WM8350_USB_SUPPLY | WM8350_BATT_SUPPLY;
	else if (co & WM8350_WALL_FB_OVRDE)
		supplies = WM8350_LINE_SUPPLY;
	else
		supplies = WM8350_BATT_SUPPLY;
		
	return supplies;
}

static int wm8350_charger_config(struct wm8350 *wm8350,
		struct wm8350_charger_policy *policy)
{
	u16 fast_limit_mA;
	int usb_ac;
	if (!policy) {
		dev_warn(wm8350->dev,
				"No charger policy, charger not configured.\n");
		return -EINVAL;
	}

	/* make sure USB fast charge current is not > 500mA */
	if (policy->fast_limit_USB_mA > 500) {
		dev_err(wm8350->dev, "USB fast charge > 500mA\n");
		return -EINVAL;
	}

	//if (wm8350_get_supplies(wm8350) & WM8350_USB_SUPPLY) {
	usb_ac = detect_usb_ac_charge();
	wm8350_reg_unlock(wm8350);
	if(usb_ac == WM8350_LINE_SUPPLY){
		fast_limit_mA =
			WM8350_CHG_FAST_LIMIT_mA(policy->fast_limit_mA);
		wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2,
				policy->charge_mV | policy->trickle_charge_mA |
				fast_limit_mA | wm8350_charge_time_min(wm8350,
					policy->charge_timeout));
	} else{
		
		fast_limit_mA =
					WM8350_CHG_FAST_LIMIT_mA(policy->fast_limit_USB_mA);
				wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2,
						policy->charge_mV | policy->trickle_charge_USB_mA |
						fast_limit_mA | wm8350_charge_time_min(wm8350,
							policy->charge_timeout));
	}
	wm8350_reg_lock(wm8350);

	return 0;
}

static ssize_t charger_current_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct wm8350 *wm8350 = dev_get_drvdata(dev);
	struct wm8350_charger_policy *policy = wm8350->power.policy;
	unsigned long fast_limit_mA;

	fast_limit_mA = simple_strtoul(buf, NULL, 0);
	if (fast_limit_mA >= 0 && fast_limit_mA <= 750) {
		dev_info(wm8350->dev, "adapter charge current:%ldmA\n", fast_limit_mA);
		policy->fast_limit_mA = fast_limit_mA;
		return (wm8350_charger_config(wm8350, policy)) ? -EINVAL : n;
	} else {
		return -EINVAL;
	}
}

static ssize_t charger_current_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct wm8350 *wm8350 = dev_get_drvdata(dev);
	u16 val;
	int fast_limit_mA;

	val = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2);
	fast_limit_mA = (val & 0xf) * 50;

	return sprintf(buf, "%d\n", fast_limit_mA);
}

static DEVICE_ATTR(charger_current, S_IRUGO | S_IWUSR, charger_current_show, charger_current_set);

static int wm8350_batt_status(struct wm8350 *wm8350)
{
	u16 state;

	state = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2);
	state &= WM8350_CHG_STS_MASK;

	switch (state) {
		case WM8350_CHG_STS_OFF:
			return POWER_SUPPLY_STATUS_DISCHARGING;

		case WM8350_CHG_STS_TRICKLE:
		case WM8350_CHG_STS_FAST:
			return POWER_SUPPLY_STATUS_CHARGING;

		default:
			return POWER_SUPPLY_STATUS_UNKNOWN;
	}
}

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
	struct wm8350 *wm8350 = dev_get_drvdata(dev);
	char *charge;
	int state;

	state = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2) &
		WM8350_CHG_STS_MASK;
	switch (state) {
		case WM8350_CHG_STS_OFF:
			charge = "Charger Off";
			break;
		case WM8350_CHG_STS_TRICKLE:
			charge = "Trickle Charging";
			break;
		case WM8350_CHG_STS_FAST:
			charge = "Fast Charging";
			break;
		default:
			return 0;
	}

	return sprintf(buf, "%s\n", charge);
}

static ssize_t charger_state_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct wm8350 *wm8350 = dev_get_drvdata(dev);
	char *p;
	int len;
	u16 val;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	if (len == 4 && !strncmp(buf, "Full", len)) {
		wm8350_reg_unlock(wm8350);
		val = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2) & 0xf; 
		wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2, val);
		wm8350_reg_lock(wm8350);
		return n;
	} else if (len == 4 && !strncmp(buf, "Stop", len)) {
		wm8350_reg_unlock(wm8350);
		val = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1) & ~WM8350_CHG_ENA_R168; 
		wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1, val);
		wm8350_reg_lock(wm8350);
		return n;
	} else {
		return -EINVAL;
	}
}

static DEVICE_ATTR(charger_state, 0644, charger_state_show, charger_state_set);

static void wm8350_charger_handler(struct wm8350 *wm8350, int irq, void *data)
{
	struct wm8350_power *power = &wm8350->power;
	struct wm8350_charger_policy *policy = power->policy;

	printk(KERN_INFO "ennic :%s ,irq:%d ..\n",__FUNCTION__,irq);
	
	switch (irq) {
		case WM8350_IRQ_CHG_BAT_FAIL:
			dev_err(wm8350->dev, "battery failed\n");
			charger_exception = "Battery Failed";
			power_supply_changed(&power->battery);
			break;
		case WM8350_IRQ_CHG_TO:
			dev_err(wm8350->dev, "charger timeout\n");
			charger_exception = "Charger Timeout";
			power_supply_changed(&power->battery);
			break;
		case WM8350_IRQ_CHG_END:
			dev_info(wm8350->dev, "charger end\n");
			power_supply_changed(&power->battery);
			break;
		//&*&*&*AL1_20100226, Add Battery temperature debug 
		case WM8350_IRQ_CHG_BAT_HOT:
			dev_info(wm8350->dev, "charger - battery too hot\n");
		case WM8350_IRQ_CHG_BAT_COLD:
			dev_info(wm8350->dev, "charger - battery too cold\n");
			power_supply_changed(&power->battery);
			break;
		//&*&*&*AL1_20100226, Add Battery temperature debug 
		case WM8350_IRQ_CHG_START:
			dev_info(wm8350->dev, "charger start\n");
			power_supply_changed(&power->battery);
			break;

		case WM8350_IRQ_CHG_FAST_RDY:
			dev_info(wm8350->dev, "fast charger ready\n");
			//wm8350_charger_config(wm8350, policy);
			wm8350_reg_unlock(wm8350);
			wm8350_set_bits(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1,
					WM8350_CHG_FAST);
			wm8350_reg_lock(wm8350);
			break;

		case WM8350_IRQ_CHG_VBATT_LT_3P9:
			dev_warn(wm8350->dev, "battery < 3.9V\n");
			break;
		case WM8350_IRQ_CHG_VBATT_LT_3P1:
			dev_warn(wm8350->dev, "battery < 3.1V\n");
			break;
		case WM8350_IRQ_CHG_VBATT_LT_2P85:
			dev_warn(wm8350->dev, "battery < 2.85V\n");
			break;
			/* Supply change.  We will overnotify but it should do
			 * no harm. */
		case WM8350_IRQ_EXT_USB_FB:
		case WM8350_IRQ_EXT_WALL_FB:
			wm8350_charger_config(wm8350, policy);
		case WM8350_IRQ_EXT_BAT_FB:   /* Fall through */
			power_supply_changed(&power->battery);
			power_supply_changed(&power->usb);
			power_supply_changed(&power->ac);
			break;

		default:
			dev_err(wm8350->dev, "Unknown interrupt %d\n", irq);
			charger_exception = "Unknown";
	}
}

/*********************************************************************
 *		AC Power
 *********************************************************************/
static int wm8350_ac_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct wm8350 *wm8350 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = !!(wm8350_get_supplies(wm8350) &
					WM8350_LINE_SUPPLY);
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = wm8350_read_line_uvolts(wm8350);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static enum power_supply_property wm8350_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

/*********************************************************************
 *		USB Power
 *********************************************************************/
static int wm8350_usb_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct wm8350 *wm8350 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = !!(wm8350_get_supplies(wm8350) &
					WM8350_USB_SUPPLY);
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = wm8350_read_usb_uvolts(wm8350);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static enum power_supply_property wm8350_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

/*********************************************************************
 *		Battery properties
 *********************************************************************/

static int wm8350_bat_check_health(struct wm8350 *wm8350)
{
	u16 reg;

	if (wm8350_read_battery_uvolts(wm8350) < 2850000)
		return POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;

	reg = wm8350_reg_read(wm8350, WM8350_CHARGER_OVERRIDES);
	if (reg & WM8350_CHG_BATT_HOT_OVRDE)
		return POWER_SUPPLY_HEALTH_OVERHEAT;

	if (reg & WM8350_CHG_BATT_COLD_OVRDE)
		return POWER_SUPPLY_HEALTH_COLD;

	return POWER_SUPPLY_HEALTH_GOOD;
}

static int wm8350_bat_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct wm8350 *wm8350 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = wm8350_batt_status(wm8350);
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = !!(wm8350_get_supplies(wm8350) &
					WM8350_BATT_SUPPLY);
			break;
		/*case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = wm8350_read_battery_uvolts(wm8350);
			break;
		*/
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = wm8350_bat_check_health(wm8350);
			break;
		default:
			ret = bq27x00_battery_get_property(psp,val);
			break;
	}
	
	return ret;
}

static enum power_supply_property wm8350_bat_props[] = {

	POWER_SUPPLY_PROP_PRESENT,
	//POWER_SUPPLY_PROP_VOLTAGE_NOW,
	//POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	//POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,

	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

/*********************************************************************
 *		charging led control
 *********************************************************************/
static void set_orange_on_light(struct wm8350 *wm8350)
{
	u16 gpio_func; 
	//Light off GPIO10 LED
	wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 10);
	gpio_func = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3) & ~WM8350_GP11_FN_MASK;
	//Change GPIO11 function as normal GPIO 
	wm8350_reg_unlock(wm8350);
	wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3, gpio_func | (WM8350_GPIO11_GPIO_OUT << 12));
	wm8350_reg_lock(wm8350);
	wm8350_clear_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 11);	
}

static void set_green_on_light(struct wm8350 *wm8350)
{
	u16 gpio_func; 
	//Light off GPIO10 LED
	gpio_func = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3) & ~WM8350_GP11_FN_MASK;
	//Change GPIO11 function as normal GPIO 
	wm8350_reg_unlock(wm8350);
	wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3, gpio_func | (WM8350_GPIO11_GPIO_OUT << 12));
	wm8350_reg_lock(wm8350);
	wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 11);
	wm8350_clear_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 10);
	
}

static void set_off_light(struct wm8350 *wm8350)
{
	u16 gpio_func; 
	gpio_func = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3) & ~WM8350_GP11_FN_MASK;
	//Change GPIO11 function as normal GPIO 
	wm8350_reg_unlock(wm8350);
	wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3, gpio_func | (WM8350_GPIO11_GPIO_OUT << 12));
	wm8350_reg_lock(wm8350);
	wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 11);
	//Light off GPIO10 LED
	wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 10);
}

static void set_orange_flash_light(struct wm8350 *wm8350)
{
	u16 gpio_func; 
	wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 10);
	//Light off GPIO10 LED
	gpio_func = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3) & ~WM8350_GP11_FN_MASK;
	//Change GPIO11 function as normal GPIO 
	wm8350_reg_unlock(wm8350);
	wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3, gpio_func | (WM8350_GPIO11_CHD_IND_IN << 12));
	//wm8350_set_bits(wm8350, WM8350_GPIO_PIN_STATUS, 1 << 11);	
	wm8350_reg_lock(wm8350);
}

static void wm8350_set_charge_led(struct power_supply *psy)
{
	struct wm8350 *wm8350 = dev_get_drvdata(psy->dev->parent);
	union power_supply_propval val = {0,};
	int supply = 0;
	wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_PRESENT, &val);
	if(val.intval == POWER_SUPPLY_HEALTH_GOOD){
		wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &val);
		supply = wm8350_get_supplies(wm8350);
		//supply = detect_usb_ac_charge();
		if(val.intval >= 95 && supply!= WM8350_BATT_SUPPLY){
			set_green_on_light(wm8350);
			printk("Enter %s, set_green_on_light, val is %d\n", __FUNCTION__, val.intval);
		}
		else if(val.intval < 95 && supply!= WM8350_BATT_SUPPLY){
			set_orange_flash_light(wm8350);
			printk("Enter %s, set_orange_flash_light \n", __FUNCTION__);
		}
		else
		{
			set_off_light(wm8350);
			printk("Enter %s, set_off_light, val is %d \n", __FUNCTION__, val.intval);
		}
	}
	else{
		set_orange_on_light(wm8350);
		printk("Enter %s, set_orange_on_light \n", __FUNCTION__);
	}
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
	if(enable_debug)
	{
		wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
		printk("Enter %s, VOLTAGE_NOW is %d\n", __FUNCTION__, val.intval);
		wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val);
		printk("Enter %s, CURRENT_NOW is %d\n", __FUNCTION__, val.intval);
		wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &val);
		if(val.intval < 95)
			supply =1;
		else
			supply =0;
		printk("Enter %s, CAPACITY is %d\n", __FUNCTION__, val.intval);
		wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_TEMP, &val);
		printk("Enter %s, Temperature is %d\n", __FUNCTION__, val.intval);
		if(val.intval>=0 && val.intval<=45)
		{
			wm8350_bat_get_property(psy, POWER_SUPPLY_PROP_STATUS, &val);
			if((val.intval==POWER_SUPPLY_STATUS_DISCHARGING) && (wm8350_get_supplies(wm8350)!=WM8350_BATT_SUPPLY) && supply)
			{
				wm8350_reg_unlock(wm8350);
				wm8350_set_bits(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1, 1 << 15);
				wm8350_reg_lock(wm8350);
				printk("Enter %s, enable charging temperature is suitable\n", __FUNCTION__);
			}
		}
		else
		{
			wm8350_reg_unlock(wm8350);
			wm8350_clear_bits(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1, 1 << 15);
			wm8350_reg_lock(wm8350);
			printk("Enter %s, disabel charging temperature is too hot or cold\n", __FUNCTION__);
		}
	}
//&*&*&*AL2_20100206, Add debug mode for trace charger issue
}
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
static void wm8350_gasgauge_timeout(unsigned long arg)
{
	struct power_supply *battery = (struct power_supply *)arg;
	if(enable_debug){
		power_supply_changed(battery);
		mod_timer(&gasgauge_timeout, jiffies+30*HZ);
	}
	else{
		detect_usb_ac_charge();
	}
}

static ssize_t charger_charger_debug_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct wm8350 *wm8350 = dev_get_drvdata(dev);
	
	if (!strncmp((const char*)buf, "debug_on", n-1)) {

		del_timer_sync(&gasgauge_timeout);
		mod_timer(&gasgauge_timeout, jiffies+30*HZ);
		enable_debug = 1;
	}
	else if (!strncmp((const char*)buf, "debug_off", n-1)) {
		del_timer_sync(&gasgauge_timeout);
		enable_debug = 0;
	}
	else if (!strncmp((const char*)buf, "led_off", n-1)) {
		set_off_light(wm8350);
	}
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
//&*&*&*AL2_20100206, Add debug mode for trace charger issue
/*********************************************************************
 *		Initialisation
 *********************************************************************/

static void wm8350_init_charger(struct wm8350 *wm8350, struct wm8350_charger_policy *policy)
{
	
	u16 reg, eoc_mA, fast_limit_mA;
	int usb_ac;

	eoc_mA = WM8350_CHG_EOC_mA(policy->eoc_mA);
    wm8350_reg_unlock(wm8350);
	wm8350_reg_write(wm8350, 0x06, 0x9200);
	wm8350_reg_write(wm8350, 0xdb, 0x1375);
	wm8350_reg_write(wm8350, 0xde, 0x2864);
	wm8350_reg_write(wm8350, 0xdb, 0x00a7);
	wm8350_reg_write(wm8350, 0xde, 0x0013);
	wm8350_reg_write(wm8350, 0xdb, 0x0013);

	wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1, 0x8331);
	wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2, 0xa138);

	reg = wm8350_reg_read(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1)
		& WM8350_CHG_ENA_R168;
	wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_1,
			reg | eoc_mA | policy->trickle_start_mV |
			WM8350_CHG_TRICKLE_TEMP_CHOKE |
			WM8350_CHG_TRICKLE_USB_CHOKE |WM8350_CHG_CHIP_TEMP_MON |
			WM8350_CHG_FAST_USB_THROTTLE);
	usb_ac = wm8350_get_supplies(wm8350);
	if(usb_ac == WM8350_LINE_SUPPLY)
	{
		fast_limit_mA =
			WM8350_CHG_FAST_LIMIT_mA(policy->fast_limit_mA);
		wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2,
				policy->charge_mV | policy->trickle_charge_mA |
				fast_limit_mA | wm8350_charge_time_min(wm8350,
					policy->charge_timeout));
	}
	else
	{
		fast_limit_mA =
			WM8350_CHG_FAST_LIMIT_mA(policy->fast_limit_USB_mA);
		wm8350_reg_write(wm8350, WM8350_BATTERY_CHARGER_CONTROL_2,
				policy->charge_mV | policy->trickle_charge_USB_mA |
				fast_limit_mA | wm8350_charge_time_min(wm8350,
					policy->charge_timeout));
	}
	wm8350_set_bits(wm8350, WM8350_POWER_MGMT_5, WM8350_CHG_ENA);
	wm8350_reg_lock(wm8350);
	
	/* register our interest in charger events */
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_BAT_HOT,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_BAT_HOT);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_BAT_COLD,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_BAT_COLD);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_BAT_FAIL,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_BAT_FAIL);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_TO,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_TO);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_END,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_END);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_START,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_START);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_FAST_RDY,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_FAST_RDY);
#if 0
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P9,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P9);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P1,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P1);
	wm8350_register_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_2P85,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_2P85);
#endif

	/* and supply change events */
	wm8350_register_irq(wm8350, WM8350_IRQ_EXT_USB_FB,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_EXT_USB_FB);
	wm8350_register_irq(wm8350, WM8350_IRQ_EXT_WALL_FB,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_EXT_WALL_FB);
	wm8350_register_irq(wm8350, WM8350_IRQ_EXT_BAT_FB,
			wm8350_charger_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_EXT_BAT_FB);
}

static void free_charger_irq(struct wm8350 *wm8350)
{
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_BAT_HOT);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_BAT_HOT);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_BAT_COLD);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_BAT_COLD);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_BAT_FAIL);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_BAT_FAIL);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_TO);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_TO);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_END);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_END);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_START);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_START);
#if 0
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P9);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P9);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P1);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P1);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_2P85);
	wm8350_free_irq(wm8350, WM8350_IRQ_CHG_VBATT_LT_2P85);
#endif
	wm8350_mask_irq(wm8350, WM8350_IRQ_EXT_USB_FB);
	wm8350_free_irq(wm8350, WM8350_IRQ_EXT_USB_FB);
	wm8350_mask_irq(wm8350, WM8350_IRQ_EXT_WALL_FB);
	wm8350_free_irq(wm8350, WM8350_IRQ_EXT_WALL_FB);
	wm8350_mask_irq(wm8350, WM8350_IRQ_EXT_BAT_FB);
	wm8350_free_irq(wm8350, WM8350_IRQ_EXT_BAT_FB);
}

static __devinit int wm8350_power_probe(struct platform_device *pdev)
{
	struct wm8350 *wm8350 = platform_get_drvdata(pdev);
	struct wm8350_power *power = &wm8350->power;
	struct wm8350_charger_policy *policy = power->policy;
	struct power_supply *usb = &power->usb;
	struct power_supply *battery = &power->battery;
	struct power_supply *ac = &power->ac;
	int ret;

//	ac->name = "wm8350-ac";
	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->properties = wm8350_ac_props;
	ac->num_properties = ARRAY_SIZE(wm8350_ac_props);
	ac->get_property = wm8350_ac_get_prop;
	ret = power_supply_register(&pdev->dev, ac);
	if (ret)
		return ret;

	battery->name = "battery";
	//battery->name = "wm8350-battery";
	battery->properties = wm8350_bat_props;
	battery->num_properties = ARRAY_SIZE(wm8350_bat_props);
	battery->get_property = wm8350_bat_get_property;
	battery->external_power_changed = wm8350_set_charge_led;
	battery->use_for_apm = 1;
	battery->num_supplicants = ARRAY_SIZE(wm8350_power_supplied_to);
	battery->supplied_to = wm8350_power_supplied_to;
	ret = power_supply_register(&pdev->dev, battery);
	if (ret)
		goto battery_failed;
	//ennic added	
	s3c_bat_create_attrs(battery->dev);
		
	//usb->name = "wm8350-usb",
	usb->name = "usb",
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = wm8350_usb_props;
	usb->num_properties = ARRAY_SIZE(wm8350_usb_props);
	usb->get_property = wm8350_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto usb_failed;

	ret = device_create_file(&pdev->dev, &dev_attr_charger_exception);
	if (ret < 0)
		dev_warn(wm8350->dev, "failed to add charge exception sysfs: %d\n", ret);

	ret = device_create_file(&pdev->dev, &dev_attr_charger_state);
	if (ret < 0)
		dev_warn(wm8350->dev, "failed to add charge state sysfs: %d\n", ret);

	ret = device_create_file(&pdev->dev, &dev_attr_charger_current);
	if (ret < 0)
		dev_warn(wm8350->dev, "failed to add charge current sysfs: %d\n", ret);
//&*&*&*AL1_20100206, Add debug mode for trace charger issue
	ret = device_create_file(&pdev->dev, &dev_attr_charger_debug);
	if (ret < 0)
		dev_warn(wm8350->dev, "failed to add charge debug sysfs: %d\n", ret);
//&*&*&*AL2_20100206, Add debug mode for trace charger issue
	ret = 0;

	init_timer(&gasgauge_timeout);
	gasgauge_timeout.function = wm8350_gasgauge_timeout;
	gasgauge_timeout.data = (unsigned long)battery;

	wm8350_init_charger(wm8350, policy);
//&*&*&*AL1_20100206, check chager supply after bootup system
	mod_timer(&gasgauge_timeout, jiffies+10*HZ);
//&*&*&*AL2_20100206, check chager supply after bootup system
	return ret;

usb_failed:
	power_supply_unregister(battery);
battery_failed:
	power_supply_unregister(ac);

	return ret;
}

static __devexit int wm8350_power_remove(struct platform_device *pdev)
{
	struct wm8350 *wm8350 = platform_get_drvdata(pdev);
	struct wm8350_power *power = &wm8350->power;

	free_charger_irq(wm8350);
	device_remove_file(&pdev->dev, &dev_attr_charger_state);
	device_remove_file(&pdev->dev, &dev_attr_charger_exception);
	device_remove_file(&pdev->dev, &dev_attr_charger_current);
	power_supply_unregister(&power->battery);
	power_supply_unregister(&power->ac);
	power_supply_unregister(&power->usb);
	return 0;
}

static struct platform_driver wm8350_power_driver = {
	.probe = wm8350_power_probe,
	.remove = __devexit_p(wm8350_power_remove),
	.driver = {
		.name = "wm8350-power",
	},
};

static int __init wm8350_power_init(void)
{
	return platform_driver_register(&wm8350_power_driver);
}
module_init(wm8350_power_init);

static void __exit wm8350_power_exit(void)
{
	platform_driver_unregister(&wm8350_power_driver);
}
module_exit(wm8350_power_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Power supply driver for WM8350");
MODULE_ALIAS("platform:wm8350-power");
