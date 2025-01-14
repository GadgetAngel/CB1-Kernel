/**
 * SPDX-License-Identifier: GPL-2.0
 *              eNand
 *       Nand flash driver scan module
 *
 * Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
 *      All Rights Reserved
 *
 */

#include "rawnand_ops.h"
#include "../../nfd/nand_osal_for_linux.h"
#include "../nand_boot.h"
#include "rawnand_chip.h"
#include "controller/ndfc_ops.h"
#include "rawnand.h"
#include "rawnand_base.h"
#include "rawnand_cfg.h"
#include "rawnand_debug.h"
#include "rawnand_ids.h"
#include "rawnand_readretry.h"

struct nand_phy_write_lsb_cache nand_phy_w_cache[NAND_OPEN_BLOCK_CNT] = {
    {0},
    {0},
};

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_erase_block_start(struct _nand_physic_op_par *npo)
{
	int ret;
	unsigned int row_addr = 0, col_addr = 0;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct nand_controller_info *nctri = nci->nctri;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	//RAWNAND_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) || (npo->block >= nci->blk_cnt_per_chip)) {
		RAWNAND_ERR("fatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d \n", nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_10;
	}
	//wait nand ready before erase
	nand_read_chip_status_ready(nci);

	nand_enable_chip(nci);

	ndfc_clean_cmd_seq(cmd_seq);

	// cmd1: 0x60
	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd = CMD_ERASE_CMD1;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd_send = 1;


	if (nci->id[0] == 0xec &&
		nci->id[1] == 0xde &&
		nci->id[2] == 0x94 &&
		nci->id[3] == 0xc3 &&
		nci->id[4] == 0xa4 &&
		nci->id[5] == 0xca) {

		row_addr = get_row_addr_2(nci->page_offset_for_next_blk, npo->block, npo->page);
	} else {

		row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	}
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	// cmd2: 0xD0
	cmd_seq->nctri_cmd[1].cmd = CMD_ERASE_CMD2;
	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd_send = 1;
	cmd_seq->nctri_cmd[1].cmd_wait_rb = 0;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);

	nand_disable_chip(nci);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_erase_block(struct _nand_physic_op_par *npo)
{
	int ret;
	//unsigned char status;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);

	ret = generic_erase_block_start(npo);
	if (ret != 0) {
		RAWNAND_ERR("erase_block wrong1\n");
		return ret;
	}

	ret = nand_read_chip_status_ready(nci);
	if (ret != 0) {
		RAWNAND_ERR("erase_block wrong2\n");
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_page_start(struct _nand_physic_op_par *npo)
{
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	u32 def_spare[32];
	int ret;
	u32 ecc_block, sect_bitmap = 0;

	if (npo->sect_bitmap % 2)
		npo->sect_bitmap += 1;

	if (npo->sect_bitmap == 0 || npo->sect_bitmap > nci->sector_cnt_per_page)
		ecc_block = nci->sector_cnt_per_page / 2;
	else
		ecc_block = npo->sect_bitmap / 2;
	sect_bitmap = ((unsigned int)1 << (ecc_block - 1)) | (((unsigned int)1 << (ecc_block - 1)) - 1);

	//wait nand ready before read
	nand_read_chip_status_ready(nci);

	//set ecc mode & randomize
	ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);
	if (nci->randomizer) {
		ndfc_set_rand_seed(nci->nctri, npo->page);
		ndfc_enable_randomize(nci->nctri);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	//command
	set_default_batch_read_cmd_seq(cmd_seq);
	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;

    //address
	if (nci->id[0] == 0xec &&
		nci->id[1] == 0xde &&
		nci->id[2] == 0x94 &&
		nci->id[3] == 0xc3 &&
		nci->id[4] == 0xa4 &&
		nci->id[5] == 0xca) {

		row_addr = get_row_addr_2(nci->page_offset_for_next_blk, npo->block, npo->page);
	} else {

		row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	}

	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	if (npo->mdata != NULL) {
		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	} else {
		//don't swap main data with host memory
		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	}
	cmd_seq->nctri_cmd[0].cmd_direction = 0; //read
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	/*
	 *cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page << 9;
	 *cmd_seq->nctri_cmd[0].cmd_data_block_mask = full_bitmap;
	 */
	cmd_seq->nctri_cmd[0].cmd_mdata_len = ecc_block << 10;
	cmd_seq->nctri_cmd[0].cmd_data_block_mask = sect_bitmap;

	if ((npo->mdata == NULL) && (npo->sdata != NULL) && (npo->sect_bitmap == 0)) {
		if (nci->sector_cnt_per_page > 4) {
			cmd_seq->nctri_cmd[0].cmd_mdata_len = 2 << 9; //12 byte spare data in 1K
			cmd_seq->nctri_cmd[0].cmd_data_block_mask = 0x1;
		} else {
			cmd_seq->nctri_cmd[0].cmd_mdata_len = 4 << 9;
			cmd_seq->nctri_cmd[0].cmd_data_block_mask = 0x3;
		}
	}
	ndfc_set_user_data_len_cfg(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);
	memset(def_spare, 0x99, 128);
	ndfc_set_spare_data(nci->nctri, (u8 *)def_spare, nci->sdata_bytes_per_page);
	ret = batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("read page start, batch cmd io send error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%p slen:%d: !\n", npo->chip, npo->block, npo->page, npo->sect_bitmap, npo->mdata, npo->slen);

		nand_disable_chip(nci);

		return ret;
	}

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_page_end_not_retry(struct _nand_physic_op_par *npo)
{
	s32 ecc_sta = 0, ret = 0;
	uchar spare[64];
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("read page end, batch cmd io wait error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%p slen:%d: !\n", npo->chip, npo->block, npo->page, npo->sect_bitmap, npo->mdata, npo->slen);
		goto ERROR;
	}

	//check ecc
	ecc_sta = ndfc_check_ecc(nci->nctri, nci->sector_cnt_per_page >> nci->ecc_sector);
	//get spare data
	ndfc_get_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	if (npo->slen != 0) {
		memcpy(npo->sdata, spare, npo->slen);
	}
	//update ecc status and spare data
	//RAWNAND_DBG("npo: 0x%x %d 0x%x\n",npo, npo->slen, npo->sdata);
	ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, spare);

ERROR:
	//disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}
	nand_disable_chip(nci);

	return ret; //ecc status
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_page_end(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = df_read_page_end.read_page_end(npo);

	if (ret == ECC_LIMIT) {
		RAWNAND_DBG("read page ecc limit,chip=%d block=%d page=%d\n", npo->chip, npo->block, npo->page);
	} else if (ret != 0) {
		RAWNAND_ERR("ecc err!read page, read page end error %d,chip=%d block=%d page=%d\n", ret, npo->chip, npo->block, npo->page);
	} else {
		;
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct nand_controller_info *nctri = nci->nctri;

	//RAWNAND_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n", __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) || (npo->block >= nci->blk_cnt_per_chip) || (npo->page >= nci->page_cnt_per_blk)) {
		RAWNAND_ERR("fatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n", nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);
		return ERR_NO_11;
	}

	if ((npo->mdata == NULL) && (npo->sdata == NULL) && (npo->sect_bitmap)) {
		RAWNAND_ERR("fatal err -1, wrong input parameter, mdata: %p  sdata: %p  sect_bitmap: 0x%x\n", npo->mdata, npo->sdata, npo->sect_bitmap);
		return ERR_NO_12;
	}

	if ((npo->mdata == NULL) && (npo->sdata == NULL) && (npo->sect_bitmap == 0)) {
		//RAWNAND_DBG("warning -0, mdata: 0x%08x  sdata: 0x%08x  sect_bitmap: 0x%x\n",npo->mdata, npo->sdata, npo->sect_bitmap);
		return 0;
	}

	ret = generic_read_page_start(npo);
	if (ret) {
		RAWNAND_ERR("read page, read page start error %d\n", ret);
		return ret;
	}

	ret = generic_read_page_end(npo);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_write_page_start(struct _nand_physic_op_par *npo, int plane_no)
{
	uchar spare[64];
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	int dummy_byte;
	unsigned int ecc_block, sect_bitmap = 0;
	int ret;

	if (npo->mdata == NULL) {
		RAWNAND_ERR("write page start, input parameter error!\n");
		return ERR_NO_13;
	}

	if (npo->sect_bitmap == 0 || npo->sect_bitmap > nci->sector_cnt_per_page)
	ecc_block = nci->sector_cnt_per_page / 2;
	else
		ecc_block = npo->sect_bitmap / 2;
	sect_bitmap = ((unsigned int)1 << (ecc_block - 1)) | (((unsigned int)1 << (ecc_block - 1)) - 1);
	if ((plane_no == 0) || (plane_no == 1))
		//wait nand ready before write
		nand_read_chip_status_ready(nci);
#if 0
	if ((nci->driver_no == 1) && (hynix16nm_read_retry_mode == 0x4) && (nci->retry_count != 0)) {
		hynix16nm_set_default_param(nci);
		nci->retry_count = 0;
	}
#endif
	//    RAWNAND_DBG("chip:%d block:%d page: %d buf:0x%x%x spare:0x%x%x%x \n",npo->chip,npo->block,npo->page,npo->mdata[0],npo->mdata[1],npo->sdata[2],npo->sdata[3],npo->sdata[4]);

	//    if((nci->driver_no == 2)&&((0x2 == hynix20nm_read_retry_mode)||(0x3 == hynix20nm_read_retry_mode))&&(nci->retry_count != 0))
	//    {
	//        hynix20nm_set_default_param(nci);
	//        nci->retry_count = 0;
	//	}

	//get spare data
	memset(spare, 0xff, 64);
	if (npo->slen != 0) {
		memcpy(spare, npo->sdata, npo->slen);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	ndfc_set_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	//set ecc mode & randomize
	if (nci->randomizer) {
		ndfc_set_rand_seed(nci->nctri, npo->page);
		ndfc_enable_randomize(nci->nctri);
	}
	ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);

	nci->nctri->current_op_type = 1;
	dummy_byte = get_dummy_byte(nci->nand_real_page_size, nci->ecc_mode, nci->sector_cnt_per_page / 2, nci->sdata_bytes_per_page);
	if (dummy_byte > 0) {
		ndfc_set_dummy_byte(nci->nctri, dummy_byte);
		ndfc_enable_dummy_byte(nci->nctri);
	}

	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;
	if (nci->random_cmd2_send_flag) {
		nci->nctri->random_cmd2 = get_random_cmd2(npo);
	}

	//command
	if (plane_no == 0) {
		set_default_batch_write_cmd_seq(cmd_seq, CMD_WRITE_PAGE_CMD1, CMD_WRITE_PAGE_CMD2);
	} else if (plane_no == 1) {
		//set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,0x11);
		set_default_batch_write_cmd_seq(cmd_seq,
						nci->opt_phy_op_par->instr.multi_plane_write_instr[0],
						nci->opt_phy_op_par->instr.multi_plane_write_instr[1]);
		//set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,CMD_WRITE_PAGE_CMD2);
	} else {
		//set_default_batch_write_cmd_seq(cmd_seq,0x81,CMD_WRITE_PAGE_CMD2);
		set_default_batch_write_cmd_seq(cmd_seq,
						nci->opt_phy_op_par->instr.multi_plane_write_instr[2],
						nci->opt_phy_op_par->instr.multi_plane_write_instr[3]);
		//set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,CMD_WRITE_PAGE_CMD2);
	}

    //address
	if (nci->id[0] == 0xec &&
		nci->id[1] == 0xde &&
		nci->id[2] == 0x94 &&
		nci->id[3] == 0xc3 &&
		nci->id[4] == 0xa4 &&
		nci->id[5] == 0xca) {

		row_addr = get_row_addr_2(nci->page_offset_for_next_blk, npo->block, npo->page);
	} else {
		row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	}
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	cmd_seq->nctri_cmd[0].cmd_direction = 1; //write
	//   cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_data_block_mask = sect_bitmap;
	/*cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page << 9;*/
	cmd_seq->nctri_cmd[0].cmd_mdata_len = ecc_block << 10;

	ndfc_set_user_data_len_cfg(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	ret = batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("read2 page start, batch cmd io send error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%p slen:%d: !\n", npo->chip, npo->block, npo->page, npo->sect_bitmap, npo->mdata, npo->slen);
		nand_disable_chip(nci);
		return ret;
	}

	return 0;
}

int generic_write_page_end(struct _nand_physic_op_par *npo)
{
	s32 ret = 0;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("write page end, batch cmd io wait error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%p slen:%d: !\n", npo->chip, npo->block, npo->page, npo->sect_bitmap, npo->mdata, npo->slen);
	}

	//disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}

	ndfc_set_dummy_byte(nci->nctri, 0);
	ndfc_disable_dummy_byte(nci->nctri);

	nci->nctri->random_cmd2_send_flag = 0;
	nci->nctri->current_op_type = 0;

	nand_disable_chip(nci);
	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_write_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct nand_controller_info *nctri = nci->nctri;

	//RAWNAND_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n", __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) || (npo->block >= nci->blk_cnt_per_chip) || (npo->page >= nci->page_cnt_per_blk)) {
		RAWNAND_ERR("wfatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n", nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);
		return ERR_NO_14;
	}

	/*
	 *if (npo->sect_bitmap != nci->sector_cnt_per_page) {
	 *        RAWNAND_ERR("wFatal err -2, wrong input parameter, unaligned write page SectBitmap: 0x%x/0x%x\n", npo->sect_bitmap, nci->sector_cnt_per_page);
	 *        return ERR_NO_15;
	 *}
	 */

	ret = generic_write_page_start(npo, 0);
	if (ret) {
		RAWNAND_ERR("write page, write page start error %d\n", ret);
		return ret;
	}

	ret = generic_write_page_end(npo);
	if (ret) {
		RAWNAND_ERR("write page, write page end error %d\n", ret);
		return ret;
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_two_plane_page_start(struct _nand_physic_op_par *npo, struct _nand_physic_op_par *npo2)
{
	int ret;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;

	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd = nci->opt_phy_op_par->instr.multi_plane_read_instr[0]; //multi_plane_read_instr_cmd[0];
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	if (nci->id[0] == 0xec &&
		nci->id[1] == 0xde &&
		nci->id[2] == 0x94 &&
		nci->id[3] == 0xc3 &&
		nci->id[4] == 0xa4 &&
		nci->id[5] == 0xca) {

		row_addr = get_row_addr_2(nci->page_offset_for_next_blk, npo->block, npo->page);
	} else {
		row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	}
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd = nci->opt_phy_op_par->instr.multi_plane_read_instr[2]; //multi_plane_read_instr_cmd[0];
	cmd_seq->nctri_cmd[1].cmd_send = 1;

	if (nci->id[0] == 0xec &&
		nci->id[1] == 0xde &&
		nci->id[2] == 0x94 &&
		nci->id[3] == 0xc3 &&
		nci->id[4] == 0xa4 &&
		nci->id[5] == 0xca) {

		row_addr = get_row_addr_2(nci->page_offset_for_next_blk, npo->block, npo->page);
	} else {
		row_addr = get_row_addr(nci->page_offset_for_next_blk, npo2->block, npo2->page);
	}
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[1].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2, cmd_seq->nctri_cmd[1].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[1].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3, cmd_seq->nctri_cmd[1].cmd_addr);
	}

	cmd_seq->nctri_cmd[2].cmd_valid = 1;
	cmd_seq->nctri_cmd[2].cmd = nci->opt_phy_op_par->instr.multi_plane_read_instr[3];
	cmd_seq->nctri_cmd[2].cmd_send = 1;
	cmd_seq->nctri_cmd[2].cmd_wait_rb = 1;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("read_two_plane_page_start failed!\n");
	}

	nand_disable_chip(nci);

	return ret;
}

