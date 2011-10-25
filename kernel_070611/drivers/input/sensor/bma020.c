/*
 * Author: Andy cheng
 * mail: chengrong070@sina.com
 * Date: 2010/04/09
 * File: drivers/gsensordriver/bma020.c
 *
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/slab.h> //mem malloc
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/major.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <asm/ubmacess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/irqs.h>


#include <mbmah/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mbmah/irq.h>
#include <plat/gpio-cfg.h>

#include "bma020.h"

#if 1
	#define BMA020_DEBUG
#else
	#ifdef BMA020_DEBUG
	    #undef BMA020_DEBUG
	#endif
#endif

#ifdef BMA020_DEBUG
	#define bprintk(x, args...)	do { printk("BMA020 sensor" x, ##args); } while(0)
#else
	#define bprintk(x, args...)	do { } while(0)
#endif

#define REG_MAX 22
#define RANGE 512

//BM_RD read the reg value
#define BM_RD(bm,reg,buf,len) ((bm)->read( (bm)->client,reg,buf,len))
//BM_WT write value to the reg. 
#define BM_WT(bm,reg,buf,len) ((bm)->write( (bm)->client,reg,buf,len))




static inline char bma020_i2c_write(struct i2c_client *client, unsigned char reg_addr, unsigned char *data, unsigned char len);
static inline char bma020_i2c_read(struct i2c_client *client, unsigned char reg_addr, unsigned char *data, unsigned char len);
static void bma020_get_axis_triple(struct bma020 *bma, struct bma020_triple *axis);

static const struct bma020_platform_data bma020_default_init = {

			.range_cfg=BANDWIDTH1500|ACCE_RANGE8;          //bandwidth 1500 range -8/8g
			.oper_reg=SPI4|ENABLE_ADV_INT|NEW_DATA_INT|LATCH_INT|~SHADOW_DIS|WAKE_UP_PAUSE20|WAKE_UP;
			
 
};
/***
 *  
 *when bma020 is accessed in write mode sequences of 2bytes 
 *(=1 control byte to define which address will be written 
 *and 1 data byte) must be sent
 *  start   |    slave addr    |   control byte   |   data  byte    |   ...    |stop|
 */
 
 
 /*
  *function: bma020_i2c_write
  *read one byte data from the sensor
  *exist bug ,didn't avoid write the bit7-bit5 of 0x14h and bit7,bit5,bit4 must be 0 of 0x0Ah
  *when write the chip ,mey be disable the interrupt
 */
/*	i2c write routine for bma150	*/
static inline char bma020_i2c_write(struct i2c_client *client, unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	int dummy;
	if( client == NULL )	/*	No global client pointer?	*/
		return -1;
    struct bma020 *bma=i2c_get_client(client);
	while(len--)
    {
		buffer[0] = reg_addr;
		buffer[1] = *data;
		if( RANGE_CFG == buffer[0] )
		     {
        		 buffer[1]&=~0xE0;
				 buffer[1]|=bma->protect_bit;
	         }
		 if( STATUS_CTL ==buffer[0] )
		      {
			     buffer[1]&=~0xB0;
			     buffer[1]|=0x40;
			  }
		dummy = i2c_master_send(bma150_client, (char*)buffer, 2);
		reg_addr++;
		data++;
		if(dummy < 0)
			return -1;
	}
	return 0;
}


/***
 *i2c multiple read protocol ,address is first written to BMA020,the RW=0;
 *i2c transfer is stopped and restarted with RW=1,address is automatically
 *incremented and the 6bytes can be sequentially read out.
 *    start  |  slave address   |   read reg    |stop
 *
 *    start  |   slave address   |   data buf    | stop
 *
 */
 
/*	i2c read routine for bma150	*/
static inline char bma020_i2c_read(struct i2c_client *client, unsigned char reg_addr, unsigned char *data, unsigned char len) 
{
	int dummy;
	if( bma150_client == NULL )	/*	No global client pointer?	*/
		return -1;

	while(len--)
	{        
		dummy = i2c_master_send(bma150_client, (char*)&reg_addr, 1);
		if(dummy < 0)
			return -1;
		dummy = i2c_master_recv(bma150_client, (char*)data, 1);
		if(dummy < 0)
			return -1;
		reg_addr++;
		data++;
	}
	return 0;
}

