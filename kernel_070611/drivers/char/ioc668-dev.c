#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/time.h>
#include <plat/regs-rtc.h>

#include <linux/i2c.h> 
//#include <mach/ioc.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h> 
#include <linux/delay.h>
#include <linux/cdev.h>
#include <asm/system.h>

#include <linux/io.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <linux/reboot.h>

#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <plat/input_key.h>
#include <linux/ioc668-dev.h>

#include "Elan_Initial_6410vs668.h"

#define SLAVE_ADDR 0x33 //0x66 //0x33 //0x10 //0x20

#define IOC_NAME "ioc"

#define RSTATUS_READY  0x04 

/* Register Address of Real Time Clock */
#define RTIME_YEAR 			0x40
#define RTIME_MONTH 		0x41 
#define RTIME_DATE 			0x42
#define RTIME_DAY 			0x43
#define RTIME_HOUR 			0x44
#define RTIME_MIN 			0x45
#define RTIME_SEC				0x46

#define RMSG_EVENT      0x6A
#define RSYSTEM_STATE 	0xFD

#define RRESET					0x90
#define RMODE_CONTROL		0x91
#define RUPGRADE_FW		 	0x92 //RUPGRADE_MODE
#define RBOARDID_VERIFY 0x93


#define IOC_POWER_ON		0
#define IOC_POWER_OFF		1

#define __DEBUG_TMP__


#ifdef __DEBUG_TMP__
	#define printk(fmt,arg...) do{printk(fmt,##arg);}while(0)
#else
	#define printk(fmt,arg...) do{}while(0)
#endif

//#define CM_LVR_FUNC 11

unsigned long morse_key_value = 0;
unsigned long morse_key_flag = 0 ;
EXPORT_SYMBOL(morse_key_value);		//William add
EXPORT_SYMBOL(morse_key_flag);

unsigned char g_mode_7 = 0x55 ;
unsigned char g_mode_6 = 0x66 ;
unsigned char g_mode_9 = 0x99 ;
unsigned char g_mode_16 = 0x10 ;

extern struct input_dev *for_morse_key_input;
extern unsigned int Morse_interrupt_flag ;

enum ioc_type
{
	ioc=0,
};


struct ioc *gp_ioc ;

static int ioc_major = 245 ;
dev_t devno ;

struct ioc_dev_t
{
	struct cdev cdev; 
};
struct ioc_dev_t ioc_dev;

enum ioc_cmd
{
	
	CM_UPDATE_FW,
	IOCSIM_REVISION,
	IOCSIM_GETSERIAL,
	IOCSIM_ENCRYPT,
	IOCSIM_DECRYPT,
	IOCSIM_GETDRMKEY,
	IOCSIM_RESETDRMKEY,
	IOCSIM_AUTH,
	IOCSIM_CHANGEPIN,
	IOCSIM_CHKAUTH,
	IOCSIM_RESETUSERKEY,
	
	CM_LVR_FUNC,
	CM_UPDATE_EEPROM,
	CM_IOC_RESET,
	CMD_END
};

unsigned short fw_a_area[8453] = {0};
unsigned short fw_b_area[8453] = {0};
unsigned short B2_CODEOPTION[3] = {0} ;
unsigned short B3_CODEOPTION[3] = {0} ;

unsigned char version_buf[0] ;


extern void Elan_Initial_6410vs668(void) ;
extern void Elan_Define_Delay500ms(void) ;
extern void Elan_Clr_AllPin(void);

unsigned char get_checksum(char *wbuf, int num)
{
	int i ;
	char *buf = wbuf ;
	unsigned char checksum = 0 ;
	for(i = 1 ; i <= num ; i++)
	{
		checksum ^= buf[i] ;
	}
	checksum ^= 0xff ;
	return checksum ;
}



