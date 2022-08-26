/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Register definitions for Rockchip's RK808/RK818 PMIC
 *
 * Copyright (c) 2014, Fuzhou Rockchip Electronics Co., Ltd
 *
 * Author: Chris Zhong <zyw@rock-chips.com>
 * Author: Zhang Qing <zhangqing@rock-chips.com>
 *
 * Copyright (C) 2016 PHYTEC Messtechnik GmbH
 *
 * Author: Wadim Egorov <w.egorov@phytec.de>
 */

#ifndef __LINUX_REGULATOR_RK808_H
#define __LINUX_REGULATOR_RK808_H

#include <linux/regulator/machine.h>
#include <linux/regmap.h>

/*
 * rk808 Global Register Map.
 */

#define RK808_DCDC1	0 /* (0+RK808_START) */
#define RK808_LDO1	4 /* (4+RK808_START) */
#define RK808_NUM_REGULATORS	14

enum rk808_reg {
	RK808_ID_DCDC1,
	RK808_ID_DCDC2,
	RK808_ID_DCDC3,
	RK808_ID_DCDC4,
	RK808_ID_LDO1,
	RK808_ID_LDO2,
	RK808_ID_LDO3,
	RK808_ID_LDO4,
	RK808_ID_LDO5,
	RK808_ID_LDO6,
	RK808_ID_LDO7,
	RK808_ID_LDO8,
	RK808_ID_SWITCH1,
	RK808_ID_SWITCH2,
};

#define RK808_SECONDS_REG	0x00
#define RK808_MINUTES_REG	0x01
#define RK808_HOURS_REG		0x02
#define RK808_DAYS_REG		0x03
#define RK808_MONTHS_REG	0x04
#define RK808_YEARS_REG		0x05
#define RK808_WEEKS_REG		0x06
#define RK808_ALARM_SECONDS_REG	0x08
#define RK808_ALARM_MINUTES_REG	0x09
#define RK808_ALARM_HOURS_REG	0x0a
#define RK808_ALARM_DAYS_REG	0x0b
#define RK808_ALARM_MONTHS_REG	0x0c
#define RK808_ALARM_YEARS_REG	0x0d
#define RK808_RTC_CTRL_REG	0x10
#define RK808_RTC_STATUS_REG	0x11
#define RK808_RTC_INT_REG	0x12
#define RK808_RTC_COMP_LSB_REG	0x13
#define RK808_RTC_COMP_MSB_REG	0x14
#define RK808_ID_MSB		0x17
#define RK808_ID_LSB		0x18
#define RK808_CLK32OUT_REG	0x20
#define RK808_VB_MON_REG	0x21
#define RK808_THERMAL_REG	0x22
#define RK808_DCDC_EN_REG	0x23
#define RK808_LDO_EN_REG	0x24
#define RK808_SLEEP_SET_OFF_REG1	0x25
#define RK808_SLEEP_SET_OFF_REG2	0x26
#define RK808_DCDC_UV_STS_REG	0x27
#define RK808_DCDC_UV_ACT_REG	0x28
#define RK808_LDO_UV_STS_REG	0x29
#define RK808_LDO_UV_ACT_REG	0x2a
#define RK808_DCDC_PG_REG	0x2b
#define RK808_LDO_PG_REG	0x2c
#define RK808_VOUT_MON_TDB_REG	0x2d
#define RK808_BUCK1_CONFIG_REG		0x2e
#define RK808_BUCK1_ON_VSEL_REG		0x2f
#define RK808_BUCK1_SLP_VSEL_REG	0x30
#define RK808_BUCK1_DVS_VSEL_REG	0x31
#define RK808_BUCK2_CONFIG_REG		0x32
#define RK808_BUCK2_ON_VSEL_REG		0x33
#define RK808_BUCK2_SLP_VSEL_REG	0x34
#define RK808_BUCK2_DVS_VSEL_REG	0x35
#define RK808_BUCK3_CONFIG_REG		0x36
#define RK808_BUCK4_CONFIG_REG		0x37
#define RK808_BUCK4_ON_VSEL_REG		0x38
#define RK808_BUCK4_SLP_VSEL_REG	0x39
#define RK808_BOOST_CONFIG_REG		0x3a
#define RK808_LDO1_ON_VSEL_REG		0x3b
#define RK808_LDO1_SLP_VSEL_REG		0x3c
#define RK808_LDO2_ON_VSEL_REG		0x3d
#define RK808_LDO2_SLP_VSEL_REG		0x3e
#define RK808_LDO3_ON_VSEL_REG		0x3f
#define RK808_LDO3_SLP_VSEL_REG		0x40
#define RK808_LDO4_ON_VSEL_REG		0x41
#define RK808_LDO4_SLP_VSEL_REG		0x42
#define RK808_LDO5_ON_VSEL_REG		0x43
#define RK808_LDO5_SLP_VSEL_REG		0x44
#define RK808_LDO6_ON_VSEL_REG		0x45
#define RK808_LDO6_SLP_VSEL_REG		0x46
#define RK808_LDO7_ON_VSEL_REG		0x47
#define RK808_LDO7_SLP_VSEL_REG		0x48
#define RK808_LDO8_ON_VSEL_REG		0x49
#define RK808_LDO8_SLP_VSEL_REG		0x4a
#define RK808_DEVCTRL_REG	0x4b
#define RK808_INT_STS_REG1	0x4c
#define RK808_INT_STS_MSK_REG1	0x4d
#define RK808_INT_STS_REG2	0x4e
#define RK808_INT_STS_MSK_REG2	0x4f
#define RK808_IO_POL_REG	0x50

