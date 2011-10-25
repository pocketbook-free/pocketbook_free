/*
 * Wacom serial touchscreen driver
 *
 * Copyright (c) Dmitry Torokhov <dtor@mail.ru>
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
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/init.h>

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-k.h>
#include <plat/gpio-bank-m.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-f.h>
//&*&*&*HC1_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code
//#include <plat/regs-oldstyle-gpio.h>
//&*&*&*HC2_20100119, remove oldstyle-gpio code and merge SMDK 2.6.29 code
#include <linux/io.h>
#include "touch_power.h"

#define DRIVER_DESC	"wacom serial touchscreen driver"

MODULE_AUTHOR("Victor Liu <liuduogc@hotmail.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

#define WACOM_LENGTH 7 

#define HANDWRITE_ATTR
#define FIRMWARE_DOWNLOAD
/*
 * Per-touchscreen data.
 */
#ifdef HANDWRITE_ATTR
struct handwrite{
	int enable;
	int brush_size;
	int start_x_pos;
	int start_y_pos;
	int end_x_pos;
	int end_y_pos;
	unsigned long delay_update;
};
#endif
struct wacom {
	struct input_dev *dev;
	struct serio *serio;
	int idx;
	unsigned char data[WACOM_LENGTH];
	char phys[32];
	struct delayed_work	delay_work; 
};
enum touch_state{
	touch_EFFECTIVE=0x80,
	touch_PRESS_DOWN=0xb0,
	touch_PRESS_UP=0xe0,
	touch_ACTIVE=0xa0
};

struct input_dev *input_dev;
#ifdef HANDWRITE_ATTR
	int enable_handwrite = 0;
	int brush_size_handwrite = 0;
	int start_x_pos_handwrite = 0;
	int start_y_pos_handwrite = 0;
	int end_x_pos_handwrite = 0;
	int end_y_pos_handwrite = 0;
#endif

#define EPD_TOUCH
#define EPD_WRITE_MODE_CHECK //20100203, Jimmy_Su@CDPG, add to check write mode

#ifdef EPD_TOUCH
#define epd_wide 	828
#define epd_heigh 1200
//#define ts_wide 	6144
#define ts_wide 	6000
#define tw_heigh 	8192
#define EPD_MEM_DRAW(a,b,c,d)     epd_memroywrite_draw(a<<d, b<<d, c)