/*
*function :bma020_read_triple
*read the value of x,y,z data
*/
void bma020_get_axis_triple(struct bma020 *bm,struct bma020_triple *current_axis)
{
    unsigned int   Buffer[3] = {0,0,0};	//Three Axis Buffer Value
	signed   int   Decimal[3] = {0,0,0}; //Three Axis Deciaml Value
	char   axis_value[6];
	struct i2c_client *client=bm->client;
	unsigned char  X_DIR = 0, Y_DIR = 1, Z_DIR = 2;  // Three Directions
    char X_Low,Y_Low,Z_Low;
	
	mutex_lock(&bm->mutex);	
    //read the value of current axis.   
    bma020_i2c_read(client,DATAXL,axis_value,6);	
	
	mutex_unlock(&bm->mutex);
	
    X_Low = axis_value[0]&0xC0>>6;
	Y_Low = axis_value[2]&0xC0>>6;
	Z_Low = axis_value[4]&0xC0>>6;

	Buffer[X_DIR] = (axis_value[1]<<2) | X_Low;
	Buffer[Y_DIR] = (axis_value[3]<<2) | Y_Low;
	Buffer[Z_DIR] = (axis_value[5]<<2) | Z_Low;

	
	// print 3-axis value in Hex
	// printk("ADXL345/346 axis triple: X axis = 0x%04x, Y axis = 0x%04x, Z axis = 0x%04x \n" , Buffer[0], Buffer[1],  Buffer[2]);

	// Translate the twos complement to true binary code for +/-
	if (Buffer[X_DIR] > 0x400)
		Decimal[X_DIR] = Buffer[X_DIR] - 0x800;
	else
		Decimal[X_DIR] = Buffer[X_DIR];

	if (Buffer[Y_DIR] > 0x400)
		Decimal[Y_DIR] = Buffer[Y_DIR] - 0x800;
	else
		Decimal[Y_DIR] = Buffer[Y_DIR];
	
	if (Buffer[Z_DIR] > 0x400)
		Decimal[Z_DIR] = Buffer[Z_DIR] - 0x800;
	else
		Decimal[Z_DIR] = Buffer[Z_DIR];
		
	current_axis->x = Decimal[Y_DIR];
	current_axis->y = Decimal[X_DIR]* -1;
	current_axis->z = Decimal[Z_DIR];

}
/***--------------------------------------------------------------------------------
 * this function didn't changed from the adxl345 sensor
*/ 

void bma020_work(struct work_struct *work)
{
	struct bma020 *bma = container_of(work, struct bma020, work);
	struct bma020_platform_data *pdata = &bma->pdata;
	unsigned char int_stat, tap_stat;
#ifdef USE_FIFO_MODE
	unsigned char fifo_state;
	int samples;
#endif
	
	//bmaT_TAP_STATUS should be read before clearing the interrupt
	 // Avoid reading bmaT_TAP_STATUS in case TAP detection is disabled
	if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))  // | SUPPRESS))
		bma_READ(bma, bmaT_TAP_STATUS, &tap_stat);
	else
		tap_stat = 0;
		
  	//read interrpt source
	bma_READ(bma, INT_SOURCE, &int_stat);
	
  	//if free fall
	if (int_stat & FREE_FALL)
	{
		dprintk(KERN_ALERT "\n%s: Free Fall %d\n",__func__,++ff_count);
#ifdef REGISTER_INPUT_DEVICE
		bma020_report_key_single(bma->input, pdata->ev_code_ff);
#endif
	}
#ifdef USE_FIFO_MODE
  	//if overrun
	if (int_stat & OVERRUN)
	{
		dprintk(KERN_ALERT "%s OVERRUN\n",__func__);
	}
