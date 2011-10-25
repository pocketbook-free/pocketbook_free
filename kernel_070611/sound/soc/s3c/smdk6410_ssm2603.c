/*
 * smdk6400_ssm2603.c
 *
 * Copyright (C) 2010, Hill.Y.Chang  Hill.Y.Chang@tmsbg.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>

#include <plat/regs-iis.h>
#include <plat/map-base.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/regs-gpio.h>

#include <mach/hardware.h>
#include <mach/audio.h>
#include <asm/io.h>
#include <plat/regs-clock.h>

#include <mach/map.h>

#include "../codecs/wm8753.h"
#include "../codecs/ssm2603.h"
#include "s3c-pcm.h"
#include "s3c-i2s.h"

#define CONFIG_SND_DEBUG
#undef CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

static int ssm2602_spk_func = 1;
/* define the scenarios */
#define SMDK6400_AUDIO_OFF		0
#define SMDK6400_CAPTURE_MIC1		3
#define SMDK6400_STEREO_TO_HEADPHONES	2
#define SMDK6400_CAPTURE_LINE_IN	1

#if defined(CONFIG_HW_EP1_EVT) || defined(CONFIG_HW_EP2_EVT)  || defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)
void SpeakerMute(u8 mute)
{
	u32 val;
  
  //s3cdbg("in %s ...\n",__func__);	  
  //GPE0
	val = __raw_readl(S3C64XX_GPECON);
	val =  (val & ~(0xF<<0)) | (1<<0);
	__raw_writel(val, S3C64XX_GPECON);	   	
	
  if(mute)
  {
		val = __raw_readl(S3C64XX_GPEDAT);	
		val =  (val & ~(0x1<<0)) | (0<<0);	
		__raw_writel(val, S3C64XX_GPEDAT);		  	
  }
  else
  {
		val = __raw_readl(S3C64XX_GPEDAT);	
		val =  (val & ~(0x1<<0)) | (1<<0);	
		__raw_writel(val, S3C64XX_GPEDAT);		  	  
  }	
}
EXPORT_SYMBOL(SpeakerMute);

#elif defined(CONFIG_HW_EP3_EVT2) || defined(CONFIG_HW_EP4_EVT2) || defined(CONFIG_HW_EP1_EVT2) || defined(CONFIG_HW_EP2_EVT2) \
   || defined(CONFIG_HW_EP3_DVT) || defined(CONFIG_HW_EP4_DVT) || defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP2_DVT) 

void SpeakerMute(u8 mute)
{
	u32 val;
  
  s3cdbg("in %s ...\n",__func__);	
  
  //GPK5
	val = __raw_readl(S3C64XX_GPKCON);
	val =  (val & ~(0xF<<20)) | (1<<20);
	__raw_writel(val, S3C64XX_GPKCON);	   	
	
  if(mute)
  {
		val = __raw_readl(S3C64XX_GPKDAT);	
		val =  (val & ~(0x1<<5)) | (0<<5);	
		__raw_writel(val, S3C64XX_GPKDAT);		  	
  }
  else
  {
		val = __raw_readl(S3C64XX_GPKDAT);	
		val =  (val & ~(0x1<<5)) | (1<<5);	
		__raw_writel(val, S3C64XX_GPKDAT);		  	  
  }
}
EXPORT_SYMBOL(SpeakerMute);
#endif

//
//void SetCodecICModeI2C()
//{
//	u32 val;
//  
//  s3cdbg("in %s ...\n",__func__);	
//  
//  //GPE3 low I2C, high SPI
//	val = __raw_readl(S3C64XX_GPECON);
//	val =  (val & ~(0xF<<12)) | (1<<12); //set low
//	__raw_writel(val, S3C64XX_GPECON);	   	
//	
//	val = __raw_readl(S3C64XX_GPEDAT);	
//	val =  (val & ~(0x1<<3)) | (0<<3);	
//	__raw_writel(val, S3C64XX_GPEDAT);		  	
//
//}