int generic_read_two_plane_page_end(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	//    int ecc_sta = 0;
	//    struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	//    struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	//    u32 row_addr = 0, col_addr = 0;
	//    u32 def_spare[32];
	//
	//    nand_enable_chip(nci);
	//
	//    //set ecc mode & randomize
	//    ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	//    ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);
	//    if (nci->randomizer)
	//    {
	//    	ndfc_set_rand_seed(nci->nctri, npo->page);
	//    	ndfc_enable_randomize(nci->nctri);
	//    }
	//
	//    ndfc_clean_cmd_seq(cmd_seq);
	//    cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	//
	//    cmd_seq->nctri_cmd[0].cmd_valid = 1;
	//    cmd_seq->nctri_cmd[0].cmd = 0x00;
	//    cmd_seq->nctri_cmd[0].cmd_send = 1;
	//    row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	//    cmd_seq->nctri_cmd[0].cmd_acnt = 5;
	//    fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	//    ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	//	if (ret)
	//    {
	//		RAWNAND_ERR("read_two_plane_page_start failed!\n");
	//		nand_disable_chip(nci);
	//		return ret;
	//	}
	//
	//    //command
	//    ndfc_clean_cmd_seq(cmd_seq);
	//
	//	cmd_seq->cmd_type = CMD_TYPE_BATCH;
	//	cmd_seq->ecc_layout = ECC_LAYOUT_INTERLEAVE;
	//
	//	cmd_seq->nctri_cmd[0].cmd = 0x05;
	//	cmd_seq->nctri_cmd[1].cmd = 0xe0;
	//	cmd_seq->nctri_cmd[2].cmd = 0x05;
	//	cmd_seq->nctri_cmd[3].cmd = 0xe0;
	//
	//    //address
	//    row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	//    cmd_seq->nctri_cmd[0].cmd_acnt = 2;
	//    fill_cmd_addr(col_addr, 2, row_addr, 0, cmd_seq->nctri_cmd[0].cmd_addr);
	//
	//    //data
	//	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	//	if (npo->mdata != NULL)
	//	{
	//		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	//		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	//	}
	//	else
	//    {
	//		//don't swap main data with host memory
	//		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
	//		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	//	}
	//	cmd_seq->nctri_cmd[0].cmd_direction = 0; //read
	//	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	//	cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page << 9;
	//
	//	memset(def_spare, 0x99, 128);
	//	ndfc_set_spare_data(nci->nctri, (u32*)def_spare, MAX_ECC_BLK_CNT);
	//	ret = rawnand_diff_func->cmd_ops.batch_cmd_io_send(nci->nctri, cmd_seq);
	//	if(ret)
	//	{
	//		RAWNAND_ERR("read3 page start, batch cmd io send error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",npo->chip,npo->block,npo->page,npo->sect_bitmap,npo->mdata,npo->slen);
	//		nand_disable_chip(nci);
	//		return ret;
	//	}
	//    ret = rawnand_diff_func->batch_cmd_io_wait(nci->nctri, cmd_seq);
	//	if (ret)
	//	{
	//		RAWNAND_ERR("read2 page end, batch cmd io wait error:chip:%d block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",npo->chip,npo->block,npo->page,npo->sect_bitmap,npo->mdata,npo->slen);
	//	}
	//
	//	//check ecc
	//	ecc_sta = rawnand_diff_func->ndfc_check_ecc(nci->nctri, nci->sector_cnt_per_page>>nci->ecc_sector);
	//	//get spare data
	//	ndfc_get_spare_data(nci->nctri, (u32*)def_spare, nci->sector_cnt_per_page>>nci->ecc_sector);
	//
	//	if (npo->slen != 0)
	//	{
	//		memcpy(npo->sdata, def_spare, npo->slen);
	//	}
	//    //update ecc status and spare data
	//    //RAWNAND_DBG("npo: 0x%x %d 0x%x\n",npo, npo->slen, npo->sdata);
	//    ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, def_spare);
	//
	//    //disable ecc mode & randomize
	//    ndfc_disable_ecc(nci->nctri);
	//    if (nci->randomizer)
	//    {
	//    	ndfc_disable_randomize(nci->nctri);
	//    }
	//
	//    nand_disable_chip(nci);

	return ret;
}

