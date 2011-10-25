/* linux/arch/arm/plat-s3c24xx/pm.c
 *
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX Power Manager (Suspend-To-RAM) support
 *
 * See Documentation/arm/Samsung-S3C24XX/Suspend.txt for more information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 * 
 * Thanks to Dimitry Andric for debugging
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/hardware.h>
#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>

#include <asm/mach/time.h>
#include <linux/wakelock.h>
#include <plat/pm.h>
#include <plat/s3c64xx-dvfs.h>
#include "../../../drivers/input/keyboard/key_for_ioctl.h"
#include <linux/ioc668-dev.h>

extern struct wake_lock usbinsert_wake_lock;
unsigned long s3c_pm_flags;

/* for external use */
#define PFX "s3c64xx-pm: "

#ifdef CONFIG_S3C64XX_DOMAIN_GATING
#include <plat/power-clock-domain.h>

#ifdef CONFIG_S3C64XX_DOMAIN_GATING_DEBUG
static int domain_hash_map[15]={0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 7};
static char *domain_name[] = {"G","V", "I", "P", "F", "S", "ETM", "IROM"}; 
#endif /* CONFIG_S3C64XX_DOMAIN_GATING_DEBUG */

//unsigned int Morse_interrupt_flag = 0 ;
static spinlock_t power_lock;

static unsigned int s3c_domain_off_stat = 0x1FFC;
unsigned long wakeup_key;
extern unsigned int suspend_routine_flag;

extern unsigned int nosuspend_routine_keycode_flag;

static void s3c_init_domain_power(void)
{
	spin_lock_init(&power_lock);

//&*&*&*HC1_20100129, disable unnecessary block power to reduce idle power
#if 0
	s3c_set_normal_cfg(S3C64XX_DOMAIN_V, S3C64XX_LP_MODE, S3C64XX_MFC);
//	s3c_set_normal_cfg(S3C64XX_DOMAIN_G, S3C64XX_LP_MODE, S3C64XX_3D);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_JPEG);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_CAMERA);
//	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_2D);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_TVENC);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_SCALER);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_ROT);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_POST);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA0);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA1);
#else

	s3c_set_normal_cfg(S3C64XX_DOMAIN_V, S3C64XX_LP_MODE, S3C64XX_MFC);

	s3c_set_normal_cfg(S3C64XX_DOMAIN_G, S3C64XX_LP_MODE, S3C64XX_3D);

	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_JPEG);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_CAMERA);
	
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_2D);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_TVENC);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_SCALER);
	
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_ROT);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_POST);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_LCD);	

	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA0);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA1);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SECURITY);	

	s3c_set_normal_cfg(S3C64XX_DOMAIN_ETM, S3C64XX_LP_MODE, S3C64XX_ETM);	

	s3c_set_normal_cfg(S3C64XX_DOMAIN_IROM, S3C64XX_LP_MODE, S3C64XX_IROM);		
#endif
//&*&*&*HC2_20100129, disable unnecessary block power to reduce idle power
}

int domain_off_check(unsigned int config)
{
	if(config == S3C64XX_DOMAIN_V) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_V_MASK)	
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_G) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_G_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_I) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_I_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_P) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_P_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_F) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_F_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_S) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_S_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_ETM) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_ETM_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_IROM) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_IROM_MASK)
			return 0;
	}
	return 1;
}

EXPORT_SYMBOL(domain_off_check);

void s3c_set_normal_cfg(unsigned int config, unsigned int flag, unsigned int deviceID)
{
	unsigned int normal_cfg;
	int power_off_flag = 0;
	spin_lock(&power_lock);
	normal_cfg = __raw_readl(S3C_NORMAL_CFG);
	if(flag == S3C64XX_ACTIVE_MODE) {
		s3c_domain_off_stat |= (1 << deviceID);
		if(!(normal_cfg & config)) {
			normal_cfg |= (config);
			__raw_writel(normal_cfg, S3C_NORMAL_CFG);
#ifdef CONFIG_S3C64XX_DOMAIN_GATING_DEBUG
			printk("===== Domain-%s Power ON NORMAL_CFG : %x \n",
					domain_name[domain_hash_map[deviceID]], normal_cfg);
#endif /* CONFIG_S3C64XX_DOMAIN_GATING_DEBUG */
		}
		
	}
	else if(flag == S3C64XX_LP_MODE) {
		s3c_domain_off_stat &= (~( 1 << deviceID));
		power_off_flag = domain_off_check(config);
		if(power_off_flag == 1) {
			if(normal_cfg & config) {
				normal_cfg &= (~config);
				__raw_writel(normal_cfg, S3C_NORMAL_CFG);
#ifdef CONFIG_S3C64XX_DOMAIN_GATING_DEBUG
				printk("===== Domain-%s Power OFF NORMAL_CFG : %x \n",
						domain_name[domain_hash_map[deviceID]], normal_cfg);
#endif /* CONFIG_S3C64XX_DOMAIN_GATING_DEBUG */
			}
		}
	}	
	spin_unlock(&power_lock);

}
EXPORT_SYMBOL(s3c_set_normal_cfg);

int s3c_wait_blk_pwr_ready(unsigned int config)
{
	unsigned int blk_pwr_stat;
	int timeout;
	int ret = 0;
	
	/* Wait max 20 ms */
	timeout = 20;
	while (!((blk_pwr_stat = __raw_readl(S3C_BLK_PWR_STAT)) & config)) {
		if (timeout == 0) {
			printk(KERN_ERR "config %x: blk power never ready.\n", config);
			ret = 1;
			goto s3c_wait_blk_pwr_ready_end;
		}
		timeout--;
		mdelay(1);
	}
s3c_wait_blk_pwr_ready_end:
	return ret;
}
EXPORT_SYMBOL(s3c_wait_blk_pwr_ready);
#endif /* CONFIG_S3C64XX_DOMAIN_GATING */

