#ifndef _NAND_FLASH_
#define _NAND_FLASH_


#include <stdint.h>


// 定义QSPI数据传输方式
#define USE_QSPI_DMA
// #define USE_QSPI_IT
// #define USE_QSPI_NORMAL


// 定义NAND Flash 型号
#define GD5F2GM7UExxG
// #define GD5F2GM7RExxG

#ifdef GD5F2GM7UExxG
#define PAGE_SIZE							2048
#elif GD5F2GM7RExxG

#endif




// 寄存器地址
#define REG_PROTECTION						0xA0
#define REG_FEATURES1						0xB0
#define REG_STATUS1							0xC0
#define REG_REATRUES2						0xD0
#define REG_STATUS2							0xF0

/* 写权限操作 */ 
#define WRITE_ENABLE_CMD					0x06
#define WRITE_DISABLE_CMD					0x04

/* 配置和状态寄存器读写操作 */
#define GET_FEATURES_CMD					0x0F
#define SET_FEATURES_CMD					0x1F

/* 读操作 */
#define PAGE_READ_TOCACHE_CMD				0x13
#define READ_CACHE_x1_CMD				    0x03 //or 0BH
#define READ_CACHE_x2_CMD					0x3B
#define READ_CACHE_x4_CMD					0x6B
#define READ_CACHE_DUAL_CMD					0xBB
#define READ_CACHE_QUAD_CMD					0xEB
#define READ_CACHE_QUADDTR_CMD				0xEE
#define READ_ID_CMD							0x9F
#define READ_PARAM_PAGE_CMD					0x13
#define READ_UID_CMD						0x13

/* Program 操作 */
#define PROGRAM_LOAD_CMD					0x02
#define PROGRAM_LOAD_x4_CMD					0x32
#define PROGRAM_EXE_CMD						0x10
#define PROGRAM_LOAD_RANDOM_DATA_CMD		0x84
#define PROGRAM_LOAD_RANDOM_DATA_x4_CMD		0xC4 //or 34H

/* Erase 操作 */
#define BLOCK_ERASE_CMD						0xD8

/* RESET 操作 */
#define RESET_CMD							0xFF
#define ENABLE_POWER_ON_RESET_CMD			0x66
#define POWER_ON_RESET_CMD					0x99

/* feature 操作 */
#define QE_BIT								0x01

/* OTP Region 操作 */
#define OTP_REGION_EN_CMD					0x40
#define OTP_REGION_PRT_CMD					0x80

/* 地址管理 */
#define ADDR_UID							0x000000
#define ADDR_PARAM_PAGE						0x000001


enum status_reg_mask_code
{
	IN_OP = 0x00,
	WEL = 0x02,
	E_FAIL = 0x04,
	P_FAIL = 0x08,
	ECCS0 = 0x10,
	ECCS1 = 0x20,
} ;






// gd5f2g function prototype
uint8_t nand_flash_initialize(void);
void nand_flash_softreset(void);
void nand_flash_poweronreset(void);
uint8_t nand_flash_status_reg_check_bit(uint8_t mask_bit);
uint8_t nand_flash_OTP_Enable(void);
uint8_t nand_flash_OTP_Disable(void);
uint8_t nand_flash_erase_block(uint32_t raw_addr);
uint8_t nand_flash_read_page_from_cache(uint32_t addr, uint8_t rd_cache_cmd, uint8_t *pbuff, uint32_t len);
uint8_t nand_flash_write_page(uint32_t column_addr, uint8_t prog_cmd, uint8_t *pbuff, uint32_t len);
#endif