int generic_read_two_plane_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	int ret0 = 0, ret1 = 0;
	uchar spare[64];
	int i;
	int len_off = 0;
	struct _nand_physic_op_par npo1;
	struct _nand_physic_op_par npo2;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	//struct _nctri_cmd_seq* cmd_seq = &nci->nctri->nctri_cmd_seq;

	npo1.chip = npo->chip;
	npo1.block = (npo->block << 1);
	npo1.page = npo->page;
	/*npo1.sect_bitmap = nci->sector_cnt_per_page;*/
	npo1.sect_bitmap = min(nci->sector_cnt_per_page, npo->sect_bitmap);;
	npo1.mdata = npo->mdata;
	npo1.sdata = npo->sdata;
	npo1.slen = npo->slen;
	if ((npo1.slen == 0) || (npo1.sdata == NULL)) {
		npo1.sdata = spare;
		npo1.slen = nci->sdata_bytes_per_page;
	}

	npo2.chip = npo->chip;
	npo2.block = (npo->block << 1) + 1;
	npo2.page = npo->page;
	/*npo2.sect_bitmap = nci->sector_cnt_per_page;*/

	len_off = npo->sect_bitmap - nci->sector_cnt_per_page;

	npo2.sect_bitmap = (len_off > 0) ? len_off : 0;

	if (npo->mdata != NULL) {
		npo2.mdata = npo->mdata + (nci->sector_cnt_per_page << 9);
	} else {
		npo2.mdata = NULL;
	}
	npo2.sdata = spare;
	npo2.slen = npo->slen;
	if ((npo2.slen == 0) || (npo2.sdata == NULL)) {
		npo2.sdata = spare;
		npo2.slen = nci->sdata_bytes_per_page;
	}


	if ((npo->mdata == NULL) && (npo->sect_bitmap == 0)) {
		npo1.sect_bitmap = 0;
		npo2.sect_bitmap = 0;
		npo2.sdata = NULL;
		npo2.slen = 0;
	}

	if (nci->sector_cnt_per_page == 4) {
		npo1.sdata = spare;
		npo2.sdata = &spare[8];
		npo2.slen = nci->sdata_bytes_per_page;
	}

	//    ret |= generic_read_two_plane_page_start(&npo1,&npo2);
	//    ret |= generic_read_two_plane_page_end(&npo1);
	//    ret |= generic_read_two_plane_page_end(&npo2);

	ret0 = generic_read_page(&npo1);
	ret1 = generic_read_page(&npo2);

	if (nci->sector_cnt_per_page == 4) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				*((unsigned char *)npo->sdata + i) = spare[i];
			} else if (i == 15) {
				*((unsigned char *)npo->sdata + i) = 0xff;
			} else {
				*((unsigned char *)npo->sdata + i) = spare[i + 1];
			}
		}
	}

	if ((ret0 == ERR_ECC) || (ret1 == ERR_ECC))
		ret = ERR_ECC;
	else if ((ret0 == ECC_LIMIT) || (ret1 == ECC_LIMIT))
		ret = ECC_LIMIT;
	else
		ret = ret0 | ret1;

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_write_two_plane_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	uchar spare[64];
	int i;
	struct _nand_physic_op_par npo1;
	struct _nand_physic_op_par npo2;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	//struct _nctri_cmd_seq* cmd_seq = &nci->nctri->nctri_cmd_seq;

	npo1.chip = npo->chip;
	npo1.block = npo->block << 1;
	npo1.page = npo->page;
	npo1.sect_bitmap = nci->sector_cnt_per_page;
	npo1.mdata = npo->mdata;
	npo1.sdata = npo->sdata;
	npo1.slen = npo->slen;

	npo2.chip = npo->chip;
	npo2.block = (npo->block << 1) + 1;
	npo2.page = npo->page;
	npo2.sect_bitmap = nci->sector_cnt_per_page;
	npo2.mdata = npo->mdata + (nci->sector_cnt_per_page << 9);
	npo2.sdata = npo->sdata;
	npo2.slen = npo->slen;

	if (nci->sector_cnt_per_page == 4) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				spare[i] = *((unsigned char *)npo->sdata + i);
			} else if (i == 8) {
				spare[i] = 0xff;
			} else {
				spare[i] = *((unsigned char *)npo->sdata + i - 1);
			}
		}
		npo1.sdata = spare;
		npo2.sdata = &spare[8];
	}
	//    if(nci->driver_no == 2)
	//    {
	//        ret |= generic_write_page_start(&npo1,0);
	//        ret |= generic_write_page_end(&npo1);
	//
	//        ret |= generic_write_page_start(&npo2,0);
	//        ret |= generic_write_page_end(&npo2);
	//    }
	//    else
	{
		ret |= generic_write_page_start(&npo1, 1);
		ret |= generic_write_page_end(&npo1);

		ret |= generic_write_page_start(&npo2, 2);
		ret |= generic_write_page_end(&npo2);
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:good block  -1:bad block
 *Note         :
 *****************************************************************************/
int generic_bad_block_check(struct _nand_physic_op_par *npo)
{
	int num, start_page, i;
	unsigned char spare[64];
	struct _nand_physic_op_par lnpo;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct nand_controller_info *nctri = nci->nctri;

	//RAWNAND_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) || (npo->block >= nci->blk_cnt_per_chip)) {
		RAWNAND_ERR("cfatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d \n", nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_16;
	}

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.mdata = NULL;
	lnpo.sdata = spare;
	lnpo.sect_bitmap = npo->sect_bitmap;
	lnpo.slen = nci->sdata_bytes_per_page;
	if (nci->bad_block_flag_position == FIRST_PAGE) {
		//the bad block flag is in the first page, same as the logical information, just read 1st page is ok
		start_page = 0;
		num = 1;
	} else if (nci->bad_block_flag_position == FIRST_TWO_PAGES) {
		//the bad block flag is in the first page or the second page, need read the first page and the second page
		start_page = 0;
		num = 2;
	} else if (nci->bad_block_flag_position == LAST_PAGE) {
		//the bad block flag is in the last page, need read the first page and the last page
		start_page = nci->page_cnt_per_blk - 1;
		num = 1;
	} else if (nci->bad_block_flag_position == LAST_TWO_PAGES) {
		//the bad block flag is in the last 2 page, so, need read the first page, the last page and the last-1 page
		start_page = nci->page_cnt_per_blk - 2;
		num = 2;
	} else {
		RAWNAND_ERR("bad block check, unknown bad block flag position\n");
		return ERR_NO_17;
	}

	//read and check 1st page
	lnpo.page = 0;
	generic_read_page(&lnpo);
	if (lnpo.sdata[0] != 0xff) {
		RAWNAND_ERR("find a bad block: %d %d %d ", lnpo.chip, lnpo.block, lnpo.page);
		RAWNAND_ERR("sdata: %02x %02x %02x %02x \n", lnpo.sdata[0], lnpo.sdata[1], lnpo.sdata[2], lnpo.sdata[3]);
		return -1;
	}

	//read and check other pages
	for (i = 0, lnpo.page = start_page; i < num; i++) {
		generic_read_page(&lnpo);
		if (lnpo.sdata[0] != 0xff) {
			RAWNAND_ERR("find a bad block: %d %d %d ", lnpo.chip, lnpo.block, lnpo.page);
			RAWNAND_ERR("sdata: %02x %02x %02x %02x \n", lnpo.sdata[0], lnpo.sdata[1], lnpo.sdata[2], lnpo.sdata[3]);
			return -1;
		}
		lnpo.page++;
	}

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_bad_block_mark(struct _nand_physic_op_par *npo)
{
	int num, start_page, i, ret;
	unsigned char spare[64];
	struct _nand_physic_op_par lnpo;
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct nand_controller_info *nctri = nci->nctri;
	unsigned char *mbuf = nand_get_temp_buf(nctri->nci->sector_cnt_per_page << 9);
	if (mbuf == NULL) {
		RAWNAND_ERR("bad block mark no memory chip: %d block:%d\n", npo->chip, npo->block);
		return ERR_NO_18;
	}
	//RAWNAND_ERR("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) || (npo->block >= nci->blk_cnt_per_chip)) {
		RAWNAND_ERR("Mfatal err -0, wrong input parameter,channel:%d chip: %d/%d  block: %d/%d\n", nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_18;
	}

	RAWNAND_DBG("bad block mark chip: %d block:%d\n", npo->chip, npo->block);

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.page = 0;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.mdata = mbuf;
	lnpo.sdata = spare;
	lnpo.slen = nci->sdata_bytes_per_page;

	if (nci->bad_block_flag_position == FIRST_PAGE) {
		start_page = 0;
		num = 1;
	} else if (nci->bad_block_flag_position == FIRST_TWO_PAGES) {
		start_page = 0;
		num = 2;
	} else if (nci->bad_block_flag_position == LAST_PAGE) {
		start_page = nci->page_cnt_per_blk - 1;
		num = 1;
	} else if (nci->bad_block_flag_position == LAST_TWO_PAGES) {
		start_page = nci->page_cnt_per_blk - 2;
		num = 2;
	} else {
		nand_free_temp_buf(mbuf);
		RAWNAND_ERR("bad block mark, unknown bad block flag position\n");
		return ERR_NO_19;
	}

	ret = generic_erase_block(&lnpo);
	if (ret) {
		nand_free_temp_buf(mbuf);
		RAWNAND_ERR("bad block mark, erase block failed, blk %d, chip %d, ch %d\n", lnpo.block, lnpo.chip, nci->nctri->channel_id);
		return ret;
	}

	memset(lnpo.sdata, 0, 64);
	generic_write_page(&lnpo);

	for (i = 0, lnpo.page = start_page; i < num; i++) {
		if (lnpo.page != 0) {
			lnpo.chip = npo->chip;
			lnpo.block = npo->block;
			lnpo.sect_bitmap = nci->sector_cnt_per_page;
			lnpo.mdata = mbuf;
			lnpo.sdata = spare;
			lnpo.slen = nci->sdata_bytes_per_page;
			generic_write_page(&lnpo);
		}
		lnpo.page++;
	}

	//check bad block flag
	memset(lnpo.sdata, 0xff, 64);

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.page = 0;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.mdata = NULL;
	lnpo.sdata = spare;
	lnpo.slen = nci->sdata_bytes_per_page;
	generic_read_page(&lnpo);

	if (lnpo.sdata[0] != 0xff) {
		nand_free_temp_buf(mbuf);
		return 0;
	}

	for (i = 0, lnpo.page = start_page; i < num; i++) {
		lnpo.chip = npo->chip;
		lnpo.block = npo->block;
		lnpo.sect_bitmap = nci->sector_cnt_per_page;
		lnpo.mdata = NULL;
		lnpo.sdata = spare;
		lnpo.slen = nci->sdata_bytes_per_page;
		generic_read_page(&lnpo);

		if (lnpo.sdata[0] != 0xff) {
			nand_free_temp_buf(mbuf);
			return 0;
		}
		lnpo.page++;
	}

	nand_free_temp_buf(mbuf);
	return ERR_NO_20;
}

int nand_phy_get_page_type(unsigned int page)
{
	/*
	   for SPECTEK L04a/L05b 3D nand
	   page type:1-> independent page,
	   page type:2-> lsb page
	   page type:3-> msb page
	   */
	if ((page <= 15) || (page >= 496))
		return 1;

	if (page % 2 == 0)
		return 2;

	return 3;
}

int nand_phy_low_page_write_cache_set(struct _nand_physic_op_par *npo,
				      unsigned int two_plane)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if (nand_phy_w_cache[i].cache_use_status == 0) {
			/*get a new lsb page cache and backup for L04a*/
			nand_phy_w_cache[i].cache_use_status = 1;
			nand_phy_w_cache[i].tmp_npo.chip = npo->chip;
			nand_phy_w_cache[i].tmp_npo.block = npo->block;
			nand_phy_w_cache[i].tmp_npo.page = npo->page;
			nand_phy_w_cache[i].tmp_npo.sect_bitmap = npo->sect_bitmap;
			nand_phy_w_cache[i].tmp_npo.slen = npo->slen;

			if (nand_phy_w_cache[i].tmp_npo.mdata == NULL) {
				if (two_plane == 1)
					nand_phy_w_cache[i].tmp_npo.mdata =
					    nand_malloc(
						2 * (npo->sect_bitmap << 9));
				else
					nand_phy_w_cache[i].tmp_npo.mdata =
					    nand_malloc(npo->sect_bitmap << 9);
			}
			if (nand_phy_w_cache[i].tmp_npo.sdata == NULL) {
				nand_phy_w_cache[i].tmp_npo.sdata = nand_malloc(npo->slen);
				memset(nand_phy_w_cache[i].tmp_npo.sdata, 0xff, npo->slen);
			}

			if (npo->mdata) {
				if (two_plane == 1)
					nand_memcpy(
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    npo->mdata,
					    2 * (npo->sect_bitmap << 9));
				else
					nand_memcpy(
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    npo->mdata,
					    (npo->sect_bitmap << 9));
			}

			if (npo->sdata)
				memcpy(nand_phy_w_cache[i].tmp_npo.sdata, npo->sdata, npo->slen);

			return 0;
		}
	}

	RAWNAND_ERR("ERR! no cache buf for lsb page!\n");

	return -1;
}

