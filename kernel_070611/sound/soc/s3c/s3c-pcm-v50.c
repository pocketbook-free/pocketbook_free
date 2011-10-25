/*
 * s3c-pcm.c  --  ALSA Soc Audio Layer
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/dma.h>
#include <mach/audio.h>

#include "s3c-i2s-v50.h"
#include "s3c-pcm-v50.h"

/* AP's duty_cycle percentage */
/* For max power savings, define least possible with no latency issues */

/*
AP's DutyCycle   Minimum Periods   Period Size
   DUTY_CYCLE       MINPRDS          PRDSIZE
   **********       *******          *******
      0               1               131072
      1               127               1024
      2               63               2048
      3               125               1024
      4               31               4096
      5               61               2048
      6               121               1024
      7               15               8192
      8               59               2048
      9               117               1024
      10               29               4096
      11               57               2048

      12               113               1024
//    12               7                16384

      13               7               16384
      14               111               1024
      15               55               2048
      16               27               4096
      17               107               1024
      18               53               2048
      19               13               8192
      20               103               1024
      21               51               2048
      22               25               4096
      23               99               1024
      24               49               2048
      25               3               32768
      26               95               1024
      27               47               2048
      28               93               1024
      29               23               4096
      30               45               2048
      31               89               1024
      32               11               8192
      33               43               2048
      34               85               1024
      35               21               4096
      36               41               2048

      37               81               1024
//    37               5                16384

      38               5               16384
      39               79               1024
      40               39               2048
      41               19               4096
      42               75               1024
      43               37               2048
      44               9               8192
      45               71               1024
      46               35               2048
      47               17               4096
      48               67               1024
      49               33               2048
      50               1               65536
*/

/* We configure AP's duty cycle as 7% */
#define MINPRDS 15
#define PRDSIZE 8192

struct s3c24xx_pcm_pdata s3c_pcm_pdat;

struct s3c24xx_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	unsigned int dma_size;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct s3c24xx_pcm_dma_params *params;
};

static struct s3c24xx_i2s_pdata *s3ci2s_func = NULL;

/* s3c24xx_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
 */
static void s3c24xx_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	int ret;

	s3cdbg("Entered %s\n", __FUNCTION__);

	while (prtd->dma_loaded < prtd->dma_limit) {
		unsigned long len = prtd->dma_period;

		s3cdbg("dma_loaded: %d\n",prtd->dma_loaded);

		if ((pos + len) > prtd->dma_end) {
			len  = prtd->dma_end - pos;
			s3cdbg(KERN_DEBUG "%s: corrected dma len %ld\n",
			       __FUNCTION__, len);
		}

		s3cdbg("enqing at %x, %d bytes\n", pos, len);
		ret = s3c2410_dma_enqueue(prtd->params->channel, substream, pos, len);

		if (ret == 0) {
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if (pos >= prtd->dma_end)
				pos = prtd->dma_start;
		} else
			break;
	}

	prtd->dma_pos = pos;
}

static void s3c24xx_audio_buffdone(struct s3c2410_dma_chan *channel,
				void *dev_id, int size,
				enum s3c2410_dma_buffresult result)
{
	struct snd_pcm_substream *substream = dev_id;
	struct s3c24xx_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (result == S3C2410_RES_ABORT || result == S3C2410_RES_ERR)
		return;
		
	if (!substream)
		return;

	prtd = substream->runtime->private_data;
	snd_pcm_period_elapsed(substream);

	spin_lock(&prtd->lock);
	if (prtd->state & ST_RUNNING) {
		prtd->dma_loaded--;
		s3c24xx_pcm_enqueue(substream);
	}
	spin_unlock(&prtd->lock);
}

static void pcm_dmaupdate(void *id, int bytes_xfer)
{
	struct snd_pcm_substream *substream = id;

	s3cdbg("%s:%d\n", __func__, __LINE__);

	snd_pcm_period_elapsed(substream); /* Only once for any num of periods */
}

