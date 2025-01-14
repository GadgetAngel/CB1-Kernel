/*
 * sound\soc\sunxi\sunx50iw9-sndcodec.c
 * (C) Copyright 2014-2018
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * huangxin <huangxin@allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnetech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/input.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/of.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>

#include "sunxi_rw_func.h"

struct sunxi_sndcodec_priv {
	int jack_gpio;
	int jack_invert;	/* 0->high is plug_in, 1->high is plug_out */
	struct snd_soc_jack jack_detect;
};

static const struct snd_kcontrol_new sunxi_card_controls[] = {
	SOC_DAPM_PIN_SWITCH("LINEOUT"),
};

static const struct snd_soc_dapm_widget sunxi_card_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_LINE("LINEIN", NULL),
};

static const struct snd_soc_dapm_route sunxi_card_routes[] = {
	{"LINEINL", NULL, "LINEIN"},
	{"LINEINR", NULL, "LINEIN"},

	{"LINEOUT", NULL, "LINEOUTL"},
	{"LINEOUT", NULL, "LINEOUTR"},
};

/*
 * Card initialization
 */
static int sunxi_card_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;

	snd_soc_dapm_enable_pin(&codec->component.dapm, "LINEIN");
	snd_soc_dapm_disable_pin(&codec->component.dapm, "LINEOUT");

	snd_soc_dapm_sync(&codec->component.dapm);
	return 0;
}

static int sunxi_card_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	unsigned int freq;
	int ret;

	switch (params_rate(params)) {
	case	8000:
	case	12000:
	case	16000:
	case	24000:
	case	32000:
	case	48000:
	case	96000:
	case	192000:
		freq = 98304000;
		break;
	case	11025:
	case	22050:
	case	44100:
		freq = 90316800;
		break;
	default:
		dev_err(card->dev, "invalid rate setting\n");
		return -EINVAL;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq, 0);
	if (ret < 0) {
		dev_err(card->dev, "set codec dai sysclk faided.\n");
		return ret;
	}

	return 0;
}

static struct snd_soc_ops sunxi_card_ops = {
	.hw_params	= sunxi_card_hw_params,
};

static int sunxi_card_suspend(struct snd_soc_card *card)
{
	return 0;
}

static int sunxi_card_resume(struct snd_soc_card *card)
{
	return 0;
}

static struct snd_soc_dai_link sunxi_card_dai_link[] = {
	{
		.name		= "audiocodec",
		.stream_name	= "SUNXI-CODEC",
		.cpu_dai_name	= "sunxi-internal-cpudai",
		.platform_name	= "sunxi-internal-cpudai",
		.codec_dai_name = "sun50iw9-codec",
		.codec_name	= "sunxi-internal-codec",
		.init		= sunxi_card_init,
		.ops		= &sunxi_card_ops,
	},
};

static struct snd_soc_card snd_soc_sunxi_card = {
	.name		= "audiocodec",
	.owner		= THIS_MODULE,
	.dai_link	= sunxi_card_dai_link,
	.num_links	= ARRAY_SIZE(sunxi_card_dai_link),
	.controls	= sunxi_card_controls,
	.num_controls	= ARRAY_SIZE(sunxi_card_controls),
	.dapm_widgets	= sunxi_card_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sunxi_card_dapm_widgets),
	.dapm_routes = sunxi_card_routes,
	.num_dapm_routes = ARRAY_SIZE(sunxi_card_routes),
	.suspend_post	= sunxi_card_suspend,
	.resume_post	= sunxi_card_resume,
};

static int sunxi_card_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &snd_soc_sunxi_card;
	struct sunxi_sndcodec_priv *sndcodec_priv;

	/* register the soc card */
	card->dev = &pdev->dev;

	sndcodec_priv = devm_kzalloc(&pdev->dev,
		sizeof(struct sunxi_sndcodec_priv), GFP_KERNEL);
	if (!sndcodec_priv)
		return -ENOMEM;

	sunxi_card_dai_link[0].cpu_dai_name = NULL;
	sunxi_card_dai_link[0].cpu_of_node = of_parse_phandle(np,
					"sunxi,cpudai-controller", 0);
	if (!sunxi_card_dai_link[0].cpu_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,cpudai-controller' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	} else {
		sunxi_card_dai_link[0].platform_name = NULL;
		sunxi_card_dai_link[0].platform_of_node =
				sunxi_card_dai_link[0].cpu_of_node;
	}
	sunxi_card_dai_link[0].codec_name = NULL;
	sunxi_card_dai_link[0].codec_of_node = of_parse_phandle(np,
						"sunxi,audio-codec", 0);
	if (!sunxi_card_dai_link[0].codec_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,audio-codec' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	snd_soc_card_set_drvdata(card, sndcodec_priv);
	ret = snd_soc_register_card(card);
	if (ret) {
		//dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
		goto err_devm_kfree;
	}

	return 0;

err_devm_kfree:
	devm_kfree(&pdev->dev, sndcodec_priv);
	return ret;
}

static int __exit sunxi_card_dev_remove(struct platform_device *pdev)
{
	struct sunxi_sndcodec_priv *sndcodec_priv = dev_get_drvdata(&pdev->dev);

	devm_kfree(&pdev->dev, sndcodec_priv);
	return 0;
}

static const struct of_device_id sunxi_card_of_match[] = {
	{ .compatible = "allwinner,sunxi-codec-machine", },
	{},
};

static struct platform_driver sunxi_machine_driver = {
	.driver = {
		.name = "sunxi-codec-machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_card_of_match,
	},
	.probe = sunxi_card_dev_probe,
	.remove = __exit_p(sunxi_card_dev_remove),
};

module_platform_driver(sunxi_machine_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Codec Machine ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-codec-machine");
