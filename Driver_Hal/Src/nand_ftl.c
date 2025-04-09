#include "nand_ftl.h"
#include <string.h>

// ftl 控制
static ftl_t ftl = {
    .boot_block = 0xFFFF,
    .fat_block = 0xFFFF,
    .root_dir_block = 0xFFFF,
};

// 引导块的元数据
static block_meta_data_t boot_meta = {.region_type = 0xFF};
// FAT区的元数据
static block_meta_data_t fat_meta = {.region_type = 0xFF};
// ROOT DIR区的元数据
static block_meta_data_t rootdir_meta = {.region_type = 0xFF};

// 引导区、FAT区、ROOTDIR 区域的替换块
static uint16_t critical_blocks[CRITICAL_BLOCKS][CRITICAL_BLOCKS];

// 预读缓存


// static function prototype
static void ftl_read_block_meta(uint32_t phy_block, block_meta_data_t* meta);
static void ftl_write_block_meta(uint32_t phy_block, block_meta_data_t* meta);
static uint8_t ftl_is_block_formatted(uint16_t phy_block, block_meta_data_t* meta);
static uint8_t ftl_load_descriptor(uint16_t block_no);
static uint16_t ftl_alloc_critical_block(uint32_t addr);


/*!
    \brief  读取块的元数据
*/
static void ftl_read_block_meta(uint32_t phy_block, block_meta_data_t* meta)
{
    nand_flash_read_page_spare(phy_block, (uint8_t *)meta, sizeof(block_meta_data_t));
}

/*!
    \brief  保存块的元数据
*/
static void ftl_write_block_meta(uint32_t phy_block, block_meta_data_t* meta)
{
    nand_flash_write_page_spare(phy_block, (uint8_t *)meta, sizeof(block_meta_data_t));
}


/*!
    \brief  检查块是否格式化
*/
static uint8_t ftl_is_block_formatted(uint16_t phy_block, block_meta_data_t* meta)
{
    ftl_read_block_meta(phy_block*NAND_PAGES_PER_BLOCK, meta);
    return meta->flags;
}

/*!
    \brief 加载ftl描述符部分内容
*/
static uint8_t ftl_load_descriptor(uint16_t block_no)
{
    uint32_t addr = block_no*NAND_PAGES_PER_BLOCK;
    uint8_t* offset = (uint8_t *)&ftl.logical_to_phy[0];
    nand_flash_read_page_from_cache(addr,
                                    READ_CACHE_QUAD_CMD,
                                    offset,
                                    LOGICAL_BLOCKS);
    nand_flash_read_page_from_cache(addr+1,
                                    READ_CACHE_QUAD_CMD,
                                    offset + LOGICAL_BLOCKS,
                                    LOGICAL_BLOCKS);
    nand_flash_read_page_from_cache(addr+2,
                                    READ_CACHE_QUAD_CMD,
                                    &ftl.last_write_page_in_block[0],
                                    LOGICAL_BLOCKS);
    if (ftl.logical_to_phy[0] == 0xFFFF || ftl.last_write_page_in_block[0] == 0xFF)
    {
        return 1;
    }
    return 0;
}

/*!
    \brief  给boot区或者FAT区和Root Dir区重新分配一个块
*/
static uint16_t ftl_alloc_critical_block(uint32_t addr)
{
    uint16_t block_no = addr / NAND_PAGES_PER_BLOCK;
    uint8_t i, j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < CRITICAL_BLOCKS; j++)
        {
            if (block_no == critical_blocks[i][j])
            {
                j++;
                return critical_blocks[i][j%CRITICAL_BLOCKS];
            }
        }
    }
    return 0;
}






/*!
    \brief 保存ftl描述符部分内容，按需调用
*/
void ftl_save_descriptor(uint16_t block_no)
{
    uint32_t addr = block_no*NAND_PAGES_PER_BLOCK;
    uint8_t* offset = (uint8_t*)&ftl.logical_to_phy[0];
    // step 1. 先擦除块
    nand_flash_erase_block(addr);
    // step 2. 写入描述符s
    nand_flash_write_page(addr, PROGRAM_LOAD_x4_CMD, offset, LOGICAL_BLOCKS);
    nand_flash_write_page(addr+1, PROGRAM_LOAD_x4_CMD, offset+LOGICAL_BLOCKS, LOGICAL_BLOCKS);
    nand_flash_write_page(addr+2, PROGRAM_LOAD_x4_CMD, (uint8_t *)&ftl.last_write_page_in_block[0], LOGICAL_BLOCKS);
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

/*!
    \brief  FTL和nand初始化
*/
void ftl_init(void)
{
    // step 1. 初始化nand flash
    nand_flash_initialize();


    // step 2. 加载ftl描述
    uint8_t load_status = ftl_load_descriptor(FTL_DESC_BLOCK);

    // step 3. 构建坏块表
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
            if (load_status)
            {
                // 没有写入过ftl 描述表，需要初始化
                ftl.logical_to_phy[valid_blocks] = block;
                valid_blocks++;
            }
        }
    }
    
    if (load_status)
    {
        ftl_save_descriptor(FTL_DESC_BLOCK);
    }
    
    // step 4. 初始化替换池
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
    \brief  标记Boot区、FAT区和ROOT_DIR区域，仅仅在f_mkdir第一次格式化，并且成功挂在后调用