static int s3c_pcm_hw_params_lp(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long totbytes = params_buffer_bytes(params);

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(totbytes != MAX_LP_BUFF){
		s3cdbg("Use full buffer(128KB) in lowpower playback mode!");
		return -EINVAL;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = totbytes;

	/* We configure callback at partial playback complete acc to dutycyle selected */
	s3ci2s_func->dma_setcallbk(pcm_dmaupdate, MINPRDS * PRDSIZE);
	s3ci2s_func->dma_enqueue((void *)substream); /* Configure to loop the whole buffer */

	s3cdbg("DmaAddr=@%x Total=%lubytes PrdSz=%u #Prds=%u\n",
				runtime->dma_addr, totbytes, 
				params_period_bytes(params), runtime->hw.periods_min);

	return 0;
}

static int s3c24xx_pcm_hw_params_nm(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s3c24xx_pcm_dma_params *dma = rtd->dai->cpu_dai->dma_data;
	unsigned long totbytes = params_buffer_bytes(params);
	int ret=0;

	s3cdbg("Entered %s, params = %p \n", __FUNCTION__, prtd->params);

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!dma)
		return 0;

	/* this may get called several times by oss emulation
	 * with different params */
	if (prtd->params == NULL) {
		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);

		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	} else if (prtd->params != dma) {

		s3c2410_dma_free(prtd->params->channel, prtd->params->client);

		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);

		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	}

	/* channel needs configuring for mem=>device, increment memory addr,
	 * sync to pclk, half-word transfers to the IIS-FIFO. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_MEM, 0,
				prtd->params->dma_addr);

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);

	} else {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_HW, 0,
				prtd->params->dma_addr);		

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);
	}

	s3c2410_dma_set_buffdone_fn(prtd->params->channel,
				    s3c24xx_audio_buffdone);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = totbytes;

	spin_lock_irq(&prtd->lock);
	prtd->dma_loaded = 0;
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);
	s3cdbg("DmaAddr=@%x Total=%lubytes PrdSz=%u #Prds=%u\n",
				runtime->dma_addr, totbytes, params_period_bytes(params), runtime->hw.periods_min);

	return 0;
}

static int s3c_pcm_hw_free_lp(struct snd_pcm_substream *substream)
{
	s3cdbg("Entered %s\n", __FUNCTION__);

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int s3c24xx_pcm_hw_free_nm(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("Entered %s\n", __FUNCTION__);

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	if (prtd->params) {
		s3c2410_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = NULL;
	}

	return 0;
}

static int s3c_pcm_prepare_lp(struct snd_pcm_substream *substream)
{
	s3cdbg("Entered %s\n", __FUNCTION__);

	/* flush the DMA channel */
	s3ci2s_func->dma_ctrl(S3C_I2SDMA_FLUSH);

	return 0;
}

static int s3c24xx_pcm_prepare_nm(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("Entered %s\n", __FUNCTION__);
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) 
	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!prtd->params)
	 	return 0;
#endif

	/* flush the DMA channel */
	s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_FLUSH);

	prtd->dma_loaded = 0;

	prtd->dma_pos = prtd->dma_start;

	/* enqueue dma buffers */
	s3c24xx_pcm_enqueue(substream);

	return 0;
}

static int s3c24xx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;

		if(s3c_pcm_pdat.lp_mode)
		   s3ci2s_func->dma_ctrl(S3C_I2SDMA_START);
		else
		   s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_START);

		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;

		if(s3c_pcm_pdat.lp_mode)
		   s3ci2s_func->dma_ctrl(S3C_I2SDMA_STOP);
		else
		   s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STOP);

		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t 
	s3c24xx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;
	unsigned long res;
	dma_addr_t src, dst;

	spin_lock(&prtd->lock);

	if(s3c_pcm_pdat.lp_mode)
	   s3ci2s_func->dma_getpos(&src, &dst);
	else
	   s3c2410_dma_getposition(prtd->params->channel, &src, &dst);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		res = src - prtd->dma_start;
	else
		res = dst - prtd->dma_start;

	spin_unlock(&prtd->lock);

	s3cdbg("Pointer %x %x\n", src, dst);

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */

	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}

	return bytes_to_frames(substream->runtime, res);
}

static int s3c24xx_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long size, offset;
	int ret;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(s3c_pcm_pdat.lp_mode){
		/* From snd_pcm_lib_mmap_iomem */
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		vma->vm_flags |= VM_IO;
		size = vma->vm_end - vma->vm_start;
		offset = vma->vm_pgoff << PAGE_SHIFT;
		ret = io_remap_pfn_range(vma, vma->vm_start,
				(runtime->dma_addr + offset) >> PAGE_SHIFT,
				size, vma->vm_page_prot);
	}else{
		ret = dma_mmap_writecombine(substream->pcm->card->dev, vma,
						runtime->dma_area,
                                		runtime->dma_addr,
						runtime->dma_bytes);
	}

	return ret;
}