/* RK818 */
#define RK818_DCDC1			0
#define RK818_LDO1			4
#define RK818_NUM_REGULATORS		17

enum rk818_reg {
	RK818_ID_DCDC1,
	RK818_ID_DCDC2,
	RK818_ID_DCDC3,
	RK818_ID_DCDC4,
	RK818_ID_BOOST,
	RK818_ID_LDO1,
	RK818_ID_LDO2,
	RK818_ID_LDO3,
	RK818_ID_LDO4,
	RK818_ID_LDO5,
	RK818_ID_LDO6,
	RK818_ID_LDO7,
	RK818_ID_LDO8,
	RK818_ID_LDO9,
	RK818_ID_SWITCH,
	RK818_ID_HDMI_SWITCH,
	RK818_ID_OTG_SWITCH,
};

#define RK818_VB_MON_REG		0x21
#define RK818_THERMAL_REG		0x22
#define RK818_DCDC_EN_REG		0x23
#define RK818_LDO_EN_REG		0x24
#define RK818_SLEEP_SET_OFF_REG1	0x25
#define RK818_SLEEP_SET_OFF_REG2	0x26
#define RK818_DCDC_UV_STS_REG		0x27
#define RK818_DCDC_UV_ACT_REG		0x28
#define RK818_LDO_UV_STS_REG		0x29
#define RK818_LDO_UV_ACT_REG		0x2a
#define RK818_DCDC_PG_REG		0x2b
#define RK818_LDO_PG_REG		0x2c
#define RK818_VOUT_MON_TDB_REG		0x2d
#define RK818_BUCK1_CONFIG_REG		0x2e
#define RK818_BUCK1_ON_VSEL_REG		0x2f
#define RK818_BUCK1_SLP_VSEL_REG	0x30
#define RK818_BUCK2_CONFIG_REG		0x32
#define RK818_BUCK2_ON_VSEL_REG		0x33
#define RK818_BUCK2_SLP_VSEL_REG	0x34
#define RK818_BUCK3_CONFIG_REG		0x36
#define RK818_BUCK4_CONFIG_REG		0x37
#define RK818_BUCK4_ON_VSEL_REG		0x38
#define RK818_BUCK4_SLP_VSEL_REG	0x39
#define RK818_BOOST_CONFIG_REG		0x3a
#define RK818_LDO1_ON_VSEL_REG		0x3b
#define RK818_LDO1_SLP_VSEL_REG		0x3c
#define RK818_LDO2_ON_VSEL_REG		0x3d
#define RK818_LDO2_SLP_VSEL_REG		0x3e
#define RK818_LDO3_ON_VSEL_REG		0x3f
#define RK818_LDO3_SLP_VSEL_REG		0x40
#define RK818_LDO4_ON_VSEL_REG		0x41
#define RK818_LDO4_SLP_VSEL_REG		0x42
#define RK818_LDO5_ON_VSEL_REG		0x43
#define RK818_LDO5_SLP_VSEL_REG		0x44
#define RK818_LDO6_ON_VSEL_REG		0x45
#define RK818_LDO6_SLP_VSEL_REG		0x46
#define RK818_LDO7_ON_VSEL_REG		0x47
#define RK818_LDO7_SLP_VSEL_REG		0x48
#define RK818_LDO8_ON_VSEL_REG		0x49
#define RK818_LDO8_SLP_VSEL_REG		0x4a
#define RK818_BOOST_LDO9_ON_VSEL_REG	0x54
#define RK818_BOOST_LDO9_SLP_VSEL_REG	0x55
#define RK818_DEVCTRL_REG		0x4b
#define RK818_INT_STS_REG1		0X4c
#define RK818_INT_STS_MSK_REG1		0x4d
#define RK818_INT_STS_REG2		0x4e
#define RK818_INT_STS_MSK_REG2		0x4f
#define RK818_IO_POL_REG		0x50
#define RK818_OTP_VDD_EN_REG		0x51
#define RK818_H5V_EN_REG		0x52
#define RK818_SLEEP_SET_OFF_REG3	0x53
#define RK818_BOOST_LDO9_ON_VSEL_REG	0x54
#define RK818_BOOST_LDO9_SLP_VSEL_REG	0x55
#define RK818_BOOST_CTRL_REG		0x56
#define RK818_DCDC_ILMAX_REG		0x90
#define RK818_CHRG_COMP_REG		0x9a
#define RK818_SUP_STS_REG		0xa0
#define RK818_USB_CTRL_REG		0xa1
#define RK818_CHRG_CTRL_REG1		0xa3
#define RK818_CHRG_CTRL_REG2		0xa4
#define RK818_CHRG_CTRL_REG3		0xa5
#define RK818_BAT_CTRL_REG		0xa6
#define RK818_BAT_HTS_TS1_REG		0xa8
#define RK818_BAT_LTS_TS1_REG		0xa9
#define RK818_BAT_HTS_TS2_REG		0xaa
#define RK818_BAT_LTS_TS2_REG		0xab
#define RK818_TS_CTRL_REG		0xac
#define RK818_ADC_CTRL_REG		0xad
#define RK818_ON_SOURCE_REG		0xae
#define RK818_OFF_SOURCE_REG		0xaf
#define RK818_GGCON_REG			0xb0
#define RK818_GGSTS_REG			0xb1
#define RK818_FRAME_SMP_INTERV_REG	0xb2
#define RK818_AUTO_SLP_CUR_THR_REG	0xb3
#define RK818_GASCNT_CAL_REG3		0xb4
#define RK818_GASCNT_CAL_REG2		0xb5
#define RK818_GASCNT_CAL_REG1		0xb6
#define RK818_GASCNT_CAL_REG0		0xb7
#define RK818_GASCNT3_REG		0xb8
#define RK818_GASCNT2_REG		0xb9
#define RK818_GASCNT1_REG		0xba
#define RK818_GASCNT0_REG		0xbb
#define RK818_BAT_CUR_AVG_REGH		0xbc
#define RK818_BAT_CUR_AVG_REGL		0xbd
#define RK818_TS1_ADC_REGH		0xbe
#define RK818_TS1_ADC_REGL		0xbf
#define RK818_TS2_ADC_REGH		0xc0
#define RK818_TS2_ADC_REGL		0xc1
#define RK818_BAT_OCV_REGH		0xc2
#define RK818_BAT_OCV_REGL		0xc3
#define RK818_BAT_VOL_REGH		0xc4
#define RK818_BAT_VOL_REGL		0xc5
#define RK818_RELAX_ENTRY_THRES_REGH	0xc6
#define RK818_RELAX_ENTRY_THRES_REGL	0xc7
#define RK818_RELAX_EXIT_THRES_REGH	0xc8
#define RK818_RELAX_EXIT_THRES_REGL	0xc9
#define RK818_RELAX_VOL1_REGH		0xca
#define RK818_RELAX_VOL1_REGL		0xcb
#define RK818_RELAX_VOL2_REGH		0xcc
#define RK818_RELAX_VOL2_REGL		0xcd
#define RK818_BAT_CUR_R_CALC_REGH	0xce
#define RK818_BAT_CUR_R_CALC_REGL	0xcf
#define RK818_BAT_VOL_R_CALC_REGH	0xd0
#define RK818_BAT_VOL_R_CALC_REGL	0xd1
#define RK818_CAL_OFFSET_REGH		0xd2
#define RK818_CAL_OFFSET_REGL		0xd3
#define RK818_NON_ACT_TIMER_CNT_REG	0xd4
#define RK818_VCALIB0_REGH		0xd5
#define RK818_VCALIB0_REGL		0xd6
#define RK818_VCALIB1_REGH		0xd7
#define RK818_VCALIB1_REGL		0xd8
#define RK818_IOFFSET_REGH		0xdd
#define RK818_IOFFSET_REGL		0xde
#define RK818_SOC_REG			0xe0
#define RK818_REMAIN_CAP_REG3		0xe1
#define RK818_REMAIN_CAP_REG2		0xe2
#define RK818_REMAIN_CAP_REG1		0xe3
#define RK818_REMAIN_CAP_REG0		0xe4
#define RK818_UPDAT_LEVE_REG		0xe5
#define RK818_NEW_FCC_REG3		0xe6
#define RK818_NEW_FCC_REG2		0xe7
#define RK818_NEW_FCC_REG1		0xe8
#define RK818_NEW_FCC_REG0		0xe9
#define RK818_NON_ACT_TIMER_CNT_SAVE_REG 0xea
#define RK818_OCV_VOL_VALID_REG		0xeb
#define RK818_REBOOT_CNT_REG		0xec
#define RK818_POFFSET_REG		0xed
#define RK818_MISC_MARK_REG		0xee
#define RK818_HALT_CNT_REG		0xef
#define RK818_CALC_REST_REGH		0xf0
#define RK818_CALC_REST_REGL		0xf1
#define RK818_SAVE_DATA19		0xf2

