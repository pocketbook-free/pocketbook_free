/*
 * s3c-i2s.h  --  ALSA Soc Audio Layer
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef S3C_I2S_H_
#define S3C_I2S_H_

//#define USE_CLKAUDIO	1 /* Don't use it for LPMP3 mode */

/* Clock dividers */
#define S3C_DIV_MCLK	0
#define S3C_DIV_BCLK	1
#define S3C_DIV_PRESCALER	2

#define S3C_IISCON		(0x00)
#define S3C_IISMOD		(0x04)
#define S3C_IISFIC		(0x08)
#define S3C_IISPSR		(0x0C)
#define S3C_IISTXD		(0x10)
#define S3C_IISRXD		(0x14)

#define S3C_IISFICS		(0x18)
#define S3C_IISTXDS		(0x1c)
#define S3C_IISAHB		(0x20)
#define S3C_IISSTR		(0x24)
#define S3C_IISSIZE		(0x28)
#define S3C_IISTRNCNT		(0x2c)
#define S3C_IISADDR0		(0x30)
#define S3C_IISADDR1		(0x34)
#define S3C_IISADDR2		(0x38)
#define S3C_IISADDR3		(0x3c)

#define S3C_IISCON_I2SACTIVE	(0x1<<0)
#define S3C_IISCON_RXDMACTIVE	(0x1<<1)
#define S3C_IISCON_TXDMACTIVE	(0x1<<2)
#define S3C_IISCON_RXCHPAUSE	(0x1<<3)
#define S3C_IISCON_TXCHPAUSE	(0x1<<4)
#define S3C_IISCON_RXDMAPAUSE	(0x1<<5)
#define S3C_IISCON_TXDMAPAUSE	(0x1<<6)
#define S3C_IISCON_FRXFULL		(0x1<<7)

#define S3C_IISCON_FTX0FULL		(0x1<<8)

#define S3C_IISCON_FRXEMPT		(0x1<<9)
#define S3C_IISCON_FTX0EMPT		(0x1<<10)
#define S3C_IISCON_LRI		(0x1<<11)

#define S3C_IISCON_FTX1FULL		(0x1<<12)
#define S3C_IISCON_FTX2FULL		(0x1<<13)
#define S3C_IISCON_FTX1EMPT		(0x1<<14)
#define S3C_IISCON_FTX2EMPT		(0x1<<15)

#define S3C_IISCON_FTXURINTEN	(0x1<<16)
#define S3C_IISCON_FTXURSTATUS	(0x1<<17)

#define S3C_IISCON_TXSDMACTIVE	(0x1<<18)
#define S3C_IISCON_TXSDMAPAUSE	(0x1<<20)
#define S3C_IISCON_FTXSFULL	(0x1<<21)
#define S3C_IISCON_FTXSEMPT	(0x1<<22)
#define S3C_IISCON_FTXSURINTEN	(0x1<<23)
#define S3C_IISCON_FTXSURSTAT	(0x1<<24)
#define S3C_IISCON_FRXOFINTEN	(0x1<<25)
#define S3C_IISCON_FRXOFSTAT	(0x1<<26)

#define S3C_IISCON_SWRESET	(0x1<<31)

#define S3C_IISMOD_BFSMASK		(3<<1)
#define S3C_IISMOD_32FS		(0<<1)
#define S3C_IISMOD_48FS		(1<<1)
#define S3C_IISMOD_16FS		(2<<1)
#define S3C_IISMOD_24FS		(3<<1)

#define S3C_IISMOD_RFSMASK		(3<<3)
#define S3C_IISMOD_256FS		(0<<3)
#define S3C_IISMOD_512FS		(1<<3)
#define S3C_IISMOD_384FS		(2<<3)
#define S3C_IISMOD_768FS		(3<<3)

#define S3C_IISMOD_SDFMASK		(3<<5)
#define S3C_IISMOD_IIS		(0<<5)
#define S3C_IISMOD_MSB		(1<<5)
#define S3C_IISMOD_LSB		(2<<5)

#define S3C_IISMOD_LRP		(1<<7)

#define S3C_IISMOD_TXRMASK		(3<<8)
#define S3C_IISMOD_TX		(0<<8)
#define S3C_IISMOD_RX		(1<<8)
#define S3C_IISMOD_TXRX		(2<<8)

#define S3C_IISMOD_IMSMASK		(3<<10)
#define S3C_IISMOD_MSTPCLK		(0<<10)
#define S3C_IISMOD_MSTCLKAUDIO	(1<<10)
#define S3C_IISMOD_SLVPCLK		(2<<10)
#define S3C_IISMOD_SLVI2SCLK	(3<<10)

#define S3C_IISMOD_CDCLKCON		(1<<12)

#define S3C_IISMOD_BLCMASK		(3<<13)
#define S3C_IISMOD_16BIT		(0<<13)
#define S3C_IISMOD_8BIT		(1<<13)
#define S3C_IISMOD_24BIT		(2<<13)

#define S3C_IISMOD_SD1EN		(1<<16)
#define S3C_IISMOD_SD2EN		(1<<17)

#define S3C_IISMOD_CCD1MASK		(3<<18)
#define S3C_IISMOD_CCD1ND		(0<<18)
#define S3C_IISMOD_CCD11STD		(1<<18)
#define S3C_IISMOD_CCD12NDD		(2<<18)