static int s3c24xx_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_tx);
	else
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_rx);

	prtd = kzalloc(sizeof(struct s3c24xx_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;

	return 0;
}

static int s3c24xx_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;

	s3cdbg("Entered %s, prtd = %p\n", __FUNCTION__, prtd);

	if (prtd)
		kfree(prtd);
	else
		printk("s3c24xx_pcm_close called with prtd == NULL\n");

	return 0;
}

static struct snd_pcm_ops s3c24xx_pcm_ops = {
	.open		= s3c24xx_pcm_open,
	.close		= s3c24xx_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	//.hw_params	= s3c24xx_pcm_hw_params,
	//.hw_free	= s3c24xx_pcm_hw_free,
	//.prepare	= s3c24xx_pcm_prepare,
	.trigger	= s3c24xx_pcm_trigger,
	.pointer	= s3c24xx_pcm_pointer,
	.mmap		= s3c24xx_pcm_mmap,
};

static int s3c24xx_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size;
	unsigned char *vaddr;
	dma_addr_t paddr;

	s3cdbg("Entered %s\n", __FUNCTION__);

	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	if(stream == SNDRV_PCM_STREAM_PLAYBACK)
	   size = s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max;
	else
	   size = s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max;

	if(s3c_pcm_pdat.lp_mode){
		/* Map Tx/Rx buff from LP_Audio SRAM */
		paddr = s3c_pcm_pdat.lp_buffs.dma_addr[stream]; /* already assigned */
		vaddr = (unsigned char *)ioremap(paddr, size);

		s3c_pcm_pdat.lp_buffs.cpu_addr[stream] = vaddr;
		s3c_pcm_pdat.lp_buffs.dma_addr[stream] = paddr;

		/* Assign PCM buffer pointers */
		buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
		buf->area = s3c_pcm_pdat.lp_buffs.cpu_addr[stream];
		buf->addr = s3c_pcm_pdat.lp_buffs.dma_addr[stream];
	}else{
		vaddr = dma_alloc_writecombine(pcm->card->dev, size,
					   &paddr, GFP_KERNEL);
		if (!vaddr)
			return -ENOMEM;
		s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = vaddr;
		s3c_pcm_pdat.nm_buffs.dma_addr[stream] = paddr;

		/* Assign PCM buffer pointers */
		buf->dev.type = SNDRV_DMA_TYPE_DEV;
		buf->area = s3c_pcm_pdat.nm_buffs.cpu_addr[stream];
		buf->addr = s3c_pcm_pdat.nm_buffs.dma_addr[stream];
	}

	s3cdbg("%s mode: preallocate buffer(%s):\tVA-%x\tPA-%x\tSz-%u\n", 
			s3c_pcm_pdat.lp_mode ? "LowPower" : "Normal", 
			stream ? "Capture": "Playback", 
			vaddr, paddr, size);

	buf->bytes = size;
	return 0;
}

static void s3c24xx_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	s3cdbg("Entered %s\n", __FUNCTION__);

	for (stream = SNDRV_PCM_STREAM_PLAYBACK; stream <= SNDRV_PCM_STREAM_CAPTURE; stream++) {

		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		if(s3c_pcm_pdat.lp_mode){
			iounmap(s3c_pcm_pdat.lp_buffs.cpu_addr[stream]);
			s3c_pcm_pdat.lp_buffs.cpu_addr[stream] = NULL;
		}else{
			dma_free_writecombine(pcm->card->dev, buf->bytes,
				      s3c_pcm_pdat.nm_buffs.cpu_addr[stream], s3c_pcm_pdat.nm_buffs.dma_addr[stream]);
			s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = NULL;
			s3c_pcm_pdat.nm_buffs.dma_addr[stream] = 0;
		}

		buf->area = NULL;
		buf->addr = 0;
	}
}

static u64 s3c24xx_pcm_dmamask = DMA_32BIT_MASK;

