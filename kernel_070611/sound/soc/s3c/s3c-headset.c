#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/switch.h>

#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/irqs.h>

#include <plat/regs-gpio.h>
//#include <plat/gpio-bank-b.h>
//#include <plat/gpio-bank-f.h>
#include <plat/gpio-bank-n.h>
//#include <plat/gpio-bank-l.h>
#include <plat/gpio-cfg.h>

#include <plat/regs-keypad.h>

#include <linux/kobject.h>

extern void msleep(unsigned int msecs);
extern void SpeakerMute(u8 mute);
int S3c_hp_status(void);

extern int ssm2603_mute_headphone(int mute);
//********************************************************************************

#define PAGESIZE 0x1024
#define MEMSTARTADDR 0x7F008000

#define rGPNCON  ( *(volatile unsigned *)(gGPIOVAddr.base+0x830) )
#define rGPNDAT  ( *(volatile unsigned *)(gGPIOVAddr.base+0x834) )
#define rGPNPUD  ( *(volatile unsigned *)(gGPIOVAddr.base+0x838) )

#define rGPKCON  ( *(volatile unsigned *)(gGPIOVAddr.base+0x800) )
#define rGPKDAT  ( *(volatile unsigned *)(gGPIOVAddr.base+0x808) )
#define rGPKPUD  ( *(volatile unsigned *)(gGPIOVAddr.base+0x80C) )

struct GPIOVirtualAddress
{
  void __iomem *base;
};
struct GPIOVirtualAddress  gGPIOVAddr;

struct work_struct pwr_work;

int mem_remap(void)
{
   if(request_mem_region(MEMSTARTADDR,PAGESIZE,"GPIOMem")!=NULL)
   {
      gGPIOVAddr.base=ioremap(MEMSTARTADDR,PAGESIZE);
   }
   else
   {
   	  printk(KERN_INFO "mem_remap: request_mem_region error\n");
      return -1;
   }

   return 0;
}

int mem_uremap(void)
{
   iounmap(gGPIOVAddr.base);
   release_mem_region(MEMSTARTADDR,PAGESIZE);
   return 0;
}

//********************************************************************************

int headset_state(void)
{

	unsigned int	hp_conectd_n = 0;
	unsigned int 	iTmp = 0;
	unsigned int 	iCntr = 0;

	/* set GPN(2) as input to get hp status*/
	s3c_gpio_cfgpin(S3C64XX_GPN(2),S3C64XX_GPN_INPUT(2));
	/* pull-up enable*/
	s3c_gpio_setpull(S3C64XX_GPN(2), S3C_GPIO_PULL_UP);

	hp_conectd_n = gpio_get_value(S3C64XX_GPN(2));

	return (!hp_conectd_n);	
}
EXPORT_SYMBOL_GPL(headset_state);

//********************************************************************************
int S3c_hp_status(void)
{

	unsigned int	hp_conectd_n = 0;
	unsigned int 	iTmp = 0;
	unsigned int 	iCntr = 0;

	/* set GPN(2) as input to get hp status*/
	s3c_gpio_cfgpin(S3C64XX_GPN(2),S3C64XX_GPN_INPUT(2));

	/* pull-up enable*/
	s3c_gpio_setpull(S3C64XX_GPN(2), S3C_GPIO_PULL_UP);

	/* check 500us to make sure the hotplug is stable */
	while (1) 
	{
		msleep(20);

		/* get gpn(4) data, 1-> hp not connected; 0-> hp connected*/
		iTmp = gpio_get_value(S3C64XX_GPN(2));

		if (iTmp!=hp_conectd_n)	
		{
			hp_conectd_n = iTmp;
			iCntr = 0;
		}
		else 
		{
			if (iCntr++>3) 
				break;
		}
	}

	return (!hp_conectd_n);
}

/////////////////////////////////////////////////////////////////////////////////

static ssize_t hp_status_show(struct device *dev, struct device_attribute *attr, 	char *buf)
{
	//int connect=S3c_hp_status();
	int connect=headset_state();
	return sprintf(buf, "%d\n", connect);
}

