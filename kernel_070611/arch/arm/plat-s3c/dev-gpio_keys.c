/* linux/arch/arm/plat-s3c/dev-gpio_keys.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>

#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mach/irq.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/input_key.h>
#include <plat/gpio-cfg.h>
#include <asm/io.h>

#include <linux/io.h> /* Tony add 2010-07-09 */
#include <plat/regs-clock.h>

#include <linux/delay.h> /* Tony add 2010-07-09 */

static struct gpio_keys_button gpio_keys[] = {
	[0] = { 
		.code	= GPIO_KEY_POWER,
		.gpio	= S3C64XX_GPN(1),
		.desc	= "Power key", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 100,
		.timer_interval	= 50,
		.longpress_time = 2000,
	},
	[1] = {  
		.code	= GPIO_MENU, 
		.gpio	= S3C64XX_GPL(8),  
		.desc	= "Menu key", 
		.type	=EV_KEY, 
		.active_low	= 1, 
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[2] = {  
		.code	= GPIO_HOME,
		.gpio	= S3C64XX_GPL(9),
		.desc	= "Home key",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[3] = {  
		.code	= GPIO_PAGE_UP,
		.gpio	= S3C64XX_GPL(10),
		.desc	= "Page up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},	
	[4] = {  
		.code	= GPIO_PAGE_DOWN,
		.gpio	= S3C64XX_GPL(11),
		.desc	= "Page down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},	
	[5] = {  
		.code	= GPIO_BACK,
		.gpio	= S3C64XX_GPL(12),
		.desc	= "Key back",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},	
  #if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP2_EVT)
	[6] = { 
		.code	= GPIO_5_UP,
		.gpio	= S3C64XX_GPN(8),
		.desc	= "Five way up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
  [7] = {  
		.code	= GPIO_5_DOWN,
		.gpio	= S3C64XX_GPN(9),
		.desc	= "Five way down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[8] = {  
		.code	= GPIO_5_LEFT,
		.gpio	= S3C64XX_GPN(6),
		.desc	= "Five way left",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[9] = {  
		.code	= GPIO_5_RIGHT,
		.gpio	= S3C64XX_GPN(12),
		.desc	= "Five way right",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[10] = {  
		.code	= GPIO_5_ENTER,
		.gpio	= S3C64XX_GPN(10),
		.desc	= "Five way enter",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[11] = {  
		.code	= GPIO_TD_SWITCH,
		.gpio	= S3C64XX_GPM(2),
		.desc	= "TD switch",
		.type	= EV_SW,
		.active_low	= 1,
		.debounce_interval	= 100,
		.timer_interval	= 200,
		.longpress_time = 0,
	},	
	[12] = { 
		.code	= GPIO_PEN_OUT,
		.gpio	= S3C64XX_GPN(4),
		.desc	= "Pen out",
		.type	= EV_KEY,
		.active_low	= 0,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	#endif
	#if defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
	[6] = { 
		.code	= GPIO_5_UP,
		.gpio	= S3C64XX_GPN(12),
		.desc	= "Five way up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
  [7] = {  
		.code	= GPIO_5_DOWN,
		.gpio	= S3C64XX_GPN(6),
		.desc	= "Five way down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[8] = {  
		.code	= GPIO_5_LEFT,
		.gpio	= S3C64XX_GPN(8),
		.desc	= "Five way left",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[9] = {  
		.code	= GPIO_5_RIGHT,
		.gpio	= S3C64XX_GPN(9),
		.desc	= "Five way right",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[10] = {  
		.code	= GPIO_5_ENTER,
		.gpio	= S3C64XX_GPN(10),
		.desc	= "Five way enter",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[11] = {  
		.code	= GPIO_TD_SWITCH,
		.gpio	= S3C64XX_GPM(2),
		.desc	= "TD switch",
		.type	= EV_SW,
		.active_low	= 1,
		.debounce_interval	= 100,
		.timer_interval	= 200,
		.longpress_time = 0,
	},	
	[12] = { 
		.code	= GPIO_PEN_OUT,
		.gpio	= S3C64XX_GPN(4),
		.desc	= "Pen out",
		.type	= EV_KEY,
		.active_low	= 0,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	#endif
	#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)
	[6] = {  
		.code	= GPIO_5WAY_KEY,
		.gpio	= S3C64XX_GPN(12),
		.desc	= "5 way key",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},	
	[7] = {  
		.code	= GPIO_TD_SWITCH,
		.gpio	= S3C64XX_GPN(8),
		.desc	= "TD switch",
		.type	= EV_SW,
		.active_low	= 1,
		.debounce_interval	= 100,
		.timer_interval	= 200,
		.longpress_time = 0,
	},	
	[8] = { 
		.code	= GPIO_PEN_OUT,
		.gpio	= S3C64XX_GPN(4),
		.desc	= "Pen out",
		.type	= EV_KEY,
		.active_low	= 0,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	
	[9] = {  
		.code	= GPIO_SIZE_Up,
		.gpio	= S3C64XX_GPM(2),
		.desc	= "Size up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[10] = {  
		.code	= GPIO_SIZE_Dn,
		.gpio	= S3C64XX_GPL(14),
		.desc	= "Size down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[11] = {  
		.code	= GPIO_Volume_Up,
		.gpio	= S3C64XX_GPN(6),
		.desc	= "Volume up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[12] = {  
		.code	= GPIO_Volume_Dn,
		.gpio	= S3C64XX_GPN(7),
		.desc	= "Volume down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	#endif
	#if defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
	[6] = {  
		.code	= GPIO_5WAY_KEY,
		.gpio	= S3C64XX_GPN(12),
		.desc	= "5 way key",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},	
	[7] = {  
		.code	= GPIO_TD_SWITCH,
		.gpio	= S3C64XX_GPN(8),
		.desc	= "TD switch",
		.type	= EV_SW,
		.active_low	= 1,
		.debounce_interval	= 100,
		.timer_interval	= 200,
		.longpress_time = 0,
	},	
	[8] = { 
		.code	= GPIO_PEN_OUT,
		.gpio	= S3C64XX_GPN(4),
		.desc	= "Pen out",
		.type	= EV_KEY,
		.active_low	= 0,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	
	[9] = {  
		.code	= GPIO_SIZE_Up,
		.gpio	= S3C64XX_GPM(2),
		.desc	= "Size up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[10] = {  
		.code	= GPIO_SIZE_Dn,
		.gpio	= S3C64XX_GPN(10),
		.desc	= "Size down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[11] = {  
		.code	= GPIO_Volume_Up,
		.gpio	= S3C64XX_GPN(6),
		.desc	= "Volume up",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	[12] = {  
		.code	= GPIO_Volume_Dn,
		.gpio	= S3C64XX_GPN(3),
		.desc	= "Volume down",
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 0,
	},
	#endif

};


static struct gpio_keys_platform_data s3c_gpio_keys_platdata = 
{
	.buttons	= gpio_keys,
	.nbuttons	= ARRAY_SIZE(gpio_keys),
};


struct platform_device s3c_device_gpio_keys = 
{
	.name		= "gpio-keys-blazer",
	.id		= 0,
	.dev		= 
	{
		.platform_data	= &s3c_gpio_keys_platdata,
	},
};
extern void s3c_gpio_keys_init(void);

void s3c_gpio_keys_init(void)
{
	printk("~~~~~~~~~~~~~~ Blazer:gpio keys dev-gpio_key for EP series~~~~~~~~~~~~~~~~~\n");
	s3c_gpio_cfgpin(S3C64XX_GPN(1), S3C_GPIO_SFN(2));     //SYS_ON
	s3c_gpio_cfgpin(S3C64XX_GPL(8), S3C_GPIO_SFN(3));     //Menu
	s3c_gpio_cfgpin(S3C64XX_GPL(9), S3C_GPIO_SFN(3));     //Home
	s3c_gpio_cfgpin(S3C64XX_GPL(10), S3C_GPIO_SFN(3));    //Page up
	s3c_gpio_cfgpin(S3C64XX_GPL(11), S3C_GPIO_SFN(3));    //Page down
	s3c_gpio_cfgpin(S3C64XX_GPL(12), S3C_GPIO_SFN(3));    //Key back
	
	s3c_gpio_setpull(S3C64XX_GPN(1), S3C_GPIO_PULL_UP);    //SYS_ON
	s3c_gpio_setpull(S3C64XX_GPL(8), S3C_GPIO_PULL_UP);    //Menu
	s3c_gpio_setpull(S3C64XX_GPL(9), S3C_GPIO_PULL_UP);    //Home
	s3c_gpio_setpull(S3C64XX_GPL(10), S3C_GPIO_PULL_UP);    //Page up
	s3c_gpio_setpull(S3C64XX_GPL(11), S3C_GPIO_PULL_UP);    //Page down
	s3c_gpio_setpull(S3C64XX_GPL(12), S3C_GPIO_PULL_UP);    //Key back
	
	set_irq_type(IRQ_EINT(1), IRQ_TYPE_LEVEL_LOW);     //SYS_ON
	set_irq_type(IRQ_EINT(16), IRQ_TYPE_EDGE_BOTH);//GPL8   Menu
	set_irq_type(IRQ_EINT(17), IRQ_TYPE_EDGE_BOTH);//GPL9   Home
	set_irq_type(IRQ_EINT(18), IRQ_TYPE_EDGE_BOTH);//GPL10  Page up
	set_irq_type(IRQ_EINT(19), IRQ_TYPE_EDGE_BOTH);//GPL11  Page down
	set_irq_type(IRQ_EINT(20), IRQ_TYPE_EDGE_BOTH);//GPL12  Key back
		
	#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
	s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));    //K5
	s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C_GPIO_SFN(2));     //K6
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(2));     //K7
	s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(2));     //K8
	s3c_gpio_cfgpin(S3C64XX_GPN(10), S3C_GPIO_SFN(2));    //K9
	s3c_gpio_cfgpin(S3C64XX_GPM(2), S3C_GPIO_SFN(3));     //TD switch
	s3c_gpio_cfgpin(S3C64XX_GPN(4), S3C_GPIO_SFN(2));     //Pen out
	
	s3c_gpio_setpull(S3C64XX_GPN(12), S3C_GPIO_PULL_UP);    //K5
	s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_UP);     //K6
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_UP);     //K7
	s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_UP);     //K8
	s3c_gpio_setpull(S3C64XX_GPN(10), S3C_GPIO_PULL_UP);    //K9
	s3c_gpio_setpull(S3C64XX_GPM(2), S3C_GPIO_PULL_UP);     //TD switch
	s3c_gpio_setpull(S3C64XX_GPN(4), S3C_GPIO_PULL_DOWN);   //Pen out	
	
	set_irq_type(IRQ_EINT(12), IRQ_TYPE_EDGE_BOTH);    //K5
	set_irq_type(IRQ_EINT(6), IRQ_TYPE_EDGE_BOTH);     //K6
	set_irq_type(IRQ_EINT(8), IRQ_TYPE_EDGE_BOTH);     //K7
	set_irq_type(IRQ_EINT(9), IRQ_TYPE_EDGE_BOTH);     //K8
	set_irq_type(IRQ_EINT(10), IRQ_TYPE_EDGE_BOTH);    //K9
	set_irq_type(IRQ_EINT(25), IRQ_TYPE_EDGE_BOTH);   //TD switch
	set_irq_type(IRQ_EINT(4), IRQ_TYPE_EDGE_BOTH);   //Pen out
	#endif
	
	#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)
	s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));    //5-way-key
	s3c_gpio_cfgpin(S3C64XX_GPF(0), S3C_GPIO_SFN(0));     //K5
	s3c_gpio_cfgpin(S3C64XX_GPF(1), S3C_GPIO_SFN(0));     //K6
	s3c_gpio_cfgpin(S3C64XX_GPF(2), S3C_GPIO_SFN(0));     //K7
	s3c_gpio_cfgpin(S3C64XX_GPF(3), S3C_GPIO_SFN(0));     //K8
	s3c_gpio_cfgpin(S3C64XX_GPF(4), S3C_GPIO_SFN(0));     //K9
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(2));     //TD switch
	s3c_gpio_cfgpin(S3C64XX_GPN(4), S3C_GPIO_SFN(2));     //Pen out	
	s3c_gpio_cfgpin(S3C64XX_GPM(2), S3C_GPIO_SFN(3));	    //Size up
	s3c_gpio_cfgpin(S3C64XX_GPL(14), S3C_GPIO_SFN(3));	  //Size down
	s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C_GPIO_SFN(2));		 //Volume up
	s3c_gpio_cfgpin(S3C64XX_GPN(7), S3C_GPIO_SFN(2));		 //Volume down	
	
	s3c_gpio_setpull(S3C64XX_GPN(12), S3C_GPIO_PULL_UP);    //5-way-key
	s3c_gpio_setpull(S3C64XX_GPF(0), S3C_GPIO_PULL_UP);    //K5
	s3c_gpio_setpull(S3C64XX_GPF(1), S3C_GPIO_PULL_UP);     //K6
	s3c_gpio_setpull(S3C64XX_GPF(2), S3C_GPIO_PULL_UP);     //K7
	s3c_gpio_setpull(S3C64XX_GPF(3), S3C_GPIO_PULL_UP);     //K8
	s3c_gpio_setpull(S3C64XX_GPF(4), S3C_GPIO_PULL_UP);    //K9
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_UP);     //TD switch
	s3c_gpio_setpull(S3C64XX_GPN(4), S3C_GPIO_PULL_DOWN);   //Pen out
	s3c_gpio_setpull(S3C64XX_GPM(2), S3C_GPIO_PULL_UP);     //Size up
	s3c_gpio_setpull(S3C64XX_GPL(14), S3C_GPIO_PULL_UP);     //Size down
	s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_UP);      //Volume up
	s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_UP);      //Volume down
	
	set_irq_type(IRQ_EINT(12), IRQ_TYPE_EDGE_BOTH);    //5-way-key
	set_irq_type(IRQ_EINT(8), IRQ_TYPE_EDGE_BOTH);   //TD switch
	set_irq_type(IRQ_EINT(4), IRQ_TYPE_EDGE_BOTH);   //Pen out
	set_irq_type(IRQ_EINT(25), IRQ_TYPE_EDGE_BOTH);   //Size up
	set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_BOTH);   //Size down
	set_irq_type(IRQ_EINT(6), IRQ_TYPE_EDGE_BOTH);   //Volume up
	set_irq_type(IRQ_EINT(7), IRQ_TYPE_EDGE_BOTH);   //Volume down
	
	#endif
	
	#if defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
	s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));    //5-way-key
	s3c_gpio_cfgpin(S3C64XX_GPF(0), S3C_GPIO_SFN(0));     //K5
	s3c_gpio_cfgpin(S3C64XX_GPF(1), S3C_GPIO_SFN(0));     //K6
	s3c_gpio_cfgpin(S3C64XX_GPF(2), S3C_GPIO_SFN(0));     //K7
	s3c_gpio_cfgpin(S3C64XX_GPF(3), S3C_GPIO_SFN(0));     //K8
	s3c_gpio_cfgpin(S3C64XX_GPF(4), S3C_GPIO_SFN(0));     //K9
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(2));     //TD switch
	s3c_gpio_cfgpin(S3C64XX_GPN(4), S3C_GPIO_SFN(2));     //Pen out	
	s3c_gpio_cfgpin(S3C64XX_GPM(2), S3C_GPIO_SFN(3));	    //Size up
	s3c_gpio_cfgpin(S3C64XX_GPN(10), S3C_GPIO_SFN(2));	  //Size down
	s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C_GPIO_SFN(2));		 //Volume up
	s3c_gpio_cfgpin(S3C64XX_GPN(3), S3C_GPIO_SFN(2));		 //Volume down	
	
	s3c_gpio_setpull(S3C64XX_GPN(12), S3C_GPIO_PULL_UP);    //5-way-key
	s3c_gpio_setpull(S3C64XX_GPF(0), S3C_GPIO_PULL_UP);    //K5
	s3c_gpio_setpull(S3C64XX_GPF(1), S3C_GPIO_PULL_UP);     //K6
	s3c_gpio_setpull(S3C64XX_GPF(2), S3C_GPIO_PULL_UP);     //K7
	s3c_gpio_setpull(S3C64XX_GPF(3), S3C_GPIO_PULL_UP);     //K8
	s3c_gpio_setpull(S3C64XX_GPF(4), S3C_GPIO_PULL_UP);    //K9
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_UP);     //TD switch
	s3c_gpio_setpull(S3C64XX_GPN(4), S3C_GPIO_PULL_DOWN);   //Pen out
	s3c_gpio_setpull(S3C64XX_GPM(2), S3C_GPIO_PULL_UP);     //Size up
	s3c_gpio_setpull(S3C64XX_GPN(10), S3C_GPIO_PULL_UP);     //Size down
	s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_UP);      //Volume up
	s3c_gpio_setpull(S3C64XX_GPN(3), S3C_GPIO_PULL_UP);      //Volume down
	
	set_irq_type(IRQ_EINT(12), IRQ_TYPE_EDGE_BOTH);    //5-way-key
	set_irq_type(IRQ_EINT(8), IRQ_TYPE_EDGE_BOTH);   //TD switch
	set_irq_type(IRQ_EINT(4), IRQ_TYPE_EDGE_BOTH);   //Pen out
	set_irq_type(IRQ_EINT(25), IRQ_TYPE_EDGE_BOTH);   //Size up
	set_irq_type(IRQ_EINT(10), IRQ_TYPE_EDGE_BOTH);   //Size down
	set_irq_type(IRQ_EINT(6), IRQ_TYPE_EDGE_BOTH);   //Volume up
	set_irq_type(IRQ_EINT(3), IRQ_TYPE_EDGE_BOTH);   //Volume down
	
	#endif
	
	/* Tony add 2010-07-09 *****start***** */
	u32 gpcon,gpdat,gppud;
	#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP4_EVT)	
		/* GPE 4 */
		gpcon=__raw_readl(S3C64XX_GPECON);
		gpcon &= ~(0x0f<<16);
		gpcon |= (0x01<<16);
		__raw_writel(gpcon,S3C64XX_GPECON);
		
		gpdat=__raw_readl(S3C64XX_GPEDAT);
		gpdat |= (0x01<<4);
		__raw_writel(gpdat,S3C64XX_GPEDAT);

		gppud=__raw_readl(S3C64XX_GPEPUD);
		gppud &= ~(0x03<<8);
		__raw_writel(gppud,S3C64XX_GPEPUD);	
	#else

		/* GPO 10 */
		gpcon=__raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
		gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);
		
	//	gpdat=__raw_readl(S3C64XX_GPODAT);
	//	gpdat |= (0x01<<10);
	//	__raw_writel(gpdat,S3C64XX_GPODAT);

		gppud=__raw_readl(S3C64XX_GPOPUD);
		gppud &= ~(0x03<<20);
		__raw_writel(gppud,S3C64XX_GPOPUD);	
	#endif	
		/* GPK 9,13,14 */
		gpcon=__raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~((0x0f<<4)|(0x0f<<20)|(0x0f<<24));
		gpcon |= ((0x01<<4)|(0x01<<20)|(0x01<<24));
		__raw_writel(gpcon,S3C64XX_GPKCON1);
		
		gpdat=__raw_readl(S3C64XX_GPKDAT);
		gpdat |= ((0x01<<9)|(0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~((0x03<<18)|(0x03<<26)|(0x03<<28));
		__raw_writel(gppud,S3C64XX_GPKPUD);
	
		/* GPM 0,1,5 */
		gpcon=__raw_readl(S3C64XX_GPMCON);
		gpcon &= ~( 0x0f |(0x0f<<4)|(0x0f<<20));
		gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPMCON);
		
		gpdat=__raw_readl(S3C64XX_GPMDAT);
		gpdat |= (0x01<<5);
		__raw_writel(gpdat,S3C64XX_GPMDAT);

		gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~( 0x03 |(0x03<<2)|(0x03<<10));
		__raw_writel(gppud,S3C64XX_GPMPUD);
		
		
    /* reset wifi/bt here */
/*		gpdat =__raw_readl(S3C64XX_GPKDAT);
		gpdat &= ~((0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);
		
		mdelay(100);
	
		gpdat =__raw_readl(S3C64XX_GPKDAT);
		gpdat |= ((0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);
*/	
		gpdat =__raw_readl(S3C64XX_GPKDAT);
		gpdat &= ~(0x01<<13);
		__raw_writel(gpdat,S3C64XX_GPKDAT);
		
		mdelay(100);
	
		gpdat =__raw_readl(S3C64XX_GPKDAT);
		gpdat |= (0x01<<13);
		__raw_writel(gpdat,S3C64XX_GPKDAT);

	/* Tony add 2010-07-09 *****end***** */	

        /* Lannis add 2010-08-04 BT UART CONFIG*****start***** */
        s3c_gpio_setpull(S3C64XX_GPA(4), S3C_GPIO_PULL_UP);
        s3c_gpio_setpull(S3C64XX_GPA(5), S3C_GPIO_PULL_UP);
        s3c_gpio_setpull(S3C64XX_GPA(6), S3C_GPIO_PULL_UP);
        s3c_gpio_setpull(S3C64XX_GPA(7), S3C_GPIO_PULL_UP);
        s3c_gpio_cfgpin(S3C64XX_GPA(4), S3C_GPIO_SFN(2));
        s3c_gpio_cfgpin(S3C64XX_GPA(5), S3C_GPIO_SFN(2));
        s3c_gpio_cfgpin(S3C64XX_GPA(6), S3C_GPIO_SFN(2));
        s3c_gpio_cfgpin(S3C64XX_GPA(7), S3C_GPIO_SFN(2));

        __raw_writel(0x0003, ioremap(0x7F005400, 4));
        __raw_writel(0x0F85, ioremap(0x7F005404, 4));
        __raw_writel(0x0011, ioremap(0x7F005408, 4));
        __raw_writel(0x0000, ioremap(0x7F00540C, 4));
        __raw_writel(0x0010, ioremap(0x7F005428, 4));
        __raw_writel(0xDFDF, ioremap(0x7F00542C, 4));
        /* Lannis add 2010-08-04 *****end***** */


      /*ADD BU ALLEN 20101130  */
             /*BT_RESET  GPK13*/
                
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<20);
		__raw_writel(gpcon,S3C64XX_GPKCON1);
                  
		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);
             
               /*WIFI_RESET GPK14 */
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<24);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);
 
                /*WF_MAC_WAKE  GPO10*/
       		gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
                gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);

               gpdat = __raw_readl(S3C64XX_GPODAT);
               gpdat &= ~(0x01<<10);
               __raw_writel(gpdat,S3C64XX_GPODAT);

                /* GPA 4 5 6 7 */
                gpcon = __raw_readl(S3C64XX_GPACON);	
	        gppud = __raw_readl(S3C64XX_GPAPUD);	
	        gpdat = __raw_readl(S3C64XX_GPADAT);

		gpcon &= ~(0xffff<<16);		
		gpdat &= ~(0x0f<<4);
		gppud &= ~(0xff<<8);

               	__raw_writel(gpcon,S3C64XX_GPACON);	
	        __raw_writel(gppud,S3C64XX_GPAPUD);	
	        __raw_writel(gpdat,S3C64XX_GPADAT);

                 /*BT_WAKEUP  GPK9*/
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<4);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);


          /*BT_PCM    GPE 0 2 3 4 */

       		gpcon = __raw_readl(S3C64XX_GPECON);
		gpcon &= ~((0x0f<<0)|(0x0fff<<8));
		__raw_writel(gpcon,S3C64XX_GPECON);
               
		gppud=__raw_readl(S3C64XX_GPEPUD);
		gppud &= ~((0x03<<0)|(0x03f<<4));
		__raw_writel(gppud,S3C64XX_GPEPUD);

               /*WIFI_WAKEUP_HOST BT_WAKEUP_HOST GPM0 GPM1 */
       		gpcon = __raw_readl(S3C64XX_GPMCON);
		gpcon &= ~(0x0ff<<0);
		__raw_writel(gpcon,S3C64XX_GPMCON);
               
		gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~(0x0f<<0);
		__raw_writel(gppud,S3C64XX_GPMPUD);


