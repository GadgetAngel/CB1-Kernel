/****************************************************************************
 * nand_osal_for_linux.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ****************************************************************************/

#include "nand_osal_for_linux.h"
#include "nand_panic.h"

#define NAND_DRV_VERSION_0 0x03
#define NAND_DRV_VERSION_1 0x7001
#define NAND_DRV_DATE 0x20201210
#define NAND_DRV_TIME 0x18231431

/**
 * nand common1 version rule vx.ab data time
 * x >= 1; 00 <= ab <= 99
 */
#define NAND_COMMON1_PHY_DRV_VERSION "v1.06 2021-02-22 16:18"

#define GPIO_BASE_ADDR 0x0300B000

#define NAND_STORAGE_TYPE_NULL 0
#define NAND_STORAGE_TYPE_RAWNAND 1
#define NAND_STORAGE_TYPE_SPINAND 2

int nand_type;

int nand_snprint(char *str, unsigned int size, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = snprintf(str, size, "%pV", &vaf);

	va_end(args);

	return rtn;
}

int nand_print(const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = printk(KERN_ERR "%pV", &vaf);

	va_end(args);

	return rtn;
}

int nand_print_dbg(const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = printk(KERN_DEBUG "%pV", &vaf);

	va_end(args);

	return rtn;
}

int nand_clk_request(struct sunxi_ndfc *ndfc, __u32 nand_index)
{
	long rate;

	if (ndfc == NULL) {
		nand_print("%s err: ndfc is null\n", __func__);
		return -1;
	}

	nand_print_dbg("nand_clk_request\n");

	ndfc->pclk = of_clk_get(ndfc->dev->of_node, 0);
	if ((ndfc->pclk == NULL) || IS_ERR(ndfc->pclk)) {
		nand_print("%s: pll clock handle invalid!\n", __func__);
		return -1;
	}

	rate = clk_get_rate(ndfc->pclk);
	nand_print_dbg("%s: get pll rate %dHZ\n", __func__, (__u32)rate);

	if (nand_index == 0) {
		ndfc->mdclk = of_clk_get(ndfc->dev->of_node, 1);

		if ((ndfc->mdclk == NULL) || IS_ERR(ndfc->mdclk)) {
			nand_print("%s: nand0 clock handle invalid!\n",
				   __func__);
			return -1;
		}

		if (clk_set_parent(ndfc->mdclk, ndfc->pclk))
			nand_print("%s:set nand0_dclk parent to pll failed\n",
				   __func__);

		rate = clk_round_rate(ndfc->mdclk, 20000000);
		if (clk_set_rate(ndfc->mdclk, rate))
			nand_print("%s: set nand0_dclk rate to %dHZ failed!\n",
				   __func__, (__u32)rate);

		if (clk_prepare_enable(ndfc->mdclk))
			nand_print("%s: enable nand0_dclk failed!\n",
				   __func__);

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			ndfc->mcclk = of_clk_get(ndfc->dev->of_node, 2);

			if ((ndfc->mcclk == NULL) || IS_ERR(ndfc->mcclk)) {
				nand_print("%s: nand0 cclock handle invalid!\n", __func__);
				return -1;
			}

			if (clk_set_parent(ndfc->mcclk, ndfc->pclk))
				nand_print("%s:set nand0_cclk parent to pll failed\n", __func__);

			rate = clk_round_rate(ndfc->mcclk, 20000000);
			if (clk_set_rate(ndfc->mcclk, rate))
				nand_print("%s: set nand0_cclk rate to %dHZ failed!\n", __func__, (__u32)rate);

			if (clk_prepare_enable(ndfc->mcclk))
				nand_print("%s: enable nand0_cclk failed!\n", __func__);

		}
	} else {
		nand_print("nand_clk_request, nand_index error: 0x%x\n",
			   nand_index);
		return -1;
	}

	return 0;
}

void nand_clk_release(struct sunxi_ndfc *ndfc, __u32 nand_index)
{
	if (nand_index == 0) {
		if (ndfc->mdclk && !IS_ERR(ndfc->mdclk)) {
			clk_disable_unprepare(ndfc->mdclk);

			clk_put(ndfc->mdclk);
			ndfc->mdclk = NULL;
		}

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if (ndfc->mcclk && !IS_ERR(ndfc->mcclk)) {
				clk_disable_unprepare(ndfc->mcclk);

				clk_put(ndfc->mcclk);
				ndfc->mcclk = NULL;
			}
		}
	} else
		nand_print("nand_clk_request, nand_index error: 0x%x\n", nand_index);

	if (ndfc->pclk && !IS_ERR(ndfc->pclk)) {
		clk_put(ndfc->pclk);
		ndfc->pclk = NULL;
	}
}

