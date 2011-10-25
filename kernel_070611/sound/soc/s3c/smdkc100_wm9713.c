/*
 * smdkc100_wm9713.c  --  SoC audio for smdkc100
 *
 * Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 * Author: Graeme Gregory
 *         graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    8th  Mar 2007   Initial version.
 *    20th Sep 2007   Apply at smdkc100
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/io.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-bank-c.h>

#include "../codecs/wm9713.h"
#include "s3c-pcm.h"
#include "s3c-ac97.h"

static struct snd_soc_machine smdkc100;

static struct snd_soc_dai_link smdkc100_dai[] = {
{
	.name = "AC97",
	.stream_name = "AC97 HiFi",
	.cpu_dai = &s3c_ac97_dai[0],
	.codec_dai = &wm9713_dai[WM9713_DAI_AC97_HIFI],
},
};

static struct snd_soc_machine smdkc100 = {
	.name = "SMDKC100",
	.dai_link = smdkc100_dai,
	.num_links = ARRAY_SIZE(smdkc100_dai),
};

static struct snd_soc_device smdkc100_snd_ac97_devdata = {
	.machine = &smdkc100,
	.platform = &s3c24xx_soc_platform,
	.codec_dev = &soc_codec_dev_wm9713,
};

static struct platform_device *smdkc100_snd_ac97_device;

static int __init smdkc100_init(void)
{
	int ret;
	unsigned int val;

	val = __raw_readl(S3C_GPCPUD);
	val &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6) | (3<<8));
	val |= (2<<0) | (2<<2) | (2<<4) | (2<<6) | (2<<8);
	__raw_writel(val, S3C_GPCPUD);

	val = __raw_readl(S3C_GPCCON);
	val &= ~((0xf<<0) | (0xf<<4) | (0xf<<8) | (0xf<<12) | (0xf<<16));
	val |= (4<<0) | (4<<4) | (4<<8) | (4<<12) |  (4<<16);
	__raw_writel(val, S3C_GPCCON);

	smdkc100_snd_ac97_device = platform_device_alloc("soc-audio", -1);
	if (!smdkc100_snd_ac97_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc100_snd_ac97_device,
				&smdkc100_snd_ac97_devdata);
	smdkc100_snd_ac97_devdata.dev = &smdkc100_snd_ac97_device->dev;
	ret = platform_device_add(smdkc100_snd_ac97_device);

	if (ret)
		platform_device_put(smdkc100_snd_ac97_device);

	return ret;
}

static void __exit smdkc100_exit(void)
{
	platform_device_unregister(smdkc100_snd_ac97_device);
}

module_init(smdkc100_init);
module_exit(smdkc100_exit);

/* Module information */
MODULE_AUTHOR("Samsung: Ryu");
MODULE_DESCRIPTION("ALSA SoC WM9713 SMDKC100");
MODULE_LICENSE("GPL");
