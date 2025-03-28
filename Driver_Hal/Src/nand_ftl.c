#include "nand_ftl.h"
#include <string.h>


static ftl_t ftl;

ftl_t* get_ftl_obj(void)
{
    return &ftl;
}




static void ftl_read_block_meta(uint16_t phy_block, block_meta_data_t* meta)
{
    nand_flash_read_page_spare(phy_block, (uint8_t *)meta, sizeof(block_meta_data_t));
}

static void ftl_write_block_meta(uint16_t phy_block, block_meta_data_t* meta)
{
    nand_flash_write_page_spare(phy_block, (uint8_t *)meta, sizeof(block_meta_data_t));
}


/*！
    \brief  检查块是否格式化
*/
static uint8_t ftl_is_block_formatted(uint16_t phy_block, block_meta_data_t* meta)
{
    ftl_read_block_meta(phy_block*NAND_PAGES_PER_BLOCK, meta);
    return meta->flags;
}


/*!
    \brief 逻辑扇区转换物理地址

*/
uint32_t ftl_convert_sector(uint32_t sector)
{
    uint32_t logic_block = sector / NAND_PAGES_PER_BLOCK;
    uint32_t page_offset = sector % NAND_PAGES_PER_BLOCK;


    return ftl.logical_to_phy[logic_block] * NAND_PAGES_PER_BLOCK + page_offset;
}


/*！
    \brief  FTL和nand初始化
*/
void ftl_init(void)
{
    // step 1. 初始化nand flash
    nand_flash_initialize();

    // step 2. 构建坏块表
    memset(&ftl, 0, sizeof(ftl_t));
    uint16_t valid_blocks = 0;
    for (uint16_t block = 0; block < NAND_TOTAL_BLOCKS; block++)
    {
        // 计算块地址，检查出厂时标记的坏块
        uint32_t addr = block * BLOCK_SIZE;
        if (nand_flash_bad_block_check(addr) != 0xFF)
        {
            ftl.bad_block_table[block/8] |= (1 << (block%8)); 
        } 
        else if (valid_blocks < LOGICAL_BLOCKS)
        {
            ftl.logical_to_phy[valid_blocks] = block;
            valid_blocks++;
        }
    }
    
    // step 3. 初始化替换池
    uint16_t spare_idx = 0;
    for (uint16_t blk = NAND_TOTAL_BLOCKS - SPARE_BLOCKS; blk < NAND_TOTAL_BLOCKS; blk++)
    {
        if (!(ftl.bad_block_table[blk/8] & (1 << (blk%8))))
        {
            ftl.spare_blocks[spare_idx++] = blk;
        }
    }
}



/*!
    \brief 垃圾回收
*/
uint8_t ftl_garbage_collect(ftl_t* ftl)
{
    // 选择回收后续块（最少有效页）
    uint16_t victim_block = 0;
    uint8_t min_valid = 0xFF;
    

    // step 1. 寻找最少有效页的块
    for (uint16_t i = 0; i < LOGICAL_BLOCKS; i++)
    {
        uint32_t addr = i*NAND_PAGES_PER_BLOCK + ftl->last_write_page_in_block[i];
        block_meta_data_t meta;
        ftl_read_block_meta(addr, &meta);
        if (meta.valid_pages < min_valid)
        {
            min_valid = meta.valid_pages;
            victim_block = i;
        }
    }

    uint16_t old_phy = ftl->logical_to_phy[victim_block];

    // old地址是Fatfs保留区，在创建文件系统，此时不需要迁移
    if (old_phy == 0)
    {
        return 0;
    }

    // step 2.分配新块
    if (ftl->spare_blocks[0] == 0xFFFF) return 1; // 无可用替换块
    uint16_t new_block = ftl->spare_blocks[0];
    memmove(ftl->spare_blocks, ftl->spare_blocks+1, (SPARE_BLOCKS-1)*2);

    // step 3. 迁移有效数据到备用区
    for (uint8_t page = 0; page < NAND_PAGES_PER_BLOCK; page++)
    {
        uint32_t src_addr = old_phy*NAND_PAGES_PER_BLOCK + page*NAND_PAGE_SIZE;
        uint32_t dst_addr = new_block*NAND_PAGES_PER_BLOCK + page*NAND_PAGE_SIZE;

        // 使用Internal Data  Move迁移
        nand_flash_internal_page_data_move(src_addr, dst_addr);
    
    }

    // step 4. 
    block_meta_data_t meta = {
        .erase_count = 1,
        .logical_block = victim_block,
        .valid_pages = NAND_PAGES_PER_BLOCK,
        .page_used = 0x01,
        .flags = 0
    };
    ftl_write_block_meta(new_block, &meta);

    // step 5. 擦除旧块
    nand_flash_erase_block(old_phy*BLOCK_SIZE);
    ftl->logical_to_phy[victim_block] = new_block;
    

    return 0;
}