/*wifi*/


 //                gpdat = __raw_readl(S3C64XX_GPODAT);
  //               gpdat &= ~(0x01<<10);
 //                __raw_writel(gpdat,S3C64XX_GPODAT);

            /* WF_SD_CMD WF_SD_CLK  GPC 4,5 */ 
       		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon &= ~((0x0f<<16)|(0x0f<<20));
		__raw_writel(gpcon,S3C64XX_GPCCON);
		
		gppud=__raw_readl(S3C64XX_GPCPUD);
		gppud &= ~(0x0f<<8);
		__raw_writel(gppud,S3C64XX_GPCPUD);
		
//		gppud=__raw_readl(S3C64XX_GPCPUD);
//		gppud &= ~(0x0f<<8);
//		__raw_writel(gppud,S3C64XX_GPCPUD);
                       
                      		/* GPH 6,7,8,9 */
		gpcon = __raw_readl(S3C64XX_GPHCON0);
		gpcon &= ~((0x0f<<24)|(0x0f<<28));
		__raw_writel(gpcon,S3C64XX_GPHCON0);
		
		gpcon = __raw_readl(S3C64XX_GPHCON1);
		gpcon &= ~(0x0f |(0x0f<<4));
		__raw_writel(gpcon,S3C64XX_GPHCON1);
		
