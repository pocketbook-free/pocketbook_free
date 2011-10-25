/*
 *  linux/drivers/mtd/onenand/s3c_onenand.c
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Dong Jin PARK <djpax,park@samsung.com>
 *
 *  Credits:
 *  Jinsung Yang <jsgood.yang@samsung.com>:
 *  Original OneNAND driver for Denali controller
 *  Copyright (C) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>	// for udelay

#include <linux/time.h>
#include <linux/timer.h>

#include <asm/io.h>
#include <plat/regs-clock.h>
#include <mach/map.h>

#include "s3c_onenand.h"

#undef CONFIG_MTD_ONENAND_CHECK_SPEED
#define DEBUG_ONENAND

#ifdef CONFIG_MTD_ONENAND_CHECK_SPEED
#include <asm/arch/regs-gpio.h>
#endif

#ifdef DEBUG_ONENAND
#define dbg(x...)       printk(x)
#else
#define dbg(x...)       do { } while (0)
#endif

#define MULTI_BASE

#if !defined(MULTI_BASE)
#if defined(CONFIG_CPU_S3C6400)
#define onenand_phys_to_virt(x)		(void __iomem *)(this->dev_base + (x & 0xffffff))
#define onenand_virt_to_phys(x)		(void __iomem *)(ONENAND_AHB_ADDR + (x & 0xffffff))
#elif defined(CONFIG_CPU_S5PC100)
#define onenand_phys_to_virt(x)		(void __iomem *)(this->dev_base + (x & 0xfffffff))
#define onenand_virt_to_phys(x)		(void __iomem *)(ONENAND_AHB_ADDR + (x & 0xfffffff))
#else
#define onenand_phys_to_virt(x)		(void __iomem *)(this->dev_base + (x & 0x3ffffff))
#define onenand_virt_to_phys(x)		(void __iomem *)(ONENAND_AHB_ADDR + (x & 0x3ffffff))
#endif
#else
static unsigned int gBase00 = 0;
static unsigned int gBase01 = 0;
static unsigned int gBase10 = 0;
static unsigned int gBase11 = 0;

/* For save&restore the onenand register on suspend&resume */
static struct onenand_registers ondreg;
static struct timer_list ondtimer;
#define OND_TIMER_VALUE	(1 * HZ)

static void __iomem*  onenand_phys_to_virt(void __iomem* input)
{
#ifdef CONFIG_CPU_S5PC100
	unsigned int map = ((unsigned int)input & 0xC000000) >> 26;
	unsigned int addr = (unsigned int)input & 0x3FFFFFF;
#else
	unsigned int map = ((unsigned int)input & 0x3000000) >> 24;
	unsigned int addr = (unsigned int)input & 0xFFFFFF;
#endif
	if (map == 0x3)		// Map11
		return (void __iomem*)(gBase00 + addr);
	if (map == 0x2)		// Map10
		return (void __iomem*)(gBase10 + addr);
	if (map == 0x1)		// Map01
		return (void __iomem*)(gBase01 + addr);
	return (void __iomem*)(gBase00 + addr);		// Map00
}

static void __iomem*  onenand_virt_to_phys(void __iomem* input)
{
#ifdef CONFIG_CPU_S5PC100
	unsigned int map = ((unsigned int)input & 0xC000000) >> 26;
	unsigned int addr = (unsigned int)input & 0x3FFFFFF;

	if (map == 0x3)		// Map11
		return (void __iomem*)(ONENAND_AHB_ADDR + 0xC000000 + addr);
	if (map == 0x2)		// Map10
		return (void __iomem*)(ONENAND_AHB_ADDR + 0x8000000 + addr);
	if (map == 0x1)		// Map01
		return (void __iomem*)(ONENAND_AHB_ADDR + 0x4000000 + addr);
	return (void __iomem*)(ONENAND_AHB_ADDR + addr);	// Map00
#else
	unsigned int map = ((unsigned int)input & 0x3000000) >> 24;
	unsigned int addr = (unsigned int)input & 0xFFFFFF;
	
	if (map == 0x3)		// Map11
		return (void __iomem*)(ONENAND_AHB_ADDR + 0x3000000 + addr);
	if (map == 0x2)		// Map10
		return (void __iomem*)(ONENAND_AHB_ADDR + 0x2000000 + addr);
	if (map == 0x1)		// Map01
		return (void __iomem*)(ONENAND_AHB_ADDR + 0x1000000 + addr);
	return (void __iomem*)(ONENAND_AHB_ADDR + addr);	// Map00
#endif
}
#endif

/**
 *  onenand_oob_128 - oob info for Flex-Onenand with 4KB page
 *  For now, we expose only 64 out of 80 ecc bytes
 */
static struct nand_ecclayout onenand_oob_128 = {
	.eccbytes	= 64,
	.eccpos		= {
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		102, 103, 104, 105
		},
	.oobfree	= {
		{2, 4}, {18, 4}, {34, 4}, {50, 4},
		{66, 4}, {82, 4}, {98, 4}, {114, 4}
	}
};

/**
 * onenand_oob_64 - oob info for large (2KB) page
 */
static struct nand_ecclayout onenand_oob_64 = {
	.eccbytes	= 20,
	.eccpos		= {
		8, 9, 10, 11, 12,
		24, 25, 26, 27, 28,
		40, 41, 42, 43, 44,
		56, 57, 58, 59, 60,
		},

	/* For YAFFS2 tag information */
	.oobfree	= {
		{2, 6}, {13, 3}, {18, 6}, {29, 3},
		{34, 6}, {45, 3}, {50, 6}, {61, 3}}
#if 0
	.oobfree	= {
		{2, 6}, {14, 2}, {18, 6}, {30, 2},
		{34, 6}, {46, 2}, {50, 6}}

	.oobfree	= {
		{2, 3}, {14, 2}, {18, 3}, {30, 2},
		{34, 3}, {46, 2}, {50, 3}, {62, 2}
	}
#endif
};




/**
 * onenand_oob_32 - oob info for middle (1KB) page
 */
static struct nand_ecclayout onenand_oob_32 = {
	.eccbytes	= 10,
	.eccpos		= {
		8, 9, 10, 11, 12,
		24, 25, 26, 27, 28,
		},
	.oobfree	= { {2, 3}, {14, 2}, {18, 3}, {30, 2} }
};

static const unsigned char ffchars[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 16 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 32 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 48 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 64 */
};

//static int onenand_write_oob_nolock(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
static int onenand_verify_oob(struct mtd_info *mtd, const u_char *buf, loff_t to, size_t len);
//static int onenand_verify_ops(struct mtd_info *mtd, struct mtd_oob_ops *ops, loff_t to, size_t len);
static int onenand_verify_ops(struct mtd_info *mtd, const u_char *buf, const u_char *oobbuf, loff_t to);
#endif

/**
 * dma client
 */
static struct s3c2410_dma_client s3c6400onenand_dma_client = {
	.name		= "s3c6400-onenand-dma",
};

static unsigned int onenand_readl(void __iomem *addr)
{
	return readl(addr);
}

static void onenand_writel(unsigned int value, void __iomem *addr)
{
	writel(value, addr);
}

static void onenand_irq_wait(struct onenand_chip *this, int stat)
{
	while (!this->read(this->base + ONENAND_REG_INT_ERR_STAT) & stat);
}

static void onenand_irq_ack(struct onenand_chip *this, int stat)
{
	this->write(stat, this->base + ONENAND_REG_INT_ERR_ACK);
}

static int onenand_irq_pend(struct onenand_chip *this, int stat)
{
	return (this->read(this->base + ONENAND_REG_INT_ERR_STAT) & stat);
}

static void onenand_irq_wait_ack(struct onenand_chip *this, int stat)
{
	onenand_irq_wait(this, stat);
	onenand_irq_ack(this, stat);
}

static int onenand_blkrw_complete(struct onenand_chip *this, int cmd)
{
	int cmp_bit = 0, fail_bit = 0, ret = 0;

	if (cmd == ONENAND_CMD_READ) {
		cmp_bit = ONENAND_INT_ERR_LOAD_CMP;
		fail_bit = ONENAND_INT_ERR_LD_FAIL_ECC_ERR;
	} else if (cmd == ONENAND_CMD_PROG) {
		cmp_bit = ONENAND_INT_ERR_PGM_CMP;
		fail_bit = ONENAND_INT_ERR_PGM_FAIL;
	} else {
		ret = 1;
	}

	onenand_irq_wait_ack(this, ONENAND_INT_ERR_INT_ACT);
	onenand_irq_wait_ack(this, ONENAND_INT_ERR_BLK_RW_CMP);
	onenand_irq_wait_ack(this, cmp_bit);

	if (onenand_irq_pend(this, fail_bit)) {
		onenand_irq_ack(this, fail_bit);
		ret = 1;
	}

	return ret;
}

/**
 * onenand_read_burst
 *
 * 16 Burst read: performance is improved up to 40%.
 */
static void onenand_read_burst(void *dest, const void *src, size_t len)
{
#if 0
	int count;

	if ((len & 0xF) != 0)
		return;

	count = len >> 4;

	// It seems that the below assembly code does not working.
	__asm__ __volatile__(
		"	stmdb	r13!, {r0-r3,r9-r12}\n"
		"	mov	r2, %0\n"
		"1:\n"
		"	ldmia	r1, {r9-r12}\n"
		"	stmia	r0!, {r9-r12}\n"
		"	subs	r2, r2, #0x1\n"
		"	bne	1b\n"
		"	ldmia	r13!, {r0-r3,r9-r12}\n" ::"r" (count));
#else
	int count;
	unsigned int *D = dest;
	volatile unsigned int *S = src;

	if ((len & 0xF) != 0)
		return;

	count = len >> 4;

	for (; count > 0; count--)
	{
		D[0] = *S;
		D[1] = *S;
		D[2] = *S;
		D[3] = *S;
		D += 4;
	}
#endif
}

//#undef ONENAND_WRITE_BURST
#define ONENAND_WRITE_BURST
#ifdef ONENAND_WRITE_BURST
/**
 * onenand_write_burst
 *
 * 16 Burst write: performance may be improved.
 */
static void onenand_write_burst(void *dest, const void *src, size_t len)
{
#if 0
	int count;

	if ((len & 0xF) != 0)
		return;

	count = len >> 4;

	__asm__ __volatile__(
		"	stmdb	r13!, {r0-r3,r9-r12}\n"
		"	mov	r2, %0\n"
		"1:\n"
		"	ldmia	r1!, {r9-r12}\n"
		"	stmia	r0, {r9-r12}\n"
		"	subs	r2, r2, #0x1\n"
		"	bne	1b\n"
		"	ldmia	r13!, {r0-r3,r9-r12}\n"::"r" (count));
#else
	int count;
	volatile unsigned int *D = dest;
	unsigned int *S = src;

	if ((len & 0xF) != 0)
		return;

	count = len >> 4;

	for (; count > 0; count--)
	{
		*D = S[0];
		*D = S[1];
		*D = S[2];
		*D = S[3];
		S += 4;
	}
#endif
}
#endif

/**
 *
 * onenand_dma_finish
 *
 */
void onenand_dma_finish(struct s3c2410_dma_chan *dma_ch, void *buf_id,
        int size, enum s3c2410_dma_buffresult result)
{
	struct onenand_chip *this = (struct onenand_chip *) buf_id;
	complete(this->done);
}

/**
 *
 * onenand_read_dma
 *
 */