static struct sleep_save core_save[] = {
	SAVE_ITEM(S3C_SDMA_SEL),
	SAVE_ITEM(S3C_HCLK_GATE),
	SAVE_ITEM(S3C_PCLK_GATE),
	SAVE_ITEM(S3C_SCLK_GATE),
	SAVE_ITEM(S3C_MEM0_CLK_GATE),
	SAVE_ITEM(S3C_CLK_SRC),
	SAVE_ITEM(S3C_CLK_DIV0),
	SAVE_ITEM(S3C_CLK_DIV1),
	SAVE_ITEM(S3C_CLK_DIV2),
	SAVE_ITEM(S3C_APLL_CON),
	SAVE_ITEM(S3C_MPLL_CON),
	SAVE_ITEM(S3C_EPLL_CON0),
	SAVE_ITEM(S3C_EPLL_CON1),
	SAVE_ITEM(S3C_NORMAL_CFG),
	SAVE_ITEM(S3C_AHB_CON0),
};

static struct sleep_save gpio_save[] = {
	SAVE_ITEM(S3C64XX_GPACON),
	SAVE_ITEM(S3C64XX_GPADAT),
	SAVE_ITEM(S3C64XX_GPAPUD),
	SAVE_ITEM(S3C64XX_GPACONSLP),
	SAVE_ITEM(S3C64XX_GPAPUDSLP),
	SAVE_ITEM(S3C64XX_GPBCON),
	SAVE_ITEM(S3C64XX_GPBDAT),
	SAVE_ITEM(S3C64XX_GPBPUD),
	SAVE_ITEM(S3C64XX_GPBCONSLP),
	SAVE_ITEM(S3C64XX_GPBPUDSLP),
	SAVE_ITEM(S3C64XX_GPCCON),
	SAVE_ITEM(S3C64XX_GPCDAT),
	SAVE_ITEM(S3C64XX_GPCPUD),
	SAVE_ITEM(S3C64XX_GPCCONSLP),
	SAVE_ITEM(S3C64XX_GPCPUDSLP),
	SAVE_ITEM(S3C64XX_GPDCON),
	SAVE_ITEM(S3C64XX_GPDDAT),
	SAVE_ITEM(S3C64XX_GPDPUD),
	SAVE_ITEM(S3C64XX_GPDCONSLP),
	SAVE_ITEM(S3C64XX_GPDPUDSLP),
	SAVE_ITEM(S3C64XX_GPECON),
	SAVE_ITEM(S3C64XX_GPEDAT),
	SAVE_ITEM(S3C64XX_GPEPUD),
	SAVE_ITEM(S3C64XX_GPECONSLP),
	SAVE_ITEM(S3C64XX_GPEPUDSLP),
	SAVE_ITEM(S3C64XX_GPFCON),
	SAVE_ITEM(S3C64XX_GPFDAT),
	SAVE_ITEM(S3C64XX_GPFPUD),
	SAVE_ITEM(S3C64XX_GPFCONSLP),
	SAVE_ITEM(S3C64XX_GPFPUDSLP),
	SAVE_ITEM(S3C64XX_GPGCON),
	SAVE_ITEM(S3C64XX_GPGDAT),
	SAVE_ITEM(S3C64XX_GPGPUD),
	SAVE_ITEM(S3C64XX_GPGCONSLP),
	SAVE_ITEM(S3C64XX_GPGPUDSLP),
	SAVE_ITEM(S3C64XX_GPHCON0),
	SAVE_ITEM(S3C64XX_GPHCON1),
	SAVE_ITEM(S3C64XX_GPHDAT),
	SAVE_ITEM(S3C64XX_GPHPUD),
	SAVE_ITEM(S3C64XX_GPHCONSLP),
	SAVE_ITEM(S3C64XX_GPHPUDSLP),
	SAVE_ITEM(S3C64XX_GPICON),
	SAVE_ITEM(S3C64XX_GPIDAT),
	SAVE_ITEM(S3C64XX_GPIPUD),
	SAVE_ITEM(S3C64XX_GPICONSLP),
	SAVE_ITEM(S3C64XX_GPIPUDSLP),
	SAVE_ITEM(S3C64XX_GPJCON),
	SAVE_ITEM(S3C64XX_GPJDAT),
	SAVE_ITEM(S3C64XX_GPJPUD),
	SAVE_ITEM(S3C64XX_GPJCONSLP),
	SAVE_ITEM(S3C64XX_GPJPUDSLP),
	SAVE_ITEM(S3C64XX_GPKCON),
	SAVE_ITEM(S3C64XX_GPKCON1),
	SAVE_ITEM(S3C64XX_GPKDAT),
	SAVE_ITEM(S3C64XX_GPKPUD),
	SAVE_ITEM(S3C64XX_GPLCON),
	SAVE_ITEM(S3C64XX_GPLCON1),
	SAVE_ITEM(S3C64XX_GPLDAT),
	SAVE_ITEM(S3C64XX_GPLPUD),
	SAVE_ITEM(S3C64XX_GPMCON),
	SAVE_ITEM(S3C64XX_GPMDAT),
	SAVE_ITEM(S3C64XX_GPMPUD),
	SAVE_ITEM(S3C64XX_GPNCON),
	SAVE_ITEM(S3C64XX_GPNDAT),
	SAVE_ITEM(S3C64XX_GPNPUD),
	SAVE_ITEM(S3C64XX_GPOCON),
	SAVE_ITEM(S3C64XX_GPODAT),
	SAVE_ITEM(S3C64XX_GPOPUD),
	SAVE_ITEM(S3C64XX_GPOCONSLP),
	SAVE_ITEM(S3C64XX_GPOPUDSLP),
	SAVE_ITEM(S3C64XX_GPPCON),
	SAVE_ITEM(S3C64XX_GPPDAT),
	SAVE_ITEM(S3C64XX_GPPPUD),
	SAVE_ITEM(S3C64XX_GPPCONSLP),
	SAVE_ITEM(S3C64XX_GPPPUDSLP),
	SAVE_ITEM(S3C64XX_GPQCON),
	SAVE_ITEM(S3C64XX_GPQDAT),
	SAVE_ITEM(S3C64XX_GPQPUD),
	SAVE_ITEM(S3C64XX_GPQCONSLP),
	SAVE_ITEM(S3C64XX_GPQPUDSLP),
	SAVE_ITEM(S3C64XX_PRIORITY),
	SAVE_ITEM(S3C64XX_SPCON),

