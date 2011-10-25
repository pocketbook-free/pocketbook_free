#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

#include <linux/args_shared_mem/args_shared_mem.h>

static unsigned char *args_mem;

static int version_proc_ex3_show(struct seq_file *m, void *v)
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
	
	seq_printf(m, linux_proc_banner_ex3, cdata->ex3_uboot_version);
	iounmap(args_mem);
	kfree(cdata);

	return 0;
}

static int version_proc_ex3_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_proc_ex3_show, NULL);
}

static const struct file_operations version_proc_ex3_fops = {
	.open		= version_proc_ex3_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_version_ex3_init(void)
{
	proc_create("EX3version", 0, NULL, &version_proc_ex3_fops);
	return 0;
}
module_init(proc_version_ex3_init);
