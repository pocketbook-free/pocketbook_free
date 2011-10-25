#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

#include <linux/args_shared_mem/args_shared_mem.h>

static struct class *args_shared_mem_class;

static unsigned char *args_mem;

struct args_data
{
	struct movi_args_t movi_data;
	int minor;
};

static int args_open(struct inode *inode, struct file *filp)
{
	int devno, i;
	struct args_data *cdata;
	unsigned char *pMme=NULL;

	devno = inode->i_rdev;
	cdata = (struct args_data *)kmalloc(sizeof(struct args_data), GFP_KERNEL);
	if (cdata == NULL)
		goto fail1;

	memset(cdata, 0, sizeof(struct args_data));
	cdata->minor = devno;
	args_mem = ioremap(ARGS_SHARED_MEM_BASE, ARGS_SHARED_MEM_SIZE);

	pMme = (unsigned char *)&cdata->movi_data;
	for (i=0 ; i<sizeof(struct movi_args_t) ; i++)
		pMme[i] = readb(args_mem+i);

#if 0
	printk("cdata->movi_high_capcity          0x%X\n", cdata->movi_data.tcminfo.movi_high_capacity);
	printk("cdata->movi_data.movi_total_blkcnt     %u\n", cdata->movi_data.tcminfo.movi_total_blkcnt);
	printk("cdata->movi_data.init_partitions       %u\n", cdata->movi_data.ofsinfo.init_partitions);
	printk("cdata->movi_data.ofsinfo.update_kernel %u\n", cdata->movi_data.ofsinfo.update_kernel);
	printk("cdata->movi_data.ofsinfo.update_rootfs %u\n", cdata->movi_data.ofsinfo.update_rootfs);
	printk("cdata->movi_data.ofsinfo.rootfs        %u\n", cdata->movi_data.ofsinfo.rootfs);
	printk("cdata->movi_data.ofsinfo.kernel        %u\n", cdata->movi_data.ofsinfo.kernel);
	printk("cdata->movi_data.ofsinfo.bl2           %u\n", cdata->movi_data.ofsinfo.bl2);
	printk("cdata->movi_data.ofsinfo.bl1           %u\n", cdata->movi_data.ofsinfo.bl1);
	printk("cdata->movi_data.ofsinfo.env           %u\n", cdata->movi_data.ofsinfo.env);
	printk("cdata->movi_data.ofsinfo.last          %u\n", cdata->movi_data.ofsinfo.last);
	printk("cdata->movi_data.epd_logo              %u\n", cdata->movi_data.ofsinfo.epd_logo);
	printk("cdata->movi_data.update_epd_logo              %u\n", cdata->movi_data.ofsinfo.update_epd_logo);
#endif

	filp->private_data = cdata;

	return 0;
fail1:
	return -EFAULT;
}

static ssize_t args_read(struct file *filp, char __user *user,size_t size,loff_t *offp)
{
	struct args_data *cdata = (struct args_data *)filp->private_data;

	if (copy_to_user(user, cdata, size)) {
		printk("copy_to_user fail\n");
		goto fail2;
	}
	return 0;
	
fail2:
	return -EFAULT;

}

static int args_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct args_data *cdata = (struct args_data *)filp->private_data;
	void __user *argp = (void __user *)arg;

	switch(cmd) {
		case ARGSIO_GET_MOVI_OFFSET:
			return copy_to_user(argp, &cdata->movi_data.ofsinfo, sizeof(cdata->movi_data.ofsinfo)) ? -EFAULT : 0;
		case ARGSIO_GET_MOVI_TCM_REG:
			return copy_to_user(argp, &cdata->movi_data.tcminfo, sizeof(cdata->movi_data.tcminfo)) ? -EFAULT : 0;
	}
}

static int args_release(struct inode *inode, struct file *filp)
{
	struct args_data *cdata = (struct args_data *)filp->private_data;	

	iounmap(args_mem);
	kfree(cdata);

	return 0;
}

struct file_operations args_fops = {
	.owner		= THIS_MODULE,
	.open           = args_open,
        .read           = args_read,
	.ioctl          = args_ioctl,	
	.release	= args_release,
};

static int __init args_shared_mem_init(void)
{
	int n;
	
	printk(KERN_NOTICE "args_shared_mem_init\n");
	n = register_chrdev (ARGS_MAJOR, ARGS_DEVICE_NAME, &args_fops);
	if (n < 0) { 
		printk(KERN_ERR "args: can't register char device\n");
		return n;
	}
	args_shared_mem_class = class_create(THIS_MODULE, "args_shared_mem");
	device_create(args_shared_mem_class, NULL, MKDEV(ARGS_MAJOR, 0), NULL, ARGS_DEVICE_NAME);
	
	return 0;
}

static void __exit args_shared_mem_exit(void)
{
	printk(KERN_NOTICE "args_share_mem_exit\n");
	device_destroy(args_shared_mem_class, MKDEV(ARGS_MAJOR, 0));
	class_destroy(args_shared_mem_class);
	unregister_chrdev (ARGS_MAJOR, ARGS_DEVICE_NAME);
}

module_init(args_shared_mem_init);
module_exit(args_shared_mem_exit);
MODULE_LICENSE("GPL");

