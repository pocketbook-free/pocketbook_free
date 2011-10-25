/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>

#include <linux/io.h> /* Tony add 2010-07-09 */
#include <plat/regs-clock.h>
#include <mach/gpio.h>//Lucifer
#include <asm/gpio.h>
#include <asm/io.h>
#include <plat/regs-gpio.h>
#include <plat/input_key.h>
#include <plat/gpio-cfg.h>

#ifdef CONFIG_STOP_MODE_SUPPORT
#include <linux/earlysuspend.h>
#endif

#include <linux/wakelock.h>
#include "../../../drivers/input/keyboard/key_for_ioctl.h"
static struct wake_lock key_delayed_work_wake_lock;
//unsigned long suspend_routine_key;
extern unsigned int suspend_routine_flag;
extern unsigned int nosuspend_routine_keycode_flag;
unsigned int nosuspend_resume_level1=0;	
unsigned int nosuspend_resume_level2=0;	
extern unsigned long wakeup_key;
//extern struct wake_lock main_wake_lock;

#define POWER_KEY_GPIO		144 + 1
#define TS_PEN_DET_GPIO		137 + 4

#define GPIOKEY_STATE_IDLE		0
#define GPIOKEY_STATE_DEBOUNCE		1
#define GPIOKEY_STATE_PRESSED		2

struct input_dev *for_morse_key_input; //for morse button

extern int bt_power(int i);


struct gpio_button_data {
	struct gpio_keys_button *button;
	struct input_dev *input;
	struct timer_list timer;
	int pressed_time;
	int gk_state;
	spinlock_t lock;
};


struct gpio_keys_drvdata {
	struct input_dev *input;
	struct gpio_button_data data[0];
};


/* Tony add 2010-07-09 *****start***** */

static int s3c_gpio_key_show_wifi_wakeup_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPMDAT);

	return snprintf(buffer, PAGE_SIZE, "%d\n", !!(gpdat & (0x01<<5)));
}		

static int s3c_gpio_key_store_wifi_wakeup_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	
	if (len < 1)
		return -EINVAL;
	
	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		;	
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		;
		
	}
	else
		return -EINVAL;

/*		
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		__raw_writel(gpdat |= (0x01<<5), ioremap(0x7F005400, 4));	
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		__raw_writel(gpdat &= ~(0x01<<5), ioremap(0x7F005400, 4));
		
	}
	else
		return -EINVAL;
*/

	return len;
}		

static int s3c_gpio_key_show_bt_wakeup_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPKDAT);

	return snprintf(buffer, PAGE_SIZE, "%d\n", !!(gpdat & (0x01<<9)));
}		

static int s3c_gpio_key_store_bt_wakeup_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u32 gpdat;
	
	if (len < 1)
		return -EINVAL;
		
	gpdat = __raw_readl(S3C64XX_GPKDAT);	

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		gpdat |= (0x01<<9);
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		gpdat &= ~(0x01<<9);
		
	}
	else
		return -EINVAL;
	
	__raw_writel(gpdat,S3C64XX_GPKDAT);	

	return len;
}		

static int s3c_gpio_key_show_wf_mac_wake(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPODAT);

	return snprintf(buffer, PAGE_SIZE, "%d\n", !!(gpdat & (0x01<<10)));
}		

static int s3c_gpio_key_store_wf_mac_wake(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	
	if (len < 1)
		return -EINVAL;
    
	
	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		bt_power(1);	
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		bt_power(0);
		
	}
	else
		return -EINVAL;

/*		
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		__raw_writel(gpdat |= (0x01<<5), ioremap(0x7F005400, 4));	
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		__raw_writel(gpdat &= ~(0x01<<5), ioremap(0x7F005400, 4));
		
	}
	else
		return -EINVAL;
*/

	return len;
}		

static int s3c_gpio_key_show_wifi_rst_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPKDAT);

	return snprintf(buffer, PAGE_SIZE, "%d\n", !!(gpdat & (0x01<<14)));
}		

static int s3c_gpio_key_store_wifi_rst_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u32 gpdat;
	
	if (len < 1)
		return -EINVAL;
	
	gpdat = __raw_readl(S3C64XX_GPKDAT);	

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		gpdat |= (0x01<<14);
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		gpdat &= ~(0x01<<14);
	}
	else
		return -EINVAL;

  __raw_writel(gpdat,S3C64XX_GPKDAT);

	return len;
}		

static int s3c_gpio_key_show_bt_rst_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPKDAT);

	return snprintf(buffer, PAGE_SIZE, "%d\n", !!(gpdat & (0x01<<13)));
}		

static int s3c_gpio_key_store_bt_rst_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u32 gpdat;
	
	if (len < 1)
		return -EINVAL;
		
	gpdat = __raw_readl(S3C64XX_GPKDAT);	

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		gpdat |= (0x01<<13);
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		gpdat &= ~(0x01<<13);
	}
	else
		return -EINVAL;
		
	__raw_writel(gpdat,S3C64XX_GPKDAT);	

	return len;
}		

static int s3c_gpio_key_show_bt_uart_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPACON);

	return snprintf(buffer, PAGE_SIZE, "%x\n", gpdat);
}		

