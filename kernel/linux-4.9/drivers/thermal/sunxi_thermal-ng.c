// SPDX-License-Identifier: GPL-2.0
/*
 * Thermal sensor driver for Allwinner SOC
 * Copyright (C) 2019 frank@allwinnertech.com
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/thermal.h>

#define MAX_SENSOR_NUM	4

#define FT_TEMP_MASK				GENMASK(11, 0)
#define TEMP_CALIB_MASK				GENMASK(11, 0)
#define CALIBRATE_DEFAULT			0x800

#define SUN50I_H616_THS_CTRL0			0x00
#define SUN50I_H616_THS_ENABLE			0x04
#define SUN50I_H616_THS_PC			0x08
#define SUN50I_H616_THS_MFC			0x30
#define SUN50I_H616_THS_TEMP_CALIB		0xa0
#define SUN50I_H616_THS_TEMP_DATA		0xc0

#define SUN50I_THS_CTRL0_T_ACQ(x)		(GENMASK(15, 0) & (x))
#define SUN50I_THS_CTRL0_FS_DIV(x)		((GENMASK(15, 0) & (x)) << 16)
#define SUN50I_THS_FILTER_EN			BIT(2)
#define SUN50I_THS_FILTER_TYPE(x)		(GENMASK(1, 0) & (x))
#define SUN50I_H616_THS_PC_TEMP_PERIOD(x)	((GENMASK(19, 0) & (x)) << 12)

struct ths_device;

struct tsensor {
	struct ths_device		*tmdev;
	struct thermal_zone_device	*tzd;
	int				id;
};

struct ths_thermal_chip {
	bool            has_bus_clk;
	int		sensor_num;
	int		offset;
	int		scale;
	int		ft_deviation;
	int		temp_data_base;
	int		(*calibrate)(struct ths_device *tmdev,
				     u16 *caldata, int callen);
	int		(*avcc_calibrate)(struct ths_device *tmdev);
	int		(*init)(struct ths_device *tmdev);
};

struct ths_device {
	int					avcc_deviation;
	const struct ths_thermal_chip		*chip;
	struct device				*dev;
	struct regmap				*regmap;
	struct reset_control			*reset;
	struct clk				*bus_clk;
	struct clk                              *mod_clk;
	struct tsensor				sensor[MAX_SENSOR_NUM];
};

/* Temp Unit: millidegree Celsius */
static int sunxi_ths_reg2temp(struct ths_device *tmdev, int reg)
{
	return (reg + tmdev->chip->offset) * tmdev->chip->scale;
}

static int sunxi_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);

	/* ths have no data yet */
	if (unlikely(!val))
		return -EAGAIN;

	*temp = sunxi_ths_reg2temp(tmdev, val);
	*temp += tmdev->chip->ft_deviation;

	/*
	 * The sensor reading is affected by the avcc voltage,
	 * and some pmu power supplies are not accurate enough.
	 *
	 * It is not very happy to stuff these things in the thermal driver.
	 * :(
	 */
	if (tmdev->avcc_deviation)
		*temp += tmdev->avcc_deviation;

	return 0;
}

static const struct thermal_zone_of_device_ops ths_ops = {
	.get_temp = sunxi_ths_get_temp,
};

static const struct regmap_config config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.fast_io = true,
};

/*
 * this calibrate is use cp data
 */
static int sun50i_ths_h616_calibrate_cp(struct ths_device *tmdev, void *data,
					int callen)
{
	int i, ft_temp;
	u32 *caldata = (u32 *)data;

	if (!caldata[0])
		return -EINVAL;

	ft_temp = (caldata[0] >> 24) & GENMASK(5, 0);

	ft_temp += 2;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int tt = (caldata[0] >> (i * 6)) & GENMASK(5, 0);
		u32 offset;
		int cdata;

		cdata = CALIBRATE_DEFAULT - ((tt - ft_temp) * 12401) / 1000;

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap,
				   SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4),
				   0xfff << offset, cdata << offset);
	}

	return 0;
}

/*
 * this calibrate is use ft data
 */