struct _nand_physic_op_par *nand_phy_low_page_cache_get_for_write(struct _nand_physic_op_par *npo)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].cache_use_status == 1) &&
		    (nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    ((nand_phy_w_cache[i].tmp_npo.page + 1) == npo->page) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.sect_bitmap == npo->sect_bitmap) &&
		    (nand_phy_w_cache[i].tmp_npo.slen == npo->slen) &&
		    (nand_phy_w_cache[i].tmp_npo.mdata != NULL) &&
		    (nand_phy_w_cache[i].tmp_npo.sdata != NULL)) {

			return &(nand_phy_w_cache[i].tmp_npo);
		}
	}
	RAWNAND_ERR("ERR! Not get LSB write cache!\n");

	return NULL;
}

int nand_phy_low_page_write_cache_cancle(struct _nand_physic_op_par *npo)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].cache_use_status == 1) &&
		    (nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.page == npo->page)) {

			nand_phy_w_cache[i].cache_use_status = 0;

			return 0;
		}
	}

	return -1;
}

int nand_phy_low_page_cache_get_for_read(struct _nand_physic_op_par *npo,
					 unsigned int two_plane)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.page == npo->page) &&
		    (nand_phy_w_cache[i].tmp_npo.mdata != NULL) &&
		    (nand_phy_w_cache[i].tmp_npo.sdata != NULL)) {

			if (npo->mdata) {
				if (two_plane == 1)
					nand_memcpy(
					    npo->mdata,
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    2 * (npo->sect_bitmap << 9));
				else
					nand_memcpy(
					    npo->mdata,
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    (npo->sect_bitmap << 9));
			}
			if (npo->sdata)
				memcpy(npo->sdata, nand_phy_w_cache[i].tmp_npo.sdata, npo->slen);

			return 0;
		}
	}

	return -1;
}

