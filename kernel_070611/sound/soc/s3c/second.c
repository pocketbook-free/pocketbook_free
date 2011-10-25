#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/timer.h> 
#include <asm/atomic.h> 

#include <linux/kernel.h>

#include <linux/device.h>
//#include <asm/hardware.h>
#include <linux/unistd.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-cfg.h>

extern int S3c_hp_status(void);
extern void SpeakerMute(u8 mute);
extern void hp_switch_state(u8 state);
extern void shedule_mute_headphone_work(int mute);
extern int ssm2603_mute_headphone(int mute);
    
struct timer_list s_timer;
int hp_state=-1;

void mySleep(unsigned long useconds)
{
   unsigned int i;
   for(i=0;i<useconds*100;i++)
	   __asm__ __volatile__ ("nop"); 
}

//int headset_state(void)
//{
//
//	unsigned int	hp_conectd_n = 0;
//	unsigned int 	iTmp = 0;
//	unsigned int 	iCntr = 0;
//
//	/* set GPN(2) as input to get hp status*/
//	s3c_gpio_cfgpin(S3C64XX_GPN(2),S3C64XX_GPN_INPUT(2));
//	/* pull-up enable*/
//	s3c_gpio_setpull(S3C64XX_GPN(2), S3C_GPIO_PULL_UP);
//
//	hp_conectd_n = gpio_get_value(S3C64XX_GPN(2));
//
//	return (!hp_conectd_n);	
//}
//EXPORT_SYMBOL_GPL(headset_state);

int headset_status(void)
{

	unsigned int	hp_conectd_n = 0;
	unsigned int 	iTmp = 0;
	unsigned int 	iCntr = 0;

	/* set GPN(4) as input to get hp status*/
	s3c_gpio_cfgpin(S3C64XX_GPN(2),S3C64XX_GPN_INPUT(2));
	/* pull-up enable*/
	s3c_gpio_setpull(S3C64XX_GPN(2), S3C_GPIO_PULL_UP);

	/* check 500us to make sure the hotplug is stable */
	while (1) 
	{
		mySleep(2);
		/* get gpn(4) data, 1-> hp not connected; 0-> hp connected*/
		iTmp = gpio_get_value(S3C64XX_GPN(2));
		if (iTmp!=hp_conectd_n)	
		{
			hp_conectd_n = iTmp;
			iCntr = 0;
		}
		else 
		{
			if (iCntr++>=3) 
				break;
		}
	}
	return (!hp_conectd_n);
}

#define SND_NO_HP      0x0

#define SND_RAMP_UP    0x0
#define SND_RAMP_DOWN  0x1

static void hp_detect_timer_handle(unsigned long arg)
{
  int state;
  mod_timer(&s_timer,jiffies + HZ/2);
  //printk("in Headphone event ...\n");

  state=headset_status();
  if(state!=hp_state)
  {
  	//printk("Headphone event hp_state:%d ...\n",hp_state);
    hp_state=state;
    if(hp_state)
    	 printk("Headphone plug in...\n");
    else
    	 printk("Headphone plug out...\n");

    //hp_switch_state(hp_state);
    SpeakerMute(hp_state);
    
    printk("hp_state: %s \n",hp_state==0?"NO_HP":"HP_IN");
    if(hp_state!=SND_NO_HP)  //headphone in
    {
    	 shedule_mute_headphone_work(SND_RAMP_UP);
    }
    else
    {
    	 shedule_mute_headphone_work(SND_RAMP_DOWN);
    //shedule_mute_headphone_work(!hp_state);
    }
  }
}

void hp_detect_timer_init(void)
{
	hp_state=headset_status();
	
	if(0!=hp_state)
    hp_state=1;
  else
  	hp_state=0;
   	    	
  init_timer(&s_timer);
  s_timer.function = &hp_detect_timer_handle;
  s_timer.expires = jiffies + HZ;
  add_timer(&s_timer);
}

int second_init(void)
{
	printk("Enter %s ...\n",__func__);
#if 1  //unuse timer detect headphone mode.
 	hp_detect_timer_init();
#endif
  return 0;
}

void second_exit(void)
{
  del_timer(&s_timer);
}

MODULE_AUTHOR("Hill");
MODULE_LICENSE("Dual BSD/GPL");

module_init(second_init);
module_exit(second_exit);