int nand_set_clk(struct sunxi_ndfc *ndfc, __u32 nand_index, __u32 nand_clk0,
		__u32 nand_clk1)
{
	long rate = 0;

	if (nand_index == 0) {
		if ((ndfc->mdclk == NULL) || IS_ERR(ndfc->mdclk)) {
			nand_print("%s: clock handle invalid!\n", __func__);
			return -1;
		}
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			rate = clk_round_rate(ndfc->mdclk, nand_clk0 * 2000000);
		} else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND) {
			rate = clk_round_rate(ndfc->mdclk, nand_clk0 * 1000000);
		}

		if (clk_set_rate(ndfc->mdclk, rate))
			nand_print("%s: set nand0_dclk to %dHZ failed! nand_clk: 0x%x\n",
				   __func__, (__u32)rate, nand_clk0);

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if ((ndfc->mcclk == NULL) || IS_ERR(ndfc->mcclk)) {
				nand_print("%s: clock handle invalid!\n", __func__);
				return -1;
			}

			rate = clk_round_rate(ndfc->mcclk, nand_clk1 * 1000000);
			if (clk_set_rate(ndfc->mcclk, rate))
				nand_print("%s: set nand0_cclk to %dHZ failed! nand_clk: 0x%x\n",
					   __func__, (__u32)rate, nand_clk1);
		}
	} else {
		nand_print("nand_set_clk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	return 0;
}

int nand_get_clk(struct sunxi_ndfc *ndfc, __u32 nand_index, __u32 *pnand_clk0,
		__u32 *pnand_clk1)
{
	long rate = 0;

	if (nand_index == 0) {
		if ((ndfc->mdclk == NULL) || IS_ERR(ndfc->mdclk)) {
			nand_print("%s: clock handle invalid!\n", __func__);
			return -1;
		}
		rate = clk_get_rate(ndfc->mdclk);
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
			*pnand_clk0 = (rate / 2000000);
		else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
			*pnand_clk0 = (rate / 1000000);

		rate = 0;
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if ((ndfc->mcclk == NULL) || IS_ERR(ndfc->mcclk)) {
				nand_print("%s: clock handle invalid!\n", __func__);
				return -1;
			}
			rate = clk_get_rate(ndfc->mcclk);
		}
		*pnand_clk1 = (rate / 1000000);
	} else {
		nand_print("nand_get_clk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	return 0;
}

void eLIBs_CleanFlushDCacheRegion_nand(void *adr, size_t bytes)
{
	/*  __flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);*/
}

__s32 nand_clean_flush_dcache_region(void *buff_addr, __u32 len)
{
	eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t)len);
	return 0;
}

__s32 nand_invaild_dcache_region(__u32 rw, __u32 buff_addr, __u32 len)
{
	return 0;
}

void *nand_dma_map_single(struct sunxi_ndfc *ndfc, __u32 rw, void *buff_addr, __u32 len)
{
	void *mem_addr;

	if (ndfc == NULL)
		nand_print("%s err: ndfc is null\n", __func__);
	if (is_on_panic())
		return (void *)nand_panic_dma_map(rw, buff_addr, len);
	if (rw == 1) {
		mem_addr = (void *)dma_map_single(ndfc->dev, (void *)buff_addr,
						  len, DMA_TO_DEVICE);
	} else {
		mem_addr = (void *)dma_map_single(ndfc->dev, (void *)buff_addr,
						  len, DMA_BIDIRECTIONAL);
	}
	if (dma_mapping_error(ndfc->dev, (dma_addr_t)mem_addr))
		nand_print("dma mapping error\n");

	return mem_addr;
}