*/
void ftl_mark_critical_regions(uint32_t boot_start_page, uint32_t fat_start_page, uint32_t root_dir_start_page) 
{
    // 拷贝引导区、fat区、根目录区信息，并转换为块地址
    ftl.boot_block = boot_start_page / NAND_PAGES_PER_BLOCK;
    ftl.fat_block = fat_start_page / NAND_PAGES_PER_BLOCK;
    ftl.root_dir_block = root_dir_start_page / NAND_PAGES_PER_BLOCK;

    // 标记引导区
    uint32_t phy_block = ftl.logical_to_phy[ftl.boot_block];
    ftl_read_block_meta(phy_block * NAND_PAGES_PER_BLOCK, &boot_meta);
    boot_meta.region_type = REGION_BOOT; // 引导区
    ftl_write_block_meta(phy_block * NAND_PAGES_PER_BLOCK, &boot_meta);

    // 标记FAT区
    phy_block = ftl.logical_to_phy[ftl.fat_block];
    for (uint32_t i = 0; i < root_dir_start_page - fat_start_page; i++) 
    {
        ftl_read_block_meta(phy_block * NAND_PAGES_PER_BLOCK + i, &fat_meta);
        fat_meta.region_type = REGION_FAT; // FAT区
        ftl_write_block_meta(phy_block * NAND_PAGES_PER_BLOCK + i, &fat_meta);
    }

    // 标记根目录区
    phy_block = ftl.logical_to_phy[ftl.root_dir_block];
    ftl_read_block_meta(phy_block * NAND_PAGES_PER_BLOCK + (root_dir_start_page & 0x3F), &rootdir_meta);
    rootdir_meta.region_type = REGION_ROOT_DIR; // 根目录区
    ftl_write_block_meta(phy_block * NAND_PAGES_PER_BLOCK + (root_dir_start_page & 0x3F), &rootdir_meta);
}

/*!
    \brief  指定引导区和FAT区和根目录区的备用块, 在Fatfs挂在成功后调用
*/
void ftl_init_critical_spare_block(uint32_t boot_start_page, uint32_t fat_start_page, uint32_t root_dir_start_page)
{
    // 拷贝引导区、fat区、根目录区信息，并转换为块地址
    ftl.boot_block = boot_start_page / NAND_PAGES_PER_BLOCK;
    ftl.fat_block = fat_start_page / NAND_PAGES_PER_BLOCK;
    ftl.root_dir_block = root_dir_start_page / NAND_PAGES_PER_BLOCK;

    critical_blocks[0][0] = boot_start_page / NAND_PAGES_PER_BLOCK;
    // 引导区原本的块数是偶数
    if (critical_blocks[0][0] % 2 == 0)
    {
        critical_blocks[0][1] = ftl.spare_blocks[0];
    }
    else
    {
        critical_blocks[0][1] = ftl.spare_blocks[1];
    }

    critical_blocks[1][0] = fat_start_page / NAND_PAGES_PER_BLOCK;
    if (critical_blocks[1][0] % 2 == 0)
    {
        critical_blocks[1][1] = ftl.spare_blocks[2];
    }
    else
    {
        critical_blocks[1][1] = ftl.spare_blocks[3];
    }
}



