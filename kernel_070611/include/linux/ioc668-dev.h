#ifndef __IOC668_DEV_H__
#define __IOC668_DEV_H__

//extern unsigned short fw_a_area[8453] ;
//extern unsigned short fw_b_area[8453] ;


struct ioc {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *hwmon_dev;
	struct mutex lock;
	struct work_struct irq_work;
//	struct delayed_work	delay_work ;
	unsigned short deviceID ;
	unsigned char ready_status ;
};

extern struct ioc *gp_ioc ;
extern int detect_morse(struct ioc *);
extern int get_epd_config() ;

extern unsigned short fw_a_area[8453] ;
extern unsigned short fw_b_area[8453] ;
extern void ioc_poweron(void);	//William add 
extern int ioc_reset(void);
extern int do_ioc_scan(void);
#endif