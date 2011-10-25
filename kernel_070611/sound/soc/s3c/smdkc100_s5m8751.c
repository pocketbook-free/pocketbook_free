/*
 * smdk6400_s5m8751.c
 *
 * Copyright (C) 2009, Samsung Elect. Ltd. - Jaswinder Singh <jassisinghbrar@gmail.com>
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

#include "../codecs/s5m8751.h"
#include "s3c-pcm.h"

#include "s3c-i2s.h"

#define SRC_CLK	s3c_i2s_get_clockrate()

/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
/* XXX RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int smdkc100_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int bfs, rfs, psr, ret;

#if 1
	writel(readl(S5P_CLKGATE_D20)|S5P_CLKGATE_D20_HCLKD2|S5P_CLKGATE_D20_I2SD2,S5P_CLKGATE_D20);
	writel(readl(S5P_LPMP_MODE_SEL) | (1<<1), S5P_LPMP_MODE_SEL);
#endif

	/* Choose BFS and RFS values combination that is supported by
	 * both the S5M8751 codec as well as the S5PC100 AP
	 *
	 * S5M8751 codec supports only S16_LE, S18_3LE, S20_3LE & S24_LE.
	 * S5PC100 AP supports only S8, S16_LE & S24_LE.
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
	case SNDRV_PCM_FORMAT_S18_3LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
 	case SNDRV_PCM_FORMAT_S24_LE:
		bfs = 48;
		rfs = 512;		/* B'coz 48-BFS needs atleast 512-RFS acc to *S5P6440* UserManual */
					/* And S5P6440 uses the same I2S IP as S3C */
 		break;
	default:
		return -EINVAL;
 	}
 
	/* Select the AP Sysclk */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, params_rate(params), SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

#ifdef USE_CLKAUDIO
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO, params_rate(params), SND_SOC_CLOCK_OUT);
#else
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_PCLK, 0, SND_SOC_CLOCK_OUT);
#endif
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the AP RFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs);
	if (ret < 0)
		return ret;

	/* Set the AP BFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	switch (params_rate(params)) {
	case 8000:
	case 11025:
	case 16000:
	case 22050:
	case 32000:
	case 44100: 
	case 48000:
	case 64000:
	case 88200:
	case 96000:
		psr = SRC_CLK / rfs / params_rate(params);
		ret = SRC_CLK / rfs - psr * params_rate(params);
		if(ret >= params_rate(params)/2)	// round off
		   psr += 1;
		psr -= 1;
		break;
	default:
		return -EINVAL;
	}

	//printk("SRC_CLK=%d PSR=%d RFS=%d BFS=%d\n", SRC_CLK, psr, rfs, bfs);

	/* Set the AP Prescalar */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, psr);
	if (ret < 0)
		return ret;

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the Codec BCLK(no option to set the MCLK) */
	ret = snd_soc_dai_set_clkdiv(codec_dai, S5M8751_BCLK, bfs);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * S5M8751 DAI operations.
 */
static struct snd_soc_ops smdkc100_ops = {
	.hw_params = smdkc100_hw_params,
};

static const struct snd_soc_dapm_widget s5m8751_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("I2S Front Jack", NULL),
	SND_SOC_DAPM_LINE("I2S Center Jack", NULL),
	SND_SOC_DAPM_LINE("I2S Rear Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
};

/* example machine audio_mapnections */
static const struct snd_soc_dapm_route audio_map[] = {

	{ "I2S Front Jack", NULL, "VOUT1L" },
	{ "I2S Front Jack", NULL, "VOUT1R" },

	{ "I2S Center Jack", NULL, "VOUT2L" },
	{ "I2S Center Jack", NULL, "VOUT2R" },

	{ "I2S Rear Jack", NULL, "VOUT3L" },
	{ "I2S Rear Jack", NULL, "VOUT3R" },

	{ "AINL", NULL, "Line In Jack" },
	{ "AINR", NULL, "Line In Jack" },
		
};

static int smdkc100_s5m8751_init(struct snd_soc_codec *codec)
{
	int i;

	/* Add smdkc100 specific widgets */
	snd_soc_dapm_new_controls(codec, s5m8751_dapm_widgets,ARRAY_SIZE(s5m8751_dapm_widgets));

	/* set up smdkc100 specific audio paths */
	snd_soc_dapm_add_routes(codec, audio_map,ARRAY_SIZE(audio_map));

	/* No jack detect - mark all jacks as enabled */
	for (i = 0; i < ARRAY_SIZE(s5m8751_dapm_widgets); i++)
		snd_soc_dapm_enable_pin(codec, s5m8751_dapm_widgets[i].name);

	snd_soc_dapm_sync(codec);

	return 0;
}

static struct snd_soc_dai_link smdkc100_dai[] = {
{
	.name = "S5M8751",
	.stream_name = "S5M8751 Playback",
	.cpu_dai = &s3c_i2s_dai,
	.codec_dai = &s5m8751_dai,
	.init = smdkc100_s5m8751_init,
	.ops = &smdkc100_ops,
},
};

static struct snd_soc_machine smdkc100 = {
	.name = "smdkc100",
	.dai_link = smdkc100_dai,
	.num_links = ARRAY_SIZE(smdkc100_dai),
};

static struct s5m8751_setup_data smdkc100_s5m8751_setup = {
	.i2c_address = 0x68,
};

static struct snd_soc_device smdkc100_snd_devdata = {
	.machine = &smdkc100,
	.platform = &s3c24xx_soc_platform,
	.codec_dev = &soc_codec_dev_s5m8751,
	.codec_data = &smdkc100_s5m8751_setup,
};

static struct platform_device *smdkc100_snd_device;

static int __init smdkc100_audio_init(void)
{
	int ret;

	smdkc100_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc100_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc100_snd_device, &smdkc100_snd_devdata);
	smdkc100_snd_devdata.dev = &smdkc100_snd_device->dev;
	ret = platform_device_add(smdkc100_snd_device);

	if (ret)
		platform_device_put(smdkc100_snd_device);
	
	return ret;
}

static void __exit smdkc100_audio_exit(void)
{
	platform_device_unregister(smdkc100_snd_device);
}

module_init(smdkc100_audio_init);
module_exit(smdkc100_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC100 S5M8751");
MODULE_LICENSE("GPL");
