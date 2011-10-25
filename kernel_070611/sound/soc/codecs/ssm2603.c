/*
 * File:         sound/soc/codecs/ssm2603.c
 * Author:       Cliff Cai <Cliff.Cai@analog.com>
 *
 * Created:      Tue June 06 2008
 * Description:  Driver for ssm2603 sound chip
 *
 * Modified:
 *               Copyright 2008 Analog Devices Inc.
 *
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "ssm2603.h"

#define SSM2603_VERSION "0.1"

#ifdef CONFIG_STOP_MODE_SUPPORT
#include <linux/earlysuspend.h>
struct platform_device *g_soc_pdev=NULL;
#endif
//#define CODEC_DEBUG
#ifdef CODEC_DEBUG
#define dprintk(x...) printk(x)
#else
#define dprintk(x...)
#endif
extern void SpeakerMute(u8 mute);
extern int headset_state(void);

#define SSM2603_HP_RAMP_UP  0x27
#define SSM2603_HP_RAMP_DW  0x37
#define SSM2603_HP_VOL      0x7F
#define SSM2603_HP_VOL_ADDR 0x02

struct snd_soc_codec_device soc_codec_dev_ssm2603;
struct snd_soc_codec *g_codec;

struct SND_Headphone *psnd_hp;

/* codec private data */
struct ssm2603_priv {
	unsigned int sysclk;
	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

/*
 * ssm2603 register cache
 * We can't read the ssm2603 register space when we are
 * using 2 wire for device control, so we cache them instead.
 * There is no point in caching the reset register
 */
static const u16 ssm2603_reg[SSM2603_CACHEREGNUM] = {
	0x0017, 0x0017, 0x002e, 0x002e,
//	0x0017, 0x0017, 0x0062, 0x0062,
	0x0000, 0x0000, 0x0000, 0x000a,    //
	0x0000, 0x0000
};

unsigned short powerreg;			//William add 20101010 
unsigned short activereg;			
/*
 * read ssm2603 register cache
 */
static inline unsigned int ssm2603_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if (reg == SSM2603_RESET)
		return 0;
	if (reg >= SSM2603_CACHEREGNUM)
		return -1;
	return cache[reg];
}

/*
 * write ssm2603 register cache
 */
static inline void ssm2603_write_reg_cache(struct snd_soc_codec *codec,
	u16 reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg >= SSM2603_CACHEREGNUM)
		return;
	cache[reg] = value;
}

/*
 * write to the ssm2603 register space
 */
static int ssm2603_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[2];

	/* data is
	 *   D15..D9 ssm2603 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	data[1] = value & 0x00ff;

	ssm2603_write_reg_cache(codec, reg, value);
	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}

#define ssm2603_reset(c)	ssm2603_write(c, SSM2603_RESET, 0)


//*******************************************************************

#define HP_VOL_ADDR   0x2
#define HP_MUTE_VOL   0x2E
#define HP_UMUTE_VOL  0x79
#define HP_VOL_LR_UPD  (0x1<<8)
/*
 * Ramp OUT1 PGA volume to minimise pops at stream startup and shutdown.
 */
static int ssm2603_hp_ramp_step(int ramp)
{
	u16 reg, val;

	if (ramp == SND_RAMP_UP) 
	{
		  for(val=HP_MUTE_VOL;val<=HP_UMUTE_VOL;val+=1)
			{
							ssm2603_write(g_codec,HP_VOL_ADDR, val | HP_VOL_LR_UPD);	 
							//schedule_timeout_interruptible(msecs_to_jiffies(1));
							udelay(50);
			}
			if(val>HP_UMUTE_VOL)
						ssm2603_write(g_codec,HP_VOL_ADDR, HP_UMUTE_VOL |HP_VOL_LR_UPD);	 				  
			//schedule_timeout_interruptible(msecs_to_jiffies(2));
			udelay(50);
	} 
	else if (ramp == SND_RAMP_DOWN) 
	{
		  for(val=HP_UMUTE_VOL;val>=HP_MUTE_VOL;val-=1)
			{
							ssm2603_write(g_codec,HP_VOL_ADDR, val | HP_VOL_LR_UPD);	 
							//schedule_timeout_interruptible(msecs_to_jiffies(1));
							udelay(50);
			}
			if(val<HP_MUTE_VOL)
						ssm2603_write(g_codec,HP_VOL_ADDR, HP_MUTE_VOL | HP_VOL_LR_UPD);	 				  	
			//schedule_timeout_interruptible(msecs_to_jiffies(2));						
			udelay(50);
	} 
	else
		return 1;

	return 0;
}
#if 0
int ssm2603_mute_headphone(int mute)
{	
 
   udelay(20);	
   //播放一個中間音量
   ssm2603_write(g_codec,HP_VOL_ADDR,( (HP_MUTE_VOL+HP_UMUTE_VOL)/2) | 0x1<<8);	 	
   udelay(20);	

	 if(mute)  //1 mute
	   return ssm2603_write(g_codec,HP_VOL_ADDR, HP_MUTE_VOL  | 0x1<<8);
	 else      //0 umut
	 	 return ssm2603_write(g_codec,HP_VOL_ADDR,HP_UMUTE_VOL | 0x1<<8);
}
#else
int ssm2603_mute_headphone(int mute)
{	  
	 if(mute)  //1 mute
	 {
	 	   return ssm2603_hp_ramp_step(SND_RAMP_DOWN);
	 }
	 else      //0 umut
	 {
	 		 return ssm2603_hp_ramp_step(SND_RAMP_UP);
	 }
}
#endif

