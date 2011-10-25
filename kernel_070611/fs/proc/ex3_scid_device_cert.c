#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>
#include <asm/uaccess.h>

#include <linux/args_shared_mem/args_shared_mem.h>

static unsigned char *args_mem;

static int scid_device_cert_proc_ex3_show(struct seq_file *m, void *v)
{
	return 0;
}

static int scid_device_cert_proc_ex3_open(struct inode *inode, struct file *file)
{
	return single_open(file, scid_device_cert_proc_ex3_show, NULL);
}

ssize_t scid_device_cert_proc_ex3_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	struct movi_args_t *cdata;
	unsigned char *pMme=NULL;
	int i;

	cdata = (struct movi_args_t *)kmalloc(sizeof(struct movi_args_t), GFP_KERNEL);
	memset(cdata, 0, sizeof(struct movi_args_t));
	args_mem = ioremap(ARGS_SHARED_MEM_BASE, ARGS_SHARED_MEM_SIZE);
	pMme = cdata;
	for (i=0 ; i<sizeof(struct movi_args_t) ; i++)
		pMme[i] = readb(args_mem+i);

	if (copy_to_user(buf, &cdata->device_cert, 5120)) {
		printk("copy to user fail\n");
		return -ENODEV;
	}
	iounmap(args_mem);
	kfree(cdata);
	return 0;
}

static const struct file_operations scid_device_cert_proc_ex3_fops = {
	.open		= scid_device_cert_proc_ex3_open,
	.read		= scid_device_cert_proc_ex3_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_scid_device_cert_ex3_init(void)
{
	proc_create("DeviceCert", 0, NULL, &scid_device_cert_proc_ex3_fops);
	return 0;
}
module_init(proc_scid_device_cert_ex3_init);
