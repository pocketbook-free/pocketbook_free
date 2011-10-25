#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/types.h>
#include <asm/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>

#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/timer.h> 

#include <asm/atomic.h> 

#include <asm/ptrace.h>

#include <linux/kernel.h>

#include <linux/device.h>
//#include <asm/hardware.h>
#include <linux/unistd.h>

extern void msleep(unsigned int msecs);
//*******************************************************
#define PAGESIZE 0x1024
#define MEMSTARTADDR 0x7F008000

#define rGPNCON  ( *(volatile unsigned *)(gGPIOVAddr.base+0x830) )
#define rGPNDAT  ( *(volatile unsigned *)(gGPIOVAddr.base+0x834) )
#define rGPNPUD  ( *(volatile unsigned *)(gGPIOVAddr.base+0x838) )

#define rGPKCON  ( *(volatile unsigned *)(gGPIOVAddr.base+0x800) )
#define rGPKDAT  ( *(volatile unsigned *)(gGPIOVAddr.base+0x808) )
#define rGPKPUD  ( *(volatile unsigned *)(gGPIOVAddr.base+0x80C) )

static signed   char bPWR_Down=-1;
static unsigned char count=0;

struct work_struct pwr_work;

extern void epd_show_second_logo(unsigned short cmd);
void myUsleep(unsigned long useconds)
{
   unsigned int i;
   for(i=0;i<useconds*100;i++)
	   __asm__ __volatile__ ("nop"); 
}

struct GPIOVirtualAddress
{
  void __iomem *base;
};
struct GPIOVirtualAddress  gGPIOVAddr;

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

//****************************************************************************
void poweroff_work(struct work_struct *work)
{
			printk("goto  poweroff ...\n");	
	    
	    epd_show_second_logo(0);

	    rGPKCON = ( rGPKCON & ~(0xf << 8 )) | ( 0x1 << 8 );
	    rGPKCON = ( rGPKCON & ~(0xf << 4 )) | ( 0x1 << 4 );
	    
	    
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);       
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       

      msleep(500);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);       	        
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);             		  
      msleep(500);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
      msleep(500);     
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       		  
      msleep(500);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
      msleep(500);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
      msleep(300);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
      msleep(200);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x1<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
      msleep(200);
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 2 )) | ( 0x0<< 2);             
		  rGPKDAT = ( rGPKDAT & ~(0x1 << 1 )) | ( 0x0<< 1);       
		  		  		  
			printk("after poweroff ...\n");		    
			
	    //msleep(10000);
	  	rGPNCON = ( rGPNCON & ~(0x3 << 24 )) | ( 0x1 << 24 );     //output mode
		  rGPNDAT = ( rGPNDAT & ~(0x1 << 12 )) | ( 0x0<< 12);       //umute speaker
}
//****************************************************************************
struct timer_list s_timer;

static void second_timer_handle(unsigned long arg)
{
  mod_timer(&s_timer,jiffies + HZ/2);

  rGPNCON = ( rGPNCON & ~(0x3 << 14 )) | ( 0x0 << 14);  //set input mode
  rGPNPUD = ( rGPNPUD & ~(0x3 << 14 )) | ( 0x2 << 14);  //set PULL UP Enable
  if( (rGPNDAT  & ( 0x1<< 7) )==0)
  {
	  if( (bPWR_Down<=0))
	  {   
		  bPWR_Down=1;
		  count++;
	    printk(KERN_INFO "power key pressed...!\n");
	  }
	  else if( (bPWR_Down==1))
	  {
			  count++;
		    printk(KERN_INFO "power key pressed...!\n");
	  }
	  
	  if(count>=10)
	  {
	  	mod_timer(&s_timer,jiffies + HZ*100);
      schedule_work(&pwr_work);  	  	
	  	count=0;
	  	bPWR_Down=0;
		}
  }
  else
  {
  	  bPWR_Down=0;
		  count=0;
	    //printk(KERN_INFO "power key released ...!\n");
  }
}


int second_init(void)
{
  mem_remap();

  msleep(2);
  
  INIT_WORK(&pwr_work, poweroff_work);

  init_timer(&s_timer);
  s_timer.function = &second_timer_handle;
  s_timer.expires = jiffies + HZ;
  add_timer(&s_timer); 
  return 0;
}

void second_exit(void)
{
  mem_uremap();
  del_timer(&s_timer);
}

MODULE_AUTHOR("Hill");
MODULE_LICENSE("Dual BSD/GPL");

module_init(second_init);
module_exit(second_exit);