EXPORT_SYMBOL_GPL(ssm2603_mute_headphone);

static void ssm2603_pga_work(struct work_struct *work)
{

	struct SND_Headphone *snd_hp = container_of(work, struct SND_Headphone, delayed_work.work);
	
	printk("[HP RAMP] ssm2603_pag_work, snd_hp->ramp:%s~\n",snd_hp->ramp==SND_RAMP_UP?"SND_RAMP_UP":"SND_RAMP_DOWN");
	switch(snd_hp->ramp)
	{
	 case 	SND_RAMP_UP:
	 	      ssm2603_mute_headphone(SND_RAMP_UP);
	 	      break;
	 case 	SND_RAMP_DOWN:
	 	      ssm2603_mute_headphone(SND_RAMP_DOWN);
	 	      break;	 		
	} 	  		
}

void shedule_mute_headphone_work(int mute)
{
	         //RAMP up Headphone
	   if(mute==SND_RAMP_UP)
	   {	
		  	 psnd_hp->ramp = SND_RAMP_UP;
				 if (!delayed_work_pending(&psnd_hp->delayed_work))
						schedule_delayed_work(&psnd_hp->delayed_work, msecs_to_jiffies(1));	  
		 }
		 else if(mute==SND_RAMP_DOWN)
		 {
		     //RAMP down Headphone  		 
		  	 psnd_hp->ramp = SND_RAMP_DOWN;
				 if (!delayed_work_pending(&psnd_hp->delayed_work))
						schedule_delayed_work(&psnd_hp->delayed_work, msecs_to_jiffies(1));	 		  	
		 }
		 return;
}
EXPORT_SYMBOL_GPL(shedule_mute_headphone_work);
//*******************************************************************

/*Appending several "None"s just for OSS mixer use*/
static const char *ssm2603_input_select[] = {
	"Line", "Mic", "None", "None", "None",
	"None", "None", "None",
};

static const char *ssm2603_deemph[] = {"None", "32Khz", "44.1Khz", "48Khz"};

static const struct soc_enum ssm2603_enum[] = {
	SOC_ENUM_SINGLE(SSM2603_APANA, 2, 2, ssm2603_input_select),
	SOC_ENUM_SINGLE(SSM2603_APDIGI, 1, 4, ssm2603_deemph),
};

static const struct snd_kcontrol_new ssm2603_snd_controls[] = {

SOC_DOUBLE_R("Master Playback Volume", SSM2603_LOUT1V, SSM2603_ROUT1V,	0, 120, 0), //127(0x7F)->120(0x78) reduce the power of HP
SOC_DOUBLE_R("Master Playback ZC Switch", SSM2603_LOUT1V, SSM2603_ROUT1V, 7, 1, 0),

SOC_DOUBLE_R("Capture Volume", SSM2603_LINVOL, SSM2603_RINVOL, 0, 31, 0),
SOC_DOUBLE_R("Capture Switch", SSM2603_LINVOL, SSM2603_RINVOL, 7, 1, 1),

SOC_SINGLE("Mic Boost (+20dB)", SSM2603_APANA, 0, 1, 0),
SOC_SINGLE("Mic Switch", SSM2603_APANA, 1, 1, 1),

SOC_SINGLE("Sidetone Playback Volume", SSM2603_APANA, 6, 3, 1),

SOC_SINGLE("ADC High Pass Filter Switch", SSM2603_APDIGI, 0, 1, 1),
SOC_SINGLE("Store DC Offset Switch", SSM2603_APDIGI, 4, 1, 0),

SOC_ENUM("Capture Source", ssm2603_enum[0]),

SOC_ENUM("Playback De-emphasis", ssm2603_enum[1]),
};

/* add non dapm controls */
static int ssm2603_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(ssm2603_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&ssm2603_snd_controls[i], codec, NULL));
		if (err < 0)
			return err;
	}

	return 0;
}

/* Output Mixer */
static const struct snd_kcontrol_new ssm2603_output_mixer_controls[] = {
SOC_DAPM_SINGLE("Line Bypass Switch", SSM2603_APANA, 3, 1, 0),
SOC_DAPM_SINGLE("Mic Sidetone Switch", SSM2603_APANA, 5, 1, 0),
SOC_DAPM_SINGLE("HiFi Playback Switch", SSM2603_APANA, 4, 1, 0),
};

/* Input mux */
static const struct snd_kcontrol_new ssm2603_input_mux_controls =
SOC_DAPM_ENUM("Input Select", ssm2603_enum[0]);

