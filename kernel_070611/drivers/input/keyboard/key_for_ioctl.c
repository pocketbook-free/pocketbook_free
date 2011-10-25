/*
	Key driver use character device driver, for ep series use
	io control to poll the key state. reserve input event for 
	usb/BT keyboard.
*/
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


#include <asm/gpio.h> 
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>


#define IOCTL_KEY_NAME 	                  "ioctl key"
#define CM_KEY 			                        106
#define CM_KEYSTATE      	                  107
#define CM_POWER_BTN                        110
#define CM_POWER_BTN_STATE                  111
#define CM_KEYLOCK                          137    

#define KEY_BACK 					                  1 << 0
#define KEY_DELETE				         	        1 << 1
#define KEY_Navigation_LEFT		     	        1 << 2
#define KEY_HOME					                  1 << 3
#define KEY_MUSIC					                  1 << 4
#define KEY_Navigation_OK			              1 << 5
#define KEY_Navigation_RIGHT	            	1 << 6
#define KEY_PLUS_Zoom_In			              1 << 7
#define KEY_MINUS_Zoom_Out			            1 << 8
#define KEY_Navigation_DOWN			            1 << 9
#define KEY_Navigation_UP		              	1 << 10
#define KEY_PREV_PAGE_right_side	          1 << 11
#define KEY_NEXT_PAGE_right_side	          1 << 12
#define KEY_NEXT_PAGE_2_left_side	          1 << 13
#define KEY_POWER_short_press		            1 << 14
#define KEY_PREV_PAGE_2_left_side	          1 << 15
#define KEY_MENU					                  1 << 16
#define TD_SWITCH                           1 << 17
#define PEN_OUT                             1 << 18
#define SIZE_Up                             1 << 19
#define SIZE_Dn                             1 << 20
#define Volume_Up                           1 << 21
#define Volume_Dn                           1 << 22


#define IO_KEY_POWER 		KEY_POWER_short_press
#define IO_KEY_MENU			KEY_MENU
#define IO_KEY_HOME			KEY_HOME
#define IO_KEY_PAGE_UP		KEY_PREV_PAGE_right_side
#define IO_KEY_PAGE_DOWN	KEY_NEXT_PAGE_right_side
#define IO_KEY_BACK			KEY_BACK
#define IO_NAVIGATE_L		KEY_Navigation_LEFT
#define IO_NAVIGATE_U		KEY_Navigation_UP
#define IO_NAVIGATE_D		KEY_Navigation_DOWN
#define IO_NAVIGATE_M		KEY_Navigation_OK
#define IO_NAVIGATE_R		KEY_Navigation_RIGHT
#define IO_TD_SWITCH		TD_SWITCH
#define IO_PEN_OUT		  PEN_OUT
#define IO_SIZE_Up      SIZE_Up
#define IO_SIZE_Dn      SIZE_Dn
#define IO_Volume_Up    Volume_Up
#define IO_Volume_Dn    Volume_Dn



#define BUF_SIZE                  20

#define MORSE_BUTTON  1

extern unsigned long morse_key_value ;
extern unsigned long morse_key_flag ;

int key_lock=0;
//static unsigned long source = 0;
unsigned long source = 0;
static int sleep_flag = 0;
struct ioctl_key_dev_t
{
	struct cdev cdev; 
};
struct ioctl_key_dev_t ioctl_key_dev;
static int ioctl_key_major = 244;

