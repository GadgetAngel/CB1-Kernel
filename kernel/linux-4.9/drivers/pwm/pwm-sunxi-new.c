/*
 * drivers/pwm/pwm-sunxi.c
 *
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2015 AllWinner
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/io.h>
#include <linux/clk.h>
#include "pwm-sunxi-new.h"

#define PWM_DEBUG 0
#define PWM_NUM_MAX 4
#define PWM_BIND_NUM 2
#define PWM_PIN_STATE_ACTIVE "active"
#define PWM_PIN_STATE_SLEEP "sleep"

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	    (((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	    (((reg) & CLRMASK(width, shift)) | (val << (shift)))

#if PWM_DEBUG
#define pwm_debug(msg...) pr_info
#else
#define pwm_debug(msg...)
#endif

#if ((defined CONFIG_ARCH_SUN8IW12P1) ||\
			(defined CONFIG_ARCH_SUN8IW17P1) ||\
			(defined CONFIG_ARCH_SUN50IW6P1) ||\
			(defined CONFIG_ARCH_SUN50IW3P1) ||\
			(defined CONFIG_ARCH_SUN50IW8P1) || \
			(defined CONFIG_ARCH_SUN50IW9P1) || \
			(defined CONFIG_ARCH_SUN50IW10P1)) || \
			(defined CONFIG_ARCH_SUN8IW18P1)
#define CLK_GATE_SUPPORT
#endif

struct sunxi_pwm_config {
	unsigned int dead_time;
	unsigned int bind_pwm;

	unsigned int clk_bypass_output;
};

struct sunxi_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct sunxi_pwm_config *config;
#if defined(CLK_GATE_SUPPORT)
	struct clk	*pwm_clk;
#endif
};

static inline struct sunxi_pwm_chip *to_sunxi_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct sunxi_pwm_chip, chip);
}

static inline u32 sunxi_pwm_readl(struct pwm_chip *chip, u32 offset)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	u32 value = 0;

	value = readl(pc->base + offset);

	return value;
}

static inline u32 sunxi_pwm_writel(struct pwm_chip *chip, u32 offset, u32 value)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	writel(value, pc->base + offset);

	return 0;
}

static int sunxi_pwm_pin_set_state(struct device *dev, char *name)
{
	struct pinctrl *pctl;
	struct pinctrl_state *state;
	int ret = -1;

	pctl = pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		dev_err(dev, "pinctrl_get failed!\n");
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		dev_err(dev, "pinctrl_lookup_state(%s) failed!\n", name);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		dev_err(dev, "pinctrl_select_state(%s) failed!\n", name);
		goto exit;
	}
	ret = 0;

exit:
	return ret;
}

static int sunxi_pwm_get_config(struct platform_device *pdev, struct sunxi_pwm_config *config)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	/* read register config */

	ret = of_property_read_u32(np, "bind_pwm", &config->bind_pwm);
	if (ret < 0) {
		/*if there is no bind pwm,set 255, dual pwm invalid!*/
		config->bind_pwm = 255;
		ret = 0;
	}

	ret = of_property_read_u32(np, "dead_time", &config->dead_time);
	if (ret < 0) {
		/*if there is  bind pwm, but not set dead time,set bind pwm 255,dual pwm invalid!*/
		config->bind_pwm = 255;
		ret = 0;
	}

	ret = of_property_read_u32(np, "clk_bypass_output", &config->clk_bypass_output);
	if (ret < 0) {
		/*if use pwm as the internal clock source!*/
		config->clk_bypass_output = 0;
		ret = 0;
	}

	of_node_put(np);

	return ret;
}

static int sunxi_pwm_set_polarity_single(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
	u32 temp;
	unsigned int reg_offset, reg_shift, reg_width;
	u32 sel;

	sel = pwm->pwm - chip->base;
	reg_offset = PWM_PCR_BASE + sel * 0x20;
	reg_shift = PWM_ACT_STA_SHIFT;
	reg_width = PWM_ACT_STA_WIDTH;

	temp = sunxi_pwm_readl(chip, reg_offset);
	if (polarity == PWM_POLARITY_NORMAL)
		temp = SET_BITS(reg_shift, 1, temp, 0);
	else
		temp = SET_BITS(reg_shift, 1, temp, 1);

	sunxi_pwm_writel(chip, reg_offset, temp);

	return 0;
}