/*--Start--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
#ifdef EPD_WRITE_MODE_CHECK
extern void epd_check_bs_handwrite(bool mode);

static void wacom_to_bs_handwrite_mode(int enable)
{
	if (enable == 1){
		epd_check_bs_handwrite(true);
	}else{
		epd_check_bs_handwrite(false);
	}
}
#endif 
/*--End--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/

extern void epd_memroywrite_draw(unsigned int x0, unsigned int y0, int pen);
extern void epd_memroywrite_update(unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
void EPD_touch_save_point(int x, int y, struct wacom *wacom);

int x_left=828,x_right=0,y_top=1200,y_bottom=0;
static int old_x=-1;
static int old_y=-1;

static void EPD_Touch_bres(unsigned int current_x, unsigned int current_y, struct wacom *wacom, unsigned int res)
{
	int x0,y0,x1,y1;
	int dx,dy,dx_sym,dy_sym;
	int dx_x2,dy_x2,di;	
	int i,j,k;
	unsigned int delay_t = 4;
	
	x0 = old_x;
	y0 = old_y;
	x1 = current_x>>res;
	y1 = current_y>>res;
  
  if ((x0 == x1)&&(y0 == y1))
  	goto exit2;
  	
	if(old_x<0 && old_y<0)
	{
		//old_x = current_x>>res;
		//old_y = current_y>>res;
		EPD_MEM_DRAW(x1, y1, brush_size_handwrite, res);
		delay_t = 0;
	}
	else
	{				
		dx = x1 - x0;
		dy = y1 - y0;
		
			//x axis
			if(dx>0)
			{
					dx_sym = 1;
			}
			else
			{
					if(dx<0)
					{
						dx_sym = -1;
					}
					else //(dx==0, x0=x1)
					{
						if(dy>0)
							for(k=y0;k<=y1;k++)
								EPD_MEM_DRAW(x0, k, brush_size_handwrite, res);
						else if(dy<0)
							for(k=y1;k<=y0;k++)
								EPD_MEM_DRAW(x0, k, brush_size_handwrite, res);
						else
								EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
						
						goto exit;		
					}
			 }
			 
			//y axis
			if(dy>0)
			{
					dy_sym = 1;
			}
			else
			{
					if(dy<0)
					{
						dy_sym = -1;
					}
					else
					{
						//dy == 0 
						if(dx>0)
							for(k=x0;k<=x1;k++)
								EPD_MEM_DRAW(k, y0, brush_size_handwrite, res);
						else if(dx<0)
							for(k=x1;k<=x0;k++)
								EPD_MEM_DRAW(k, y0, brush_size_handwrite, res);
						else
								EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
								
  					goto exit;
					}
			 }
			 
			 dx = dx_sym * dx;
			 dy = dy_sym * dy;
			 
			 dx_x2 = dx*2;
			 dy_x2 = dy*2;
			 
			 //Bresenham
			 if(dx>=dy)
			 {
			 		di = dy_x2 - dx;
			 		while(x0!=x1)
			 		{
							EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
							
			 				x0 += dx_sym;
			 				if(di<0)
			 				{
			 					di += dy_x2;
			 				}
			 				else
			 				{
			 					di += dy_x2 - dx_x2;
			 					y0 += dy_sym;
			 				}
			 		}
					EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
			 	}
			 	else
			 	{
			 		di = dx_x2 - dy;
			 		while(y0!=y1)
			 		{
							EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
								
			 				y0 += dy_sym;
			 				if(di<0)
			 				{
			 					di += dx_x2;
			 				}
			 				else
			 				{
			 					di += dx_x2 - dy_x2;
			 					x0 += dx_sym;
			 				}
			 			}
			 			
					EPD_MEM_DRAW(x0, y0, brush_size_handwrite, res);
			 					
			 	}
			 
			 
			 
		}
		

exit:
	old_x = current_x>>res;
	old_y = current_y>>res;

	schedule_delayed_work(&wacom->delay_work, delay_t /*HZ / 20*/);


exit2:	
	return;
	
}

void EPD_touch_save_point(int ts_x, int ts_y, struct wacom *wacom)
{
		unsigned int x,y;
		
		x = (unsigned int)(((ts_wide - ts_x)*epd_wide)/ts_wide) ;
		y = (unsigned int)(((8192 - ts_y)*epd_heigh)/tw_heigh) ;
		
		if(x<0)x=0;	if(y<0)y=0;

		
		if(start_x_pos_handwrite <= x && x <= end_x_pos_handwrite && 
			 start_y_pos_handwrite <= y && y <= end_y_pos_handwrite )	
		{		
					EPD_Touch_bres(x,y,wacom,brush_size_handwrite>>3);
					
					if(x<x_left)x_left=x;
					if(x>x_right)x_right=x;
					if(y<y_top)y_top=y;
					if(y>y_bottom)y_bottom=y;					
		}
		else
		{	old_x=-1;	old_y=-1;}
			
								
}

static int collision_detect(void)
{	
	int size = 50;

	if((x_left-size)<=0) 			x_left=0;			else x_left-=size;
	if((x_right+size)>=828) 	x_right=828;	else x_right+=size;
	if((y_top-size)<=0) 			y_top=0; 			else y_top-=size;
	if((y_bottom+size)>=1200)	y_bottom=1200;else y_bottom+=size;	
		
	epd_memroywrite_update(/*x1*/x_left&0xFFFE, /*x2*/x_right, /*y1*/y_top, /*y2*/y_bottom);
	
	x_left=828,x_right=0,y_top=1200,y_bottom=0;
	
	return 0;
}

#endif
/*
 * Decode serial data (9 bytes per packet)
 * First byte
 * 1 C 0 0 R S S S
 */
 