/*
static void ioctl_key_init_register(void)
{
	#if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
	s3c_gpio_cfgpin(S3C64XX_GPN(1), S3C_GPIO_SFN(0) );//KEY_POWER 
	s3c_gpio_setpull(S3C64XX_GPN(1), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S3C64XX_GPL(8), S3C_GPIO_SFN(0));//KEY_MENU
	s3c_gpio_cfgpin(S3C64XX_GPL(9), S3C_GPIO_SFN(0));//KEY_HOME
	s3c_gpio_cfgpin(S3C64XX_GPL(10), S3C_GPIO_SFN(0));//KEY_UP
	s3c_gpio_cfgpin(S3C64XX_GPL(11), S3C_GPIO_SFN(0));//KEY_DOWN
	s3c_gpio_cfgpin(S3C64XX_GPL(12), S3C_GPIO_SFN(0));//KEY_BACK
	s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(0));//KEY_5
	s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C_GPIO_SFN(0));//KEY_6
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(0));//KEY_7
	s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(0));//KEY_8
	s3c_gpio_cfgpin(S3C64XX_GPN(10), S3C_GPIO_SFN(0));//KEY_9
	s3c_gpio_cfgpin(S3C64XX_GPM(2), S3C_GPIO_SFN(0));//TD_SWITCH
	s3c_gpio_cfgpin(S3C64XX_GPN(4), S3C_GPIO_SFN(0));//PEN_OUT
	
	#endif
	
	
	#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)||defined(CONFIG_HW_EP4_DVT)
	s3c_gpio_cfgpin(S3C64XX_GPN(1), S3C_GPIO_SFN(0));//KEY_POWER
	s3c_gpio_cfgpin(S3C64XX_GPL(8), S3C_GPIO_SFN(0));//KEY_MENU
	s3c_gpio_cfgpin(S3C64XX_GPL(9), S3C_GPIO_SFN(0));//KEY_HOME
	s3c_gpio_cfgpin(S3C64XX_GPL(10), S3C_GPIO_SFN(0));//KEY_UP
	s3c_gpio_cfgpin(S3C64XX_GPL(11), S3C_GPIO_SFN(0));//KEY_DOWN
	s3c_gpio_cfgpin(S3C64XX_GPL(12), S3C_GPIO_SFN(0));//KEY_BACK
	s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(0));//5 way key control
	s3c_gpio_cfgpin(S3C64XX_GPF(0), S3C_GPIO_SFN(0));//KEY_5
	s3c_gpio_cfgpin(S3C64XX_GPF(1), S3C_GPIO_SFN(0));//KEY_6
	s3c_gpio_cfgpin(S3C64XX_GPF(2), S3C_GPIO_SFN(0));//KEY_7
	s3c_gpio_cfgpin(S3C64XX_GPF(3), S3C_GPIO_SFN(0));//KEY_8
	s3c_gpio_cfgpin(S3C64XX_GPF(4), S3C_GPIO_SFN(0));//KEY_9
	s3c_gpio_cfgpin(S3C64XX_GPN(8), S3C_GPIO_SFN(0));//TD_SWITCH
	s3c_gpio_cfgpin(S3C64XX_GPN(4), S3C_GPIO_SFN(0));//PEN_OUT
	
	s3c_gpio_cfgpin(S3C64XX_GPM(2), S3C_GPIO_SFN(0));//SIZE_Up
	s3c_gpio_cfgpin(S3C64XX_GPL(14), S3C_GPIO_SFN(0));//SIZE_Dn
	s3c_gpio_cfgpin(S3C64XX_GPN(6), S3C_GPIO_SFN(0));//Volume_Up
	s3c_gpio_cfgpin(S3C64XX_GPN(7), S3C_GPIO_SFN(0));//Volume_Dn
	#endif
}
*/