static const struct snd_soc_dapm_widget ssm2603_dapm_widgets[] = {
SND_SOC_DAPM_MIXER("Output Mixer", SSM2603_PWR, 4, 1, &ssm2603_output_mixer_controls[0], 	ARRAY_SIZE(ssm2603_output_mixer_controls)), //#
SND_SOC_DAPM_DAC("DAC", "HiFi Playback", SSM2603_PWR, 3, 1),
SND_SOC_DAPM_OUTPUT("LOUT"),
SND_SOC_DAPM_OUTPUT("LHPOUT"),
SND_SOC_DAPM_OUTPUT("ROUT"),
SND_SOC_DAPM_OUTPUT("RHPOUT"),
SND_SOC_DAPM_ADC("ADC", "HiFi Capture", SSM2603_PWR, 2, 1),
SND_SOC_DAPM_MUX("Input Mux", SND_SOC_NOPM, 0, 0, &ssm2603_input_mux_controls), //#
SND_SOC_DAPM_PGA("Line Input", SSM2603_PWR, 0, 1, NULL, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias", SSM2603_PWR, 1, 1),
SND_SOC_DAPM_INPUT("MICIN"),
SND_SOC_DAPM_INPUT("RLINEIN"),
SND_SOC_DAPM_INPUT("LLINEIN"),
};

static const struct snd_soc_dapm_route audio_conn[] = {
	/* output mixer */
	{"Output Mixer", "Line Bypass Switch", "Line Input"},
	{"Output Mixer", "HiFi Playback Switch", "DAC"},
	{"Output Mixer", "Mic Sidetone Switch", "Mic Bias"},

	/* outputs */
	{"RHPOUT", NULL, "Output Mixer"},
	{"ROUT", NULL, "Output Mixer"},
	{"LHPOUT", NULL, "Output Mixer"},
	{"LOUT", NULL, "Output Mixer"},

	/* input mux */
	{"Input Mux", "Line", "Line Input"},
	{"Input Mux", "Mic", "Mic Bias"},
	{"ADC", NULL, "Input Mux"},

	/* inputs */
	{"Line Input", NULL, "LLINEIN"},
	{"Line Input", NULL, "RLINEIN"},
	{"Mic Bias", NULL, "MICIN"},
};

static int ssm2603_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, ssm2603_dapm_widgets,
				  ARRAY_SIZE(ssm2603_dapm_widgets));

	snd_soc_dapm_add_routes(codec, audio_conn, ARRAY_SIZE(audio_conn));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}

struct _coeff_div {
	u32 mclk;
	u32 rate;
	u16 fs;
	u8 sr:4;
	u8 bosr:1;
	u8 usb:1;
};
#if 1
/* codec mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
	/* 8k */
	{12288000, 8000, 1536, 0x3, 0x0, 0x0},
	{18432000, 8000, 2304, 0x3, 0x1, 0x0},
	{11289600, 8000, 1408, 0xb, 0x0, 0x0},
	{16934400, 8000, 2112, 0xb, 0x1, 0x0},
	{12000000, 8000, 1500, 0x3, 0x0, 0x0},
  
  /* 8.02k */
	{12000000, 8020, 1496, 0xb, 0x0, 0x0},

  /* 11.0259k */
  {12000000, 11000, 1088, 0xc, 0x0, 0x0},  
  {12000000, 11025, 1088, 0xc, 0x0, 0x0},  
  
  /* 12k */ 
  {12000000, 12000, 1000, 0x8, 0x0, 0x0},  
    
  /* 16k */
  //mclk		 rate		fs   sr   bosr usb	
  {12000000, 16000, 750, 0xa, 0x0, 0x0},
  //{12000000, 16000, 750, 0xa, 0x0, 0x0},

  /* 22.0588k */
  {12000000, 22050, 544, 0xd, 0x0, 0x0},

  /* 24k */
  {12000000, 24000, 500, 0xe, 0x0, 0x0},

	/* 32k */
	{12288000, 32000, 384, 0x6, 0x0, 0x0},
	{18432000, 32000, 576, 0x6, 0x1, 0x0},
	{12000000, 32000, 375, 0x6, 0x0, 0x0},

	/* 44.1k */
	{11289600, 44100, 256, 0x8, 0x0, 0x0},
	{16934400, 44100, 384, 0x8, 0x1, 0x0},
	//{12000000, 44100, 272, 0x8, 0x1, 0x1},
	{12000000, 44100, 272, 0xa, 0x0, 0x0},

	/* 48k */
	{12288000, 48000, 256, 0x0, 0x0, 0x0},
	{18432000, 48000, 384, 0x0, 0x1, 0x0},
	{12000000, 48000, 250, 0x0, 0x0, 0x0},

	/* 88.2k */
	{11289600, 88200, 128, 0xf, 0x0, 0x0},
	{16934400, 88200, 192, 0xf, 0x1, 0x0},
	{12000000, 88200, 136, 0xf, 0x1, 0x1},

	/* 96k */
	{12288000, 96000, 128, 0x7, 0x0, 0x0},
	{18432000, 96000, 192, 0x7, 0x1, 0x0},
	{12000000, 96000, 125, 0x7, 0x0, 0x1},
};
#else    //original config 20101009

