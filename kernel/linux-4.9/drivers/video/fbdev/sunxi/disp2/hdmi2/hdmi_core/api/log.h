/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef LOG_H_
#define LOG_H_
#include "../../config.h"

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm.h>
#include <asm/div64.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>

#define TRUE true
#define FALSE false

typedef enum {
	SNPS_ERROR = 0,
	SNPS_WARN,
	SNPS_NOTICE,
	SNPS_DEBUG,
	SNPS_TRACE,
	SNPS_ASSERT
} log_t;

extern u32 hdmi_printf;
#define VIDEO_INF(fmt, args...) \
			do { \
				if ((hdmi_printf == 8) || (hdmi_printf == 4) \
							|| (hdmi_printf > 6)) \
					printk(fmt, ##args); \
			} while (0)


#define EDID_INF(fmt, args...) \
	do { \
		if ((hdmi_printf == 8) || (hdmi_printf == 4) \
					|| (hdmi_printf > 6)) \
			printk(fmt, ##args); \
	} while (0)

#define AUDIO_INF(fmt, args...) \
	do { \
		if ((hdmi_printf == 3) || (hdmi_printf == 4) \
					|| (hdmi_printf > 6)) \
			printk(fmt, ##args); \
	} while (0)


#define CEC_INF(fmt, args...) \
	do { if ((hdmi_printf > 6) || (hdmi_printf == 5)) printk(fmt, ##args); } while (0)

#define HDCP_INF(fmt, args...) \
	do { if (hdmi_printf > 5) printk(fmt, ##args); } while (0)


#if 1
#define LOG_TRACE() \
	do { if (hdmi_printf > 7) pr_warn("[HDMI-FUNCTION]%s\n", __func__); } while (0)
#define LOG_TRACE1(a) \
	do { if (hdmi_printf > 7) pr_warn("[HDMI-FUNCTION]%s:  %d\n", __func__, a); } while (0)
#define LOG_TRACE2(a, b) \
	do { if (hdmi_printf > 7) pr_warn("[HDMI-FUNCTION] %s:  %d  %d\n", __func__, a, b); } while (0)
#define LOG_TRACE3(a, b, c) \
	do { if (hdmi_printf > 7) pr_warn("[HDMI-FUNCTION] %s:  %d  %d  %d\n", __func__, a, b, c); } while (0)
#else
#define LOG_TRACE()
#define LOG_TRACE1(a)
#define LOG_TRACE2(a, b)
#define LOG_TRACE3(a, b, c)
#endif


#endif	/* LOG_H_ */
