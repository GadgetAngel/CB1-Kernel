/*
 * drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/de_dcsc.c
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
#include "de_rtmx.h"
#include "de_csc_type.h"
#include "de_csc.h"

#define DCSC_OFST	0xB0000
#define CSC_ENHANCE_MODE_NUM 3
/* must equal to ENHANCE_MODE_NUM */

static volatile struct __csc_reg_t *dcsc_dev[DE_NUM];
static volatile struct __csc2_reg_t *dcsc2_dev[DE_NUM];
static struct de_reg_blocks dcsc_coeff_block[DE_NUM];
static struct de_reg_blocks dcsc_enable_block[DE_NUM];
static struct disp_csc_config g_dcsc_config[DE_NUM];

static unsigned int is_in_smbl[DE_NUM];
/* device csc and smbl in the same module or not */

static int de_dcsc_set_reg_base(unsigned int sel, void *base)
{
	__inf("sel=%d, base=0x%p\n", sel, base);
	if (is_in_smbl[sel])
		dcsc2_dev[sel] = (struct __csc2_reg_t *) base;
	else
		dcsc_dev[sel] = (struct __csc_reg_t *) base;

	return 0;
}

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config)
{
	int csc_coeff[12];
	unsigned int enhance_mode;
	int csc_enhance_setting[CSC_ENHANCE_MODE_NUM][4] = {
		{50, 50, 50, 50},
		/* normal */
		{50, 50, 50, 50},
		/* vivid */
		{50, 40, 50, 50},
		/* soft */
	};

	config->enhance_mode =
	    (config->enhance_mode > CSC_ENHANCE_MODE_NUM - 1)
	     ? g_dcsc_config[sel].enhance_mode : config->enhance_mode;
	enhance_mode = config->enhance_mode;
	config->brightness = csc_enhance_setting[enhance_mode][0];
	config->contrast = csc_enhance_setting[enhance_mode][1];
	config->saturation = csc_enhance_setting[enhance_mode][2];
	config->hue = csc_enhance_setting[enhance_mode][3];

	__inf("sel=%d, in_fmt=%d, mode=%d, out_fmt=%d, mode=%d, range=%d\n",
	      sel, config->in_fmt, config->in_mode, config->out_fmt,
	      config->out_mode, config->out_color_range);

	memcpy(&g_dcsc_config[sel], config, sizeof(struct disp_csc_config));
	de_csc_coeff_calc(config->in_fmt, config->in_mode, config->out_fmt,
			  config->out_mode, config->brightness,
			  config->contrast, config->saturation, config->hue,
			  config->out_color_range, csc_coeff);

	if (is_in_smbl[sel]) {
		dcsc2_dev[sel]->c00.dwval = *(csc_coeff);
		dcsc2_dev[sel]->c01.dwval = *(csc_coeff + 1);
		dcsc2_dev[sel]->c02.dwval = *(csc_coeff + 2);
		dcsc2_dev[sel]->c03.dwval = *(csc_coeff + 3) >> 6;
		dcsc2_dev[sel]->c10.dwval = *(csc_coeff + 4);
		dcsc2_dev[sel]->c11.dwval = *(csc_coeff + 5);
		dcsc2_dev[sel]->c12.dwval = *(csc_coeff + 6);
		dcsc2_dev[sel]->c13.dwval = *(csc_coeff + 7) >> 6;
		dcsc2_dev[sel]->c20.dwval = *(csc_coeff + 8);
		dcsc2_dev[sel]->c21.dwval = *(csc_coeff + 9);
		dcsc2_dev[sel]->c22.dwval = *(csc_coeff + 10);
		dcsc2_dev[sel]->c23.dwval = *(csc_coeff + 11) >> 6;
		dcsc2_dev[sel]->bypass.bits.enable = 1;
		/* always enable csc */
	} else {
		dcsc_dev[sel]->c00.dwval = *(csc_coeff);
		dcsc_dev[sel]->c01.dwval = *(csc_coeff + 1);
		dcsc_dev[sel]->c02.dwval = *(csc_coeff + 2);
		dcsc_dev[sel]->c03.dwval = *(csc_coeff + 3) + 0x200;
		dcsc_dev[sel]->c10.dwval = *(csc_coeff + 4);
		dcsc_dev[sel]->c11.dwval = *(csc_coeff + 5);
		dcsc_dev[sel]->c12.dwval = *(csc_coeff + 6);
		dcsc_dev[sel]->c13.dwval = *(csc_coeff + 7) + 0x200;
		dcsc_dev[sel]->c20.dwval = *(csc_coeff + 8);
		dcsc_dev[sel]->c21.dwval = *(csc_coeff + 9);
		dcsc_dev[sel]->c22.dwval = *(csc_coeff + 10);
		dcsc_dev[sel]->c23.dwval = *(csc_coeff + 11) + 0x200;
		dcsc_dev[sel]->bypass.bits.enable = 1;
		/* always enable csc */
	}

	dcsc_coeff_block[sel].dirty = 1;
	dcsc_enable_block[sel].dirty = 1;

	return 0;
}