#define RK818_H5V_EN			BIT(0)
#define RK818_REF_RDY_CTRL		BIT(1)
#define RK818_USB_ILIM_SEL_MASK		0xf
#define RK818_USB_ILMIN_2000MA		0x7
#define RK818_USB_CHG_SD_VSEL_MASK	0x70

/* RK805 */
enum rk805_reg {
	RK805_ID_DCDC1,
	RK805_ID_DCDC2,
	RK805_ID_DCDC3,
	RK805_ID_DCDC4,
	RK805_ID_LDO1,
	RK805_ID_LDO2,
	RK805_ID_LDO3,
};

/* CONFIG REGISTER */
#define RK805_VB_MON_REG		0x21
#define RK805_THERMAL_REG		0x22

/* POWER CHANNELS ENABLE REGISTER */
#define RK805_DCDC_EN_REG		0x23
#define RK805_SLP_DCDC_EN_REG		0x25
#define RK805_SLP_LDO_EN_REG		0x26
#define RK805_LDO_EN_REG		0x27

/* BUCK AND LDO CONFIG REGISTER */
#define RK805_BUCK_LDO_SLP_LP_EN_REG	0x2A
#define RK805_BUCK1_CONFIG_REG		0x2E
#define RK805_BUCK1_ON_VSEL_REG		0x2F
#define RK805_BUCK1_SLP_VSEL_REG	0x30
#define RK805_BUCK2_CONFIG_REG		0x32
#define RK805_BUCK2_ON_VSEL_REG		0x33
#define RK805_BUCK2_SLP_VSEL_REG	0x34
#define RK805_BUCK3_CONFIG_REG		0x36
#define RK805_BUCK4_CONFIG_REG		0x37
#define RK805_BUCK4_ON_VSEL_REG		0x38
#define RK805_BUCK4_SLP_VSEL_REG	0x39
#define RK805_LDO1_ON_VSEL_REG		0x3B
#define RK805_LDO1_SLP_VSEL_REG		0x3C
#define RK805_LDO2_ON_VSEL_REG		0x3D
#define RK805_LDO2_SLP_VSEL_REG		0x3E
#define RK805_LDO3_ON_VSEL_REG		0x3F
#define RK805_LDO3_SLP_VSEL_REG		0x40

