/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "nand_flash.h"
#include "nand_ftl.h"
#include <string.h>

/* Definitions of physical drive number for each drive */
// #define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
// #define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
// #define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
static uint8_t sector_cache[512];

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = RES_OK;
	stat = STA_NOINIT;
  	stat &= ~STA_NOINIT;
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	// ftl_init();
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	
	// 读取数据
	//res = nand_flash_read_page_from_cache(sector, READ_CACHE_QUAD_CMD, buff, PAGE_SIZE);
	// res = nand_flash_read_multi_page(sector, buff, count);
	for (int i = 0; i < count; i++)
	{
		ftl_read_page(sector, buff + i*SECTOR_SIZE);
	}
	
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
uint8_t test_buff[2048];
#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_OK;
	
	//res =nand_flash_write_page(sector, PROGRAM_LOAD_x4_CMD, buff, PAGE_SIZE);
	// res = nand_flash_write_multi_page(sector, buff, count);
	
	// res = nand_flash_read_page_from_cache(sector, READ_CACHE_QUAD_CMD, test_buff, PAGE_SIZE);
	for (int i = 0; i < count; i++)
	{
		if (ftl_write_page(sector, buff + i*SECTOR_SIZE))
		{
			return RES_ERROR;
		}
	}
	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	// Process of the command for the RAM drive
	switch(cmd)
	{	
	  
	  case CTRL_SYNC :
	    ftl_garbage_collect(get_ftl_obj());
		res = RES_OK;
	  break;	
	  
	  case CTRL_TRIM:
		res = RES_OK;
	  break;
	  
	  case GET_SECTOR_COUNT:
	  {
		DWORD total_sectors = LOGICAL_BLOCKS * NAND_PAGES_PER_BLOCK * SECTORS_PER_PAGE;
		*(int*)buff = total_sectors;
		res = RES_OK;
	  }
	  break;
	  
	  case GET_SECTOR_SIZE:
	  {
		*(int*)buff = SECTOR_SIZE;
		res = RES_OK;
	  }
	  break;
	  
	  case GET_BLOCK_SIZE:
	  {
		*(int*)buff = BLOCK_SIZE;
		res = RES_OK;
	  }
	  break;
	  
	}
  
	return res;
}