void *nand_dma_unmap_single(struct sunxi_ndfc *ndfc, __u32 rw, void *buff_addr, __u32 len)
{
	void *mem_addr = buff_addr;

	if (ndfc == NULL) {
		nand_print("%s ndfc is null\n", __func__);
		return NULL;
	}

	if (is_on_panic()) {
		nand_panic_dma_unmap(rw, (dma_addr_t)buff_addr, len);
		return mem_addr;
	}

	if (rw == 1) {
		dma_unmap_single(ndfc->dev, (dma_addr_t)mem_addr, len,
				 DMA_TO_DEVICE);
	} else {
		dma_unmap_single(ndfc->dev, (dma_addr_t)mem_addr, len,
				 DMA_BIDIRECTIONAL);
	}

	return mem_addr;
}

void *nand_va_to_pa(void *buff_addr)
{
	return (void *)(__pa((void *)buff_addr));
}

__s32 nand_pio_request(struct sunxi_ndfc *ndfc, __u32 nand_index)
{
	struct pinctrl *pinctrl = NULL;

	pinctrl = pinctrl_get_select(ndfc->dev, "default");
	if (!pinctrl || IS_ERR(pinctrl)) {
		nand_print("nand_pio_request: set nand0 pin error!\n");
		return -1;
	}

	return 0;
}

void nand_pio_release(struct sunxi_ndfc *ndfc, __u32 nand_index)
{

	struct pinctrl *pinctrl = NULL;

	pinctrl = pinctrl_get_select(ndfc->dev, "sleep");
	if (!pinctrl || IS_ERR(pinctrl))
		nand_print("nand_pio_release: set nand0 pin error!\n");
}

void nand_memset(void *pAddr, unsigned char value, unsigned int len)
{
	memset(pAddr, value, len);
}

int nand_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int nand_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

void nand_memcpy(void *pAddr_dst, void *pAddr_src, unsigned int len)
{
	memcpy(pAddr_dst, pAddr_src, len);
}

void *nand_malloc(unsigned int size)
{
	return kmalloc(size, GFP_KERNEL);
}

void nand_free(void *addr)
{
	kfree(addr);
}


/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
DEFINE_SEMAPHORE(nand_physic_mutex);

int nand_physic_lock_init(void)
{
	return 0;
}

int nand_physic_lock(void)
{
	if (is_on_panic())
		return 0;
	down(&nand_physic_mutex);
	return 0;
}

int nand_physic_unlock(void)
{
	if (is_on_panic())
		return 0;
	up(&nand_physic_mutex);
	return 0;
}

int nand_physic_lock_exit(void)
{
	return 0;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
/* request dma channel and set callback function */
#ifdef AWSPINAND

void *dma_map_addr;
struct dma_chan *dma_hdl_tx;
struct dma_chan *dma_hdl_rx;

int spinand_request_tx_dma(void)
{
	dma_cap_mask_t mask;

	nand_print_dbg("request tx DMA\n");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl_tx == NULL) {
		dma_hdl_tx = dma_request_channel(mask, NULL, NULL);
		if (dma_hdl_tx == NULL) {
			nand_print_dbg("Request tx DMA failed!\n");
			return -EINVAL;
		}
	}
	return 0;
}

int spinand_request_rx_dma(void)
{
	dma_cap_mask_t mask;

	nand_print_dbg("request rx DMA\n");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl_rx == NULL) {
		dma_hdl_rx = dma_request_channel(mask, NULL, NULL);
		if (dma_hdl_rx == NULL) {
			nand_print_dbg("Request rx DMA failed!\n");
			return -EINVAL;
		}
	}
	return 0;
}

int spinand_releasetxdma(void)
{
	if (dma_hdl_tx != NULL) {
		printk("spinand release tx dma\n");
		dma_release_channel(dma_hdl_tx);
		dma_hdl_tx = NULL;
		return 0;
	}

	return 0;
}

int spinand_releaserxdma(void)
{
	if (dma_hdl_rx != NULL) {
		nand_print_dbg("spinand release rx dma\n");
		dma_release_channel(dma_hdl_rx);
		dma_hdl_rx = NULL;
		return 0;
	}

	return 0;
}
void prepare_spinand_dma_callback(void)
{
	/*init_completion(&spinand_dma_done);*/
}

void spinand_dma_callback(void *arg)
{
	/*complete(&spinand_dma_done);*/
}