static int smdk6410_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int pll_out = 0, bclk = 0,clk=0;
	int ret = 0;
	unsigned int iispsr, iismod, iiscon;
	u32*    regs;

	unsigned int prescaler = 4;
	int bfs, rfs;	
  regs = ioremap(S3C_IIS_PABASE, 0x100);

  ///////////////////////////////////////////////////////////////////////////////
	u32 val;
	/* Configure the GPE pins in I2S and Pull-Up mode */
	// Set GPE0~4 pin 
	val = __raw_readl(S3C64XX_GPDPUD);
	val &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6) | (3<<8));
	val |= (2<<0) | (2<<2) | (2<<4) | (2<<6) | (2<<8);
	__raw_writel(val, S3C64XX_GPDPUD);

	val = __raw_readl(S3C64XX_GPDCON);
	val &= ~((0xf<<0) | (0xf<<4) | (0xf<<8) | (0xf<<12) | (0xf<<16));
	val |= (3<<0) | (3<<4) | (3<<8) | (3<<12) | (3<<16);
	__raw_writel(val, S3C64XX_GPDCON);   
  ///////////////////////////////////////////////////////////////////////////////
  
	s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));
	s3cdbg("S3C_IIS_PABASE=0x%x \n",S3C_IIS_PABASE);

//  SetCodecICModeI2C();

  
	/*PCLK & SCLK gating enable*/
	writel(readl(S3C_PCLK_GATE)|S3C_CLKCON_PCLK_IIS0, S3C_PCLK_GATE);
	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_AUDIO0, S3C_SCLK_GATE);

	iismod = readl(regs+S3C64XX_IIS0MOD);
	iismod &=~(0x3<<3);  //set MCLK 256*FS
	iismod &=~(0x3<<1);  //Set BCLK 32*FS

	/*Clear I2S prescaler value [13:8] and disable prescaler*/
	iispsr = readl(regs+S3C64XX_IIS0PSR);
	iispsr &=~((0x3f<<8)|(1<<15));
	writel(iispsr, regs+S3C64XX_IIS0PSR);
		writel(readl(S3C_CLK_DIV0)&(~(1<<4)), S3C_CLK_DIV0);

	s3cdbg("In %s  Line:%d , params = %d \n", __FUNCTION__, __LINE__, params_rate(params));
  
//  //EPLL= 4.0 * Fin
//	writel(0, S3C_EPLL_CON1);
//	writel((1<<31)|(32<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);  //4.000		

	switch (params_rate(params)) 
	{
	case 8000:
	case 12000:
	case 16000:
	case 24000:				
	case 32000:
	case 48000:
	case 64000:
	case 96000:
//			writel(50332, S3C_EPLL_CON1);
//			writel((1<<31)|(32<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);  //49.152

			writel(readl(S3C_MPLL_CON)&(~(0x3ff<<16))&(~(0x3f<<8))&(~(7<<0)), S3C_MPLL_CON);
			writel(readl(S3C_MPLL_CON)|(262<<16)|(8<<8)|(3<<0), S3C_MPLL_CON);//49.125
			break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
//			writel(6903, S3C_EPLL_CON1);
//			writel((1<<31)|(30<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);  //45.1584

				writel(readl(S3C_MPLL_CON)&(~(0x3ff<<16))&(~(0x3f<<8))&(~(7<<0)), S3C_MPLL_CON);
				writel((readl(S3C_MPLL_CON))|(301<<16)|(10<<8)|(3<<0), S3C_MPLL_CON);
			break;
	default:
	//		writel(0, S3C_EPLL_CON1);
	//		writel((1<<31)|(128<<16)|(25<<8)|(0<<0) ,S3C_EPLL_CON0); //5.12
			
			writel(readl(S3C_MPLL_CON)&(~(0x3ff<<16))&(~(0x3f<<8))&(~(7<<0)), S3C_MPLL_CON);
			writel((readl(S3C_MPLL_CON))|(273<<16)|(40<<8)|(4<<0), S3C_MPLL_CON);//5.118
			break;
	}

	s3cdbg("Enter %s, IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
			__FUNCTION__ , readl(regs+S3C64XX_IIS0CON), readl(regs+S3C64XX_IIS0MOD),
			readl(regs+S3C64XX_IIS0FIC), readl(regs+S3C64XX_IIS0PSR));
  
	while(!(__raw_readl(S3C_EPLL_CON0)&(1<<30)));
 
	/* MUXepll : FOUTepll */
	writel(readl(S3C_CLK_SRC)|S3C_CLKSRC_MPLL_CLKSEL, S3C_CLK_SRC);
	/* AUDIO0 sel : FOUTepll */
	writel((readl(S3C_CLK_SRC)&~(0x7<<7))|(1<<7), S3C_CLK_SRC);

	/* AUDIO0 CLK_DIV2 setting */
	writel(readl(S3C_CLK_DIV2)&~(0xf<<8),S3C_CLK_DIV2);
	
	iismod |= S3C64XX_IIS0MOD_256FS;   //Set MCLK=256*Fs
  iismod |= S3C64XX_IIS0MOD_32FS;    //Set BCLK=32*Fs
  
	switch (params_rate(params)) 
	{
	case 8000:
		prescaler = 24;
		break;
	case 11025:
		prescaler = 16;
		break;
	case 12000:
		prescaler = 16;
		break;
	case 16000:
		prescaler = 12;
		break;
	case 22050:
		prescaler = 8;
		break;
	case 24000:
		prescaler = 8;
		break;
	case 32000:
		prescaler = 6;
		break;
	case 44100:
		prescaler = 4;
		break;
	case 48000:
		prescaler = 4;
		break;
	case 64000:
		prescaler = 3;
		break;
	case 88200:
		prescaler = 2;
		break;
	case 96000:
		prescaler = 2;
		break;
	default:
		prescaler = 1;
		break;
	}
  //prescaler = 4;
  
	writel(iismod , regs+S3C64XX_IIS0MOD);