int get_epd_config()
{
	int h_type = 0 ;	
	int gpa3 = -1; //xyd:2011.04.21 SC8 EPD
	gpa3 = (((__raw_readl(S3C64XX_GPADAT) & 0x008) >> 3) & 0x01) ;
	printk("xuyuda:%s, gpa3=0x%x\n",__FUNCTION__, gpa3);	
	
	#if  defined(CONFIG_HW_EP1_DVT)  || defined(CONFIG_HW_EP2_DVT)	
    	int gpj765 = -1;
		gpj765 = (((__raw_readl(S3C64XX_GPJDAT) & 0x0e0) >> 5) & 0x07) ;
		if (gpa3 == 0x00){	
		   	if( gpj765 == 0x07 || gpj765 == 0x03 || gpj765 == 0x00 )
	    	{
	    		h_type = 4 ;//EPD SC4
	    		printk("The SC4 type^^^^^^^^^^^^^^^^^\n") ;
	    	}
		}else if(gpa3 == 0x01){
	    	if( gpj765 == 0x06 || gpj765 == 0x02 || gpj765 == 0x04 )
	    	{
	    		h_type = 8 ;//EPD SC8
	    		printk("The SC8 type^^^^^^^^^^^^^^^^^\n") ;
	    	}
		}
    #elif defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT)
		int gpo11_13 = -1; //xyd:2011.04.21 SC8 EPD
		gpo11_13 = (((__raw_readl(S3C64XX_GPODAT) & (0x7 << 11)) >> 11) & 0x07);
		printk("xuyuda:%s, gpo11_13=0x%x\n",__FUNCTION__, gpo11_13);
		if(gpa3 == 0x00){
		   	if( gpo11_13 == 0x00 || gpo11_13 == 0x04 || gpo11_13 == 0x07 )
	    	{
	    		h_type = 4 ;//EPD SC4
	    		printk("The SC4 type^^^^^^^^^^^^^^^^^\n") ;
	    	}
		}else if(gpa3 == 0x01){
	    	if( gpo11_13 == 0x02 || gpo11_13 == 0x06 || gpo11_13 == 0x01 )
	    	{
	    		h_type = 8 ;//EPD SC8
	    		printk("The SC8 type^^^^^^^^^^^^^^^^^\n") ;
	    	}
		}
	
    #endif
    	return h_type ;
   
}
EXPORT_SYMBOL(get_epd_config) ;


//epd_type = get_epd_config() ;

/****************************************************************************
 * 
 * FUNCTION NAME:  ioc_read_reg   
 *  
 *
 * DESCRIPTION:   read from ioc reg  
 *
 *
 * ARGUMENTS:        
 * 
 * ARGUMENT         TYPE            I/O     DESCRIPTION 
 * --------         ----            ----    ----------- 
 *  @p_ioc       struct ioc *                    client
 *  @reg          u8                                start reg address
 *  @var	        unsigned char *              buf address of value reading from reg             
 *  @bytes       int					    num of bytes of read value
 *
 * RETURNS: num of bytes write to ioc reg or else EIO when error
 * 
 ***************************************************************************/

int ioc_read_reg(struct ioc *p_ioc, u8 start_reg, unsigned char *val_buf, int bytes)
{
	int ret;
	int time_delay = 5 ;
	int k = 0 ;
	/*
	  * William modify : resolve IOC pull down i2c DATA pin issue.	
	*/
	struct i2c_adapter *adap = p_ioc->client->adapter;
	struct i2c_msg msg[2];
	msg[0].addr = p_ioc->client->addr;
	msg[0].flags = 0; //Write //p_ioc->client->flags  &I2C_M_TEN; 
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	msg[1].addr = p_ioc->client->addr;
	msg[1].flags = I2C_M_RD ; //p_ioc->client->flags  &I2C_M_TEN; //Write
	msg[1].len = bytes;
	msg[1].buf = val_buf;
	ret = i2c_transfer(adap, &msg, 2);
	
	return ret ;
}


EXPORT_SYMBOL(ioc_read_reg) ;

/****************************************************************************
 * 
 * FUNCTION NAME:  ioc_write_reg   
 *  
 *
 * DESCRIPTION:   write to ioc reg  
 *
 *
 * ARGUMENTS:        
 * 
 * ARGUMENT         TYPE            I/O     DESCRIPTION 
 * --------         ----            ----    ----------- 
 *  @p_ioc        struct ioc *              client
 *	@reg         u8                         start reg address
 *	@var				 unsigned char *            buf address of value writing to reg             
 *  @bytes			 int												num of bytes of writing value
 *
 * RETURNS: num of bytes write to ioc reg or else EIO when error
 * 
 ***************************************************************************/

int ioc_write_reg(struct ioc *p_ioc, u8 reg, unsigned char *val, int bytes)
{
	u8 buf[32] ;
	int ret ;
	
	buf[0] = reg;
	if(bytes > 31)
	{
		printk("write too many bytes one time.\n") ;//for ioc only
		return -1 ;
	}
	memcpy(&buf[1], val, bytes);
	ret = i2c_master_send(p_ioc->client, buf, bytes + 1);
	if (ret < 0)
		return ret;
	if (ret != bytes + 1)
		return -EIO;
	return ret;
}

EXPORT_SYMBOL(ioc_write_reg) ;



