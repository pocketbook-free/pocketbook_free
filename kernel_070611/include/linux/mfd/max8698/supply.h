/*
 * supply.h  --  Power Supply Driver for max8698 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
 
#include <linux/mutex.h>
#include <linux/power_supply.h>


#define CHG_STATUS_OFF 1
#define CHG_STATUS_CHARGE 0


#define MAX8698_IRQ_DETECT_POWER			0

struct max8698_charger_policy {

	/* charger state machine policy  - set in machine driver */
	int eoc_mA;		/* end of charge current (mA)  */
	int charge_mV;		/* charge voltage */
	int fast_limit_mA;	/* fast charge current limit */
	int fast_limit_USB_mA;	/* USB fast charge current limit */
	int charge_timeout;	/* charge timeout (mins) */
	int trickle_start_mV;	/* trickle charge starts at mV */
	int trickle_charge_mA;	/* trickle charge current */
	int trickle_charge_USB_mA;	/* USB trickle charge current */
};

struct max8698_power {
	struct platform_device *pdev;
	struct power_supply battery;
	struct power_supply usb;
	struct power_supply ac;
	struct power_supply max;
	struct max8698_charger_policy *policy;

	int rev_g_coeff;
};

struct max17043_power {
	struct platform_device *pdev;
	struct power_supply battery;
	struct power_supply usb;
	struct power_supply ac;
	struct power_supply max;
//	struct max8698_charger_policy *policy;

	int rev_g_coeff;
};

extern int max17043_battery_get_property(enum power_supply_property psp,
										union power_supply_propval *val);