//	 switch (params_rate(params)) 
//	 {
//	   case 8000:
//	   case 16000:
//	   case 48000:
//	   case 96000:
//	   case 11025:
//	   case 22050:
//	   case 44100:
//	         clk = 12000000;
//	         break;
//	 }
   //added for all HZ
   clk = 12000000;
   
   

	/* Choose BFS and RFS values combination that is supported by
	 * both the SSM2602 codec as well as the S3C AP
	 *
	 * SSM2602 codec supports only S16_LE, S20_3LE, S24_LE & S32_LE.
	 * S3C AP supports only S8, S16_LE & S24_LE.
	 * We implement all for completeness but only S16_LE & S24_LE bit-lengths 
	 * are possible for this AP-Codec combination.
	 */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		bfs = 16;
		rfs = 256;		/* Can take any RFS value for AP */
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		rfs = 256;		/* Can take any RFS value for AP */
 		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
 	case SNDRV_PCM_FORMAT_S24_LE:
		bfs = 48;
		rfs = 512;		/* B'coz 48-BFS needs atleast 512-RFS acc to *S5P6440* UserManual */
 		break;
 	case SNDRV_PCM_FORMAT_S32_LE:	/* Impossible, as the AP doesn't support 64fs or more BFS */
	default:
		return -EINVAL;
 	}

  s3cdbg("%s trace 1...\n",__func__);

	s3cdbg("In %s, IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
			__FUNCTION__ , readl(regs+S3C64XX_IIS0CON), readl(regs+S3C64XX_IIS0MOD),
			readl(regs+S3C64XX_IIS0FIC), readl(regs+S3C64XX_IIS0PSR));

       /* set codec DAI configuration */
       /* I2S,Slave,16 Bit, BCLK not inverted */
       ret = codec_dai->ops.set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
       if (ret < 0)
             return ret;

  s3cdbg("%s trace 2...\n",__func__);
       /* set cpu DAI configuration */
       ret = cpu_dai->ops.set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);//SND_SOC_DAIFMT_CBM_CFM);
       if (ret < 0)
             return ret;

  s3cdbg("%s trace 3...\n",__func__);
	     /* set MCLK division for sample rate */
       ret = cpu_dai->ops.set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs );
       if (ret < 0)
             return ret;

