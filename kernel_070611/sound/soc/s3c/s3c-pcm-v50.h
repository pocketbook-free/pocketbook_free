/*
 *  s5pc1xx-pcm.h --
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  ALSA PCM interface for the Samsung CPU
 */

#ifndef _S5PC1XX_PCM_H
#define _S5PC1XX_PCM_H

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)

#define MAX_LP_BUFF	(128 * 1024) /* 128KB is available for Playback and Capture combined */

#define LP_TXBUFF_ADDR    (0xC0000000)

struct s3c24xx_pcm_dma_params {
	struct s3c2410_dma_client *client;	/* stream identifier */
	int channel;				/* Channel ID */
	dma_addr_t dma_addr;
	int dma_size;			/* Size of the DMA transfer */
};

struct s3c_buffs {
	unsigned char    *cpu_addr[2]; /* Vir addr of Playback and Capture stream */
	dma_addr_t        dma_addr[2];  /* DMA addr of Playback and Capture stream */
};

struct s3c24xx_pcm_pdata {
	int               lp_mode;
	struct s3c_buffs  nm_buffs;
	struct s3c_buffs  lp_buffs;
	struct snd_soc_platform pcm_pltfm;
	struct snd_pcm_hardware pcm_hw_tx;
	struct snd_pcm_hardware pcm_hw_rx;
	void (*set_mode)(int lp_mode, void *ptr);
};

#define S3CPCM_DCON 	S3C2410_DCON_SYNC_PCLK|S3C2410_DCON_HANDSHAKE
#define S3CPCM_HWCFG 	S3C2410_DISRCC_INC|S3C2410_DISRCC_APB	

#endif