static int sun50i_h616_ths_calibrate(struct ths_device *tmdev,
				   u16 *caldata, int callen)
{
	struct device *dev = tmdev->dev;
	int i, ft_temp;

	if (!caldata[0])
		return -EINVAL;

	/*
	 * efuse layout:
	 *
	 * 0      11  16     27   32     43   48    57
	 * +----------+-----------+-----------+-----------+
	 * |  temp |  |sensor0|   |sensor1|   |sensor2|   |
	 * +----------+-----------+-----------+-----------+
	 *                      ^           ^           ^
	 *                      |           |           |
	 *                      |           |           sensor3[11:8]
	 *                      |           sensor3[7:4]
	 *                      sensor3[3:0]
	 *
	 * The calibration data on the H616 is the ambient temperature and
	 * sensor values that are filled during the factory test stage.
	 *
	 * The unit of stored FT temperature is 0.1 degreee celusis.
	 *
	 * We need to calculate a delta between measured and caluclated
	 * register values and this will become a calibration offset.
	 */
	ft_temp = caldata[0] & FT_TEMP_MASK;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		if (i == 3)
			reg = (caldata[1] >> 12)
			      | (caldata[2] >> 12 << 4)
			      | (caldata[3] >> 12 << 8);
		else
			reg = (int)caldata[i + 1] & TEMP_CALIB_MASK;

		/*
		 * Our calculation formula is like this,
		 * the temp unit above is Celsius:
		 *
		 * T = (sensor_data + a) / b
		 * cdata = 0x800 - [(ft_temp - T) * b]
		 *
		 * b is a floating-point number
		 * with an absolute value less than 1000.
		 *
		 * sunxi_ths_reg2temp uses milli-degrees Celsius,
		 * with offset and scale parameters.
		 * T = (sensor_data + a) * 1000 / b
		 *
		 * ----------------------------------------------
		 *
		 * So:
		 *
		 * offset = a, scale = 1000 / b
		 * cdata = 0x800 - [(ft_temp - T) * 1000 / scale]
		 */
		delta = (ft_temp * 100 - sunxi_ths_reg2temp(tmdev, reg))
			/ tmdev->chip->scale;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			dev_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap,
				   SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4),
				   0xfff << offset,
				   cdata << offset);
	}

	return 0;
}

static int sun50i_h616_ths_avcc_calibrate(struct ths_device *tmdev)
{
	struct nvmem_cell *avcc_calcell;
	struct device *dev = tmdev->dev;
	int ret = 0;
	u16 *caldata;
	size_t callen;
	u8 calib_value;

	avcc_calcell = devm_nvmem_cell_get(dev, "avcc_calibration");
	if (IS_ERR(avcc_calcell)) {
		if (PTR_ERR(avcc_calcell) == -EPROBE_DEFER) {
			return -EPROBE_DEFER;
		} else {
			dev_warn(dev, "avcc is not calibrated\n");
			goto out;
		}
	}

	caldata = nvmem_cell_read(avcc_calcell, &callen);
	if (IS_ERR(caldata)) {
		ret = PTR_ERR(caldata);
		goto out_put;
	}

	/*
	 * efuse layout:
	 *
	 * 0          1           2
	 * +----------+-----------+
	 * |          |           |
	 * +----------+-----------+
	 *          ^^           ^
	 *          ||           |
	 *          ||           bit0
	 *          |bit1
	 *          |
	 *          bit2
	 */

	calib_value = (((*caldata) & BIT(15)) ? BIT(0) : 0) |
		      (((*caldata) & BIT(7)) ? BIT(1) : 0) |
		      (((*caldata) & BIT(6)) ? BIT(2) : 0);

	switch (calib_value) {
	case 0b000:
		tmdev->avcc_deviation = 14500;
		break;
	case 0b100:
		tmdev->avcc_deviation = 10500;
		break;
	case 0b010:
		tmdev->avcc_deviation = -10500;
		break;
	case 0b110:
		tmdev->avcc_deviation = -14500;
		break;
	case 0b101:
		tmdev->avcc_deviation = 6000;
		break;
	case 0b011:
		tmdev->avcc_deviation = -6000;
		break;
	default:
		break;
	}

	kfree(caldata);

out_put:
	devm_nvmem_cell_put(dev, avcc_calcell);
out:
	return ret;
}