	/* Special register */
	SAVE_ITEM(S3C64XX_SPC_BASE),
	SAVE_ITEM(S3C64XX_SPCONSLP),
	SAVE_ITEM(S3C64XX_SLPEN),
	SAVE_ITEM(S3C64XX_MEM0CONSTOP),
	SAVE_ITEM(S3C64XX_MEM1CONSTOP),
	SAVE_ITEM(S3C64XX_MEM0CONSLP0),
	SAVE_ITEM(S3C64XX_MEM0CONSLP1),
	SAVE_ITEM(S3C64XX_MEM1CONSLP),
	SAVE_ITEM(S3C64XX_MEM0DRVCON),
	SAVE_ITEM(S3C64XX_MEM1DRVCON),
};

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S3C64XX_VIC0INTSELECT),
	SAVE_ITEM(S3C64XX_VIC1INTSELECT),
	SAVE_ITEM(S3C64XX_VIC0INTENABLE),
	SAVE_ITEM(S3C64XX_VIC1INTENABLE),
	SAVE_ITEM(S3C64XX_VIC0SOFTINT),
	SAVE_ITEM(S3C64XX_VIC1SOFTINT),
};

static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S3C64XX_SROM_BW),
	SAVE_ITEM(S3C64XX_SROM_BC0),
	SAVE_ITEM(S3C64XX_SROM_BC1),
	SAVE_ITEM(S3C64XX_SROM_BC2),
	SAVE_ITEM(S3C64XX_SROM_BC3),
	SAVE_ITEM(S3C64XX_SROM_BC4),
	SAVE_ITEM(S3C64XX_SROM_BC5),
};

#ifdef CONFIG_S3C_PM_DEBUG

#define SAVE_UART(va) \
	SAVE_ITEM((va) + S3C_ULCON), \
	SAVE_ITEM((va) + S3C_UCON), \
	SAVE_ITEM((va) + S3C_UFCON), \
	SAVE_ITEM((va) + S3C_UMCON), \
	SAVE_ITEM((va) + S3C_UBRDIV)

static struct sleep_save uart_save[] = {
	SAVE_UART(S3C24XX_VA_UART0),
	SAVE_UART(S3C24XX_VA_UART1),
#ifndef CONFIG_CPU_S3C2400
	SAVE_UART(S3C24XX_VA_UART2),
#endif	/* CONFIG_CPU_S3C2400 */
};

/* debug
 *
 * we send the debug to printascii() to allow it to be seen if the
 * system never wakes up from the sleep
*/


extern void printascii(const char *);


void pm_dbg(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	printascii(buff);
}

static void s3c_pm_debug_init(void)
{
	unsigned long tmp = __raw_readl(S3C_CLKCON);

	/* re-start uart clocks */
	tmp |= S3C_CLKCON_UART0;
	tmp |= S3C_CLKCON_UART1;
	tmp |= S3C_CLKCON_UART2;

	__raw_writel(tmp, S3C_CLKCON);
	udelay(10);
}

#define DBG(fmt...) pm_dbg(fmt)
#else
#define DBG(fmt...)

#define s3c6410_pm_debug_init() do { } while(0)
#endif	/* CONFIG_S3C_PM_DEBUG */

#if defined(CONFIG_S3C_PM_CHECK) && CONFIG_S3C_PM_CHECK_CHUNKSIZE != 0

/* suspend checking code...
 *
 * this next area does a set of crc checks over all the installed
 * memory, so the system can verify if the resume was ok.
 *
 * CONFIG_S3C6410_PM_CHECK_CHUNKSIZE defines the block-size for the CRC,
 * increasing it will mean that the area corrupted will be less easy to spot,
 * and reducing the size will cause the CRC save area to grow
*/

#define CHECK_CHUNKSIZE (CONFIG_S3C_PM_CHECK_CHUNKSIZE * 1024)

static u32 crc_size;	/* size needed for the crc block */
static u32 *crcs;	/* allocated over suspend/resume */

typedef u32 *(run_fn_t)(struct resource *ptr, u32 *arg);

/* s3c6410_pm_run_res
 *
 * go thorugh the given resource list, and look for system ram
*/