static int s3c24xx_pcm_new(struct snd_card *card, 
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &s3c24xx_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (dai->playback.channels_min) {
		ret = s3c24xx_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (dai->capture.channels_min) {
		ret = s3c24xx_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

static void s3c_pcm_setmode(int lpmd, void *ptr)
{
	s3ci2s_func = (struct s3c24xx_i2s_pdata *) ptr;
	s3c_pcm_pdat.lp_mode = lpmd;

	if(s3c_pcm_pdat.lp_mode){
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_params = s3c_pcm_hw_params_lp;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_free = s3c_pcm_hw_free_lp;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->prepare = s3c_pcm_prepare_lp;
		/* Configure Playback Channel */
		/* We use periods_min as a hint, to audio application, about h/w period size,
		 * which in turn is configured for selected duty-cycle of the AP.
		 * Ideally, application should use this value, periods_min*buffer_bytes_max, 
		 * in snd_pcm_sw_params_set_avail_min. AND, use period_bytes_max in snd_pcm_hw_params_set_period_size.
		 * AND, (period_bytes_max * periods_max) in snd_pcm_hw_params_set_buffer_size.
		 */
		s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max = MAX_LP_BUFF;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_min = PRDSIZE;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_max = s3c_pcm_pdat.pcm_hw_tx.period_bytes_min;
		s3c_pcm_pdat.pcm_hw_tx.periods_min = MINPRDS;
		s3c_pcm_pdat.pcm_hw_tx.periods_max = MAX_LP_BUFF / s3c_pcm_pdat.pcm_hw_tx.period_bytes_min;
		/* Configure Capture Channel */
		s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max = 0;
		s3c_pcm_pdat.pcm_hw_rx.channels_min = 0;
		s3c_pcm_pdat.pcm_hw_rx.channels_max = 0;
	}else{
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_params = s3c24xx_pcm_hw_params_nm;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_free = s3c24xx_pcm_hw_free_nm;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->prepare = s3c24xx_pcm_prepare_nm;

		/* Configure Playback Channel */
		s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max = MAX_LP_BUFF/2;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_min = PAGE_SIZE;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_max = PAGE_SIZE * 2;
		s3c_pcm_pdat.pcm_hw_tx.periods_min = 2;
		s3c_pcm_pdat.pcm_hw_tx.periods_max = 128;
		/* Configure Capture Channel */
		s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max = MAX_LP_BUFF/2;
		s3c_pcm_pdat.pcm_hw_rx.channels_min = 2;
		s3c_pcm_pdat.pcm_hw_rx.channels_max = 2;
	}
	s3cdbg("PrdsMn=%d PrdsMx=%d PrdSzMn=%d PrdSzMx=%d\n", 
					s3c_pcm_pdat.pcm_hw_tx.periods_min, s3c_pcm_pdat.pcm_hw_tx.periods_max,
					s3c_pcm_pdat.pcm_hw_tx.period_bytes_min, s3c_pcm_pdat.pcm_hw_tx.period_bytes_max);
}

struct s3c24xx_pcm_pdata s3c_pcm_pdat = {
	.lp_mode   = 0,
	.lp_buffs = {
		.dma_addr[SNDRV_PCM_STREAM_PLAYBACK] = LP_TXBUFF_ADDR,
		//.dma_addr[SNDRV_PCM_STREAM_CAPTURE]  = LP_RXBUFF_ADDR,
	},
	.set_mode = s3c_pcm_setmode,
	.pcm_pltfm = {
		.name		= "s3c-audio",
		.pcm_ops 	= &s3c24xx_pcm_ops,
		.pcm_new	= s3c24xx_pcm_new,
		.pcm_free	= s3c24xx_pcm_free_dma_buffers,
	},
	.pcm_hw_tx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.channels_min		= 2,
		.channels_max		= 2,
		.period_bytes_min	= PAGE_SIZE,
		.fifo_size		= 64,
	},
	.pcm_hw_rx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.period_bytes_min	= PAGE_SIZE,
		.period_bytes_max	= PAGE_SIZE*2,
		.periods_min		= 2,
		.periods_max		= 128,
		.fifo_size		= 64,
	},
};
EXPORT_SYMBOL_GPL(s3c_pcm_pdat);

MODULE_AUTHOR("Ben Dooks, Jassi");
MODULE_DESCRIPTION("Samsung S5PC1XX PCM module");
MODULE_LICENSE("GPL");