static const struct _coeff_div coeff_div[] = 
{	/* 48k */	
{12288000, 48000, 256, 0x0, 0x0, 0x0},	
{18432000, 48000, 384, 0x0, 0x1, 0x0},	
{12000000, 48000, 250, 0x0, 0x0, 0x1},	/* 32k */	
{12288000, 32000, 384, 0x6, 0x0, 0x0},	
{18432000, 32000, 576, 0x6, 0x1, 0x0},	
{12000000, 32000, 375, 0x6, 0x0, 0x1},	/* 8k */	
{12288000, 8000, 1536, 0x3, 0x0, 0x0},	
{18432000, 8000, 2304, 0x3, 0x1, 0x0},	
{11289600, 8000, 1408, 0xb, 0x0, 0x0},	
{16934400, 8000, 2112, 0xb, 0x1, 0x0},	
{12000000, 8000, 1500, 0x3, 0x0, 0x1},	/* 96k */	
{12288000, 96000, 128, 0x7, 0x0, 0x0},	
{18432000, 96000, 192, 0x7, 0x1, 0x0},	
{12000000, 96000, 125, 0x7, 0x0, 0x1},	/* 44.1k */	
{11289600, 44100, 256, 0x8, 0x0, 0x0},	
{16934400, 44100, 384, 0x8, 0x1, 0x0},	
{12000000, 44100, 272, 0x8, 0x1, 0x1},	/* 88.2k */	
{11289600, 88200, 128, 0xf, 0x0, 0x0},	
{16934400, 88200, 192, 0xf, 0x1, 0x0},	
{12000000, 88200, 136, 0xf, 0x1, 0x1},
};


#endif

static inline int get_coeff(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}
	dprintk("i=%d \n",i);
	return i;
}

static int ssm2603_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	u16 srate;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct ssm2603_priv *ssm2603 = codec->private_data;
	struct i2c_client *i2c = codec->control_data;
	u16 iface = ssm2603_read_reg_cache(codec, SSM2603_IFACE) & 0xfff3;
	int i = get_coeff(ssm2603->sysclk, params_rate(params));

  printk("[DEBUG] ssm2603_hw_params ~\n");
  dprintk("ssm2603->sysclk= %d params_rate(params)=%d...\n",ssm2603->sysclk,params_rate(params));
  dprintk("Enter %s  params=%d ...\n",__func__,params_rate(params));

	if (substream == ssm2603->slave_substream) {
		dev_dbg(&i2c->dev, "Ignoring hw_params for slave substream\n");
		return 0;
	}
  
	/*no match is found*/
	if (i >= ARRAY_SIZE(coeff_div))
	{
	  printk("i == ARRAY_SIZE(coeff_div) error quit!...\n");
		return -EINVAL;
  }
  
	srate = (coeff_div[i].sr << 2) |
		(coeff_div[i].bosr << 1) | coeff_div[i].usb;

	ssm2603_write(codec, SSM2603_ACTIVE, 0);
	ssm2603_write(codec, SSM2603_SRATE, srate);

	/* bit size */
	switch (params_format(params))
	{
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		iface |= 0x0004;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		iface |= 0x0008;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		iface |= 0x000c;
		break;
	}
	
	ssm2603_write(codec, SSM2603_IFACE, iface);
	mdelay(100);		//William add 20101010
	printk("**********ACTIVE_ACTIVATE_CODEC**********\n");
	ssm2603_write(codec, SSM2603_ACTIVE, ACTIVE_ACTIVATE_CODEC);
  dprintk("In %s SSM2603_PWR:0x%x \n",__func__,ssm2603_read_reg_cache(codec, SSM2603_IFACE));
  dprintk("Leave %s  params=%d ...\n",__func__,params_rate(params));	
	return 0;
}

static int ssm2603_startup(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct ssm2603_priv *ssm2603 = codec->private_data;
	struct i2c_client *i2c = codec->control_data;
	struct snd_pcm_runtime *master_runtime;

  dprintk("Enter %s  ...\n",__func__);

	/* The DAI has shared clocks so if we already have a playback or
	 * capture going then constrain this substream to match it.
	 * TODO: the ssm2603 allows pairs of non-matching PB/REC rates
	 */
	if (ssm2603->master_substream) 
	{
		master_runtime = ssm2603->master_substream->runtime;
		//dev_dbg(&i2c->dev, 
			printk("Constraining to %d bits at %dHz\n",
			master_runtime->sample_bits,
			master_runtime->rate);

		snd_pcm_hw_constraint_minmax(substream->runtime,
					     SNDRV_PCM_HW_PARAM_RATE,
					     master_runtime->rate,
					     master_runtime->rate);

		snd_pcm_hw_constraint_minmax(substream->runtime,
					     SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
					     master_runtime->sample_bits,
					     master_runtime->sample_bits);

		ssm2603->slave_substream = substream;
	} else
		ssm2603->master_substream = substream;

	return 0;
}