static int sunxi_pwm_set_polarity_dual(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity, int bind_num)
{
	u32 temp[2];
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	u32 sel[2] = {0};

	sel[0] = pwm->pwm - chip->base;
	sel[1] = bind_num - chip->base;
	/* config current pwm*/
	reg_offset[0] = PWM_PCR_BASE + sel[0] * 0x20;
	reg_shift[0] = PWM_ACT_STA_SHIFT;
	reg_width[0] = PWM_ACT_STA_WIDTH;
	temp[0] = sunxi_pwm_readl(chip, reg_offset[0]);
	if (polarity == PWM_POLARITY_NORMAL)
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 0);
	else
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 1);

	/* config bind pwm*/
	reg_offset[1] = PWM_PCR_BASE + sel[1] * 0x20;
	reg_shift[1] = PWM_ACT_STA_SHIFT;
	reg_width[1] = PWM_ACT_STA_WIDTH;
	temp[1] = sunxi_pwm_readl(chip, reg_offset[1]);

	/*bind pwm's polarity is reverse compare with the  current pwm*/
	if (polarity == PWM_POLARITY_NORMAL)
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 1);
	else
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 0);

	/*config register at the same time*/
	sunxi_pwm_writel(chip, reg_offset[0], temp[0]);
	sunxi_pwm_writel(chip, reg_offset[1], temp[1]);

	return 0;

}

static u32 get_pccr_reg_offset(u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = PWM_PCCR01;
		break;
	case 2:
	case 3:
		*reg_offset = PWM_PCCR23;
		break;
	case 4:
	case 5:
		*reg_offset = PWM_PCCR45;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PCCR67;
		break;
	case 8:
		*reg_offset = PWM_PCCR8;
		break;
	default:
		pr_err("%s:Not supported!\n", __func__);
		break;
	}
	return 0;
}

static u32 get_pdzcr_reg_offset(u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = PWM_PDZCR01;
		break;
	case 2:
	case 3:
		*reg_offset = PWM_PDZCR23;
		break;
	case 4:
	case 5:
		*reg_offset = PWM_PDZCR45;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PDZCR67;
		break;
	default:
		pr_err("%s:Not supported!\n", __func__);
		break;
	}
	return 0;
}

static int sunxi_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
	int bind_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_set_polarity_single(chip, pwm, polarity);
	else
		sunxi_pwm_set_polarity_dual(chip, pwm, polarity, bind_num);

	return 0;
}


#define PRESCALE_MAX 256

