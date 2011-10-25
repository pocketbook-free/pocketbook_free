/*
 *Autor :andy cheng 
 *mail  :chengrong070@sina.com
 * Digital Accelerometer characteristics are highly application specific
 * and may vary between boards and models. The platform_data for the
 * device's "struct device" holds this information.
 */

#ifndef _BMA020_H_
#define _BMA020_H_

//BMA device register and bit slice define 
#define CHIP_ID             0x00                 //chip id
    #define BMA_ID          0x2
#define CHIP_VERSION        0x01                 //chip version
#define DATAXL              0x02                 //
#define DATAXM              0x03
#define DATAYL              0x04
#define DATAYM              0x05
#define DATAZL              0x06
#define DATAZM              0x07
//0x08 unused now
#define STATUS_REG          0x09
    #define ST_RESULT       1<<7
	#define ALERT_PHASE     1<<4
	#define LG_LATCHED      1<<3
	#define HG_LATCHED      1<<2
	#define STATUS_LG       1<<1
	#define STATUS_HG       1<<0
	
#define STATUS_CTL          0x0A
    #define RESET_INT       1<<6
	#define SELF_TEST_1     1<<3
	#define SELF_TEST_0     1<<2
	#define SOFT_RESET      1<<1
	#define SLEEP           0<<0
 
#define INTERRUPT_SET       0x0B
    #define ALERT           1<<7
	#define ANY_MOTION      1<<6
	
	#define COUNT_HG_RST    0x0<<4
	#define COUNT_HG_1      0x1<<4
	#define COUNT_HG_2      0x2<<4
	#define COUNT_HG_3      0x3<<4
	
	#define COUNT_LG_RST    0x0<<2
	#define COUNT_LG_1      0x1<<2
	#define COUNT_LG_2      0x2<<2
	#define COUNT_LG_3      0x3<<2
	#define ENABLE_HG       1<<1
	#define ENABLE_LG       1<<0
	

#define LG_THRES            0x0C
#define LG_DUR              0x0D
#define HG_THRES            0x0E
#define HG_DUR              0x0F
#define ANY_MOTION_THRES    0x10
#define DUR_HYST            0x11
    #define ANY_MOTION_DUR  0x3<<6
	#define HG_HYST         0x7<<3
	#define LG_HYSY         0x7<<0
#define CUSTOM_REG1         0x12         //used by customer
#define CUSTOM_REG2         0x13         //used by custome

#define RANGE_CFG           0x14
    #define ACCE_RANGE2     0x0<<3
    #define ACCE_RANGE4	    0x1<<3
	#define ACCE_RANGE8     0x2<<3
	#define BANDWIDTH25     0x0<<0
	#define BANDWIDTH50     0x1<<0
	#define BANDWIDTH100    0x2<<0
	#define BANDWIDTH190    0x3<<0
	#define BANDWIDTH375    0x4<<0
	#define BANDWIDTH750    0x5<<0
	#define BANDWIDTH1500   0x6<<0
	
#define OPER_REG            0x15
    #define SPI4            1<<7
	#define SPI3            0<<7
	#define ENABLE_ADV_INT  1<<6
	#define NEW_DATA_INT    1<<5
	#define LATCH_INT       1<<4
	#define SHADOW_DIS      1<<3
	#define WAKE_UP_PAUSE20 0x0<<1
    #define WAKE_UP_PAUSE80 0x1<<1
    #define WAKE_UP_PAUSE320 0x2<<1
    #define WAKE_UP_PAUSE2560 0x3<<1    
	#define WAKE_UP           1<<0
struct bma020_platform_data {
       /***0x15h
		 *bit7 spi select bit
		 *bit6 advantage int enable
		 *bit5 new data interrupt enable
		 *bit4 when new data latch the interrupt
		 *bit3 shadow the MSB
		 *bit2-bit1 define the wake up time
		 *bit0 enable automatic change the mode 
		 *default 0x61
         */  
		 unsigned char oper_reg;
	   /***0x14h
		 *range_cfg define bit4,bit3 define the range ,
		 *bit2-bit0 define the bandwidth
         */        
		unsigned char range_cfg;
		  /***
		 *range_cfg define bit4,bit3 define the range ,
		 *bit2-bit0 define the bandwidth
         */
		        
				  

	
};
#endif



struct bma020;

struct bma020_triple {
	short int x;
	short int y;
	short int z;
};



struct bma020 {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct mutex mutex;	 /* reentrant protection for struct */
	struct bma020_platform_data pdata;
	struct axis_triple swcal;
	struct axis_triple hwcal;
	struct axis_triple saved;
	char   protect_bit;
	char   chip_id;
	char   version_id;
	char phys[32];
	int (*config_gpio)(void);
	void (*reset)(struct bma020 *bm);
	int (*init)(struct i2c_client *client, struct bma020 *bma);
	int (*read) (struct i2c_client *client, unsigned char reg, unsigned char * buf,unsigned char len);
	int (*write) (struct i2c_client *client, unsigned char reg, unsigned char *val,unsigned char len);
};