static int s3c_gpio_key_store_bt_uart_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u32 gpdat,gpcon,gppud;
	
	if (len < 1)
		return -EINVAL;
		
	gpcon = __raw_readl(S3C64XX_GPACON);	
	gppud = __raw_readl(S3C64XX_GPAPUD);	
	gpdat = __raw_readl(S3C64XX_GPADAT);
	
	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		gpcon &= ~(0xffff<<16);
		gpcon |= ((0x02<<16)|(0x02<<20)|(0x02<<24)|(0x02<<28));
		gppud |= ((0x01<<8)|(0x01<<10)|(0x01<<12)|(0x01<<14));
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		gpcon &= ~(0xffff<<16);		
		gpdat &= ~(0x0f<<4);
		gppud &= ~(0xff<<8);

	}
	else
		return -EINVAL;
		
	__raw_writel(gpcon,S3C64XX_GPACON);	
	__raw_writel(gppud,S3C64XX_GPAPUD);	
	__raw_writel(gpdat,S3C64XX_GPADAT);	

	return len;
}
		
static int wifi_sleep_flag=0;

static int s3c_gpio_key_show_wifi_sleep_status(struct device *dev, struct device_attribute *attr, char *buffer)
{

	return snprintf(buffer, PAGE_SIZE, "%d\n", wifi_sleep_flag);
}		

static int s3c_gpio_key_store_wifi_sleep_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u32 gpdat,gpcon,gppud;
	
	if (len < 1)
		return -EINVAL;
	
	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		
		if(wifi_sleep_flag==0)
			return len;
			
		/* GPK 9,13,14 */
		gpcon=__raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~((0x0f<<4)|(0x0f<<20)|(0x0f<<24));
		gpcon |= ((0x01<<4)|(0x01<<20)|(0x01<<24));
		__raw_writel(gpcon,S3C64XX_GPKCON1);
		
		gpdat=__raw_readl(S3C64XX_GPKDAT);
		gpdat |= ((0x01<<9)|(0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);

		/* GPM 0,1,5 */
		gpcon=__raw_readl(S3C64XX_GPMCON);
		gpcon &= ~(0x0f<<20);
		gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPMCON);
		
		gpdat=__raw_readl(S3C64XX_GPMDAT);
		gpdat |= (0x01<<5);
		__raw_writel(gpdat,S3C64XX_GPMDAT);
		
		/* GPE 4 */
		gpcon=__raw_readl(S3C64XX_GPECON);
		gpcon &= ~(0x0f<<16);
		gpcon |= (0x01<<16);
		__raw_writel(gpcon,S3C64XX_GPECON);
		
		gpdat=__raw_readl(S3C64XX_GPEDAT);
		gpdat |= (0x01<<4);
		__raw_writel(gpdat,S3C64XX_GPEDAT);
	
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
		
		/* RESET wifi/bt here */
		gpdat=__raw_readl(S3C64XX_GPKDAT);
		gpdat &= ~((0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);
		
		mdelay(100);
		
		gpdat=__raw_readl(S3C64XX_GPKDAT);
		gpdat |= ((0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);
				
		wifi_sleep_flag=0;
	}
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0){
		
		if(wifi_sleep_flag==1)
			return len;
		
		/* GPK 9,13,14 */
		gpcon=__raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~((0x0f<<4)|(0x0f<<20)|(0x0f<<24));
		__raw_writel(gpcon,S3C64XX_GPKCON1);
		
		gpdat=__raw_readl(S3C64XX_GPKDAT);
		gpdat &= ~((0x01<<9)|(0x01<<13)|(0x01<<14));
		__raw_writel(gpdat,S3C64XX_GPKDAT);

		gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~((0x03<<18)|(0x03<<26)|(0x03<<28));
		__raw_writel(gppud,S3C64XX_GPKPUD);
	
		/* GPM 0,1,5 */
		gpcon=__raw_readl(S3C64XX_GPMCON);
		gpcon &= ~( 0x0f |(0x0f<<4)|(0x0f<<20));
		__raw_writel(gpcon,S3C64XX_GPMCON);
		
		gpdat=__raw_readl(S3C64XX_GPMDAT);
		gpdat &= ~( 0x01 |(0x01<<1)|(0x01<<5));
		__raw_writel(gpdat,S3C64XX_GPMDAT);

		gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~( 0x03 |(0x03<<2)|(0x03<<10));
		__raw_writel(gppud,S3C64XX_GPMPUD);
		
		/* GPE 4 */
		gpcon=__raw_readl(S3C64XX_GPECON);
		gpcon &= ~(0x0f<<16);
		__raw_writel(gpcon,S3C64XX_GPECON);
		
		gpdat=__raw_readl(S3C64XX_GPEDAT);
		gpdat &= ~(0x01<<4);
		__raw_writel(gpdat,S3C64XX_GPEDAT);

		gppud=__raw_readl(S3C64XX_GPEPUD);
		gppud &= ~(0x03<<8);
		__raw_writel(gppud,S3C64XX_GPEPUD);	
		
		/* GPA 4,5,6,7 */
		gpcon = __raw_readl(S3C64XX_GPACON);
		gpcon &= ~(0xffff<<16);
		__raw_writel(gpcon,S3C64XX_GPACON);
		
		gpdat=__raw_readl(S3C64XX_GPADAT);
		gpdat &= ~(0x0f<<4);
		__raw_writel(gpdat,S3C64XX_GPADAT);
		
		gppud=__raw_readl(S3C64XX_GPAPUD);
		gppud &= ~(0xff<<8);
		__raw_writel(gppud,S3C64XX_GPAPUD);	
		
		/* GPC 4,5 */
		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon &= ~((0x0f<<16)|(0x0f<<20));
		__raw_writel(gpcon,S3C64XX_GPCCON);
		
		gpdat=__raw_readl(S3C64XX_GPCDAT);
		gpdat &= ~(0x03<<4);
		__raw_writel(gpdat,S3C64XX_GPCDAT);
		
		gppud=__raw_readl(S3C64XX_GPCPUD);
		gppud &= ~(0x0f<<8);
		__raw_writel(gppud,S3C64XX_GPCPUD);	
		
		/* GPH 6,7,8,9 */
		gpcon = __raw_readl(S3C64XX_GPHCON0);
		gpcon &= ~((0x0f<<24)|(0x0f<<28));
		__raw_writel(gpcon,S3C64XX_GPHCON0);
		
		gpcon = __raw_readl(S3C64XX_GPHCON1);
		gpcon &= ~(0x0f |(0x0f<<4));
		__raw_writel(gpcon,S3C64XX_GPHCON1);
		
		gpdat=__raw_readl(S3C64XX_GPHDAT);
		gpdat &= ~(0x0f<<6);
		__raw_writel(gpdat,S3C64XX_GPHDAT);
		
		gppud=__raw_readl(S3C64XX_GPHPUD);
		gppud &= ~(0xff<<12);
		__raw_writel(gppud,S3C64XX_GPHPUD);	
		
		wifi_sleep_flag=1;

	}
	else
		return -EINVAL;
		
	return len;
}		