/*!
    \brief  boot,FAT,Root Dir专用写入函数
*/
uint8_t ftl_write_critical(uint32_t sector, uint8_t* data, uint8_t region_type)
{
    uint32_t phy_addr = ftl_convert_sector(sector);
    block_meta_data_t meta;
    ftl_read_block_meta(phy_addr, &meta);
	uint8_t em[20];
    // 关键区域块若需擦除，优先使用高耐久备用块
    if (meta.page_used == 0x01) { 
        uint16_t new_block = ftl_alloc_critical_block(phy_addr);
        nand_flash_internal_block_move(phy_addr / BLOCK_SIZE, new_block, phy_addr & 0x3F);
        phy_addr = new_block * BLOCK_SIZE + (phy_addr % BLOCK_SIZE);
        ftl.logical_to_phy[sector / BLOCK_SIZE] = new_block;
    }
    nand_flash_read_page_from_cache(phy_addr, READ_CACHE_QUAD_CMD, em, 20);
    meta.bad_block_marker = 0xFF;
    meta.page_used = 0x01;
    meta.erase_count++;
    meta.flags = 0x01;
    meta.region_type = region_type;
    nand_flash_write_page_spare(phy_addr, (uint8_t*)&meta, sizeof(block_meta_data_t));
    uint8_t ret = nand_flash_write_page(phy_addr, PROGRAM_LOAD_x4_CMD, data, PAGE_SIZE);
	// nand_flash_read_page_from_cache(phy_addr, READ_CACHE_QUAD_CMD, em, 20);
    
    return ret;
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
        if (meta.region_type == 1 || meta.region_type == 2)
            continue;
        else if (meta.region_type == 0) // 数据区域直接擦除，不需要迁移
            nand_flash_erase_block(addr);
        
        if (meta.valid_pages < min_valid)
        {
            min_valid = meta.valid_pages;
            victim_block = i;
        }
    }

    // uint16_t old_block = ftl->logical_to_phy[victim_block];

    // // old_block地址是Fatfs保留区和FAT区域以及root directory区，此时不需要迁移
    // if (old_block == 0 || old_block == 1)
    // {
    //     return 0;
    // }

    // // step 2.分配新块
    // if (ftl->spare_blocks[0] == 0xFFFF) return 1; // 无可用替换块
    // uint16_t new_block = ftl->spare_blocks[1];
    // memmove(&ftl->spare_blocks[1], ftl->spare_blocks+3, (SPARE_BLOCKS-1)*2);
    // block_meta_data_t meta;
    // // step 3. 迁移有效数据到备用区
    // if (nand_flash_internal_block_move(old_block, new_block))
    // {
    //     uint8_t res;
    //     res = 1;
    // }

    

    // step 4. 
    // block_meta_data_t meta = {
    //     .erase_count = 1,
    //     .logical_block = victim_block,
    //     .valid_pages = NAND_PAGES_PER_BLOCK,
    //     .page_used = 0x01,
    //     .flags = 0
    // };
    // ftl_write_block_meta(new_block, &meta);

    // step 5. 擦除旧块
    // nand_flash_erase_block(old_block*BLOCK_SIZE);
    // ftl->logical_to_phy[victim_block] = new_block;
    
    //  // 关键区域专用回收策略（仅当块损坏时触发）
    //  for (uint16_t i = 0; i < CRITICAL_SPARE_BLOCKS; i++) {
    //     if (ftl->critical_spare_blocks[i] == 0xFFFF) continue;
    //     block_meta_data_t meta;
    //     ftl_read_block_meta(ftl->critical_spare_blocks[i] * BLOCK_SIZE, &meta);
    //     if (meta.bad_block_marker != 0xFF) {
    //         replace_critical_block(ftl->critical_spare_blocks[i]);
    //     }
    // }
    return 0;
}




// 写数据区
uint8_t ftl_write_page(uint32_t sector, uint8_t* pbuff)
{
    uint32_t block_base = sector / NAND_PAGES_PER_BLOCK;
    uint8_t  page = sector % NAND_PAGES_PER_BLOCK;
    block_meta_data_t block_meta = {0};
	uint8_t tem[20];

    uint32_t phy_addr = ftl_convert_sector(sector);

    // step 1. 检查该块是否格式化了？
    uint8_t format_flag = ftl_is_block_formatted(block_base, &block_meta);
    if (format_flag != 0x01)
    {
        // 若擦过了，就不擦除了
        if (format_flag != 0xFF)   
        {
            // 该页有数据，先擦除
            nand_flash_erase_block(phy_addr);
            block_meta.erase_count++;
        }
        
        // 写入元数据
        block_meta.erase_count = 1;
        block_meta.bad_block_marker = 0xFF;
        block_meta.valid_pages = NAND_PAGES_PER_BLOCK-1;
        block_meta.page_used = 0x01;
        block_meta.flags = 0x01;
        block_meta.region_type = 0xFF;

        // if (page != 0)
        // {
        //     //写入页不是第一页
        //     ftl_write_block_meta(phy_addr, &block_meta);
		// 	block_meta.page_used = 0xFF;
		// 	block_meta.valid_pages = NAND_PAGES_PER_BLOCK-2;
        // }
        // 在首页写入该block_base的元数据
        ftl.last_write_page_in_block[block_base] = page;    //记录最后一次操作的页码，该页记录最新的元数据
        ftl_write_block_meta(phy_addr, &block_meta);
        nand_flash_write_page(phy_addr, PROGRAM_LOAD_x4_CMD, pbuff, NAND_PAGE_SIZE);
        return 0;
    }


    // step 2. 检查要写入的页是否写过了，若写过，则擦除块
    block_meta_data_t page_meta = {0};
    ftl_read_block_meta(phy_addr, &page_meta);
    if (page_meta.page_used == 0x01)
    {
        nand_flash_erase_block(phy_addr);
        block_meta.erase_count++;
    }


    // step 2. 更新元数据
    block_meta.valid_pages -= page;
    block_meta.page_used = 0x01;
    ftl_write_block_meta(phy_addr, &block_meta);
    ftl.last_write_page_in_block[block_base] = page;    //记录最后一次操作的页码，该页记录最新的元数据

    // step 3. 写入数据。坏块管理？
    nand_flash_write_page(phy_addr, PROGRAM_LOAD_RANDOM_DATA_x4_CMD, pbuff, NAND_PAGE_SIZE);
    // nand_flash_read_page_from_cache(phy_addr, READ_CACHE_QUAD_CMD, tem, 20);
    return 0;
}