static void ioc_enter_normal_mode()
{
	unsigned int go_8,go_0 ;
	
	s3c_gpio_cfgpin(S3C64XX_GPB(5), S3C_GPIO_SFN(2));// recover i2c function
	s3c_gpio_cfgpin(S3C64XX_GPB(6), S3C_GPIO_SFN(2));
	
	s3c_gpio_cfgpin(S3C64XX_GPB(2), S3C_GPIO_SFN(6)); //recover i2c function
	s3c_gpio_cfgpin(S3C64XX_GPB(3), S3C_GPIO_SFN(6));//
	
	//set VDD and RST to let 668 enter into normal work mode
	s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));//OUTPUT
	s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
	
	__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) ;//GPO0=L, VDD=H
	__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) ;//GPO8=L, RST=H
	//mdelay(5) ;
	Elan_Define_Delay500ms() ;
	go_8 = ((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01) ;
//	go_8 = gpio_get_value(S3C64XX_GPO(8)) ;
	go_0 = ((__raw_readl(S3C64XX_GPODAT) >> 0) & 0x01) ;
//	printk("IOC_RST pin is %d , IOC_PWRER is %d \n", go_8, go_0) ;
}


#define MOR_BTN_DEBOUNCE_INTERVAL	500
int detect_morse(struct ioc * ptmp_ioc) 
{
	int value ;
	unsigned char r_buf[1] ;
	unsigned char r_6B_buf[1] ;
//	unsigned char up_down = 1 ;
	unsigned short morsecode = 0 ;
	unsigned long irq_flag = 0 ;
	
//	local_irq_save(irq_flag) ;
	printk("%s: 0\n", __FUNCTION__);
	mutex_lock(&ptmp_ioc->lock) ;
	printk("%s: 1\n", __FUNCTION__);
	value = ioc_read_reg(ptmp_ioc, 0x6A, r_buf, 1) ;

	msleep(2) ;
	if(value < 0)
	{
		mutex_unlock(&ptmp_ioc->lock) ;
		return ;
	}
	printk("1: read from 6A : r_buf[0] = 0x%x \n", r_buf[0]) ;
	if(r_buf[0] == 0x20)
	{
		value = ioc_write_reg(ptmp_ioc, 0x6A, r_buf, 1) ;
	//	ioc_write_reg(ptmp_ioc->client->addr, 0x6A, r_buf, 1) ;
		msleep(10) ;
		printk("11: write to 6A : r_buf[0] = 0x%x \n", r_buf[0]) ;
		if(value < 0)
		{
			mutex_unlock(&ptmp_ioc->lock) ;
			return ;
		}
	}
	else
	{
		printk("3: read from 6A: r_buf[0] = 0x%x , not button ignore!!\n", r_buf[0]) ;
//		r_buf[0] = 0 ;
//		r_6B_buf[0] = 0 ;
		mutex_unlock(&ptmp_ioc->lock) ;
		return 0;
	}
	
	value = ioc_read_reg(ptmp_ioc, 0x6B, r_6B_buf, 1) ;
	msleep(2) ;
	if(value < 0)
	{
			mutex_unlock(&ptmp_ioc->lock) ;
			return ;
	}
	//value = ioc_read_reg(ptmp_ioc->client->addr, 0x6B, r_6B_buf, 1) ;
	
	printk("2: read from 6B : r_6B_buf[0] = 0x%x \n", r_6B_buf[0]) ;
	
	morse_key_value = 0x40000;
	if(0x1C == r_6B_buf[0])
	{
		morse_key_value |= (1 << 12) ; //morse page down
//		morse_key_value |= GPIO_PAGE_DOWN ; //morse page down
		morse_key_flag = 1 ;
		
		printk("Page Down, Morse button short press 1 time. \n") ;
		
	}
	if(0x1D == r_6B_buf[0])
	{
		morse_key_value |= (1 << 5) ; //morse ok
		//morse_key_value = GPIO_5_ENTER ; //morse ok
		morse_key_flag = 1 ;
		printk("OK Middle key, Morse button short press 2 times. \n") ;
	}
	if(0x1E == r_6B_buf[0])
	{
		morse_key_value |= (1 << 11) ; //morse page up
		//morse_key_value = GPIO_PAGE_UP ; //morse page up
		//morse_key_value = 0x40800 ;
		morse_key_flag = 1 ;
		printk("Page Up ,Morse button long press within 3 second. \n") ;
	}
	if(0x1F == r_6B_buf[0])
	{
		#if defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_DVT)
			morse_key_value |= (1 << 3) ; //ep12 morse home
			morsecode = GPIO_HOME ;
		#endif
		
		#if defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT)
	//	morse_key_value = GPIO_HOME ; //morse home
		//morse_key_value = 0x40008 ;
			morse_key_value |= (1 << 16) ; //ep34 morse home: as for layout reverse home and menu
			morsecode = GPIO_MENU ;
		#endif
		morse_key_flag = 1 ;
		
		printk("Home Key, Morse button long press over 3 second. \n") ;
	}
	
	r_buf[0] = 0 ;
	r_6B_buf[0] = 0 ;
//	printk("ioc668 morse, input_event keycode: 0x%x,\n", morsecode) ;
//	enable_irq(ptmp_ioc->client->irq);
//	mdelay(100);

//	s3c_gpio_cfgpin(S3C64XX_GPB(5), S3C_GPIO_SFN(2));// recover i2c function
//	s3c_gpio_cfgpin(S3C64XX_GPB(6), S3C_GPIO_SFN(2));
	
//	__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffdf , S3C64XX_GPBDAT); //GPB5=L
//	__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffbf , S3C64XX_GPBDAT); //GPB6=L
	mutex_unlock(&ptmp_ioc->lock) ;
//	local_irq_restore(irq_flag) ;
	

	return 0 ;
}