static void s3c6410_pm_run_res(struct resource *ptr, run_fn_t fn, u32 *arg)
{
	while (ptr != NULL) {
		if (ptr->child != NULL)
			s3c6410_pm_run_res(ptr->child, fn, arg);

		if ((ptr->flags & IORESOURCE_MEM) &&
		    strcmp(ptr->name, "System RAM") == 0) {
			DBG("Found system RAM at %08lx..%08lx\n",
			    ptr->start, ptr->end);
			arg = (fn)(ptr, arg);
		}

		ptr = ptr->sibling;
	}
}

static void s3c6410_pm_run_sysram(run_fn_t fn, u32 *arg)
{
	s3c6410_pm_run_res(&iomem_resource, fn, arg);
}

static u32 *s3c6410_pm_countram(struct resource *res, u32 *val)
{
	u32 size = (u32)(res->end - res->start)+1;

	size += CHECK_CHUNKSIZE-1;
	size /= CHECK_CHUNKSIZE;

	DBG("Area %08lx..%08lx, %d blocks\n", res->start, res->end, size);

	*val += size * sizeof(u32);
	return val;
}

/* s3c6410_pm_prepare_check
 *
 * prepare the necessary information for creating the CRCs. This
 * must be done before the final save, as it will require memory
 * allocating, and thus touching bits of the kernel we do not
 * know about.
*/

static void s3c6410_pm_check_prepare(void)
{
	crc_size = 0;

	s3c6410_pm_run_sysram(s3c6410_pm_countram, &crc_size);

	DBG("s3c6410_pm_prepare_check: %u checks needed\n", crc_size);

	crcs = kmalloc(crc_size+4, GFP_KERNEL);
	if (crcs == NULL)
		printk(KERN_ERR "Cannot allocated CRC save area\n");
}

static u32 *s3c6410_pm_makecheck(struct resource *res, u32 *val)
{
	unsigned long addr, left;

	for (addr = res->start; addr < res->end;
	     addr += CHECK_CHUNKSIZE) {
		left = res->end - addr;

		if (left > CHECK_CHUNKSIZE)
			left = CHECK_CHUNKSIZE;

		*val = crc32_le(~0, phys_to_virt(addr), left);
		val++;
	}

	return val;
}

/* s3c6410_pm_check_store
 *
 * compute the CRC values for the memory blocks before the final
 * sleep.
*/

static void s3c6410_pm_check_store(void)
{
	if (crcs != NULL)
		s3c6410_pm_run_sysram(s3c6410_pm_makecheck, crcs);
}

/* in_region
 *
 * return TRUE if the area defined by ptr..ptr+size contatins the
 * what..what+whatsz
*/

static inline int in_region(void *ptr, int size, void *what, size_t whatsz)
{
	if ((what+whatsz) < ptr)
		return 0;

	if (what > (ptr+size))
		return 0;

	return 1;
}

static u32 *s3c6410_pm_runcheck(struct resource *res, u32 *val)
{
	void *save_at = phys_to_virt(s3c6410_sleep_save_phys);
	unsigned long addr;
	unsigned long left;
	void *ptr;
	u32 calc;

	for (addr = res->start; addr < res->end;
	     addr += CHECK_CHUNKSIZE) {
		left = res->end - addr;

		if (left > CHECK_CHUNKSIZE)
			left = CHECK_CHUNKSIZE;

		ptr = phys_to_virt(addr);

		if (in_region(ptr, left, crcs, crc_size)) {
			DBG("skipping %08lx, has crc block in\n", addr);
			goto skip_check;
		}

		if (in_region(ptr, left, save_at, 32*4 )) {
			DBG("skipping %08lx, has save block in\n", addr);
			goto skip_check;
		}

		/* calculate and check the checksum */

		calc = crc32_le(~0, ptr, left);
		if (calc != *val) {
			printk(KERN_ERR PFX "Restore CRC error at "
			       "%08lx (%08x vs %08x)\n", addr, calc, *val);

			DBG("Restore CRC error at %08lx (%08x vs %08x)\n",
			    addr, calc, *val);
		}

	skip_check:
		val++;
	}

	return val;
}

/* s3c6410_pm_check_restore
 *
 * check the CRCs after the restore event and free the memory used
 * to hold them
*/

static void s3c6410_pm_check_restore(void)
{
	if (crcs != NULL) {
		s3c6410_pm_run_sysram(s3c6410_pm_runcheck, crcs);
		kfree(crcs);
		crcs = NULL;
	}
}

#else

#define s3c6410_pm_check_prepare() do { } while(0)
#define s3c6410_pm_check_restore() do { } while(0)
#define s3c6410_pm_check_store()   do { } while(0)
#endif	/* defined(CONFIG_S3C_PM_CHECK) && CONFIG_S3C_PM_CHECK_CHUNKSIZE != 0 */

/* helper functions to save and restore register state */

void s3c6410_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		//DBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/* s3c6410_pm_do_restore
 *
 * restore the system from the given list of saved registers
 *
 * Note, we do not use DBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s3c6410_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		//printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       //ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s3c6410_pm_do_restore_core
 *
 * similar to s36410_pm_do_restore_core
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