uint8_t ftl_read_page(uint32_t sector, uint8_t* pbuff)
{
    uint32_t phy_addr = ftl_convert_sector(sector);
    // 坏块管理？
    return nand_flash_read_page_from_cache(phy_addr, READ_CACHE_QUAD_CMD, pbuff, NAND_PAGE_SIZE);
}


uint8_t ftl_identify_region(uint32_t sector)
{
    uint8_t region_type = REGION_DATA;
    uint16_t src_block = sector / NAND_PAGES_PER_BLOCK;
    if (src_block == ftl.boot_block) region_type = REGION_BOOT;
    else if (src_block == ftl.fat_block) region_type = REGION_FAT;
    else if (src_block == ftl.root_dir_block) region_type = REGION_ROOT_DIR;
    
    return region_type;
}

// uint8_t ftl_is_root_region(uint32_t sector)
// {
//     block_meta_data_t meta;
//     nand_flash_read_page_spare(sector, (uint8_t *)&meta, sizeof(block_meta_data_t));

//     return (meta.region_type == REGION_BOOT) ? 1 : 0;
// }

// uint8_t ftl_is_fat_region(uint32_t sector)
// {
//     block_meta_data_t meta;
//     nand_flash_read_page_spare(sector, (uint8_t *)&meta, sizeof(block_meta_data_t));

//     return (meta.region_type == REGION_FAT) ? 1 : 0;
// }

// uint8_t ftl_is_rootdir_region(uint32_t sector)
// {
//     block_meta_data_t meta;
//     nand_flash_read_page_spare(sector, (uint8_t *)&meta, sizeof(block_meta_data_t));

//     return (meta.region_type == REGION_ROOT_DIR) ? 1 : 0;
// }






double_buffer_t dbuf;
void ftl_init_double_buffer(void)
{
    dbuf.max_lba = LOGICAL_BLOCKS * NAND_PAGES_PER_BLOCK - 1;
    dbuf.next_lba = 0;
    dbuf.active_buf = 0;
    dbuf.buf_ready[1] = 0;
    
    // 预加载第一个缓冲区
    ftl_read_page(0x80, dbuf.buffer[0]);
    dbuf.buf_ready[0] = 1;
    dbuf.current_lba = 0x80;
    dbuf.next_lba = dbuf.current_lba + 1;  
}

int8_t ftl_read_data(uint8_t lun, uint8_t *buf, uint32_t lba, uint32_t len)
{
    // ---- 第1步：检查请求是否在缓存范围内 ----
    if (lba >= dbuf.current_lba && 
        lba <= (dbuf.current_lba + 1)) 
    {
      // 命中缓存：直接从活动缓冲区拷贝数据
      memcpy(buf, &dbuf.buffer[dbuf.active_buf][0], len * NAND_PAGE_SIZE);
      if (dbuf.buffer[dbuf.active_buf][0] == 0xFF)
      {
          // 读取数据失败
          return -1;
      }
      dbuf.buf_ready[dbuf.active_buf] = 0; // 清除就绪标志
      dbuf.active_buf = 1 - dbuf.active_buf; // 切换活动缓冲区
      dbuf.current_lba = lba + 1;
      dbuf.next_lba = dbuf.current_lba + 1;

      
      return 0; // 成功
    }
    else
    {
        
    }

}