int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config)
{
	memcpy(config, &g_dcsc_config[sel], sizeof(struct disp_csc_config));

	return 0;
}

int de_dcsc_update_regs(unsigned int sel)
{
	unsigned int reg_val;
	if (dcsc_enable_block[sel].dirty == 0x1) {
		if (is_in_smbl[sel]) {
			reg_val =
			    readl((void __iomem *)dcsc_enable_block[sel].off);
			reg_val &= 0xfffffffd;
			reg_val |=
			    (*((unsigned int *)dcsc_enable_block[sel].val));
			writel(reg_val,
			       (void __iomem *)dcsc_enable_block[sel].off);
		} else {
			memcpy((void *)dcsc_enable_block[sel].off,
			   dcsc_enable_block[sel].val,
			   dcsc_enable_block[sel].size);
		}
		dcsc_enable_block[sel].dirty = 0x0;
	}

	if (dcsc_coeff_block[sel].dirty == 0x1) {
		memcpy((void *)dcsc_coeff_block[sel].off,
		   dcsc_coeff_block[sel].val, dcsc_coeff_block[sel].size);
		dcsc_coeff_block[sel].dirty = 0x0;
	}

	return 0;
}

int de_dcsc_init(disp_bsp_init_para *para)
{
	uintptr_t base;
	void *memory;
	int screen_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		is_in_smbl[screen_id] = de_feat_is_support_smbl(screen_id);

		base = para->reg_base[DISP_MOD_DE]
		    + (screen_id + 1) * 0x00100000 + DCSC_OFST;

		__inf("sel %d, Dcsc_base=0x%p\n", screen_id, (void *)base);

		if (is_in_smbl[screen_id]) {
			memory = kmalloc(sizeof(struct __csc2_reg_t),
			    GFP_KERNEL | __GFP_ZERO);
			if (NULL == memory) {
				__wrn("malloc Ccsc[%d] mm fail! size=0x%x\n",
				     screen_id,
				     (unsigned int)sizeof(struct __csc2_reg_t));
				return -1;
			}

			dcsc_enable_block[screen_id].off = base;
			dcsc_enable_block[screen_id].val = memory;
			dcsc_enable_block[screen_id].size = 0x04;
			dcsc_enable_block[screen_id].dirty = 0;

			dcsc_coeff_block[screen_id].off = base + 0x80;
			dcsc_coeff_block[screen_id].val = memory + 0x80;
			dcsc_coeff_block[screen_id].size = 0x30;
			dcsc_coeff_block[screen_id].dirty = 0;

		} else {
			memory =  kmalloc(sizeof(struct __csc_reg_t),
			    GFP_KERNEL | __GFP_ZERO);
			if (NULL == memory) {
				__wrn("malloc Ccsc[%d] mm fail! size=0x%x\n",
				     screen_id,
				     (unsigned int)sizeof(struct __csc_reg_t));
				return -1;
			}

			dcsc_enable_block[screen_id].off = base;
			dcsc_enable_block[screen_id].val = memory;
			dcsc_enable_block[screen_id].size = 0x04;
			dcsc_enable_block[screen_id].dirty = 0;

			dcsc_coeff_block[screen_id].off = base + 0x10;
			dcsc_coeff_block[screen_id].val = memory + 0x10;
			dcsc_coeff_block[screen_id].size = 0x30;
			dcsc_coeff_block[screen_id].dirty = 0;
		}
		g_dcsc_config[screen_id].enhance_mode = 0;
		de_dcsc_set_reg_base(screen_id, memory);
	}

	return 0;
}