static int ssm2603_pcm_prepare(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	u16 reg;
#if 1
  ssm2603_write(codec, SSM2603_ACTIVE, 0);
	mdelay(100);
	/* set active */
	printk("%s =========ACTIVE_ACTIVATE_CODEC=======\n", __FUNCTION__);
	ssm2603_write(codec, 0x2, 0x1<<8 | 0x2e);

  //Added for power on ssm2602's DAC,OUT power.
  reg=ssm2603_read_reg_cache(codec,SSM2603_PWR);
  reg &=~(PWR_OUT_PDN | PWR_DAC_PDN);
  ssm2603_write(codec, SSM2603_PWR, reg);
	//PWR_OUT_PDN PWR_DAC_PDN 
	
	udelay(100);

	ssm2603_write(codec, SSM2603_ACTIVE, ACTIVE_ACTIVATE_CODEC);
	//mdelay(500);			///William add 20101010
	printk("[AFTER] ACTIVE_ACTIVATE_CODEC=======\n");
  
	
#endif
	return 0;
}

static void ssm2603_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct ssm2603_priv *ssm2603 = codec->private_data;
	
	dprintk("enter %s \n",__func__);
	//&&&&&&&&&& Hill mark it
	///* deactivate */
	if (!codec->active)
		ssm2603_write(codec, SSM2603_ACTIVE, 0);
        ////////////////////////////////////////////////////////////////
	//Added for power on ssm2602's DAC,OUT power.
	  
	u16 reg;
	reg=ssm2603_read_reg_cache(codec,SSM2603_PWR);
	reg &=~(PWR_OUT_PDN | PWR_DAC_PDN | PWR_POWER_OFF);
	ssm2603_write(codec, SSM2603_PWR, reg);
		        
	//PWR_OUT_PDN PWR_DAC_PDN 
        ////////////////////////////////////////////////////////////////

  flush_work(&psnd_hp->delayed_work);

	if (ssm2603->master_substream == substream)
		ssm2603->master_substream = ssm2603->slave_substream;

	ssm2603->slave_substream = NULL;
}

//static int ssm2603_hp_ramp_step(struct snd_soc_codec *codec, u8 ramp)
//{
//	u8 vol;
//	if(ramp==SSM2603_HP_RAMP_UP)
//	{
//		for(vol=0x2f;vol<=0x7f;vol+=8)
//		{
//		   ssm2603_write(codec, SSM2603_HP_VOL_ADDR, vol|(0x1<<8) );
//		   udelay(20);
//		   
//		   if(vol+8>0x7F)
//		   {
//		   		  ssm2603_write(codec, SSM2603_HP_VOL_ADDR, 0x7f|(0x1<<8) );
//		   		  break;
//		   }
//		}       
//	}
//	else if(ramp==SSM2603_HP_RAMP_DW)
//	{
//		for(vol=0x7F;vol>=0x2f;vol-=8)
//		{
//		   ssm2603_write(codec, SSM2603_HP_VOL_ADDR, vol|(0x1<<8) );
//		   udelay(20);
//		   
//		   if(vol-8<0x2f)
//		   {
//		   		  ssm2603_write(codec, SSM2603_HP_VOL_ADDR, 0x2f|(0x1<<8) );		   	
//		   		  break;
//		   }
//		}
//	}	
//	
//	return 0;
//}

/*
#define SSM2603_HP_RAMP_UP  0x27
#define SSM2603_HP_RAMP_DW  0x37
#define SSM2603_HP_VOL      0x7F
#define SSM2603_HP_VOL_ADDR 0x02
static int ssm2603_hp_ramp_step(struct snd_soc_codec *codec, u8 ramp)
*/
#if 0
static int ssm2603_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = ssm2603_read_reg_cache(codec, SSM2603_APDIGI) & ~APDIGI_ENABLE_DAC_MUTE;
	
	printk("Enter %s mute:%d ...\n",__func__,mute);
	
	if (mute)   //1: mute devices
	{
	  SpeakerMute(SND_SPK_MUTE);     //Close Speaker
    ssm2603_mute_headphone(SND_RAMP_DOWN); //disable headphone 	
	  udelay(100);
	      
    ssm2603_hp_ramp_step(codec,SSM2603_HP_RAMP_DW);     //ramp down hp vol
		ssm2603_write(codec, SSM2603_APDIGI, mute_reg | APDIGI_ENABLE_DAC_MUTE);
	}
	else       //0: unmute devices
	{
		SpeakerMute(SND_SPK_MUTE);
		udelay(100);
		  		
		ssm2603_write(codec, SSM2603_APDIGI, mute_reg);

	  if(!headset_state())  //HP not exist, use speaker
	  {
		  udelay(100);
 		  SpeakerMute(SND_SPK_UMUTE);     //Use Speaker
 		}
    ssm2603_hp_ramp_step(codec,SSM2603_HP_RAMP_UP);     //ramp up hp vol
	}
	return 0;
}
#else
static int ssm2603_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = ssm2603_read_reg_cache(codec, SSM2603_APDIGI) & ~APDIGI_ENABLE_DAC_MUTE;
	if (mute)
	{	  
	  SpeakerMute(SND_SPK_MUTE);     //Close Speaker
	  udelay(100);
	  
	  //&&&&&&&&&& Hill mark it 
		ssm2603_write(codec, SSM2603_APDIGI, mute_reg | APDIGI_ENABLE_DAC_MUTE);
	}
	else
		ssm2603_write(codec, SSM2603_APDIGI, mute_reg);
	return 0;
}

