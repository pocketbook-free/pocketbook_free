/*
 * s3c-i2s.c  --  ALSA Soc Audio Layer
 *
 * (c) 2009 Samsung Electronics   - Jaswinder Singh Brar <jassi.brar@samsung.com>
 *  Derived from Ben Dooks' driver for s3c24xx
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/map.h>
#include <plat/regs-clock.h>

#include "s3c-pcm-v50.h"
#include "s3c-i2s-v50.h"

static struct s3c2410_dma_client s3c_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct s3c24xx_pcm_dma_params s3c_i2s_pcm_stereo_out = {
	.client		= &s3c_dma_client_out,
	.channel	= S3C_DMACH_I2S_OUT,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISTXD,
	.dma_size	= 4,
};

static struct s3c24xx_pcm_dma_params s3c_i2s_pcm_stereo_in = {
	.client		= &s3c_dma_client_in,
	.channel	= S3C_DMACH_I2S_IN,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISRXD,
	.dma_size	= 4,
};

struct s3c_i2s_info {
	void __iomem  *regs;
	struct clk    *iis_clk;
	struct clk    *audio_bus;
	u32           iiscon;
	u32           iismod;
	u32           iisfic;
	u32           iispsr;
	u32           slave;
	u32           clk_rate;
};

static struct s3c_i2s_info s3c_i2s;

struct s3c24xx_i2s_pdata s3c_i2s_pdat;

#define S3C_IISFIC_LP 			(s3c_i2s_pdat.lp_mode ? S3C_IISFICS : S3C_IISFIC)
#define S3C_IISCON_TXDMACTIVE_LP 	(s3c_i2s_pdat.lp_mode ? S3C_IISCON_TXSDMACTIVE : S3C_IISCON_TXDMACTIVE)
#define S3C_IISCON_TXDMAPAUSE_LP 	(s3c_i2s_pdat.lp_mode ? S3C_IISCON_TXSDMAPAUSE : S3C_IISCON_TXDMAPAUSE)
#define S3C_IISMOD_BLCMASK_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_BLCSMASK : S3C_IISMOD_BLCPMASK)
#define S3C_IISMOD_8BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S8BIT : S3C_IISMOD_P8BIT)
#define S3C_IISMOD_16BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S16BIT : S3C_IISMOD_P16BIT)
#define S3C_IISMOD_24BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S24BIT : S3C_IISMOD_P24BIT)

#define dump_i2s()	do{	\
				printk("%s:%s:%d\t", __FILE__, __func__, __LINE__);	\
				printk("\tS3C_IISCON : %x", readl(s3c_i2s.regs + S3C_IISCON));		\
				printk("\tS3C_IISMOD : %x\n", readl(s3c_i2s.regs + S3C_IISMOD));	\
				printk("\tS3C_IISFIC : %x", readl(s3c_i2s.regs + S3C_IISFIC_LP));	\
				printk("\tS3C_IISPSR : %x\n", readl(s3c_i2s.regs + S3C_IISPSR));	\
				printk("\tS3C_IISAHB : %x\n", readl(s3c_i2s.regs + S3C_IISAHB));	\
				printk("\tS3C_IISSTR : %x\n", readl(s3c_i2s.regs + S3C_IISSTR));	\
				printk("\tS3C_IISSIZE : %x\n", readl(s3c_i2s.regs + S3C_IISSIZE));	\
				printk("\tS3C_IISADDR0 : %x\n", readl(s3c_i2s.regs + S3C_IISADDR0));	\
			}while(0)

static void s3c_snd_txctrl(int on)
{
	u32 iiscon;

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_TXCHPAUSE;
		iiscon  &= ~S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  |= S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}else{
		iiscon &= ~S3C_IISCON_I2SACTIVE;
		iiscon  |= S3C_IISCON_TXCHPAUSE;
		iiscon  |= S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  &= ~S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}
}

static void s3c_snd_rxctrl(int on)
{
	u32 iiscon;

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_RXCHPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMAPAUSE;
		iiscon  |= S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}else{
		iiscon &= ~S3C_IISCON_I2SACTIVE;
		iiscon  |= S3C_IISCON_RXCHPAUSE;
		iiscon  |= S3C_IISCON_RXDMAPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}

}

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s3c_snd_lrsync(void)
{
	u32 iiscon;
	int timeout = 50; /* 5ms */

	while (1) {
		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if (iiscon & S3C_IISCON_LRI)
			break;

		if (!timeout--)
			return -ETIMEDOUT;
		udelay(100);
	}

	return 0;
}

