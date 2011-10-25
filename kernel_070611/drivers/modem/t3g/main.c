#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>


#include <linux/irq.h>
#include <linux/interrupt.h>	
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
#include <asm/uaccess.h>	

static int modem_PW = 0;
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status

static int modem_gpio_cfg(void);
void modem_hsdpa_pwr (int on);
//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
void usb_interface_pwr (int on);
static int modem_switch_status(void);
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
//static irqreturn_t modem_switch_irq_handler(int irqno, void *dev_id);
static int modem_td_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int modem_td_suspend(void);
static int modem_td_resume(void);



//MODULE_LICENSE("Dual BSD/GPL");
MODULE_LICENSE("GPL");

#define MODEM_MAJOR     232
#define MODEM_TD_DRIVER	"Modem_TD"

#define SET_PPPD_PID      0x00
#define GET_PPPD_PID      0x01	
#define SET_POWER_OFF	  0x10
#define SET_POWER_ON	  0x11
//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
#define SET_USB_I_F_OFF	  0x12
#define SET_USB_I_F_ON	  0x13
#define GET_MODEM_POWER_STATUS     0x14
#define GET_MODEM_SWITCH_STATUS    0x15
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status

static int pppd_pid=-1;


////////////////////////////////////////////////////////
static struct file_operations modem_file_operations = 
{
	.owner = THIS_MODULE,
	.ioctl   =  modem_td_ioctl
};


static int modem_td_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int status;

    switch(cmd)
    {
    case SET_PPPD_PID:
        pppd_pid = (int) arg;
        printk(">>>set pppd_pid=%d\n", pppd_pid);
        break;

    case GET_PPPD_PID:
        printk(">>>get pppd_pid=%d\n", pppd_pid);
        break;

    case SET_POWER_OFF:
        printk(">>> Modem power off\n");
        modem_hsdpa_pwr(0);
        break;

    case SET_POWER_ON:
        printk(">>> Modem power on\n");
        modem_hsdpa_pwr(1);
        break;
//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
    case SET_USB_I_F_OFF:
        printk(">>> modem USB-IF off\n");
        usb_interface_pwr(0);
        break;

    case SET_USB_I_F_ON:
        printk(">>> modem USB-IF on\n");
        usb_interface_pwr(1);
        break;

    case GET_MODEM_POWER_STATUS:
        copy_to_user((void __user *)arg, &modem_PW, sizeof(int));
        break;

    case GET_MODEM_SWITCH_STATUS:
        status = modem_switch_status();
        copy_to_user((void __user *)arg, &status, sizeof(int));
        break;	 	  
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status      	  
    default:
        printk("Error: cmd=%d \n",cmd);
        break;
    }

    return 0;
}

void modem_kill_pppd (void)
{
    if (pppd_pid > 0)
    {
        printk("TD: kill pppd, pid= %d\n", pppd_pid);
       
        sys_kill(pppd_pid, 1);
        pppd_pid = -1;
    }
}

void modem_hsdpa_pwr (int on)
{
    if (!on)
        modem_kill_pppd();

//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status     	
    printk("TD: HSDPA_3.3V%c\n", on ? '+' : '-');
#ifdef CONFIG_EX3_HARDWARE_DVT
    gpio_set_value(S3C64XX_GPM(1), on);
#else
    gpio_set_value(S3C64XX_GPK(15), on);
#endif
    modem_PW = on;
    //gpio_set_value(S3C64XX_GPL(12), on);
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status      
}
EXPORT_SYMBOL(modem_hsdpa_pwr);

//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
void usb_interface_pwr (int on)
{
    printk("TD: I/F %s\n", on ? "enable" : "disable");
    gpio_set_value(S3C64XX_GPL(12), on);
}
EXPORT_SYMBOL(usb_interface_pwr);

static int modem_switch_status()
{    
    int gpio_value;

    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(0));
    gpio_value = ! gpio_get_value(S3C64XX_GPN(12)); //&*&*&*_ZH,EX3: match ME/ID design   

    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));
   
    return gpio_value;
}
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status

#if 0
static irqreturn_t modem_switch_irq_handler(int irqno, void *dev_id)
{
    int gpio_value;

    printk(">>> modem_switch_irq_handler()\n");

    /* set GPN(12) as input to get modem switch status*/
    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(0));
    gpio_value = gpio_get_value(S3C64XX_GPN(12));

    /* set GPN(12) as EXT_INT(12) for modem switch */
    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));

    if (gpn12_value != gpio_value)
    {
        printk("  GPN12= %d\n", gpio_value);
        gpn12_value = gpio_value;
        // set TD_KEYOFF
        modem_hsdpa_pwr(gpn12_value);
    }
    
    printk("<<< modem_switch_irq_handler()\n");    
    return IRQ_HANDLED;
}
#endif