/* s3c6410_pm_do_save_phy
 *
 * save register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

int s3c_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		printk(KERN_ERR "%s resource get error\n",__FUNCTION__);
		return 0;
	}
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		ptr->val = readl(target_reg + (ptr->reg));
	}

	return 0;
}

/* s3c6410_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

int s3c_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		printk(KERN_ERR "%s resource get error\n",__FUNCTION__);
		return 0;
	}
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		writel(ptr->val, (target_reg + ptr->reg));
	}

	return 0;
}

static void s3c6410_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		__raw_writel(ptr->val, ptr->reg);
	}
}

extern void s3c_config_wakeup_source(void);
extern void s3c_config_sleep_gpio(void);
extern void s3c_config_wakeup_gpio(void);

static unsigned int s3c_eint_mask_val;

static void s3c6410_pm_configure_extint(void)
{
	/* for each of the external interrupts (EINT0..EINT15) we
	 * need to check wether it is an external interrupt source,
	 * and then configure it as an input if it is not
	*/
	s3c_eint_mask_val = __raw_readl(S3C_EINT_MASK);

	s3c_config_wakeup_source();
}
/* Wakeup source */

#define WAKEUP_RTC		0
#define WAKEUP_OTHERS		1

static int get_wakeup_source(void)
{
	int wakeup_stat, eintpend;
#ifdef CONFIG_STOP_MODE_SUPPORT
	if (power_get_mode() == POWER_MODE_STOP)
	{
		wakeup_stat = __raw_readl(S3C_WAKEUP_STAT);
		eintpend = __raw_readl(S3C64XX_EINT0PEND);
		__raw_writel(wakeup_stat, S3C_WAKEUP_STAT);
		__raw_writel(eintpend, S3C64XX_EINT0PEND);
	}
	else
#endif
	{
		wakeup_stat = __raw_readl(S3C_INFORM1);
		eintpend = __raw_readl(S3C_INFORM2);
	}
	
	printk("eintpend = %x, wakeup_stat = %x\n", eintpend, wakeup_stat);
	
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		wake_lock(&usbinsert_wake_lock);	
	}
	if (wakeup_stat & 0x6)
	{
		return WAKEUP_RTC;
	}
	else
	{
		return WAKEUP_OTHERS;
	}
}
void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))

#ifdef CONFIG_STOP_MODE_SUPPORT
static void before_stop(void)
{
	#define IRQ_TYPE_EDGE_FALLING 0x02
	unsigned int val;
//	gpio_set_value(S3C64XX_GPC(7), 1);
//	gpio_set_value(S3C64XX_GPC(6), 1);
	gpio_set_value(S3C64XX_GPP(2), 0);
//	gpio_set_value(S3C64XX_GPP(9), 0);

/* Jack noted on 2010/11/06 for avoid GPP7 and GPP0 conflict */

//	val=__raw_readl(S3C64XX_GPPCON);
//	val&=~(0x3<<2*7);
//	__raw_writel(val|0x2<<2*7,S3C64XX_GPPCON);
//	val=__raw_readl(S3C64XX_GPPCON);
//	val&=~(0x3<<2*0);
//	__raw_writel(val|0x2<<2*0,S3C64XX_GPPCON);
//	val=__raw_readl(S3C64XX_MEM0CONSTOP)|0x3<<26;
//	__raw_writel(val,S3C64XX_MEM0CONSTOP);



//	val=__raw_readl(S3C64XX_GPNCON);
//	val&=~(0x3<<2*5);
//	__raw_writel(val | 0X2<<2*5,S3C64XX_GPNCON);
////	__raw_writel(val,S3C64XX_GPNCON);
//	
//	val=__raw_readl(S3C64XX_GPNPUD);
//	val&=~(0x3<<2*5);
//	__raw_writel(val|0x2<<2*5,S3C64XX_GPNPUD);//PULL UP enable

////Jack changed for morse button
//	s3c_gpio_cfgpin(S3C64XX_GPN(5), S3C_GPIO_SFN(2)); 
//	s3c_gpio_setpull(S3C64XX_GPN(5), S3C_GPIO_PULL_UP);
//	set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_FALLING);


#if 0
	val=__raw_readl(S3C64XX_GPPCON);
	val&=~(0x3<<2*7);
	__raw_writel(val,S3C64XX_GPPCON);
	mdelay(1);
	val=__raw_readl(S3C64XX_GPPPUD);
	val&=~(0x3<<2*7);
	__raw_writel(val|0x2<<2*7,S3C64XX_GPPPUD);
	//val=__raw_readl(S3C64XX_GPPDAT);
	//val&=~(0x1<<7);
	//__raw_writel(val,S3C64XX_GPPDAT);
	
	val=__raw_readl(S3C64XX_GPPCON);
	val&=~(0x3<<2*0);
	__raw_writel(val,S3C64XX_GPPCON);
	mdelay(1);
	val=__raw_readl(S3C64XX_GPPPUD);
	val&=~(0x3<<2*0);
	__raw_writel(val|0x2<<2*0,S3C64XX_GPPPUD);
	
	//val=__raw_readl(S3C64XX_GPPCON);
        //val&=~(0x3<<2*0);
        //__raw_writel(val|0x1<<2*0,S3C64XX_GPPCON);
        
	//val=__raw_readl(S3C64XX_GPPDAT);
        //val&=~(0x1<<0);
        //__raw_writel(val,S3C64XX_GPPDAT);
#endif


}

static void after_stop(void)
{
//	gpio_set_value(S3C64XX_GPC(7), 0);
//	gpio_set_value(S3C64XX_GPC(6), 0);
	gpio_set_value(S3C64XX_GPP(2), 1);
	if(gpio_get_value(S3C64XX_GPM(3)) == 0)
	{
		 gpio_set_value(S3C64XX_GPK(0), 1);
     s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
		 s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_UP);
	}
	else
	{
		 gpio_set_value(S3C64XX_GPK(0), 0);
     s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
		 s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);		
	}
}