static int td_power_show(struct device *dev, struct device_attribute *attr, char *buffer)
{
	u32 gpdat = __raw_readl(S3C64XX_GPKDAT);

	return snprintf(buffer, PAGE_SIZE, "%s\n", !!(gpdat & (0x01<<11))?"on":"off");
}		

extern int wakeup_suspend_control;		//20100121
static int td_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char *p;
	int len;

	p = memchr(buf, '\n', count);
	len = p ? p - buf : count;
        
	if (len == 1 && !strncmp(buf, "1", len)) 
	{ 
	//GPK15 as output low,open the power
		s3c_gpio_setpull(S3C64XX_GPK(11),  2);
		s3c_gpio_cfgpin(S3C64XX_GPK(11),  1);
		gpio_set_value(S3C64XX_GPK(11),	1); 

    }
    else if(len == 1 && !strncmp(buf, "0", len))  
    {
	//GPK15 as output high,close the power
		s3c_gpio_setpull(S3C64XX_GPK(11),  1);
		s3c_gpio_cfgpin(S3C64XX_GPK(11),  1);
		gpio_set_value(S3C64XX_GPK(11),	0); 
		
	}
//20100121
    else if(len == 1 && !strncmp(buf, "2", len))  
    {
	wakeup_suspend_control = 0;	
	printk("no autosuspend\n");
	}
    else if(len == 1 && !strncmp(buf, "3", len))  
    {
	wakeup_suspend_control = 1;	
	printk("autosuspend\n");
	}
    else if(len == 1 && !strncmp(buf, "4", len))  
    {
	printk("%d\n", wakeup_suspend_control);
	}
	
	return count;
}	
static int eink_power_show(struct device *dev, struct device_attribute *attr, char *buffer)
{

	u32 gpdat = __raw_readl(S3C64XX_GPCDAT);

	return snprintf(buffer, PAGE_SIZE, "EINK_3.3_state= %s,eink_1.8_state=%s\n", !!(gpdat & (0x01<<7))?"off":"on",!!(gpdat & (0x01<<6))?"off":"on");
}		

static int eink_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char *p;
	int len;
      u32 gpdat;
	p = memchr(buf, '\n', count);
	len = p ? p - buf : count;
        
	if (len == 1 && !strncmp(buf, "1", len)) 
	{ 
	//GPC6,GPC7 as output low,open the power
		gpdat=__raw_readl(S3C64XX_GPCDAT);
		gpdat &= ~(0x03<<6);
		__raw_writel(gpdat,S3C64XX_GPCDAT);
    }
    else if(len == 1 && !strncmp(buf, "0", len))  
    {
	//GPC6,GPC7 as output high,close the power
	    gpdat=__raw_readl(S3C64XX_GPCDAT);
		gpdat &= ~(0x03<<6);
		gpdat |= (0x03<<6);
		__raw_writel(gpdat,S3C64XX_GPCDAT);
	}
	return count;
}	
static DEVICE_ATTR(td_power, 0644,
			td_power_show,
			td_power_store);
static DEVICE_ATTR(eink_power, 0644,
			eink_power_show,
			eink_power_store);			
static DEVICE_ATTR(wifi_wakeup_status, 0644,
			s3c_gpio_key_show_wifi_wakeup_status,
			s3c_gpio_key_store_wifi_wakeup_status);
			