EXPORT_SYMBOL(detect_morse) ;

void ioc_work(struct work_struct *work)
{
	struct ioc *ptmp_ioc = container_of(work, struct ioc, irq_work);
	
	printk("---~~~~---%s-------------\n", __FUNCTION__) ;
	
//	s3c_gpio_cfgpin(S3C64XX_GPB(5), S3C_GPIO_SFN(2));// recover i2c function
//	s3c_gpio_cfgpin(S3C64XX_GPB(6), S3C_GPIO_SFN(2));
//	
//	s3c_gpio_cfgpin(S3C64XX_GPB(2), S3C_GPIO_SFN(6)); //recover i2c function
//	s3c_gpio_cfgpin(S3C64XX_GPB(3), S3C_GPIO_SFN(6));//

//mutex_lock(&ptmp_ioc->lock) ;
	detect_morse(ptmp_ioc) ;
//mutex_unlock(&ptmp_ioc->lock) ;	
//	input_event(for_morse_key_input , EV_KEY , morse_key_value, 0);
//	input_sync(for_morse_key_input);
//	morse_key_value = 0 ;
	
//	input_event(for_morse_key_input , EV_KEY , GPIO_SIZE_Up, 0/1);
//enable_irq(ptmp_ioc->client->irq);
}


void get_morse_key_status(unsigned long * pvalue, unsigned long * pflag)
{
	*pvalue = morse_key_value ;
	*pflag  = morse_key_flag ;
}
EXPORT_SYMBOL(get_morse_key_status) ;

void ioc_schedule(struct ioc *pioc)
{
	schedule_work(&pioc->irq_work);
}
static irqreturn_t ioc_irq(int irq, void *dev_id) 
{
	struct ioc *ptmp_ioc = (struct ioc *)dev_id;
	printk("------%s-------------\n", __FUNCTION__) ;
	//verify_morse_button(ptmp_ioc) ;
	int value;
	value = gpio_get_value(S3C64XX_GPN(5));
	printk("GPN5 is %d \n", value);
	ioc_schedule(ptmp_ioc);
	//schedule_work(&ptmp_ioc->irq_work);//jack noted on 2010/12/17
//	schedule_delayed_work(&ptmp_ioc->irq_work);
  
//	disable_irq_nosync(irq);
//	schedule_work(&ptmp_ioc->irq_work);

	return IRQ_HANDLED;
}

int  do_ioc_scan(void)
{	
	int value;
	 value = gpio_get_value(S3C64XX_GPN(5));
	if(!value)
	{	
		ioc_schedule(gp_ioc);
	}
	return value;
}

EXPORT_SYMBLE(do_ioc_scan);

static int ioc_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int result, err ;
	struct ioc *ptmp_ioc;
	int status;
//	unsigned short tmp;
	unsigned char w_buf[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07} ;
	unsigned char r_buf[7] ;
	
	printk("21111111111111111111111111111111111111111111111111111111 %s \n", __FUNCTION__);
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA)) {
		printk("adapter doesnt support SMBUS\n");
		return -ENODEV;
	}
	
	s3c_gpio_cfgpin(S3C64XX_GPN(5), S3C_GPIO_SFN(2)); 
	s3c_gpio_setpull(S3C64XX_GPN(5), S3C_GPIO_PULL_UP);
	//set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_FALLING);
	
	ptmp_ioc = kzalloc(sizeof(struct ioc), GFP_KERNEL);
	if (!ptmp_ioc ) {
		printk("kzalloc failed\n");
		return -ENOMEM;
	}
	gp_ioc = ptmp_ioc;
	
	i2c_set_clientdata(client, ptmp_ioc);
	
//	INIT_WORK(&ac->work, adxl345_work);
//	mutex_init(&ac->mutex);
	
	
	printk("1:client-irq is %d \n", client->irq);
	
	//在這里執行 ioc_read_multi_reg 會出現內核空指針的問題,也就是說必須先賦值 client
	
	gp_ioc->client = client;

	INIT_WORK(&gp_ioc->irq_work, ioc_work);