/*
 * Set s3c_ I2S DAI format
 */
static int s3c_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~S3C_IISMOD_SDFMASK;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		s3c_i2s.slave = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		s3c_i2s.slave = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iismod &= ~S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iismod |= S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iismod |= S3C_IISMOD_LSB;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iismod &= ~S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iismod |= S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	default:
		printk("Inv-combo(%d) not supported!\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}

static int s3c_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 iismod;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_out;
	else
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_in;

	/* Working copies of register */
	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~(S3C_IISMOD_BLCMASK | S3C_IISMOD_BLCMASK_LP);

	/* TODO */
	switch(params_channels(params)) {
	case 1:
		break;
	case 2:
		break;
	case 4:
		break;
	case 6:
		break;
	default:
		break;
	}

	/* RFS & BFS are set by dai_link(machine specific) code via set_clkdiv */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C_IISMOD_8BIT | S3C_IISMOD_8BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
 		iismod |= S3C_IISMOD_16BIT | S3C_IISMOD_16BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S24_LE:
 		iismod |= S3C_IISMOD_24BIT | S3C_IISMOD_24BIT_LP;
 		break;
	default:
		return -EINVAL;
 	}
 
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

static int s3c_i2s_startup(struct snd_pcm_substream *substream)
{
	u32 iiscon, iisfic;

	iiscon = readl(s3c_i2s.regs + S3C_IISCON);

	/* FIFOs must be flushed before enabling PSR and other MOD bits, so we do it here. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if(!(iiscon & S3C_IISCON_I2SACTIVE)){
			iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
			iisfic |= S3C_IISFIC_TFLUSH;
			writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
		}

		do{
	   	   cpu_relax();
		   iiscon = __raw_readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC_LP) & 0x7f) >> 8);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic &= ~S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	}else{
		if(!(iiscon & S3C_IISCON_I2SACTIVE)){
			iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
			iisfic |= S3C_IISFIC_RFLUSH;
			writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
		}

		do{
	   	   cpu_relax();
		   iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC_LP) & 0x7f) >> 0);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic &= ~S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	}

	return 0;
}

static int s3c_i2s_prepare(struct snd_pcm_substream *substream)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_RX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}else{
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_TX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

static int s3c_i2s_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (s3c_i2s.slave) {
			ret = s3c_snd_lrsync();
			if (ret)
				goto exit_err;
		}

		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(1);
		else
			s3c_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(0);
		else
			s3c_snd_txctrl(0);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit_err:
	return ret;
}

/*
 * Set s3c_ Clock source
 * Since, we set frequencies using PreScaler and BFS, RFS, we select input clock source to the IIS here.
 */
static int s3c_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	struct clk *clk;
	u32 iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	switch (clk_id) {
	case S3C_CLKSRC_PCLK:
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);
		break;

#ifdef USE_CLKAUDIO
	case S3C_CLKSRC_CLKAUDIO:
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
/*
8000 x 256 = 2048000
         49152000 mod 2048000 = 0
         32768000 mod 2048000 = 0 
         73728000 mod 2048000 = 0

11025 x 256 = 2822400
         67738000 mod 2822400 = 400

16000 x 256 = 4096000
         49152000 mod 4096000 = 0
         32768000 mod 4096000 = 0 
         73728000 mod 4096000 = 0

22050 x 256 = 5644800
         67738000 mod 5644800 = 400

32000 x 256 = 8192000
         49152000 mod 8192000 = 0
         32768000 mod 8192000 = 0
         73728000 mod 8192000 = 0

44100 x 256 = 11289600
         67738000 mod 11289600 = 400

48000 x 256 = 12288000
         49152000 mod 12288000 = 0
         73728000 mod 12288000 = 0

64000 x 256 = 16384000
         49152000 mod 16384000 = 0
         32768000 mod 16384000 = 0

88200 x 256 = 22579200
         67738000 mod 22579200 = 400

96000 x 256 = 24576000
         49152000 mod 24576000 = 0
         73728000 mod 24576000 = 0

	From the table above, we find that 49152000 gives least(0) residue 
	for most sample rates, followed by 67738000.
*/
		clk = clk_get(NULL, "fout_epll");
		if (IS_ERR(clk)) {
			printk("failed to get FOUTepll\n");
			return -EBUSY;
		}
		clk_disable(clk);
		switch (freq) {
		case 8000:
		case 16000:
		case 32000:
		case 48000:
		case 64000:
		case 96000:
			clk_set_rate(clk, 49152000);
			break;
		case 11025:
		case 22050:
		case 44100:
		case 88200:
		default:
			clk_set_rate(clk, 67738000);
			break;
		}
		clk_enable(clk);
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
		//printk("Setting FOUTepll to %dHz", s3c_i2s.clk_rate);
		clk_put(clk);
		break;
#endif

	case S3C_CLKSRC_SLVPCLK:
	case S3C_CLKSRC_I2SEXT:
		if(!s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		break;

	/* Not sure about these two! */
	case S3C_CDCLKSRC_INT:
		iismod &= ~S3C_IISMOD_CDCLKCON;
		break;

	case S3C_CDCLKSRC_EXT:
		iismod |= S3C_IISMOD_CDCLKCON;
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}

/*
 * Set s3c_ Clock dividers
 * NOTE: NOT all combinations of RFS, BFS and BCL are supported! XXX
 * Machine specific(dai-link) code must consider that while setting MCLK and BCLK in this function. XXX
 */
/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS_VAL(must be a multiple of BFS)                                 XXX */
/* XXX RFS_VAL & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int s3c_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	u32 reg;

	switch (div_id) {
	case S3C_DIV_MCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_RFSMASK;
		switch(div) {
		case 256: div = S3C_IISMOD_256FS; break;
		case 512: div = S3C_IISMOD_512FS; break;
		case 384: div = S3C_IISMOD_384FS; break;
		case 768: div = S3C_IISMOD_768FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_BCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_BFSMASK;
		switch(div) {
		case 16: div = S3C_IISMOD_16FS; break;
		case 24: div = S3C_IISMOD_24FS; break;
		case 32: div = S3C_IISMOD_32FS; break;
		case 48: div = S3C_IISMOD_48FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_PRESCALER:
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSRAEN;
		writel(reg, s3c_i2s.regs + S3C_IISPSR);
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSVALA;
		div &= 0x3f;
		writel(reg | (div<<8) | S3C_IISPSR_PSRAEN, s3c_i2s.regs + S3C_IISPSR);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon, iisahb, val;

	//dump_i2s();

	iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);
	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	if(iiscon & S3C_IISCON_FTXSURSTAT) {
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s3cdbg("TX_S underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}
	if(S3C_IISCON_FTXURSTATUS & iiscon) {
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s3cdbg("TX_P underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}

	val = 0;
	val |= ((iisahb & S3C_IISAHB_LVL0INT) ? S3C_IISAHB_CLRLVL0 : 0);
	//val |= ((iisahb & S3C_IISAHB_DMAEND) ? S3C_IISAHB_DMACLR : 0);
	if(val){
		iisahb |= val;
		iisahb |= S3C_IISAHB_INTENLVL0;
		writel(iisahb, s3c_i2s.regs + S3C_IISAHB);

		if(iisahb & S3C_IISAHB_LVL0INT){
			val = readl(s3c_i2s.regs + S3C_IISADDR0) - LP_TXBUFF_ADDR; /* current offset */
			val += s3c_i2s_pdat.dma_prd; /* Length before next Lvl0 Intr */
			val %= MAX_LP_BUFF; /* Round off at boundary */
			writel(LP_TXBUFF_ADDR + val, s3c_i2s.regs + S3C_IISADDR0); /* Update start address */
		}

		if(iisahb & S3C_IISAHB_DMAEND)
			printk("Didnt expect S3C_IISAHB_DMAEND\n");

		iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);
		iisahb |= S3C_IISAHB_DMAEN;
		writel(iisahb, s3c_i2s.regs + S3C_IISAHB);

		/* Keep callback in the end */
		if(s3c_i2s_pdat.dma_cb){
		   val = (iisahb & S3C_IISAHB_LVL0INT) ? 
				s3c_i2s_pdat.dma_prd:
				MAX_LP_BUFF;
		   s3c_i2s_pdat.dma_cb(s3c_i2s_pdat.dma_token, val);
		}
	}

	return IRQ_HANDLED;
}