#define S3C_IISMOD_CCD2MASK		(3<<20)
#define S3C_IISMOD_CCD2ND		(0<<20)
#define S3C_IISMOD_CCD21STD		(1<<20)
#define S3C_IISMOD_CCD22NDD		(2<<20)

#define S3C_IISMOD_BLCPMASK		(3<<24)
#define S3C_IISMOD_P16BIT		(0<<24)
#define S3C_IISMOD_P8BIT		(1<<24)
#define S3C_IISMOD_P24BIT		(2<<24)
#define S3C_IISMOD_BLCSMASK		(3<<26)
#define S3C_IISMOD_S16BIT		(0<<26)
#define S3C_IISMOD_S8BIT		(1<<26)
#define S3C_IISMOD_S24BIT		(2<<26)
#define S3C_IISMOD_TXSLP		(1<<28)
#define S3C_IISMOD_OPMSK		(3<<30)
#define S3C_IISMOD_OPCCO		(0<<30)
#define S3C_IISMOD_OPCCI		(1<<30)
#define S3C_IISMOD_OPBCO		(2<<30)
#define S3C_IISMOD_OPPCLK		(3<<30)

#define S3C_IISFIC_FRXCNTMSK	(0x7f<<0)
#define S3C_IISFIC_RFLUSH		(1<<7)
#define S3C_IISFIC_FTX0CNTMSK	(0x7f<<8)
#define S3C_IISFIC_TFLUSH		(1<<15)

#define S3C_IISPSR_PSVALA		(0x3f<<8)
#define S3C_IISPSR_PSRAEN		(1<<15)

#define S3C_IISAHB_INTENLVL3	(1<<27)
#define S3C_IISAHB_INTENLVL2	(1<<26)
#define S3C_IISAHB_INTENLVL1	(1<<25)
#define S3C_IISAHB_INTENLVL0	(1<<24)
#define S3C_IISAHB_LVL3INT	(1<<23)
#define S3C_IISAHB_LVL2INT	(1<<22)
#define S3C_IISAHB_LVL1INT	(1<<21)
#define S3C_IISAHB_LVL0INT	(1<<20)
#define S3C_IISAHB_CLRLVL3	(1<<19)
#define S3C_IISAHB_CLRLVL2	(1<<18)
#define S3C_IISAHB_CLRLVL1	(1<<17)
#define S3C_IISAHB_CLRLVL0	(1<<16)
#define S3C_IISAHB_DMARLD	(1<<5)
#define S3C_IISAHB_DISRLDINT	(1<<3)
#define S3C_IISAHB_DMAEND	(1<<2)
#define S3C_IISAHB_DMACLR	(1<<1)
#define S3C_IISAHB_DMAEN	(1<<0)

#define S3C_IISSIZE_SHIFT	(16)
#define S3C_IISSIZE_TRNMSK	(0xffff)
#define S3C_IISTRNCNT_MASK	(0xffffff)

#define S3C_IISADDR_MASK	(0x3fffff)
#define S3C_IISADDR_SHIFT	(10)
#define S3C_IISADDR_ENSTOP	(1<<0)

/* clock sources */
#define S3C_CLKSRC_PCLK		S3C_IISMOD_MSTPCLK
#define S3C_CLKSRC_CLKAUDIO	S3C_IISMOD_MSTCLKAUDIO
#define S3C_CLKSRC_SLVPCLK	S3C_IISMOD_SLVPCLK
#define S3C_CLKSRC_I2SEXT	S3C_IISMOD_SLVI2SCLK
#define S3C_CDCLKSRC_INT	(4<<10)
#define S3C_CDCLKSRC_EXT	(5<<10)

#define S3C_PA_IIS_V32		S5PC1XX_PA_IIS_V32
#define S3C_PA_IIS_V50		S5PC1XX_PA_IIS_V50
#define IRQ_S3C_IISV32		IRQ_I2S1
#define IRQ_S3C_IISV50		IRQ_I2S0

#define S3C_DMACH_I2S_OUT	DMACH_I2S_V50_OUT
#define S3C_DMACH_I2S_IN	DMACH_I2S_V50_IN
#define S3C_IIS_PABASE		S3C_PA_IIS_V50
#define S3C_IISIRQ		IRQ_S3C_IISV50
#define PCLKCLK			"i2s_v50"
#define EXTCLK			"sclk_audio0"
#define PLBK_CHAN		6
#define S3C_DESC		"S3C AP I2S-V5.0 Interface"

/* dma_state */
#define S3C_I2SDMA_START   1
#define S3C_I2SDMA_STOP    2
#define S3C_I2SDMA_FLUSH   3

u32 s3c_i2s_get_clockrate(void);

struct s3c24xx_i2s_pdata {
	int lp_mode;
	u32 *p_rate;
	unsigned  dma_prd;
	void *dma_token;
	dma_addr_t dma_saddr;
	unsigned int dma_size;
	int dma_state;
	void (*dma_cb)(void *dt, int bytes_xfer);
	void (*dma_setcallbk)(void (*dcb)(void *, int), unsigned chunksize);
	void (*dma_getpos)(dma_addr_t *src, dma_addr_t *dst);
	int (*dma_enqueue)(void *id);
	void (*dma_ctrl)(int cmd);
	struct snd_soc_dai i2s_dai;
	void (*set_mode)(int lp_mode);
	spinlock_t lock;
};

#endif /*S3C_I2S_H_*/