#endif

static int ssm2603_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
  dprintk("enter %s EINVAL:%d freq:%d ...\n",__func__,-EINVAL,freq);

	struct snd_soc_codec *codec = codec_dai->codec;
	struct ssm2603_priv *ssm2603 = codec->private_data;

	switch (freq) 
	{
	case 11289600:
	case 12000000:
	case 12288000:
	case 16934400:
	case 18432000:
		dprintk("in %s ...1\n",__func__);
		ssm2603->sysclk = freq;
		dprintk("in %s ...2\n",__func__);
		return 0;
	}
	return -EINVAL;
}

static int ssm2603_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = 0;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) 
	{
	case SND_SOC_DAIFMT_CBM_CFM:
		iface |= 0x0040;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 0x0002;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= 0x0001;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= 0x0013;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface |= 0x0003;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x0090;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x0080;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x0010;
		break;
	default:
		return -EINVAL;
	}

	/* set iface */
	ssm2603_write(codec, SSM2603_IFACE, iface);
	return 0;
}

static int ssm2603_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	dprintk("Enter %s  cmd:%d \n", __FUNCTION__, cmd);

	switch (cmd) 
	{
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		  if(!headset_state())  //If not exist HP
		  {
		     printk("[SP]trigger on~\n");
         SpeakerMute(SND_SPK_UMUTE); //enable speaker amplifier	
         udelay(100);
      }				
      else
      {
      	  SpeakerMute(SND_SPK_MUTE);
         //ssm2603_mute_headphone(SND_RAMP_UP); //enable headphone amplifier	         
         //RAMP up Headphone  	         
				 shedule_mute_headphone_work(SND_RAMP_UP);
					
         udelay(100);
		  	  printk("[HP]trigger on~\n");
      }
			break;
			
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
 	  	 printk("pcm trigger off~\n");
  		 SpeakerMute(SND_SPK_MUTE); //disable speaker amplifier
  		 //ssm2603_mute_headphone(SND_RAMP_DOWN); //disable headphone amplifier  		   		 
       //RAMP down Headphone
			 shedule_mute_headphone_work(SND_RAMP_DOWN);			
  		 udelay(100);
			 break;
	default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
static int ssm2603_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	u16 reg = ssm2603_read_reg_cache(codec, SSM2603_PWR) & 0xff7f;
	printk("%s level = %d reg is 0x%04x~\n", __FUNCTION__, level, reg);
	switch (level) {
	case SND_SOC_BIAS_ON:
		/* vref/mid, osc on, dac unmute */
		ssm2603_write(codec, SSM2603_PWR, reg);
		break;
	case SND_SOC_BIAS_PREPARE:
		printk("[MUTE_HP] SND_SOC_BIAS_PREPARE ...\n");
		ssm2603_write(codec, SSM2603_LOUT1V, LINVOL_LRIN_BOTH | 0x2E );
		break;
	case SND_SOC_BIAS_STANDBY:
		/* everything off except vref/vmid, */
		//ssm2603_write(codec, SSM2603_PWR, reg | PWR_CLK_OUT_PDN);
		break;
	case SND_SOC_BIAS_OFF:
		printk("[MUTE_HP] SND_SOC_BIAS_OFF ...\n");
		
		//&&&&&&&&&& Hill mark it
		ssm2603_write(codec, SSM2603_LOUT1V, LINVOL_LRIN_BOTH | 0x2E );
		udelay(100);	
		///* everything off, dac mute, inactive */
		ssm2603_write(codec, SSM2603_ACTIVE, 0);
		//ssm2603_write(codec, SSM2603_PWR, 0xffff);
		ssm2603_write(codec, SSM2603_PWR, 0x7f);
		break;

	}
	codec->bias_level = level;
	return 0;
}

#define SSM2603_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
		SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
		SNDRV_PCM_RATE_96000)

#define SSM2603_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
		SNDRV_PCM_FMTBIT_S24_LE )//| SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_dai ssm2603_dai = {
	.name = "SSM2603",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SSM2603_RATES,
		.formats = SSM2603_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SSM2603_RATES,
		.formats = SSM2603_FORMATS,},
	.ops = {
		.startup = ssm2603_startup,
		.prepare = ssm2603_pcm_prepare,
		.hw_params = ssm2603_hw_params,
		.shutdown = ssm2603_shutdown,
		.digital_mute = ssm2603_mute,
		.set_sysclk = ssm2603_set_dai_sysclk,
		.set_fmt = ssm2603_set_dai_fmt,
		.trigger = ssm2603_trigger,
	}
};
EXPORT_SYMBOL_GPL(ssm2603_dai);