//		gpdat=__raw_readl(S3C64XX_GPHDAT);
//		gpdat &= ~(0x0f<<6);
//		__raw_writel(gpdat,S3C64XX_GPHDAT);
		
		gppud=__raw_readl(S3C64XX_GPHPUD);
		gppud &= ~(0xff<<12);
		__raw_writel(gppud,S3C64XX_GPHPUD);
                

              printk("%d\n", !!(gpdat & (0x01<<8)));
                /*  WF_WAKEUP*/
    		gpcon = __raw_readl(S3C64XX_GPMCON);
		gpcon &= ~(0x0f<<20);
		__raw_writel(gpcon,S3C64XX_GPMCON);          

		gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~(0x3<<10);
		__raw_writel(gppud,S3C64XX_GPMPUD);

#if 1
              printk("wifi power on\n");
		/* GPC 4,5 */
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon &= ~((0x0f<<16)|(0x0f<<20));
		gpcon |= ((0x03<<16)|(0x03<<20));
		__raw_writel(gpcon,S3C64XX_GPCCON);
				
		/* GPH 6,7,8,9 */
		gpcon = __raw_readl(S3C64XX_GPHCON0);
		gpcon &= ~((0x0f<<24)|(0x0f<<28));
		gpcon |= ((0x03<<24)|(0x03<<28));	
		__raw_writel(gpcon,S3C64XX_GPHCON0);
		
		gpcon = __raw_readl(S3C64XX_GPHCON1);
		gpcon &= ~(0x0f |(0x0f<<4));
		gpcon |= (0x03 |(0x03<<4));
		__raw_writel(gpcon,S3C64XX_GPHCON1);

                /*WF_WAKE_UP*/
                gpcon = __raw_readl(S3C64XX_GPMCON);
		gpcon &= ~(0x0f<<20);
                gpcon |=(0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPMCON);
    
                gpdat = __raw_readl(S3C64XX_GPMDAT);	
		gpdat |= (0x01<<5);
                __raw_writel(gpdat,S3C64XX_GPMDAT);

		gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~(0x03<<10);
		__raw_writel(gppud,S3C64XX_GPMPUD);
  #if 0        
              /*WF_MAC_WAKE */
   //             gpcon = __raw_readl(S3C64XX_GPOCON);