static DEVICE_ATTR(bt_wakeup_status, 0644,
			s3c_gpio_key_show_bt_wakeup_status,
			s3c_gpio_key_store_bt_wakeup_status);			
			
static DEVICE_ATTR(wf_mac_wake, 0644,
			s3c_gpio_key_show_wf_mac_wake,
			s3c_gpio_key_store_wf_mac_wake);			
			
static DEVICE_ATTR(wifi_rst_status, 0644,
			s3c_gpio_key_show_wifi_rst_status,
			s3c_gpio_key_store_wifi_rst_status);			

static DEVICE_ATTR(bt_rst_status, 0644,
			s3c_gpio_key_show_bt_rst_status,
			s3c_gpio_key_store_bt_rst_status);

static DEVICE_ATTR(bt_uart_status,0644,
			s3c_gpio_key_show_bt_uart_status,
			s3c_gpio_key_store_bt_uart_status);
			
static DEVICE_ATTR(wifi_sleep_status,0644,
			s3c_gpio_key_show_wifi_sleep_status,
			s3c_gpio_key_store_wifi_sleep_status);
			
/* Tony add 2010-07-09 *****end***** */			


static void gpio_keys_sw_event(struct gpio_button_data *bdata)
{
	struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ? : EV_SW;
	int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;

	input_event(input, type, button->code, !!state);
	input_sync(input);
	mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
}

static int group_key_code=0;

static void judge_5_way_code(struct gpio_button_data *bdata)
{
	struct gpio_keys_button *button = bdata->button;
	int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;
	//printk("---------------judge 5 way key-------------\n");

	if(button->code == GPIO_5WAY_KEY  && state == 1)// 5 way key at low,alway interrupt
	{
		if(!gpio_get_value(S3C64XX_GPF(0))) {//key 5 press
			group_key_code = GPIO_5_LEFT; 
		}
		else if(!gpio_get_value(S3C64XX_GPF(1))) {//key 6 press
			group_key_code = GPIO_5_UP; 
		}
		else if(!gpio_get_value(S3C64XX_GPF(2))) {//key 7 press
			group_key_code = GPIO_5_DOWN; 
		}
		else if(!gpio_get_value(S3C64XX_GPF(3))) {//key 8 press
			group_key_code = GPIO_5_ENTER; 
		}
		else if(!gpio_get_value(S3C64XX_GPF(4))) {//key 9 press
			group_key_code = GPIO_5_RIGHT; 
		}
	}
}