static void io_key_poll(unsigned long * key_val)
{
	static int count = 0;
	*key_val = 0;
//	printk("morse_key_flag = %d \n", morse_key_flag) ;
	#if MORSE_BUTTON
		
		if(morse_key_flag == 1) //morse
		{
			printk("***************== %s ==*************\n", __FUNCTION__) ;
			printk("morse_key_flag = %ld \n", morse_key_flag) ;
			*key_val = morse_key_value ;
			printk("*key_val = 0x%x \n", *key_val) ;
			//morse_key_flag = 0 ;
			count++;
			if(count > 3)
			{
				 morse_key_value = 0x40000;
				 morse_key_flag = 0 ;
				 count = 0;
			}
			return ;
		}
	#endif
	//return 0 ;
	  #if defined(CONFIG_HW_EP1_EVT)||defined(CONFIG_HW_EP2_EVT)
		if(!gpio_get_value(S3C64XX_GPN(1)))		{ *key_val |= IO_KEY_POWER;}		
		if(!gpio_get_value(S3C64XX_GPL(8)))		{ *key_val |= IO_KEY_MENU; }
		if(!gpio_get_value(S3C64XX_GPL(9)))		{ *key_val |= IO_KEY_HOME; }
		if(!gpio_get_value(S3C64XX_GPL(10)))	    { *key_val |= IO_KEY_PAGE_UP; }
		if(!gpio_get_value(S3C64XX_GPL(11)))	    { *key_val |= IO_KEY_PAGE_DOWN; }
		if(!gpio_get_value(S3C64XX_GPL(12)))	    { *key_val |= IO_KEY_BACK; }
		if(!gpio_get_value(S3C64XX_GPN(6)))	    { *key_val |= IO_NAVIGATE_L; }
		if(!gpio_get_value(S3C64XX_GPN(12)))	    { *key_val |= IO_NAVIGATE_R; }
		if(!gpio_get_value(S3C64XX_GPN(8)))		{ *key_val |= IO_NAVIGATE_U; }
		if(!gpio_get_value(S3C64XX_GPN(9)))		{ *key_val |= IO_NAVIGATE_D; }
		if(!gpio_get_value(S3C64XX_GPN(10)))	    { *key_val |= IO_NAVIGATE_M; }
		if(!gpio_get_value(S3C64XX_GPM(2)))	    { *key_val |= IO_TD_SWITCH; }
		if(!gpio_get_value(S3C64XX_GPN(4)))	    { *key_val |= IO_PEN_OUT; }
		#endif
		
		#if defined(CONFIG_HW_EP1_EVT2)||defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_EVT2)||defined(CONFIG_HW_EP2_DVT)
		if(!gpio_get_value(S3C64XX_GPN(1)))		{ *key_val |= IO_KEY_POWER; }
		if(!gpio_get_value(S3C64XX_GPL(8)))		{ *key_val |= IO_KEY_MENU; }
		if(!gpio_get_value(S3C64XX_GPL(9)))		{ *key_val |= IO_KEY_HOME; }
		if(!gpio_get_value(S3C64XX_GPL(10)))	    { *key_val |= IO_KEY_PAGE_UP; }
		if(!gpio_get_value(S3C64XX_GPL(11)))	    { *key_val |= IO_KEY_PAGE_DOWN; }
		if(!gpio_get_value(S3C64XX_GPL(12)))	    { *key_val |= IO_KEY_BACK; }
		if(!gpio_get_value(S3C64XX_GPN(6)))	    { *key_val |= IO_NAVIGATE_D; }
		if(!gpio_get_value(S3C64XX_GPN(12)))	    { *key_val |= IO_NAVIGATE_U; }
		if(!gpio_get_value(S3C64XX_GPN(8)))		{ *key_val |= IO_NAVIGATE_L; }
		if(!gpio_get_value(S3C64XX_GPN(9)))		{ *key_val |= IO_NAVIGATE_R; }
		if(!gpio_get_value(S3C64XX_GPN(10)))	    { *key_val |= IO_NAVIGATE_M; }
		if(!gpio_get_value(S3C64XX_GPM(2)))	    { *key_val |= IO_TD_SWITCH; }
		if(!gpio_get_value(S3C64XX_GPN(4)))	    { *key_val |= IO_PEN_OUT; }
		#endif
		
		#if defined(CONFIG_HW_EP3_EVT)||defined(CONFIG_HW_EP3_EVT2)||defined(CONFIG_HW_EP4_EVT)||defined(CONFIG_HW_EP4_EVT2)
		if(!gpio_get_value(S3C64XX_GPN(1)))		{ *key_val |= IO_KEY_POWER; }
	    if(!gpio_get_value(S3C64XX_GPL(8)))		{ *key_val |= IO_KEY_MENU; }
	    if(!gpio_get_value(S3C64XX_GPL(9)))		{ *key_val |= IO_KEY_HOME; }
	    if(!gpio_get_value(S3C64XX_GPL(10)))	{ *key_val |= IO_KEY_PAGE_UP; }
	    if(!gpio_get_value(S3C64XX_GPL(11)))	{ *key_val |= IO_KEY_PAGE_DOWN; }
	    if(!gpio_get_value(S3C64XX_GPL(12)))	{ *key_val |= IO_KEY_BACK; }
	    if(!gpio_get_value(S3C64XX_GPF(0)))		{ *key_val |= IO_NAVIGATE_L; }
     	if(!gpio_get_value(S3C64XX_GPF(1)))		{ *key_val |= IO_NAVIGATE_U; }
    	if(!gpio_get_value(S3C64XX_GPF(2)))		{ *key_val |= IO_NAVIGATE_D; }
    	if(!gpio_get_value(S3C64XX_GPF(3)))		{ *key_val |= IO_NAVIGATE_M; }
	    if(!gpio_get_value(S3C64XX_GPF(4)))		{ *key_val |= IO_NAVIGATE_R; }
		if(!gpio_get_value(S3C64XX_GPN(8)))	  { *key_val |= IO_TD_SWITCH; }
		if(!gpio_get_value(S3C64XX_GPN(4)))	  { *key_val |= IO_PEN_OUT; }	
		
		if(!gpio_get_value(S3C64XX_GPM(2)))	  { *key_val |= IO_SIZE_Up; }
		if(!gpio_get_value(S3C64XX_GPL(14)))	{ *key_val |= IO_SIZE_Dn; }
		if(!gpio_get_value(S3C64XX_GPN(6)))	  { *key_val |= IO_Volume_Up; }
		if(!gpio_get_value(S3C64XX_GPN(7)))	  { *key_val |= IO_Volume_Dn; }		  
	  #endif
	  
	  #if defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
		if(!gpio_get_value(S3C64XX_GPN(1)))		{ *key_val |= IO_KEY_POWER; }
	    if(!gpio_get_value(S3C64XX_GPL(8)))		{ *key_val |= IO_KEY_MENU; }
	    if(!gpio_get_value(S3C64XX_GPL(9)))		{ *key_val |= IO_KEY_HOME; }
	    if(!gpio_get_value(S3C64XX_GPL(10)))	{ *key_val |= IO_KEY_PAGE_UP; }
	    if(!gpio_get_value(S3C64XX_GPL(11)))	{ *key_val |= IO_KEY_PAGE_DOWN; }
	    if(!gpio_get_value(S3C64XX_GPL(12)))	{ *key_val |= IO_KEY_BACK; }
	    if(!gpio_get_value(S3C64XX_GPF(0)))		{ *key_val |= IO_NAVIGATE_L; }
   	    if(!gpio_get_value(S3C64XX_GPF(1)))		{ *key_val |= IO_NAVIGATE_U; }
     	if(!gpio_get_value(S3C64XX_GPF(2)))		{ *key_val |= IO_NAVIGATE_D; }
    	if(!gpio_get_value(S3C64XX_GPF(3)))		{ *key_val |= IO_NAVIGATE_M; }
	    if(!gpio_get_value(S3C64XX_GPF(4)))		{ *key_val |= IO_NAVIGATE_R; }
		if(!gpio_get_value(S3C64XX_GPN(8)))	  { *key_val |= IO_TD_SWITCH; }
		if(!gpio_get_value(S3C64XX_GPN(4)))	  { *key_val |= IO_PEN_OUT; }	
		
		if(!gpio_get_value(S3C64XX_GPM(2)))	  { *key_val |= IO_SIZE_Up; }
		if(!gpio_get_value(S3C64XX_GPN(10)))	{ *key_val |= IO_SIZE_Dn; }
		if(!gpio_get_value(S3C64XX_GPN(6)))	  { *key_val |= IO_Volume_Up; }
		if(!gpio_get_value(S3C64XX_GPN(3)))	  { *key_val |= IO_Volume_Dn; }		  
	  #endif
	
//	printk("not mouse button, *key_val = 0x%x \n", *key_val) ;
//		#ifdef MORSE_BUTTON
//			*key_val |= morse_button_value ;
//			get_mouse_button(key_val);
//		#endif		
}

