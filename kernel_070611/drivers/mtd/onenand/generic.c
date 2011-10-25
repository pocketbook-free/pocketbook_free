/*
 *  linux/drivers/mtd/onenand/generic.c
 *
 *  Copyright (c) 2005 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This is a device driver for the OneNAND flash for generic boards.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
//#include <linux/mtd/onenand.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/mach/flash.h>
#include <linux/err.h>
#include <linux/clk.h> 
#include "s3c_onenand.h"
#define DRIVER_NAME	"onenand"


#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "cmdlinepart", NULL,  };
#endif

struct onenand_info {
	struct mtd_info		mtd;
	struct mtd_partition	*parts;
	struct onenand_chip	onenand;
};
struct mtd_info *s3c_onenand_interface;//Add by James

static int __devinit generic_onenand_probe(struct platform_device *pdev)
{
	struct onenand_info *info;
	struct flash_platform_data *pdata = pdev->dev.platform_data;
	struct resource *res = pdev->resource;
	unsigned long size = res->end - res->start + 1;
	int err, i;
	s3c_onenand_interface = kmalloc((sizeof(struct mtd_info)),GFP_KERNEL);//Add by James
	info = kzalloc(sizeof(struct onenand_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (!request_mem_region(res->start, size, pdev->name)) {
		err = -EBUSY;
		goto out_free_info;
	}

	info->onenand.base = ioremap(res->start, size);
	if (!info->onenand.base) {
		err = -ENOMEM;
		goto out_release_mem_region;
	}
	
	printk(KERN_INFO "OneNAND: generic_onenand_probe - (Virt) Base Address: 0x%08X\n", info->onenand.base);
	//info->onenand.mmcontrol = pdata->mmcontrol;
	info->onenand.irq = platform_get_irq(pdev, 0);

	info->mtd.name = pdev->dev.bus_id;
	info->mtd.priv = &info->onenand;
	info->mtd.owner = THIS_MODULE;

	if (onenand_scan(&info->mtd, 1)) {
		err = -ENXIO;
		goto out_iounmap;
	}

	//printk("the s3c_nand name is %s\n", (*s3c_mtd).name);
	//////////////////////////Add by James/////////////////////////////////////
 	printk("*************start write onenand*********************\n"); 
 	size_t retlen = 256*1024;
	u_char *buf1 = kmalloc((sizeof(u_char) * 262144),GFP_KERNEL);
	u_char *buf2 = "abcdefghijklmn";
	u_char *buf3 = kmalloc((sizeof(u_char) * 262144),GFP_KERNEL);
	//*buf1 = 0;
	printk("the info->mtd->size is %ld ...\n", (info->mtd).size);
	printk("the info->mtd->name is %s ...\n", (info->mtd).name);
	onenand_read(&info->mtd, 0, 256*1024, &retlen, buf1);
	for(i=0; i<100; i++)
	{
		printk("0x%02x     ", buf1[i]);
	}
/* 	onenand_write(&info->mtd, 262144, 256*1024, &retlen, buf2);
	onenand_read(&info->mtd, 262144, 256*1024, &retlen, buf3);

	for(i=0; i<100; i++)
	{
		printk("0x%02x     ", buf3[i]);
	} */
		
	printk("*************end read onenand*********************\n");  	

	memcpy(s3c_onenand_interface, &info->mtd, sizeof(struct mtd_info));//init global struct
	//////////////////////////////////////////////////////////////////////
#ifdef CONFIG_MTD_PARTITIONS
	err = parse_mtd_partitions(&info->mtd, part_probes, &info->parts, 0);
	if (err > 0)
		add_mtd_partitions(&info->mtd, info->parts, err);
	else if (err <= 0 && pdata->parts)
		add_mtd_partitions(&info->mtd, pdata->parts, pdata->nr_parts);
	else
#endif
		err = add_mtd_device(&info->mtd);

	dev_set_drvdata(&pdev->dev, info);

	return 0;

out_iounmap:
	iounmap(info->onenand.base);
out_release_mem_region:
	release_mem_region(res->start, size);
out_free_info:
	kfree(info);

	return err;
}

/* PM Support */
#ifdef CONFIG_PM
static int generic_onenand_suspend(struct platform_device *pdev, pm_message_t pm)
{
	struct onenand_info *info = dev_get_drvdata(&pdev->dev);
	struct mtd_info *mtd = &info->mtd;

	/* Execute device specific suspend code */
	mtd->suspend(mtd);

	return 0;
}

static int generic_onenand_resume(struct platform_device *pdev)
{
	struct onenand_info *info = dev_get_drvdata(&pdev->dev);
	struct mtd_info *mtd = &info->mtd;

	/* Execute device specific resume code */
	mtd->resume(mtd);

	return 0;
}
#else
#define generic_onenand_suspend		NULL
#define generic_onenand_resume		NULL
#endif

static int __devexit generic_onenand_remove(struct platform_device *pdev)
{
	struct onenand_info *info = dev_get_drvdata(&pdev->dev);
	struct resource *res = pdev->resource;
	unsigned long size = res->end - res->start + 1;

	dev_set_drvdata(&pdev->dev, NULL);

	if (info) {
		if (info->parts)
			del_mtd_partitions(&info->mtd);
		else
			del_mtd_device(&info->mtd);

		onenand_release(&info->mtd);
		release_mem_region(res->start, size);
		iounmap(info->onenand.base);
		kfree(info);
	}

	return 0;
}

static struct platform_driver generic_onenand_driver = {
	.probe		= generic_onenand_probe,
	.remove		= __devexit_p(generic_onenand_remove),
	.suspend	= generic_onenand_suspend,
	.resume		= generic_onenand_resume,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

MODULE_ALIAS(DRIVER_NAME);

static int __init generic_onenand_init(void)
{
#ifdef CONFIG_CPU_S5PC100
	printk(KERN_INFO "S5PC100 OneNAND Driver.\n");
#endif

	return platform_driver_register(&generic_onenand_driver);
}

static void __exit generic_onenand_exit(void)
{
	platform_driver_unregister(&generic_onenand_driver);
}

module_init(generic_onenand_init);
module_exit(generic_onenand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_DESCRIPTION("Glue layer for OneNAND flash on generic boards");