#endif
 	 //if single tap
	if (int_stat & SINGLE_TAP) 
	{	
		if (tap_stat & TAP_X_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapX  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_single(bma->input,pdata->ev_code_tap_x);
#endif
		}
		if (tap_stat & TAP_Y_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapY  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_single(bma->input,pdata->ev_code_tap_y);
#endif
		}
		if (tap_stat & TAP_Z_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapZ  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_single(bma->input,pdata->ev_code_tap_z);
#endif
		}
	}
  	//if double tap
	if (int_stat & DOUBLE_TAP) 
	{
		if (tap_stat & TAP_X_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapX  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_double(bma->input,pdata->ev_code_tap_x);
#endif
		}
		if (tap_stat & TAP_Y_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapY  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_double(bma->input,pdata->ev_code_tap_y);
#endif
		}
		if (tap_stat & TAP_Z_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapZ  %d\n",__func__,++tap_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_double(bma->input,pdata->ev_code_tap_z);
#endif
		}
	}
  	//if bmativity or inbmativity
	if (pdata->ev_code_bmativity || pdata->ev_code_inbmativity) 
	{	
		if (int_stat & bmaTIVITY)
		{
			if(tap_stat & bmaT_X_SRC)
			{
				dprintk(KERN_ALERT "\n%s: bmativity bmatX  %d\n",__func__,++bmat_count);
			}
			if(tap_stat & bmaT_Y_SRC)
			{
				dprintk(KERN_ALERT "\n%s: bmativity bmatY  %d\n",__func__,++bmat_count);
			}
			if(tap_stat & bmaT_Z_SRC)
			{
				dprintk(KERN_ALERT "\n%s: bmativity bmatZ  %d\n",__func__,++bmat_count);
			}
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_single(bma->input, pdata->ev_code_bmativity);
#endif
		}
		if (int_stat & INbmaTIVITY)
		{
			dprintk(KERN_ALERT "\n%s: Inbmativity %d\n",__func__, ++inbmat_count);
#ifdef REGISTER_INPUT_DEVICE
			bma020_report_key_single(bma->input, pdata->ev_code_inbmativity);
#endif
		}
	}
#ifdef USE_FIFO_MODE
	//if data ready | watermark
	if (int_stat & (DATA_READY | WATERMARK)) 
	{
		//dprintk(KERN_ALERT "%s: DataReady or WaterMark\n",__func__);
		if (pdata->fifo_mode)
		{
			bma_READ(bma, FIFO_STATUS, &fifo_state);
			samples = ENTRIES(fifo_state) + 1;
		}
		else
			samples = 1;

		for (; samples>0; samples--) 
		{
#ifdef REGISTER_INPUT_DEVICE
			bma020_service_ev_fifo(bma);
#endif
			/* To ensure that the FIFO has
			 * completely popped, there must be at least 5 us between
			 * the end of reading the data registers, signified by the
			 * transition to register 0x38 from 0x37 or the CS pin
			 * going high, and the start of new reads of the FIFO or
			 * reading the FIFO_STATUS register. For SPI operation at
			 * 1.5 MHz or lower, the register addressing portion of the
			 * transmission is sufficient delay to ensure the FIFO has
			 * completely popped. It is necessary for SPI operation
			 * greater than 1.5 MHz to de-assert the CS pin to ensure a
			 * total of 5 us, which is at most 3.4 us at 5 MHz
			 * operation.*/
			if (bma->fifo_delay && (samples>1))
				udelay(3);
		}
	}
#endif
	enable_irq(bma->client->irq);
}

static void bma020_sleep(struct bma020 *bm)
{
    
	unsigned char data;
	mutex_lock(&bm->mutex);
	if (bm->nomal) 
	{
		BM_RD(bm,OPER_REG,&data,1);
		data|=~0x01;
		BM_WR(bm,OPER_REG,&data,1);
	}
	mutex_unlock(&bm->mutex);
}