#if 1
static void onenand_read_dma(struct onenand_chip *this, unsigned int *dst, void __iomem *src, size_t len)
{
	void __iomem *phys_addr = onenand_virt_to_phys((uint) src);

	DECLARE_COMPLETION_ONSTACK(complete);
	this->done = &complete;

#if 0
	if (s3c_dma_request(this->dma, this->dma_ch, &s3c6400onenand_dma_client, NULL)) {
		printk(KERN_WARNING "Unable to get DMA channel.\n");
		return;
	}

	s3c_dma_set_buffdone_fn(this->dma, this->dma_ch, onenand_dma_finish);
	s3c_dma_devconfig(this->dma, this->dma_ch, S3C_DMA_MEM2MEM, 1, 0, (u_long) phys_addr);
	s3c_dma_config(this->dma, this->dma_ch, ONENAND_DMA_TRANSFER_WORD, ONENAND_DMA_TRANSFER_WORD);
	s3c_dma_setflags(this->dma, this->dma_ch, S3C2410_DMAF_AUTOSTART);
	consistent_sync((void *) dst, len, DMA_FROM_DEVICE);
	s3c_dma_enqueue(this->dma, this->dma_ch, (void *) this, (dma_addr_t) virt_to_dma(NULL, dst), len);

	wait_for_completion(&complete);

	s3c_dma_free(this->dma, this->dma_ch, &s3c6400onenand_dma_client);
#else
	if (s3c2410_dma_request(DMACH_ONENAND_IN, &s3c6400onenand_dma_client, NULL)) {
		printk(KERN_WARNING "Unable to get DMA channel.\n");
		return;
	}

	s3c2410_dma_set_buffdone_fn(DMACH_ONENAND_IN, onenand_dma_finish);
	s3c2410_dma_devconfig(DMACH_ONENAND_IN, S3C_DMA_MEM2MEM_P, 1, (u_long) phys_addr);
	s3c2410_dma_config(DMACH_ONENAND_IN, ONENAND_DMA_TRANSFER_WORD, ONENAND_DMA_TRANSFER_WORD);
	s3c2410_dma_setflags(DMACH_ONENAND_IN, S3C2410_DMAF_AUTOSTART);
	dma_cache_maint((void *) dst, len, DMA_FROM_DEVICE);
	s3c2410_dma_enqueue(DMACH_ONENAND_IN, (void *) this, (dma_addr_t) virt_to_dma(NULL, dst), len);

	wait_for_completion(&complete);

	s3c2410_dma_free(DMACH_ONENAND_IN, &s3c6400onenand_dma_client);
#endif
}
#else
extern s3c_dma_mainchan_t s3c_mainchans[S3C_DMA_CONTROLLERS];
extern void s3c_enable_dmac(unsigned int channel);
extern void s3c_disable_dmac(unsigned int channel);
static void onenand_read_dma(struct onenand_chip *this, unsigned int *dst, void __iomem *src, size_t len)
{
	s3c_dma_mainchan_t *mainchan = &s3c_mainchans[this->dma];
	void __iomem *phys_addr = onenand_virt_to_phys((uint) src);

	s3c_enable_dmac(this->dma);
	s3c_dma_set_buffdone_fn(this->dma, this->dma_ch, onenand_dma_finish);
	s3c_dma_devconfig(this->dma, this->dma_ch, S3C_DMA_MEM2MEM_P, 1, 0, (u_long) phys_addr);
	s3c_dma_config(this->dma, this->dma_ch, ONENAND_DMA_TRANSFER_WORD, ONENAND_DMA_TRANSFER_WORD);
	s3c_dma_setflags(this->dma, this->dma_ch, S3C_DMAF_AUTOSTART);
	consistent_sync((void *) dst, len, DMA_FROM_DEVICE);
	s3c_dma_enqueue(this->dma, this->dma_ch, (void *) this, (dma_addr_t) virt_to_dma(NULL, dst), len);

	while (readl(mainchan->regs + S3C_DMAC_ENBLD_CHANNELS) & (1 << this->dma_ch));

	s3c_dma_ctrl(this->dma, this->dma_ch, S3C_DMAOP_STOP);
	s3c_disable_dmac(this->dma);
}
#endif

/**
 * onenand_command_map - [DEFAULT] Get command type
 * @param cmd		command
 * @return		command type (00, 01, 10, 11)
 *
 */
static int onenand_command_map(int cmd)
{
	int type = ONENAND_CMD_MAP_FF;

	switch (cmd) {
	case ONENAND_CMD_READ:
	case ONENAND_CMD_READOOB:
	case ONENAND_CMD_PROG:
	case ONENAND_CMD_PROGOOB:
		type = ONENAND_CMD_MAP_01;
		break;

	case ONENAND_CMD_UNLOCK:
	case ONENAND_CMD_LOCK:
	case ONENAND_CMD_LOCK_TIGHT:
	case ONENAND_CMD_UNLOCK_ALL:
	case ONENAND_CMD_ERASE:
	case ONENAND_CMD_OTP_ACCESS:
	case ONENAND_CMD_PIPELINE_READ:
	case ONENAND_CMD_PIPELINE_WRITE:
	//case ONENAND_CMD_READOOB:
	//case ONENAND_CMD_PROGOOB:
		type = ONENAND_CMD_MAP_10;
		break;

	case ONENAND_CMD_RESET:
	case ONENAND_CMD_READID:
		printk("OneNAND : MAP11 Command\n");
		type = ONENAND_CMD_MAP_11;
		break;
	default:
		printk("OneNAND : %d Command is not supported\n",cmd);
		type = ONENAND_CMD_MAP_FF;
		break;
	}

	return type;
}

/**
 * onenand_addr_field - [DEFAULT] Generate address field
 * @param dev_id	device id
 * @param fba		block number
 * @param fpa		page number
 * @param fsa		sector number
 * @return		address field
 *
 * Refer to Table 7-1 MEM_ADDR Fields in S5PC100 User's Manual
 */