static void do_softint(struct work_struct *work)
{
		collision_detect();
	
		
}
#ifdef FIRMWARE_DOWNLOAD
static ssize_t han_download_show(struct device_driver *ddp, char *buf)
{
	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);
	return 0;
}

static ssize_t han_download_store(struct device_driver *ddp, const char * buf, size_t count)
{
	int value = (*buf) - '0';
	printk("[Hanwang Trace] Enter %s value is %d\n", __FUNCTION__, value);
#ifdef CONFIG_EX3_HARDWARE_DVT
	if (value == 1)
	{
		//Touch_EN GPM0 -> OUTPUT low
   	 	gpio_set_value(S3C64XX_GPM(0),  0);
		s3c_gpio_cfgpin(S3C64XX_GPM(0),  S3C64XX_GPM_OUTPUT(0));
		
		//Touch_Pro GPN9 -> OUTPUT  	
		//Touch_Pro GPN9 -> HIGH
		gpio_set_value(S3C64XX_GPN(9), 1);
		s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C64XX_GPN_OUTPUT(9));
		mdelay(1);
        
		//PLT_RST GPK0 -> OUTPUT high
		gpio_set_value(S3C64XX_GPK(0),  1);
		s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));
		mdelay(1);
	
		//PLT_RST GPK0 -> OUTPUT low
		gpio_set_value(S3C64XX_GPK(0),  0);
		s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));
		mdelay(1);
	
		//PLT_RST GPK0 -> OUTPUT high
		gpio_set_value(S3C64XX_GPK(0),  1);
		s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));	
	}
#else
	unsigned long gpcon;
	unsigned long gpdata;
	if (value == 1)
	{
		//GPA6 -> OUTPUT
  		gpcon = __raw_readl(S3C64XX_GPACON);
		gpcon = gpcon & ~(0xf << (24));
  		__raw_writel(gpcon|(0x1<<24),S3C64XX_GPACON);
		//GPA6 -> HI
		gpdata = __raw_readl(S3C64XX_GPADAT);
		gpdata =(gpdata & ~(1<<6));
		__raw_writel(gpdata|(0x1<<6),S3C64XX_GPADAT);
		mdelay(150);
		
	}
	else
	{
		//GPA6 -> OUTPUT
  		gpcon = __raw_readl(S3C64XX_GPACON);
		gpcon = gpcon & ~(0xf << (24));
  		__raw_writel(gpcon|(0x1<<24),S3C64XX_GPACON);
		//GPA6 -> LOW
		gpdata = __raw_readl(S3C64XX_GPADAT);
		gpdata =(gpdata & ~(1<<6));
		__raw_writel(gpdata ,S3C64XX_GPADAT);
		mdelay(150);
		//sleep pin
		s3c_gpio_cfgpin(S3C64XX_GPK(1),  S3C_GPIO_OUTPUT);
    	//s3c_gpio_setpin(S3C_GPK1,  0);
    	gpio_set_value(S3C64XX_GPK(1), 0);
	}
	//reset  pin
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    	//s3c_gpio_setpin(S3C_GPK0,  1);
	gpio_set_value(S3C64XX_GPK(0), 1);
	mdelay(100);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    	//s3c_gpio_setpin(S3C_GPK0,  0);
    	gpio_set_value(S3C64XX_GPK(0), 0);
	mdelay(100);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    	//s3c_gpio_setpin(S3C_GPK0,  1);
    	gpio_set_value(S3C64XX_GPK(0), 1);
	mdelay(100);

#endif
	return count;
}
static DRIVER_ATTR(download, /*S_IRUGO | S_IWUSR*/0666, han_download_show, han_download_store);
#endif

#ifdef HANDWRITE_ATTR
static ssize_t handwrite_show(struct device_driver *wa_drv, char *buf)
{
	//struct wacom * hangwa = dev_get_drvdata(dev);
	//sscanf(buf, "%u", &val);
	//return sprintf(buf, "%d\n", battery->alarm * 1000);
	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);
	return 0;
}

