/*
 * linux/sound/soc.h -- ALSA SoC Layer
 *
 * Author:		Liam Girdwood
 * Created:		Aug 11th 2005
 * Copyright:	Wolfson Microelectronics. PLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_SND_SOC_H
#define __LINUX_SND_SOC_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/ac97_codec.h>

#define SND_SOC_VERSION "0.12"

/*
 * Convenience kcontrol builders
 */
#define SOC_SINGLE_VALUE(reg,shift,mask,invert) ((reg) | ((shift) << 8) |\
	((shift) << 12) | ((mask) << 16) | ((invert) << 24))
#define SOC_SINGLE_VALUE_EXT(reg,mask,invert) ((reg) | ((mask) << 16) |\
	((invert) << 31))
#define SOC_SINGLE(xname, reg, shift, mask, invert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw,\
	.put = snd_soc_put_volsw, \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, mask, invert) }
#define SOC_DOUBLE(xname, reg, shift_left, shift_right, mask, invert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw, \
	.put = snd_soc_put_volsw, \
	.private_value = (reg) | ((shift_left) << 8) | \
		((shift_right) << 12) | ((mask) << 16) | ((invert) << 24) }
#define SOC_DOUBLE_R(xname, reg_left, reg_right, shift, mask, invert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.info = snd_soc_info_volsw_2r, \
	.get = snd_soc_get_volsw_2r, .put = snd_soc_put_volsw_2r, \
	.private_value = (reg_left) | ((shift) << 8)  | \
		((mask) << 12) | ((invert) << 20) | ((reg_right) << 24) }
#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, xtexts) \
{	.reg = xreg, .shift_l = xshift_l, .shift_r = xshift_r, \
	.mask = xmask, .texts = xtexts }
#define SOC_ENUM_SINGLE(xreg, xshift, xmask, xtexts) \
	SOC_ENUM_DOUBLE(xreg, xshift, xshift, xmask, xtexts)
#define SOC_ENUM_SINGLE_EXT(xmask, xtexts) \
{	.mask = xmask, .texts = xtexts }
#define SOC_ENUM(xname, xenum) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
	.info = snd_soc_info_enum_double, \
	.get = snd_soc_get_enum_double, .put = snd_soc_put_enum_double, \
	.private_value = (unsigned long)&xenum }
#define SOC_SINGLE_EXT(xname, xreg, xmask, xinvert,\
	 xhandler_get, xhandler_put) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_volsw_ext, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value =  SOC_SINGLE_VALUE_EXT(xreg, xmask, xinvert) }
#define SOC_SINGLE_BOOL_EXT(xname, xdata, xhandler_get, xhandler_put) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_bool_ext, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = xdata }
#define SOC_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_enum_ext, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = (unsigned long)&xenum }

/*
 * Digital Audio Interface (DAI) types
 */
#define SND_SOC_DAI_AC97	0x1
#define SND_SOC_DAI_I2S		0x2
#define SND_SOC_DAI_PCM		0x4

/*
 * DAI hardware audio formats
 */
#define SND_SOC_DAIFMT_I2S		(1 << 0)	/* I2S mode */
#define SND_SOC_DAIFMT_RIGHT_J	(1 << 1)	/* Right justified mode */
#define SND_SOC_DAIFMT_LEFT_J	(1 << 2)	/* Left Justified mode */
#define SND_SOC_DAIFMT_DSP_A	(1 << 3)	/* L data msb after FRM or LRC */
#define SND_SOC_DAIFMT_DSP_B	(1 << 4)	/* L data msb during FRM or LRC */
#define SND_SOC_DAIFMT_AC97		(1 << 5)	/* AC97 */

/*
 * DAI hardware signal inversions
 */
#define SND_SOC_DAIFMT_NB_NF		(1 << 8)	/* normal bit clock + frame */
#define SND_SOC_DAIFMT_NB_IF		(1 << 9)	/* normal bclk + inv frm */
#define SND_SOC_DAIFMT_IB_NF		(1 << 10)	/* invert bclk + nor frm */
#define SND_SOC_DAIFMT_IB_IF		(1 << 11)	/* invert bclk + frm */

/*
 * DAI hardware clock masters
 * This is wrt the codec, the inverse is true for the interface
 * i.e. if the codec is clk and frm master then the interface is
 * clk and frame slave.
 */