/* INTERRUPT REGISTER */
#define RK805_PWRON_LP_INT_TIME_REG	0x47
#define RK805_PWRON_DB_REG		0x48
#define RK805_DEV_CTRL_REG		0x4B
#define RK805_INT_STS_REG		0x4C
#define RK805_INT_STS_MSK_REG		0x4D
#define RK805_GPIO_IO_POL_REG		0x50
#define RK805_OUT_REG			0x52
#define RK805_ON_SOURCE_REG		0xAE
#define RK805_OFF_SOURCE_REG		0xAF

#define RK805_NUM_REGULATORS		7

#define RK805_PWRON_FALL_RISE_INT_EN	0x0
#define RK805_PWRON_FALL_RISE_INT_MSK	0x81

/* RK805 IRQ Definitions */
#define RK805_IRQ_PWRON_RISE		0
#define RK805_IRQ_VB_LOW		1
#define RK805_IRQ_PWRON			2
#define RK805_IRQ_PWRON_LP		3
#define RK805_IRQ_HOTDIE		4
#define RK805_IRQ_RTC_ALARM		5
#define RK805_IRQ_RTC_PERIOD		6
#define RK805_IRQ_PWRON_FALL		7

#define RK805_IRQ_PWRON_RISE_MSK	BIT(0)
#define RK805_IRQ_VB_LOW_MSK		BIT(1)
#define RK805_IRQ_PWRON_MSK		BIT(2)
#define RK805_IRQ_PWRON_LP_MSK		BIT(3)
#define RK805_IRQ_HOTDIE_MSK		BIT(4)
#define RK805_IRQ_RTC_ALARM_MSK		BIT(5)
#define RK805_IRQ_RTC_PERIOD_MSK	BIT(6)
#define RK805_IRQ_PWRON_FALL_MSK	BIT(7)

#define RK805_PWR_RISE_INT_STATUS	BIT(0)
#define RK805_VB_LOW_INT_STATUS		BIT(1)
#define RK805_PWRON_INT_STATUS		BIT(2)
#define RK805_PWRON_LP_INT_STATUS	BIT(3)
#define RK805_HOTDIE_INT_STATUS		BIT(4)
#define RK805_ALARM_INT_STATUS		BIT(5)
#define RK805_PERIOD_INT_STATUS		BIT(6)
#define RK805_PWR_FALL_INT_STATUS	BIT(7)

#define RK805_BUCK1_2_ILMAX_MASK	(3 << 6)
#define RK805_BUCK3_4_ILMAX_MASK        (3 << 3)
#define RK805_RTC_PERIOD_INT_MASK	(1 << 6)
#define RK805_RTC_ALARM_INT_MASK	(1 << 5)
#define RK805_INT_ALARM_EN		(1 << 3)
#define RK805_INT_TIMER_EN		(1 << 2)

/* RK808 IRQ Definitions */
#define RK808_IRQ_VOUT_LO	0
#define RK808_IRQ_VB_LO		1
#define RK808_IRQ_PWRON		2
#define RK808_IRQ_PWRON_LP	3
#define RK808_IRQ_HOTDIE	4
#define RK808_IRQ_RTC_ALARM	5
#define RK808_IRQ_RTC_PERIOD	6
#define RK808_IRQ_PLUG_IN_INT	7
#define RK808_IRQ_PLUG_OUT_INT	8
#define RK808_NUM_IRQ		9