static void init_i2s(void)
{
	u32 iiscon, iismod, iisahb;

	writel(S3C_IISCON_I2SACTIVE | S3C_IISCON_SWRESET, s3c_i2s.regs + S3C_IISCON);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	iismod  = readl(s3c_i2s.regs + S3C_IISMOD);
	iisahb  = S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT;

	/* Enable all interrupts to find bugs */
	iiscon |= S3C_IISCON_FRXOFINTEN;
	iismod &= ~S3C_IISMOD_OPMSK;
	iismod |= S3C_IISMOD_OPPCLK;

	if(s3c_i2s_pdat.lp_mode){
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXSURINTEN;
		iismod |= S3C_IISMOD_TXSLP;
		//iisahb |= S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT;
	}else{
		iiscon &= ~S3C_IISCON_FTXSURINTEN;
		iiscon |= S3C_IISCON_FTXURINTEN;
		iismod &= ~S3C_IISMOD_TXSLP;
		iisahb = 0;
	}

	writel(iisahb, s3c_i2s.regs + S3C_IISAHB);
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(iiscon, s3c_i2s.regs + S3C_IISCON);
}

static int s3c_i2s_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	int ret = 0;
	struct clk *cm, *cf;

	s3c_i2s.regs = ioremap(S3C_IIS_PABASE, 0x100);
	if (s3c_i2s.regs == NULL)
		return -ENXIO;

	ret = request_irq(S3C_IISIRQ, s3c_iis_irq, 0, "s3c-i2s", pdev);
	if (ret < 0) {
		printk("fail to claim i2s irq , ret = %d\n", ret);
		iounmap(s3c_i2s.regs);
		return -ENODEV;
	}

	s3c_i2s.iis_clk = clk_get(&pdev->dev, PCLKCLK);
	if (IS_ERR(s3c_i2s.iis_clk)) {
		printk("failed to get clk(%s)\n", PCLKCLK);
		goto lb5;
	}
	clk_enable(s3c_i2s.iis_clk);
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);

