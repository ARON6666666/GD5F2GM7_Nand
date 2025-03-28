#ifndef _NAND_FTL_H_
#define _NAND_FTL_H_
#include "nand_flash.h"

// 物理存储参数（根据GD5F2GM7数据手册定义）
#define NAND_PAGE_SIZE        PAGE_SIZE         // 主数据区大小
#define NAND_SPARE_SIZE       64                // 备用区大小 64byte（ECC开启时）
#define NAND_PAGES_PER_BLOCK  BLOCK_SIZE        // 每块页数
#define NAND_TOTAL_BLOCKS     BLOCK_COUNT       // 总块数（2Gb）



/*!
    \brief  逻辑块元数据结构
*/
#pragma pack(1)
typedef struct
{
    uint8_t bad_block_marker;           // 坏块标记
    uint16_t erase_count;               // 擦除次数
    uint16_t logical_block;             // 逻辑块号
    uint8_t valid_pages;                // 有效页数
    uint8_t page_used;                  // 当前页已使用标志
    uint8_t flags;                      // 标志位
} block_meta_data_t;
#pragma pack()

#define LOGICAL_BLOCKS     2000        // 用户可用逻辑块
#define SPARE_BLOCKS       48          // 保留块池
#define SECTOR_SIZE        NAND_PAGE_SIZE // FatFs扇区大小
#define SECTORS_PER_PAGE   1 

/*!
    \brief  ftl管理结构
*/
typedef struct {
    uint16_t logical_to_phy[LOGICAL_BLOCKS]; // 逻辑块到物理块映射
    uint8_t last_write_page_in_block[LOGICAL_BLOCKS];
    uint8_t bad_block_table[256];                // 坏块表 (2048 / 8 = 256字节)，一个位表示一个块
    uint16_t spare_blocks[SPARE_BLOCKS];            // 当前替换块指针
    uint32_t gc_counter;                      // 垃圾回收计数器
} ftl_t;



void ftl_init(void);
uint32_t ftl_convert_sector(uint32_t sector);
uint8_t ftl_garbage_collect(ftl_t*);
ftl_t* get_ftl_obj(void);
uint8_t ftl_write_page(uint32_t sector, uint8_t* pbuff);
uint8_t ftl_read_page(uint32_t sector, uint8_t* pbuff);
#endif