#define SND_SOC_DAIFMT_CBM_CFM	(1 << 12) /* codec clk & frm master */
#define SND_SOC_DAIFMT_CBS_CFM	(1 << 13) /* codec clk slave & frm master */
#define SND_SOC_DAIFMT_CBM_CFS	(1 << 14) /* codec clk master & frame slave */
#define SND_SOC_DAIFMT_CBS_CFS	(1 << 15) /* codec clk & frm slave */

#define SND_SOC_DAIFMT_FORMAT_MASK		0x00ff
#define SND_SOC_DAIFMT_INV_MASK			0x0f00
#define SND_SOC_DAIFMT_CLOCK_MASK		0xf000

/*
 * DAI hardware audio direction
 */
#define SND_SOC_DAIDIR_PLAYBACK		0x1
#define SND_SOC_DAIDIR_CAPTURE		0x2

/*
 * DAI hardware Time Division Multiplexing (TDM) Slots
 * Left and Right data word positions
 * This is measured in words (sample size) and not bits.
 */
#define SND_SOC_DAITDM_LRDW(l,r)	((l << 8) | r)

/*
 * DAI hardware clock ratios
 * bit clock can either be a generated by dividing mclk or
 * by multiplying sample rate, hence there are 2 definitions below
 * depending on codec type.
 */
/* ratio of sample rate to mclk/sysclk */
#define SND_SOC_FS_ALL			0xffff	/* all mclk supported */

/* bit clock dividers */
#define SND_SOC_FSBD(x)			(1 << (x - 1))	/* ratio mclk:bclk */
#define SND_SOC_FSBD_REAL(x)	(ffs(x))

/* bit clock ratio to (sample rate * channels * word size) */
#define SND_SOC_FSBW(x)			(1 << (x - 1))
#define SND_SOC_FSBW_REAL(x)		(ffs(x))
/* all bclk ratios supported */
#define SND_SOC_FSB_ALL			~0ULL

/*
 * DAI hardware flags
 */
/* use bfs mclk divider mode (BCLK = MCLK / x) */
#define SND_SOC_DAI_BFS_DIV		0x1
/* use bfs rate mulitplier  (BCLK = RATE * x)*/
#define SND_SOC_DAI_BFS_RATE	0x2
/* use bfs rcw multiplier (BCLK = RATE * CHN * WORD SIZE) */
#define SND_SOC_DAI_BFS_RCW		0x4
/* capture and playback can use different clocks */
#define SND_SOC_DAI_ASYNC		0x8

/*
 * AC97 codec ID's bitmask
 */
#define SND_SOC_DAI_AC97_ID0	(1 << 0)
#define SND_SOC_DAI_AC97_ID1	(1 << 1)
#define SND_SOC_DAI_AC97_ID2	(1 << 2)
#define SND_SOC_DAI_AC97_ID3	(1 << 3)

struct snd_soc_device;
struct snd_soc_pcm_stream;
struct snd_soc_ops;
struct snd_soc_dai_mode;
struct snd_soc_pcm_runtime;
struct snd_soc_codec_dai;
struct snd_soc_cpu_dai;
struct snd_soc_codec;
struct snd_soc_machine_config;
struct soc_enum;
struct snd_soc_ac97_ops;
struct snd_soc_clock_info;

typedef int (*hw_write_t)(void *,const char* ,int);
typedef int (*hw_read_t)(void *,char* ,int);

extern struct snd_ac97_bus_ops soc_ac97_ops;

/* pcm <-> DAI connect */
void snd_soc_free_pcms(struct snd_soc_device *socdev);
int snd_soc_new_pcms(struct snd_soc_device *socdev, int idx, const char *xid);
int snd_soc_register_card(struct snd_soc_device *socdev);

/* set runtime hw params */
int snd_soc_set_runtime_hwparams(struct snd_pcm_substream *substream,
	const struct snd_pcm_hardware *hw);
int snd_soc_get_rate(int rate);

/* codec IO */
#define snd_soc_read(codec, reg) codec->read(codec, reg)
#define snd_soc_write(codec, reg, value) codec->write(codec, reg, value)

/* codec register bit access */
int snd_soc_update_bits(struct snd_soc_codec *codec, unsigned short reg,
				unsigned short mask, unsigned short value);
int snd_soc_test_bits(struct snd_soc_codec *codec, unsigned short reg,
				unsigned short mask, unsigned short value);

int snd_soc_new_ac97_codec(struct snd_soc_codec *codec,
	struct snd_ac97_bus_ops *ops, int num);