#ifdef USE_CLKAUDIO
	if(s3c_i2s_pdat.lp_mode){
		printk("Don't use CLKAUDIO in LP_Audio mode!\n");
		goto lb4;
	}

	s3c_i2s.audio_bus = clk_get(NULL, EXTCLK);
	if (IS_ERR(s3c_i2s.audio_bus)) {
		printk("failed to get clk(%s)\n", EXTCLK);
		goto lb4;
	}

	cm = clk_get(NULL, "mout_epll");
	if (IS_ERR(cm)) {
		printk("failed to get mout_epll\n");
		goto lb3;
	}
	if(clk_set_parent(s3c_i2s.audio_bus, cm)){
		printk("failed to set MOUTepll as parent of CLKAUDIO0\n");
		goto lb2;
	}

	cf = clk_get(NULL, "fout_epll");
	if (IS_ERR(cf)) {
		printk("failed to get fout_epll\n");
		goto lb2;
	}
	clk_enable(cf);
	if(clk_set_parent(cm, cf)){
		printk("failed to set FOUTepll as parent of MOUTepll\n");
		goto lb1;
	}
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
	clk_put(cf);
	clk_put(cm);
#endif

	init_i2s();

	s3c_snd_txctrl(0);
	s3c_snd_rxctrl(0);

	return 0;