extern bool get_sierra_module_status(void);
static void s3c6410_stop(void)
{
        unsigned long regs_save[16];
        unsigned int tmp;

        /* ensure the debug is initialised (if enabled) */
   //     printk("1: s3c6410_pm_enter(%d)\n", state);
//	printk("s3c6410_stop\n");
		 gpio_set_value(S3C64XX_GPK(0), 0);
	   s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
		 s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_DOWN);

	before_stop();
	
        if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
                printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
                return -EINVAL;
        }

        /* prepare check area if configured */
//        s3c6410_pm_check_prepare();

        /* store the physical address of the register recovery block */
//        s3c6410_sleep_save_phys = virt_to_phys(regs_save);

//        DBG("s3c6410_sleep_save_phys=0x%08lx\n", s3c6410_sleep_save_phys);

        /* save all necessary core registers not covered by the drivers */

//        s3c6410_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
//        s3c6410_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
//        s3c6410_pm_do_save(core_save, ARRAY_SIZE(core_save));
//        s3c6410_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));

        /* ensure INF_REG0  has the resume address */
//        __raw_writel(virt_to_phys(s3c6410_cpu_resume), S3C_INFORM0);

        /* set the irq configuration for wake */
        s3c6410_pm_configure_extint();

        /* call cpu specific preperation */

	/* Henry@2010-10-8 turn off HSDPA_PWREN */
       u32 gpiocfg;    
	if (get_sierra_module_status() == false)
		{
		    gpiocfg=__raw_readl(S3C64XX_GPKDAT);
		    gpiocfg &= ~(0x1<<11);		//gpk11
		   __raw_writel(gpiocfg, S3C64XX_GPKDAT);
		}
        pm_cpu_prep();

        /* flush cache back to ram */

//      flush_cache_all();

//        s3c6410_pm_check_store();

        /* send the cpu to sleep... */

        __raw_writel(1, S3C_OSC_STABLE);
      //__raw_writel(8, S3C_MTC_STABLE);
      //__raw_writel(8, S3C_PWR_STABLE);


//        printk(KERN_ALERT "1.S3C_NORMAL_CFG=%x\n",__raw_readl(S3C_NORMAL_CFG));

//        tmp=__raw_readl(S3C_NORMAL_CFG);
//      __raw_writel(tmp|0xff00, S3C_NORMAL_CFG);

        /* Set WFI instruction to STOP mode */
        tmp = __raw_readl(S3C_STOP_CFG);

//        tmp |= 0x20120103<<0;
//        tmp &=~(0x100100<<0);
        tmp |= 0x20120103<<0;
	/* Henry@2010-10-6 --> leave bit8 of STOP_CFG as 1 (TOP_LOGIC active mode)
	    Otherwise device will crash at STOP mode while 3g is running */
	if (get_sierra_module_status() == false)
        tmp &=~(0x100<<0);
        //tmp &=~(0x20120100<<0);
        __raw_writel(tmp, S3C_STOP_CFG);

        tmp = __raw_readl(S3C_PWR_CFG);
        tmp &= ~(0x60<<0);
        tmp |= (0x2<<5);
        __raw_writel(tmp, S3C_PWR_CFG);

//      tmp = __raw_readl(S3C_SLEEP_CFG);
//      tmp &= ~(0x61<<0);
//      __raw_writel(tmp, S3C_SLEEP_CFG);

//      __raw_writel(0x2, S3C64XX_SLPEN);
        /* Clear WAKEUP_STAT register for next wakeup -jc.lee */
        /* If this register do not be cleared, Wakeup will be failed */
//        tmp = __raw_readl(S3C_WAKEUP_STAT);
//        __raw_writel(tmp, S3C_WAKEUP_STAT);

        __raw_writel(0x3fffffe, S3C_MEM0_CLK_GATE);
//      __raw_writel(0xffffff00, S3C_NORMAL_CFG);
        /* s3c6410_cpu_save will also act as our return point from when
         * we resume as it saves its own register state, so use the return
         * code to differentiate return from save and return from sleep */

        if (s3c6410_cpu_save(regs_save) == 0) {
        //      flush_cache_all();
                pm_cpu_sleep();
        }
//         unsigned int val;
//        printk(KERN_ALERT "3.EINT0PEND=%x\n",__raw_readl(S3C64XX_EINT0PEND));
//        __raw_writel(0xffffff00, S3C_NORMAL_CFG);
        /* restore the cpu state */
        cpu_init();

//        printk(KERN_ALERT "2.S3C_NORMAL_CFG=%x\n",__raw_readl(S3C_NORMAL_CFG));
        /* restore the system state */
//        s3c6410_pm_do_restore_core(core_save, ARRAY_SIZE(core_save));
//        s3c6410_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
//        s3c6410_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
//        s3c6410_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));


	after_stop();
//        __raw_writel(0x0, S3C64XX_SLPEN);
//        set_sleep_flag(1);
//        wakeup_key=getwakeupsource();
//        printk("source is 0x%08x \n", wakeup_key);
//	tmp = __raw_readl(S3C64XX_EINT0PEND);
//        __raw_writel(tmp, S3C64XX_EINT0PEND);

//        DBG("post sleep, preparing to return\n");

        /* Clear WAKEUP_STAT register for next wakeup -jc.lee */
        /* If this register do not be cleared, Wakeup will be failed */
//        tmp = __raw_readl(S3C_WAKEUP_STAT);
//        __raw_writel(tmp, S3C_WAKEUP_STAT);

