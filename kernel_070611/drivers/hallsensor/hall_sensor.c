/**********************************************
 * FileName:	hall_sensor.c
 *
 * Description:
 * 		This file deal with irq registion, etc.
 * Platform:
 * 		EX3 DVT development board
 * 		Linux-2.6.29
 * Author:
 * 		aimar.liu <aimar.ts.liu@foxconn.com>
 *
 * $Id: hall_sensor.c,v 0.5 2010/01/08 $
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-l.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/smp_lock.h>

struct s3c_sensor_gpio
{
   int eint;
   int gpio;
   int gpio_af;
   int keycode;
};

struct combine_sensor {
	struct input_dev *dev;
	struct s3c_sensor_gpio	*gpio_key;
	int gpio_key_num;
	int value;
	struct work_struct  work;
	int irq;
	spinlock_t			lock;	/* protects all state */	
};

struct s3c_sensor_gpio s3c_sensor_gpio_key[] = {
#ifdef CONFIG_EX3_HARDWARE_DVT
	{IRQ_EINT(4),  S3C64XX_GPN(4), 2, KEY_END}, //HALL Sensor
	{IRQ_EINT(11),  S3C64XX_GPN(11), 2, KEY_F2} //Pen Out

#else
	{IRQ_EINT(21),  S3C64XX_GPL(13), 3, KEY_END}
#endif
};

/*************************************
 *
 * 		register IRQ handler
 *
 *************************************/

//static irqreturn_t combine_sensor_irq(int irq, void *handle)
static void combine_sensor_work_func(struct work_struct *work)
{
	
	struct combine_sensor *combine_sensor = container_of(work, struct combine_sensor, work);
	//struct combine_sensor  *combine_sensor = (struct combine_sensor *) handle;
	struct s3c_sensor_gpio *gpio_key = combine_sensor->gpio_key;
	struct input_dev       *dev = combine_sensor->dev;
	int i, state=0;
	unsigned int 	iTmp = 0, iCntr = 0;
	
	spin_lock(&combine_sensor->lock);
	
	for(i=0; i<combine_sensor->gpio_key_num; i++)
	{
			if(gpio_key[i].eint == combine_sensor->irq)
			{
					gpio_key = &gpio_key[i];
					break;
			}
	}
	
	spin_unlock(&combine_sensor->lock);
	
	if(gpio_key != NULL)
	{
		
		while (1) 
		{
			msleep(10);

			/* get gpn(4) data, 1-> hp not connected; 0-> hp connected*/
			iTmp = gpio_get_value(gpio_key->gpio);

			if (iTmp!=state)	{
				state = iTmp;
				iCntr = 0;
			}
			else {
				if (iCntr++>25) break;
			}
		}
			
			
		//state = gpio_get_value(gpio_key->gpio);
		printk("combine_sensor IRQ %d, value is %d.\r\n", combine_sensor->irq, state);
		
		if(state == 0)
			input_report_key(dev, gpio_key->keycode, 1);
			input_report_key(dev, gpio_key->keycode, 0);
			
	}
	
	//return IRQ_HANDLED;
}

static irqreturn_t combine_sensor_irq(int irq, void *handle)
{
	struct combine_sensor *combine_sensor = handle;
	spin_lock(&combine_sensor->lock);	
	combine_sensor->irq = irq;
	spin_unlock(&combine_sensor->lock);
	schedule_work(&combine_sensor->work);
	return IRQ_HANDLED;	
	
}


static int __init hall_sensor_init(void)
{
	int err, i;
	
	struct combine_sensor 	*combine_sensor;
	struct s3c_sensor_gpio	*gpio_key;
	struct input_dev *input_dev;
	
	combine_sensor = kzalloc(sizeof(struct combine_sensor), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!input_dev || !combine_sensor) {
			err = -ENOMEM;
			goto err_alloc;
	}
	
	spin_lock_init(&combine_sensor->lock);
	
	INIT_WORK(&combine_sensor->work, combine_sensor_work_func);
	
	combine_sensor->dev = input_dev;
	combine_sensor->gpio_key = &s3c_sensor_gpio_key[0];
	combine_sensor->gpio_key_num = sizeof(s3c_sensor_gpio_key)/sizeof(struct s3c_sensor_gpio);
	gpio_key = combine_sensor->gpio_key;
	
	set_bit(EV_KEY, input_dev->evbit);
		for(i=0;i<combine_sensor->gpio_key_num;i++)
	{
			input_set_capability(input_dev, EV_KEY, (gpio_key+i)->keycode);
	}
	
	input_dev->name = "Hall Sensor and Pen Out";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0002;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0002;	
	
	//GPN4 and GPN11 -> Interrupt
	for(i=0;i<combine_sensor->gpio_key_num;i++)
	{
 			s3c_gpio_cfgpin((gpio_key+i)->gpio, S3C_GPIO_SFN((gpio_key+i)->gpio_af));
 			s3c_gpio_setpull((gpio_key+i)->gpio, S3C_GPIO_PULL_DOWN);	
  
  		set_irq_type((gpio_key+i)->eint, IRQ_TYPE_EDGE_BOTH);
  		err = request_irq((gpio_key+i)->eint, combine_sensor_irq, IRQF_SHARED, "combine_sensor irg", (void*)combine_sensor);
			if (err) 
			{
					printk("ENTER %s, register %d interrupt handler failed.\r\n", __FUNCTION__, (gpio_key+i)->eint);
					goto err_irq;
			}
	}
	
	err = input_register_device(input_dev);
	if (err)
		goto out;
		
	return err;

out:
	for(i=0;i<combine_sensor->gpio_key_num;i++)
		free_irq((gpio_key+i)->eint, combine_sensor);
err_irq:
		input_free_device(input_dev);
		kfree(combine_sensor);
err_alloc:
		return err;
}

static void __exit hall_sensor_exit(void)
{
	free_irq(s3c_sensor_gpio_key[0].eint, combine_sensor_irq);
	free_irq(s3c_sensor_gpio_key[1].eint, combine_sensor_irq);
}

module_init(hall_sensor_init);
module_exit(hall_sensor_exit);
