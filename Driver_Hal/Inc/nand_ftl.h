#ifndef _NAND_FTL_H_
#define _NAND_FTL_H_
#include "nand_flash.h"
#include "ff.h"

// 物理存储参数（根据GD5F2GM7数据手册定义）
#define NAND_PAGE_SIZE        PAGE_SIZE         // 主数据区大小
#define NAND_SPARE_SIZE       64                // 备用区大小 64byte（ECC开启时）
#define NAND_PAGES_PER_BLOCK  BLOCK_SIZE        // 每块页数
#define NAND_TOTAL_BLOCKS     BLOCK_COUNT       // 总块数（2Gb）
#define CRITICAL_BLOCKS       2

#define REGION_BOOT           0
#define REGION_FAT            1
#define REGION_ROOT_DIR       2
#define REGION_DATA           255

/*!
    \brief  逻辑块元数据结构
*/
#pragma pack(1)
typedef struct
{
    uint8_t bad_block_marker;           // 坏块标记
    uint16_t erase_count;               // 擦除次数
    uint8_t valid_pages;                // 有效页数
    uint8_t page_used;                  // 当前页已使用标志
    uint8_t flags;                      // 格式化标志位
    uint8_t region_type;                // 区域类型 255 数据区 0 引导区 1 FAT区 2 根目录区
} block_meta_data_t;
#pragma pack()

#define LOGICAL_BLOCKS     2000        // 用户可用逻辑块
#define SPARE_BLOCKS       48          // 保留块池
#define SECTOR_SIZE        NAND_PAGE_SIZE // FatFs扇区大小
#define SECTORS_PER_PAGE   1 
#define FTL_DESC_BLOCK     2047         // 这个块用来存放FTL描述信息


/*!
    \brief  ftl管理结构
*/
#pragma pack(1)
typedef struct {
    uint16_t logical_to_phy[LOGICAL_BLOCKS];     // 逻辑块到物理块映射
    uint8_t last_write_page_in_block[LOGICAL_BLOCKS];
    uint8_t bad_block_table[256];                // 坏块表 (2048 / 8 = 256字节)，一个位表示一个块
    uint16_t spare_blocks[SPARE_BLOCKS];            // 当前替换块指针
    uint32_t gc_counter;                         // 垃圾回收计数器
    uint16_t boot_block;                           // 引导区起始块
    uint16_t fat_block;                           // fat起始块
    uint16_t root_dir_block;                      // 根目录起始块
} ftl_t;
#pragma pack()

typedef struct
{
    uint8_t buffer[2][2048];      // 双缓冲区（大小需与存储介质块对齐）
    volatile uint32_t active_buf;  // 当前活动缓冲区索引（0/1）
    volatile uint8_t buf_ready[2];    // 缓冲区就绪标志
    uint32_t current_lba;          // 当前已缓存的LBA地址
    uint32_t next_lba;             // 预读取的LBA地址
    uint32_t max_lba;              // 存储介质最大LBA
} double_buffer_t;
extern double_buffer_t dbuf;


void ftl_init(void);
uint32_t ftl_convert_sector(uint32_t sector);
uint8_t ftl_garbage_collect(ftl_t*);

uint8_t ftl_write_page(uint32_t sector, uint8_t* pbuff);
uint8_t ftl_write_critical(uint32_t sector, uint8_t* data, uint8_t region_type);
uint8_t ftl_read_page(uint32_t sector, uint8_t* pbuff);

uint8_t ftl_identify_region(uint32_t sector);
// uint8_t ftl_is_root_region(uint32_t sector);
// uint8_t ftl_is_rootdir_region(uint32_t sector);
// uint8_t ftl_is_fat_region(uint32_t sector);

void ftl_save_descriptor(uint16_t block_no);

void ftl_mark_critical_regions(uint32_t boot_start_page, uint32_t fat_start_page, uint32_t root_dir_start_page);
void ftl_init_critical_spare_block(uint32_t boot_start_page, uint32_t fat_start_page, uint32_t root_dir_start_page);

int8_t ftl_read_data(uint8_t lun, uint8_t *buf, uint32_t lba, uint32_t len);
void ftl_init_double_buffer(void);

#endif