//        s3c6410_pm_check_restore();

        /* ok, let's return from sleep */
//        DBG("S3C6410 PM Resume (post-restore)\n");

//read XEINT5 for ioc : Jack added on 2010/10/18
//			tmp = __raw_readl(S3C64XX_EINT0PEND);
//			printk("%s , tmp = 0x%x \n", __FUNCTION__, tmp) ;
//			if(tmp & 0x20) //GPN5 = 1 , interrupt occur
//			{
//				printk("Morse key interrupt has occured .\n") ;
//			//	Morse_interrupt_flag = 1 ;
//				detect_morse(gp_ioc);
//			}
			
		//prevent wakeup lose morsekey
			if(!gpio_get_value(S3C64XX_GPN(5)))
			{
				printk("Morse key interrupt has occured .\n") ;
			//	Morse_interrupt_flag = 1 ;
				detect_morse(gp_ioc);//Jack noted on 2010/12/17
			}
		//	msleep(10) ;
			else
			{
				set_sleep_flag(1);
      	wakeup_key=getwakeupsource();
      	printk("else set_sleep_flag, and getwakeupsource.\n") ;
      }
        return 0;
}
#endif

/* s3c6410_pm_enter
 *
 * central control for sleep/resume process
*/

#define CONFIG_STOP_TIMEOUT		5
static int s3c6410_pm_enter(suspend_state_t state)
{
	unsigned long regs_save[16];
	unsigned int tmp;
	unsigned int wakeup_stat = 0x0;
	unsigned int eint0pend = 0x0;
	unsigned int val;
	if(nosuspend_routine_keycode_flag==1)
	{
	//printk("1.s3c6410_pm_enter\n");
	nosuspend_routine_keycode_flag=0;
       //set_sleep_flag(1); 		//Try: 1020
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		wake_lock(&usbinsert_wake_lock);	
	}
	return 0;
	}	



	if(suspend_routine_flag==1)	
	{
	//printk("2.s3c6410_pm_enter\n");
	suspend_routine_flag=0;
	//set_sleep_flag(1); 
	//printk("suspend course key event generate\n");
	if(gpio_get_value(S3C64XX_GPM(3)) == 1)
	{
		wake_lock(&usbinsert_wake_lock);	
	}
	return 0; 
	}
	gpio_set_value(S3C64XX_GPK(0), 0);
    s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
	s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_UP);
#ifdef CONFIG_STOP_MODE_SUPPORT
	static int stop_times = 0;

	if (power_get_mode() == POWER_MODE_STOP)
	{
		s3c6410_stop();
		get_wakeup_source();
/*
		if (get_wakeup_source() == WAKEUP_RTC)
		{
			stop_times++;
			if (stop_times >= CONFIG_STOP_TIMEOUT)
			{
				power_set_mode(POWER_MODE_SLEEP);
				stop_times = 0;
			}
		}
		else
		{
			stop_times = 0;
		}
*/
		
		return 0;
	}
#endif

	/* ensure the debug is initialised (if enabled) */

	printk("2:s3c6410_pm_enter(%d)\n", state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
		return -EINVAL;
	}

	/* prepare check area if configured */
	s3c6410_pm_check_prepare();

	/* store the physical address of the register recovery block */
	s3c6410_sleep_save_phys = virt_to_phys(regs_save);

	printk("s3c6410_sleep_save_phys=0x%08lx\n", s3c6410_sleep_save_phys);


		#if 0		//William: resolve can not use mouse-button wake up CPU
        val=__raw_readl(S3C64XX_GPNCON);
        val&=~(0x3<<2*5);
        __raw_writel(val,S3C64XX_GPNCON); //as input

        val=__raw_readl(S3C64XX_GPNPUD);
        val&=~(0x3<<2*5);
        __raw_writel(val|0x2<<2*5,S3C64XX_GPNPUD);//PULL UP enabled
		#endif


	s3c6410_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
	s3c6410_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	s3c6410_pm_do_save(core_save, ARRAY_SIZE(core_save));
	s3c6410_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));

	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s3c6410_cpu_resume), S3C_INFORM0);

	/* set the irq configuration for wake */
	s3c6410_pm_configure_extint();

	/* call cpu specific preperation */

	pm_cpu_prep();

	/* flush cache back to ram */

	flush_cache_all();

	s3c6410_pm_check_store();

	s3c_config_sleep_gpio();	

	tmp = __raw_readl(S3C64XX_SPCONSLP);
	tmp &= ~(0x3 << 12);
	__raw_writel(tmp | (0x1 << 12), S3C64XX_SPCONSLP);

	/* send the cpu to sleep... */

	__raw_writel(0xffffffff, S3C64XX_VIC0INTENCLEAR);
	__raw_writel(0xffffffff, S3C64XX_VIC1INTENCLEAR);
	__raw_writel(0xffffffff, S3C64XX_VIC0SOFTINTCLEAR);
	__raw_writel(0xffffffff, S3C64XX_VIC1SOFTINTCLEAR);

	/* Unmask clock gating and block power turn on */
	__raw_writel(0x43E00041, S3C_HCLK_GATE); 
	__raw_writel(0xF2040000, S3C_PCLK_GATE);
	__raw_writel(0x80000011, S3C_SCLK_GATE);
	__raw_writel(0x00000000, S3C_MEM0_CLK_GATE);

	__raw_writel(0x1, S3C_OSC_STABLE);
	__raw_writel(0x3, S3C_PWR_STABLE);

	/* Set WFI instruction to SLEEP mode */

	tmp = __raw_readl(S3C_PWR_CFG);
	tmp &= ~(0x3<<5);
	tmp |= (0x3<<5);
	__raw_writel(tmp, S3C_PWR_CFG);

	tmp = __raw_readl(S3C_SLEEP_CFG);
	tmp &= ~(0x61<<0);
	__raw_writel(tmp, S3C_SLEEP_CFG);

	__raw_writel(0x2, S3C64XX_SLPEN);

	/* Clear WAKEUP_STAT register for next wakeup -jc.lee */
	/* If this register do not be cleared, Wakeup will be failed */
	__raw_writel(__raw_readl(S3C_WAKEUP_STAT), S3C_WAKEUP_STAT);