#define RK808_IRQ_VOUT_LO_MSK		BIT(0)
#define RK808_IRQ_VB_LO_MSK		BIT(1)
#define RK808_IRQ_PWRON_MSK		BIT(2)
#define RK808_IRQ_PWRON_LP_MSK		BIT(3)
#define RK808_IRQ_HOTDIE_MSK		BIT(4)
#define RK808_IRQ_RTC_ALARM_MSK		BIT(5)
#define RK808_IRQ_RTC_PERIOD_MSK	BIT(6)
#define RK808_IRQ_PLUG_IN_INT_MSK	BIT(0)
#define RK808_IRQ_PLUG_OUT_INT_MSK	BIT(1)

/* RK818 IRQ Definitions */
#define RK818_IRQ_VOUT_LO	0
#define RK818_IRQ_VB_LO		1
#define RK818_IRQ_PWRON		2
#define RK818_IRQ_PWRON_LP	3
#define RK818_IRQ_HOTDIE	4
#define RK818_IRQ_RTC_ALARM	5
#define RK818_IRQ_RTC_PERIOD	6
#define RK818_IRQ_USB_OV	7
#define RK818_IRQ_PLUG_IN	8
#define RK818_IRQ_PLUG_OUT	9
#define RK818_IRQ_CHG_OK	10
#define RK818_IRQ_CHG_TE	11
#define RK818_IRQ_CHG_TS1	12
#define RK818_IRQ_TS2		13
#define RK818_IRQ_CHG_CVTLIM	14
#define RK818_IRQ_DISCHG_ILIM	15

#define RK818_IRQ_VOUT_LO_MSK		BIT(0)
#define RK818_IRQ_VB_LO_MSK		BIT(1)
#define RK818_IRQ_PWRON_MSK		BIT(2)
#define RK818_IRQ_PWRON_LP_MSK		BIT(3)
#define RK818_IRQ_HOTDIE_MSK		BIT(4)
#define RK818_IRQ_RTC_ALARM_MSK		BIT(5)
#define RK818_IRQ_RTC_PERIOD_MSK	BIT(6)
#define RK818_IRQ_USB_OV_MSK		BIT(7)
#define RK818_IRQ_PLUG_IN_MSK		BIT(0)
#define RK818_IRQ_PLUG_OUT_MSK		BIT(1)
#define RK818_IRQ_CHG_OK_MSK		BIT(2)
#define RK818_IRQ_CHG_TE_MSK		BIT(3)
#define RK818_IRQ_CHG_TS1_MSK		BIT(4)
#define RK818_IRQ_TS2_MSK		BIT(5)
#define RK818_IRQ_CHG_CVTLIM_MSK	BIT(6)
#define RK818_IRQ_DISCHG_ILIM_MSK	BIT(7)

#define RK818_NUM_IRQ		16

#define RK808_VBAT_LOW_2V8	0x00
#define RK808_VBAT_LOW_2V9	0x01
#define RK808_VBAT_LOW_3V0	0x02
#define RK808_VBAT_LOW_3V1	0x03
#define RK808_VBAT_LOW_3V2	0x04
#define RK808_VBAT_LOW_3V3	0x05
#define RK808_VBAT_LOW_3V4	0x06
#define RK808_VBAT_LOW_3V5	0x07
#define VBAT_LOW_VOL_MASK	(0x07 << 0)
#define EN_VABT_LOW_SHUT_DOWN	(0x00 << 4)
#define EN_VBAT_LOW_IRQ		(0x1 << 4)
#define VBAT_LOW_ACT_MASK	(0x1 << 4)

#define BUCK_ILMIN_MASK		(7 << 0)
#define BOOST_ILMIN_MASK	(7 << 0)
#define BUCK1_RATE_MASK		(3 << 3)
#define BUCK2_RATE_MASK		(3 << 3)
#define MASK_ALL	0xff

#define BUCK_UV_ACT_MASK	0x0f
#define BUCK_UV_ACT_DISABLE	0

#define SWITCH2_EN	BIT(6)
#define SWITCH1_EN	BIT(5)
#define DEV_OFF_RST	BIT(3)
#define DEV_RST		BIT(2)
#define DEV_OFF		BIT(0)
#define RTC_STOP	BIT(0)

#define VB_LO_ACT		BIT(4)
#define VB_LO_SEL_3500MV	(7 << 0)

#define VOUT_LO_INT	BIT(0)
#define CLK32KOUT2_EN	BIT(0)
#define CLK32KOUT2_FUNC		(0 << 1)
#define CLK32KOUT2_FUNC_MASK	BIT(1)

#define TEMP115C			0x0c
#define TEMP_HOTDIE_MSK			0x0c
#define SLP_SD_MSK			(0x3 << 2)
#define SHUTDOWN_FUN			(0x2 << 2)
#define SLEEP_FUN			(0x1 << 2)
#define RK8XX_ID_MSK			0xfff0
#define PWM_MODE_MSK			BIT(7)
#define FPWM_MODE			BIT(7)
#define AUTO_PWM_MODE			0