static uint onenand_addr_field(int dev_id, int fba, int fpa, int fsa)
{
	uint mem_addr = 0;
	int ddp, density;

	ddp = dev_id & ONENAND_DEVICE_IS_DDP;
	density = dev_id >> ONENAND_DEVICE_DENSITY_SHIFT;

	switch (density & 0xf) {
	case ONENAND_DEVICE_DENSITY_128Mb:
		mem_addr = (((fba & ONENAND_FBA_MASK_128Mb) << ONENAND_FBA_SHIFT) | \
				((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
				(fsa << ONENAND_FSA_SHIFT));
		break;

	case ONENAND_DEVICE_DENSITY_256Mb:
		mem_addr = (((fba & ONENAND_FBA_MASK_256Mb) << ONENAND_FBA_SHIFT) | \
				((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
				(fsa << ONENAND_FSA_SHIFT));
		break;

	case ONENAND_DEVICE_DENSITY_512Mb:
		mem_addr = (((fba & ONENAND_FBA_MASK_512Mb) << ONENAND_FBA_SHIFT) | \
				((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
				((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		break;

	case ONENAND_DEVICE_DENSITY_1Gb:
		if (ddp) {
			if(fba > ONENAND_FBA_MASK_1Gb_DDP)
				ddp = 1;
			else
				ddp = 0;

			mem_addr = ((ddp << ONENAND_DDP_SHIFT_1Gb) | \
					((fba & ONENAND_FBA_MASK_1Gb_DDP) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		} else {
			mem_addr = (((fba & ONENAND_FBA_MASK_1Gb) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		}

		break;

	case ONENAND_DEVICE_DENSITY_2Gb:
		if (ddp) {
			if(fba > ONENAND_FBA_MASK_4Gb_DDP)
				ddp = 1;
			else
				ddp = 0;

			mem_addr = ((ddp << ONENAND_DDP_SHIFT_2Gb) | \
					((fba & ONENAND_FBA_MASK_2Gb_DDP) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		} else {
			mem_addr = (((fba & ONENAND_FBA_MASK_2Gb) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		}

		break;

	case ONENAND_DEVICE_DENSITY_4Gb:
		if (ddp) {
			if(fba > ONENAND_FBA_MASK_4Gb_DDP)
				ddp = 1;
			else
				ddp = 0;

			mem_addr = ((ddp << ONENAND_DDP_SHIFT_4Gb) | \
					((fba & ONENAND_FBA_MASK_4Gb_DDP) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		} else {
			mem_addr = (((fba & ONENAND_FBA_MASK_4Gb) << ONENAND_FBA_SHIFT) | \
					((fpa & ONENAND_FPA_MASK) << ONENAND_FPA_SHIFT) | \
					((fsa & ONENAND_FSA_MASK) << ONENAND_FSA_SHIFT));
		}

		break;
	}

	return mem_addr;
}

/**
 * onenand_command_address - [DEFAULT] Generate command address
 * @param mtd		MTD device structure
 * @param cmd_type	command type
 * @param fba		block number
 * @param fpa		page number
 * @param fsa		sector number
 * @param onenand_addr	onenand device address to access directly (command 00/11)
 * @return		command address
 *
 * Refer to 'Command Mapping' in S5PC100 User's Manual
 */
static uint onenand_command_address(struct mtd_info *mtd, int cmd_type, int fba, int fpa, int fsa, int onenand_addr)
{
	struct onenand_chip *this = mtd->priv;
	uint cmd_addr = (ONENAND_AHB_ADDR | (cmd_type << ONENAND_CMD_SHIFT));
	int dev_id;

	dev_id = this->read(this->base + ONENAND_REG_DEVICE_ID);

	switch (cmd_type) {
	case ONENAND_CMD_MAP_00:
		cmd_addr |= ((onenand_addr & 0xffff) << 1);
		break;

	case ONENAND_CMD_MAP_01:
	case ONENAND_CMD_MAP_10:
		cmd_addr |= (onenand_addr_field(dev_id, fba, fpa, fsa) & ONENAND_MEM_ADDR_MASK);
		break;

	case ONENAND_CMD_MAP_11:
		cmd_addr |= ((onenand_addr & 0xffff) << 2);
		break;

	default:
		cmd_addr = 0;
		break;
	}

	return cmd_addr;
}

/**
 * onenand_command - [DEFAULT] Generate command address
 * @param mtd		MTD device structure
 * @param cmd		command
 * @param addr		onenand device address
 * @return		command address
 *
 */
static uint onenand_command(struct mtd_info *mtd, int cmd, loff_t addr)
{
	struct onenand_chip *this = mtd->priv;
	int sectors = 4;//, onenand_addr = -1;
	int cmd_type, fba = 0, fpa = 0, fsa = 0, page = 0;
	uint cmd_addr;

	cmd_type = onenand_command_map(cmd);

	switch (cmd) {
	case ONENAND_CMD_UNLOCK:
	case ONENAND_CMD_UNLOCK_ALL:
	case ONENAND_CMD_LOCK:
	case ONENAND_CMD_LOCK_TIGHT:
	case ONENAND_CMD_ERASE:
		fba = (int) (addr >> this->erase_shift);
		page = -1;
		break;

	default:
		fba = (int) (addr >> this->erase_shift);
		page = (int) (addr >> this->page_shift);
		page &= this->page_mask;
		fpa = page & ONENAND_FPA_MASK;
		fsa = sectors & ONENAND_FSA_MASK;
		break;
	}

	//cmd_addr = onenand_command_address(mtd, cmd_type, fba, fpa, fsa, onenand_addr);
	cmd_addr = onenand_command_address(mtd, cmd_type, fba, fpa, fsa, addr);		// djpark

	if (!cmd_addr) {
		printk(KERN_ERR "OneNAND: Command address mapping failed\n");
		return -1;
	}

	return cmd_addr;
}

/**
 * onenand_get_device - [GENERIC] Get chip for selected access
 * @param mtd		MTD device structure
 * @param new_state	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
int onenand_get_device(struct mtd_info *mtd, int new_state)
{
	struct onenand_chip *this = mtd->priv;
	DECLARE_WAITQUEUE(wait, current);

	/*
	 * Grab the lock and see if the device is available
	 */
	while (1) {
		spin_lock(&this->chip_lock);
		if (this->state == FL_READY) {
			this->state = new_state;
			spin_unlock(&this->chip_lock);
			break;
		}
		if (new_state == FL_PM_SUSPENDED) {
			spin_unlock(&this->chip_lock);
			return (this->state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
		}

		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&this->wq, &wait);
		spin_unlock(&this->chip_lock);
		schedule();
		remove_wait_queue(&this->wq, &wait);
	}
	
	return 0;
}

/**
 * onenand_release_device - [GENERIC] release chip
 * @param mtd		MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void onenand_release_device(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;

	/* Release the chip */
	spin_lock(&this->chip_lock);
	this->state = FL_READY;
	wake_up(&this->wq);
	spin_unlock(&this->chip_lock);
}

/**
 * onenand_block_isbad_nolock - [GENERIC] Check if a block is marked bad
 * @param mtd		MTD device structure
 * @param ofs		offset from device start
 * @param allowbbt	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int onenand_block_isbad_nolock(struct mtd_info *mtd, loff_t ofs, int allowbbt)
{
	struct onenand_chip *this = mtd->priv;

	if (this->options & ONENAND_CHECK_BAD) {
		struct bbm_info *bbm = this->bbm;

		/* Return info from the table */
		return bbm->isbad_bbt(mtd, ofs, allowbbt);
	} else
		return 0;
}

/**
 * onenand_set_pipeline - [MTD Interface] Set pipeline ahead
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 *
 */
static int onenand_set_pipeline(struct mtd_info *mtd, loff_t from, size_t len)
{
	struct onenand_chip *this = mtd->priv;
	int page_cnt = (int) (len >> this->page_shift);
	void __iomem *cmd_addr;

	if (len % mtd->writesize > 0)
		page_cnt++;

	if (page_cnt > 1) {
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_PIPELINE_READ, from));
		this->write(ONENAND_DATAIN_PIPELINE_READ | page_cnt, cmd_addr);
	}

	return 0;
}

/**
 * onenand_read - [MTD Interface] Read data from flash
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 *
 */
int onenand_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct onenand_chip *this = mtd->priv;
	struct mtd_ecc_stats stats;
	int i, ret = 0, read = 0, col, thislen;
	uint *buf_poi = (uint *) this->page_buf;
	void __iomem *cmd_addr;

	//DEBUG(MTD_DEBUG_LEVEL3, "onenand_read: from = 0x%08x, len = %i, col = %i\n", (unsigned int) from, (int) len, (int) col);
	//printk("onenand_read: from = 0x%08x, len = %i, col = %i\n", (unsigned int) from, (int) len, (int) col);
	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_read: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}
	//printk("@@@@@@@@@@@@@@@@@@@@1@@@@@@@@@@@@@\n");
	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_READING);
	/* TODO handling oob */
	stats = mtd->ecc_stats;
	/* column (start offset to read) */
	col = (int)(from & (mtd->writesize - 1));

	if (this->options & ONENAND_PIPELINE_AHEAD)
		onenand_set_pipeline(mtd, from, len);
#ifdef CONFIG_MTD_ONENAND_CHECK_SPEED
	if (len > 100000) {
		writel((readl(S3C_GPNCON) & ~(0x3 << 30)) | (0x1 << 30), S3C_GPNCON);
		writel(1 << 15, S3C_GPNDAT);
	}
#endif
 	//while (!ret) {
	while (1) {
		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, from, 0)) {
				printk (KERN_WARNING "\nonenand_read: skipped to read from a bad block at addr 0x%08x.\n", (unsigned int) from);
				from += (1 << this->erase_shift);

				if (col != 0)
					col = (int)(from & (mtd->writesize - 1));

				continue;
			}
		}
		/* get start address to read data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, from));
		//cmd_addr = phys_to_virt(this->command(mtd, ONENAND_CMD_READ, from));
		switch (this->options & ONENAND_READ_MASK) {
		case ONENAND_READ_BURST:
			onenand_read_burst(buf_poi, cmd_addr, mtd->writesize);
			break;

		case ONENAND_READ_DMA:
			onenand_read_dma(this, buf_poi, cmd_addr, mtd->writesize);
			break;

		case ONENAND_READ_POLLING:
			for (i = 0; i < (mtd->writesize / 4); i++)
				*buf_poi++ = this->read(cmd_addr);

			buf_poi -= (mtd->writesize / 4);
			break;

		default:
			printk(KERN_ERR "onenand_read: read mode undefined.\n");
			return -1;
		}

		if (onenand_blkrw_complete(this, ONENAND_CMD_READ)) {
			printk(KERN_WARNING "onenand_read: Read operation failed:0x%08x.\n", (unsigned int)from);
#if 0
			return -1;
#endif
		}

		thislen = min_t(int, mtd->writesize - col, len - read);
		memcpy(buf, this->page_buf + col, thislen);

		read += thislen;

		if (read == len)
 			break;

		buf += thislen;
		from += mtd->writesize;
		col = 0;
 	}
	//printk("@@@@@@@@@@@@@@@@@@@@8@@@@@@@@@@@@@\n");

#ifdef CONFIG_MTD_ONENAND_CHECK_SPEED
	if (len > 100000) {
		printk("len: %d\n", len);
		writel(0 << 15, S3C_GPNDAT);
	}
#endif

	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = read;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	if (ret)
		return ret;

	return mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}

/**
 * onenand_transfer_auto_oob - [Internal] oob auto-placement transfer
 * @param mtd		MTD device structure
 * @param buf		destination address
 * @param column	oob offset to read from
 * @param thislen	oob length to read
 */
static int onenand_transfer_auto_oob(struct mtd_info *mtd, uint8_t *buf, int column,
				int thislen)
{
	struct onenand_chip *this = mtd->priv;
	struct nand_oobfree *free;
	int readcol = column;
	int readend = column + thislen;
	int lastgap = 0;
	unsigned int i;
	uint8_t *oob_buf = this->oob_buf;

	free = this->ecclayout->oobfree;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free->length; i++, free++) {
		if (readcol >= lastgap)
			readcol += free->offset - lastgap;
		if (readend >= lastgap)
			readend += free->offset - lastgap;
		lastgap = free->offset + free->length;
	}
	free = this->ecclayout->oobfree;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free->length; i++, free++) {
		int free_end = free->offset + free->length;
		if (free->offset < readend && free_end > readcol) {
			int st = max_t(int,free->offset,readcol);
			int ed = min_t(int,free_end,readend);
			int n = ed - st;
			memcpy(buf, oob_buf + st, n);
			buf += n;
		} else if (column == 0)
			break;
	}
	return 0;
}

/**
 * onenand_read_ops_nolock - [OneNAND Interface] OneNAND read main and/or out-of-band
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param ops:		oob operation description structure
 *
 * OneNAND read main and/or out-of-band data
 */
static int onenand_read_ops_nolock(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct onenand_chip *this = mtd->priv;
	struct mtd_ecc_stats stats;
	int read = 0, thislen, column, oobsize;
	int i;
	void __iomem *cmd_addr;
	uint *buf_poi, *dbuf_poi;

	int len = ops->ooblen;
	u_char *buf = ops->datbuf;
	u_char *sparebuf = ops->oobbuf;
	DEBUG(MTD_DEBUG_LEVEL3, "onenand_read_ops_nolock: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Initialize return length value */
	ops->retlen = 0;
	ops->oobretlen = 0;

	if (ops->mode == MTD_OOB_AUTO)
		oobsize = this->ecclayout->oobavail;
	else
		oobsize = mtd->oobsize;
	
	column = from & (mtd->oobsize - 1);

	if (unlikely(column >= oobsize)) {
		printk(KERN_ERR "onenand_read_ops_nolock: Attempted to start read outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		column + len > ((mtd->size >> this->page_shift) -
			(from >> this->page_shift)) * oobsize)) {
		printk(KERN_ERR "onenand_read_ops_nolock: Attempted to read beyond end of device\n");
		return -EINVAL;
	}

	dbuf_poi = (uint *)buf;

	if (this->options & ONENAND_PIPELINE_AHEAD)
		onenand_set_pipeline(mtd, from, len);
	
	/* on the TRANSFER SPARE bit */
	this->write(ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	//stats = mtd->ecc_stats;
	memcpy(&stats, &mtd->ecc_stats, sizeof(mtd->ecc_stats));
	while (read < len) {
#if 0
		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, from, 0)) {
				printk (KERN_WARNING "onenand_read_ops_nolock: skipped to read oob from a bad block at addr 0x%08x.\n", (unsigned int) from);
				from += (1 << this->erase_shift);

				if (column != 0)
					column = from & (mtd->oobsize - 1);

				continue;
			}
		}
#endif
		/* get start address to read data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, from));

		thislen = oobsize - column;
		thislen = min_t(int, thislen, len);
		
		buf_poi = (uint *)this->oob_buf;

		switch (this->options & ONENAND_READ_MASK) {
		case ONENAND_READ_BURST:
			onenand_read_burst(dbuf_poi, cmd_addr, mtd->writesize);
			onenand_read_burst(buf_poi, cmd_addr, mtd->oobsize);
			break;

		case ONENAND_READ_DMA:
			onenand_read_dma(this, dbuf_poi, cmd_addr, mtd->writesize);
			onenand_read_dma(this, buf_poi, cmd_addr, mtd->oobsize);
			break;

		case ONENAND_READ_POLLING:
			/* read main data and throw into garbage box */
			for (i = 0; i < (mtd->writesize / 4); i++)
				*dbuf_poi = this->read(cmd_addr);

			/* read spare data */
			for (i = 0; i < (mtd->oobsize / 4); i++)
				*buf_poi++ = this->read(cmd_addr);

			break;
		}

		if (onenand_blkrw_complete(this, ONENAND_CMD_READ)) {
			printk(KERN_WARNING "onenand_read_ops_nolock: Read operation failed:0x%x\n", (unsigned int)from);
			//printk("ECC_ERR_STAT : 0x%x",readl(this->base+ONENAND_REG_ECC_ERR_STAT));
			static int reread_count = 0;
			if(readl(this->base+ONENAND_REG_ECC_ERR_STAT) != 0) {
				if(reread_count <= 3) {
					reread_count++;
					continue;
				} else {
					reread_count = 0;
				}
				
			}
#if 0
			this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);
			return -1;
#endif
		}
		
		if (ops->mode == MTD_OOB_AUTO)
			onenand_transfer_auto_oob(mtd, sparebuf, column, thislen);

		read += thislen;

		if (read == len)
			break;

		buf += thislen;

		/* Read more? */
		if (read < len) {
			/* Page size */
			from += mtd->writesize;
			column = 0;
		}
	}

	/* off the TRANSFER SPARE bit */
	this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	ops->retlen = mtd->writesize;
	ops->oobretlen = read;
	
	if (mtd->ecc_stats.failed - stats.failed) {
		printk("ecc fail~~!!\n");
		return -EBADMSG;
	}

	return mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}


/**
 * onenand_read_oob_nolock - [MTD Interface] OneNAND read out-of-band
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param ops:		oob operation description structure
 *
 * OneNAND read out-of-band data from the spare area
 */
static int onenand_read_oob_nolock(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct onenand_chip *this = mtd->priv;
	struct mtd_ecc_stats stats;
	int read = 0, thislen, column, oobsize;
	int i, ret = 0;
	void __iomem *cmd_addr;
	uint *buf_poi;

	size_t len = ops->ooblen;
	mtd_oob_mode_t mode = ops->mode;
	u_char *buf = ops->oobbuf;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_read_oob_nolock: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Initialize return length value */
	ops->oobretlen = 0;

	if (mode == MTD_OOB_AUTO)
		oobsize = this->ecclayout->oobavail;
	else
		oobsize = mtd->oobsize;

	column = from & (mtd->oobsize - 1);

	if (unlikely(column >= oobsize)) {
		printk(KERN_ERR "onenand_read_oob_nolock: Attempted to start read outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		column + len > ((mtd->size >> this->page_shift) -
				(from >> this->page_shift)) * oobsize)) {
		printk(KERN_ERR "onenand_read_oob_nolock: Attempted to read beyond end of device\n");
		return -EINVAL;
	}

	if (this->options & ONENAND_PIPELINE_AHEAD)
		onenand_set_pipeline(mtd, from, len);

	/* on the TRANSFER SPARE bit */
	this->write(ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

#if	defined(CONFIG_CPU_S5PC100)
	/* setting for read oob area only */
	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READOOB, from));

	this->write(ONENAND_DATAIN_ACCESS_SPARE, cmd_addr);
#endif /* CONFIG_CPU_S5PC100 */

	//stats = mtd->ecc_stats;
	memcpy(&stats, &mtd->ecc_stats, sizeof(mtd->ecc_stats));
	while (read < len) {
#if 0
		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, from, 0)) {
				printk (KERN_WARNING "\nonenand_do_read_oob: skipped to read oob from a bad block at addr 0x%08x.\n", (unsigned int) from);
				from += (1 << this->erase_shift);

				if (column != 0)
					column = from & (mtd->oobsize - 1);

				continue;
			}
		}
