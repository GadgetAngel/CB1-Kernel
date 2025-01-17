/*
 * drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/de_clock.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "de_clock.h"

static void __iomem *de_base;

static struct de_clk_para de_clk_tbl[] = {
	{
		.clk_no			= DE_CLK_CORE0,
		.div			= 1,
		.ahb_gate_adr		= 0x04,
		.ahb_gate_shift		= 0,
		.ahb_reset_adr		= 0x08,
		.ahb_reset_shift	= 0,
		.dram_gate_adr		= 0x00,
		.dram_gate_shift	= 32,
		.mod_adr		= 0x00,
		.mod_enable_shift	= 0,
		.mod_div_adr		= 0x0c,
		.mod_div_shift		= 0,
		.mod_div_width		= 4,
	},
	{
		.clk_no			= DE_CLK_CORE1,
		.div			= 1,
		.ahb_gate_adr		= 0x04,
		.ahb_gate_shift		= 1,
		.ahb_reset_adr		= 0x08,
#if defined(DE_WB_RESET_SHARE)
		.ahb_reset_shift	= 2,
#else
		.ahb_reset_shift	= 1,
#endif
		.dram_gate_adr		= 0x00,
		.dram_gate_shift	= 32,
		.mod_adr		= 0x00,
		.mod_enable_shift	= 1,
		.mod_div_adr		= 0x0c,
		.mod_div_shift		= 4,
		.mod_div_width		= 4,
	},
	{
		.clk_no			= DE_CLK_WB,
		.div			= 1,
		.ahb_gate_adr		= 0x04,
		.ahb_gate_shift		= 2,
		.ahb_reset_adr		= 0x08,
		.ahb_reset_shift	= 2,
		.dram_gate_adr		= 0x00,
		.dram_gate_shift	= 32,
		.mod_adr		= 0x00,
		.mod_enable_shift	= 2,
		.mod_div_adr		= 0x0c,
		.mod_div_shift		= 8,
		.mod_div_width		= 4,
	},
};

static s32 de_clk_set_div(u32 clk_no, u32 div)
{
	u32 i = 0;
	u32 reg_val;
	u32 len = sizeof(de_clk_tbl) / sizeof(struct de_clk_para);

	for (i = 0; i < len; i++) {
		if ((de_clk_tbl[i].clk_no == clk_no)) {
			reg_val = readl(de_clk_tbl[i].mod_div_adr + de_base);
			reg_val =
			    SET_BITS(de_clk_tbl[i].mod_div_shift,
				     de_clk_tbl[i].mod_div_width, reg_val,
				     (div - 1));
			writel(reg_val, de_clk_tbl[i].mod_div_adr + de_base);

			return 0;
		}
	}

	__wrn("clk %d not foundis not initializd\n", clk_no);

	return -1;
}

static s32 __de_clk_enable(u32 clk_no)
{
	u32 i;
	u32 reg_val;
	u32 len = sizeof(de_clk_tbl) / sizeof(struct de_clk_para);

	for (i = 0; i < len; i++) {
		if ((de_clk_tbl[i].clk_no == clk_no)) {
			/* set clk div */
			de_clk_set_div(clk_no, de_clk_tbl[i].div);

			if (de_clk_tbl[i].ahb_reset_shift < 32) {
				reg_val = readl(de_clk_tbl[i].ahb_reset_adr
				    + de_base);
				reg_val =
				    SET_BITS(de_clk_tbl[i].ahb_reset_shift, 1,
					     reg_val, 1);
				writel(reg_val,
				       de_clk_tbl[i].ahb_reset_adr + de_base);
				__inf("clk %d reset enable\n", clk_no);
			}

			if (de_clk_tbl[i].ahb_gate_shift < 32) {
				reg_val =
				    readl(de_clk_tbl[i].ahb_gate_adr + de_base);
				reg_val =
				    SET_BITS(de_clk_tbl[i].ahb_gate_shift, 1,
				    reg_val, 1);
				writel(reg_val,
				       de_clk_tbl[i].ahb_gate_adr + de_base);
				__inf("clk %d gate enable\n", clk_no);
			}

			if (de_clk_tbl[i].mod_enable_shift < 32) {
				reg_val =
				    readl(de_clk_tbl[i].mod_adr + de_base);
				reg_val =
				    SET_BITS(de_clk_tbl[i].mod_enable_shift, 1,
					     reg_val, 1);
				writel(reg_val,
				       de_clk_tbl[i].mod_adr + de_base);
				__inf("clk %d mod enable\n", clk_no);
			}

			if (de_clk_tbl[i].dram_gate_shift < 32) {
				reg_val =
				    readl(de_clk_tbl[i].dram_gate_adr
				    + de_base);
				reg_val =
				    SET_BITS(de_clk_tbl[i].dram_gate_shift, 1,
					     reg_val, 1);
				writel(reg_val,
				       de_clk_tbl[i].dram_gate_adr + de_base);
				__inf("clk %d dram enable\n", clk_no);
			}
		}
	}

	return 0;
}

static s32 __de_clk_disable(u32 clk_no)
{
	u32 i;
	u32 reg_val;
	u32 len = sizeof(de_clk_tbl) / sizeof(struct de_clk_para);

	for (i = 0; i < len; i++) {
		if ((de_clk_tbl[i].clk_no != clk_no))
			continue;

		if (de_clk_tbl[i].dram_gate_shift < 32) {
			reg_val =
			    readl(de_clk_tbl[i].dram_gate_adr
			    + de_base);
			reg_val =
			    SET_BITS(de_clk_tbl[i].dram_gate_shift, 1,
				     reg_val, 0);
			writel(reg_val,
			       de_clk_tbl[i].dram_gate_adr + de_base);
			__inf("clk %d dram disable\n", clk_no);
		}

		if (de_clk_tbl[i].mod_enable_shift < 32) {
			reg_val =
			    readl(de_clk_tbl[i].mod_adr + de_base);
			reg_val =
			    SET_BITS(de_clk_tbl[i].mod_enable_shift, 1,
				     reg_val, 0);
			writel(reg_val,
			       de_clk_tbl[i].mod_adr + de_base);
			__inf("clk %d mod disable\n", clk_no);
		}

		if (de_clk_tbl[i].ahb_gate_shift < 32) {
			reg_val =
			    readl(de_clk_tbl[i].ahb_gate_adr + de_base);
			reg_val =
			    SET_BITS(de_clk_tbl[i].ahb_gate_shift, 1,
			    reg_val, 0);
			writel(reg_val,
			       de_clk_tbl[i].ahb_gate_adr + de_base);
			__inf("clk %d gate disable\n", clk_no);
		}

		if (de_clk_tbl[i].ahb_reset_shift < 32) {
#if defined(DE_WB_RESET_SHARE)
			if ((clk_no == DE_CLK_CORE1)
				|| (clk_no == DE_CLK_WB))
				continue;
#endif
			reg_val = readl(de_clk_tbl[i].ahb_reset_adr
					+ de_base);
			reg_val = SET_BITS(de_clk_tbl[i].ahb_reset_shift, 1,
						reg_val, 0);
			writel(reg_val, de_clk_tbl[i].ahb_reset_adr + de_base);
			__inf("clk %d reset disable\n", clk_no);
		}
	}

	return 0;
}

s32 de_clk_enable(u32 clk_no)
{
	return __de_clk_enable(clk_no);
}

s32 de_clk_disable(u32 clk_no)
{
	return __de_clk_disable(clk_no);
}

s32 de_clk_set_reg_base(uintptr_t reg_base)
{
	de_base = (void __iomem *)reg_base;

	return 0;
}