enum rk817_reg_id {
	RK817_ID_DCDC1 = 0,
	RK817_ID_DCDC2,
	RK817_ID_DCDC3,
	RK817_ID_DCDC4,
	RK817_ID_LDO1,
	RK817_ID_LDO2,
	RK817_ID_LDO3,
	RK817_ID_LDO4,
	RK817_ID_LDO5,
	RK817_ID_LDO6,
	RK817_ID_LDO7,
	RK817_ID_LDO8,
	RK817_ID_LDO9,
	RK817_ID_BOOST,
	RK817_ID_BOOST_OTG_SW,
	RK817_NUM_REGULATORS
};

enum rk809_reg_id {
	RK809_ID_DCDC5 = RK817_ID_BOOST,
	RK809_ID_SW1,
	RK809_ID_SW2,
	RK809_NUM_REGULATORS
};

#define RK817_SECONDS_REG		0x00
#define RK817_MINUTES_REG		0x01
#define RK817_HOURS_REG			0x02
#define RK817_DAYS_REG			0x03
#define RK817_MONTHS_REG		0x04
#define RK817_YEARS_REG			0x05
#define RK817_WEEKS_REG			0x06
#define RK817_ALARM_SECONDS_REG		0x07
#define RK817_ALARM_MINUTES_REG		0x08
#define RK817_ALARM_HOURS_REG		0x09
#define RK817_ALARM_DAYS_REG		0x0a
#define RK817_ALARM_MONTHS_REG		0x0b
#define RK817_ALARM_YEARS_REG		0x0c
#define RK817_RTC_CTRL_REG		0xd
#define RK817_RTC_STATUS_REG		0xe
#define RK817_RTC_INT_REG		0xf
#define RK817_RTC_COMP_LSB_REG		0x10
#define RK817_RTC_COMP_MSB_REG		0x11

/* RK817 Codec Registers */
#define RK817_CODEC_DTOP_VUCTL		0x12
#define RK817_CODEC_DTOP_VUCTIME	0x13
#define RK817_CODEC_DTOP_LPT_SRST	0x14
#define RK817_CODEC_DTOP_DIGEN_CLKE	0x15
#define RK817_CODEC_AREF_RTCFG0		0x16
#define RK817_CODEC_AREF_RTCFG1		0x17
#define RK817_CODEC_AADC_CFG0		0x18
#define RK817_CODEC_AADC_CFG1		0x19
#define RK817_CODEC_DADC_VOLL		0x1a
#define RK817_CODEC_DADC_VOLR		0x1b
#define RK817_CODEC_DADC_SR_ACL0	0x1e
#define RK817_CODEC_DADC_ALC1		0x1f
#define RK817_CODEC_DADC_ALC2		0x20
#define RK817_CODEC_DADC_NG		0x21
#define RK817_CODEC_DADC_HPF		0x22
#define RK817_CODEC_DADC_RVOLL		0x23
#define RK817_CODEC_DADC_RVOLR		0x24
#define RK817_CODEC_AMIC_CFG0		0x27
#define RK817_CODEC_AMIC_CFG1		0x28
#define RK817_CODEC_DMIC_PGA_GAIN	0x29
#define RK817_CODEC_DMIC_LMT1		0x2a
#define RK817_CODEC_DMIC_LMT2		0x2b
#define RK817_CODEC_DMIC_NG1		0x2c
#define RK817_CODEC_DMIC_NG2		0x2d
#define RK817_CODEC_ADAC_CFG0		0x2e
#define RK817_CODEC_ADAC_CFG1		0x2f
#define RK817_CODEC_DDAC_POPD_DACST	0x30
#define RK817_CODEC_DDAC_VOLL		0x31
#define RK817_CODEC_DDAC_VOLR		0x32
#define RK817_CODEC_DDAC_SR_LMT0	0x35
#define RK817_CODEC_DDAC_LMT1		0x36
#define RK817_CODEC_DDAC_LMT2		0x37
#define RK817_CODEC_DDAC_MUTE_MIXCTL	0x38
#define RK817_CODEC_DDAC_RVOLL		0x39
#define RK817_CODEC_DDAC_RVOLR		0x3a
#define RK817_CODEC_AHP_ANTI0		0x3b
#define RK817_CODEC_AHP_ANTI1		0x3c
#define RK817_CODEC_AHP_CFG0		0x3d
#define RK817_CODEC_AHP_CFG1		0x3e
#define RK817_CODEC_AHP_CP		0x3f
#define RK817_CODEC_ACLASSD_CFG1	0x40
#define RK817_CODEC_ACLASSD_CFG2	0x41
#define RK817_CODEC_APLL_CFG0		0x42
#define RK817_CODEC_APLL_CFG1		0x43
#define RK817_CODEC_APLL_CFG2		0x44
#define RK817_CODEC_APLL_CFG3		0x45
#define RK817_CODEC_APLL_CFG4		0x46
#define RK817_CODEC_APLL_CFG5		0x47
#define RK817_CODEC_DI2S_CKM		0x48
#define RK817_CODEC_DI2S_RSD		0x49
#define RK817_CODEC_DI2S_RXCR1		0x4a
#define RK817_CODEC_DI2S_RXCR2		0x4b
#define RK817_CODEC_DI2S_RXCMD_TSD	0x4c
#define RK817_CODEC_DI2S_TXCR1		0x4d
#define RK817_CODEC_DI2S_TXCR2		0x4e
#define RK817_CODEC_DI2S_TXCR3_TXCMD	0x4f