static int ssm2603_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	u16 *cache = codec->reg_cache;
	printk("=======================>%s\n", __FUNCTION__);
  shedule_mute_headphone_work(SND_RAMP_DOWN); //防止 resume 時 headphone 的 pop 音  
  SpeakerMute(SND_SPK_MUTE);                  //防止 resume 時 speaker 的 pop 音
  udelay(100);
  powerreg  = cache[SSM2603_PWR];
	activereg =	cache[SSM2603_ACTIVE];
	printk("cache[SSM2603_PWR] 0x%04x \n", cache[SSM2603_PWR]);
	printk("cache[SSM2603_ACTIVE] 0x%04x \n", cache[SSM2603_ACTIVE]);
	ssm2603_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int ssm2603_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	int i;
	u8 data[2];
	u16 *cache = codec->reg_cache;
	unsigned short regtmp;
  printk("##############>%s \n",__FUNCTION__);
  //SpeakerMute(1);
  //udelay(100);
	printk("powerreg is 0x%04x \n", powerreg);
	printk("activereg is 0x%04x \n", activereg);
	
	powerreg=0x07;  //force to write to pwr regisger, poweron need unit
	
	ssm2603_write(codec, SSM2603_PWR, powerreg);
	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(ssm2603_reg); i++) 
	{
		data[0] = (i << 1) | ((cache[i] >> 8) & 0x0001);
		data[1] = cache[i] & 0x00ff;
		if(i == SSM2603_PWR)				/// William add 20101010
		{
			data[1] = cache[i] | (0x1<<4);
		}
		else if(i == SSM2603_ACTIVE)
		{
			continue;
		}
		//printk("i = %d \n", i);
		codec->hw_write(codec->control_data, data, 2);
	}

	mdelay(100);
	ssm2603_write(codec, SSM2603_ACTIVE, activereg);
	ssm2603_write(codec, SSM2603_PWR, powerreg);
	
	ssm2603_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	ssm2603_set_bias_level(codec, codec->suspend_bias_level);
	printk("codec->suspend_bias_level is %d \n", codec->suspend_bias_level);
//避免 speaker 先於 Codec 打開 	
//  if(!headset_state())  //If not exist HP
//  {  	
//  	udelay(100);
//    SpeakerMute(0); //enable speaker amplifier	
//  }
	return 0;
}

////////////////////////////////////////////////////////////////////

#ifdef CONFIG_STOP_MODE_SUPPORT

static void soc_early_suspend(struct early_suspend *h)
{
	if (g_soc_pdev)
	{
		ssm2603_suspend(g_soc_pdev, PMSG_SUSPEND);
	}
}

static void soc_late_resume(struct early_suspend *h)
{
	if (g_soc_pdev)
	{
		ssm2603_resume(g_soc_pdev);
	}
}

static struct early_suspend soc_early_suspend_desc = {
        .level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
        .suspend = soc_early_suspend,
        .resume = soc_late_resume,
};
#endif

////////////////////////////////////////////////////////////////////
/*
 * initialise the ssm2603 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int ssm2603_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int reg, ret = 0;

	codec->name = "SSM2603";
	codec->owner = THIS_MODULE;
	codec->read = ssm2603_read_reg_cache;
	codec->write = ssm2603_write;
	codec->set_bias_level = ssm2603_set_bias_level;
	codec->dai = &ssm2603_dai;
	codec->num_dai = 1;
	codec->reg_cache_size = sizeof(ssm2603_reg);
	codec->reg_cache = kmemdup(ssm2603_reg, sizeof(ssm2603_reg),
					GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;
	printk("%s\n", __FUNCTION__);
	ssm2603_reset(codec);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		pr_err("ssm2603: failed to create pcms\n");
		goto pcm_err;
	}
	
	/*power on device*/
	ssm2603_write(codec, SSM2603_ACTIVE, 0);
	
	reg = ssm2603_read_reg_cache(codec,SSM2603_PWR);
	ssm2603_write(codec, SSM2603_PWR, reg | PWR_OUT_PDN);			//William add 20101010

	/* set the update bits */
	reg = ssm2603_read_reg_cache(codec, SSM2603_LINVOL);
	ssm2603_write(codec, SSM2603_LINVOL, reg | LINVOL_LRIN_BOTH);
	reg = ssm2603_read_reg_cache(codec, SSM2603_RINVOL);
	ssm2603_write(codec, SSM2603_RINVOL, reg | RINVOL_RLIN_BOTH);
	reg = ssm2603_read_reg_cache(codec, SSM2603_LOUT1V);
	ssm2603_write(codec, SSM2603_LOUT1V, reg | LOUT1V_LRHP_BOTH);
	reg = ssm2603_read_reg_cache(codec, SSM2603_ROUT1V);
	ssm2603_write(codec, SSM2603_ROUT1V, reg | ROUT1V_RLHP_BOTH);
	/*select Line in as default input*/
	ssm2603_write(codec, SSM2603_APANA,
			APANA_ENABLE_MIC_BOOST2 | APANA_SELECT_DAC |
			APANA_ENABLE_MIC_BOOST);

  //////////////////////////////////////////////////////////////////////

		psnd_hp = kzalloc(sizeof(struct SND_Headphone), GFP_KERNEL);
		if (psnd_hp == NULL)
			return -ENOMEM;
	   
	  INIT_DELAYED_WORK(&psnd_hp->delayed_work, ssm2603_pga_work);

  //////////////////////////////////////////////////////////////////////

	//mdelay(500);			//William add 20101010
	mdelay(100);
	printk("======================activate digital core  \n");
	ssm2603_write(codec, SSM2603_ACTIVE, ACTIVE_ACTIVATE_CODEC);
	
	udelay(100);
  //////////////////////////////////////////////////////////////	
	//ADD to minimize the bootup pop noise.
	ssm2603_write(codec, SSM2603_RINVOL, 0x2e | RINVOL_RLIN_BOTH);
	SpeakerMute(SND_SPK_MUTE);
	
	mdelay(20);
	ssm2603_write(codec, SSM2603_PWR, 0);
	