int tx_dma_config_start(dma_addr_t addr, __u32 length)
{
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_MEM_TO_DEV;
	dma_conf.dst_addr = SPI_BASE_ADDR + SPI_TX_DATA_REG;

	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 8;
	dma_conf.dst_maxburst = 8;

	dma_conf.slave_id = sunxi_slave_id(DRQDST_SPI0_TX, DRQSRC_SDRAM);

	dmaengine_slave_config(dma_hdl_tx, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl_tx, addr, length,
					       DMA_TO_DEVICE, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!dma_desc) {
		nand_print_dbg("tx dmaengine prepare failed!\n");
		return -1;
	}

	prepare_spinand_dma_callback();
	dma_desc->callback = spinand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl_tx);

	return 0;
}
int rx_dma_config_start(dma_addr_t addr, __u32 length)
{
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = SPI_BASE_ADDR + SPI_RX_DATA_REG;

	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 8;
	dma_conf.dst_maxburst = 8;
	dma_conf.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SPI0_RX);

	dmaengine_slave_config(dma_hdl_rx, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl_rx, addr, length,
					       DMA_FROM_DEVICE, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!dma_desc) {
		nand_print_dbg("rx dmaengine prepare failed!\n");
		return -1;
	}

	prepare_spinand_dma_callback();
	dma_desc->callback = spinand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl_rx);

	return 0;
}

int spinand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
	int ret;
	dma_map_addr = nand_dma_map_single(&aw_ndfc, rw, (void *)addr, length);

	if (rw)
		ret = tx_dma_config_start((dma_addr_t)dma_map_addr, length);
	else
		ret = rx_dma_config_start((dma_addr_t)dma_map_addr, length);

	if (ret != 0) /* fail */
		nand_dma_unmap_single(&aw_ndfc, rw, dma_map_addr, length);
	return ret;
}

int nand_dma_end(__u32 rw, __u32 addr, __u32 length)
{

	nand_dma_unmap_single(&aw_ndfc, rw, dma_map_addr, length);

	return 0;
}

int spinand_dma_terminate(__u32 rw)
{
	int ret;
	if (rw)
		ret = dmaengine_terminate_sync(dma_hdl_tx);
	else
		ret = dmaengine_terminate_sync(dma_hdl_rx);
	return ret;
}
#endif
int rawnand_dma_terminate(__u32 rw)
{
	return 0;
}

int nand_dma_terminate(__u32 rw)
{
	int ret = 0;
	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
		ret = rawnand_dma_terminate(rw);
	else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
		;
		/*ret = spinand_dma_terminate(rw);*/
	return ret;
}
int nand_wait_dma_finish(__u32 tx_flag, __u32 rx_flag)
{
#if 0
	__u32 timeout = 2000;
	__u32 timeout_flag = 0;
	if (is_on_panic())
		return 0;
	if (tx_flag && rx_flag)
		nand_print("only one completion, not support wait both tx and rx!!\n");
	if (tx_flag || rx_flag) {
		timeout_flag = wait_for_completion_timeout(&spinand_dma_done, msecs_to_jiffies(timeout));
		if (!timeout_flag) {
			nand_print_dbg("wait dma finish timeout!! tx_flag:%d rx_flag:%d\n", tx_flag, rx_flag);
			if (rx_flag)
				nand_dma_terminate(0);
			if (tx_flag)
				nand_dma_terminate(1);
			return -1;
		}
	}
#endif
	return 0;
}


int rawnand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
#if 0
/*no use extern  DMA*/
	struct dma_slave_config dma_conf = { 0 };
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = 0x01c03300;
	dma_conf.dst_addr = 0x01c03300;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id =
	    rw ? sunxi_slave_id(DRQDST_NAND0,
				DRQSRC_SDRAM) : sunxi_slave_id(DRQDST_SDRAM,
							       DRQSRC_NAND0);
	dmaengine_slave_config(dma_hdl, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl, addr, length,
					       (rw ? DMA_TO_DEVICE :
						DMA_FROM_DEVICE),
					       DMA_PREP_INTERRUPT |
					       DMA_CTRL_ACK);
	if (!dma_desc) {
		nand_print("dmaengine prepare failed!\n");
		return -1;
	}

	dma_desc->callback = (void *)nand_dma_callback;
	if (rw == 0)
		dma_desc->callback_param = NULL;
	else
		dma_desc->callback_param = (void *)(dma_desc);

	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl);
#endif
	return 0;
}