extern int get_global_usb_plug_in(void);
static void gpio_keys_key_event(struct gpio_button_data *bdata)
{
	struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ? : EV_KEY;
	int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;

	//printk(KERN_ALERT "---group_key_code=%x\n", group_key_code);

  //printk("gpio:%d, state:%d, gk_state:%d, pressed_time:%d\n", button->gpio, state, bdata->gk_state, bdata->pressed_time);
	//printk("bdata->gk_state=%d\n",bdata->gk_state);
	switch (bdata->gk_state)
	{
		case GPIOKEY_STATE_IDLE:
			break;

		case GPIOKEY_STATE_DEBOUNCE:
			
			if (state)
			{
				{
					//if(suspend_routine_flag==1)
					//{
					//wakeup_key=getwakeupsource();
					//nosuspend_resume_flag=1;	
					//}
				
					printk("*********************hi,debounce_keys,gpio=%d*********************\n",button->gpio);
					//if(gpio_get_value(S3C64XX_GPM(3)) == 0)
					if (get_global_usb_plug_in() == 0)					
					{
			 
						wakeup_key=getwakeupsource();
						if(suspend_routine_flag==1)
						{
							nosuspend_resume_level1=1;	
							if(nosuspend_routine_keycode_flag==1)
							{
								nosuspend_resume_level2=1;
							}
						}	
					}
					if(button->code == GPIO_5WAY_KEY) {
							//printk(KERN_ALERT "GPIOKEY_STATE_DEBOUNCE group_key_code=%x\n", group_key_code);
							//printk(KERN_ALERT "GPIOKEY_STATE_DEBOUNCE:keycode=%x,irq=%d\n",group_key_code,gpio_to_irq(button->gpio));
							input_event(input, type, group_key_code, !!state);
							input_sync(input);
					} else {
						    //printk(KERN_ALERT "GPIOKEY_STATE_DEBOUNCE:%s,keycode=%x,irq=%d,state=%d\n",button->desc,button->code,gpio_to_irq(button->gpio),state);
							input_event(input, type, button->code, !!state);
							input_sync(input);
					}
				}
				bdata->pressed_time = 0;
				bdata->gk_state = GPIOKEY_STATE_PRESSED;
				mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
			}
			else
			{
				enable_irq(gpio_to_irq(button->gpio));
				bdata->gk_state = GPIOKEY_STATE_IDLE;
			}
			
			wake_unlock(&key_delayed_work_wake_lock);
			break;

		case GPIOKEY_STATE_PRESSED:
			if (state)
			{
				if (bdata->pressed_time >= button->longpress_time)
				{
					{
						if(button->code == GPIO_5WAY_KEY) {
							//printk(KERN_ALERT "GPIOKEY_STATE_PRESSED still group_key_code=%x\n", group_key_code);
							//printk(KERN_ALERT "GPIOKEY_STATE_DEBOUNCE:keycode=%x,irq=%d\n",group_key_code,gpio_to_irq(button->gpio));
							input_event(input, type, group_key_code, 2);
							input_sync(input);
						} else {
							//printk(KERN_ALERT "GPIOKEY_STATE_PRESSED still:%s,keycode=%x,irq=%d,state=%d\n",button->desc,button->code,gpio_to_irq(button->gpio),state);
							input_event(input, type, button->code, 2);
							input_sync(input);
						}
					}
					bdata->pressed_time = 0;
				}
				else
				{
					bdata->pressed_time += button->timer_interval;
				}
				mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
			}
			else
			{
				{
					if(button->code == GPIO_5WAY_KEY) {
							//printk(KERN_ALERT "GPIOKEY_STATE_PRESSED release group_key_code=%x\n", group_key_code);
							//printk(KERN_ALERT "GPIOKEY_STATE_DEBOUNCE:keycode=%x,irq=%d\n",group_key_code,gpio_to_irq(button->gpio));
							input_event(input, type, group_key_code, !!state);
							input_sync(input); 
					} else {
							//printk(KERN_ALERT "GPIOKEY_STATE_PRESSED release:%s,keycode=%x,irq=%d,state=%d\n",button->desc,button->code,gpio_to_irq(button->gpio),state);
							input_event(input, type, button->code, !!state);
							input_sync(input);
					}
				}
				bdata->pressed_time = 0;
				enable_irq(gpio_to_irq(button->gpio));
	
				if (button->gpio == POWER_KEY_GPIO)
				{
					__raw_writel(1UL << (IRQ_EINT(1) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
				}
				
				if (button->gpio == TS_PEN_DET_GPIO)
				{
					__raw_writel(1UL << (IRQ_EINT(27) - IRQ_EINT(0)), S3C64XX_EINT0PEND);
				}

				bdata->gk_state = GPIOKEY_STATE_IDLE;
			}
			break;

		default:
			break;
	}
}

#if defined (CONFIG_HW_EP3_DVT) || defined (CONFIG_HW_EP1_DVT)
extern void hanwang_touch_power_off(void);
extern void hanwang_touch_power_on(void);
#endif

static irqreturn_t gpio_keys_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	BUG_ON(irq != gpio_to_irq(button->gpio));
//	printk("*********************gpio_keys_isr %s,gpio=%d*********************\n", button->desc,button->gpio);

	//added to solve the problem that the pen_out key event can only be reported once.   blazer 2010.09.21
	static int up = 0, down = 0;
	int state;
    
	if (button->gpio == S3C64XX_GPN(4))	
	{
		//printk("pen_out_key, state=%d,irq=%d\n", gpio_get_value(button->gpio),gpio_to_irq(button->gpio));		
		state = (gpio_get_value(S3C64XX_GPN(4)) ? 1 : 0) ^ button->active_low;	
#if defined (CONFIG_HW_EP3_DVT) || defined (CONFIG_HW_EP1_DVT)
		if (state == 1)
		    hanwang_touch_power_off();
		else
            hanwang_touch_power_on();
#endif
		if (!!state)		
		{		
			input_event(input, button->type, button->code, up);		
			input_sync(input);		
			up = !up;		
		}		
		else		
		{		
			input_event(input, button->type, button->code, down);		
			input_sync(input);		
			down = !down;		
		}		
		return 0;	
	}
	if (button->debounce_interval)
	{
		if (gpio_get_value(button->gpio))
		{
					if(suspend_routine_flag==1)
					{
					nosuspend_resume_level1=1;	
					if(nosuspend_routine_keycode_flag==1)
					{
					nosuspend_resume_level2=1;
					}
					}	
			return IRQ_HANDLED;
		}
		bdata->gk_state = GPIOKEY_STATE_DEBOUNCE;

		disable_irq(irq);
		wake_lock(&key_delayed_work_wake_lock);
		if(button->code == GPIO_5WAY_KEY) {
				judge_5_way_code(bdata);
		}
		
		mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->debounce_interval));
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_STOP_MODE_SUPPORT
static struct platform_device *gpio_keys_pdev = NULL;
static int gpio_keys_suspend(struct platform_device *pdev, pm_message_t state);

static void gpio_keys_early_suspend(struct early_suspend *h)
{
        if (gpio_keys_pdev)
        {
                gpio_keys_suspend(gpio_keys_pdev, PMSG_SUSPEND);
        }
}

static void gpio_keys_late_resume(struct early_suspend *h)
{
	return;
}

struct early_suspend gpio_keys_early_suspend_desc = {
        .level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
        .suspend = gpio_keys_early_suspend,
        .resume = gpio_keys_late_resume,
};
#endif

void s3c_gpio_keys_init(void);


static int __devinit gpio_keys_probe(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_drvdata *ddata;
	struct input_dev *input;
	int i, error;
	int wakeup = 0;

	/* Tony add 2010-07-09 *****start***** */
	int ret=0;
	
	ret = device_create_file(&(pdev->dev), &dev_attr_td_power);
	if (ret < 0){
		printk(KERN_WARNING "td_power: failed to add entries\n");			
  }
    ret = device_create_file(&(pdev->dev), &dev_attr_eink_power);
	if (ret < 0){
		printk(KERN_WARNING "eink_power: failed to add entries\n");			
  }
  
	ret = device_create_file(&(pdev->dev), &dev_attr_wifi_wakeup_status);
	if (ret < 0){
		printk(KERN_WARNING "wifi_wakeup_status: failed to add entries\n");			
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_bt_wakeup_status);
	if (ret < 0){
		printk(KERN_WARNING "bt_wakeup_status: failed to add entries\n");			
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_wf_mac_wake);
	if (ret < 0){
		printk(KERN_WARNING "wf_mac_wake: failed to add entries\n");			
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_wifi_rst_status);
	if (ret < 0){
		printk(KERN_WARNING "wifi_rst_status: failed to add entries\n");			
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_bt_rst_status);
	if (ret < 0){
		printk(KERN_WARNING "bt_rst_status: failed to add entries\n");			
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_bt_uart_status);
	if (ret < 0){
		printk(KERN_WARNING "bt_uart_status: failed to add entries\n");
  }
  
  ret = device_create_file(&(pdev->dev), &dev_attr_wifi_sleep_status);
	if (ret < 0){
		printk(KERN_WARNING "wifi_sleep_status: failed to add entries\n");
  }
  /* Tony add 2010-07-09 *****end***** */	
//<<<<<<< .mine
//=======
  
  	/*  blazer add 2010.08.12	*/
	printk("~~~~~~~~~~~~~~Blazer:gpio key driver for EP series~~~~~~~~~~~~~~~~~\n");
//>>>>>>> .r147
	printk("nbutton = %d\n", pdata->nbuttons);
  /*  blazer add 2010.08.12	*/

	s3c_gpio_keys_init();
	
	//enable_irq(IRQ_EINT(4));

	ddata = kzalloc(sizeof(struct gpio_keys_drvdata) +
			pdata->nbuttons * sizeof(struct gpio_button_data), GFP_KERNEL);
	input = input_allocate_device();
	if (!ddata || !input) 
	{
		error = -ENOMEM;
		goto fail1;
	}

	platform_set_drvdata(pdev, ddata);

	input->name = pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	ddata->input = input;
	for_morse_key_input = input;   //for morse

	for (i = 0; i < pdata->nbuttons; i++) 
	{
		struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];
		int irq;
		unsigned int type = button->type ?: EV_KEY;

		bdata->input = input;
		bdata->button = button;
		bdata->gk_state = GPIOKEY_STATE_IDLE;
	
		if (button->type == EV_SW)
		{
			//printk("----------------------------------------\n");
			setup_timer(&bdata->timer, gpio_keys_sw_event, (unsigned long)bdata);
			mod_timer(&bdata->timer,jiffies + msecs_to_jiffies(200));
			//printk("button->type == EV_SW\n");
			//printk("button->desc=%s,button->code=%x\n",button->desc,button->code);
			//printk("----------------------------------------\n");
		}
		else
		{
			setup_timer(&bdata->timer, gpio_keys_key_event, (unsigned long)bdata);
		}
		if (button->type != EV_SW)
		{
			//printk("----------------------------------------\n");
			//printk("button->type == EV_KEY\n");
			irq = gpio_to_irq(button->gpio);
			//printk("button->desc=%s,button->code=%x\n",button->desc,button->code);
			//printk("irq=%d\n",irq);
			//printk("----------------------------------------\n");
			if (irq < 0) 
			{
				error = irq;
				pr_err("gpio-keys: Unable to get irq number"
							" for GPIO %d, error %d\n", button->gpio, error);
				gpio_free(button->gpio);
				goto fail2;
			}


			error = request_irq(irq, gpio_keys_isr, IRQ_DISABLED,
							    button->desc ? button->desc : "gpio_keys", bdata);
			if (error) 
			{
				pr_err("gpio-keys: Unable to claim irq %d; error %d\n", irq, error);
				gpio_free(button->gpio);
				goto fail2;
			}		

		}

		if (button->wakeup)
			wakeup = 1;
		input_set_capability(input, type, button->code);
	}//end of for
	
	/*
		blazer add 2010.08.16
	*/
	input_set_capability(input, EV_KEY, GPIO_5_UP);
	input_set_capability(input, EV_KEY, GPIO_5_DOWN);
	input_set_capability(input, EV_KEY, GPIO_5_LEFT);
	input_set_capability(input, EV_KEY, GPIO_5_RIGHT);
	input_set_capability(input, EV_KEY, GPIO_5_ENTER);

	input_set_capability(input, EV_KEY, GPIO_PAGE_DOWN);
//	input_set_capability(input, EV_KEY, GPIO_5_ENTER);
	input_set_capability(input, EV_KEY, GPIO_PAGE_UP);
	input_set_capability(input, EV_KEY, GPIO_HOME);

	error = input_register_device(input);
	if (error) 
	{
		pr_err("gpio-keys: Unable to register input device, " "error: %d\n", error);
		goto fail2;
	}

	device_init_wakeup(&pdev->dev, wakeup);

#ifdef CONFIG_STOP_MODE_SUPPORT
        gpio_keys_pdev = pdev;
        register_early_suspend(&gpio_keys_early_suspend_desc);
#endif

	return 0;

 fail2:
	while (--i >= 0) 
	{
		free_irq(gpio_to_irq(pdata->buttons[i].gpio), &ddata->data[i]);
		if (pdata->buttons[i].debounce_interval)
			del_timer_sync(&ddata->data[i].timer);
		gpio_free(pdata->buttons[i].gpio);
	}

	platform_set_drvdata(pdev, NULL);
 fail1:
	input_free_device(input);
	kfree(ddata);

	return error;
}