#ifdef USE_CLKAUDIO
lb1:
	clk_put(cf);
lb2:
	clk_put(cm);
lb3:
	clk_put(s3c_i2s.audio_bus);
#endif
lb4:
	clk_disable(s3c_i2s.iis_clk);
	clk_put(s3c_i2s.iis_clk);
lb5:
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);
	
	return -ENODEV;
}

static void s3c_i2s_remove(struct platform_device *pdev,
		       struct snd_soc_dai *dai)
{
	writel(0, s3c_i2s.regs + S3C_IISCON);

#ifdef USE_CLKAUDIO
	clk_put(s3c_i2s.audio_bus);
#endif
	clk_disable(s3c_i2s.iis_clk);
	clk_put(s3c_i2s.iis_clk);
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);
}

#ifdef CONFIG_PM
static int s3c_i2s_suspend(struct platform_device *pdev,
		struct snd_soc_dai *cpu_dai)
{
	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
	s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
	s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);

	clk_disable(s3c_i2s.iis_clk);

	return 0;
}

static int s3c_i2s_resume(struct platform_device *pdev,
		struct snd_soc_dai *cpu_dai)
{
	clk_enable(s3c_i2s.iis_clk);

	writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
	writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);

	return 0;
}
#else
#define s3c_i2s_suspend NULL
#define s3c_i2s_resume NULL
#endif

static void s3c_i2sdma_getpos(dma_addr_t *src, dma_addr_t *dst)
{
	*dst = s3c_i2s_pcm_stereo_out.dma_addr;
	*src = LP_TXBUFF_ADDR + (readl(s3c_i2s.regs + S3C_IISTRNCNT) & 0xffffff) * 4;
}

static int s3c_i2sdma_enqueue(void *id)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_token = id;
	spin_unlock(&s3c_i2s_pdat.lock);

	s3cdbg("%s: %d@%x\n", __func__, MAX_LP_BUFF, LP_TXBUFF_ADDR);
	s3cdbg("CB @%x", LP_TXBUFF_ADDR + MAX_LP_BUFF);
	if(s3c_i2s_pdat.dma_prd != MAX_LP_BUFF)
		s3cdbg(" and @%x\n", LP_TXBUFF_ADDR + s3c_i2s_pdat.dma_prd);
	else
		s3cdbg("\n");

	val = LP_TXBUFF_ADDR + s3c_i2s_pdat.dma_prd;
	//val |= S3C_IISADDR_ENSTOP;
	writel(val, s3c_i2s.regs + S3C_IISADDR0);

	val = readl(s3c_i2s.regs + S3C_IISSTR);
	val = LP_TXBUFF_ADDR;
	writel(val, s3c_i2s.regs + S3C_IISSTR);

	val = readl(s3c_i2s.regs + S3C_IISSIZE);
	val &= ~(S3C_IISSIZE_TRNMSK << S3C_IISSIZE_SHIFT);
	val |= (((MAX_LP_BUFF >> 2) & S3C_IISSIZE_TRNMSK) << S3C_IISSIZE_SHIFT);
	writel(val, s3c_i2s.regs + S3C_IISSIZE);

	val = readl(s3c_i2s.regs + S3C_IISAHB);
	val |= S3C_IISAHB_INTENLVL0;
	writel(val, s3c_i2s.regs + S3C_IISAHB);

	return 0;
}