//  printk("%s trace 4...\n",__func__);             
//       //Hill Added
//       ret = cpu_dai->ops.set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, params_rate(params), SND_SOC_CLOCK_OUT);
//       if (ret < 0)
//             return ret;       
//
//  printk("%s trace 5...\n",__func__);             
//       ret = cpu_dai->ops.set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO, params_rate(params), SND_SOC_CLOCK_OUT);
//       if (ret < 0)
//             return ret;

       s3cdbg("%s trace 6...\n",__func__);
       ret = codec_dai->ops.set_sysclk(codec_dai, SSM2603_SYSCLK, clk, SND_SOC_CLOCK_IN);
       
       s3cdbg("%s trace 6.0 ret:%d ...\n",__func__,ret);
       if (ret < 0)
       {
       		   s3cdbg("ret<0 quite ...\n");
             return ret;
       }

  s3cdbg("%s trace 7... prescaler:%d \n",__func__,prescaler);
  /* set prescaler division for sample rate */
  ret = cpu_dai->ops.set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, ((prescaler - 1) << 0x8));
  if (ret < 0)
    return 0;
          
	//s3cdbg("Before Leave %s, IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
	//		__FUNCTION__ , readl(regs+S3C64XX_IIS0CON), readl(regs+S3C64XX_IIS0MOD),
	//		readl(regs+S3C64XX_IIS0FIC), readl(regs+S3C64XX_IIS0PSR));          
	//
	//writel(0x0e07,regs+S3C64XX_IIS0CON);
	//writel(0x0006,regs+S3C64XX_IIS0MOD);
	//writel(0x0050|(0x1<<15),regs+S3C64XX_IIS0PSR);	
	
	s3cdbg("Leave %s, IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
			__FUNCTION__ , readl(regs+S3C64XX_IIS0CON), readl(regs+S3C64XX_IIS0MOD),
			readl(regs+S3C64XX_IIS0FIC), readl(regs+S3C64XX_IIS0PSR));
			
	return 0;
}

/*
 * SSM2603 DAI operations.
 */
static struct snd_soc_ops smdk6410_ops = {
	.hw_params = smdk6410_hw_params,
};

static int smdk6410_ssm2603_init(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_dai_link smdk6410_dai[] = 
{
	{
		.name = "ssm2603",
		.stream_name = "ssm2603 Playback",
		.cpu_dai = &s3c_i2s_dai,
		.codec_dai = &ssm2603_dai,
		.init = smdk6410_ssm2603_init,
		.ops = &smdk6410_ops,
	},
};

static struct snd_soc_card smdk6410 = 
{
	.name = "smdk6410",
	.platform= &s3c24xx_soc_platform,	
	.dai_link = smdk6410_dai,
	.num_links = ARRAY_SIZE(smdk6410_dai),
};

static struct ssm2603_setup_data smdk6410_ssm2603_setup = {
	.i2c_address = 0x1a,
};

static struct snd_soc_device smdk6410_snd_devdata = {
	.card =&smdk6410,
	.codec_dev = &soc_codec_dev_ssm2603,
	.codec_data = &smdk6410_ssm2603_setup,
};

static struct platform_device *smdk6410_snd_device;

static int __init smdk6410_audio_init(void)
{
	int ret;
  
	s3cdbg("in %s ...\n",__func__);
  
	smdk6410_snd_device = platform_device_alloc("soc-audio", 0);
	//Hill Modify it
 	//smdk6410_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdk6410_snd_device)
	{
		printk("platform_device_alloc error!\n");
		return -ENOMEM;
	}

	platform_set_drvdata(smdk6410_snd_device, &smdk6410_snd_devdata);
	smdk6410_snd_devdata.dev = &smdk6410_snd_device->dev;
	ret = platform_device_add(smdk6410_snd_device);

  s3cdbg(" platform_device_add complete!\n");
 
	if (ret)
	{
		s3cdbg("in %s if(ret=%d) ...\n",__func__,ret);
		platform_device_put(smdk6410_snd_device);
	}
	else
	{
		s3cdbg("in %s else (ret=%d) ...\n",__func__,ret);
	}
		
	return ret;
}

static void __exit smdk6410_audio_exit(void)
{
	platform_device_unregister(smdk6410_snd_device);
}

module_init(smdk6410_audio_init);
module_exit(smdk6410_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDK6410 ssm2603");
MODULE_LICENSE("GPL");

