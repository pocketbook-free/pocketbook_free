/*
 * linux/fs/proc/hwconfig.c
 * 
 * Function : IO driver for CM_PLATFORM, CM_HWCONFIG, CM_SET_HWCONFIG
 * Author   : BSP03 of TMSBG Gavin liu
 * Date     : 20100910
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/gpio.h>
#define   HWCONFIG_MAJOR    252
#define   HWCONFIG_NAME     "hwconfig"


#define   CM_PLATFORM       164
#define   CM_HWCONFIG       165
#define   CM_SET_HWCONFIG   166


unsigned long long hwconfig;
char hw_config[22];
char platform[16];

int hwconfig_major = HWCONFIG_MAJOR;

struct hwconfig 
{
struct cdev cdev;
};

struct hwconfig  *hwconfig_devp;


static  int hwinfo_from_uboot( void)
{

char *command_line = saved_command_line;
char *pch;
char *pf;

pf = strstr(command_line,"platform=");
if (pf != 0)
{
memcpy(platform,pf,16);

}

pch = strstr(command_line,"0x");
if(pch != 0)
{
    memcpy(hw_config,pch,22);
}

return 0;
}

static int hwconfig_open(struct inode *inode, struct file *filp)
{
  
  return 0;
}


static int hwconfig_ioctl(struct inode *inodep,struct file *filp,unsigned int cmd,unsigned long arg)
{
   
  int retval = 0;
   switch(cmd)
    {    
     
      case CM_PLATFORM :
        
             retval = copy_to_user((char *)arg,platform,sizeof(platform));
             printk(KERN_ALERT"OOOOOO=%d\n",retval);
             break;

      case CM_HWCONFIG :
      	   
            retval = copy_to_user((unsigned long long *)arg,&hwconfig,sizeof(hwconfig));
            printk(KERN_ALERT"AAAAAAAA=%d\n",retval);
	    break;
     
      case CM_SET_HWCONFIG :
           
            if(! capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = copy_from_user(&hwconfig,(unsigned long long *)arg,sizeof(hwconfig)); 
            printk(KERN_ALERT"BBBBBBB=%d\n",retval);
            printk(KERN_ALERT"hwconfig=%lld\n",hwconfig);
            break; 

      default:
	    	return - EINVAL;
	  }
	  return retval;  	     
	         
}

static int hwconfig_release(struct inode *inode, struct file *filp)
{
return 0;
}


struct file_operations hwconfig_fops =

{
   .owner = THIS_MODULE,
   .open  = hwconfig_open,
   .ioctl = hwconfig_ioctl,
   .release = hwconfig_release,	
};


static int __init hwconfig_init(void)
{

  int result,err;
  dev_t devno = MKDEV(hwconfig_major,0);
 
 hwinfo_from_uboot();
  printk(KERN_ALERT"HWCONFIG=%s\n",hw_config);
  
  
  printk(KERN_ALERT"PLATFORM=%s\n",platform);
  
  hwconfig = simple_strtoull(hw_config,NULL,16);
  printk(KERN_ALERT"hwconfig=%lld\n",hwconfig);
  


if(hwconfig_major)
	 result = register_chrdev_region(devno,1,HWCONFIG_NAME);
else 
{
	 result = alloc_chrdev_region(&devno,0,1,HWCONFIG_NAME);
	 hwconfig_major = MAJOR(devno);
}

if (result < 0)
	return result;
		
hwconfig_devp = kmalloc(sizeof(struct hwconfig),GFP_KERNEL);

if(!hwconfig_devp)
{
  result = - ENOMEM;
  goto fail_malloc;
}

memset(hwconfig_devp,0,sizeof(struct hwconfig));

 	 	 
cdev_init(&hwconfig_devp->cdev,&hwconfig_fops);

err = cdev_add(&hwconfig_devp->cdev,devno,1);

if(err)
	
printk(KERN_NOTICE "Error cdev_add!\n");

return 0;

fail_malloc: unregister_chrdev_region(devno,1);
return result;	
	
}

void hwconfig_exit(void)
{
 cdev_del(&hwconfig_devp->cdev);
 kfree(hwconfig_devp);
 unregister_chrdev_region(MKDEV(hwconfig_major,0),1);
}


module_init(hwconfig_init);
module_exit(hwconfig_exit);

MODULE_AUTHOR("TMSBG-BSP03 Gavin liu");
MODULE_DESCRIPTION("IO driver for CM_PLATFORM, CM_HWCONFIG, CM_SET_HWCONFIG");
MODULE_LICENSE("Dual BSD/GPL");