#endif

		/* get start address to read data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, from));

		thislen = oobsize - column;
		thislen = min_t(int, thislen, len);

		if (mode == MTD_OOB_AUTO)
			buf_poi = (uint *)this->oob_buf;
		else
			buf_poi = (uint *)buf;

		switch (this->options & ONENAND_READ_MASK) {
		case ONENAND_READ_BURST:
#if	!defined(CONFIG_CPU_S5PC100)
			onenand_read_burst(buf_poi, cmd_addr, mtd->writesize);
#endif /* CONFIG_CPU_S5PC100 */
			onenand_read_burst(buf_poi, cmd_addr, mtd->oobsize);
			break;

		case ONENAND_READ_DMA:
#if	!defined(CONFIG_CPU_S5PC100)
			onenand_read_dma(this, buf_poi, cmd_addr, mtd->writesize);
#endif /* CONFIG_CPU_S5PC100 */
			onenand_read_dma(this, buf_poi, cmd_addr, mtd->oobsize);
			break;

		case ONENAND_READ_POLLING:
#if	!defined(CONFIG_CPU_S5PC100)
			/* read main data and throw into garbage box */
			for (i = 0; i < (mtd->writesize / 4); i++)
				*buf_poi = this->read(cmd_addr);
#endif /* CONFIG_CPU_S5PC100 */
			/* read spare data */
			for (i = 0; i < (mtd->oobsize / 4); i++)
				*buf_poi++ = this->read(cmd_addr);

			break;
		}


		if (onenand_blkrw_complete(this, ONENAND_CMD_READ)) {
			printk(KERN_WARNING "onenand_read_oob_nolock: Read operation failed:0x%x\n", (unsigned int)from);
#if 0
			this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);
			return -1;
#endif
		}

		if (mode == MTD_OOB_AUTO)
			onenand_transfer_auto_oob(mtd, buf, column, thislen);

		read += thislen;

		if (read == len)
			break;

		buf += thislen;

		/* Read more? */
		if (read < len) {
			/* Page size */
			from += mtd->writesize;
			column = 0;
		}
	}

#if	defined(CONFIG_CPU_S5PC100)
	/* setting for read oob area only */
	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READOOB, from));
	this->write(ONENAND_DATAIN_ACCESS_MAIN, cmd_addr);
#endif /* CONFIG_CPU_S5PC100 */

	/* off the TRANSFER SPARE bit */
	this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	ops->oobretlen = read;

	if (ret)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return 0;
}

/**
 * onenand_read_oob - [MTD Interface] NAND write data and/or out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob operation description structure
 */
static int onenand_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int ret;

	switch (ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
		break;
	case MTD_OOB_RAW:
		/* Not implemented yet */
	default:
		printk("Don't support this read OOB_MODE!!!\n");
		return -EINVAL;
	}

	onenand_get_device(mtd, FL_READING);
	if (ops->datbuf)
		ret = onenand_read_ops_nolock(mtd, from, ops);
	else
		ret = onenand_read_oob_nolock(mtd, from, ops);
	onenand_release_device(mtd);

	return ret;
}

/**
 * onenand_bbt_read_oob - [MTD Interface] OneNAND read out-of-band for bbt scan
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param ops		oob operation description structure
 *
 * OneNAND read out-of-band data from the spare area for bbt scan
 */
int onenand_bbt_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *cmd_addr;
	int i;
	size_t len = ops->ooblen;
	u_char *buf = ops->oobbuf;
	uint bbinfo;
	u_char *bb_poi = (u_char *)&bbinfo;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_bbt_read_oob: from = 0x%08x, len = %zi\n", (unsigned int) from, len);

	/* Initialize return value */
	ops->oobretlen = 0;

	/* Do not allow reads past end of device */
	if (unlikely((from + len) > mtd->size)) {
		printk(KERN_ERR "onenand_bbt_read_oob: Attempt read beyond end of device\n");
		return ONENAND_BBT_READ_FATAL_ERROR;
	}

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_READING);

	/* on the TRANSFER SPARE bit */
	this->write(ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	/* get start address to read data */
	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, from));
	
	switch (this->options & ONENAND_READ_MASK) {
	case ONENAND_READ_BURST:
		onenand_read_burst((uint *)this->page_buf, cmd_addr, mtd->writesize);
		onenand_read_burst((uint *)this->oob_buf, cmd_addr, mtd->oobsize);
		buf[0] = this->oob_buf[0];
		buf[1] = this->oob_buf[1];
		break;

	case ONENAND_READ_DMA:
		onenand_read_dma(this, (uint *)this->page_buf, cmd_addr, mtd->writesize);
		onenand_read_dma(this, (uint *)this->oob_buf, cmd_addr, mtd->oobsize);
		buf[0] = this->oob_buf[0];
		buf[1] = this->oob_buf[1];
		break;

	case ONENAND_READ_POLLING:
		/* read main data and throw into garbage box */
		for (i = 0; i < (mtd->writesize / 4); i++)
			this->read(cmd_addr);

		/* read first 4 bytes of spare data */
		bbinfo = this->read(cmd_addr);
		buf[0] = bb_poi[0];
		buf[1] = bb_poi[1];

		for (i = 0; i < (mtd->oobsize / 4) - 1; i++)
			this->read(cmd_addr);
		break;
	}

	onenand_blkrw_complete(this, ONENAND_CMD_READ);

	/* off the TRANSFER SPARE bit */
	this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	ops->oobretlen = len;
	return 0;
}

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
/**
 * onenand_verify_page - [GENERIC] verify the chip contents after a write
 * @param mtd		MTD device structure
 * @param buf		the databuffer to verify
 * @param addr		address to read
 * @return		0, if ok
 */
static int onenand_verify_page(struct mtd_info *mtd, const uint *buf, loff_t addr)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *cmd_addr;
	int i, ret = 0;
	uint *written = (uint *)kmalloc(mtd->writesize, GFP_KERNEL);

	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, addr));
	
	/* write all data of 1 page by 4 bytes at a time */
	for (i = 0; i < (mtd->writesize / 4); i++) {
		*written = this->read(cmd_addr);
		written++;
	}

	written -= (mtd->writesize / 4);

	/* Check, if data is same */
	if (memcmp(written, buf, mtd->writesize))
		ret = -EBADMSG;

	kfree(written);

	return ret;
}

/**
 * onenand_verify_oob - [GENERIC] verify the oob contents after a write
 * @param mtd		MTD device structure
 * @param buf		the databuffer to verify
 * @param to		offset to read from
 *
 */
static int onenand_verify_oob(struct mtd_info *mtd, const u_char *buf, loff_t to, size_t len)
{
	struct onenand_chip *this = mtd->priv;
	char oobbuf[128];
	uint *dbuf_poi;
	uint *buf_poi;
	int read = 0, thislen, column, oobsize, i;
	void __iomem *cmd_addr;
	mtd_oob_mode_t	mode = MTD_OOB_AUTO;

	if (mode == MTD_OOB_AUTO)
		oobsize = this->ecclayout->oobavail;
	else
		oobsize = mtd->oobsize;

	column = to & (mtd->oobsize - 1);
	
	/* get start address to read data */
	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, to));
	
	thislen = oobsize - column;
	thislen = min_t(int, thislen, oobsize);

	dbuf_poi = (uint *)this->page_buf;
	if (mode == MTD_OOB_AUTO)
		buf_poi = (uint *)this->oob_buf;
	else
		buf_poi = (uint *)oobbuf;
	
	onenand_read_burst(dbuf_poi, cmd_addr, mtd->writesize);
	onenand_read_burst(buf_poi, cmd_addr, mtd->oobsize);

	if (onenand_blkrw_complete(this, ONENAND_CMD_READ)) {
		printk(KERN_WARNING "onenand_verify_oob: Read operation failed:0x%x\n", (unsigned int)to);
	}

	if (mode == MTD_OOB_AUTO)
		onenand_transfer_auto_oob(mtd, oobbuf, column, thislen);

	for (i = 0; i < oobsize; i++)
		if (buf[i] != oobbuf[i])
			return -EBADMSG;

	return 0;
}

/**
 * onenand_verify_ops - [GENERIC] verify the oob contents after a write
 * @param mtd		MTD device structure
 * @param buf		data buffer to verify
 * @param oobbuf 	spareram buffer to verify
 * @param to		offset to read from
 * @param mode		read mode
 *
 */
static int onenand_verify_ops(struct mtd_info *mtd, const u_char *buf, const u_char *oobbuf, loff_t to)
{
	struct onenand_chip *this = mtd->priv;
	char local_oobbuf[128];
	int oobsize;
	void __iomem *cmd_addr;
	int ret = 0, i;

	/* get start address to read data */
	cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, to));

	onenand_read_burst((uint *)this->page_buf, cmd_addr, mtd->writesize);
	onenand_read_burst((uint *)local_oobbuf, cmd_addr, mtd->oobsize);

	if (onenand_blkrw_complete(this, ONENAND_CMD_READ)) {
		printk(KERN_WARNING "onenand_verify_ops: Read operation failed:0x%x\n", (unsigned int)to);
		return -EBADMSG;
	}

	/* Check, if data is same */
	if (memcmp(buf, this->page_buf, mtd->writesize)) {
		printk("Invalid data buffer : 0x%x\n", (unsigned int)to);
		ret = -EBADMSG;
	}

	/* Take a simple comparison method */
	/* The following method cannot check validity if buf[i] is 0xFF even if i is a data position.*/
	for (i = 0; i < mtd->oobsize; i++)
		if (oobbuf[i] != 0xFF && oobbuf[i] != local_oobbuf[i]) {
			printk("Invalid OOB buffer :0x%x\n", (unsigned int)to);
			ret = -EBADMSG;
			break;			
		}

	return ret;
}
#endif