static struct device_attribute hp_status = {
	.attr = {.name = "headphone_status", .mode = 0644},
	.show = hp_status_show,
};

/////////////////////////////////////////////////////////////////////////////////
#define MUTE_SPEAKER 1
#define UMUTE_SPEAKER 0

static ssize_t mute_speaker_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
 	char *p;
	int len;
  
	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;
        
     //   buf[len]='\0';
    //    printk("len: %d buf:%s",len,buf);
 
  // "mute", "umute"
	if (len == 4 && !strncmp(buf, "mute", len)) 
	{   
		  printk("mute_speaker...\n");
		  SpeakerMute(MUTE_SPEAKER);
	}
	else if (len == 5 && !strncmp(buf, "umute", len)) 
	{   
		  printk("umute_speaker...\n");
		  SpeakerMute(UMUTE_SPEAKER);
	}	
	else if (len == 1 && !strncmp(buf, "1", len)) 
	{   
		  printk("mute_speaker...\n");
		  SpeakerMute(MUTE_SPEAKER);
	}
	else if (len == 1 && !strncmp(buf, "0", len)) 
	{   
		  printk("umute_speaker...\n");
		  SpeakerMute(UMUTE_SPEAKER);
	}	
	
	
	return n;
}

static struct device_attribute mute_speaker_status = {
	.attr = {.name = "mute_speaker", .mode = 0222},
	.store = mute_speaker_store,
};

///////////////////////////////////////////////////////////////////////////////////
#define MUTE_HEADPHONE  1
#define UMUTE_HEADPHONE 0

static ssize_t mute_headphone_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
 	char *p;
	int len;
  
	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

      // "mute", 
	if ( (strncmp(buf, "mute", len)==0) || (strncmp(buf, "1", len)==0) )
	{   
		  printk("mute_headphone...\n");
		  ssm2603_mute_headphone(MUTE_HEADPHONE);
	}
     	//"umute"
	else if ( (strncmp(buf, "umute", len)==0) || (strncmp(buf, "0", len)==0) )
	{   
		  printk("umute_headphone...\n");
		  ssm2603_mute_headphone(UMUTE_HEADPHONE);
	}
	
	return n;
}

static struct device_attribute mute_headphone_status = {
	.attr = {.name = "mute_headphone", .mode = 0222},
	.store = mute_headphone_store,
};