//	INIT_DELAYED_WORK(&gp_ioc->delay_work, ioc_work);//struct delayed_work	delay_work
	
	mutex_init(&gp_ioc->lock);
	
	printk("client->addr is 0x%2x \n", client->addr) ;
	
	printk("***********ioc irq is %d ****************\n", client->irq);
	printk("client->dev.driver->name is %s \n", client->dev.driver->name) ;
	err = request_irq(client->irq, ioc_irq, IRQ_TYPE_EDGE_FALLING, client->dev.driver->name, gp_ioc);//IRQ_TYPE_EDGE_RISING
	if (err) 
	{
		dev_err(&client->dev, "ioc irq %d busy?\n", client->irq);
	}	
	result = ioc_read_reg(ptmp_ioc, 0x02, version_buf, 1) ;
	printk("read version reg: result = 0x%x \n", result) ;
	printk("IOC FW version is 0x%x \n", version_buf[0]) ;
	
	
	
	return 0;
}

static int ioc_remove(struct i2c_client *client)
{
	struct ioc *gioc = i2c_get_clientdata(client);//hwmon_dev->driver_data
	
	//hwmon_device_unregister(gioc->hwmon_dev);
	free_irq(gioc->client->irq, gioc);
	i2c_set_clientdata(client, NULL);
	kfree(gioc);
	gp_ioc = NULL;

	return 0;
}

static int iocf668_suspend(struct i2c_client *client,const struct i2c_device_id *id)
{
	//set GPP0 and GPP7 as input, pull down/up disable, Jack added on 2010/11/6 
	printk("%s \n", __FUNCTION__);
	s3c_gpio_cfgpin(S3C64XX_GPP(7), S3C_GPIO_SFN(0)); 
	s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(S3C64XX_GPP(0), S3C_GPIO_SFN(0)); 
	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_NONE);
  
  return 0;
}

static int iocf668_resume(struct i2c_client *client,const struct i2c_device_id *id)
{		printk("%s \n", __FUNCTION__);
        return 0;
}

static const struct i2c_device_id ioc_ids[] = {
	{ "ioc", 0 },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, ioc_ids);

/* This is the driver that will be inserted */
static struct i2c_driver ioc_driver = {
//	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "ioc",
		.owner = THIS_MODULE,
	},
	.probe		= ioc_probe,
	.remove		= ioc_remove,
	.suspend= iocf668_suspend,
  .resume = iocf668_resume,
	.id_table	= ioc_ids,
//	.detect		= ioc_detect,
//	.address_data	= &addr_data,
};

int ioc_open(struct inode *inode, struct file *filep)
{
  //  filep->private_data = gp_ioc->client ;

  filep->private_data = & ioc_dev.cdev ;
    return 0;
}

int ioc_release(struct inode *inode, struct file *filep)
{
	return 0 ;
}
void ioc_poweron(void)  //William add 
{	
	int h_cfg ;
	printk("===>%s\n", __FUNCTION__);
	if( (h_cfg = get_epd_config()) == 8 )
	{
		s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
		 gpio_set_value(S3C64XX_GPO(8), IOC_POWER_ON);
		 gpio_get_value(S3C64XX_GPO(8));
		 printk("S3C64XX_GPO(8) is %d\n", gpio_get_value(S3C64XX_GPO(8)));
		 mdelay(10);
	}
	else
	{	
	 s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
	 gpio_set_value(S3C64XX_GPO(0), IOC_POWER_ON);
	 gpio_get_value(S3C64XX_GPO(0));
	 printk("S3C64XX_GPO(0) is %d\n", gpio_get_value(S3C64XX_GPO(0)));
	 mdelay(10);
	}
	
}
EXPORT_SYMBOL(ioc_poweron);


void ioc_poweroff(void)
{
	int h_cfg ;
	printk("===>%s\n", __FUNCTION__);
	
	if( (h_cfg = get_epd_config()) == 8 )
	{
		s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
		gpio_set_value(S3C64XX_GPO(8), IOC_POWER_OFF);	
		mdelay(1);
	}
	
	else 
	{	
		s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
		gpio_set_value(S3C64XX_GPO(0), IOC_POWER_OFF);	
		mdelay(1);
	}
	
}
	

