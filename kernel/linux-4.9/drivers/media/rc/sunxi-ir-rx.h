/*
 * linux-4.4/drivers/media/rc/sunxi-ir-rx.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#ifndef SUNXI_IR_RX_H
#define SUNXI_IR_RX_H

/* Registers */
#define IR_CTRL_REG		(0x00)	/* IR Control */
#define IR_RXCFG_REG		(0x10)	/* Rx Config */
#define IR_RXDAT_REG		(0x20)	/* Rx Data */
#define IR_RXINTE_REG		(0x2C)	/* Rx Interrupt Enable */
#define IR_RXINTS_REG		(0x30)	/* Rx Interrupt Status */
#define IR_SPLCFG_REG		(0x34)	/* IR Sample Config */

#define IR_FIFO_SIZE		(64)	/* 64Bytes */

#define IR_SIMPLE_UNIT		(21000)		/* simple in ns */
#define IR_CLK			(24000000)	/* 24Mhz */
#define IR_SAMPLE_DEV		(0x3<<0)	/* 24MHz/512 =46875Hz (~21us) */

/* Active Threshold (0+1)*128clock*21us = 2.6ms */
#define IR_ACTIVE_T		((0&0xff)<<16)

/* Filter Threshold = 16*21us = 336us < 500us */
#define IR_RXFILT_VAL		(((16)&0x3f)<<2)

/* Filter Threshold = 22*21us = 336us < 500us */
#define IR_RXFILT_VAL_RC5	(((22)&0x3f)<<2)

/* Idle Threshold = (5+1)*128clock*21us = 16ms > 9ms */
#define IR_RXIDLE_VAL		(((5)&0xff)<<8)

/* Active Threshold (0+1)*128clock*21us = 2.6ms */
#define IR_ACTIVE_T_SAMPLE	((16&0xff)<<16)

#define IR_ACTIVE_T_C		(1<<23)		/* Active Threshold */
#define IR_CIR_MODE		(0x3<<4)	/* CIR mode enable */
#define IR_ENTIRE_ENABLE	(0x3<<0)	/* IR entire enable */
#define IR_FIFO_20		(((20)-1)<<8)
#define IR_IRQ_STATUS		((0x1<<4)|0x3)
#define IR_BOTH_PULSE		(0x1 << 6)
#define IR_LOW_PULSE		(0x2 << 6)
#define IR_HIGH_PULSE		(0x3 << 6)

/*Bit Definition of IR_RXINTS_REG Register*/
#define IR_RXINTS_RXOF		(0x1<<0)	/* Rx FIFO Overflow */
#define IR_RXINTS_RXPE		(0x1<<1)	/* Rx Packet End */
#define IR_RXINTS_RXDA		(0x1<<4)	/* Rx FIFO Data Available */

#define MAX_ADDR_NUM		(32)

#define RTC_108				0x07000120

#define RC_MAP_SUNXI "rc_map_sunxi"

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

enum ir_mode {
	CIR_MODE_ENABLE,
	IR_MODULE_ENABLE,
	IR_BOTH_PULSE_MODE, /* new feature to avoid noisy */
	IR_LOW_PULSE_MODE,
	IR_HIGH_PULSE_MODE,
};

enum ir_sample_config {
	IR_SAMPLE_REG_CLEAR,
	IR_CLK_SAMPLE,
	IR_FILTER_TH_NEC,
	IR_FILTER_TH_RC5,
	IR_IDLE_TH,
	IR_ACTIVE_TH,
	IR_ACTIVE_TH_SAMPLE,
};

enum ir_irq_config {
	IR_IRQ_STATUS_CLEAR,
	IR_IRQ_ENABLE,
	IR_IRQ_FIFO_SIZE,
};

enum {
	IR_SUPLY_DISABLE = 0,
	IR_SUPLY_ENABLE,
};

struct sunxi_ir_data {
	void __iomem *reg_base;
	struct platform_device	*pdev;
	struct clk *mclk;
	struct clk *pclk;
	struct rc_dev *rcdev;
	struct regulator *suply;
	struct pinctrl *pctrl;
	u32 suply_vol;
	int irq_num;
	u32 ir_protocols;
	u32 ir_addr_cnt;
	u32 ir_addr[MAX_ADDR_NUM];
	u32 ir_powerkey[MAX_ADDR_NUM];
	int wakeup;
};

int init_sunxi_ir_map(void);
void exit_sunxi_ir_map(void);
#ifdef CONFIG_SUNXI_KEYMAPPING_SUPPORT
int init_sunxi_ir_map_ext(void *addr, int num);
#endif
#endif /* SUNXI_IR_RX_H */
