#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>

static int hwversion_proc_ep_show(struct seq_file *m, void *v)
{
   char version[3];
   
  // version=version|( gpio_get_value(S3C64XX_GPK(11))|gpio_get_value(S3C64XX_GPK(12))<<1|gpio_get_value(S3C64XX_GPK(13))<<1);
 #if (defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT))
   s3c_gpio_cfgpin(S3C64XX_GPO(11), S3C_GPIO_SFN(0) );
   s3c_gpio_cfgpin(S3C64XX_GPO(12), S3C_GPIO_SFN(0) );
   s3c_gpio_cfgpin(S3C64XX_GPO(13), S3C_GPIO_SFN(0) );
   sprintf(version,"%c%c%c",!!gpio_get_value(S3C64XX_GPO(11))?'1':'0',!!gpio_get_value(S3C64XX_GPO(12))?'1':'0',!!gpio_get_value(S3C64XX_GPO(13))?'1':'0');
#elif (defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_DVT))
   s3c_gpio_cfgpin(S3C64XX_GPJ(5), S3C_GPIO_SFN(0) );
   s3c_gpio_cfgpin(S3C64XX_GPJ(6), S3C_GPIO_SFN(0) );
   s3c_gpio_cfgpin(S3C64XX_GPJ(7), S3C_GPIO_SFN(0) );
   sprintf(version,"%c%c%c",!!gpio_get_value(S3C64XX_GPJ(5))?'1':'0',!!gpio_get_value(S3C64XX_GPJ(6))?'1':'0',!!gpio_get_value(S3C64XX_GPJ(7))?'1':'0');
#endif 
   seq_printf(m, "version=%s\n", version);
	return 0;
}

static int hwversion_proc_ep_open(struct inode *inode, struct file *file)
{
	return single_open(file, hwversion_proc_ep_show, NULL);
}

static const struct file_operations hwinfo_proc_ep_fops = {
	.open		= hwversion_proc_ep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_ephardware_info_init(void)
{
#if (defined(CONFIG_HW_EP3_DVT)||defined(CONFIG_HW_EP4_DVT))
   s3c_gpio_cfgpin(S3C64XX_GPO(11), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPO(11), S3C_GPIO_PULL_NONE);
   s3c_gpio_cfgpin(S3C64XX_GPO(12), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPO(12), S3C_GPIO_PULL_NONE); 
   s3c_gpio_cfgpin(S3C64XX_GPO(13), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPO(13), S3C_GPIO_PULL_NONE);
#elif (defined(CONFIG_HW_EP1_DVT)||defined(CONFIG_HW_EP2_DVT))
   s3c_gpio_cfgpin(S3C64XX_GPJ(5), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPJ(5), S3C_GPIO_PULL_NONE);
   s3c_gpio_cfgpin(S3C64XX_GPJ(6), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPJ(6), S3C_GPIO_PULL_NONE);
   s3c_gpio_cfgpin(S3C64XX_GPJ(7), S3C_GPIO_SFN(0) );
   s3c_gpio_setpull(S3C64XX_GPJ(7), S3C_GPIO_PULL_NONE);
#endif 
	proc_create("epversion", 0, NULL, &hwinfo_proc_ep_fops);
	return 0;
}
module_init(proc_ephardware_info_init);