void snd_soc_free_ac97_codec(struct snd_soc_codec *codec);

/*
 *Controls
 */
struct snd_kcontrol *snd_soc_cnew(const struct snd_kcontrol_new *_template,
	void *data, char *long_name);
int snd_soc_info_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_info_enum_ext(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_get_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);
int snd_soc_put_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);
int snd_soc_info_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_info_volsw_ext(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_info_bool_ext(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);
int snd_soc_put_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);
int snd_soc_info_volsw_2r(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo);
int snd_soc_get_volsw_2r(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);
int snd_soc_put_volsw_2r(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol);

/* SoC PCM stream information */
struct snd_soc_pcm_stream {
	char *stream_name;
	unsigned int rate_min;		/* min rate */
	unsigned int rate_max;		/* max rate */
	unsigned int channels_min;	/* min channels */
	unsigned int channels_max;	/* max channels */
	unsigned int active:1;		/* stream is in use */
};

/* SoC audio ops */
struct snd_soc_ops {
	int (*startup)(struct snd_pcm_substream *);
	void (*shutdown)(struct snd_pcm_substream *);
	int (*hw_params)(struct snd_pcm_substream *, struct snd_pcm_hw_params *);
	int (*hw_free)(struct snd_pcm_substream *);
	int (*prepare)(struct snd_pcm_substream *);
	int (*trigger)(struct snd_pcm_substream *, int);
};

/* SoC DAI hardware mode */
struct snd_soc_dai_mode {
	u16 fmt;		/* SND_SOC_DAIFMT_* */
	u16 tdm;		/* SND_SOC_HWTDM_* */
	u64 pcmfmt; 	/* SNDRV_PCM_FMTBIT_* */
	u16 pcmrate;	/* SND_SOC_HWRATE_* */
	u16 pcmdir:2;	/* SND_SOC_HWDIR_* */
	u16 flags:8;	/* hw flags */
	u16 fs;			/* mclk to rate divider */
	u64 bfs;		/* mclk to bclk dividers */
	unsigned long priv;		/* private mode data */
};

/* DAI capabilities */
struct snd_soc_dai_cap {
	int num_modes;				/* number of DAI modes */
	struct snd_soc_dai_mode *mode;	/* array of supported DAI modes */
};

/* SoC Codec DAI */
struct snd_soc_codec_dai {
	char *name;
	int id;

	/* DAI capabilities */
	struct snd_soc_pcm_stream playback;
	struct snd_soc_pcm_stream capture;
	struct snd_soc_dai_cap caps;

	/* DAI runtime info */
	struct snd_soc_dai_mode dai_runtime;
	struct snd_soc_ops ops;
	unsigned int (*config_sysclk)(struct snd_soc_codec_dai*,
		struct snd_soc_clock_info *info, unsigned int clk);
	int (*digital_mute)(struct snd_soc_codec *,
		struct snd_soc_codec_dai*, int);
	unsigned int mclk;		/* the audio master clock */
	unsigned int pll_in;	/* the PLL input clock */
	unsigned int pll_out;	/* the PLL output clock */
	unsigned int clk_div;	/* internal clock divider << 1 (for fractions) */
	unsigned int active;
	unsigned char pop_wait:1;

	/* DAI private data */
	void *private_data;
};

/* SoC CPU DAI */
struct snd_soc_cpu_dai {

	/* DAI description */
	char *name;
	unsigned int id;
	unsigned char type;

	/* DAI callbacks */
	int (*probe)(struct platform_device *pdev);
	void (*remove)(struct platform_device *pdev);
	int (*suspend)(struct platform_device *pdev,
		struct snd_soc_cpu_dai *cpu_dai);
	int (*resume)(struct platform_device *pdev,
		struct snd_soc_cpu_dai *cpu_dai);
	unsigned int (*config_sysclk)(struct snd_soc_cpu_dai *cpu_dai,
		struct snd_soc_clock_info *info, unsigned int clk);

	/* DAI capabilities */
	struct snd_soc_pcm_stream capture;
	struct snd_soc_pcm_stream playback;
	struct snd_soc_dai_cap caps;

	/* DAI runtime info */
	struct snd_soc_dai_mode dai_runtime;
	struct snd_soc_ops ops;
	struct snd_pcm_runtime *runtime;
	unsigned char active:1;
	unsigned int mclk;
	void *dma_data;

	/* DAI private data */
	void *private_data;
};

