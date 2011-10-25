/* linux/arch/arm/plat-s3c/include/plat/partition.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Partition information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <asm/mach/flash.h>

struct mtd_partition s3c_partition_info[] = {
#if defined(CONFIG_SPLIT_ROOT_FOR_ANDROID)
        {
                .name		= "U-Boot",
                .offset		= 0,		/* for bootloader */
                .size		= (1024*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "ParameterLogo",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (4*SZ_1M),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "Kernel",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (4*SZ_1M),
        },
        {
                .name		= "RamDisk",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (1*SZ_1M),
        },
        {
                .name		= "Kernel-Backup",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (4*SZ_1M),
        },
        {
                .name		= "RamDisk-Backup",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (1*SZ_1M),
        },
	{
                .name		= "System",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (110*SZ_1M),
        },
        {
                .name		= "UserData",
                .offset		= MTDPART_OFS_APPEND,
                .size		= (110*SZ_1M),
        },
        {
                .name		= "Cache",
                .offset		= MTDPART_OFS_APPEND,
                .size		= MTDPART_SIZ_FULL,
        },
#else
        {
                .name		= "Bootloader",
                .offset		= 0,
                .size		= (256*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "Kernel",
                .offset		= (256*SZ_1K),
                .size		= (4*SZ_1M) - (256*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
#if defined(CONFIG_SPLIT_ROOT_FILESYSTEM)
        {
                .name		= "Rootfs",
                .offset		= (4*SZ_1M),
                .size		= (48*SZ_1M),
        },
#endif
        {
                .name		= "File System",
                .offset		= MTDPART_OFS_APPEND,
                .size		= MTDPART_SIZ_FULL,
        }
#endif
};

struct s3c_nand_mtd_info s3c_nand_mtd_part_info = {
	.chip_nr = 1,
	.mtd_part_nr = ARRAY_SIZE(s3c_partition_info),
	.partition = s3c_partition_info,
};

struct flash_platform_data s3c_onenand_data = {
	.parts		= s3c_partition_info,
	.nr_parts	= ARRAY_SIZE(s3c_partition_info),
};