///////////////////////////////////////////////////////////////////////////////////
//int s3c_pwrkey_status(void)
//{
//	unsigned int	pwrkey_conectd_n = 0;
//	unsigned int 	iTmp = 0;
//	unsigned int 	iCntr = 0;
//
//	/* set GPN(7) as input to get pwrkey status*/
//	s3c_gpio_cfgpin(S3C64XX_GPN(7),S3C64XX_GPN_INPUT(7));
//	/* pull-up enable*/
//	s3c_gpio_setpull(S3C64XX_GPN(7), S3C_GPIO_PULL_UP);
//
//	/* check 500us to make sure the hotplug is stable */
//	while (1) 
//	{
//		msleep(20);
//
//		/* get gpn(4) data, 1-> hp not connected; 0-> hp connected*/
//		iTmp = gpio_get_value(S3C64XX_GPN(7));
//
//		if (iTmp!=pwrkey_conectd_n)	
//		{
//			pwrkey_conectd_n = iTmp;
//			iCntr = 0;
//		}
//		else 
//		{
//			if (iCntr++>2) 
//				break;
//		}
//	}
//
//	return (!pwrkey_conectd_n);
//}
//
//extern void kernel_power_off(void);
//extern void epd_show_second_logo(unsigned short cmd);
//void poweroff_work(struct work_struct *work)
//{
//			printk("goto  poweroff ...\n");	
//#ifdef 	CONFIG_FB_EINK    
//	    epd_show_second_logo(0);
//#endif
//
//	    rGPKCON = ( rGPKCON & ~(0xf << 8 )) | ( 0x1 << 8 );
//	    rGPKCON = ( rGPKCON & ~(0xf << 4 )) | ( 0x1 << 4 );
//	    
//	    
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);       
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//
//      msleep(500);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);       	        
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);             		  
//      msleep(500);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//      msleep(500);     
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       		  
//      msleep(500);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//      msleep(500);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//      msleep(300);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//      msleep(200);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//      msleep(200);
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
//		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
//		  		  		  
//			printk("after poweroff ...\n");		    
//			kernel_power_off();
//}
//
//static ssize_t s3c_pwrkey_status_store(struct kobject *kobj, struct kobj_attribute *attr,
//			   const char *buf, size_t n)
//{
// 	char *p;
//	int len;
//
//	p = memchr(buf, '\n', n);
//	len = p ? p - buf : n;
//
//	if (len == 8 && !strncmp(buf, "poweroff", len)) 
//	{   
//		  schedule_work(&pwr_work);
//	}
//  //printk("in state_store func...\n");
//  
//  return n;
//}
//
//static ssize_t pwrkey_status_show(struct device *dev, struct device_attribute *attr, 	char *buf)
//{
//	int connect=s3c_pwrkey_status();
//	return sprintf(buf, "%d\n", connect);
//}
//
//static struct device_attribute pwrkey_status = {
//	.attr = {.name = "pwrkey", .mode = 0644},
//	.show = pwrkey_status_show,
//	.store= s3c_pwrkey_status_store,
//};
//
//
///////////////////////////////////////////////////////////////////////////////////

static int headset_s3c_probe(struct platform_device *pdev)
{
	int ret;
	
	printk("s3c: Registering s3c (headset) driver +++\n");

  //mute_headphone_status
  ret=device_create_file(&(pdev->dev), &mute_headphone_status);
  ret=device_create_file(&(pdev->dev), &mute_speaker_status);
  ret=device_create_file(&(pdev->dev), &hp_status);

  //ret=device_create_file(&(pdev->dev), &pwrkey_status);
 
  return ret;
}

static int headset_s3c_remove(struct platform_device *pdev)
{

	if ( !(&(pdev->dev)) )
		return -1;

	device_remove_file(&(pdev->dev), &mute_headphone_status);
	device_remove_file(&(pdev->dev), &mute_speaker_status);
	device_remove_file(&(pdev->dev), &hp_status);

//device_remove_file(&(pdev->dev), &pwrkey_status);			
	return 0;
}

static int headset_s3c_suspend(struct platform_device *pdev, pm_message_t state)
{
	//SpeakerMute(1);
	return 0;
}	
static int headset_s3c_resume(struct platform_device *pdev)
{
	//SpeakerMute(0);
	return 0;
}

static struct platform_device headset_s3c_device = {
	.name		= "gpio-keys",
};

static struct platform_driver headset_s3c_driver = {
	.probe		= headset_s3c_probe,
	.remove		= headset_s3c_remove,
	.suspend    = headset_s3c_suspend,
	.resume     = headset_s3c_resume,
	.driver		= {
		.name		= "gpio-keys",
		.owner		= THIS_MODULE,
	},
};

static int __init headset_s3c_init(void)
{
	int ret;
	printk("[headsetset] driver !\n");
	
  //mem_remap();  
  //INIT_WORK(&pwr_work, poweroff_work);
  
	ret = platform_driver_register(&headset_s3c_driver);
	if (ret)
		return ret;
	return platform_device_register(&headset_s3c_device);
}

static void __exit headset_s3c_exit(void)
{
	//mem_uremap();
	
	platform_device_unregister(&headset_s3c_device);
	platform_driver_unregister(&headset_s3c_driver);
}

late_initcall(headset_s3c_init);
module_exit(headset_s3c_exit);

MODULE_AUTHOR("Hill.Y.Zhang");
MODULE_DESCRIPTION("driver for headset");
MODULE_LICENSE("GPL");
