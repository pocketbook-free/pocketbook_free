/*
 * hanvon serial digital tier touchscreen power control driver.
 *
 * Copyright (c) foxconn <aimar.ts.liu@foxconn.com>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-m.h>
#include <plat/gpio-bank-n.h>
#include <linux/io.h>
#include <linux/irq.h>
#include "touch_power.h"
#ifdef CONFIG_STOP_MODE_SUPPORT
#include <linux/earlysuspend.h>
struct serio_driver *touch_power_pdev=NULL;
#endif

#if defined(TOUCH_POWER_CONTROL) && defined(CONFIG_HW_EP3_DVT)
//&*&*&*AL1_20100222, enhanced command response ratio
static void send_region_cmds(struct touch_power* touch_power)
{
	struct serio* serio = touch_power->serio;
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[0]);
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[1]);
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[2]);
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[3]);
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[4]);
	serio_write(serio,(unsigned char )touch_power->hanvon_idle.aa_region[5]);
	printk("[Hanwang Trace] Enter %s. aa data[0] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[0]);
	printk("[Hanwang Trace] Enter %s. aa data[1] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[1]);
	printk("[Hanwang Trace] Enter %s. aa data[2] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[2]);
	printk("[Hanwang Trace] Enter %s. aa data[3] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[3]);
	printk("[Hanwang Trace] Enter %s. aa data[4] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[4]);
	printk("[Hanwang Trace] Enter %s. aa data[5] is 0x%08x\n", __FUNCTION__, touch_power->hanvon_idle.aa_region[5]);	
}
//&*&*&*AL2_20100222, enhanced command response ratio
static ssize_t han_idle_show(struct device_driver *han, char *buf)
{
	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);
	return 0;
}
static ssize_t han_idle_store(struct device_driver *han, const char * buf, size_t count)
{
	struct serio_driver	*serio_drvier =
		container_of(han, struct serio_driver, driver);
	struct touch_power	* touch_power = serio_drvier->private;
	//&*&*&*AL1_20100222, enhanced command response ratio
	//struct serio* serio = touch_power->serio;
	//&*&*&*AL1_20100222, enhanced command response ratio
	printk("[Hanwang Trace] Enter %s, %s.\n", __FUNCTION__, buf);
	unsigned long flag;
	
	int i=0;
	int j=0;
	int x = 0;
	char buf_tmp[3];
	
	if(touch_power != NULL)
	{
		while (i<count && x<=4)
		{
			if(buf[i]!=','){
				buf_tmp[j++] = buf[i++];
			}
			else{
				buf_tmp[j] = NULL;
				j = 0;
				i++;
				strict_strtoul(buf_tmp, 10, &touch_power->hanvon_idle.aa_region[x]);
				if(x == 0)
					touch_power->hanvon_idle.aa_region[x] += 64;
				x++;
			}
		}
		if(x == 4)
		{
			buf_tmp[j] = NULL;
			strict_strtoul(buf_tmp, 10, &touch_power->hanvon_idle.aa_region[x]);
			touch_power->hanvon_idle.aa_region[x+1] = 0xFE;
			spin_lock_irqsave(&touch_power->spinlock,flag);
			if(touch_power->start_idle== 1) 
			{
				cancel_delayed_work(&touch_power->delay_work_idle);
		    	touch_power->start_idle = 0;
			}
			touch_power->wait_idle_cmd_response = 1;
			spin_unlock_irqrestore(&touch_power->spinlock, flag);
			//&*&*&*AL1_20100222, enhanced command response ratio
			send_region_cmds(touch_power);
			touch_power->send_cmd.cmd = TOUCH_REGION_CMD;
			touch_power->send_cmd.send_count = 5;
			mod_timer(&touch_power->send_cmd_timeout, jiffies+10);	
			//&*&*&*AL2_20100222, enhanced command response ratio
		}
	}
	
	return count;
}
static DRIVER_ATTR(idle, 0666, han_idle_show, han_idle_store);

static ssize_t han_wake_up_show(struct device_driver *han, char *buf)
{
	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);
	return 0;
}
static ssize_t han_wake_up_store(struct device_driver *han, const char * buf, size_t count)
{
	struct serio_driver	*serio_drvier =
		container_of(han, struct serio_driver, driver);
	struct touch_power	* touch_power = serio_drvier->private;
	struct serio* serio = touch_power->serio;
	unsigned long val, flag;

	strict_strtoul(buf, 10, &val);
	printk("[Hanwang Trace] Enter %s. wake_up is 0x%08x\n", __FUNCTION__, val);

	spin_lock_irqsave(&touch_power->spinlock,flag);
	if(touch_power->start_idle == 1) 
	{
		cancel_delayed_work(&touch_power->delay_work_idle);
		touch_power->start_idle = 0;
	}
	touch_power->wait_idle_cmd_response = 1;
	//&*&*&*AL1_20100222, added and fixed 0x4F Qery mode
	if(val == TOUCH_QUERY_MODE)
		touch_power->wait_idle_cmd_response = 3;
	//&*&*&*AL2_20100222, added and fixed 0x4F Qery mode
	spin_unlock_irqrestore(&touch_power->spinlock, flag);
	serio_write(serio, (unsigned char )val);

	//&*&*&*AL1_20100222, enhanced command response ratio
	touch_power->send_cmd.cmd = (unsigned char )val;
	touch_power->send_cmd.send_count = 10;
	mod_timer(&touch_power->send_cmd_timeout, jiffies+5);
	if(val == TOUCH_NORMAL_MODE)
	{
		wait_event(touch_power->send_cmd.wait_rsp, (!touch_power->wait_idle_cmd_response) || (!touch_power->send_cmd.send_count));
		del_timer_sync(&touch_power->send_cmd_timeout);
		if((touch_power->send_cmd.send_count == 0) && touch_power->wait_idle_cmd_response)
			return -EIO;
	}
	//&*&*&*AL2_20100222, enhanced command response ratio
	
	return count;
}
static DRIVER_ATTR(wake_up, 0666, han_wake_up_show, han_wake_up_store);

//&*&*&*AL1_20100222, enhanced command response ratio
static void send_cmds_timeout(unsigned long arg)
{
	struct touch_power* touch_power = (struct touch_power*)arg;
//	printk("[Hanwang Trace] enter %s\n", __FUNCTION__);
	
	if(touch_power){
		if(touch_power->transaction_mode == D0 && touch_power->wait_idle_cmd_response && touch_power->send_cmd.send_count)
		{
			if(touch_power->send_cmd.cmd == TOUCH_REGION_CMD)
			{
				send_region_cmds(touch_power);
				mod_timer(&touch_power->send_cmd_timeout, jiffies+10);
				//touch_power->send_cmd.send_count--;
			}
			else
			{
				serio_write(touch_power->serio,touch_power->send_cmd.cmd);
				mod_timer(&touch_power->send_cmd_timeout, jiffies+5);
				//touch_power->send_cmd.send_count--;
			}
		}
		else if(touch_power->transaction_mode == D1 && touch_power->wait_idle_cmd_response && touch_power->send_cmd.send_count)
		{
			serio_write(touch_power->serio,touch_power->send_cmd.cmd);
			mod_timer(&touch_power->send_cmd_timeout, jiffies+1);
			//touch_power->send_cmd.send_count--;
		}
//		printk("Send cmd is 0x%08x, remain repeat count is %d\n", touch_power->send_cmd.cmd, touch_power->send_cmd.send_count);
		if(touch_power->send_cmd.send_count == 0)
			wake_up(&touch_power->send_cmd.wait_rsp);
		else
			touch_power->send_cmd.send_count--;	
	}
}
//&*&*&*AL2_20100222, enhanced command response ratio

int touch_power_transaction(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct touch_power *touch_power;
	printk("[Hanwang Trace] enter %s\n", __FUNCTION__);
	if(serio->drv->private)
		touch_power = serio->drv->private;
	
	if(data== TOUCH_IDLE_MODE)
	{
		printk("Enter %s, reciver 0x4D, Enter Sleep mode\n", __FUNCTION__);
		//&*&*&*AL1_20100222, added and fixed 0x4F Qery mode
		//touch_power->wait_idle_cmd_response = 0;
		touch_power->wait_idle_cmd_response--;
		//&*&*&*AL2_20100222, added and fixed 0x4F Qery mode
		touch_power->transaction_mode = D1;
		return 0;
	}
	else if(data == TOUCH_NORMAL_MODE)
	{
		printk("Enter %s, reciver 0x4E, leave Sleep mode\n", __FUNCTION__);
		//&*&*&*AL1_20100222, added and fixed 0x4F Qery mode
		//touch_power->wait_idle_cmd_response = 0;
		touch_power->wait_idle_cmd_response--;
		//&*&*&*AL2_20100222, added and fixed 0x4F Qery mode
		touch_power->transaction_mode = D0;
		//&*&*&*AL1_20100222, enhanced command response ratio
		wake_up(&touch_power->send_cmd.wait_rsp);
		//&*&*&*AL2_20100222, enhanced command response ratio
		return 0;
	}
	else if(data >= 0x41 && data <= 0x4a)
	{
		touch_power->wait_idle_cmd_response = 6;
		return 0;
	}

	if(touch_power->wait_idle_cmd_response--)
	{
		printk("Enter %s, reciver data is 0x%08x\n", __FUNCTION__, data);
		if(touch_power->wait_idle_cmd_response == 1 && data == 0xFE){
			printk("Enter %s, set aa region success.\n", __FUNCTION__);
			touch_power->wait_idle_cmd_response = 0;
			//&*&*&*AL1_20100222, enhanced command response ratio
			wake_up(&touch_power->send_cmd.wait_rsp);
			//&*&*&*AL2_20100222, enhanced command response ratio
		}
		else{
			;//whether check other respnse cmds?
		}
		return 0;
	}
	return 1;
}

static void  do_softint_idle (struct work_struct *work)
{
	struct touch_power *touch_power =
		container_of(work, struct touch_power, delay_work_idle.work);
	struct serio *serio = touch_power->serio;
	unsigned long flag;

//	printk("[Hanwang Trace] Enter %s. wacom address is 0x%08x\n", __FUNCTION__, touch_power);
	spin_lock_irqsave(&touch_power->spinlock,flag);
	touch_power->start_idle = 0;
	touch_power->wait_idle_cmd_response = 1;
	spin_unlock_irqrestore(&touch_power->spinlock, flag);
	serio_write(serio,TOUCH_IDLE_MODE);
	//&*&*&*AL1_20100222, added and fixed 0x4F Qery mode
	touch_power->send_cmd.cmd = TOUCH_IDLE_MODE;
	touch_power->send_cmd.send_count = 5;
	mod_timer(&touch_power->send_cmd_timeout, jiffies+5);
	//&*&*&*AL2_20100222, added and fixed 0x4F Qery mode
}

static irqreturn_t hanvon_power_irq(int irq, void *handle)
{
	int value;
	struct touch_power* touch_power = (struct touch_power*) handle;
	unsigned long flag;
	int startidle;
	spin_lock_irqsave(&touch_power->spinlock,flag);
	startidle = touch_power->start_idle;
	value = gpio_get_value(S3C64XX_GPM(4));
	
	spin_unlock_irqrestore(&touch_power->spinlock, flag);
	if(value == 0)
	{
		if(startidle == 1) 
		{
			cancel_delayed_work(&touch_power->delay_work_idle);
			startidle = 0;
		}
//		printk("Enter %s , value is %d (2)\n", __FUNCTION__, value);
	}
	else
	{
		if(startidle == 0)
		{
			schedule_delayed_work(&touch_power->delay_work_idle, HZ+HZ/2);
			startidle = 1;
		}
//		printk("Enter %s , value is %d (4)\n", __FUNCTION__, value);
	}
	spin_lock_irqsave(&touch_power->spinlock,flag);
	touch_power->start_idle = startidle;
	spin_unlock_irqrestore(&touch_power->spinlock, flag);

	return IRQ_HANDLED;
}

#ifdef CONFIG_STOP_MODE_SUPPORT

static void touch_power_early_suspend(struct early_suspend *h)
{
	if (touch_power_pdev)
	{
        struct touch_power* touch_power = (struct touch_power*)touch_power_pdev->private;
    	struct serio *serio = touch_power->serio;
    	unsigned long flag;

    	printk("[Hanwang Trace] Enter %s\n", __FUNCTION__);
    	spin_lock_irqsave(&touch_power->spinlock,flag);
    	touch_power->start_idle = 0;
    	touch_power->wait_idle_cmd_response = 1;
    	spin_unlock_irqrestore(&touch_power->spinlock, flag);
    	serio_write(serio,TOUCH_IDLE_MODE);
    	//&*&*&*AL1_20100222, added and fixed 0x4F Qery mode
    	touch_power->send_cmd.cmd = TOUCH_IDLE_MODE;
    	touch_power->send_cmd.send_count = 5;
    	mod_timer(&touch_power->send_cmd_timeout, jiffies+5);
	}
}

static void touch_power_late_resume(struct early_suspend *h)
{
	if (touch_power_pdev)
	{
		return;
	}
}

static struct early_suspend touch_power_early_suspend_desc = {
        .level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
        .suspend = touch_power_early_suspend,
        .resume = touch_power_late_resume,
};
#endif
int touch_power_init(struct serio_driver* serio_drv, struct serio* serio)
{
	int err;
	printk("[Hanwang Trace] enter %s\n", __FUNCTION__);
	struct touch_power* touch_power = (struct touch_power*)serio_drv->private;

	if(serio && touch_power->serio == NULL)
		touch_power->serio = serio;
	if(touch_power->transaction == NULL)
		touch_power->transaction = touch_power_transaction;
	
	//PEN_DET GPM4 ->pen detect
	s3c_gpio_cfgpin(S3C64XX_GPM(4), S3C_GPIO_SFN(3));		//GPM4
	s3c_gpio_setpull(S3C64XX_GPM(4), S3C_GPIO_PULL_UP);
//	set_irq_type(IRQ_EINT(27), IRQ_TYPE_EDGE_FALLING);	

	err = request_irq(IRQ_EINT(27), hanvon_power_irq, IRQ_TYPE_EDGE_BOTH, "hanvon power irg", touch_power);
	if (err) 
	{
		printk("register %d interrupt handler failed.\r\n", IRQ_EINT(27));
		return err;
	}
	
	return 0;
}
EXPORT_SYMBOL(touch_power_init);

int touch_power_deinit(struct serio_driver* serio_drv)
{
	unsigned long flag;
	struct touch_power* touch_power = (struct touch_power*)serio_drv->private;
	if(touch_power->serio)
		touch_power->serio = NULL;
	spin_lock_irqsave(&touch_power->spinlock,flag);
	if(touch_power->start_idle == 1) 
	{
		cancel_delayed_work(&touch_power->delay_work_idle);
		touch_power->start_idle = 0;
	}
	spin_unlock_irqrestore(&touch_power->spinlock, flag);

	free_irq(IRQ_EINT(27), touch_power);

	//&*&*&*AL1_20100223, enhanced command response ratio
	touch_power->suspend = 1;
	//&*&*&*AL2_20100223, enhanced command response ratio
	return 0;
}
EXPORT_SYMBOL(touch_power_deinit);

int touch_power_register(struct serio_driver* serio_drv)
{
	int err;
	struct touch_power* touch_power = kzalloc(sizeof(struct touch_power), GFP_KERNEL);
	printk("[Hanwang Trace] enter %s\n", __FUNCTION__);
	if(touch_power == NULL)
	{
		err = -ENOMEM;
		return err;
	}
	serio_drv->private = touch_power;
	INIT_DELAYED_WORK(&touch_power->delay_work_idle, do_softint_idle);
	spin_lock_init(&touch_power->spinlock);

	//&*&*&*AL1_20100222, enhanced command response ratio
	init_timer(&touch_power->send_cmd_timeout);
	touch_power->send_cmd_timeout.function = send_cmds_timeout;
	touch_power->send_cmd_timeout.data = (unsigned long)touch_power;
	init_waitqueue_head(&touch_power->send_cmd.wait_rsp);
	//&*&*&*AL2_20100222, enhanced command response ratio
	
	err = driver_create_file(&serio_drv->driver, &driver_attr_idle);
	err = driver_create_file(&serio_drv->driver, &driver_attr_wake_up);
	printk("Enter %s, err is %d\n", __FUNCTION__, err);

	//&*&*&*AL1_20100222, enhanced command response ratio
	touch_power->hanvon_idle.aa_region[0] = 0x41;
	touch_power->hanvon_idle.aa_region[1] = 0x00;
	touch_power->hanvon_idle.aa_region[2] = 0x00;
	touch_power->hanvon_idle.aa_region[3] = 0x22;
	touch_power->hanvon_idle.aa_region[4] = 0x2E;
	touch_power->hanvon_idle.aa_region[5] = 0xFE;
	//&*&*&*AL2_20100222, enhanced command response ratio
	#ifdef CONFIG_STOP_MODE_SUPPORT
		touch_power_pdev = serio_drv;
		register_early_suspend(&touch_power_early_suspend_desc);
    #endif
	return 0;
}
EXPORT_SYMBOL(touch_power_register);

void touch_power_unregister(struct serio_driver* serio_drv)
{
	struct touch_power* touch_power = (struct touch_power*)serio_drv->private;
	if(touch_power)
	{
		//&*&*&*AL1_20100222, enhanced command response ratio
		del_timer_sync(&touch_power->send_cmd_timeout);
		//&*&*&*AL2_20100222, enhanced command response ratio
		kfree(touch_power);
	}
	driver_remove_file(&serio_drv->driver, &driver_attr_idle);
	driver_remove_file(&serio_drv->driver, &driver_attr_wake_up);
    #ifdef CONFIG_STOP_MODE_SUPPORT
        touch_power_pdev = NULL;
        unregister_early_suspend(&touch_power_early_suspend_desc);
    #endif
}
EXPORT_SYMBOL(touch_power_unregister);
#endif