#define NOTALIGNED(x)	((x & (this->subpagesize - 1)) != 0)

/**
 * onenand_write - [MTD Interface] write buffer to FLASH
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 *
 */
int onenand_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct onenand_chip *this = mtd->priv;
	int written = 0;
	int i, ret = 0;
	void __iomem *cmd_addr;
	uint *buf_poi = (uint *)buf;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_write: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely((to + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_write: Attempt write to past end of device\n");
		return -EINVAL;
	}

	/* Reject writes, which are not page aligned */
	if (unlikely(NOTALIGNED(to)) || unlikely(NOTALIGNED(len))) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_write: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_WRITING);

	/* Loop until all data write */
	while (written < len) {
		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, to, 0)) {
				printk (KERN_WARNING "onenand_write: skipped to write to a bad block at addr 0x%08x.\n", (unsigned int) to);
				to += (1 << this->erase_shift);
				continue;
			}
		}

		/* get start address to write data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_PROG, to));
		//cmd_addr = phys_to_virt(this->command(mtd, ONENAND_CMD_PROG, to));
		
		/* write all data of 1 page by 4 bytes at a time */
		for (i = 0; i < (mtd->writesize / 4); i++) {
			this->write(*buf_poi, cmd_addr);
			buf_poi++;
		}

		if (onenand_blkrw_complete(this, ONENAND_CMD_PROG)) {
			printk(KERN_WARNING "onenand_write: Program operation failed.\n");
			return -1;
		}

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
		/* Only check verify write turn on */
		ret = onenand_verify_page(mtd, buf_poi - (mtd->writesize / 4), to);

		if (ret) {
			printk("onenand_write: verify failed:0x%x.\n", (unsigned int)to);
			break;
		}
#else
		/* It seems that denali controller requires timing delay
		 * between write transactions 
		 * To Do: REMOVE THE FOLLOWING udelay code
		 */
		udelay(2);
#endif

		written += mtd->writesize;

		if (written == len)
			break;

		to += mtd->writesize;
	}

	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	*retlen = written;

	return ret;
}

/**
 * onenand_fill_auto_oob - [Internal] oob auto-placement transfer
 * @param mtd		MTD device structure
 * @param oob_buf	oob buffer
 * @param buf		source address
 * @param column	oob offset to write to
 * @param thislen	oob length to write
 */
static int onenand_fill_auto_oob(struct mtd_info *mtd, u_char *oob_buf,
				  const u_char *buf, int column, int thislen)
{
	struct onenand_chip *this = mtd->priv;
	struct nand_oobfree *free;
	int writecol = column;
	int writeend = column + thislen;
	int lastgap = 0;
	unsigned int i;

	free = this->ecclayout->oobfree;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free->length; i++, free++) {
		if (writecol >= lastgap)
			writecol += free->offset - lastgap;
		if (writeend >= lastgap)
			writeend += free->offset - lastgap;
		lastgap = free->offset + free->length;
	}
	free = this->ecclayout->oobfree;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free->length; i++, free++) {
		int free_end = free->offset + free->length;
		if (free->offset < writeend && free_end > writecol) {
			int st = max_t(int,free->offset,writecol);
			int ed = min_t(int,free_end,writeend);
			int n = ed - st;
			memcpy(oob_buf + st, buf, n);
			buf += n;
		} else if (column == 0)
			break;
	}
	return 0;
}

/**
 * onenand_write_ops_nolock - [OneNAND Interface] write main and/or out-of-band
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param ops		oob operation description structure
 *
 * Write main and/or oob with ECC
 */
static int onenand_write_ops_nolock(struct mtd_info *mtd, loff_t to,
					struct mtd_oob_ops *ops)
{
	struct onenand_chip *this = mtd->priv;
	int written = 0, column, thislen;
	int oobwritten = 0, oobcolumn, thisooblen, oobsize;
	size_t len = ops->len;
	size_t ooblen = ops->ooblen;
	const u_char *buf = ops->datbuf;
	const u_char *oob = ops->oobbuf;
	u_char *oobbuf;
	int i, ret = 0;
	void __iomem *cmd_addr;
	uint *buf_poi;
	
	DEBUG(MTD_DEBUG_LEVEL3, "onenand_write_ops_nolock: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);
	
	/* Initialize retlen, in case of early exit */
	ops->retlen = 0;
	ops->oobretlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely((to + len) > mtd->size)) {
		printk(KERN_ERR "onenand_write_ops_nolock: Attempt write to past end of device\n");
		return -EINVAL;
	}

    /* Reject writes, which are not page aligned */
	if (unlikely(NOTALIGNED(to) || NOTALIGNED(len))) {
		printk(KERN_ERR "onenand_write_ops_nolock: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Check zero length */
	if (!len)
		return 0;
	
	if (ops->mode == MTD_OOB_AUTO)
		oobsize = this->ecclayout->oobavail;
	else
		oobsize = mtd->oobsize;

	oobcolumn = to & (mtd->oobsize - 1);
	column = to & (mtd->writesize - 1);

	/* on the TRANSFER SPARE bit */
	this->write(ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	/* Loop until all data write */
	while (written < len) {
		thislen = min_t(int, mtd->writesize - column, len - written);
		thisooblen = min_t(int, oobsize - oobcolumn, ooblen - oobwritten);

		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, to, 0)) {
				printk (KERN_WARNING "onenand_write_ops_nolock: skipped to write oob to a bad block at addr 0x%08x.\n", (unsigned int) to);
				to += (1 << this->erase_shift);

				if (column != 0)
					column = to & (mtd->writesize - 1);

				continue;
			}
		}

		/* get start address to write data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_PROG, to));

#ifdef ONENAND_WRITE_BURST
		onenand_write_burst(cmd_addr, buf, mtd->writesize);
#else
		/* write all data of 1 page by 4 bytes at a time */
		buf_poi = (uint *)buf;
		for (i = 0; i < (mtd->writesize / 4); i++) {
			this->write(*buf_poi, cmd_addr);
			buf_poi++;
		}
#endif

		if (oob) {
			oobbuf = this->oob_buf;

			/* We send data to spare ram with oobsize
			 * to prevent byte access */
			memset(oobbuf, 0xff, mtd->oobsize);
			if (ops->mode == MTD_OOB_AUTO)
				onenand_fill_auto_oob(mtd, oobbuf, oob, column, thislen);
			else
				memcpy(oobbuf + column, buf, thislen);

			oobwritten += thisooblen;
			oob += thisooblen;
			oobcolumn = 0;
		} else
			oobbuf = (u_char *)ffchars;

#ifdef ONENAND_WRITE_BURST
		onenand_write_burst(cmd_addr, oobbuf, mtd->oobsize);
#else
		buf_poi = (uint *)oobbuf;
		for (i = 0; i < (mtd->oobsize / 4); i++) {
			this->write(*buf_poi, cmd_addr);
			buf_poi++;
		}
#endif

		if (onenand_blkrw_complete(this, ONENAND_CMD_PROG)) {
			printk(KERN_WARNING "onenand_write_ops_nolock: Program operation failed.\n");
			ret = -1;
			goto err;
		}

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
		ret = onenand_verify_ops(mtd, buf, oobbuf, to);

		if (ret) {
			printk(KERN_ERR "onenand_write_ops_nolock: verify failed :0x%x\n", (unsigned int)to);
			break;
		}
#else
		/* It seems that denali controller requires timing delay
		 * between write transactions 
		 * To Do: REMOVE THE FOLLOWING udelay code
		 */
		udelay(2);
#endif

		written += thislen;

		if (written == len)
			break;

		column = 0;
		to += mtd->writesize;
		buf += thislen;
	}

	ops->retlen = written;
	ops->oobretlen = oobwritten;

err:
	/* off the TRANSFER SPARE bit */
	this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);
	
	return ret;
}


/**
 * onenand_write_oob_nolock - [Internal] OneNAND write out-of-band
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 * @param mode		operation mode
 *
 * OneNAND write out-of-band
 */
static int onenand_write_oob_nolock(struct mtd_info *mtd, loff_t to,
					struct mtd_oob_ops *ops)
{
	struct onenand_chip *this = mtd->priv;
	int i, column, ret = 0, oobsize;
	int written = 0;
	u_char *oobbuf, *orgbuf;
	void __iomem *cmd_addr;
	uint *buf_poi;

	size_t len = ops->ooblen;
	const u_char *buf = ops->oobbuf;
	mtd_oob_mode_t mode = ops->mode;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_write_oob_nolock: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	ops->oobretlen = 0;

	if (mode == MTD_OOB_AUTO)
		oobsize = this->ecclayout->oobavail;
	else
		oobsize = mtd->oobsize;

	column = to & (mtd->oobsize - 1);

	if (unlikely(column >= oobsize)) {
		printk(KERN_ERR "onenand_write_oob_nolock: Attempted to start write outside oob\n");
		return -EINVAL;
	}

	/* For compatibility with NAND: Do not allow write past end of page */
	if (unlikely(column + len > oobsize)) {
		printk(KERN_ERR "onenand_write_oob_nolock: "
		      "Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(to >= mtd->size ||
		     column + len > ((mtd->size >> this->page_shift) -
				     (to >> this->page_shift)) * oobsize)) {
		printk(KERN_ERR "onenand_write_oob_nolock: Attempted to write past end of device\n");
		return -EINVAL;
	}

	orgbuf = ops->oobbuf;
	oobbuf = this->oob_buf;
	buf_poi = (uint *)this->oob_buf;

	/* on the TRANSFER SPARE bit */
	this->write(ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	/* Loop until all data write */
	while (written < len) {
		int thislen = min_t(int, oobsize, len - written);

		if (this->options & ONENAND_CHECK_BAD) {
			if (onenand_block_isbad_nolock(mtd, to, 0)) {
				printk (KERN_WARNING "\nonenand_do_write_oob: skipped to write oob to a bad block at addr 0x%08x.\n", (unsigned int) to);
				to += (1 << this->erase_shift);

				if (column != 0)
					column = to & (mtd->oobsize - 1);

				continue;
			}
		}

		/* get start address to write data */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_PROG, to));
		
		/* We send data to spare ram with oobsize
		 * to prevent byte access */
		memset(oobbuf, 0xff, mtd->oobsize);

		if (mode == MTD_OOB_AUTO)
			onenand_fill_auto_oob(mtd, oobbuf, buf, column, thislen);
		else
			memcpy(oobbuf + column, buf, thislen);

		for (i = 0; i < (mtd->writesize / 4); i++)
			this->write(0xffffffff, cmd_addr);

#ifdef ONENAND_WRITE_BURST
		onenand_write_burst(cmd_addr, buf_poi, mtd->oobsize);
#else
		for (i = 0; i < (mtd->oobsize / 4); i++) {
			this->write(*buf_poi, cmd_addr);
			buf_poi++;
		}
#endif
		
		if (onenand_blkrw_complete(this, ONENAND_CMD_PROG)) {
			printk(KERN_WARNING "onenand_write_oob_nolock: Program operation failed.\n");
			this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);
			return -1;
		}
		

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
		ret = onenand_verify_oob(mtd, orgbuf, to, len);

		if (ret) {
			printk(KERN_ERR "onenand_write_oob_nolock: verify failed :0x%x\n", (unsigned int)to);
			break;
		}
#else
		/* It seems that denali controller requires timing delay
		 * between write transactions 
		 * To Do: REMOVE THE FOLLOWING udelay code
		 */
		udelay(2);
