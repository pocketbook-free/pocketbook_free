/*
 * core.h  --  Core Driver for Wolfson Max8698 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
 
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#include <linux/mfd/max8698/gpio.h>
#include <linux/mfd/max8698/pmic.h>
#include <linux/mfd/max8698/supply.h>


#define MAX8698_MAX_REGISTER   0x0F
#define MAX8698_NUM_IRQ  32

struct max8698_irq {
	void (*handler) (struct max8698 *, int, void *);
	void *data;
};

struct max8698 {
	struct device *dev;

	/* device IO */
	union {
		struct i2c_client *i2c_client;
		struct spi_device *spi_device;
	};
	int (*read_dev)(struct max8698 *max8698, char reg, int size, void *dest);
	int (*write_dev)(struct max8698 *max8698, char reg, int size,
			 void *src);
	u16 *reg_cache;


/* Interrupt handling */
	struct work_struct irq_work;
	struct mutex irq_mutex; /* IRQ table mutex */
	struct max8698_irq irq[MAX8698_NUM_IRQ];
	int chip_irq;

	/* Client devices 
	struct max8698_gpio gpio;
	struct max8698_pmic pmic;
	*/
	struct max8698_power power;

};

#if 1
struct max17043_device_info;

struct max17043_access_methods {
	int (*read)(u8 reg, int *rt_value, int b_single,
		struct max17043_device_info *di);
	int (*write)(u8 reg,int bytes,void *src,
		struct max17043_device_info *di);
};

struct max17043_irq {
	void (*handler) (struct max17043_device_info *, int, void *);
	void *data;
};

struct max17043_device_info {
	struct device 		*dev;
	int			id;
	int			voltage_uV;
	int			current_uA;
	int			temp_C;
	int			charge_rsoc;
	struct max17043_access_methods	*bus;
	struct power_supply	bat;
	
	struct work_struct irq_work;
	struct mutex irq_mutex; /* IRQ table mutex */
	struct max17043_irq irq[32];
	int chip_irq;

	struct i2c_client	*client;

	struct max17043_power power;
};


#endif
/**
 * Data to be supplied by the platform to initialise the WM8350.
 *
 * @init: Function called during driver initialisation.  Should be
 *        used by the platform to configure GPIO functions and similar.
 */
struct max17043_platform_data {
	int (*init)(struct max17043_device_info *max17043);
//	struct work_struct charge_state_irq_work;
	struct mutex irq_mutex; /* IRQ table mutex */
	int pmic_fault_irq;
	int low_battery_irq;
	int charge_state_irq;
};

struct max8698_platform_data {
	int (*init)(struct max8698 *max8698);
};

u8 max8698_reg_read(struct max8698 *max8698,int reg);
int max8698_reg_write(struct max8698 *max8698,int reg,u8 val);

int max8698_device_init(struct max8698 *max8698,struct max8698_platform_data *pdata);
void max8698_device_exit(struct max8698 *max8698);

u8 max8698_reg_read(struct max8698 *max8698,int reg);
int max8698_reg_write(struct max8698 *max8698,int reg,u8 val);

int max8698_register_irq(struct max8698 *max8698, int irq,
			void (*handler) (struct max8698 *, int, void *),
			void *data);
int max8698_free_irq(struct max8698 *max8698, int irq);

int max17043_power_charger_init(struct max17043_device_info *max17043,int irq,struct max17043_platform_data *pdata);
void max17043_device_exit(struct max17043_device_info *max17043);

//int charge_state_irq_init(struct max17043_device_info *max17043);
int charge_state_irq_init(struct max17043_device_info *max17043,struct max17043_platform_data *pdata);

int max17043_register_irq(struct max17043_device_info *max17043, int irq,
			void (*handler) (struct max17043_device_info *, int, void *),
			void *data);
int max17043_free_irq(struct max17043_device_info *max17043, int irq);

struct max17043_temp_access {
	u16 V_bat_ntc;		/* max17043 NTC voltage value */
	u16 R_center;		/* max17043 R value */
	int T_temp_ntc;		/* max17043 NTC temperature value */
};

extern const struct max17043_temp_access max17043_temp_map[];