int ioc_reset(void)    
{// 0: ok  1: fail
//	unsigned long irq_flag = 0 ;
	unsigned int gpo_0,gpo_8 ;
	unsigned int flag = 0 ;
	int h_cfg ;
	printk("===>%s\n", __FUNCTION__);
	
	if( (h_cfg = get_epd_config()) == 8 )
	{
		s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
		__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) ;//GPO8=L, VDD=H
		mdelay(10) ;
		gpo_8 = ((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01) ;
		printk("2: gpo_8 = 0x%x \n", gpo_8) ;	
		if(gpo_8 != 0)
			flag = 1 ;
			
		s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
		__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000001 , S3C64XX_GPODAT) ; //GPO0=H, RST=L
		gpo_0 = ((__raw_readl(S3C64XX_GPODAT)) & 0x01) ;
		msleep(10) ;
		
		printk("gpo_0 = 0x%x \n", gpo_0) ;		
		if(gpo_0 != 1)
			flag = 1 ;
			
		__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) ;//GPO0=L, RST=H
		mdelay(10) ;
		gpo_0 = ((__raw_readl(S3C64XX_GPODAT)) & 0x01) ;
		printk("gpo_0 = 0x%x \n", gpo_0) ;		
		if(gpo_0 != 0)
			flag = 1 ;
	}
	else
	{
		s3c_gpio_cfgpin(S3C64XX_GPO(0), S3C_GPIO_SFN(1));
		__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffffe , S3C64XX_GPODAT) ;//GPO0=L, VDD=H
		mdelay(10) ;
		gpo_0 = ((__raw_readl(S3C64XX_GPODAT)) & 0x01) ;
		printk("gpo_0 = 0x%x \n", gpo_0) ;		
		if(gpo_0 != 0)
			flag = 1 ;
			
		s3c_gpio_cfgpin(S3C64XX_GPO(8), S3C_GPIO_SFN(1));
		__raw_writel(__raw_readl(S3C64XX_GPODAT) | 0x00000100 , S3C64XX_GPODAT) ; //GPO8=H, RST=L
		gpo_8 = ((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01) ;
		msleep(10) ;
		
		printk("1: gpo_8 = 0x%x \n", gpo_8) ;	
		if(gpo_8 != 1)
			flag = 1 ;
			
		__raw_writel(__raw_readl(S3C64XX_GPODAT) & 0xfffffeff , S3C64XX_GPODAT) ;//GPO8=L, RST=H
		gpo_8 = ((__raw_readl(S3C64XX_GPODAT) >> 8) & 0x01) ;
		printk("2: gpo_8 = 0x%x \n", gpo_8) ;	
		if(gpo_8 != 0)
			flag = 1 ;
	}
	
	
	printk("flag = %d \n ", flag) ;
	return flag ;
}

EXPORT_SYMBOL(ioc_reset);

static int ioc_ioctl(struct inode *inode, struct file *filp, \
					unsigned int cmd, unsigned long arg)
{
	
//	printk("%s \n", __FUNCTION__) ;
	unsigned long irq_flag = 0 ;
	unsigned int gb5,gb6 ;
	unsigned short res = 0 ;
	unsigned char device_id[1] ;
	unsigned long reset_result[1] ;
	unsigned long lvr_result[1]  ;
	
	unsigned int i ;
	
	switch(cmd)
	{
		case IOCSIM_REVISION:
//			printk("VVVVVVVVVVVVVVVVVVV IOCSIM_REVISION YYYYYYYYYYYYYYYYYYY\n") ;
//			if( copy_from_user((unsigned long *)revision_read_buf, (unsigned long *)arg, sizeof(revision_read_buf) ) != 0 )
//					return -EFAULT;
//			revision_write_buf[0] = 0x01 ;// ;
//			len = ioc_write_reg(gp_ioc->client->addr, 0x2E , revision_write_buf, 1) ;
//			printk("Write 0x2E to get SN , len = %d \n", len) ;
//			mdelay(500) ;
//			len = ioc_read_reg(gp_ioc->client->addr, 0x2E ,revision_read_buf , 5) ;
//			printk("Read 0x2E to get SN , len = %d \n", len) ;
//			for(len = 0 ; len < 5 ; len++)
//			{
//				printk("revision_read_buf[%d] = 0x%x ", len, revision_read_buf[len]) ;
//			}
//			if( copy_to_user( (unsigned long *)arg, (unsigned long *)revision_read_buf, sizeof(revision_read_buf) ) != 0 )
//	  			return -EFAULT;
			break ;
		case IOCSIM_GETSERIAL:
			break ;
		case CM_LVR_FUNC: //CM_LVR_FUNC  CM_BAT_CAP
			s3c_gpio_cfgpin(S3C64XX_GPB(5), S3C_GPIO_SFN(1)); //GPB5 as output
			s3c_gpio_cfgpin(S3C64XX_GPB(6), S3C_GPIO_SFN(1));//GPB6 as output
//			mdelay(100) ;
			__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffdf , S3C64XX_GPBDAT); //GPB5=L
			__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xffffffbf , S3C64XX_GPBDAT); //GPB6=L
		//	mdelay(100) ;
//			printk("set GPB5 GPB6 as output , and set LOW, and mdelay 100 .\n") ;
			gb5 = ((__raw_readl(S3C64XX_GPBDAT) >> 5) & 0x01) ;
			gb6 = ((__raw_readl(S3C64XX_GPBDAT) >> 6) & 0x01) ;
		//	mdelay(100) ;
	//		printk("gb5 = 0x%x, gb6 = 0x%x \n", gb5,gb6) ;
			
	//for BAT_I2C_CLK		
			s3c_gpio_cfgpin(S3C64XX_GPB(2), S3C_GPIO_SFN(1)); //GPB2 as output
			s3c_gpio_cfgpin(S3C64XX_GPB(3), S3C_GPIO_SFN(1));//GPB3 as output
