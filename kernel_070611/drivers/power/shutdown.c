/*
 * Wi-Fi built-in chip control
 *
 * Copyright (c) 2010 Foxconn TMSBG
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/resume-trace.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#define power_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

typedef int __bitwise wifi_state_t;

#define WIFI_POWER_OFF		((__force wifi_state_t) 0)
#define WIFI_POWER_ON	((__force wifi_state_t) 1)
#define WIFI_POWER_MAX		((__force wifi_state_t) 2)

static const char * const wifi_states[WIFI_POWER_MAX] = {
	[WIFI_POWER_OFF]	= "off",
	[WIFI_POWER_ON]		= "on",
};

static ssize_t power_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
	int value = gpio_get_value(S3C64XX_GPN(1));
		return sprintf(buf,"%d\n",value );
}

static ssize_t power_store(struct kobject *kobj, struct kobj_attribute *attr,
	       const char *buf, size_t n)
{
	return 0;
}

power_attr(power);

static struct attribute * attr = &power_attr.attr;

static int __init wifi_power_init(void)
{
	power_kobj = kobject_create_and_add("shutdown", NULL);
	if (!power_kobj)
		return -ENOMEM;
	return sysfs_create_file(power_kobj, attr);
}

core_initcall(wifi_power_init);