static int sunxi_pwm_config_single(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	unsigned int temp;
	unsigned long long c = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int reg_bypass_shift;
	unsigned int reg_clk_src_shift, reg_clk_src_width;
	unsigned int reg_div_m_shift, reg_div_m_width;
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	u32 sel = 0;
	u32 pre_scal[][2] = {

		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	sel = pwm->pwm - chip->base;

	get_pccr_reg_offset(sel, &reg_offset);
	if ((sel % 2) == 0)
		reg_bypass_shift = 0x5;
	else
		reg_bypass_shift = 0x6;
	/*src clk reg*/
	reg_clk_src_shift = PWM_CLK_SRC_SHIFT;
	reg_clk_src_width = PWM_CLK_SRC_WIDTH;

	if (period_ns > 0 && period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass. */
		c = 100000000;
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_bypass_shift, 1, temp, 1);
		sunxi_pwm_writel(chip, reg_offset, temp);

		/*clk_src_reg*/
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
		sunxi_pwm_writel(chip, reg_offset, temp);

		return 0;
	} else if (period_ns > 10 && period_ns <= 334) {
		/* if freq between 3M~100M, then select 100M as clock */
		c = 100000000;
		/*set clk bypass_output reg to 1 when pwm is used as the internal clock source.*/
		if (pc->config[pwm->pwm - chip->base].clk_bypass_output == 1) {
			temp = sunxi_pwm_readl(chip, reg_offset);
			temp = SET_BITS(reg_bypass_shift, 1, temp, 1);
			sunxi_pwm_writel(chip, reg_offset, temp);
		}

		/*clk_src_reg*/
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
		sunxi_pwm_writel(chip, reg_offset, temp);
	} else if (period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		c = 24000000;
		/*set clk bypass_output reg to 1 when pwm is used as the internal clock source.*/
		if (pc->config[pwm->pwm - chip->base].clk_bypass_output == 1) {
			temp = sunxi_pwm_readl(chip, reg_offset);
			temp = SET_BITS(reg_bypass_shift, 1, temp, 1);
			sunxi_pwm_writel(chip, reg_offset, temp);
		}

		/*clk_src_reg*/
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
		sunxi_pwm_writel(chip, reg_offset, temp);
	}
	pwm_debug("duty_ns=%d period_ns=%d c =%llu.\n", duty_ns, period_ns, c);

	c = c * period_ns;
	do_div(c, 1000000000);
	entire_cycles = (unsigned long)c;

	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= 65536)
				break;
		for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
			entire_cycles = (entire_cycles/pre_scal[pre_scal_id][1])/(prescale + 1);
			if (entire_cycles <= 65536) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
	}

	c = (unsigned long long)entire_cycles * duty_ns;
	do_div(c, period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config  clk div_m*/
	reg_div_m_shift = PWM_DIV_M_SHIFT;
	reg_div_m_width = PWM_DIV_M_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, div_m);
	sunxi_pwm_writel(chip, reg_offset, temp);

	/* config prescal */
	reg_offset = PWM_PCR_BASE + 0x20 * sel;
	reg_shift = PWM_PRESCAL_SHIFT;
	reg_width = PWM_PRESCAL_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, prescale);
	sunxi_pwm_writel(chip, reg_offset, temp);

	/* config active cycles */
	reg_offset = PWM_PPR_BASE + 0x20 * sel;
	reg_shift = PWM_ACT_CYCLES_SHIFT;
	reg_width = PWM_ACT_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
	sunxi_pwm_writel(chip, reg_offset, temp);

	/* config period cycles */
	reg_offset = PWM_PPR_BASE + 0x20 * sel;
	reg_shift = PWM_PERIOD_CYCLES_SHIFT;
	reg_width = PWM_PERIOD_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, (entire_cycles - 1));

	sunxi_pwm_writel(chip, reg_offset, temp);

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);
	return 0;
}