//			mdelay(100) ;
			__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xfffffffb , S3C64XX_GPBDAT); //GPB2=L
			__raw_writel(__raw_readl(S3C64XX_GPBDAT) & 0xfffffff7 , S3C64XX_GPBDAT); //GPB3=L
			
			
			local_irq_save(irq_flag) ;
			
//			printk("Elan_Initial_6410vs668() ;") ;
			Elan_Initial_6410vs668() ;
			
	//		printk("CM_LVR_FUNC: version_buf[0] = 0x%x \n", version_buf[0]) ;
			if(version_buf[0] <= 0x14)
			{
			//	g_mode_9 = Elan_Update668FW_func(0x09) ;
				g_mode_6 = Elan_Update668FW_func(0x06) ;
				
	//			printk("after mode 0x06,fw_b_area[8448] & 0x180 == 0x%x\n", fw_b_area[8448] & 0x180) ;

				for(i = 8448 ; i < 8451 ; i++)
				{//打印設置 lvr 后的codeoption值
					B3_CODEOPTION[i-8448] = fw_b_area[i] ;
			//		printk("B3_CODEOPTION[%d] = 0x%x\n", i-8448, B3_CODEOPTION[i-8448]) ;
				}
				
		//		printk("Elan_Update668FW_func(0x06) result : g_mode_6 = 0x%x\n", g_mode_6) ;

				if( (B3_CODEOPTION[0] & 0x180) != 0x80)
				{//if lvr 沒有設置		
					g_mode_16 = Elan_Update668FW_func(0x10) ;
					if(g_mode_16 == 0x80)
					{
			//			printk("after mode16, codeoption word0 :fw_b_area[8448] = 0x%x\n", fw_b_area[8448]) ;
						
						for(i = 8448 ; i < 8451 ; i++)
						{
			//				printk("fw_b_area[%d] = 0x%x \n", i,fw_b_area[i]) ;
						}
						
						if (B2_CODEOPTION[1] != 0x2058)
						{//must be 0R0 或者 mos 狀態下沒有成功進入燒錄模式（誤判就誤判：Allen）
//							printk("^^^^B2_CODEOPTION[0] = 0x%x \n", B2_CODEOPTION[0]) ;
//							printk("^^^^B2_CODEOPTION[1] = 0x%x \n", B2_CODEOPTION[1]) ;
//							printk("^^^^B2_CODEOPTION[2] = 0x%x \n", B2_CODEOPTION[2]) ;
//							printk("kernel: no need to set lvr!\n") ;//no need to set lvr ( reset:(2) )
							lvr_result[0] = 1 ;//no need to set lvr ( reset:(2) )
				//			printk("2:lvr_result = %ld \n", lvr_result[0]) ;
							if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
				  			return -EFAULT;
				//  		printk("error or pass\n") ;
						}
						else if( (fw_b_area[8448] == (B3_CODEOPTION[0] | 0x80)) && ((fw_b_area[8448] & 0x180) == 0x80) &&  (fw_b_area[8449] == B3_CODEOPTION[1]) && (fw_b_area[8450] == B3_CODEOPTION[2]))//
						{//對于所有的沒有誤判的 CMOS 而言
					//		printk("kernel: set lvr success !\n") ;
							lvr_result[0] = 0 ;
				//			printk("lvr_result = %ld \n", lvr_result[0]) ;
							if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
				  			return -EFAULT;
						}
						else
						{
						
					//		printk("kernel: set lvr fail !\n") ;
							lvr_result[0] = 3 ;
					//		printk("2:lvr_result = %ld \n", lvr_result[0]) ;
							if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
				  			return -EFAULT;
						}
					}
					else
					{
			//			printk("kernel: set lvr fail !\n") ;
						lvr_result[0] = 3 ;
			//			printk("2:lvr_result = %ld \n", lvr_result[0]) ;
						if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
			  			return -EFAULT;
					//	break ;
					}
				}
				else
				{
	//				printk("2:kernel: set lvr success !\n") ;
					lvr_result[0] = 0 ;
			//		printk("lvr_result = %ld \n", lvr_result[0]) ;
					if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
		  			return -EFAULT;
				}
				
			}
			
//				g_mode_6 = Elan_Update668FW_func(0x06) ;
//
//				for(i = 8448 ; i < 8451 ; i++)
//				{
//					printk("fw_b_area[%d] = 0x%x\n", i, fw_b_area[i]) ;
//				}
				