static ssize_t handwrite_store(struct device_driver *wa_drv, const char * buf, size_t count)
{
	//struct wacom * hangwa = dev_get_drvdata(dev);
	int i=0;
	int j=0;
	char buff_temp[8];
	int para_count = 0;

	if(buf == NULL)
		return -1;
	
	printk("Enter %s, buf is %s, count is %d\n", __FUNCTION__,buf, count);

	memset(buff_temp, 0, sizeof(buff_temp));
	while (i<count)
	{
		if(buf[i]==',')
		{
			buff_temp[j] = NULL;
			para_count++;
			if(para_count == 1)
			{
				enable_handwrite = simple_strtoul(buff_temp, NULL, 10);
			}
			else if(para_count == 2)
			{
				brush_size_handwrite = simple_strtoul(buff_temp, NULL, 10);
			}
			else if(para_count == 3)
			{
				start_x_pos_handwrite = simple_strtoul(buff_temp, NULL, 10);
			}
			else if(para_count == 4)
			{
				start_y_pos_handwrite = simple_strtoul(buff_temp, NULL, 10);
			}
			else if(para_count == 5)
			{
				end_x_pos_handwrite = simple_strtoul(buff_temp, NULL, 10);
			}
			
			j=0;
			i++;
			memset(buff_temp, 0, sizeof(buff_temp));
			continue;
		}
		else
			buff_temp[j++] = buf[i++];
	}
	buff_temp[j] = "\0";
	para_count++;
	if(para_count == 6)
	{
		end_y_pos_handwrite = simple_strtoul(buff_temp, NULL, 10);
	}
		
	/*--Start--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
	#ifdef EPD_WRITE_MODE_CHECK
	wacom_to_bs_handwrite_mode(enable_handwrite);
	#endif
	/*--End--, Jimmy_Su@CDPG, 20100201, enable EPD power saving mode*/
		
	return count;
}
static DRIVER_ATTR(handwrite, /*S_IRUGO | S_IWUSR*/0666, handwrite_show, handwrite_store);
#endif