//		gpcon &= ~(0x03<<20);
  //              gpcon |=(0x01<<20);
//		__raw_writel(gpcon,S3C64XX_GPOCON);

                gpdat = __raw_readl(S3C64XX_GPODAT);
                gpdat |= (0x01<<10);
                __raw_writel(gpdat,S3C64XX_GPODAT);
               mdelay(200);
         
               gpdat = __raw_readl(S3C64XX_GPODAT);
               gpdat &= ~(0x01<<10);
               __raw_writel(gpdat,S3C64XX_GPODAT);
               mdelay(200);
               gpdat = __raw_readl(S3C64XX_GPODAT);
               gpdat &= ~(0x01<<10);
               __raw_writel(gpdat,S3C64XX_GPODAT);
               mdelay(200);

//		gppud=__raw_readl(S3C64XX_GPOPUD);
//		gppud &= ~(0x03<<20);
//		__raw_writel(gppud,S3C64XX_GPOPUD);
               
 #endif                     /*WF_RESET*/
                gpcon = __raw_readl(S3C64XX_GPKCON);
		gpcon &= ~(0x0f<<24);
                gpcon |=(0x01<<24);
		__raw_writel(gpcon,S3C64XX_GPKCON);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<14);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

                mdelay(100);
 
                gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat &= ~(0x01<<14);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

                mdelay(100);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<14);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);
