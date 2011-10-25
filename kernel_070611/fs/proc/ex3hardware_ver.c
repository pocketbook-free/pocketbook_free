#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/args_shared_mem/args_shared_mem.h>

static unsigned char *args_mem;

static int boardversion_proc_ex3_show(struct seq_file *m, void *v)
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
	
	switch(cdata->board_id)
	{ 
		default:	
		case BOARD_VERSION_UNKNOWN:
			seq_printf(m, "BoardVer:%03X\n", BOARD_VERSION_UNKNOWN);
			break;
			
		case BOARD_ID_EVT:
			seq_printf(m, "BoardVer:%03X\n", BOARD_ID_EVT);
			break;
			
		case BOARD_ID_DVT:
			seq_printf(m, "BoardVer:%03X\n", BOARD_ID_DVT);
			break;
	}
	iounmap(args_mem);
	kfree(cdata);

	return 0;
}

static int boardversion_proc_ex3_open(struct inode *inode, struct file *file)
{
	return single_open(file, boardversion_proc_ex3_show, NULL);
}

static const struct file_operations boardversion_proc_ex3_fops = {
	.open		= boardversion_proc_ex3_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_boardversion_ex3_init(void)
{
	proc_create("EX3BoardVer", 0, NULL, &boardversion_proc_ex3_fops);
	return 0;
}
module_init(proc_boardversion_ex3_init);
