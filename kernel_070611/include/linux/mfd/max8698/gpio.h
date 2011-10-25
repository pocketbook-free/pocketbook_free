/*
 * gpio.h  --  GPIO Driver for max8698 PMIC
 *
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
 
 #include <linux/platform_device.h>
 
 
 struct max8698;
 
 struct max8698_gpio {
	struct platform_device *pdev;
};