static int sunxi_pwm_config_dual(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns, int bind_num)
{
	u32 value[2] = {0};
	unsigned int temp;
	unsigned long long c = 0, clk = 0, clk_temp = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	unsigned int reg_bypass_shift;
	unsigned int reg_dz_en_offset[2], reg_dz_en_shift[2], reg_dz_en_width[2];
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	int src_clk_sel = 0;
	int i = 0;
	unsigned int dead_time = 0, duty = 0;
	u32 pre_scal[][2] = {

		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/* if duty time < dead time,it is wrong. */
	dead_time = pc->config[pwm_index[0]].dead_time;
	duty = (unsigned int)duty_ns;
	/* judge if the pwm eanble dead zone */
	get_pdzcr_reg_offset(pwm_index[0], &reg_dz_en_offset[0]);
	reg_dz_en_shift[0] = PWM_DZ_EN_SHIFT;
	reg_dz_en_width[0] = PWM_DZ_EN_WIDTH;

	value[0] = sunxi_pwm_readl(chip, reg_dz_en_offset[0]);
	value[0] = SET_BITS(reg_dz_en_shift[0], reg_dz_en_width[0], value[0], 1);
	sunxi_pwm_writel(chip, reg_dz_en_offset[0], value[0]);
	temp = sunxi_pwm_readl(chip, reg_dz_en_offset[0]);
	temp &=  (1u << reg_dz_en_shift[0]);
	if (duty < dead_time || temp == 0) {
		pr_err("[PWM]duty time or dead zone error.\n");
		return -EINVAL;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		if ((i % 2) == 0)
			reg_bypass_shift = 0x5;
		else
			reg_bypass_shift = 0x6;
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = reg_bypass_shift;
		reg_width[i] = PWM_BYPASS_WIDTH;
	}

	if (period_ns > 0 && period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass */
		clk = 100000000;
		src_clk_sel = 1;

		/* config the two pwm bypass */
		for (i = 0; i < PWM_BIND_NUM; i++) {
			temp = sunxi_pwm_readl(chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(chip, reg_offset[i], temp);

			reg_shift[i] = PWM_CLK_SRC_SHIFT;
			reg_width[i] = PWM_CLK_SRC_WIDTH;

			temp = sunxi_pwm_readl(chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(chip, reg_offset[i], temp);
		}

		return 0;
	} else if (period_ns > 10 && period_ns <= 334) {
		clk = 100000000;
		src_clk_sel = 1;
	} else if (period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		clk = 24000000;
		src_clk_sel = 0;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_CLK_SRC_SHIFT;
		reg_width[i] = PWM_CLK_SRC_WIDTH;

		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, src_clk_sel);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	c = clk;
	c *= period_ns;
	do_div(c, 1000000000);
	entire_cycles = (unsigned long)c;

	/* get div_m and prescale,which satisfy: deat_val <= 256, entire <= 65536 */
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
			entire_cycles = (entire_cycles/pre_scal[pre_scal_id][1])/(prescale + 1);
			clk_temp = clk;
			do_div(clk_temp, pre_scal[pre_scal_id][1] * (prescale + 1));
			clk_temp *= dead_time;
			do_div(clk_temp, 1000000000);
			if (entire_cycles <= 65536 && clk_temp <= 256) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
		if (entire_cycles <= 65536 && clk_temp <= 256)
				break;
		else {
			pr_err("%s:config dual err.entire_cycles=%lu, dead_zone_val=%llu",
					__func__, entire_cycles, clk_temp);
			return -EINVAL;
		}
	}

	c = (unsigned long long)entire_cycles * duty_ns;
	do_div(c,  period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config  clk div_m*/
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_DIV_M_SHIFT;
		reg_width[i] = PWM_DIV_M_SHIFT;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, div_m);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config prescal */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = PWM_PCR_BASE + 0x20 * pwm_index[i];
		reg_shift[i] = PWM_PRESCAL_SHIFT;
		reg_width[i] = PWM_PRESCAL_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, prescale);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config active cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = PWM_PPR_BASE + 0x20 * pwm_index[i];
		reg_shift[i] = PWM_ACT_CYCLES_SHIFT;
		reg_width[i] = PWM_ACT_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, active_cycles);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config period cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = PWM_PPR_BASE + 0x20 * pwm_index[i];
		reg_shift[i] = PWM_PERIOD_CYCLES_SHIFT;
		reg_width[i] = PWM_PERIOD_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, (entire_cycles - 1));
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);

	/* config dead zone, one config for two pwm */
	reg_offset[0] = reg_dz_en_offset[0];
	reg_shift[0] = PWM_PDZINTV_SHIFT;
	reg_width[0] = PWM_PDZINTV_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset[0]);
	temp = SET_BITS(reg_shift[0], reg_width[0], temp, (unsigned int)clk_temp);
	sunxi_pwm_writel(chip, reg_offset[0], temp);

	return 0;
}

static int sunxi_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	int bind_num;

	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_config_single(chip, pwm, duty_ns, period_ns);
	else
		sunxi_pwm_config_dual(chip, pwm, duty_ns, period_ns, bind_num);

	return 0;
}