// Test Maybe it is better to close these code.
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0x10);
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0x10);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0x10);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0);			
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0x10);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0);
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0x10);		
//	mdelay(20);
//	ssm2603_write(codec, SSM2603_PWR, 0);	
  //////////////////////////////////////////////////////////////	

	ssm2603_add_controls(codec);
	ssm2603_add_widgets(codec);
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		pr_err("ssm2603: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;
}

static struct snd_soc_device *ssm2603_socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
/*
 * ssm2603 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
*/
static int ssm2603_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = ssm2603_socdev;
	struct snd_soc_codec *codec = socdev->codec;
	int ret;

	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = ssm2603_init(socdev);
	if (ret < 0)
		pr_err("failed to initialise SSM2603\n");

	return ret;
}

static int ssm2603_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	return 0;
}

static const struct i2c_device_id ssm2603_i2c_id[] = 
{
	{ "ssm2603", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ssm2603_i2c_id);
/* corgi i2c codec control layer */
static struct i2c_driver ssm2603_i2c_driver = {
	.driver = {
		.name = "SSM2603 I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe = ssm2603_i2c_probe,
	.remove = ssm2603_i2c_remove,
	.id_table = ssm2603_i2c_id,
};

static int ssm2603_add_i2c_device(struct platform_device *pdev,
				  const struct ssm2603_setup_data *setup)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;

	ret = i2c_add_driver(&ssm2603_i2c_driver);
	if (ret != 0) {
		dev_err(&pdev->dev, "can't add i2c driver\n");
		return ret;
	}
	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = setup->i2c_address;
	strlcpy(info.type, "ssm2603", I2C_NAME_SIZE);
	adapter = i2c_get_adapter(setup->i2c_bus);
	if (!adapter) {
		dev_err(&pdev->dev, "can't get i2c adapter %d\n",
		setup->i2c_bus);
		goto err_driver;
	}
	client = i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (!client) {
		dev_err(&pdev->dev, "can't add i2c device at 0x%x\n",
		(unsigned int)info.addr);
		goto err_driver;
	}
	return 0;
err_driver:
	i2c_del_driver(&ssm2603_i2c_driver);
	return -ENODEV;
}
#endif

static int ssm2603_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct ssm2603_setup_data *setup;
	struct snd_soc_codec *codec;
	struct ssm2603_priv *ssm2603;
	int ret = 0;

	pr_info("ssm2603 Audio Codec %s", SSM2603_VERSION);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	ssm2603 = kzalloc(sizeof(struct ssm2603_priv), GFP_KERNEL);
	if (ssm2603 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

  g_codec	= codec;

	codec->private_data = ssm2603; 
	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ssm2603_socdev = socdev;
/////////////////////////////////////////////////////////

#ifdef CONFIG_STOP_MODE_SUPPORT
		g_soc_pdev = pdev;
		register_early_suspend(&soc_early_suspend_desc);
#endif

/////////////////////////////////////////////////////////
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		codec->hw_write = (hw_write_t)i2c_master_send;
		ret = ssm2603_add_i2c_device(pdev, setup);
	}
#else
	/* other interfaces */
#endif
	return ret;
}

/* remove everything here */
static int ssm2603_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec->control_data)
		ssm2603_set_bias_level(codec, SND_SOC_BIAS_OFF);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_unregister_device(codec->control_data);
	i2c_del_driver(&ssm2603_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_ssm2603 = {
	.probe = 		ssm2603_probe,
	.remove = 	ssm2603_remove,
	.suspend = 	ssm2603_suspend,
	.resume =		ssm2603_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ssm2603);

static int __init ssm2603_modinit(void)
{
	//ssm2603_mute_headphone(1);
	return snd_soc_register_dai(&ssm2603_dai);
}
module_init(ssm2603_modinit);

static void __exit ssm2603_exit(void)
{
	snd_soc_unregister_dai(&ssm2603_dai);
}
module_exit(ssm2603_exit);

MODULE_DESCRIPTION("ASoC ssm2603 driver");
MODULE_AUTHOR("Cliff Cai");
MODULE_LICENSE("GPL");