#endif

		written += thislen;
		if (written == len)
			break;

		to += mtd->writesize;
		buf += thislen;
		column = 0;
	}

	/* off the TRANSFER SPARE bit */
	this->write(~ONENAND_TRANS_SPARE_TSRF_INC, this->base + ONENAND_REG_TRANS_SPARE);

	ops->oobretlen = written;

	return ret;
}

/**
 * onenand_write_oob - [MTD Interface] NAND write data and/or out-of-band
 * @param mtd:		MTD device structure
 * @param to:		offset to write
 * @param ops:		oob operation description structure
 */
static int onenand_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	int ret;

	switch (ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
		break;
	case MTD_OOB_RAW:
		/* Not implemented yet */
	default:
		printk("Don't support this write OOB_MODE!!!\n");
		return -EINVAL;
	}

	onenand_get_device(mtd, FL_WRITING);
	if (ops->datbuf != NULL)
		ret = onenand_write_ops_nolock(mtd, to, ops);
	else
		ret = onenand_write_oob_nolock(mtd, to, ops);
	onenand_release_device(mtd);
	return ret;

}

/**
 * onenand_erase - [MTD Interface] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 *
 * Erase one ore more blocks
 */
int onenand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct onenand_chip *this = mtd->priv;
	unsigned int block_size;
	loff_t addr;
	int len, ret = 0;
	void __iomem *cmd_addr;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: start = 0x%08x, len = %i\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	block_size = (1 << this->erase_shift);

	/* Start address must align on block boundary */
	if (unlikely(instr->addr & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (unlikely(instr->len & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: Length not block aligned\n");
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if (unlikely((instr->len + instr->addr) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_ERASING);

	/* Loop throught the pages */
	len = instr->len;
	addr = instr->addr;

	instr->state = MTD_ERASING;

	while (len) {
		if (this->options & ONENAND_CHECK_BAD) {
			/* Check if we have a bad block, we do not erase bad blocks */
			if (onenand_block_isbad_nolock(mtd, addr, 0)) {
				printk (KERN_WARNING "onenand_erase: attempt to erase a bad block at addr 0x%08x\n", (unsigned int) addr);
				instr->state = MTD_ERASE_FAILED;
				goto erase_exit;
			}
		}

		/* get address to erase */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_ERASE, addr));

		this->write(ONENAND_DATAIN_ERASE_SINGLE, cmd_addr); /* single block erase */

		/* wait irq */
		onenand_irq_wait_ack(this, ONENAND_INT_ERR_INT_ACT);
		onenand_irq_wait_ack(this, ONENAND_INT_ERR_ERS_CMP);

		this->write(ONENAND_DATAIN_ERASE_VERIFY, cmd_addr);

		/* wait irq */
		onenand_irq_wait_ack(this, ONENAND_INT_ERR_INT_ACT);
		onenand_irq_wait_ack(this, ONENAND_INT_ERR_ERS_CMP);

		/* check fail */
		if (onenand_irq_pend(this, ONENAND_INT_ERR_ERS_FAIL)) {
			DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: block %d erase verify failed.\n", ((unsigned int)addr >> this->erase_shift));
			onenand_irq_ack(this, ONENAND_INT_ERR_ERS_FAIL);

			/* check lock */
			if (onenand_irq_pend(this, ONENAND_INT_ERR_LOCKED_BLK)) {
				DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: block %d is locked.\n", ((unsigned int)addr >> this->erase_shift));
				onenand_irq_ack(this, ONENAND_INT_ERR_LOCKED_BLK);
			}
		}

		len -= block_size;
		addr += block_size;
	}

	instr->state = MTD_ERASE_DONE;

erase_exit:
	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	return ret;
}

/**
 * onenand_sync - [MTD Interface] sync
 * @param mtd		MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void onenand_sync(struct mtd_info *mtd)
{
	DEBUG(MTD_DEBUG_LEVEL3, "onenand_sync: called\n");

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_SYNCING);

	/* Release it and go back */
	onenand_release_device(mtd);
}

/**
 * onenand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 *
 * Check whether the block is bad
 */
static int onenand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	/* Check for invalid offset */
	if ((uint32_t)ofs > mtd->size)
		return -EINVAL;

	onenand_get_device(mtd, FL_READING);
	ret = onenand_block_isbad_nolock(mtd, ofs, 0);
	onenand_release_device(mtd);

	return ret;
}

/**
 * onenand_default_block_markbad - [DEFAULT] mark a block bad
 * @param mtd		MTD device structure
 * @param ofs		offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
 */
static int onenand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct onenand_chip *this = mtd->priv;
	struct bbm_info *bbm = this->bbm;
	u_char buf[2] = {0, 0};
	struct mtd_oob_ops ops = {
		.mode = MTD_OOB_PLACE,
		.ooblen = 2,
		.oobbuf = buf,
		.ooboffs = 0,
	};
	int block;

	/* Get block number */
	block = ((int) ofs) >> bbm->bbt_erase_shift;
        if (bbm->bbt)
                bbm->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

        /* We write two bytes, so we dont have to mess with 16 bit access */
        ofs += mtd->oobsize + (bbm->badblockpos & ~0x01);
         return onenand_write_oob_nolock(mtd, ofs, &ops);
}

/**
 * onenand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 *
 * Mark the block as bad
 */
static int onenand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct onenand_chip *this = mtd->priv;
	int ret;

	if (this->options & ONENAND_CHECK_BAD) {
		ret = onenand_block_isbad(mtd, ofs);
		if (ret) {
			/* If it was bad already, return success and do nothing */
			if (ret > 0)
				return 0;
			return ret;
		}

		return this->block_markbad(mtd, ofs);
	} else
		return 0;
}

/**
 * onenand_do_lock_cmd - [OneNAND Interface] Lock or unlock block(s)
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 * @param len		number of bytes to lock or unlock
 *
 * Lock or unlock one or more blocks
 */
static int onenand_do_lock_cmd(struct mtd_info *mtd, loff_t ofs, size_t len, int cmd)
{
	struct onenand_chip *this = mtd->priv;
	int start, end, ofs_end, block_size;
	int datain1, datain2;
	void __iomem *cmd_addr;

	start = ofs >> this->erase_shift;
	end = len >> this->erase_shift;
	block_size = 1 << this->erase_shift;
	ofs_end = ofs + len - block_size;

	if (cmd == ONENAND_CMD_LOCK) {
		datain1 = ONENAND_DATAIN_LOCK_START;
		datain2 = ONENAND_DATAIN_LOCK_END;
	} else {
		datain1 = ONENAND_DATAIN_UNLOCK_START;
		datain2 = ONENAND_DATAIN_UNLOCK_END;
	}

	if (ofs < ofs_end) {
		cmd_addr = onenand_phys_to_virt(this->command(mtd, cmd, ofs));
		this->write(datain1, cmd_addr);
	}

	cmd_addr = onenand_phys_to_virt(this->command(mtd, cmd, ofs));
	this->write(datain2, cmd_addr);

	if (cmd == ONENAND_CMD_LOCK) {
		if (!this->read(this->base + ONENAND_REG_INT_ERR_STAT) & ONENAND_INT_ERR_LOCKED_BLK) {
			DEBUG(MTD_DEBUG_LEVEL3, "onenand_do_lock_cmd: lock failed.\n");
			return -1;
		}
	} else {
		if (this->read(this->base + ONENAND_REG_INT_ERR_STAT) & ONENAND_INT_ERR_LOCKED_BLK) {
			DEBUG(MTD_DEBUG_LEVEL3, "onenand_do_lock_cmd: unlock failed.\n");
			return -1;
		}
	}

	return 0;
}

/**
 * onenand_lock - [MTD Interface] Lock block(s)
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 * @param len		number of bytes to lock
 *
 * Lock one or more blocks
 */
static int onenand_lock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return onenand_do_lock_cmd(mtd, ofs, len, ONENAND_CMD_LOCK);
}

/**
 * onenand_unlock - [MTD Interface] Unlock block(s)
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 * @param len		number of bytes to unlock
 *
 * Unlock one or more blocks
 */
int onenand_unlock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return  onenand_do_lock_cmd(mtd, ofs, len, ONENAND_CMD_UNLOCK);
}

/**
 * onenand_check_lock_status - [OneNAND Interface] Check lock status
 * @param this		onenand chip data structure
 *
 * Check lock status
 */
static void onenand_check_lock_status(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	unsigned int block, end;
	void __iomem *cmd_addr;
	int tmp;

	end = this->chipsize >> this->erase_shift;

	for (block = 0; block < end; block++) {
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_READ, block << this->erase_shift));
		tmp = this->read(cmd_addr);

		if (this->read(this->base + ONENAND_REG_INT_ERR_STAT) & ONENAND_INT_ERR_LOCKED_BLK) {
			printk(KERN_ERR "block %d is write-protected!\n", block);
			this->write(ONENAND_INT_ERR_LOCKED_BLK, this->base + ONENAND_REG_INT_ERR_ACK);
		}
	}
}

/**
 * onenand_unlock_all - [OneNAND Interface] unlock all blocks
 * @param mtd		MTD device structure
 *
 * Unlock all blocks
 */
static void onenand_unlock_all(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *cmd_addr;

	if (this->options & ONENAND_HAS_UNLOCK_ALL) {
		/* write unlock command */
		cmd_addr = onenand_phys_to_virt(this->command(mtd, ONENAND_CMD_UNLOCK_ALL, 0));
		this->write(ONENAND_DATAIN_UNLOCK_ALL, cmd_addr);
		
		/* Workaround for all block unlock in DDP */
		if (this->device_id & ONENAND_DEVICE_IS_DDP) {
			loff_t ofs;
			ofs = this->chipsize >> 1;
			cmd_addr = onenand_phys_to_virt(this->command(mtd,
						ONENAND_CMD_UNLOCK_ALL, ofs));
			this->write(ONENAND_DATAIN_UNLOCK_ALL, cmd_addr);
		}

		onenand_check_lock_status(mtd);

		return;
	}

	onenand_unlock(mtd, 0x0, this->chipsize);

	return;
}

/**
 * onenand_lock_scheme - Check and set OneNAND lock scheme
 * @param mtd		MTD data structure
 *
 * Check and set OneNAND lock scheme
 */
static void onenand_lock_scheme(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	unsigned int density, process;

	/* Lock scheme depends on density and process */
	density = this->device_id >> ONENAND_DEVICE_DENSITY_SHIFT;
	process = this->version_id >> ONENAND_VERSION_PROCESS_SHIFT;

	/* Lock scheme */
	if (density >= ONENAND_DEVICE_DENSITY_2Gb) {
		printk(KERN_DEBUG "Chip support all block unlock\n");
		this->options |= ONENAND_HAS_UNLOCK_ALL;
	} else if (density == ONENAND_DEVICE_DENSITY_1Gb) {
		/* A-Die has all block unlock */
		if (process) {
			printk(KERN_DEBUG "Chip support all block unlock\n");
			this->options |= ONENAND_HAS_UNLOCK_ALL;
		}
	} else {
		/* Some OneNAND has continues lock scheme */
		if (!process) {
			printk(KERN_DEBUG "Lock scheme is Continues Lock\n");
			this->options |= ONENAND_HAS_CONT_LOCK;
		}
	}
}

/**
 * onenand_print_device_info - Print device ID
 * @param device        device ID
 *
 * Print device ID
 */