int nand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
	int ret = 0;

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
		ret = rawnand_dma_config_start(rw, addr, length);
	else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
		;
		/*ret = spinand_dma_config_start(rw, addr, length);*/
	return ret;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 nand_get_ndfc_dma_mode(void)
{
	return 1;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 nand_get_nand_extern_para(struct sunxi_ndfc *ndfc, __u32 para_num)
{
	int ret = 0;
	int nand_para = 0xffffffff;

	if (para_num == 0) { /*frequency */
		ret = of_property_read_u32(ndfc->dev->of_node, "nand0_p0",
					   &nand_para);
		if (ret) {
			nand_print("Failed to get nand_p0\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			nand_print_dbg("nand_p0 is no used\n");
			nand_para = 0xffffffff;
		} else
			nand_print_dbg("nand: get nand_p0 %x\n", nand_para);
	} else if (para_num == 1) { /*SUPPORT_TWO_PLANE */
		ret = of_property_read_u32(ndfc->dev->of_node, "nand0_p1",
					   &nand_para);
		if (ret) {
			nand_print("Failed to get nand_p1\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			nand_print_dbg("nand_p1 is no used\n");
			nand_para = 0xffffffff;
		} else
			nand_print_dbg("nand : get nand_p1 %x\n", nand_para);
	} else if (para_num == 2) { /*SUPPORT_VERTICAL_INTERLEAVE */
		ret = of_property_read_u32(ndfc->dev->of_node, "nand0_p2",
					   &nand_para);
		if (ret) {
			nand_print("Failed to get nand_p2\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			nand_print_dbg("nand_p2 is no used\n");
			nand_para = 0xffffffff;
		} else
			nand_print_dbg("nand : get nand_p2 %x\n", nand_para);
	} else if (para_num == 3) { /*SUPPORT_DUAL_CHANNEL */
		ret = of_property_read_u32(ndfc->dev->of_node, "nand0_p3",
					   &nand_para);
		if (ret) {
			nand_print("Failed to get nand_p3\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			nand_print_dbg("nand_p3 is no used\n");
			nand_para = 0xffffffff;
		} else
			nand_print_dbg("nand: get nand_p3 %x\n", nand_para);
	} else {
		nand_print("NAND_GetNandExtPara: wrong para num: %d\n",
			   para_num);
		return 0xffffffff;
	}
	return nand_para;
}

__u32 nand_get_nand_id_number_ctrl(struct sunxi_ndfc *ndfc)
{
	int ret;
	int id_number_ctl = 0;

	ret = of_property_read_u32(ndfc->dev->of_node, "nand0_id_number_ctl",
				   &id_number_ctl);
	if (ret) {
		nand_print_dbg("Failed to get id_number_ctl\n");
		id_number_ctl = 0;
	} else {
		if (id_number_ctl == 0x55aaaa55) {
			nand_print_dbg("id_number_ctl is no used\n");
			id_number_ctl = 0;
		} else
			nand_print_dbg("nand : get id_number_ctl %x\n",
				       id_number_ctl);
	}
	return id_number_ctl;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 nand_get_max_channel_cnt(void)
{
	return 1;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_request_dma(struct sunxi_ndfc *ndfc)
{
	dma_cap_mask_t mask;

	nand_print_dbg("request DMA");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (ndfc->dma_hdl == NULL) {
		ndfc->dma_hdl = dma_request_channel(mask, NULL, NULL);
		if (ndfc->dma_hdl == NULL) {
			nand_print("Request DMA failed!\n");
			return -EINVAL;
		}
	}
	nand_print_dbg("chan_id: %d", ndfc->dma_hdl->chan_id);

	return 0;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_release_dma(struct sunxi_ndfc *ndfc, __u32 nand_index)
{
	if (ndfc->dma_hdl != NULL) {
		nand_print_dbg("nand release dma\n");
		dma_release_channel(ndfc->dma_hdl);
		ndfc->dma_hdl = NULL;
		return 0;
	}

	return 0;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 nand_get_ndfc_version(void)
{
	return 1;
}
#if 0

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void *RAWNAND_GetIOBaseAddrCH0(void)
{
//	return NDFC0_BASE_ADDR;
}

void *RAWNAND_GetIOBaseAddrCH1(void)
{
//	return NDFC1_BASE_ADDR;
}
void *SPINAND_GetIOBaseAddrCH0(void)
{
//	return SPIC0_IO_BASE;
}

void *SPINAND_GetIOBaseAddrCH1(void)
{
	return NULL;
}
#endif

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : wait rb
*****************************************************************************/
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH1);

__s32 nand_rb_wait_time_out(__u32 no, __u32 *flag)
{
	__s32 ret;

	if (no == 0)
		ret = wait_event_timeout(NAND_RB_WAIT_CH0, *flag, HZ >> 1);
	else
		ret = wait_event_timeout(NAND_RB_WAIT_CH1, *flag, HZ >> 1);

	return ret;
}

__s32 nand_rb_wake_up(__u32 no)
{
	if (no == 0)
		wake_up(&NAND_RB_WAIT_CH0);
	else
		wake_up(&NAND_RB_WAIT_CH1);

	return 0;
}

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : wait dma
*****************************************************************************/
static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH1);

__s32 nand_dma_wait_time_out(__u32 no, __u32 *flag)
{
	__s32 ret;

	if (no == 0)
		ret = wait_event_timeout(NAND_DMA_WAIT_CH0, *flag, HZ >> 1);
	else
		ret = wait_event_timeout(NAND_DMA_WAIT_CH1, *flag, HZ >> 1);

	return ret;
}

__s32 nand_dma_wake_up(__u32 no)
{
	if (no == 0)
		wake_up(&NAND_DMA_WAIT_CH0);
	else
		wake_up(&NAND_DMA_WAIT_CH1);

	return 0;
}

__u32 nand_dma_callback(void *para)
{
	return 0;
}

int nand_get_voltage(struct sunxi_ndfc *ndfc)
{

	int ret = 0;
	const char *sti_vcc_nand = NULL;
	const char *sti_vcc_io = NULL;

	ret = of_property_read_string(ndfc->dev->of_node, "nand0_regulator1",
				      &sti_vcc_nand);
	nand_print_dbg("nand0_regulator1 %s\n", sti_vcc_nand);
	if (ret)
		nand_print_dbg("Failed to get vcc_nand\n");

	ndfc->regu1 = regulator_get(NULL, sti_vcc_nand);
	if (IS_ERR(ndfc->regu1))
		nand_print_dbg("nand:fail to get regulator vcc-nand!\n");
	else {
		/*enable regulator */
		ret = regulator_enable(ndfc->regu1);
		if (IS_ERR(ndfc->regu1)) {
			nand_print_dbg("nand:fail to enable regulator vcc-nand!\n");
			return -1;
		}
		nand_print_dbg("nand:get voltage vcc-nand ok:%p\n", ndfc->regu1);
	}

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		ret = of_property_read_string(ndfc->dev->of_node,
					      "nand0_regulator2", &sti_vcc_io);
		nand_print_dbg("nand0_regulator2 %s\n", sti_vcc_io);
		if (ret)
			nand_print_dbg("Failed to get vcc_io\n");

		ndfc->regu2 = regulator_get(NULL, sti_vcc_io);
		if (IS_ERR(ndfc->regu2))
			nand_print_dbg("nand:fail to get regulator vcc-io!\n");
		else {
			/*enable regulator */
			ret = regulator_enable(ndfc->regu2);
			if (IS_ERR(ndfc->regu2)) {
				nand_print("fail to enable regulator vcc-io!\n");
				return -1;
			}
			nand_print_dbg("nand:get voltage vcc-io ok:%p\n", ndfc->regu2);
		}
	}

	nand_print_dbg("nand:has already get voltage\n");

	return ret;
}

int nand_release_voltage(struct sunxi_ndfc *ndfc)
{
	int ret = 0;

	if (!IS_ERR(ndfc->regu1)) {
		nand_print_dbg("nand release voltage vcc-nand\n");
		ret = regulator_disable(ndfc->regu1);
		if (ret)
			nand_print_dbg("nand: regu1 disable fail, ret 0x%x\n", ret);
		if (IS_ERR(ndfc->regu1))
			nand_print_dbg("nand: fail to disable regulator vcc-nand!");

		/*put regulator when module exit */
		regulator_put(ndfc->regu1);

		ndfc->regu1 = NULL;
	}

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		if (!IS_ERR(ndfc->regu2)) {
			nand_print_dbg("nand release voltage vcc-io\n");
			ret = regulator_disable(ndfc->regu2);
			if (ret)
				nand_print_dbg("nand: regu2 disable fail,ret 0x%x\n", ret);
			if (IS_ERR(ndfc->regu2))
				nand_print_dbg("nand: fail to disable regulator vcc-io!\n");

			/*put regulator when module exit */
			regulator_put(ndfc->regu2);

			ndfc->regu2 = NULL;
		}
	}

	nand_print_dbg("nand had already release voltage\n");

	return ret;
}

int nand_is_secure_sys(void)
{
	if (sunxi_soc_is_secure()) {
		nand_print_dbg("secure system\n");
		return 1;
	}
	nand_print_dbg("non secure\n");

	return 0;
}

__u32 nand_print_level(struct sunxi_ndfc *ndfc)
{
	int ret;
	int print_level = 0xffffffff;

	ret = of_property_read_u32(ndfc->dev->of_node, "nand0_print_level",
				   &print_level);
	if (ret) {
		nand_print_dbg("Failed to get print_level\n");
		print_level = 0xffffffff;
	} else {
		if (print_level == 0x55aaaa55) {
			nand_print_dbg("print_level is no used\n");
			print_level = 0xffffffff;
		} else
			nand_print_dbg("nand : get print_level %x\n", print_level);
	}

	return print_level;
}

int nand_get_dragon_board_flag(struct sunxi_ndfc *ndfc)
{
	int ret;
	int dragonboard_flag = 0;

	ret = of_property_read_u32(ndfc->dev->of_node, "nand0_dragonboard",
				   &dragonboard_flag);
	if (ret) {
		nand_print_dbg("Failed to get dragonboard_flag\n");
		dragonboard_flag = 0;
	} else {
		nand_print_dbg("nand: dragonboard_flag %x\n", dragonboard_flag);
	}

	return dragonboard_flag;
}

void nand_print_version(void)
{
	int val[4] = {0};

	val[0] = NAND_DRV_VERSION_0;
	val[1] = NAND_DRV_VERSION_1;
	val[2] = NAND_DRV_DATE;
	val[3] = NAND_DRV_TIME;

	nand_print("kernel: nand version: %x %x %x %x\n", val[0],
		   val[1], val[2], val[3]);
}

int nand_get_drv_version(int *ver_main, int *ver_sub, int *date, int *time)
{
	*ver_main = NAND_DRV_VERSION_0;
	*ver_sub = NAND_DRV_VERSION_1;
	*date = NAND_DRV_DATE;
	*time = NAND_DRV_TIME;
	return 0;
}

void nand_cond_resched(void)
{
	cond_resched();
}

__u32 get_storage_type(void)
{

	return nand_type;
}
/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/

int nand_get_bsp_code(char *chip_code)
{
	int ret;
	const char *pchip_code = NULL;
	struct device_node *np = NULL;

	np = of_find_node_by_type(NULL, "nand0");
	if (np == NULL) {
		nand_print("ERROR! get nand0 node failed!\n");
		return -1;
	}

	ret = of_property_read_string(np, "chip_code", &pchip_code);
	if (ret) {
		nand_print("ERROR! get chip_code failed\n");
		return -1;
	}

	memcpy(chip_code, pchip_code, strlen(pchip_code));

	return 0;
}

/**
 * nand_fdt_get_node_by_byte:
 *
 * @type: device node type
 *
 * if device node exist return device node, otherwise return NULL
 */
static inline struct device_node *nand_fdt_get_node_by_type(const char *type)
{
	static char *s_type = "none";
	static struct device_node *np;

	if (memcmp((char *)s_type, (char *)type, strlen(type))) {
		np = of_find_node_by_type(NULL, type);
		if (np == NULL) {
			/*nand_print("get %s err\n", type);*/
			return NULL;
		}
		s_type = (char *)type;
	}

	return np;

}
/**
 * nand_fdt_get_node_by_path:
 *
 * @path: device node path the path should be absolute path.
 *		eg. /soc@0xxxx/nand0@0xxxx
 *
 * if device node exist return device node, otherwise return NULL
 */
static inline struct device_node *nand_fdt_get_node_by_path(const char *path)
{
	static char *s_path = "none";
	static struct device_node *np;

	if (strlen(path) != strlen(s_path) ||
			!!memcmp((char *)s_path, (char *)path, strlen(path))) {
		np = of_find_node_by_path(path);
		if (np == NULL) {
			/*nand_print("get %s err\n", path);*/
			return NULL;
		}
		s_path = (char *)path;
	}

	return np;
}
/**
 * nand_fdt_get_prop_string_by_path:
 *
 * @node: device node
 * @path: device node path the path should be absolute path.
 *		eg. /soc@0xxxx/nand0@0xxxx
 * @prop_name: device prop name
 *
 * if prop exist return it's value, otherwise erturn NULL
 */
const char *nand_fdt_get_prop_string_by_path(struct device_node *node, const char *path, const char *prop_name)
{

	const char *prop = NULL;
	struct device_node *np = NULL;

	if (!node) {
		np = nand_fdt_get_node_by_path(path);
		if (!np)
			return NULL;
	} else {
		np = node;
	}

	of_property_read_string(np, prop_name, &prop);
	if (prop == NULL) {
		return NULL;
	}

	return prop;

}
/**
 * nand_fdt_get_prop_string_by_type:
 *
 * @node: device node
 * @type: device node type
 * @prop_name: device prop name
 *
 * if prop exist return it's value, otherwise erturn NULL
 */
const char *nand_fdt_get_prop_string_by_type(struct device_node *node, const char *type, const char *prop_name)
{
	const char *prop = NULL;
	struct device_node *np = NULL;

	if (!node) {
		np = nand_fdt_get_node_by_type(type);
		if (!np)
			return NULL;
	} else {
		np = node;
	}


	of_property_read_string(np, prop_name, &prop);
	if (prop == NULL) {
		return NULL;
	}

	return prop;
}
/**
 * nand_fdt_get_node_addr_by_type:
 *
 * @node: device node
 * @type: device node type
 *
 * if prop exist return device addr, otherwise erturn errno.
 */
u32 nand_fdt_get_node_addr_by_type(struct device_node *node, const char *type)
{

	struct resource res;
	struct device_node *np = NULL;

	if (!node) {
		np = nand_fdt_get_node_by_type(type);
		if (!np)
			return -ENXIO;
	} else {
		np = node;
	}

	if (of_address_to_resource(np, 0, &res)) {
		nand_dbg_inf("get resource fail\n");
		return -ENXIO;
	}

	return res.start;

}
/**
 * nand_fdt_get_node_addr_by_path:
 *
 * @node: device node
 * @path: device node path
 *
 * if prop exist return device addr, otherwise erturn errno.
 */
u32 nand_fdt_get_node_addr_by_path(struct device_node *node, const char *path)
{

	struct resource res;
	struct device_node *np = NULL;

	if (!node) {
		np = nand_fdt_get_node_by_path(path);
		if (!np)
			return -ENXIO;
	} else {
		np = node;
	}

	if (of_address_to_resource(np, 0, &res)) {
		nand_dbg_inf("get resource fail\n");
		return -ENXIO;
	}

	return res.start;

}
/*
*Name         : nand_get_support_boot_check_crc
*Description  :
*Parameter    :
*Return       :
*Note         : disable return 0, otherwise return 1
**/

int nand_get_support_boot_check_crc(void)
{

	const char *boot_crc = NULL;

	boot_crc = nand_fdt_get_prop_string_by_type(aw_ndfc.dev->of_node, "nand0", "boot_crc");

	if (boot_crc == NULL)
		return 1; /*default enable*/

	if (!memcmp(boot_crc, "disabled", sizeof("disabled")) ||
		!memcmp(boot_crc, "disable", sizeof("disable")))
		return 0;

	return 1;
}

char *nand_get_cur_task_name(void)
{
	return current->comm;
}

pid_t nand_get_cur_task_pid(void)
{
	return task_pid_nr(current);
}

#define NAND_SIGNAL_VOLTAGE_330	0
#define NAND_SIGNAL_VOLTAGE_180	1

int nand_vccq_3p3v_enable(void)
{
	return sunxi_sel_pio_mode(aw_ndfc.p, NAND_SIGNAL_VOLTAGE_330);
}
int nand_vccq_1p8v_enable(void)
{
	return sunxi_sel_pio_mode(aw_ndfc.p, NAND_SIGNAL_VOLTAGE_180);
}

void nand_common1_show_version(void)
{
	static int flag;
	if (flag)
		return;

	printk(KERN_INFO "raw nand common1 phy version:%s\n", NAND_COMMON1_PHY_DRV_VERSION);

	flag = 1;
}