uint8_t ftl_write_page(uint32_t sector, uint8_t* pbuff)
{
    uint32_t block_base = sector / NAND_PAGES_PER_BLOCK;
    uint8_t  page = sector % NAND_PAGES_PER_BLOCK;
    block_meta_data_t block_meta = {0};

    uint32_t phy_addr = ftl_convert_sector(sector);

    // step 1. 检查该块是否格式化了？若没有格式化.
    uint8_t format_flag = ftl_is_block_formatted(block_base, &block_meta);
    if (format_flag != 0x01)
    {
        if (format_flag != 0xFF) nand_flash_erase_block(phy_addr);
        block_meta.bad_block_marker = 0xFF;
        block_meta.erase_count = 1;
        block_meta.logical_block = block_base;
        block_meta.valid_pages = NAND_PAGES_PER_BLOCK-1;
        block_meta.page_used = 0x01;
        block_meta.flags = 0x01;

        if (page != 0)
        {
            //写入页不是第一页
            ftl_write_block_meta(phy_addr, &block_meta);
			block_meta.page_used = 0xFF;
			block_meta.valid_pages = NAND_PAGES_PER_BLOCK-2;
        }
        // 在首页写入该block_base的元数据
        ftl.last_write_page_in_block[block_base] = page;    //记录最后一次操作的页码，该页记录最新的元数据
        ftl_write_block_meta(phy_addr & 0x1FFC0, &block_meta);
        nand_flash_write_page(phy_addr, PROGRAM_LOAD_x4_CMD, pbuff, NAND_PAGE_SIZE);
        return 0;
    }

    // step 2. 检查该页是否已经写过了？
    block_meta_data_t current_meta = {0};
    ftl_read_block_meta(phy_addr, &current_meta);
    if (current_meta.page_used == 0x01)
    {
        // 该页已经写过了，不再写入，垃圾回收
        ftl_garbage_collect(&ftl);
        // return 0;
    }

    
    // step 3. 更新元数据
    block_meta.logical_block = block_base;
    block_meta.valid_pages -= page;
    block_meta.page_used = 0x01;
    ftl_write_block_meta(phy_addr, &block_meta);
    ftl.last_write_page_in_block[block_base] = page;    //记录最后一次操作的页码，该页记录最新的元数据

    // step 4. 写入数据。坏块管理？
    nand_flash_write_page(phy_addr, PROGRAM_LOAD_x4_CMD, pbuff, NAND_PAGE_SIZE);
    return 0;
}

uint8_t ftl_read_page(uint32_t sector, uint8_t* pbuff)
{
    uint32_t phy_addr = ftl_convert_sector(sector);
    // 坏块管理？
    return nand_flash_read_page_from_cache(phy_addr, READ_CACHE_QUAD_CMD, pbuff, NAND_PAGE_SIZE);
}
/*!
    \brief ECC校验处理
*/
// static int ftl_check_ecc(uint32_t addr)
// {
//     uint8_t status = nand_flash_get
// }