static void s3c_i2sdma_setcallbk(void (*cb)(void *id, int result), unsigned prd)
{
	if(!prd || prd > MAX_LP_BUFF)
	   prd = MAX_LP_BUFF;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_cb = cb;
	s3c_i2s_pdat.dma_prd = prd;
	spin_unlock(&s3c_i2s_pdat.lock);
}

static void s3c_i2s_setmode(int lpmd)
{
	s3c_i2s_pdat.lp_mode = lpmd;

	spin_lock_init(&s3c_i2s_pdat.lock);

	if(s3c_i2s_pdat.lp_mode){
	   s3c_i2s_pdat.i2s_dai.capture.channels_min = 0;
	   s3c_i2s_pdat.i2s_dai.capture.channels_max = 0;
	   s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXDS,
	   writel((readl(S5P_LPMP_MODE_SEL) & ~(1<<1)) | (1<<0), S5P_LPMP_MODE_SEL);
	}else{
	   s3c_i2s_pdat.i2s_dai.capture.channels_min = 2;
	   s3c_i2s_pdat.i2s_dai.capture.channels_max = 2;
	   s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXD,
	   writel((readl(S5P_LPMP_MODE_SEL) & ~(1<<0)) | (1<<1), S5P_LPMP_MODE_SEL);
	}

	writel(readl(S5P_CLKGATE_D20) | S5P_CLKGATE_D20_HCLKD2 | S5P_CLKGATE_D20_I2SD2, S5P_CLKGATE_D20);
}

static void s3c_i2sdma_ctrl(int state)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);

	val = readl(s3c_i2s.regs + S3C_IISAHB);

	switch(state){
	   case S3C_I2SDMA_START:
				  val |= S3C_IISAHB_INTENLVL0 | S3C_IISAHB_DMAEN;
		                  break;

	   case S3C_I2SDMA_FLUSH:
	   case S3C_I2SDMA_STOP:
				  val &= ~(S3C_IISAHB_INTENLVL0 | S3C_IISAHB_DMAEN);
		                  break;

	   default:
				  spin_unlock(&s3c_i2s_pdat.lock);
				  return;
	}

	writel(val, s3c_i2s.regs + S3C_IISAHB);

	s3c_i2s_pdat.dma_state = state;

	spin_unlock(&s3c_i2s_pdat.lock);
}

struct s3c24xx_i2s_pdata s3c_i2s_pdat = {
	.lp_mode = 0,
	.set_mode = s3c_i2s_setmode,
	.p_rate = &s3c_i2s.clk_rate,
	.dma_getpos = s3c_i2sdma_getpos,
	.dma_enqueue = s3c_i2sdma_enqueue,
	.dma_setcallbk = s3c_i2sdma_setcallbk,
	.dma_token = NULL,
	.dma_cb = NULL,
	.dma_ctrl = s3c_i2sdma_ctrl,
	.i2s_dai = {
		.name = "s3c-i2s",
		.id = 0,
		.type = SND_SOC_DAI_I2S,
		.probe = s3c_i2s_probe,
		.remove = s3c_i2s_remove,
		.suspend = s3c_i2s_suspend,
		.resume = s3c_i2s_resume,
		.playback = {
			.channels_min = 2,
			.channels_max = PLBK_CHAN,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
			},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = {
			.hw_params = s3c_i2s_hw_params,
			.prepare   = s3c_i2s_prepare,
			.startup   = s3c_i2s_startup,
			.trigger   = s3c_i2s_trigger,
		},
		.dai_ops = {
			.set_fmt = s3c_i2s_set_fmt,
			.set_clkdiv = s3c_i2s_set_clkdiv,
			.set_sysclk = s3c_i2s_set_sysclk,
		},
	},
};

EXPORT_SYMBOL_GPL(s3c_i2s_pdat);

/* Module information */
MODULE_AUTHOR("Jaswinder Singh <jassi.brar@samsung.com>");
MODULE_DESCRIPTION(S3C_DESC);
MODULE_LICENSE("GPL");