static int __devexit gpio_keys_remove(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;
	int i;
	
#ifdef CONFIG_STOP_MODE_SUPPORT
        gpio_keys_pdev = NULL;
        unregister_early_suspend(&gpio_keys_early_suspend_desc);
#endif

	device_init_wakeup(&pdev->dev, 0);
	
	for (i = 0; i < pdata->nbuttons; i++) 
	{
		int irq = gpio_to_irq(pdata->buttons[i].gpio);
		free_irq(irq, &ddata->data[i]);
		if (pdata->buttons[i].debounce_interval)
		del_timer_sync(&ddata->data[i].timer);
		gpio_free(pdata->buttons[i].gpio);
	}
	
	input_unregister_device(input);
	return 0;
}


#ifdef CONFIG_PM
static int gpio_keys_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	int i;
	
	if (device_may_wakeup(&pdev->dev)) 
	{
		for (i = 0; i < pdata->nbuttons; i++) 
		{
			struct gpio_keys_button *button = &pdata->buttons[i];
			if (button->wakeup) 
			{
				int irq = gpio_to_irq(button->gpio);
				enable_irq_wake(irq);
			}
		}
	}
	if(nosuspend_resume_level1==0)
	suspend_routine_flag=0;
	else	
	nosuspend_resume_level1=0;	  
	
	if(nosuspend_resume_level2==0)
	nosuspend_routine_keycode_flag=0;
	else
	nosuspend_resume_level2=0;	  
	

	//Now the Power key irq is disabled in wakelock.c before enter sleep progress
	//So here do nothing
