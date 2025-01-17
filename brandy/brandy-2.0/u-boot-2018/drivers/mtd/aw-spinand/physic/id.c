// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "sunxi-spinand-phy: " fmt

#include <common.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mtd/aw-spinand.h>

#include "physic.h"

#define KB (1024)
#define MB (KB * 1024)
#define to_kb(size) (size / KB)
#define to_mb(size) (size / MB)

/* manufacture num */
#define MICRON_MANUFACTURE	0x2c
#define GD_MANUFACTURE		0xc8
#define ATO_MANUFACTURE		0x9b
#define WINBOND_MANUFACTURE	0xef
#define MXIC_MANUFACTURE	0xc2
#define TOSHIBA_MANUFACTURE	0x98
#define ETRON_MANUFACTURE	0xd5
#define XTXTECH_MANUFACTURE	0xa1
#define DSTECH_MANUFACTURE	0xe5
#define FORESEE_MANUFACTURE	0xcd
#define ZETTA_MANUFACTURE	0xba

struct aw_spinand_phy_info gigadevice[] =
{
	{
		.Model		= "GD5F1GQ4UCYIG",
		.NandID		= {0xc8, 0xb1, 0x48, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_ONEDUMMY_AFTER_RANDOMREAD,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT3_LIMIT2_TO_6_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "GD5F1GQ4UBYIG",
		.NandID		= {0xc8, 0xd1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN8_OFF4,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* GD5F2GQ4UB9IG did not check yet */
		.Model		= "GD5F2GQ4UB9IG",
		.NandID		= {0xc8, 0xd2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "F50L1G41LB(2M)",
		.NandID		= {0xc8, 0x01, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag	= BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info micron[] =
{
	{
		.Model		= "MT29F1G01ABAGDWB",
		.NandID		= {0x2c, 0x14, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT3_LIMIT5_ERR2,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "MT29F2G01ABAGDWB",
		.NandID		= {0x2c, 0x24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT3_LIMIT5_ERR2 ,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info xtx[] =
{

};

struct aw_spinand_phy_info etron[] =
{

};

struct aw_spinand_phy_info toshiba[] =
{

};

struct aw_spinand_phy_info ato[] =
{

};

struct aw_spinand_phy_info mxic[] =
{
	{
		.Model		= "MX35LF1GE4AB",
		.NandID		= {0xc2, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccFlag	= HAS_EXT_ECC_STATUS,
		.EccType	= BIT4_LIMIT3_TO_4_ERR15,
		/**
		 * MX35LF1GE4AB should use SIZE16_OFF4_LEN12, however, in order
		 * to compatibility with versions already sent to customers,
		 * which do not use general physical layout, we used
		 * SIZE16_OFF4_LEN4_OFF8 instead.
		 */
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info winbond[] =
{
	{
		.Model		= "W25N01GVZEIG",
		.NandID		= {0xef, 0xaa, 0x21, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info dosilicon[] =
{
	{
		.Model		= "DS35X1GAXXX",
		.NandID		= {0xe5, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info foresee[] =
{
	{
		.Model		= "FS35ND01G-S1F1QWFI000",
		.NandID		= {0xcd, 0xb1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT3_LIMIT3_TO_4_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info zetta[] =
{
	{
		.Model		= "ZD35Q1GAIB",
		.NandID		= {0xba, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

static const char *aw_spinand_info_model(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->Model;
}

static void aw_spinand_info_nandid(struct aw_spinand_chip *chip, unsigned char *id,
		int cnt)
{
	int i;
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	cnt = min(cnt, MAX_ID_LEN);
	for (i = 0; i < cnt; i++)
		id[i] = pinfo->NandID[i];
}

static unsigned int aw_spinand_info_sector_size(struct aw_spinand_chip *chip)
{
	return 1 << SECTOR_SHIFT;
}

static unsigned int aw_spinand_info_phy_page_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->SectCntPerPage * aw_spinand_info_sector_size(chip);
}

static unsigned int aw_spinand_info_page_size(struct aw_spinand_chip *chip)
{
#if SIMULATE_MULTIPLANE
	return aw_spinand_info_phy_page_size(chip) * 2;
#else
	return aw_spinand_info_phy_page_size(chip);
#endif
}

static unsigned int aw_spinand_info_phy_block_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->PageCntPerBlk * aw_spinand_info_phy_page_size(chip);
}

static unsigned int aw_spinand_info_block_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->PageCntPerBlk * aw_spinand_info_page_size(chip);
}

static unsigned int aw_spinand_info_phy_oob_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->OobSizePerPage;
}

static unsigned int aw_spinand_info_oob_size(struct aw_spinand_chip *chip)
{
#if SIMULATE_MULTIPLANE
	return aw_spinand_info_phy_oob_size(chip) * 2;
#else
	return aw_spinand_info_phy_oob_size(chip);
#endif
}

static unsigned int aw_spinand_info_die_cnt(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->DieCntPerChip;
}

static unsigned int aw_spinand_info_total_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->DieCntPerChip * pinfo->BlkCntPerDie *
		aw_spinand_info_phy_block_size(chip);
}

static int aw_spinand_info_operation_opt(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->OperationOpt;
}

static int aw_spinand_info_max_erase_times(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->MaxEraseTimes;
}

struct spinand_manufacture {
	unsigned char id;
	const char *name;
	struct aw_spinand_phy_info *info;
	unsigned int cnt;
};

#define SPINAND_FACTORY_INFO(_id, _name, _info)			\
	{							\
		.id = _id,					\
		.name = _name,					\
		.info = _info,					\
		.cnt = ARRAY_SIZE(_info),			\
	}
static struct spinand_manufacture spinand_factory[] = {
	SPINAND_FACTORY_INFO(MICRON_MANUFACTURE, "Micron", micron),
	SPINAND_FACTORY_INFO(GD_MANUFACTURE, "GD", gigadevice),
	SPINAND_FACTORY_INFO(ATO_MANUFACTURE, "ATO", ato),
	SPINAND_FACTORY_INFO(WINBOND_MANUFACTURE, "Winbond", winbond),
	SPINAND_FACTORY_INFO(MXIC_MANUFACTURE, "Mxic", mxic),
	SPINAND_FACTORY_INFO(TOSHIBA_MANUFACTURE, "Toshiba", toshiba),
	SPINAND_FACTORY_INFO(ETRON_MANUFACTURE, "Etron", etron),
	SPINAND_FACTORY_INFO(XTXTECH_MANUFACTURE, "XTX", xtx),
	SPINAND_FACTORY_INFO(DSTECH_MANUFACTURE, "Dosilicon", dosilicon),
	SPINAND_FACTORY_INFO(FORESEE_MANUFACTURE, "Foresee", foresee),
	SPINAND_FACTORY_INFO(ZETTA_MANUFACTURE, "Zetta", zetta),
};

static const char *aw_spinand_info_manufacture(struct aw_spinand_chip *chip)
{
	int i, j;
	struct spinand_manufacture *m;
	struct aw_spinand_phy_info *pinfo;

	for (i= 0; i < ARRAY_SIZE(spinand_factory); i++) {
		m = &spinand_factory[i];
		pinfo = chip->info->phy_info;
		for (j = 0; j < m->cnt; j++)
			if (pinfo == &m->info[j])
				return m->name;
	}
	return NULL;
}

static struct spinand_manufacture *spinand_detect_munufacture(unsigned char id)
{
	int index;
	struct spinand_manufacture *m;

	for (index = 0; index < ARRAY_SIZE(spinand_factory); index++) {
		m = &spinand_factory[index];
		if (m->id == id) {
			pr_info("detect munufacture: %s\n", m->name);
			return m;
		}
	}

	pr_err("not detect any munufacture\n");
	return NULL;
}

static struct aw_spinand_phy_info *spinand_match_id(
		struct spinand_manufacture *m,
		unsigned char *id)
{
	int i, j, match_max = 1, match_index = 0;
	struct aw_spinand_phy_info *pinfo;

	for (i = 0; i < m->cnt; i++) {
		int match = 1;

		pinfo = &m->info[i];
		for (j = 1; j < MAX_ID_LEN; j++) {
			/* 0xFF matching all ID value */
			if (pinfo->NandID[j] != id[j] &&
					pinfo->NandID[j] != 0xFF)
				break;

			if (pinfo->NandID[j] != 0xFF)
				match++;
		}

		if (match > match_max) {
			match_max = match;
			match_index = i;
		}
	}

	if (match_max > 1)
		return &m->info[match_index];
	return NULL;
}

static struct aw_spinand_info aw_spinand_info = {
	.model = aw_spinand_info_model,
	.manufacture = aw_spinand_info_manufacture,
	.nandid = aw_spinand_info_nandid,
	.die_cnt = aw_spinand_info_die_cnt,
	.oob_size = aw_spinand_info_oob_size,
	.sector_size = aw_spinand_info_sector_size,
	.page_size = aw_spinand_info_page_size,
	.block_size = aw_spinand_info_block_size,
	.phy_oob_size = aw_spinand_info_phy_oob_size,
	.phy_page_size = aw_spinand_info_phy_page_size,
	.phy_block_size = aw_spinand_info_phy_block_size,
	.total_size = aw_spinand_info_total_size,
	.operation_opt = aw_spinand_info_operation_opt,
	.max_erase_times = aw_spinand_info_max_erase_times,
};

static int aw_spinand_info_init(struct aw_spinand_chip *chip,
		struct aw_spinand_phy_info *pinfo)
{
	chip->info = &aw_spinand_info;
	chip->info->phy_info = pinfo;

	pr_info("========== arch info ==========\n");
	pr_info("Model:               %s\n", pinfo->Model);
	pr_info("Munufacture:         %s\n", aw_spinand_info_manufacture(chip));
	pr_info("DieCntPerChip:       %u\n", pinfo->DieCntPerChip);
	pr_info("BlkCntPerDie:        %u\n", pinfo->BlkCntPerDie);
	pr_info("PageCntPerBlk:       %u\n", pinfo->PageCntPerBlk);
	pr_info("SectCntPerPage:      %u\n", pinfo->SectCntPerPage);
	pr_info("OobSizePerPage:      %u\n", pinfo->OobSizePerPage);
	pr_info("BadBlockFlag:        0x%x\n", pinfo->BadBlockFlag);
	pr_info("OperationOpt:        0x%x\n", pinfo->OperationOpt);
	pr_info("MaxEraseTimes:       %d\n", pinfo->MaxEraseTimes);
	pr_info("EccFlag:             0x%x\n", pinfo->EccFlag);
	pr_info("EccType:             %d\n", pinfo->EccType);
	pr_info("EccProtectedType:    %d\n", pinfo->EccProtectedType);
	pr_info("========================================\n");
	pr_info("\n");
	pr_info("========== physical info ==========\n");
	pr_info("TotalSize:    %u M\n", to_mb(aw_spinand_info_total_size(chip)));
	pr_info("SectorSize:   %u B\n", aw_spinand_info_sector_size(chip));
	pr_info("PageSize:     %u K\n", to_kb(aw_spinand_info_phy_page_size(chip)));
	pr_info("BlockSize:    %u K\n", to_kb(aw_spinand_info_phy_block_size(chip)));
	pr_info("OOBSize:      %u B\n", aw_spinand_info_phy_oob_size(chip));
	pr_info("========================================\n");
	pr_info("\n");
	pr_info("========== logical info ==========\n");
	pr_info("TotalSize:    %u M\n", to_mb(aw_spinand_info_total_size(chip)));
	pr_info("SectorSize:   %u B\n", aw_spinand_info_sector_size(chip));
	pr_info("PageSize:     %u K\n", to_kb(aw_spinand_info_page_size(chip)));
	pr_info("BlockSize:    %u K\n", to_kb(aw_spinand_info_block_size(chip)));
	pr_info("OOBSize:      %u B\n", aw_spinand_info_oob_size(chip));
	pr_info("========================================\n");

	return 0;
}

int aw_spinand_chip_detect(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo;
	struct spinand_manufacture *m;
	unsigned char id[MAX_ID_LEN] = {0xFF};
	struct aw_spinand_chip_ops *ops = chip->ops;
	int ret, dummy = 0;

retry:
	ret = ops->read_id(chip, id, MAX_ID_LEN, dummy);
	if (ret) {
		pr_err("read id failed : %d\n", ret);
		return ret;
	}

	m = spinand_detect_munufacture(id[0]);
	if (!m)
		goto not_detect;

	pinfo = spinand_match_id(m, id);
	if (pinfo)
		goto detect;

not_detect:
	/* retry with dummy */
	if (!dummy) {
		dummy++;
		goto retry;
	}
	pr_info("not match spinand: %x %x\n",
			*(__u32 *)id,
			*((__u32 *)id + 1));
	return -ENODEV;
detect:
	pr_info("detect spinand id: %x %x\n",
			*((__u32 *)pinfo->NandID),
			*((__u32 *)pinfo->NandID + 1));
	return aw_spinand_info_init(chip, pinfo);
}