static int sunxi_pwm_enable_single(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int value = 0, index = 0;
	unsigned int reg_offset, reg_shift;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;

	index = pwm->pwm - chip->base;
	sub_np = of_parse_phandle(chip->dev->of_node, "pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);

	/* enable clk for pwm controller */
	get_pccr_reg_offset(index, &reg_offset);
	reg_shift = PWM_CLK_GATING_SHIFT;
	value = sunxi_pwm_readl(chip, reg_offset);
	value = SET_BITS(reg_shift, 1, value, 1);
	sunxi_pwm_writel(chip, reg_offset, value);

	/* enable pwm controller */
	reg_offset = PWM_PER;
	reg_shift = index;
	value = sunxi_pwm_readl(chip, reg_offset);
	value = SET_BITS(reg_shift, 1, value, 1);
	sunxi_pwm_writel(chip, reg_offset, value);

	return 0;
}

#if 0
unsigned long long sunxi_get_clk_freq(struct pwm_chip *chip, int pwm)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned long long c = 0;
	unsigned int temp;
	unsigned int index = 0;
	u32 pre_scal[][2] = {

		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	index = pwm - chip->base;
	reg_offset = pc->config[index].reg_clk_src_offset;
	reg_shift = pc->config[index].reg_clk_src_shift;
	reg_width = pc->config[index].reg_clk_src_width;

	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = temp >> reg_shift;
	temp = temp & ((1u << reg_width) - 1);
	if (temp == 0) {
		c = 24000000;
	} else if (temp == 1) {
		c = 100000000;
	}

	/* check if clk is bypass*/
	reg_offset = pc->config[index].reg_bypass_offset;
	reg_shift = pc->config[index].reg_bypass_shift;
	reg_width = pc->config[index].reg_bypass_width;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = temp >> reg_shift;
	temp = temp & ((1u << reg_width) - 1);
	if (temp == 1)
		return c;

	/* check clk div m */
	reg_offset = pc->config[index].reg_clk_div_m_offset;
	reg_shift = pc->config[index].reg_clk_div_m_shift;
	reg_width = pc->config[index].reg_clk_div_m_width;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = temp >> reg_shift;
	temp = temp & ((1u << reg_width) - 1);
	do_div(c, pre_scal[temp][1]);

	/* check clk prescal */
	reg_offset = pc->config[index].reg_prescal_offset;
	reg_shift = pc->config[index].reg_prescal_shift;
	reg_width = pc->config[index].reg_prescal_width;
	temp = sunxi_pwm_readl(chip, reg_offset);
	temp = temp >> reg_shift;
	temp = temp & ((1u << reg_width) - 1);
	do_div(c, temp + 1);

	return c;
}
#endif

static int sunxi_pwm_enable_dual(struct pwm_chip *chip, struct pwm_device *pwm, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0;
	unsigned int pwm_index[2] = {0};

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/*set current pwm pin state*/
	sub_np[0] = of_parse_phandle(chip->dev->of_node, "pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	/*set bind pwm pin state*/
	sub_np[1] = of_parse_phandle(chip->dev->of_node, "pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_ACTIVE);
	sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_ACTIVE);

	/* enable clk for pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = PWM_CLK_GATING_WIDTH;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* enable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = PWM_PER;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	return 0;
}

static int sunxi_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int bind_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_enable_single(chip, pwm);
	else
		sunxi_pwm_enable_dual(chip, pwm, bind_num);

	return 0;
}


static void sunxi_pwm_disable_single(struct pwm_chip *chip, struct pwm_device *pwm)
{
	u32 value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;

	index = pwm->pwm - chip->base;
	sub_np = of_parse_phandle(chip->dev->of_node, "pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return;
	}

	/* disable pwm controller */
	reg_offset = PWM_PER;
	reg_shift = index;
	reg_width = 0x1;
	value = sunxi_pwm_readl(chip, reg_offset);
	value = SET_BITS(reg_shift, reg_width, value, 0);
	sunxi_pwm_writel(chip, reg_offset, value);

	/*
	 * 0 , 1 --> 0
	 * 2 , 3 --> 2
	 * 4 , 5 --> 4
	 * 6 , 7 --> 6
	 */
	reg_shift &= ~(1);
	/* disable pwm controller */
	if (GET_BITS(reg_shift, 2, value) == 0) {
		/* disable clk for pwm controller. */
		get_pccr_reg_offset(index, &reg_offset);
		reg_shift = PWM_CLK_GATING_SHIFT;
		reg_width = 0x1;
		value = sunxi_pwm_readl(chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 0);
		sunxi_pwm_writel(chip, reg_offset, value);
	}

	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);
}

static void sunxi_pwm_disable_dual(struct pwm_chip *chip, struct pwm_device *pwm, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0;
	unsigned int pwm_index[2] = {0};

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/* get current index pwm device */
	sub_np[0] = of_parse_phandle(chip->dev->of_node, "pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return;
	}
	/* get bind pwm device */
	sub_np[1] = of_parse_phandle(chip->dev->of_node, "pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return;
	}

	/* disable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = PWM_PER;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* disable pwm clk gating */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* disable pwm dead zone,one for the two pwm */
	get_pdzcr_reg_offset(pwm_index[0], &reg_offset[0]);
	reg_shift[0] = PWM_DZ_EN_SHIFT;
	reg_width[0] = PWM_DZ_EN_WIDTH;
	value[0] = sunxi_pwm_readl(chip, reg_offset[0]);
	value[0] = SET_BITS(reg_shift[0], reg_width[0], value[0], 0);
	sunxi_pwm_writel(chip, reg_offset[0], value[0]);

	/* config pin sleep */
	sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_SLEEP);
	sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_SLEEP);
}