//	disable_irq(gpio_to_irq(TS_PEN_DET_GPIO));
	return 0;
}


static int gpio_keys_resume(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	int i;
	
	if (device_may_wakeup(&pdev->dev)) 
	{
		for (i = 0; i < pdata->nbuttons; i++) 
		{
			struct gpio_keys_button *button = &pdata->buttons[i];
			if (button->wakeup) 
			{
				int irq = gpio_to_irq(button->gpio);
				disable_irq_wake(irq);
			}
		}
	}
	return 0;
}
#else
#define gpio_keys_suspend	NULL
#define gpio_keys_resume	NULL
#endif


static struct platform_driver gpio_keys_device_driver = 
{
	.probe		= gpio_keys_probe,
	.remove		= __devexit_p(gpio_keys_remove),
	.suspend	= gpio_keys_suspend,
	.resume		= gpio_keys_resume,
	.driver		= 
	{
		.name	= "gpio-keys-blazer",
		.owner	= THIS_MODULE,
	}
};


static int __init gpio_keys_init(void)
{

	wake_lock_init(&key_delayed_work_wake_lock, WAKE_LOCK_SUSPEND, "key_delayed_work");
	return platform_driver_register(&gpio_keys_device_driver);
}


static void __exit gpio_keys_exit(void)
{
	platform_driver_unregister(&gpio_keys_device_driver);
}


module_init(gpio_keys_init);
module_exit(gpio_keys_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for CPU GPIOs");
MODULE_ALIAS("platform:gpio-keys");



/*20101129 add by allen*/


int wifi_power(int i)
{
    u32 gpdat, gpcon;

         if(i==0){        
                 printk("wifi power off\n");

               /*WIFI_RESET GPK14 */
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<24);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

                     /*WF_MAC_WAKE  GPO10*/
       		gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
                gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);

               gpdat = __raw_readl(S3C64XX_GPODAT);
               gpdat &= ~(0x01<<10);
               __raw_writel(gpdat,S3C64XX_GPODAT);


 //                gpdat = __raw_readl(S3C64XX_GPODAT);
  //               gpdat &= ~(0x01<<10);
 //                __raw_writel(gpdat,S3C64XX_GPODAT);

            /* WF_SD_CMD WF_SD_CLK  GPC 4,5 */ 
       		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon &= ~((0x0f<<16)|(0x0f<<20));
		__raw_writel(gpcon,S3C64XX_GPCCON);
		
		/*gppud=__raw_readl(S3C64XX_GPCPUD);
		gppud &= ~(0x0f<<8);
		__raw_writel(gppud,S3C64XX_GPCPUD);*/
		
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
                
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<20);
		__raw_writel(gpcon,S3C64XX_GPKCON1);
                  
		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/
 
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
	        //gppud = __raw_readl(S3C64XX_GPAPUD);	
	        gpdat = __raw_readl(S3C64XX_GPADAT);

		gpcon &= ~(0xffff<<16);		
		gpdat &= ~(0x0f<<4);
		//gppud &= ~(0xff<<8);

               	__raw_writel(gpcon,S3C64XX_GPACON);	
	        //__raw_writel(gppud,S3C64XX_GPAPUD);	
	        __raw_writel(gpdat,S3C64XX_GPADAT);

                 /*BT_WAKEUP  GPK9*/
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<4);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

   }
    
     else  if(i==1){
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

		/*gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~(0x03<<10);
		__raw_writel(gppud,S3C64XX_GPMPUD);*/
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

                mdelay(200);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<14);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

              /* GPA 4 5 6 7 */
               gpcon = __raw_readl(S3C64XX_GPACON);	