static irqreturn_t wacom_interrupt(struct serio *serio,
				     unsigned char data, unsigned int flags)
{
	struct wacom *wacom = serio_get_drvdata(serio);
#ifdef EPD_TOUCH
	unsigned int abs_x,abs_y,abs_pressure;
	unsigned int touch_button;
#endif		
#ifdef TOUCH_POWER_CONTROL
	int enter_cmds_mode;
	struct touch_power *touch_power;
	if(serio->drv->private)
		touch_power = serio->drv->private;
	if(touch_power)
	{
		enter_cmds_mode = touch_power->wait_idle_cmd_response;
		if(enter_cmds_mode)
		{
			if(!touch_power->transaction(serio, data, flags))
				return IRQ_HANDLED;
		}
	}
#endif
	if (wacom->idx == 0) 
	{
		/* resync skip until start of frame */
		if ((data & 0x80) != touch_EFFECTIVE)
		{	
			return IRQ_HANDLED;
		}
	} 
	else 
	{
		/* resync skip garbage */
		if ((data & 0x80)==touch_EFFECTIVE) 
		{
			wacom->idx = 0;
		}
	}
	
	wacom->data[wacom->idx++] = data;

	struct input_dev *dev = wacom->dev;
	static int pen_up_flag = 0;

	if (wacom->idx == WACOM_LENGTH) {	
			//&*&*&*AL1_20100226, pen down detect
			/*
			if((wacom->data[0] & 0xf0) == (unsigned char)touch_PRESS_DOWN)
					touch_button=1;
			if((wacom->data[0] & 0xf0) ==  (unsigned char)touch_PRESS_UP)
					touch_button=0;
			*/
			if(wacom->data[0] & 0x01)
				touch_button=1;
			else
				touch_button=0;
			//&*&*&*AL2_20100226, pen down detect
			abs_x=(wacom->data[1]<<9|wacom->data[2]<<2|wacom->data[6]>>5);
			abs_y=(wacom->data[3]<<9|wacom->data[4]<<2|((wacom->data[6]>>3)&0x3));
			abs_pressure=((wacom->data[5]|((wacom->data[6]& 0x7) << 7)));
  		
			if(enable_handwrite)
			{
					if(touch_button == 1 && abs_pressure > 0 && abs_x <= ts_wide/*6000*/ && abs_y <= 8192)
							EPD_touch_save_point(abs_x,abs_y,wacom);
					else //if(touch_button == 0 && abs_pressure == 0)
							{	old_x=-1;	old_y=-1;}		
			}	
			if(touch_button == 1 && abs_pressure > 0)
			{
					input_report_key(dev, BTN_TOUCH,touch_button);
					input_report_abs(dev, ABS_X,(ts_wide-abs_x));
			 		//printk("[hanwa trace - down]ABS_X=%d,",abs_x);
					input_report_abs(dev, ABS_Y,(8192-abs_y));
					//printk("[hanwa trace - down]ABS_Y=%d\n",abs_y);
					input_report_abs(dev, ABS_PRESSURE,abs_pressure);
					//printk("[hanwa trace - down]ABS_PRESSURE=%d\n",abs_pressure);
					input_sync(dev);
					wacom->idx = 0;
					
					pen_up_flag = 0;
			}
			else if(abs_pressure ==0 && touch_button==0)
			{
					input_report_key(dev, BTN_TOUCH,touch_button);
					input_report_abs(dev, ABS_X,(ts_wide-abs_x));
			 		//printk("[hanwa trace - up]ABS_X=%d,",abs_x);
					input_report_abs(dev, ABS_Y,(8192-abs_y));
					//printk("[hanwa trace - up]ABS_Y=%d\n",abs_y);
					input_report_abs(dev, ABS_PRESSURE,abs_pressure);
					//printk("hanwa trace - up]ABS_PRESSURE=%d\n",abs_pressure);
					input_sync(dev);
					wacom->idx = 0;
			}
			else if(abs_pressure ==0 && touch_button==1 && pen_up_flag == 0)
			{
					input_report_key(dev, BTN_TOUCH,0);
					input_report_abs(dev, ABS_X,(ts_wide-abs_x));
			 		//printk("[hanwa trace - up]ABS_X=%d,",abs_x);
					input_report_abs(dev, ABS_Y,(8192-abs_y));
					//printk("[hanwa trace - up]ABS_Y=%d\n",abs_y);
					input_report_abs(dev, ABS_PRESSURE,abs_pressure);
					//printk("hanwa trace - up]ABS_PRESSURE=%d\n",abs_pressure);
					input_sync(dev);
					wacom->idx = 0;
					
					pen_up_flag = 1;				
			}
			
//			schedule_delayed_work(&wacom->delay_work, HZ / 20);
			wacom->idx = 0;
	}

	return IRQ_HANDLED;

}

/*
 * wacom_disconnect() is the opposite of wacom_connect()
 */
static void wacom_disconnect(struct serio *serio)
{
	struct wacom *wacom = serio_get_drvdata(serio);

	//Don't unregister input device(event2) when suspend, It will make input2(event2) lose.
	//input_get_device(wacom->dev);
	//input_unregister_device(wacom->dev);
#ifdef TOUCH_POWER_CONTROL
	touch_power_deinit(serio->drv);
#endif
	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	//input_put_device(wacom->dev);
	kfree(wacom);
#ifdef CONFIG_EX3_HARDWARE_DVT	
	//Touch_RX GPA0 -> input
	s3c_gpio_cfgpin(S3C64XX_GPA(0), S3C64XX_GPA_INPUT(0));
	s3c_gpio_setpull(S3C64XX_GPA(0), S3C_GPIO_PULL_NONE);
	
	//Touch_TX GPA1 -> input
	s3c_gpio_cfgpin(S3C64XX_GPA(1), S3C64XX_GPA_INPUT(1));
	s3c_gpio_setpull(S3C64XX_GPA(1), S3C_GPIO_PULL_NONE);
		
	//PEN_DET GPN8 -> input
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C64XX_GPN_INPUT(8));
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_NONE);
	
	//PEN_SLP GPF15 -> input
	s3c_gpio_cfgpin(S3C64XX_GPF(15), S3C64XX_GPF_INPUT(15));
	s3c_gpio_setpull(S3C64XX_GPF(15), S3C_GPIO_PULL_NONE);
	
	//PLT_RST GPK0 -> input
	s3c_gpio_cfgpin(S3C64XX_GPK(0), S3C64XX_GPK_INPUT(0));
	s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_NONE);
	
	//Touch_EN GPM0 -> OUTPUT high
    gpio_set_value(S3C64XX_GPM(0),  1);
	s3c_gpio_cfgpin(S3C64XX_GPM(0),  S3C64XX_GPM_OUTPUT(0));