static void bma020_wakeup(struct bma020 *bm)
{
	unsigned char data;
	mutex_lock(&bm->mutex);
	if (!bm->nomal) 
	{
		BM_RD(bm,OPER_REG,&data,1);
		data|=0x01;
		BM_WR(bm,OPER_REG,&data,1);
	}
	mutex_unlock(&bm->mutex);
}
static void bma020_reset(struct bma020 *bm)
{
    unsigned char data;
	mutex_lock(&bm->mutex);
        {
		BM_RD(bm,STATUS_CTL,&data,1);
		data|=0x01;
		BM_WR(bm,STATUS_CTL,&data,1);
	    }
	mutex_unlock(&bm->mutex);
}


static irqreturn_t bma020_irq(int irq, void *handle)
{
	struct bma020 *bma = handle;
	disable_irq_nosync(irq);
	schedule_work(&bma->work);
	return IRQ_HANDLED;
}

static void bma020_read_and_show_regs(struct bma020 *bm)
{ 
  //unsigned char reg_data;
  unsigned char buf[REG_MAX];
  short int data;
  
  BM_RD(bm,CHIP_ID,buf,REG_MAX);
  dprintk(KERN_ALERT "CHIP_ID is %d.\n",buf[0]);
  dprintk(KERN_ALERT "CHIP_VERSION is %d.\n",buf[1]);
  dprintk(KERN_ALERT "DATAXL  is %d.\n",buf[2]);
  dprintk(KERN_ALERT "DATAXM is %d.\n",buf[3]);
  dprintk(KERN_ALERT "DATAYL  is %d.\n",buf[4]);
  dprintk(KERN_ALERT "DATAYM is 0x%x.\n",buf[5]);
  dprintk(KERN_ALERT "DATAZL  is %d.\n",buf[6]);
  dprintk(KERN_ALERT "DATAZM is 0x%x.\n",buf[7]);
 
  dprintk(KERN_ALERT "STATUS_REG   is 0x%x.\n",buf[9]);
  dprintk(KERN_ALERT "STATUS_CTL     is 0x%x.\n",buf[10]);
  dprintk(KERN_ALERT "INTERRUPT_SET    is 0x%x.\n",buf[11]);
  dprintk(KERN_ALERT "LG_THRES is 0x%x.\n",buf[12]);
  dprintk(KERN_ALERT "LG_DUR     is 0x%x.\n",buf[13]);
  dprintk(KERN_ALERT "HG_THRES   is 0x%x.\n",buf[14]);

  dprintk(KERN_ALERT "HG_DUR  is 0x%x.\n",buf[15]);
  dprintk(KERN_ALERT "ANY_MOTION_THRES     is 0x%x.\n",buf[0x10]);
  dprintk(KERN_ALERT "DUR_HYST  is 0x%x.\n",buf[0x11]);
  dprintk(KERN_ALERT "CUSTOM_REG1 is 0x%x.\n",buf[0x12]);
  dprintk(KERN_ALERT "CUSTOM_REG2 is 0x%x.\n",buf[0x13]);
  dprintk(KERN_ALERT "RANGE_CFG is 0x%x.\n",buf[0x14]);
  dprintk(KERN_ALERT "OPER_CFG is 0x%x.\n",buf[0x15]);
}