/* SoC Audio Codec */
struct snd_soc_codec {
	char *name;
	struct module *owner;
	struct mutex mutex;

	/* callbacks */
	int (*dapm_event)(struct snd_soc_codec *codec, int event);

	/* runtime */
	struct snd_card *card;
	struct snd_ac97 *ac97;  /* for ad-hoc ac97 devices */
	unsigned int active;
	unsigned int pcm_devs;
	void *private_data;

	/* codec IO */
	void *control_data; /* codec control (i2c/3wire) data */
	unsigned int (*read)(struct snd_soc_codec *, unsigned int);
	int (*write)(struct snd_soc_codec *, unsigned int, unsigned int);
	hw_write_t hw_write;
	hw_read_t hw_read;
	void *reg_cache;
	short reg_cache_size;
	short reg_cache_step;

	/* dapm */
	struct list_head dapm_widgets;
	struct list_head dapm_paths;
	unsigned int dapm_state;
	unsigned int suspend_dapm_state;
	struct delayed_work delayed_work;

	/* codec DAI's */
	struct snd_soc_codec_dai *dai;
	unsigned int num_dai;
};

/* codec device */
struct snd_soc_codec_device {
	int (*probe)(struct platform_device *pdev);
	int (*remove)(struct platform_device *pdev);
	int (*suspend)(struct platform_device *pdev, pm_message_t state);
	int (*resume)(struct platform_device *pdev);
};

/* SoC platform interface */
struct snd_soc_platform {
	char *name;

	int (*probe)(struct platform_device *pdev);
	int (*remove)(struct platform_device *pdev);
	int (*suspend)(struct platform_device *pdev,
		struct snd_soc_cpu_dai *cpu_dai);
	int (*resume)(struct platform_device *pdev,
		struct snd_soc_cpu_dai *cpu_dai);

	/* pcm creation and destruction */
	int (*pcm_new)(struct snd_card *, struct snd_soc_codec_dai *,
		struct snd_pcm *);
	void (*pcm_free)(struct snd_pcm *);

	/* platform stream ops */
	struct snd_pcm_ops *pcm_ops;
};

/* SoC machine DAI configuration, glues a codec and cpu DAI together */
struct snd_soc_dai_link  {
	char *name;			/* Codec name */
	char *stream_name;		/* Stream name */

	/* DAI */
	struct snd_soc_codec_dai *codec_dai;
	struct snd_soc_cpu_dai *cpu_dai;
	u32 flags;			/* DAI config preference flags */

	/* codec/machine specific init - e.g. add machine controls */
	int (*init)(struct snd_soc_codec *codec);

	/* audio sysclock configuration */
	unsigned int (*config_sysclk)(struct snd_soc_pcm_runtime *rtd,
		struct snd_soc_clock_info *info);
};

/* SoC machine */
struct snd_soc_machine {
	char *name;

	int (*probe)(struct platform_device *pdev);
	int (*remove)(struct platform_device *pdev);

	/* the pre and post PM functions are used to do any PM work before and
	 * after the codec and DAI's do any PM work. */
	int (*suspend_pre)(struct platform_device *pdev, pm_message_t state);
	int (*suspend_post)(struct platform_device *pdev, pm_message_t state);
	int (*resume_pre)(struct platform_device *pdev);
	int (*resume_post)(struct platform_device *pdev);

	/* machine stream operations */
	struct snd_soc_ops *ops;

	/* CPU <--> Codec DAI links  */
	struct snd_soc_dai_link *dai_link;
	int num_links;
};

/* SoC Device - the audio subsystem */
struct snd_soc_device {
	struct device *dev;
	struct snd_soc_machine *machine;
	struct snd_soc_platform *platform;
	struct snd_soc_codec *codec;
	struct snd_soc_codec_device *codec_dev;
	struct delayed_work delayed_work;
	void *codec_data;
};

/* runtime channel data */
struct snd_soc_pcm_runtime {
	struct snd_soc_codec_dai *codec_dai;
	struct snd_soc_cpu_dai *cpu_dai;
	struct snd_soc_device *socdev;
};

/* enumerated kcontrol */
struct soc_enum {
	unsigned short reg;
	unsigned short reg2;
	unsigned char shift_l;
	unsigned char shift_r;
	unsigned int mask;
	const char **texts;
	void *dapm;
};

/* clocking configuration data */
struct snd_soc_clock_info {
	unsigned int rate;
	unsigned int fs;
	unsigned int bclk_master;
};

#endif
