/*
 * pmic.h  --  Power Managment Driver for max8698 PMIC
 *
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
 
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/regulator/machine.h>


//#define NUM_MAX8698_REGULATORS			16


struct max8698_pmic{
	/* Number of regulators of each type on this device */
	int max_ldo;
	int max_buck;

	/* ISINK to DCDC mapping */
//	int isink_A_dcdc;
//	int isink_B_dcdc;

	/* hibernate configs 
	u16 dcdc1_hib_mode;
	u16 dcdc3_hib_mode;
	u16 dcdc4_hib_mode;
	u16 dcdc6_hib_mode;*/

	/* regulator devices */
	struct platform_device *pdev;

};

int max8698_register_regulator(struct max8698 *max8698, int reg,
			      struct regulator_init_data *initdata);
