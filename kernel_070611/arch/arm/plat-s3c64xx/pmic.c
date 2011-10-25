#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <plat/s3c64xx-dvfs.h>
#include <linux/regulator/consumer.h>


#ifdef CONFIG_DVFS_LOGMESSAGE
#include <linux/cpufreq.h>
#include <linux/kernel.h>
extern void dvfs_debug_printk(unsigned int type, const char *prefix, const char *fmt, ...);
#define dprintk(msg...) dvfs_debug_printk(CPUFREQ_DEBUG_DRIVER, "dvfs", msg)
#else

#define dprintk(msg...) do { } while(0)

#endif /* CONFIG_CPU_FREQ_DEBUG */

/* wm8350 voltage table */
static const unsigned int voltage_table[32] = {
	1625, 1600, 1575, 1550, 1525, 1500, 1475, 1450, 
	1425, 1400, 1375, 1350, 1325, 1300, 1275, 1250, 
	1225, 1200, 1175, 1150, 1125, 1100, 1075, 1050, 
	1025, 1000, 975, 950, 925, 900, 875, 850,
};	

struct regulator *pVcc_arm = NULL;
struct regulator *pVcc_int = NULL;

int set_pmic(unsigned int pwr, unsigned int voltage)
{
	int position = 0;
	int first = 0;

	int last = 31;

	unsigned int tmp;

	if(voltage > voltage_table[0] || voltage < voltage_table[last]) {
		dprintk("[ERROR]: voltage value over limits!!!");
		return -EINVAL;
	}

	while(first <= last) {
		position = (first + last) / 2;
		if(voltage_table[position] == voltage) {

			position &=0x1f;

			if(pwr == VCC_ARM) {
				
				if (pVcc_arm != NULL)				
					regulator_set_voltage(pVcc_arm, voltage*1000, voltage*1000);
				
				dprintk("============ VDD_ARM = %d\n",voltage);
				return 0;
			}
			else if(pwr == VCC_INT) {

				if (pVcc_int != NULL)
					regulator_set_voltage(pVcc_int, voltage*1000, voltage*1000);
				
				dprintk("============ VDD_INT = %d\n",voltage);
				return 0;
			}
			else {
				dprintk("[error]: set_power, check mode [pwr] value\n");			
				return -EINVAL;
			}

		}
		else if (voltage > voltage_table[position])
			last = position - 1;
		else
			first = position + 1;
	}
	dprintk("[error]: Can't find adquate voltage table list value\n");
	return -EINVAL;
}

void pmic_init(unsigned int arm_voltage, unsigned int int_voltage)
{

	pVcc_arm = regulator_get(NULL, "vcc_arm");

	if (IS_ERR(pVcc_arm))
	{
		pVcc_arm = NULL;
		printk(KERN_ERR "%s: cant get vcc_arm\n", __FUNCTION__);
		return ;
	}	

	pVcc_int = regulator_get(NULL, "vcc_1.2v");

	if (IS_ERR(pVcc_int))
	{
		pVcc_int = NULL;
		printk(KERN_ERR "%s: cant get vcc_1.2v\n", __FUNCTION__);
		return ;
	}	
	
	/* set maximum voltage */
	set_pmic(VCC_ARM, arm_voltage);
	set_pmic(VCC_INT, int_voltage);

}

EXPORT_SYMBOL(pmic_init);