/* RK817_CODEC_DI2S_CKM */
#define RK817_I2S_MODE_MASK		(0x1 << 0)
#define RK817_I2S_MODE_MST		(0x1 << 0)
#define RK817_I2S_MODE_SLV		(0x0 << 0)

/* RK817_CODEC_DDAC_MUTE_MIXCTL */
#define DACMT_MASK			(0x1 << 0)
#define DACMT_ENABLE			(0x1 << 0)
#define DACMT_DISABLE			(0x0 << 0)

/* RK817_CODEC_DI2S_RXCR2 */
#define VDW_RX_24BITS			(0x17)
#define VDW_RX_16BITS			(0x0f)

/* RK817_CODEC_DI2S_TXCR2 */
#define VDW_TX_24BITS			(0x17)
#define VDW_TX_16BITS			(0x0f)

/* RK817_CODEC_AMIC_CFG0 */
#define MIC_DIFF_MASK			(0x1 << 7)
#define MIC_DIFF_DIS			(0x0 << 7)
#define MIC_DIFF_EN			(0x1 << 7)

#define RK817_POWER_EN_REG(i)		(0xb1 + (i))
#define RK817_POWER_SLP_EN_REG(i)	(0xb5 + (i))

#define RK817_POWER_CONFIG		(0xb9)

#define RK817_BUCK_CONFIG_REG(i)	(0xba + (i) * 3)

#define RK817_BUCK1_ON_VSEL_REG		0xBB
#define RK817_BUCK1_SLP_VSEL_REG	0xBC

#define RK817_BUCK2_CONFIG_REG		0xBD
#define RK817_BUCK2_ON_VSEL_REG		0xBE
#define RK817_BUCK2_SLP_VSEL_REG	0xBF

#define RK817_BUCK3_CONFIG_REG		0xC0
#define RK817_BUCK3_ON_VSEL_REG		0xC1
#define RK817_BUCK3_SLP_VSEL_REG	0xC2

#define RK817_BUCK4_CONFIG_REG		0xC3
#define RK817_BUCK4_ON_VSEL_REG		0xC4
#define RK817_BUCK4_SLP_VSEL_REG	0xC5

#define RK817_LDO_ON_VSEL_REG(idx)	(0xcc + (idx) * 2)
#define RK817_BOOST_OTG_CFG		(0xde)

#define RK817_ID_MSB			0xed
#define RK817_ID_LSB			0xee

#define RK817_SYS_STS			0xf0
#define RK817_SYS_CFG(i)		(0xf1 + (i))

#define RK817_ON_SOURCE_REG		0xf5
#define RK817_OFF_SOURCE_REG		0xf6

/* INTERRUPT REGISTER */
#define RK817_INT_STS_REG0		0xf8
#define RK817_INT_STS_MSK_REG0		0xf9
#define RK817_INT_STS_REG1		0xfa
#define RK817_INT_STS_MSK_REG1		0xfb
#define RK817_INT_STS_REG2		0xfc
#define RK817_INT_STS_MSK_REG2		0xfd
#define RK817_GPIO_INT_CFG		0xfe

/* IRQ Definitions */
#define RK817_IRQ_PWRON_FALL		0
#define RK817_IRQ_PWRON_RISE		1
#define RK817_IRQ_PWRON			2
#define RK817_IRQ_PWMON_LP		3
#define RK817_IRQ_HOTDIE		4
#define RK817_IRQ_RTC_ALARM		5
#define RK817_IRQ_RTC_PERIOD		6
#define RK817_IRQ_VB_LO			7
#define RK817_IRQ_PLUG_IN		8
#define RK817_IRQ_PLUG_OUT		9
#define RK817_IRQ_CHRG_TERM		10
#define RK817_IRQ_CHRG_TIME		11
#define RK817_IRQ_CHRG_TS		12
#define RK817_IRQ_USB_OV		13
#define RK817_IRQ_CHRG_IN_CLMP		14
#define RK817_IRQ_BAT_DIS_ILIM		15
#define RK817_IRQ_GATE_GPIO		16
#define RK817_IRQ_TS_GPIO		17
#define RK817_IRQ_CODEC_PD		18
#define RK817_IRQ_CODEC_PO		19
#define RK817_IRQ_CLASSD_MUTE_DONE	20
#define RK817_IRQ_CLASSD_OCP		21
#define RK817_IRQ_BAT_OVP               22
#define RK817_IRQ_CHRG_BAT_HI		23
#define RK817_IRQ_END			(RK817_IRQ_CHRG_BAT_HI + 1)