#endif	
	
}

/*
 * wacom_connect() is the routine that is called when someone adds a
 * new serio device that supports the Wacom protocol and registers it
 * as input device.
 */
static int wacom_connect(struct serio *serio, struct serio_driver *drv)
{
	struct wacom *wacom;

	int err;
	printk("[hanwa trace]Enter %s.\n", __FUNCTION__);
#ifdef CONFIG_EX3_HARDWARE_DVT		
	//Touch_EN GPM0 -> OUTPUT low
    gpio_set_value(S3C64XX_GPM(0),  0);
	s3c_gpio_cfgpin(S3C64XX_GPM(0),  S3C64XX_GPM_OUTPUT(0));
	
#ifdef FIRMWARE_DOWNLOAD
	//Touch_Pro GPN9 -> OUTPUT  	
	//Touch_Pro GPN9 -> LOW
	gpio_set_value(S3C64XX_GPN(9), 0);
	s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C64XX_GPN_OUTPUT(9));	
	mdelay(150);
#endif
	//struct input_dev *input_dev;
	
	//Touch_RX GPA0 -> RX
	s3c_gpio_cfgpin(S3C64XX_GPA(0), S3C64XX_GPA0_UART_RXD0);	
	
	//Touch_TX GPA1 -> TX
	s3c_gpio_cfgpin(S3C64XX_GPA(1), S3C64XX_GPA1_UART_TXD0);


	//PEN_DET GPN8 -> input
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C64XX_GPN_INPUT(8));
	s3c_gpio_setpull(S3C64XX_GPN(8), S3C_GPIO_PULL_NONE);	
	
	//PEN_SLP GPF15 -> OUTPUT low
	gpio_set_value(S3C64XX_GPF(15),  0);
	s3c_gpio_cfgpin(S3C64XX_GPF(15),  S3C64XX_GPF_OUTPUT(15)); 
        
	//PLT_RST GPK0 -> OUTPUT high
	gpio_set_value(S3C64XX_GPK(0),  1);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));
	mdelay(100);
	
	//PLT_RST GPK0 -> OUTPUT low
	gpio_set_value(S3C64XX_GPK(0),  0);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));
	mdelay(100);
	
	//PLT_RST GPK0 -> OUTPUT high
	gpio_set_value(S3C64XX_GPK(0),  1);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C64XX_GPK_OUTPUT(0));	
#else	

#ifdef FIRMWARE_DOWNLOAD
	unsigned long gpcon;
	unsigned long gpdata;
	//GPA6 -> OUTPUT
  	gpcon = __raw_readl(S3C64XX_GPACON);
	gpcon = gpcon & ~(0xf << (24));
  	__raw_writel(gpcon|(0x1<<24),S3C64XX_GPACON);
	//GPA6 -> LOW
	gpdata = __raw_readl(S3C64XX_GPADAT);
	gpdata =(gpdata & ~(1<<6));
	__raw_writel(gpdata,S3C64XX_GPADAT);
	mdelay(150);
#endif
	//struct input_dev *input_dev;
	s3c_gpio_cfgpin(S3C64XX_GPK(1),  S3C_GPIO_OUTPUT);
    //s3c_gpio_setpin(S3C_GPK1,  0);
    gpio_set_value(S3C64XX_GPK(1), 0);
//    s3c_gpio_setpin(S3C64XX_GPK(1),  0);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    //s3c_gpio_setpin(S3C_GPK0,  1);
	gpio_set_value(S3C64XX_GPK(0), 1);    