static void sunxi_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int bind_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_disable_single(chip, pwm);
	else
		sunxi_pwm_disable_dual(chip, pwm, bind_num);
}

static struct pwm_ops sunxi_pwm_ops = {
	.config = sunxi_pwm_config,
	.enable = sunxi_pwm_enable,
	.disable = sunxi_pwm_disable,
	.set_polarity = sunxi_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int sunxi_pwm_probe(struct platform_device *pdev)
{
	int ret;
	struct sunxi_pwm_chip *pwm;
	struct device_node *np = pdev->dev.of_node;
	int i;
	struct platform_device *pwm_pdevice;
	struct device_node *sub_np;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		return -ENOMEM;
	}

	/* io map pwm base */
	pwm->base = (void __iomem *)of_iomap(pdev->dev.of_node, 0);
	if (!pwm->base) {
		dev_err(&pdev->dev, "unable to map pwm registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	/* read property pwm-number */
	ret = of_property_read_u32(np, "pwm-number", &pwm->chip.npwm);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pwm number: %d, force to one!\n", ret);
		/* force to one pwm if read property fail */
		pwm->chip.npwm = 1;
	}

	/* read property pwm-base */
	ret = of_property_read_u32(np, "pwm-base", &pwm->chip.base);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pwm-base: %d, force to -1 !\n", ret);
		/* force to one pwm if read property fail */
		pwm->chip.base = -1;
	}
	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &sunxi_pwm_ops;

	/* add pwm chip to pwm-core */
	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		goto err_add;
	}
	platform_set_drvdata(pdev, pwm);

	pwm->config = devm_kzalloc(&pdev->dev, sizeof(*pwm->config) * pwm->chip.npwm, GFP_KERNEL);
	if (!pwm->config) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		goto err_alloc;
	}

	for (i = 0; i < pwm->chip.npwm; i++) {
		sub_np = of_parse_phandle(np, "pwms", i);
		if (IS_ERR_OR_NULL(sub_np)) {
			pr_err("%s: can't parse \"pwms\" property\n", __func__);
			return -EINVAL;
		}

		pwm_pdevice = of_find_device_by_node(sub_np);
		ret =	sunxi_pwm_get_config(pwm_pdevice, &pwm->config[i]);
		if (ret) {
			pr_err("Get pwm config failed,exit!\n");
			goto err_get_config;
		}
	}
#if defined(CLK_GATE_SUPPORT)
	pwm->pwm_clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(pwm->pwm_clk)) {
		pr_err("%s: can't get pwm clk\n", __func__);
		return -EINVAL;
	}
	clk_prepare_enable(pwm->pwm_clk);