/*
 * rtc_ctrl 0xd
 * same as 808, except bit4
 */
#define RK817_RTC_CTRL_RSV4		BIT(4)

/* power config 0xb9 */
#define RK817_BUCK3_FB_RES_MSK		BIT(6)
#define RK817_BUCK3_FB_RES_INTER	BIT(6)
#define RK817_BUCK3_FB_RES_EXT		0

/* buck config 0xba */
#define RK817_RAMP_RATE_OFFSET		6
#define RK817_RAMP_RATE_MASK		(0x3 << RK817_RAMP_RATE_OFFSET)
#define RK817_RAMP_RATE_3MV_PER_US	(0x0 << RK817_RAMP_RATE_OFFSET)
#define RK817_RAMP_RATE_6_3MV_PER_US	(0x1 << RK817_RAMP_RATE_OFFSET)
#define RK817_RAMP_RATE_12_5MV_PER_US	(0x2 << RK817_RAMP_RATE_OFFSET)
#define RK817_RAMP_RATE_25MV_PER_US	(0x3 << RK817_RAMP_RATE_OFFSET)

/* sys_cfg1 0xf2 */
#define RK817_HOTDIE_TEMP_MSK		(0x3 << 4)
#define RK817_HOTDIE_85			(0x0 << 4)
#define RK817_HOTDIE_95			(0x1 << 4)
#define RK817_HOTDIE_105		(0x2 << 4)
#define RK817_HOTDIE_115		(0x3 << 4)

#define RK817_TSD_TEMP_MSK		BIT(6)
#define RK817_TSD_140			0
#define RK817_TSD_160			BIT(6)

#define RK817_CLK32KOUT2_EN		BIT(7)

/* sys_cfg3 0xf4 */
#define RK817_SLPPIN_FUNC_MSK		(0x3 << 3)
#define SLPPIN_NULL_FUN			(0x0 << 3)
#define SLPPIN_SLP_FUN			(0x1 << 3)
#define SLPPIN_DN_FUN			(0x2 << 3)
#define SLPPIN_RST_FUN			(0x3 << 3)

#define RK817_RST_FUNC_MSK		(0x3 << 6)
#define RK817_RST_FUNC_SFT		(6)
#define RK817_RST_FUNC_CNT		(3)
#define RK817_RST_FUNC_DEV		(0) /* reset the dev */
#define RK817_RST_FUNC_REG		(0x1 << 6) /* reset the reg only */

#define RK817_SLPPOL_MSK		BIT(5)
#define RK817_SLPPOL_H			BIT(5)
#define RK817_SLPPOL_L			(0)

/* gpio&int 0xfe */
#define RK817_INT_POL_MSK		BIT(1)
#define RK817_INT_POL_H			BIT(1)
#define RK817_INT_POL_L			0
#define RK809_BUCK5_CONFIG(i)		(RK817_BOOST_OTG_CFG + (i) * 1)

enum {
	BUCK_ILMIN_50MA,
	BUCK_ILMIN_100MA,
	BUCK_ILMIN_150MA,
	BUCK_ILMIN_200MA,
	BUCK_ILMIN_250MA,
	BUCK_ILMIN_300MA,
	BUCK_ILMIN_350MA,
	BUCK_ILMIN_400MA,
};

enum {
	BOOST_ILMIN_75MA,
	BOOST_ILMIN_100MA,
	BOOST_ILMIN_125MA,
	BOOST_ILMIN_150MA,
	BOOST_ILMIN_175MA,
	BOOST_ILMIN_200MA,
	BOOST_ILMIN_225MA,
	BOOST_ILMIN_250MA,
};

enum {
	RK805_BUCK1_2_ILMAX_2500MA,
	RK805_BUCK1_2_ILMAX_3000MA,
	RK805_BUCK1_2_ILMAX_3500MA,
	RK805_BUCK1_2_ILMAX_4000MA,
};

enum {
	RK805_BUCK3_ILMAX_1500MA,
	RK805_BUCK3_ILMAX_2000MA,
	RK805_BUCK3_ILMAX_2500MA,
	RK805_BUCK3_ILMAX_3000MA,
};

enum {
	RK805_BUCK4_ILMAX_2000MA,
	RK805_BUCK4_ILMAX_2500MA,
	RK805_BUCK4_ILMAX_3000MA,
	RK805_BUCK4_ILMAX_3500MA,
};

enum {
	RK805_ID = 0x8050,
	RK808_ID = 0x0000,
	RK809_ID = 0x8090,
	RK817_ID = 0x8170,
	RK818_ID = 0x8180,
};

struct rk808 {
	struct i2c_client		*i2c;
	struct regmap_irq_chip_data	*irq_data;
	struct regmap			*regmap;
	long				variant;
	const struct regmap_config	*regmap_cfg;
	const struct regmap_irq_chip	*regmap_irq_chip;
};
#endif /* __LINUX_REGULATOR_RK808_H */