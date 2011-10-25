#ifndef __LINUX_MTD_S3C_ONENAND_H
#define __LINUX_MTD_S3C_ONENAND_H

#include <linux/spinlock.h>
#include <linux/completion.h>
#include <plat/regs-onenand.h>
#include <linux/mtd/bbm.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <plat/dma.h>

#define MAX_BUFFERRAM		2

/* Scan and identify a OneNAND device */
extern int onenand_scan(struct mtd_info *mtd, int max_chips);
/* Free resources held by the OneNAND device */
extern void onenand_release(struct mtd_info *mtd);

/*
 * onenand_state_t - chip states
 * Enumeration for OneNAND flash chip state
 */
typedef enum {
	FL_READY,
	FL_READING,
	FL_WRITING,
	FL_ERASING,
	FL_SYNCING,
	FL_LOCKING,
	FL_RESETING,
	FL_OTPING,
	FL_PM_SUSPENDED,
} onenand_state_t;

/**
 * struct onenand_bufferram - OneNAND BufferRAM Data
 * @block:		block address in BufferRAM
 * @page:		page address in BufferRAM
 * @valid:		valid flag
 */
struct onenand_bufferram {
	int blockpage;
};

/**
 * struct onenand_chip - OneNAND Private Flash Chip Data
 * @base:		[BOARDSPECIFIC] address to access OneNAND
 * @chipsize:		[INTERN] the size of one chip for multichip arrays
 * @device_id:		[INTERN] device ID
 * @density_mask:	chip density, used for DDP devices
 * @verstion_id:	[INTERN] version ID
 * @options:		[BOARDSPECIFIC] various chip options. They can
 *			partly be set to inform onenand_scan about
 * @erase_shift:	[INTERN] number of address bits in a block
 * @page_shift:		[INTERN] number of address bits in a page
 * @page_mask:		[INTERN] a page per block mask
 * @bufferram_index:	[INTERN] BufferRAM index
 * @bufferram:		[INTERN] BufferRAM info
 * @readw:		[REPLACEABLE] hardware specific function for read short
 * @writew:		[REPLACEABLE] hardware specific function for write short
 * @command:		[REPLACEABLE] hardware specific function for writing
 *			commands to the chip
 * @wait:		[REPLACEABLE] hardware specific function for wait on ready
 * @read_bufferram:	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @write_bufferram:	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @read_word:		[REPLACEABLE] hardware specific function for read
 *			register of OneNAND
 * @write_word:		[REPLACEABLE] hardware specific function for write
 *			register of OneNAND
 * @mmcontrol:		sync burst read function
 * @block_markbad:	function to mark a block as bad
 * @scan_bbt:		[REPLACEALBE] hardware specific function for scanning
 *			Bad block Table
 * @chip_lock:		[INTERN] spinlock used to protect access to this
 *			structure and the chip
 * @wq:			[INTERN] wait queue to sleep on if a OneNAND
 *			operation is in progress
 * @state:		[INTERN] the current state of the OneNAND device
 * @page_buf:		data buffer
 * @subpagesize:	[INTERN] holds the subpagesize
 * @ecclayout:		[REPLACEABLE] the default ecc placement scheme
 * @bbm:		[REPLACEABLE] pointer to Bad Block Management
 * @priv:		[OPTIONAL] pointer to private chip date
 */
struct onenand_chip {
	void __iomem		*base;		/* SFR base address (0x7010_0000 ~) */
	unsigned int		chipsize;
	unsigned int		device_id;
	unsigned int		version_id;
	unsigned int		density_mask;
	unsigned int		options;

	unsigned int		erase_shift;
	unsigned int		page_shift;
	unsigned int		page_mask;
	unsigned int		writesize;

	unsigned int			bufferram_index;
	struct onenand_bufferram	bufferram[MAX_BUFFERRAM];

	uint (*command)(struct mtd_info *mtd, int cmd, loff_t address);
	int (*wait)(struct mtd_info *mtd, int state);
	void (*unlock_all)(struct mtd_info *mtd);
	int (*read_bufferram)(struct mtd_info *mtd, int area,
			unsigned char *buffer, int offset, size_t count);
	int (*write_bufferram)(struct mtd_info *mtd, int area,
			const unsigned char *buffer, int offset, size_t count);
	unsigned int (*read)(void __iomem *addr);
	void (*write)(unsigned int value, void __iomem *addr);
	void (*mmcontrol)(struct mtd_info *mtd, int sync_read);
	int (*block_markbad)(struct mtd_info *mtd, loff_t ofs);
	int (*scan_bbt)(struct mtd_info *mtd);

	struct completion	complete;
	int			irq;

	spinlock_t		chip_lock;
	wait_queue_head_t	wq;
	onenand_state_t		state;

	unsigned char		*page_buf;
	unsigned char		*oob_buf;

	int			subpagesize;
	struct nand_ecclayout	*ecclayout;

	void			*bbm;
	void			*priv;

	// Added for S5PC100
	//void __iomem	*dev_base;	/* virtual address base for AHB Port Address (0xB000_0000 ~ 0xB0FF_FFFF) */
	int				dma;
	unsigned int	dma_ch;
	void			*done;		/* completion */
};

extern int onenand_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf);
extern int onenand_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf);
extern int onenand_erase(struct mtd_info *mtd, struct erase_info *instr);
extern int onenand_get_device(struct mtd_info *mtd, int new_state);

/*Add by James for a struct use in onenand interface*/
extern struct mtd_info *s3c_onenand_interface;

struct onenand_registers {
	uint mem_cfg;
	uint burst_len;
	uint mem_reset;
	uint int_err_stat;
	uint int_err_mask;
	uint int_err_ack;
	uint manufact_id;
	uint device_id;
	uint data_buf_size;
	uint boot_buf_size;
	uint buf_amount;
	uint tech;
	uint fba_width;
	uint fpa_width;
	uint fsa_width;
	uint revision;
	uint dataram0;
	uint dataram1;
	uint sync_mode;
	uint trans_spare;
	uint dbs_dfs_width;
	uint page_cnt;
	uint err_page_addr;
	uint burst_rd_lat;
	uint int_pin_enable;
	uint int_mon_cyc;
	uint acc_clock;
	uint slow_rd_path;
	uint err_blk_addr;
	uint flash_ver_id;
	uint flash_aux_cntrl;
	uint flash_afifo_cnt;
};

/*
 * Options bits
 */
#define ONENAND_HAS_CONT_LOCK		(0x0001)
#define ONENAND_HAS_UNLOCK_ALL		(0x0002)
#define	ONENAND_CHECK_BAD		(0x0004)
#define	ONENAND_READ_POLLING		(0x0010)
#define ONENAND_READ_BURST		(0x0020)
#define ONENAND_READ_DMA		(0x0040)
#define ONENAND_PIPELINE_AHEAD		(0x0100)
#define ONENAND_READ_MASK		(0x00F0)
#define ONENAND_PAGEBUF_ALLOC		(0x1000)
#define ONENAND_OOBBUF_ALLOC		(0x2000)

/*
 * OneNAND Flash Manufacturer ID Codes
 */
#define ONENAND_MFR_SAMSUNG	0xec

/**
 * struct onenand_manufacturers - NAND Flash Manufacturer ID Structure
 * @name:	Manufacturer name
 * @id:		manufacturer ID code of device.
*/
struct onenand_manufacturers {
        int id;
        char *name;
};

#endif	/* __LINUX_MTD_ONENAND_H */

