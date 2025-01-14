/*
 * Allwinner SoCs eink sys driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _EINK_SYS_SOURCE_H_
#define _EINK_SYS_SOURCE_H_

#include <common.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <sunxi_display2.h>
#include <sunxi_metadata.h>
#include <sys_config.h>
#include <pwm.h>
#include <asm/arch/timer.h>
#include <linux/list.h>
#include <asm/memory.h>
#include <div64.h>
#include <fdt_support.h>
#include <sunxi_power/axp.h>
#include "asm/io.h"
#include <linux/compat.h>
#include <asm/arch/timer.h>
#include <time.h>

#include "eink_driver.h"

#define EINK_PRINT_LEVEL 0x1
/* #define TIME_COUNTER_DEBUG */
//#define DE_WB_DEBUG
//#define SAVE_DE_WB_BUF
//#define WAVEDATA_DEBUG
/* #define INDEX_DEBUG */
/* #define DECODE_DEBUG */
/* #define ION_DEBUG_DUMP */
//#define TIMING_BUF_DEBUG
#define PIPELINE_DEBUG
#define BUFFER_LIST_DEBUG
//#define VIRTUAL_REGISTER /* for test on local */
//#define REGISTER_PRINT

#ifdef DE_WB_DEBUG
#define DEFAULT_GRAY_PIC_PATH "/system/eink_image.bin"
#endif

#ifdef WAVEDATA_DEBUG
#define DEFAULT_INIT_WAV_PATH "/system/init_wf.bin"
#define DEFAULT_GC16_WAV_PATH "/system/gc16_wf.bin"
#endif
#ifdef SAVE_DE_WB_BUF
#define SAVE_BUF_PATH "./eink_wb_img.bin"
#endif

#ifdef TIMING_BUF_DEBUG
#define DEFAULT_TIMING_BUF_PATH "/system/timing_buf.bin"
#endif

typedef struct file ES_FILE;
extern u32 eink_dbg_info;

#define EINK_INFO_MSG(fmt, args...) \
	do {\
		if (eink_dbg_info & 0x1)\
		pr_info("[EINK-%-24s] line:%04d: " fmt, __func__, __LINE__, ##args);\
	} while (0)

#define EINK_DEFAULT_MSG(fmt, args...) \
	do {\
		if (eink_dbg_info & 0x2)\
		pr_info("[EINK-%-24s] line:%04d: " fmt, __func__, __LINE__, ##args);\
	} while (0)

#define EINKALIGN(value, align) ((align == 0) ? \
				value : \
				(((value) + ((align) - 1)) & ~((align) - 1)))

struct fb_address_transfer {
	enum disp_pixel_format format;
	struct disp_rectsz size[3];
	unsigned int align[3];
	int depth;
	dma_addr_t dma_addr;
	unsigned long long addr[3];
	unsigned long long trd_right_addr[3];
};

struct eink_format_attr {
	enum upd_pixel_fmt format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

#define EINK_IRQ_RETURN IRQ_HANDLED
#define EINK_PIN_STATE_ACTIVE "active"
#define EINK_PIN_STATE_SLEEP "sleep"

/* BMP文件：文件头(地址0x0000~0x000D，共14字节) */
#pragma pack(1)
typedef struct {
	u8	bfType[2];         //表明位图文件的类型，必须为BM
	u32	bfSize;		   //表明bmp 文件的大小，以字节为单位
	u16	bfReserved1;       //属于保留字，必须为本0
	u16	bfReserved2;	   //也是保留字，必须为本0
	u32	bfOffBits;	   //位图阵列的起始位置，以字节为单位
} BMP_FILE_HEADER;

#define BMP_FILE_HEADER_SIZE	(sizeof(BMP_FILE_HEADER))

/* BMP文件：信息头(地址0x000E~0x0035，共40字节) */
typedef struct {
	u32	biSize;               //指出本数据结构所需要的字节数
	u32	biWidth;              //以象素为单位，给出BMP图象的宽度
	u32	biHeight;             //以象素为单位，给出BMP图象的高度
	u16	biPlanes;	      //输出设备的位平面数，必须置为1
	u16	biBitCount;	      //给出每个象素的位数
	u32	biCompress;	      //给出位图的压缩类型
	u32	biSizeImage;	      //给出图象字节数的多少
	u32	biXPelsPerMeter;      //图像的水平分辨率
	u32	biYPelsPerMeter;      //图象的垂直分辨率
	u32	biClrUsed;	      //调色板中图象实际使用的颜色素数, 为0 ，表示默认大小等于(2 ^ biBitCount)  Bytes
	u32	biClrImportant;	      //给出重要颜色的索引值
} BMP_INFO_HEADER;
#pragma pack()

#define BMP_INFO_HEADER_SIZE	(sizeof(BMP_INFO_HEADER))
#define BMP_MIN_SIZE		(BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)

//BMP文件：彩色表
typedef struct {
	u8 r;				//red
	u8 g;				//green
	u8 b;				//blue
	u8 reserved;			//alpha透明度，一般不用
} ST_ARGB;

typedef struct {
	u32  width;	                //以象素为单位，给出BMP图象的宽度
	u32  height;	                //以象素为单位，给出BMP图象的高度
	u16  bit_count;			//给出每个象素的位数
	u32  image_size;	        //给出图象字节数的多少
	u32  color_tbl_size;		//调色板中图象实际使用的颜色素数, 为0 ，表示默认大小等于(2 ^ biBitCount)  Bytes
} BMP_INFO;

#pragma pack(1)
typedef struct {
	u8 blue;
	u8 green;
	u8 red;
} ST_RGB;
#pragma pack()

#define BMP_COLOR_SIZE              (sizeof(ST_ARGB) * 256)
#define BMP_IMAGE_DATA_OFFSET       (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + BMP_COLOR_SIZE)

extern bool is_upd_win_zero(struct upd_win update_area);
extern void *malloc_aligned(u32 size, u32 alignment);
extern void free_aligned(void *aligned_ptr);
extern int eink_sys_script_get_item(char *main_name, char *sub_name, int value[], int type);
extern void save_upd_rmi_buffer(u32 order, u8 *buf, u32 len);
extern void save_one_wavedata_buffer(u8 *buf, bool is_edma);
extern void save_rearray_waveform_to_mem(u8 *buf, u32 len);
extern s32 eink_panel_pin_cfg(u32 en);
extern int eink_sys_gpio_request(struct eink_gpio_cfg *gpio_list, u32 group_count_max);
extern int eink_sys_gpio_release(int p_handler, s32 if_release_to_default_status);
extern int eink_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name);


extern struct dmabuf_item *eink_dma_map(int fd);
extern void eink_dma_unmap(struct dmabuf_item *item);

extern void eink_put_gray_to_mem(u32 order, char *buf, u32 width, u32 height);
extern void save_waveform_to_mem(u32 order, u8 *buf, u32 frames, u32 bit_num);
extern int eink_get_gray_from_mem(__u8 *buf, char *file_name, __u32 length, loff_t pos);
extern int save_as_bin_file(__u8 *buf, char *file_name, __u32 length, loff_t pos);
extern void print_free_pipe_list(struct pipe_manager *mgr);
extern void print_used_pipe_list(struct pipe_manager *mgr);
extern void eink_print_register(unsigned long start_addr, unsigned long end_addr);
extern void eink_fdt_init(void);
extern int eink_fdt_nodeoffset(char *main_name);
extern uintptr_t eink_getprop_regbase(char *main_name, char *sub_name, u32 index);
extern u32 eink_getprop_irq(char *main_name, char *sub_name, u32 index);
#endif
