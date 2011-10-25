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

#define wifi_power_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

int wifi_power_enabled = 0;

typedef int __bitwise wifi_state_t;

#define WIFI_POWER_OFF		((__force wifi_state_t) 0)
#define WIFI_POWER_ON	((__force wifi_state_t) 1)
#define WIFI_POWER_MAX		((__force wifi_state_t) 2)

static const char * const wifi_states[WIFI_POWER_MAX] = {
	[WIFI_POWER_OFF]	= "off",
	[WIFI_POWER_ON]		= "on",
};

static int wifi_power_configure(int status)
{
	if(status)
	{
		gpio_set_value(S3C64XX_GPN(6), 1);		
		s3c_gpio_cfgpin(S3C64XX_GPN(6),S3C64XX_GPN_OUTPUT(6));	
		s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_UP);
  }
  else
  {
  	gpio_set_value(S3C64XX_GPN(6), 0);		
		s3c_gpio_cfgpin(S3C64XX_GPN(6),S3C64XX_GPN_OUTPUT(6));	
		s3c_gpio_setpull(S3C64XX_GPN(6), S3C_GPIO_PULL_DOWN);
  }
  return 0;
}

static ssize_t wifi_power_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
		return sprintf(buf,"%s\n", wifi_states[wifi_power_enabled]);
}

static ssize_t wifi_power_store(struct kobject *kobj, struct kobj_attribute *attr,
	       const char *buf, size_t n)
{
//	int val;
	int len = 0;
	wifi_state_t state = WIFI_POWER_OFF;
	const char * const *s;
	char *p;
	
	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;
	
	for (s = &wifi_states[state]; state < WIFI_POWER_MAX; s++, state++) 
		{
		if (*s && len == strlen(*s) && !strncmp(buf, *s, len))
			break;
		}
	
	if (state < WIFI_POWER_MAX && *s)
	{
	 if (state == WIFI_POWER_ON)
	  {
	  	wifi_power_enabled = 1;
		 wifi_power_configure(1);
		 return n;
	  }
	else
	 {
	 	wifi_power_enabled = 0;
		wifi_power_configure(0);
		return n;
	 }
  }
	return -EINVAL;
}

wifi_power_attr(wifi_power);

static struct attribute * attr = &wifi_power_attr.attr;

static int __init wifi_power_init(void)
{
	power_kobj = kobject_create_and_add("wifi", NULL);
	if (!power_kobj)
		return -ENOMEM;
	return sysfs_create_file(power_kobj, attr);
}

core_initcall(wifi_power_init);