#endif
  return 0;

err_get_config:
err_alloc:
	pwmchip_remove(&pwm->chip);
err_add:
	iounmap(pwm->base);
err_iomap:
	return ret;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);
#if defined CLK_GATE_SUPPORT
	clk_disable(pwm->pwm_clk);
#endif
	return pwmchip_remove(&pwm->chip);
}

#ifdef CONFIG_PM
static int sunxi_pwm_suspend(struct device *dev)
{
	int i = 0;
	bool pwm_state;
	struct platform_device *pdev = container_of(dev,
			struct platform_device, dev);
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);

	for (i = 0; i < pwm->chip.npwm; i++) {
		pwm_state = pwm->chip.pwms[i].state.enabled;
		pwm_disable(&pwm->chip.pwms[i]);
		pwm->chip.pwms[i].state.enabled = pwm_state;
	}

#if defined CLK_GATE_SUPPORT
	if (IS_ERR_OR_NULL(pwm->pwm_clk)) {
		pr_err("%s: not get pwm clk\n", __func__);
		return -EINVAL;
	}
	clk_disable_unprepare(pwm->pwm_clk);
#endif

	return 0;
}

static int sunxi_pwm_resume(struct device *dev)
{
	int i = 0;
	int period, duty_cycle, polarity;
	struct platform_device *pdev = container_of(dev,
			struct platform_device, dev);
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);

#if defined(CLK_GATE_SUPPORT)
	if (IS_ERR_OR_NULL(pwm->pwm_clk)) {
		pr_err("%s: not get pwm clk\n", __func__);
		return -EINVAL;
	}
	clk_prepare_enable(pwm->pwm_clk);
#endif

	for (i = 0; i < pwm->chip.npwm; i++) {
		period = pwm->chip.pwms[i].state.period;
		pwm->chip.pwms[i].state.period = 0;
		duty_cycle = pwm->chip.pwms[i].state.duty_cycle;
		pwm->chip.pwms[i].state.duty_cycle = 0;
		pwm_config(&pwm->chip.pwms[i], duty_cycle, period);

		polarity = pwm->chip.pwms[i].state.polarity;
		pwm->chip.pwms[i].state.polarity = PWM_POLARITY_NORMAL;
		pwm_set_polarity(&pwm->chip.pwms[i], polarity);

		if (pwm->chip.pwms[i].state.enabled == true) {
			pwm->chip.pwms[i].state.enabled = false;
			pwm_enable(&pwm->chip.pwms[i]);
		}
	}
	return 0;
}

static const struct dev_pm_ops pwm_pm_ops = {
	.suspend_late = sunxi_pwm_suspend,
	.resume_early = sunxi_pwm_resume,
};
#else
static const struct dev_pm_ops pwm_pm_ops;
#endif

#if !defined(CONFIG_OF)
struct platform_device sunxi_pwm_device = {
	.name = "sunxi_pwm",
	.id = -1,
};
#else
static const struct of_device_id sunxi_pwm_match[] = {
	{ .compatible = "allwinner,sunxi-pwm", },
	{ .compatible = "allwinner,sunxi-s_pwm", },
	{},
};
#endif

static struct platform_driver sunxi_pwm_driver = {
	.probe = sunxi_pwm_probe,
	.remove = sunxi_pwm_remove,
	.driver = {
		.name = "sunxi_pwm",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_pwm_match,
		.pm = &pwm_pm_ops,
	 },
};

static int __init pwm_module_init(void)
{
	int ret = 0;

	pr_info("pwm module init!\n");

#if !defined(CONFIG_OF)
	ret = platform_device_register(&sunxi_pwm_device);
#endif
	if (ret == 0) {
		ret = platform_driver_register(&sunxi_pwm_driver);
	}

	return ret;
}

static void __exit pwm_module_exit(void)
{
	pr_info("pwm module exit!\n");

	platform_driver_unregister(&sunxi_pwm_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&sunxi_pwm_device);
#endif
}

subsys_initcall(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("zengqi");
MODULE_AUTHOR("liuli");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");