static int sun50i_h616_ths_calibrate_wrap(struct ths_device *tmdev,
					  u16 *caldata, int callen)
{
	struct nvmem_device *ndev;
	struct device *dev = tmdev->dev;
	u32 t;
	u16 d;

	ndev = devm_nvmem_device_get(dev, "sid");
	if (IS_ERR(ndev)) {
		if (PTR_ERR(ndev) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		else
			return sun50i_h616_ths_calibrate(tmdev, caldata,
							 callen);
	}

	nvmem_device_read(ndev, 0, 2, &d);

	switch (d) {
	case 0x7400:
	case 0x7c00:
	case 0x2400:
	case 0x2c00:
		break;
	case 0x5000:
	case 0x5400:
	case 0x5c00:
	default:
		/* ft style */
		devm_nvmem_device_put(dev, ndev);
		return sun50i_h616_ths_calibrate(tmdev, caldata, callen);
	}

	/* cp style */
	nvmem_device_read(ndev, 0x1c, 4, &t);
	sun50i_ths_h616_calibrate_cp(tmdev, &t, 4);

	devm_nvmem_device_put(dev, ndev);

	return 0;
}

static int sunxi_ths_calibrate(struct ths_device *tmdev)
{
	struct nvmem_cell *calcell;
	struct device *dev = tmdev->dev;
	u16 *caldata;
	size_t callen;
	int ret = 0;

	calcell = devm_nvmem_cell_get(dev, "calibration");
	if (IS_ERR(calcell)) {
		if (PTR_ERR(calcell) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		goto out;
	}

	caldata = nvmem_cell_read(calcell, &callen);
	if (IS_ERR(caldata)) {
		ret = PTR_ERR(caldata);
		goto put;
	}

	tmdev->chip->calibrate(tmdev, caldata, callen);

	kfree(caldata);

	if (tmdev->chip->avcc_calibrate)
		ret = tmdev->chip->avcc_calibrate(tmdev);

put:
	devm_nvmem_cell_put(dev, calcell);
out:
	return ret;
}

static int sunxi_ths_resource_init(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *mem;
	void __iomem *base;
	int ret;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, mem);
	if (IS_ERR(base))
		return PTR_ERR(base);

	tmdev->regmap = devm_regmap_init_mmio(dev, base, &config);
	if (IS_ERR(tmdev->regmap))
		return PTR_ERR(tmdev->regmap);

	if (tmdev->chip->has_bus_clk) {
		tmdev->bus_clk = devm_clk_get(&pdev->dev, "bus");
		if (IS_ERR(tmdev->bus_clk))
			return PTR_ERR(tmdev->bus_clk);
	}

	ret = clk_prepare_enable(tmdev->bus_clk);
	if (ret)
		return ret;

	ret = sunxi_ths_calibrate(tmdev);
	if (ret)
		goto bus_disable;

	return 0;

bus_disable:
	clk_disable_unprepare(tmdev->bus_clk);

	return ret;
}

static int sun50i_h616_thermal_init(struct ths_device *tmdev)
{
	int val;

	/*
	 * For sun50iw9p1:
	 * It is necessary that reg[0x03000000] bit[16] is 0.
	 */
	regmap_write(tmdev->regmap, SUN50I_H616_THS_CTRL0,
		     SUN50I_THS_CTRL0_T_ACQ(47) | SUN50I_THS_CTRL0_FS_DIV(479));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_MFC,
		     SUN50I_THS_FILTER_EN |
		     SUN50I_THS_FILTER_TYPE(1));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_PC,
		     SUN50I_H616_THS_PC_TEMP_PERIOD(365));
	val = GENMASK(tmdev->chip->sensor_num - 1, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_ENABLE, val);

	return 0;
}

static int sunxi_ths_register(struct ths_device *tmdev)
{
	int i;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		tmdev->sensor[i].tmdev = tmdev;
		tmdev->sensor[i].id = i;
		tmdev->sensor[i].tzd =
			devm_thermal_zone_of_sensor_register(tmdev->dev,
							     i,
							     &tmdev->sensor[i],
							     &ths_ops);
		if (IS_ERR(tmdev->sensor[i].tzd))
			return PTR_ERR(tmdev->sensor[i].tzd);
	}

	return 0;
}

static int sunxi_ths_probe(struct platform_device *pdev)
{
	struct ths_device *tmdev;
	struct device *dev = &pdev->dev;
	int ret;

	tmdev = devm_kzalloc(dev, sizeof(*tmdev), GFP_KERNEL);
	if (!tmdev)
		return -ENOMEM;

	tmdev->dev = dev;
	tmdev->chip = of_device_get_match_data(&pdev->dev);
	if (!tmdev->chip)
		return -EINVAL;

	platform_set_drvdata(pdev, tmdev);

	ret = sunxi_ths_resource_init(tmdev);
	if (ret)
		return ret;

	ret = tmdev->chip->init(tmdev);
	if (ret)
		return ret;

	ret = sunxi_ths_register(tmdev);
	if (ret)
		return ret;

	return ret;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	struct ths_device *tmdev = platform_get_drvdata(pdev);

	clk_disable_unprepare(tmdev->bus_clk);

	return 0;
}

static int __maybe_unused sunxi_thermal_suspend(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

	clk_disable_unprepare(tmdev->bus_clk);

	return 0;
}

static int __maybe_unused sunxi_thermal_resume(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

	clk_prepare_enable(tmdev->bus_clk);
	sunxi_ths_calibrate(tmdev);
	tmdev->chip->init(tmdev);

	return 0;
}

static const struct ths_thermal_chip sun50iw9p1_ths = {
	.sensor_num = 4,
	.has_bus_clk = true,
	.offset = -3255,
	.scale = -81,
	.ft_deviation = 8000,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.avcc_calibrate = sun50i_h616_ths_avcc_calibrate,
	.calibrate = sun50i_h616_ths_calibrate_wrap,
	.init = sun50i_h616_thermal_init,
};

static const struct of_device_id of_ths_match[] = {
	{ .compatible = "arm,sun50iw9p1", .data = &sun50iw9p1_ths },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, of_ths_match);

static SIMPLE_DEV_PM_OPS(sunxi_thermal_pm_ops,
			 sunxi_thermal_suspend, sunxi_thermal_resume);

static struct platform_driver ths_driver = {
	.probe = sunxi_ths_probe,
	.remove = sunxi_ths_remove,
	.driver = {
		.name = "sunxi-thermal",
		.pm = &sunxi_thermal_pm_ops,
		.of_match_table = of_ths_match,
	},
};
module_platform_driver(ths_driver);

MODULE_DESCRIPTION("Thermal sensor driver for Allwinner SOC");
MODULE_LICENSE("GPL v2");