#ifdef CONFIG_MACH_SMDK6410
	/* ALL sub block "ON" before enterring sleep mode - EVT0 bug*/
	__raw_writel(0xffffff00, S3C_NORMAL_CFG);

	/* Open all clock gate to enter sleep mode - EVT0 bug*/
	__raw_writel(0xffffffff, S3C_HCLK_GATE);
	__raw_writel(0xffffffff, S3C_PCLK_GATE);
	__raw_writel(0xffffffff, S3C_SCLK_GATE);

	// Need to check further !!!
	// S3C_MEM0_CLK_GATE [0] is DMC0.
	// Should be off when entering sleep mode.
	// Otherwise system might be stopped when entering sleep mode
	__raw_writel(0x3fffffe, S3C_MEM0_CLK_GATE);

	// Turn On All Block Block Power
    	/* ALL sub block "ON" before enterring sleep mode - EVT0 bug*/
	__raw_writel(0xffffff00, S3C_NORMAL_CFG);
#endif /* CONFIG_MACH_SMDK6410 */

	/* s3c6410_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

	if (s3c6410_cpu_save(regs_save) == 0) {
		flush_cache_all();
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	__raw_writel(s3c_eint_mask_val, S3C_EINT_MASK);

	/* restore the system state */
	s3c6410_pm_do_restore_core(core_save, ARRAY_SIZE(core_save));
	s3c6410_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
	s3c6410_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
	s3c6410_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
	gpio_set_value(S3C64XX_GPK(0), 1);
    s3c_gpio_cfgpin(S3C64XX_GPK(0),S3C64XX_GPK_OUTPUT(0));
	s3c_gpio_setpull(S3C64XX_GPK(0), S3C_GPIO_PULL_UP);
	__raw_writel(0x0, S3C64XX_SLPEN);
	mdelay(1);
	
	//Jack added for sleep Wakeup of ioc
//	tmp = __raw_readl(S3C_INFORM2);
//	printk("%s , tmp = 0x%x \n", __FUNCTION__, tmp) ;
//	
//	if(tmp & 0x20) //GPN5 = 1 , interrupt occur
//	{
//		printk("Morse key interrupt has occured .\n") ;
//		//	Morse_interrupt_flag = 1 ;
//		detect_morse(gp_ioc);
//	}
//	else
//	{
//		set_sleep_flag(1);
//		wakeup_key=getwakeupsource();
//	}
	
	set_sleep_flag(1);
	wakeup_key=getwakeupsource();
	printk("__%s__ , source is 0x%08x \n", __FUNCTION__, wakeup_key);
	wakeup_stat = __raw_readl(S3C_WAKEUP_STAT);
	eint0pend = __raw_readl(S3C64XX_EINT0PEND);

	__raw_writel(eint0pend, S3C64XX_EINT0PEND);

	printk("post sleep, preparing to return\n");

	s3c6410_pm_check_restore();

	pr_info("%s: WAKEUP_STAT(0x%08x), EINT0PEND(0x%08x)\n",
			__func__, wakeup_stat, eint0pend);

	s3c_config_wakeup_gpio();	
	get_wakeup_source();
#ifdef CONFIG_STOP_MODE_SUPPORT
/*
	if (get_wakeup_source() != WAKEUP_RTC)
	{
		power_set_mode(POWER_MODE_STOP);
	}
*/
#endif

	/* ok, let's return from sleep */
	printk("S3C6410 PM Resume (post-restore)\n");
	return 0;
}
EXPORT_SYMBOL(wakeup_key);
static struct platform_suspend_ops s3c6410_pm_ops = {
	.enter		= s3c6410_pm_enter,
	.valid		= suspend_valid_only_mem,
};

/* s3c6410_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s3c6410_pm_init(void)
{
	printk("S3C6410 Power Management, (c) 2008 Samsung Electronics\n");

	suspend_set_ops(&s3c6410_pm_ops);
#ifdef CONFIG_STOP_MODE_SUPPORT
	power_set_mode(POWER_MODE_STOP);
#endif

#ifdef CONFIG_S3C64XX_DOMAIN_GATING
	s3c_init_domain_power();
#endif /* CONFIG_S3C64XX_DOMAIN_GATING */
#ifndef CONFIG_MACH_SMDK6410
	/* set to zero value unused H/W IPs clock */
	__raw_writel(S3C_HCLK_MASK, S3C_HCLK_GATE);
	__raw_writel(S3C_SCLK_MASK, S3C_SCLK_GATE);
	__raw_writel(S3C_PCLK_MASK, S3C_PCLK_GATE);
	/* enable onenand0, onenand1 memory clock */
	__raw_writel(0x18, S3C_MEM0_CLK_GATE);
#endif /* CONFIG_MACH_SMDK6410 */

	return 0;
}