static int modem_gpio_cfg(void)
{
    printk("TD: configure modem I/F gpio\n");

    // W_DISABLEn (GPL 12)
    s3c_gpio_cfgpin(S3C64XX_GPL(12), S3C_GPIO_SFN(1)); 
    s3c_gpio_setpull(S3C64XX_GPL(12), S3C_GPIO_PULL_NONE);
   
    // TD_PWRON  (GPN 12)
    // gpio_set_value(S3C64XX_GPN(12), 0);
    s3c_gpio_setpull(S3C64XX_GPN(12), S3C_GPIO_PULL_NONE);

//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
	/* set GPN(12) as input to get modem switch status*/
/*    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(0));
    gpn12_value = gpio_get_value(S3C64XX_GPN(12));
    printk("  GPN12= %d\n", gpn12_value);*/
    
    /* set GPN(12) as EXT_INT(12) for modem switch */    
    s3c_gpio_cfgpin(S3C64XX_GPN(12), S3C_GPIO_SFN(2));
    set_irq_type(IRQ_EINT(12), IRQ_TYPE_EDGE_BOTH);    
#ifdef CONFIG_EX3_HARDWARE_DVT    
    // TD_KEYOFF (GPM 1)
    s3c_gpio_cfgpin(S3C64XX_GPM(1), S3C_GPIO_SFN(1)); 
    s3c_gpio_setpull(S3C64XX_GPM(1), S3C_GPIO_PULL_NONE);
#else
    // TD_KEYOFF (GPK 15)
    s3c_gpio_cfgpin(S3C64XX_GPK(15), S3C_GPIO_SFN(1)); 
    s3c_gpio_setpull(S3C64XX_GPK(15), S3C_GPIO_PULL_NONE);
#endif

	//modem_hsdpa_pwr(gpn12_value);
    
    if(modem_switch_status()){
        usb_interface_pwr(1);
        modem_hsdpa_pwr(1);	
    }else{
        usb_interface_pwr(0);
        modem_hsdpa_pwr(0);
    }    	
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
    return 0;
}


static int modem_td_probe(struct platform_device *pdev)
{
	printk(">>> modem_td_probe\n");

    modem_gpio_cfg();	
//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
/*	  
	request_irq(IRQ_EINT(12), modem_switch_irq_handler, 0, MODEM_TD_DRIVER, NULL);
*/
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status
    printk("<<< modem_td_probe\n");
	return 0;	
}

#ifdef CONFIG_PM
static int modem_td_suspend(void)
{
	printk(">>> modem_td_suspend\n");
	modem_kill_pppd();
//+&*&*&*YT_100224,EX3: disable I/F when system suspend
	printk("TD: disable I/F\n");
    gpio_set_value(S3C64XX_GPL(12), 0);
//-&*&*&*YT_100224,EX3: disable I/F when system suspend
    return 0;	
}

static int modem_td_resume(void)
{
    printk(">>> modem_td_resume\n");

#if 0
	/* workaround for modem resume issue */
	printk(" HSDPA_3.3V-\n");
#ifdef CONFIG_EX3_HARDWARE_DVT
    gpio_set_value(S3C64XX_GPM(1), 0);
#else
    gpio_set_value(S3C64XX_GPK(15), 0);
#endif
	
    msleep(100);
	
	printk(" HSDPA_3.3V+\n");
#ifdef CONFIG_EX3_HARDWARE_DVT
    gpio_set_value(S3C64XX_GPM(1), 1);
#else
    gpio_set_value(S3C64XX_GPK(15), 1);
#endif
#endif	

//+&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status	
/*	printk("TD: enable I/F\n");
      gpio_set_value(S3C64XX_GPL(12), 1);*/
//-&*&*&*YT_100129,EX3: control modem on/off, usb interface on/off and report modem switch status	      
    return 0;
}
#endif


static struct platform_driver modem_td_driver = {
	.probe		= modem_td_probe,
#ifdef CONFIG_PM
	.suspend	= modem_td_suspend,
	.resume		= modem_td_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MODEM_TD_DRIVER
	},
};


static int modem_init(void)
{
	int retval;
	struct platform_device *device;
	
	printk(">>> modem_init\n");
	
	retval = platform_driver_register(&modem_td_driver);
	if (retval < 0)
	  return retval;
	
      device = platform_device_register_simple(MODEM_TD_DRIVER, -1, NULL, 0);
	 if (IS_ERR(device))
	   return -1;
	
	retval = register_chrdev(MODEM_MAJOR, MODEM_TD_DRIVER, &modem_file_operations);
	if (retval < 0)
	{
		printk(KERN_WARNING "Can't get major %d\n", MODEM_MAJOR);
		return retval;
	}
	
	printk("<<< modem_init\n");	
	return 0;
}

static void modem_exit(void)
{
    printk("Modem_exit...");
    unregister_chrdev(MODEM_MAJOR, MODEM_TD_DRIVER);
}


module_init(modem_init);
module_exit(modem_exit);