//			if(g_mode_7 == 0x80)//if 是14版，并且 g_mode_7 已經燒過了，
//			{
//				printk("g_mode_7 = 0x%x, LVR has been started.\n", g_mode_7) ;
//				lvr_result[0] = 0 ;
//				printk("lvr_result = %ld \n", lvr_result[0]) ;
//				if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
//	  			return -EFAULT;
//			//	break ;
//			}
//			else
//			{
//				lvr_result[0] = 5 ;
//				printk("2:lvr_result = %ld \n", lvr_result[0]) ;
//				if( copy_to_user( (unsigned long *)arg, (unsigned long *)lvr_result, sizeof(lvr_result) ) != 0 )
//	  			return -EFAULT;
//			//	break ;
//			}
			
			
	//		printk("finish lvr set .\n") ;
			
		//	Elan_Clr_AllPin();
		//	printk("finish Elan_Clr_AllPin()\n") ;
		//	Elan_Define_Delay500ms();
		//	printk("finish Elan_Define_Delay500ms()\n") ;
			
			//set GPP0 and GPP7 as input, pull down/up disable, Jack added on 2010/11/6 
			s3c_gpio_cfgpin(S3C64XX_GPP(7), S3C_GPIO_SFN(0)); 
			s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S3C64XX_GPP(0), S3C_GPIO_SFN(0)); 
			s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_NONE);
	//  	printk("ready to enter normal mode.\n") ;
			ioc_enter_normal_mode() ;
			
			local_irq_restore(irq_flag) ;
			
			break ;
		case CM_UPDATE_EEPROM:
			printk("VVVVVVVVVVVVVVVVVVV CM_UPDATE_EEPROM YYYYYYYYYYYYYYYYYYY\n") ;
			
			
			if( copy_from_user((unsigned long *)fw_a_area, (unsigned long *)arg, sizeof(fw_a_area) ) != 0 )
					return -EFAULT;
			break ;
		case CM_IOC_RESET:
			
			reset_result[0] = ioc_reset() ;
			printk("kernel: reset_result = %ld \n", reset_result[0]) ;
			if( copy_to_user( (unsigned long *)arg, (unsigned long *)reset_result, sizeof(reset_result) ) != 0 )
	  			return -EFAULT;
			break ;
		default:
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations ioc_fops = {
	.owner 		= THIS_MODULE,
	.open		  = ioc_open,
	.release	= ioc_release,
	.ioctl		= ioc_ioctl,
};

static int __init ioc_init(void)
{
	int h_cfg ;
	int res;
	
	printk("%s\n", __FUNCTION__);	
	printk("~~~~~~~~~~~~~~ Jack: EP ioc init ~~~~~~~~~~~~~~~~~\n");
	
	res = i2c_add_driver(&ioc_driver);//Here excute ioc_probe
	if(res)
	   goto out ;
	
	if (ioc_major)
	{
		printk("ioc_major = %d \n", ioc_major) ;
		printk("~~~~~~~~~~~~~~ Jack: EP ioc init ~~~~~~~~~~~~~~~~~\n");
		devno = MKDEV(ioc_major, 0) ;
		res = register_chrdev_region(devno, 1, IOC_NAME);
	}
	else
	{
		res = alloc_chrdev_region(&devno, 0, 1, IOC_NAME);
		ioc_major = MAJOR(devno);
	}
	if (res)
	{
		printk("register chrdev region fail.\n") ;
		goto out_unreg_chrdev_region ;
	}

	cdev_init(&ioc_dev.cdev, &ioc_fops);
	
	res = cdev_add(&ioc_dev.cdev, devno, 1);
	if (res)
	{
		printk("cdev_add fail.\n") ;
		//goto out ;
	}
	
//set GPP0 and GPP7 as input, pull down/up disable, Jack added on 2010/11/6 
	s3c_gpio_cfgpin(S3C64XX_GPP(7), S3C_GPIO_SFN(0)); 
	s3c_gpio_setpull(S3C64XX_GPP(7), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(S3C64XX_GPP(0), S3C_GPIO_SFN(0)); 
	s3c_gpio_setpull(S3C64XX_GPP(0), S3C_GPIO_PULL_NONE);
	if( (h_cfg = get_epd_config()) == 8 )
	{
		mdelay(1) ;
	}
	else
	{
		ioc_reset() ;
		mdelay(5) ;
	}

	ioc_enter_normal_mode() ;
	return 0 ;
	
out_unreg_chrdev_region: unregister_chrdev_region(devno, 1) ;
out: printk(KERN_ERR "%s: ioc-i2c Driver Initialisation failed \n", __FILE__);
     
  return res;
	
}

static void __exit ioc_exit(void)
{
	printk("%s\n", __FUNCTION__);
	
	unregister_chrdev_region(devno, 1) ;
	i2c_del_driver(&ioc_driver);
	cdev_del(&ioc_dev.cdev);
}

module_init(ioc_init);
module_exit(ioc_exit);

MODULE_AUTHOR("Jack Liu<TMSBG-BSP03/CEN/FOXCONN>");
MODULE_DESCRIPTION("ioc-i2c Driver");
MODULE_LICENSE("GPL");