void onenand_print_device_info(int device, int version)
{
	int vcc, demuxed, ddp, density;

	vcc = device & ONENAND_DEVICE_VCC_MASK;
	demuxed = device & ONENAND_DEVICE_IS_DEMUX;
	ddp = device & ONENAND_DEVICE_IS_DDP;
	density = device >> ONENAND_DEVICE_DENSITY_SHIFT;
	printk(KERN_INFO "%sOneNAND%s %dMB %sV 16-bit (0x%02x)\n",
		demuxed ? "" : "Muxed ",
		ddp ? "(DDP)" : "",
		(16 << density),
		vcc ? "2.65/3.3" : "1.8",
		device);
	printk(KERN_INFO "OneNAND version = 0x%04x\n", version);
}

static const struct onenand_manufacturers onenand_manuf_ids[] = {
        {ONENAND_MFR_SAMSUNG, "Samsung"},
};

/**
 * onenand_check_maf - Check manufacturer ID
 * @param manuf         manufacturer ID
 *
 * Check manufacturer ID
 */
static int onenand_check_maf(int manuf)
{
	int size = ARRAY_SIZE(onenand_manuf_ids);
	char *name;
        int i;

	for (i = 0; i < size; i++)
                if (manuf == onenand_manuf_ids[i].id)
                        break;

	if (i < size)
		name = onenand_manuf_ids[i].name;
	else
		name = "Unknown";

	printk(KERN_DEBUG "OneNAND Manufacturer: %s (0x%0x)\n", name, manuf);

	return (i == size);
}

/*
 * Setting address width registers
 * (FBA_WIDTH, FPA_WIDTH, FSA_WIDTH, DFS_DBS_WIDTH)
 */
static void s3c_onenand_width_regs(struct onenand_chip *this)
{
	int dev_id, ddp, density;
	int w_dfs_dbs = 0, w_fba = 10, w_fpa = 6, w_fsa = 2;

	dev_id = readl(this->base + ONENAND_REG_DEVICE_ID);

	ddp = dev_id & ONENAND_DEVICE_IS_DDP;
	density = dev_id >> ONENAND_DEVICE_DENSITY_SHIFT;

	switch (density & 0xf) {
	case ONENAND_DEVICE_DENSITY_128Mb:
		w_dfs_dbs = 0;
		w_fba = 8;
		w_fpa = 6;
		w_fsa = 1;
		break;

	case ONENAND_DEVICE_DENSITY_256Mb:
		w_dfs_dbs = 0;
		w_fba = 9;
		w_fpa = 6;
		w_fsa = 1;
		break;

	case ONENAND_DEVICE_DENSITY_512Mb:
		w_dfs_dbs = 0;
		w_fba = 9;
		w_fpa = 6;
		w_fsa = 2;
		break;

	case ONENAND_DEVICE_DENSITY_1Gb:
		if (ddp) {
			w_dfs_dbs = 1;
			w_fba = 9;
		} else {
			w_dfs_dbs = 0;
			w_fba = 10;
		}

		w_fpa = 6;
		w_fsa = 2;
		break;

	case ONENAND_DEVICE_DENSITY_2Gb:
		if (ddp) {
			w_dfs_dbs = 1;
			w_fba = 10;
		} else {
			w_dfs_dbs = 0;
			w_fba = 11;
		}

		w_fpa = 6;
		w_fsa = 2;
		break;

	case ONENAND_DEVICE_DENSITY_4Gb:
		if (ddp) {
			w_dfs_dbs = 1;
			w_fba = 11;
		} else {
			w_dfs_dbs = 0;
			w_fba = 12;
		}

		w_fpa = 6;
		w_fsa = 2;
		break;
	}

	writel(w_fba, this->base + ONENAND_REG_FBA_WIDTH);
	writel(w_fpa, this->base + ONENAND_REG_FPA_WIDTH);
	writel(w_fsa, this->base + ONENAND_REG_FSA_WIDTH);
	writel(w_dfs_dbs, this->base + ONENAND_REG_DBS_DFS_WIDTH);
}

/*
 * Board-specific NAND initialization. The following members of the
 * argument are board-specific (per include/linux/mtd/nand.h):
 * - base : address that OneNAND is located at.
 * - scan_bbt: board specific bad block scan function.
 * Members with a "?" were not set in the merged testing-NAND branch,
 * so they are not set here either.
 */
static int s3c_onenand_init (struct onenand_chip *this)
{
	int value;

	this->options |= (ONENAND_READ_BURST | ONENAND_CHECK_BAD | ONENAND_PIPELINE_AHEAD);
	//this->options |= (ONENAND_READ_POLLING | ONENAND_CHECK_BAD /*| ONENAND_PIPELINE_AHEAD */);//edit by James
	/*** Initialize Controller ***/

#if defined(CONFIG_CPU_S5PC100)
	/* D0 Domain OneNAND Clock Gating */
	value = readl(S5P_SCLKGATE0);
//	value = (value & ~(1 << 2)) | (1<< 2);
	value = (value & ~(S5P_CLKGATE_SCLK0_ONENAND)) | (S5P_CLKGATE_SCLK0_ONENAND);
	writel(value, S5P_SCLKGATE0);

	/* ONENAND Select */
	value = readl(S5P_CLK_SRC0);
	value = value & ~(1 << 24);
	value = value & ~(1 << 20);
	writel(value, S5P_CLK_SRC0);

	/* SYSCON */
	value = readl(S5P_CLK_DIV1);
	value = (value & ~(3 << 16)) | (1 << 16);
	writel(value, S5P_CLK_DIV1);
#elif defined(CONFIG_CPU_S3C6410)
	/* SYSCON */
	value = readl(S3C_CLK_DIV0);
	value = (value & ~(3 << 16)) | (1 << 16);
	writel(value, S3C_CLK_DIV0);

	writel(ONENAND_FLASH_AUX_WD_DISABLE, this->base + ONENAND_REG_FLASH_AUX_CNTRL);
#endif

	/* Cold Reset */
	writel(ONENAND_MEM_RESET_COLD, this->base + ONENAND_REG_MEM_RESET);

	/* Access Clock Register */
	writel(ONENAND_ACC_CLOCK_134_67, this->base + ONENAND_REG_ACC_CLOCK);

	/* FBA, FPA, FSA, DBS_DFS Width Register */
	s3c_onenand_width_regs(this);

	/* Enable Interrupts */
	//writel(0x3ff, this->base + ONENAND_REG_INT_ERR_MASK);
	writel(0x3fff, this->base + ONENAND_REG_INT_ERR_ACK);
	writel(0x37ff, this->base + ONENAND_REG_INT_ERR_MASK);
	writel(ONENAND_INT_PIN_ENABLE, this->base + ONENAND_REG_INT_PIN_ENABLE);
	writel(readl(this->base + ONENAND_REG_INT_ERR_MASK) & ~(ONENAND_INT_ERR_RDY_ACT),
		this->base + ONENAND_REG_INT_ERR_MASK);

	/* Memory Device Configuration Register */
	value = (ONENAND_MEM_CFG_SYNC_READ | ONENAND_MEM_CFG_BRL_4 | \
			ONENAND_MEM_CFG_BL_16| ONENAND_MEM_CFG_IOBE | \
			ONENAND_MEM_CFG_INT_HIGH | ONENAND_MEM_CFG_RDY_HIGH | ONENAND_MEM_CFG_WM_SYNC);
	writel(value, this->base + ONENAND_REG_MEM_CFG);
	printk("the onenand->base is 0x%08x ...\n", this->base);
	/* Burst Length Register */
	writel(ONENAND_BURST_LEN_16, this->base + ONENAND_REG_BURST_LEN);

#ifdef CONFIG_MTD_ONENAND_CHECK_SPEED
	writel((readl(S3C_GPNCON) & ~(0x3 << 30)) | (0x1 << 30), S3C_GPNCON);
	writel(0 << 15, S3C_GPNDAT);
#endif

	return 0;
}

static void dummy_read(struct mtd_info *mtd)
{
	onenand_get_device(mtd, FL_READING);
	
	struct mtd_oob_ops dummy_ops = {
		.mode = MTD_OOB_AUTO,
		.ooblen = 0x800,
		.datbuf = kzalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL),
		.oobbuf = kzalloc(mtd->writesize, GFP_KERNEL),
		.ooboffs = 0,
	};
	loff_t dummy_from = 0x9000000;
	onenand_read_ops_nolock(mtd, dummy_from, &dummy_ops);

	kfree(dummy_ops.datbuf);
	kfree(dummy_ops.oobbuf);
	
	onenand_release_device(mtd);
}

/*
static void resume_timeover(unsigned long arg)
{
	struct mtd_info *info = arg;
	printk("resume_timeover~!!\n");
	del_timer(&ondtimer);
	onenand_release_device(info);
}
*/

/**
 * onenand_suspend - [MTD Interface] Suspend the OneNAND flash
 * @param mtd		MTD device structure
 */