int generic_rw_page(struct _nand_physic_op_par *npo, unsigned int function, unsigned int two_plane)
{
	int ret = 0;

	if ((function == 0) && (two_plane == 0)) {
		ret |= generic_read_page(npo);
	} else if ((function == 0) && (two_plane == 1)) {
		ret |= generic_read_two_plane_page(npo);
	} else if ((function == 1) && (two_plane == 0)) {
		ret |= generic_write_page(npo);
	} else if ((function == 1) && (two_plane == 1)) {
		ret |= generic_write_two_plane_page(npo);
	} else {
		;
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_use_chip_function(struct _nand_physic_op_par *npo, unsigned int function)
{
	int ret, i;
	struct _nand_physic_op_par lnpo;
	struct nand_chip_info *nci;
	struct nand_super_chip_info *nsci = nsci_get_from_nssi(g_nssi, npo->chip);
	unsigned int chip[8];
	unsigned int block[8];
	unsigned int block_num;

	if (nsci->two_plane == 0) {
		if ((nsci->vertical_interleave == 1) && (nsci->dual_channel == 1)) {
			block_num = 4;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->d_channel_nci_2->chip_no;
			block[1] = npo->block;

			chip[2] = nsci->v_intl_nci_2->chip_no;
			block[2] = npo->block;

			nci = nci_get_from_nctri(nsci->d_channel_nci_2->nctri, nsci->v_intl_nci_1->chip_no);
			chip[3] = nci->chip_no;
			block[3] = npo->block;
		} else if (nsci->vertical_interleave == 1) {
			block_num = 2;
			chip[0] = nsci->v_intl_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->v_intl_nci_2->chip_no;
			block[1] = npo->block;
		} else if (nsci->dual_channel == 1) {
			block_num = 2;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->d_channel_nci_2->chip_no;
			block[1] = npo->block;
		} else {
			block_num = 1;
			chip[0] = npo->chip;
			block[0] = npo->block;
		}
	} else {
		if ((nsci->vertical_interleave == 1) && (nsci->dual_channel == 1)) {
			block_num = 8;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->d_channel_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->d_channel_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->d_channel_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;

			chip[4] = nsci->v_intl_nci_2->chip_no;
			block[4] = npo->block << 1;
			chip[5] = nsci->v_intl_nci_2->chip_no;
			block[5] = (npo->block << 1) + 1;

			nci = nci_get_from_nctri(nsci->d_channel_nci_2->nctri, nsci->v_intl_nci_1->chip_no);
			chip[6] = nci->chip_no;
			block[6] = npo->block << 1;
			chip[7] = nci->chip_no;
			block[7] = (npo->block << 1) + 1;
		} else if (nsci->vertical_interleave == 1) {
			block_num = 4;
			chip[0] = nsci->v_intl_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->v_intl_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->v_intl_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->v_intl_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;
		} else if (nsci->dual_channel == 1) {
			block_num = 4;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->d_channel_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->d_channel_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->d_channel_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;
		} else {
			block_num = 2;
			chip[0] = npo->chip;
			block[0] = npo->block << 1;
			chip[1] = npo->chip;
			block[1] = (npo->block << 1) + 1;
		}
	}

	for (i = 0, ret = 0; i < block_num; i++) {
		lnpo.chip = chip[i];
		lnpo.block = block[i];
		lnpo.page = 0;
		if (function == 0) {
			ret |= generic_erase_block(&lnpo);
			//ret |= generic_erase_block_start(&lnpo);
		} else if (function == 1) {
			ret |= generic_bad_block_check(&lnpo);
			if (ret != 0) {
				break;
			}
		} else if (function == 2) {
			ret |= generic_bad_block_mark(&lnpo);
		} else {
			;
		}
	}

	nand_wait_all_rb_ready();

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_rw_use_chip_function(struct _nand_physic_op_par *npo, unsigned int function)
{
	int ret = 0;
	struct _nand_physic_op_par lnpo;
	struct _nand_physic_op_par *low_npo;
	struct nand_super_chip_info *nsci = nsci_get_from_nssi(g_nssi, npo->chip);
	struct nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);

	unsigned int chip[4];
	unsigned int block[4];
	unsigned int page[4];
	unsigned char *mdata[4];
	unsigned char *sdata[4];
	unsigned int slen[4];
	unsigned int sect_bitmap[4];
	//	unsigned int block_num;
	unsigned char oob_temp[64];
	unsigned int i;

	if ((nsci->dual_channel == 1) && (nsci->two_plane == 1) && (function == 1)) {
		//		block_num = 4;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block << 1;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block << 1;
		page[1] = npo->page;
		mdata[1] = npo->mdata + (nsci->d_channel_nci_1->sector_cnt_per_page << 10);
		sect_bitmap[1] = nsci->d_channel_nci_2->sector_cnt_per_page;
		sdata[1] = npo->sdata;
		slen[1] = npo->slen;

		chip[2] = nsci->d_channel_nci_1->chip_no;
		block[2] = (npo->block << 1) + 1;
		page[2] = npo->page;
		mdata[2] = npo->mdata + (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[2] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[2] = npo->sdata;
		slen[2] = npo->slen;

		chip[3] = nsci->d_channel_nci_2->chip_no;
		block[3] = (npo->block << 1) + 1;
		page[3] = npo->page;
		mdata[3] = npo->mdata + (nsci->d_channel_nci_2->sector_cnt_per_page << 10) + (nsci->d_channel_nci_2->sector_cnt_per_page << 9);
		sect_bitmap[3] = nsci->d_channel_nci_2->sector_cnt_per_page;
		sdata[3] = npo->sdata;
		slen[3] = npo->slen;

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			for (i = 0; i < 16; i++) {
				if (i < 8) {
					oob_temp[i] = *((unsigned char *)npo->sdata + i);
				} else if (i == 8) {
					oob_temp[i] = 0xff;
				} else {
					oob_temp[i] = *((unsigned char *)npo->sdata + i - 1);
				}
			}
			sdata[0] = &oob_temp[0];
			sdata[1] = &oob_temp[0];
			sdata[2] = &oob_temp[8];
			sdata[3] = &oob_temp[8];
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= generic_write_page_start(&lnpo, 1);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		ret |= generic_write_page_start(&lnpo, 1);

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= generic_write_page_end(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[2];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		ret |= generic_write_page_start(&lnpo, 2);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		ret |= generic_write_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[3];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		ret |= generic_write_page_start(&lnpo, 2);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[2];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		ret |= generic_write_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[3];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		ret |= generic_write_page_end(&lnpo);

		return ret;
	}

	if ((nsci->dual_channel == 1) && (nsci->two_plane == 1) && (function == 0)) {
		//		block_num = 4;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block << 1;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block << 1;
		page[1] = npo->page;
		mdata[1] = npo->mdata + (nsci->d_channel_nci_1->sector_cnt_per_page << 10);
		sect_bitmap[1] = sect_bitmap[0];
		sdata[1] = NULL;
		slen[1] = 0;

		chip[2] = nsci->d_channel_nci_1->chip_no;
		block[2] = (npo->block << 1) + 1;
		page[2] = npo->page;
		mdata[2] = npo->mdata + (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[2] = sect_bitmap[0];
		sdata[2] = NULL;
		slen[2] = 0;

		chip[3] = nsci->d_channel_nci_2->chip_no;
		block[3] = (npo->block << 1) + 1;
		page[3] = npo->page;
		mdata[3] = npo->mdata + (nsci->d_channel_nci_2->sector_cnt_per_page << 10) + (nsci->d_channel_nci_2->sector_cnt_per_page << 9);
		sect_bitmap[3] = sect_bitmap[0];
		sdata[3] = NULL;
		slen[3] = 0;

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			sdata[0] = &oob_temp[0];
			sdata[1] = NULL;
			sdata[2] = &oob_temp[8];
			sdata[3] = NULL;
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= generic_read_page_start(&lnpo);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (mdata[0] != NULL)
			ret |= generic_read_page_start(&lnpo);

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= generic_read_page_end(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		if (mdata[0] != NULL)
			ret |= generic_read_page_start(&lnpo);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (mdata[0] != NULL)
			ret |= generic_read_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		if (mdata[0] != NULL)
			ret |= generic_read_page_start(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		if (mdata[0] != NULL)
			ret |= generic_read_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		if (mdata[0] != NULL)
			ret |= generic_read_page_end(&lnpo);

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			for (i = 0; i < 16; i++) {
				if (i < 8) {
					*((unsigned char *)npo->sdata + i) = oob_temp[i];
				} else if (i == 15) {
					*((unsigned char *)npo->sdata + i) = 0xff;
				} else {
					*((unsigned char *)npo->sdata + i) = oob_temp[i + 1];
				}
			}
		}

		return ret;
	}

	if (nsci->dual_channel == 1) {
		//		block_num = 2;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block;
		page[1] = npo->page;
		mdata[1] = npo->mdata + (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[1] = sect_bitmap[0];
		sdata[1] = npo->sdata;
		slen[1] = npo->slen;
		if (function == 0) {
			sdata[1] = NULL;
			slen[1] = 0;
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		if (function == 1) {
			ret |= generic_write_page_start(&lnpo, 0);
		} else {
			ret |= generic_read_page_start(&lnpo);
		}

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (function == 1) {
			ret |= generic_write_page_start(&lnpo, 0);
		} else {
			if (mdata[0] != NULL)
				ret |= generic_read_page_start(&lnpo);
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		if (function == 1) {
			ret |= generic_write_page_end(&lnpo);
		} else {
			ret |= generic_read_page_end(&lnpo);
		}

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (function == 1) {
			ret |= generic_write_page_end(&lnpo);
		} else {
			if (mdata[0] != NULL)
				ret |= generic_read_page_end(&lnpo);
		}

		return ret;
	}

	if (nsci->vertical_interleave == 1) {
		if (npo->page & 0x01) {
			lnpo.chip = nsci->v_intl_nci_2->chip_no;
		} else {
			lnpo.chip = nsci->v_intl_nci_1->chip_no;
		}
		lnpo.page = npo->page >> 1;
	} else {
		lnpo.chip = nsci->nci_first->chip_no;
		lnpo.page = npo->page;
	}
	lnpo.block = npo->block;
	lnpo.mdata = npo->mdata;
	/*lnpo.sect_bitmap = nsci->nci_first->sector_cnt_per_page;*/
	lnpo.sect_bitmap = npo->sect_bitmap;
	lnpo.sdata = npo->sdata;
	lnpo.slen = npo->slen;

	if (lnpo.mdata == NULL)
		lnpo.sect_bitmap = 0;

	if ((nci->npi->operation_opt & NAND_PAIRED_PAGE_SYNC) && (function == 1)) {
		if (nand_phy_get_page_type(lnpo.page) == 2) {
			ret = nand_phy_low_page_write_cache_set(
			    &lnpo, nsci->two_plane);
			return ret;
		} else if (nand_phy_get_page_type(lnpo.page) == 3) {
			low_npo = nand_phy_low_page_cache_get_for_write(&lnpo);

			if (low_npo) {
				ret |= generic_rw_page(low_npo, function, nsci->two_plane);
				nand_phy_low_page_write_cache_cancle(low_npo);
			} else {
				RAWNAND_ERR("ERR! Not get low page cache when write uper page\n");
			}
		} else {
			;
		}
	} else if ((nci->npi->operation_opt & NAND_PAIRED_PAGE_SYNC) &&
		   (function == 0)) {
		if ((nand_phy_get_page_type(lnpo.page) == 2) &&
		    (nand_phy_low_page_cache_get_for_read(
			 &lnpo, nsci->two_plane) == 0))
			return 0;
	} else {
		;
	}

	ret |= generic_rw_page(&lnpo, function, nsci->two_plane);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_erase_super_block(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = generic_use_chip_function(npo, 0);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_read_super_page(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = generic_rw_use_chip_function(npo, 0);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_write_super_page(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = generic_rw_use_chip_function(npo, 1);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_super_bad_block_check(struct _nand_physic_op_par *npo)
{

	int ret;

	ret = generic_use_chip_function(npo, 1);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_super_bad_block_mark(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = generic_use_chip_function(npo, 2);

	return ret;
}

struct df_read_page_end df_read_page_end = {
    .read_page_end = generic_read_page_end_not_retry,
};

struct rawnand_ops rawnand_ops = {
    .erase_single_block = generic_erase_block,
    .write_single_page = generic_write_page,
    .read_single_page = generic_read_page,
    .single_bad_block_check = generic_bad_block_check,
    .single_bad_block_mark = generic_bad_block_mark,
    .erase_super_block = generic_erase_super_block,
    .write_super_page = generic_write_super_page,
    .read_super_page = generic_read_super_page,
    .super_bad_block_check = generic_super_bad_block_check,
    .super_bad_block_mark = generic_super_bad_block_mark,
};