#endif


/*for bt*/
#if 1
              /* GPA 4 5 6 7 */
               gpcon = __raw_readl(S3C64XX_GPACON);	
//	       gppud = __raw_readl(S3C64XX_GPAPUD);	
	       gpdat = __raw_readl(S3C64XX_GPADAT);

		gpcon &= ~(0xffff<<16);	
		gpcon |= ((0x02<<16)|(0x02<<20)|(0x02<<24)|(0x02<<28));
//		gppud |= ((0x01<<8)|(0x01<<10)|(0x01<<12)|(0x01<<14));
  
               	__raw_writel(gpcon,S3C64XX_GPACON);	
//	        __raw_writel(gppud,S3C64XX_GPAPUD);	
//	        __raw_writel(gpdat,S3C64XX_GPADAT);
               
                /*BT_WAKEUP*/
                gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<4);
                gpcon |=(0x01<<4);
		__raw_writel(gpcon,S3C64XX_GPKCON1);
    
                gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<9);
                __raw_writel(gpdat,S3C64XX_GPKDAT);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);

              /*WF_MAC_WAKE*/
  //              gpcon = __raw_readl(S3C64XX_GPOCON);
//		gpcon &= ~(0x03<<20);
  //              gpcon |=(0x01<<20);
//		__raw_writel(gpcon,S3C64XX_GPOCON);

  //              gpdat = __raw_readl(S3C64XX_GPODAT);
  //              gpdat |= (0x01<<10);
 //               __raw_writel(gpdat,S3C64XX_GPODAT);

//		gppud=__raw_readl(S3C64XX_GPOPUD);
//		gppud &= ~(0x03<<20);
//		__raw_writel(gppud,S3C64XX_GPOPUD);

                      /*BT_RESET*/
                gpcon = __raw_readl(S3C64XX_GPKCON);
		gpcon &= ~(0x0f<<20);
                gpcon |=(0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPKCON);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<13);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

                mdelay(100);
 
                gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat &= ~(0x01<<13);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

                mdelay(100);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<13);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);


#endif
         
/* ADD BY ALLEN END */


}