//	       gppud = __raw_readl(S3C64XX_GPAPUD);	
	      // gpdat = __raw_readl(S3C64XX_GPADAT);

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

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

              /*WF_MAC_WAKE*/
                gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
                gpcon |=(0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);

                gpdat = __raw_readl(S3C64XX_GPODAT);
                gpdat |= (0x01<<10);
                __raw_writel(gpdat,S3C64XX_GPODAT);

		/*gppud=__raw_readl(S3C64XX_GPOPUD);
		gppud &= ~(0x03<<20);
		__raw_writel(gppud,S3C64XX_GPOPUD);*/

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

                mdelay(200);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<13);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

    }
	else
		return -EINVAL;
		
              return i;         

}

EXPORT_SYMBOL(wifi_power);



int bt_power(int i)
{
       	u32 gpdat,gpcon;
	
	if (i==1){
               printk("bt power on\n");

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

		/*gppud=__raw_readl(S3C64XX_GPMPUD);
		gppud &= ~(0x03<<10);
		__raw_writel(gppud,S3C64XX_GPMPUD);*/
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

                mdelay(200);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<14);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

              /* GPA 4 5 6 7 */
               gpcon = __raw_readl(S3C64XX_GPACON);	
//	       gppud = __raw_readl(S3C64XX_GPAPUD);	
	      // gpdat = __raw_readl(S3C64XX_GPADAT);

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

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

              /*WF_MAC_WAKE*/
                gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
                gpcon |=(0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);

                gpdat = __raw_readl(S3C64XX_GPODAT);
                gpdat |= (0x01<<10);
                __raw_writel(gpdat,S3C64XX_GPODAT);

		/*gppud=__raw_readl(S3C64XX_GPOPUD);
		gppud &= ~(0x03<<20);
		__raw_writel(gppud,S3C64XX_GPOPUD);*/

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

                mdelay(200);

	        gpdat = __raw_readl(S3C64XX_GPKDAT);	
		gpdat |= (0x01<<13);
   	        __raw_writel(gpdat,S3C64XX_GPKDAT);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

	}
	else if (i==0){
              printk("bt power off\n");
               /*BT_RESET  GPK13*/

               /*WIFI_RESET GPK14 */
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<24);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<28);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

                     /*WF_MAC_WAKE  GPO10*/
       		gpcon = __raw_readl(S3C64XX_GPOCON);
		gpcon &= ~(0x03<<20);
                gpcon |= (0x01<<20);
		__raw_writel(gpcon,S3C64XX_GPOCON);

               gpdat = __raw_readl(S3C64XX_GPODAT);
               gpdat &= ~(0x01<<10);
               __raw_writel(gpdat,S3C64XX_GPODAT);


 //                gpdat = __raw_readl(S3C64XX_GPODAT);
  //               gpdat &= ~(0x01<<10);
 //                __raw_writel(gpdat,S3C64XX_GPODAT);

            /* WF_SD_CMD WF_SD_CLK  GPC 4,5 */ 
       		gpcon = __raw_readl(S3C64XX_GPCCON);
		gpcon &= ~((0x0f<<16)|(0x0f<<20));
		__raw_writel(gpcon,S3C64XX_GPCCON);
		
		/*gppud=__raw_readl(S3C64XX_GPCPUD);
		gppud &= ~(0x0f<<8);
		__raw_writel(gppud,S3C64XX_GPCPUD);*/
		
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
                
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<20);
		__raw_writel(gpcon,S3C64XX_GPKCON1);
                  
		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<26);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/
 
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
	        //gppud = __raw_readl(S3C64XX_GPAPUD);	
	        gpdat = __raw_readl(S3C64XX_GPADAT);

		gpcon &= ~(0xffff<<16);		
		gpdat &= ~(0x0f<<4);
		//gppud &= ~(0xff<<8);

               	__raw_writel(gpcon,S3C64XX_GPACON);	
	        //__raw_writel(gppud,S3C64XX_GPAPUD);	
	        __raw_writel(gpdat,S3C64XX_GPADAT);

                 /*BT_WAKEUP  GPK9*/
       		gpcon = __raw_readl(S3C64XX_GPKCON1);
		gpcon &= ~(0x0f<<4);
		__raw_writel(gpcon,S3C64XX_GPKCON1);

		/*gppud=__raw_readl(S3C64XX_GPKPUD);
		gppud &= ~(0x03<<18);
		__raw_writel(gppud,S3C64XX_GPKPUD);*/

	}
	else
		return -EINVAL;
		
	
	return i;
}

EXPORT_SYMBOL(bt_power);