static int __devinit bma020_initialize(struct i2c_client *client, struct bma020 *bma)
{

	struct input_dev *input_dev;
	
	struct bma020_platform_data *devpd = client->dev.platform_data;
	struct bma020_platform_data *pdata;
	int err;
	unsigned char chipid,protect;
 	
 	dprintk(KERN_ALERT "Enter %s \n", __FUNCTION__);
	
	if (!client->irq) 
	{
		dprintk(KERN_ALERT "%s no IRQ?\n",__func__);
		return -ENODEV;
	}

	if (!devpd) 
	{
		dprintk(KERN_ALERT "%s No platfrom data: Using default initialization\n",__func__);
		devpd = (struct bma020_platform_data *)&bma020_default_init;
	}

	memcpy(&bma->pdata, devpd, sizeof(bma->pdata));
	pdata = &bma->pdata;
	input_dev = input_allocate_device();
	if (!input_dev)
	{
		return -ENOMEM;
	}
	bma->input = input_dev;

	INIT_WORK(&bma->work, bma020_work);
	mutex_init(&bma->mutex);

	BM_RD(bma, CHIP_ID, &chipid,1);    
	if( BMA_ID == (chipid&0xFF))
	{
	     dprintk(KERN_ALERT "************************************************************\n");
		 dprintk(KERN_ALERT "bma020 registered and  ID is 0x%x.\n",BMA_ID);
		 dprintk(KERN_ALERT "************************************************************\n");
	}
	else 
	{
	     dprintk(KERN_ALERT "%s Failed to probe, no find sensor device.\n", __func__);
		 err = -ENODEV;
		 goto err_free_mem;
	}
    bma->chip_id=chipid;
	//bit 7,6,5,of register addresses 14H do contain critial
	//sensor individual calibration data which must not be changed 
	
	BM_RD(bma, CHIP_VERSION, &bma->version_id,1); 
	dprintk(KERN_ALERT "bma020 registered and  ID is 0x%x.\n",BMA_ID);
	
	 BM_RD(bma, RANGE_CFG, &protect,1);	 
	 bma->protect_bit=protect&0xE0;
	
	snprintf(bma->phys, sizeof(bma->phys), "%s/input0", dev_name(&client->dev));

	input_dev->name = "bma020 celerometer";
	input_dev->phys = bma->phys;
	input_dev->dev.parent = &client->dev;
	input_dev->id.product = BMA_ID;
	input_dev->id.bustype = BUS_I2C;	
	input_set_drvdata(input_dev, bma);//input_dev->dev.driver_data = bma;

	__set_bit(bma->pdata.ev_type, input_dev->evbit);
	
	if (bma->pdata.ev_type == EV_REL) 
	{
		__set_bit(REL_X, input_dev->relbit);
		__set_bit(REL_Y, input_dev->relbit);
		__set_bit(REL_Z, input_dev->relbit);
	} 
	else /* EV_ABS */
	{
		__set_bit(ABS_X, input_dev->absbit);
		__set_bit(ABS_Y, input_dev->absbit);
		__set_bit(ABS_Z, input_dev->absbit);

		input_set_abs_params(input_dev, ABS_X, -RANGE, RANGE, 3, 3);
		input_set_abs_params(input_dev, ABS_Y, -RANGE, RANGE, 3, 3);
		input_set_abs_params(input_dev, ABS_Z, -RANGE, RANGE, 3, 3);
	}
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(pdata->ev_code_tap_x, input_dev->keybit);
	__set_bit(pdata->ev_code_tap_y, input_dev->keybit);
	__set_bit(pdata->ev_code_tap_z, input_dev->keybit);

	if (pdata->ev_code_ff) 
	{
		bma->int_mask = FREE_FALL;
		__set_bit(pdata->ev_code_ff, input_dev->keybit);
	}

	if (pdata->ev_code_bmativity)
	{
		__set_bit(pdata->ev_code_bmaativity, input_dev->keybit);
	}
	
	if (pdata->ev_code_bmainativity)
	{
		__set_bit(pdata->ev_code_bmainativity, input_dev->keybit);
	}
	/*
	//some thing error
	bma->int_mask = FREE_FALL;
	bma->int_mask |= bmaTIVITY | INbmaTIVITY;
	
	
	//if have watermark, it must use fifo.
	if (pdata->watermark) 
	{
		bma->int_mask |= WATERMARK;
		if (!FIFO_MODE(pdata->fifo_mode))
			pdata->fifo_mode |= FIFO_STREAM;
	} 
	else 
	{
		//bma->int_mask |= DATA_READY;
	}

	if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))// | SUPPRESS))
		bma->int_mask |= SINGLE_TAP | DOUBLE_TAP;

	if (FIFO_MODE(pdata->fifo_mode) == FIFO_BYPASS)
		bma->fifo_delay = 0;

	bma_WRITE(bma, POWER_CTL, 0);
  
	err = request_irq(client->irq, bma020_irq, IRQF_TRIGGER_FALLING, client->dev.driver->name, bma);
	if (err) 
	{
		dev_err(&client->dev, "irq %d busy?\n", client->irq);
		goto err_free_mem;
	}	
*/
	err = input_register_device(input_dev);
	if (err)
		goto err_remove_attr;

	//***** Setting g-sensor via I2C initialize the gsensor*****
	bma_WRITE(bma, THRESH_TAP, pdata->tap_threshold);
	

	pdata->power_mode &= (PCTL_AUTO_SLEEP | PCTL_LINK);

	return 0;

 err_free_irq:
	 free_irq(client->irq, bma);
 err_free_mem:
	 input_free_device(input_dev);

	return err;
}