//    s3c_gpio_setpin(S3C64XX_GPK(0),  1);
	mdelay(100);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    //s3c_gpio_setpin(S3C_GPK0,  0);
	gpio_set_value(S3C64XX_GPK(0), 0);    
//        s3c_gpio_setpin(S3C64XX_GPK(0),  0);
	mdelay(100);
	s3c_gpio_cfgpin(S3C64XX_GPK(0),  S3C_GPIO_OUTPUT);
    //s3c_gpio_setpin(S3C_GPK0,  1);
	gpio_set_value(S3C64XX_GPK(0), 1);    
//   s3c_gpio_setpin(S3C64XX_GPK(0),  1);

#endif
    wacom = kzalloc(sizeof(struct wacom), GFP_KERNEL);
	wacom->serio = serio;
	wacom->dev = input_dev;
	INIT_DELAYED_WORK(&wacom->delay_work, do_softint);
	snprintf(wacom->phys, sizeof(wacom->phys),
		 "%s/input0", serio->phys);

	input_dev->phys = wacom->phys;

	serio_set_drvdata(serio, wacom);
	err = serio_open(serio, drv);
	if (err)
		goto fail2;

    serio_write(wacom->serio,'0');
	mdelay(100);
#ifdef TOUCH_POWER_CONTROL
		touch_power_init(drv, serio);
#endif

	return 0;
 fail2:
 	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	kfree(wacom);
	return err;
}

/*
 * The serio driver structure.
 */
static struct serio_device_id wacom_serio_ids[] = {
	{
		.type	= SERIO_RS232,
		.proto	= SERIO_ANY,//0x3a,//SERIO_WACOM,
		.id	= SERIO_ANY,//0x0,//SERIO_ANY,
		.extra	= SERIO_ANY,//0x0,//SERIO_ANY,
	},
	{ 0 }
};

MODULE_DEVICE_TABLE(serio, wacom_serio_ids);

static struct serio_driver wacom_drv = {
	.driver		= {
		.name	= "wacom_ts",
	},
	.description	= DRIVER_DESC,
	.id_table	= wacom_serio_ids,
	.interrupt	= wacom_interrupt,
	.connect	= wacom_connect,
	.disconnect	= wacom_disconnect,
};

static int __init wacom_init(void)
{
	//printk(KERN_ALERT "****this is wacom init function****\n");
	int err;
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		goto fail1;
	}

	input_dev->name = "Wacom Serial Touchscreen";
	input_dev->id.bustype = BUS_RS232;
	input_dev->id.vendor = SERIO_WACOM;
	input_dev->id.product = 0;
	input_dev->id.version = 0x0100;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, 6144, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 8192, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0,1024, 0, 0);
	
	err = input_register_device(input_dev);
	if (err)
		goto fail3;
	err = serio_register_driver(&wacom_drv);

#ifdef TOUCH_POWER_CONTROL
	touch_power_register(&wacom_drv);
#endif
#ifdef HANDWRITE_ATTR
	err = driver_create_file(&wacom_drv.driver, &driver_attr_handwrite);
#endif
#ifdef FIRMWARE_DOWNLOAD
	return driver_create_file(&wacom_drv.driver, &driver_attr_download);
#else
	return err;
#endif
fail3:
	input_free_device(input_dev);
fail1:
	printk("[hanwa trace]Enter %s, input_allocate_device error!\n", __FUNCTION__);
	return err;
}

static void __exit wacom_exit(void)
{
#ifdef TOUCH_POWER_CONTROL
	touch_power_unregister(&wacom_drv);
#endif
#ifdef HANDWRITE_ATTR
	driver_remove_file(&wacom_drv.driver, &driver_attr_handwrite);
#endif
#ifdef FIRMWARE_DOWNLOAD
	driver_remove_file(&wacom_drv.driver, &driver_attr_download);
#endif
	serio_unregister_driver(&wacom_drv);
}

module_init(wacom_init);
//late_initcall(wacom_init);
module_exit(wacom_exit);