static int onenand_suspend(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	//printk(KERN_INFO "OneNAND Suspend!\n");	// DEBUG
	
	ondreg.mem_cfg = readl(this->base + ONENAND_REG_MEM_CFG);
	ondreg.burst_len = readl(this->base + ONENAND_REG_BURST_LEN);
	ondreg.int_err_mask= readl(this->base + ONENAND_REG_INT_ERR_MASK);
	ondreg.manufact_id = readl(this->base + ONENAND_REG_MANUFACT_ID);
	ondreg.device_id = readl(this->base + ONENAND_REG_DEVICE_ID);
	ondreg.data_buf_size = readl(this->base + ONENAND_REG_DATA_BUF_SIZE);
	ondreg.fba_width = readl(this->base + ONENAND_REG_FBA_WIDTH);
	ondreg.fpa_width = readl(this->base + ONENAND_REG_FPA_WIDTH);
	ondreg.fsa_width = readl(this->base + ONENAND_REG_FSA_WIDTH);
	ondreg.dataram0 = readl(this->base + ONENAND_REG_DATARAM0);
	ondreg.dataram1 = readl(this->base + ONENAND_REG_DATARAM1);
	ondreg.trans_spare = readl(this->base + ONENAND_REG_TRANS_SPARE);
	ondreg.dbs_dfs_width = readl(this->base + ONENAND_REG_DBS_DFS_WIDTH);
	ondreg.int_pin_enable = readl(this->base + ONENAND_REG_INT_PIN_ENABLE);
	ondreg.int_mon_cyc = readl(this->base + ONENAND_REG_INT_MON_CYC);
	ondreg.acc_clock = readl(this->base + ONENAND_REG_ACC_CLOCK);
	ondreg.slow_rd_path = readl(this->base + ONENAND_REG_SLOW_RD_PATH);
	ondreg.flash_aux_cntrl= readl(this->base + ONENAND_REG_FLASH_AUX_CNTRL);

	return onenand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * onenand_resume - [MTD Interface] Resume the OneNAND flash
 * @param mtd		MTD device structure
 */
static void onenand_resume(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;

	//printk(KERN_INFO "OneNAND Resume!\n");	// DEBUG
#if defined(CONFIG_CPU_S5PC100)
	/* Set offset address (Denali controller) */
	this->write(ONENAND_FSA_SHIFT, this->base + ONENAND_REG_OFFSET_ADDR);

	/* Set correct page size */
	if (this->device_id == 0x50 || this->device_id == 0x68) {
		this->write(0x2, this->base + ONENAND_REG_DEV_PAGE_SIZE);
	}
#endif
	writel(ondreg.mem_cfg, this->base + ONENAND_REG_MEM_CFG);
	writel(ondreg.burst_len, this->base + ONENAND_REG_BURST_LEN);
	writel(ondreg.int_err_mask, this->base + ONENAND_REG_INT_ERR_MASK);
	writel(ondreg.manufact_id, this->base + ONENAND_REG_MANUFACT_ID);
	writel(ondreg.device_id, this->base + ONENAND_REG_DEVICE_ID);
	writel(ondreg.data_buf_size, this->base + ONENAND_REG_DATA_BUF_SIZE);
	writel(ondreg.fba_width, this->base + ONENAND_REG_FBA_WIDTH);
	writel(ondreg.fpa_width, this->base + ONENAND_REG_FPA_WIDTH);
	writel(ondreg.fsa_width, this->base + ONENAND_REG_FSA_WIDTH);
	writel(ondreg.dataram0, this->base + ONENAND_REG_DATARAM0);
	writel(ondreg.dataram1, this->base + ONENAND_REG_DATARAM1);
	writel(ondreg.trans_spare, this->base + ONENAND_REG_TRANS_SPARE);
	writel(ondreg.dbs_dfs_width, this->base + ONENAND_REG_DBS_DFS_WIDTH);
	writel(ondreg.int_pin_enable, this->base + ONENAND_REG_INT_PIN_ENABLE);
	writel(ondreg.int_mon_cyc, this->base + ONENAND_REG_INT_MON_CYC);
	writel(ondreg.acc_clock, this->base + ONENAND_REG_ACC_CLOCK);
	writel(ondreg.slow_rd_path, this->base + ONENAND_REG_SLOW_RD_PATH);
	writel(ondreg.flash_aux_cntrl, this->base + ONENAND_REG_FLASH_AUX_CNTRL);

	/* Unlock whole block */
	onenand_unlock_all(mtd);

	if (this->state == FL_PM_SUSPENDED)
		onenand_release_device(mtd);
	else
		printk(KERN_ERR "resume() called for the chip which is not"
				"in suspended state\n");
	/*
	onenand_get_device(mtd, FL_SYNCING);
	init_timer(&ondtimer);
	ondtimer.expires = jiffies + OND_TIMER_VALUE;
	ondtimer.data = (unsigned long)mtd;
	ondtimer.function = resume_timeover;
	add_timer(&ondtimer);
	*/
	dummy_read(mtd);
}

/**
 * onenand_probe - [OneNAND Interface] Probe the OneNAND device
 * @param mtd		MTD device structure
 *
 * OneNAND detection method:
 *   Compare the the values from command with ones from register
 */
static int onenand_probe(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	int maf_id, dev_id, ver_id, density;
	
	s3c_onenand_init(this);

#if !defined(MULTI_BASE)
	printk(KERN_INFO "onenand_probe: Use single base (256MB)\n");
	this->dev_base = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR, SZ_256M);

	if (!this->dev_base) {
		printk("ioremap failed.\n");
		return -1;
	}

	printk("onenand_probe: OneNAND memory device is remapped at 0x%08x.\n", (uint) this->dev_base);	
#else
#ifdef CONFIG_CPU_S5PC100
	printk(KERN_INFO "onenand_probe: Use four bases (128K, %dM, %dM, 256K)\n",
		2 << ONENAND_FSA_SHIFT, 2 << ONENAND_FSA_SHIFT);
	gBase00 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR,
												SZ_128K);
	gBase01 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0x4000000,
												SZ_2M << ONENAND_FSA_SHIFT);
	gBase10 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0x8000000,
												SZ_2M << ONENAND_FSA_SHIFT);
	gBase11 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0xC000000,
												SZ_256K);

	if (!gBase00 || !gBase01 || !gBase10 || !gBase11) {
		printk("ioremap failed.\n");
		return -1;
	}
#else
	printk(KERN_INFO "onenand_probe: Use four bases (128K, 16M, 16M, 256K)\n");
	gBase00 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR,
												SZ_128K);
	gBase01 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0x1000000,
												SZ_16M);
	gBase10 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0x2000000,
												SZ_16M);
	gBase11 = (void __iomem *)ioremap_nocache(ONENAND_AHB_ADDR + 0x3000000,
												SZ_256K);

	if (!gBase00 || !gBase01 || !gBase10 || !gBase11) {
		printk("ioremap failed.\n");
		return -1;
	}
#endif
#endif

	this->dma = ONENAND_DMA_CON;
	this->dma_ch = DMACH_ONENAND_IN;

	/* Read manufacturer and device IDs from Register */
	maf_id = this->read(this->base + ONENAND_REG_MANUFACT_ID);
	dev_id = this->read(this->base + ONENAND_REG_DEVICE_ID);
	ver_id = this->read(this->base + ONENAND_REG_FLASH_VER_ID);

	/* Check manufacturer ID */
	if (onenand_check_maf(maf_id))
		return -ENXIO;

	/* Flash device information */
	onenand_print_device_info(dev_id, ver_id);
	this->device_id = dev_id;
	this->version_id = ver_id;

	density = dev_id >> ONENAND_DEVICE_DENSITY_SHIFT;
	this->chipsize = (16 << density) << 20;
	printk("the this->chipsize is %ld ...\n", this->chipsize);
	/* Set density mask. it is used for DDP */
	this->density_mask = (1 << (density + 6));

#if defined(CONFIG_CPU_S5PC100)
	if (this->device_id == 0x50 || this->device_id == 0x68) {
		mtd->writesize = 0x1000;
		// It seems that Denali does not work correctly at 4KB page.
		// When device id is 0x50,
		//    Denali set 0x1 (page size: 2 KB) at SFR of ONENAND_REG_DEV
		this->write(0x2, this->base + ONENAND_REG_DEV_PAGE_SIZE);
	} else {
		/* OneNAND page size & block size */
		/* The data buffer size is equal to page size */
		mtd->writesize = this->read(this->base + ONENAND_REG_DATA_BUF_SIZE);
	}
#else
	/* OneNAND page size & block size */
	/* The data buffer size is equal to page size */
	mtd->writesize = this->read(this->base + ONENAND_REG_DATA_BUF_SIZE);
#endif
	mtd->oobsize = mtd->writesize >> 5;
	/* Pagers per block is always 64 in OneNAND */
	mtd->erasesize = mtd->writesize << 6;

	this->erase_shift = ffs(mtd->erasesize) - 1;
	this->page_shift = ffs(mtd->writesize) - 1;
	this->page_mask = (mtd->erasesize / mtd->writesize) - 1;

	/* REVIST: Multichip handling */

	mtd->size = this->chipsize;
	printk("the mtd->size is %ld ...\n", mtd->size);
	/* Check OneNAND lock scheme */
	onenand_lock_scheme(mtd);

#if defined(CONFIG_CPU_S5PC100)
	/* Set Offset Address (Denali Controller) */
	this->write(ONENAND_FSA_SHIFT, this->base + ONENAND_REG_OFFSET_ADDR);
#endif

	return 0;
}

int s3c_onenand_scan_bbt(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;

	if (this->options & ONENAND_CHECK_BAD)
		return onenand_default_bbt(mtd);
	else
		return 0;
}

/**
 * onenand_scan - [OneNAND Interface] Scan for the OneNAND device
 * @param mtd		MTD device structure
 * @param maxchips	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 */
int onenand_scan(struct mtd_info *mtd, int maxchips)
{
	int i;
	struct onenand_chip *this = mtd->priv;

	if (!this->read)
		this->read = onenand_readl;

	if (!this->write)
		this->write = onenand_writel;

	if (!this->command)
		this->command = onenand_command;

	if (!this->unlock_all)
		this->unlock_all = onenand_unlock_all;
		
	if (!this->block_markbad)
		this->block_markbad = onenand_default_block_markbad;

	if (!this->scan_bbt)
		this->scan_bbt = s3c_onenand_scan_bbt;

	if (onenand_probe(mtd))
		return -ENXIO;

	/* Allocate buffers, if necessary */
	if (!this->page_buf) {
		this->page_buf = kzalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL);
		if (!this->page_buf) {
			printk(KERN_ERR "onenand_scan(): Can't allocate page_buf\n");
			return -ENOMEM;
		}
		this->options |= ONENAND_PAGEBUF_ALLOC;
	}
	if (!this->oob_buf) {
		this->oob_buf = kzalloc(mtd->writesize, GFP_KERNEL);
		if (!this->oob_buf) {
			printk(KERN_ERR "onenand_scan(): Can't allocate oob_buf\n");
			if (this->options & ONENAND_PAGEBUF_ALLOC) {
				this->options &= ~ONENAND_PAGEBUF_ALLOC;
				kfree(this->page_buf);
			}
			return -ENOMEM;
		}
		this->options |= ONENAND_OOBBUF_ALLOC;
	}

	this->state = FL_READY;
	init_waitqueue_head(&this->wq);
	spin_lock_init(&this->chip_lock);

	/*
	 * Allow subpage writes up to oobsize.
	 */
	switch (mtd->oobsize) {
	case 128:
		this->ecclayout = &onenand_oob_128;
		/* FIXME : Check in data sheet */
		mtd->subpage_sft = 2;
		break;

	case 64:
		this->ecclayout = &onenand_oob_64;
		mtd->subpage_sft = 2;
		break;

	case 32:
		this->ecclayout = &onenand_oob_32;
		mtd->subpage_sft = 1;
		break;

	default:
		printk(KERN_WARNING "No OOB scheme defined for oobsize %d\n",
			mtd->oobsize);
		mtd->subpage_sft = 0;
		/* To prevent kernel oops */
		this->ecclayout = &onenand_oob_32;
		break;
	}

	this->subpagesize = mtd->writesize >> mtd->subpage_sft;

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area
	 */
	this->ecclayout->oobavail = 0;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES &&
		this->ecclayout->oobfree[i].length; i++) {
		this->ecclayout->oobavail +=
			this->ecclayout->oobfree[i].length;
	}
	mtd->oobavail = this->ecclayout->oobavail;
	mtd->ecclayout = this->ecclayout;
	DEBUG(MTD_DEBUG_LEVEL1, "oobavail: %d\n", mtd->oobavail);
	
	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->erase = onenand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = onenand_read;
	mtd->write = onenand_write;
	mtd->read_oob = onenand_read_oob;
	mtd->write_oob = onenand_write_oob;
	mtd->sync = onenand_sync;
	mtd->lock = onenand_lock;
	mtd->unlock = onenand_unlock;
	mtd->suspend = onenand_suspend;
	mtd->resume = onenand_resume;
	mtd->block_isbad = onenand_block_isbad;
	mtd->block_markbad = onenand_block_markbad;
	mtd->owner = THIS_MODULE;

	/* Unlock whole block */
	onenand_unlock_all(mtd);

	return this->scan_bbt(mtd);
}

/**
 * onenand_release - [OneNAND Interface] Free resources held by the OneNAND device
 * @param mtd		MTD device structure
 */
void onenand_release(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions (mtd);
#endif
	/* Deregister the device */
	del_mtd_device (mtd);

	/* Free bad block table memory, if allocated */
	if (this->bbm) {
		struct bbm_info *bbm = this->bbm;
		kfree(bbm->bbt);
		kfree(this->bbm);
	}

	/* Buffer allocated by onenand_scan */
	if (this->options & ONENAND_PAGEBUF_ALLOC)
		kfree(this->page_buf);
	if (this->options & ONENAND_OOBBUF_ALLOC)
		kfree(this->oob_buf);
}

EXPORT_SYMBOL_GPL(onenand_scan);
EXPORT_SYMBOL_GPL(onenand_release);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dong Jin PARK <djpax.park@samsung.com>");
MODULE_DESCRIPTION("S5PC100 OneNAND Controller Driver");