int set_sleep_flag(int flag)
{
	sleep_flag =flag;
	return sleep_flag;
}
int get_sleep_flag(void)
{
	return sleep_flag;
}
unsigned long getwakeupsource(void)
{
	//unsigned int eint0pend = __raw_readl(S3C64XX_EINT0PEND);
	io_key_poll(&source);
//	source=0x41000 ;
	printk("1:source = 0x%x \n", source) ;
	return source;	
}

EXPORT_SYMBOL(getwakeupsource);
EXPORT_SYMBOL(set_sleep_flag);

extern unsigned long wakeup_key;
static void powerkey_state(int * v, int waketest)
{
	unsigned long key_val; 
	io_key_poll(&key_val);
	if(key_val & IO_KEY_POWER)
	{   
		* v = 0; 
	}
	else if(waketest && (wakeup_key&IO_KEY_POWER))
	{
		* v = 0;
		wakeup_key=0;
	}
	else
		* v = 1;
}


static void keylock(int * v_lock)
{
	if(* v_lock==1)
	{
			key_lock=1;
	}
	else
		  key_lock=0;
	
}
static int ioctl_key_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ioctl_key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ioctl_key_ioctl(struct inode *inode, struct file *filp, \
					unsigned int cmd, unsigned long arg)
{
	unsigned long key_value ;
	int v;
	int v_lock;

	switch(cmd)
	{
		case CM_KEY:
			if(get_sleep_flag())
			{
				set_sleep_flag(0);		//clear flag
				key_value = source;
				printk("\nkey_value = source : 0x%x \n", key_value) ;
				source=0x40000;	
			}
			else		
			    io_key_poll(&key_value);
 				//printk("-----CM_KEY---key_value= %x ------\n",key_value& 0x03ffff);
			if(copy_to_user((unsigned long *)arg, &key_value, \
							sizeof(key_value)) != 0)
			return -EFAULT;
			break;

		case CM_KEYSTATE:
		    	io_key_poll(&key_value);
				//printk("-----CM_KEYSTATE---key_value= %x ------\n",key_value& 0x03ffff);
			if(copy_to_user((unsigned long *)arg, &key_value, \
						sizeof(key_value)) != 0)
			return -EFAULT;			
			break;

		case CM_POWER_BTN:
			powerkey_state(&v, 1);
			if(copy_to_user((unsigned long *)arg, &v, \
						sizeof(v)) != 0)
				return -EFAULT;
			break;

		case CM_POWER_BTN_STATE:
			powerkey_state(&v, 0);
			if(copy_to_user((unsigned long *)arg, &v, \
						sizeof(v)) != 0)
				return -EFAULT;
				break;
				
		case CM_KEYLOCK:
			if(copy_from_user(&v_lock, (const unsigned long *)arg, \
							sizeof(v_lock)) != 0)
				return -EFAULT;
				printk("the v_lock is %d\n",v_lock);
			    keylock(&v_lock);
				printk("the key_lock is %d\n",key_lock);	
				break;
		default:
			return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(key_lock);

static const struct file_operations ioctl_key_fops = {
	.owner 		= THIS_MODULE,
	.open		  = ioctl_key_open,
	.release	= ioctl_key_release,
	.ioctl		= ioctl_key_ioctl,
};

static int __init ioctl_keys_init(void)
{
	int ret;
	dev_t devno = MKDEV(ioctl_key_major, 0);
	printk("~~~~~~~~~~~~~~ Blazer:EP series ioctl key~~~~~~~~~~~~~~~~~\n");
	//ioctl_key_init_register();
	if (ioctl_key_major)
		ret = register_chrdev_region(devno, 1, IOCTL_KEY_NAME);
	else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, IOCTL_KEY_NAME);
		ioctl_key_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;

	cdev_init(&ioctl_key_dev.cdev, &ioctl_key_fops);
	
	ret = cdev_add(&ioctl_key_dev.cdev, devno, 1);
	if (ret)
		printk(KERN_NOTICE "Error cannot add ioctl key\n");
	
	return ret;
}

static void __exit ioctl_keys_exit(void)
{
	unregister_chrdev_region(ioctl_key_major, 1);
	cdev_del(&ioctl_key_dev.cdev);
}

module_init(ioctl_keys_init);
module_exit(ioctl_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Blazer Lai");
MODULE_DESCRIPTION("Key device driver for IO control");
MODULE_ALIAS("platform:keys_for_ioctl");