#ifdef CONFIG_PM
static int bma020_suspend(struct i2c_client *client, pm_message_t message)
{
	struct bma020 *bma = dev_get_drvdata(&client->dev);
	bma020_sleep(bma);
	return 0;
}

static int bma020_resume(struct i2c_client *client)
{
	struct bma020 *bma = dev_get_drvdata(&client->dev);
	bma020_wakeup(bma);
	return 0;
}
#else
#define bma020_suspend NULL
#define bma020_resume  NULL
#endif


/*
 *config the bma020 gpio 
*/
static int bma020_gpio_config()
{
//config the gpn3 as the irq gpio
    s3c_gpio_cfgpin(S3C64XX_GPN(3), S3C_GPIO_SFN(3)); 
	s3c_gpio_setpull(S3C64XX_GPN(3), S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_EINT(gpio_to_irq(S3C64XX_GPN(3)), IRQ_TYPE_EDGE_FALLING);
	
	return 0;

}
static int __devinit bma020_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bma020 *bma;
	int error;
	
	dprintk("Enter %s \n", __FUNCTION__);
	bma = kzalloc(sizeof(struct bma020), GFP_KERNEL);
	if (!bma)
		return -ENOMEM;

	i2c_set_clientdata(client, bma);//client->dev.driver_data = bma;

	bma->client = client;
	bma->read = bma020_i2c_read;
	bma->write = bma020_i2c_write;
	bma->init = bma020_initialize;
	bma->config_gpio = bma020_gpio_config;
	bma->reset=bma020_reset;
	
	if(bma->config_gpio)
	{
	   bma->config_gpio();
	}
	if(bma->init){
	   error = bma->init(client, bma);
	      if (error) 
	         {
		        i2c_set_clientdata(client, NULL);
		        kfree(bma);
	          }
     }

	return 0;
}

static int __devexit bma020_cleanup(struct i2c_client *client, struct bma020 *bma)
{
	bma020_disable(bma);
	free_irq(bma->client->irq, bma);
	input_unregister_device(bma->input);
	dprintk(KERN_ALERT "%s unregistered bmacelerometer\n",__func__);
	return 0;
}

static int __devexit bma020_i2c_remove(struct i2c_client *client)
{
	struct bma020 *bma = dev_get_drvdata(&client->dev);
	bma020_cleanup(client, bma);
	i2c_set_clientdata(client, NULL);
	kfree(bma);
	return 0;
}

static const struct i2c_device_id bma020_id[] = {
	{ "bma020", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma020_id);

static struct i2c_driver bma020_driver = {
	.driver = {
		.name = "bma020",
		.owner = THIS_MODULE,
	},
	.probe    = bma020_i2c_probe,
	.remove   = __devexit_p(bma020_i2c_remove),
	.suspend  = bma020_suspend,
	.resume   = bma020_resume,
	.id_table = bma020_id,
};

static int __init bma020_i2c_init(void)
{
	return i2c_add_driver(&bma020_driver);
}
module_init(bma020_i2c_init);

static void __exit bma020_i2c_exit(void)
{
	i2c_del_driver(&bma020_driver);
}
module_exit(bma020_i2c_exit);

MODULE_AUTHOR("Andy cheng <chengrong070@sina.com>");
MODULE_DESCRIPTION("bma020 Digital ,triaxial bmacelerometer Sensor Driver");
MODULE_LICENSE("GPL");


