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

/* Definitions of physical drive number for each drive */
// #define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
// #define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
// #define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


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
	res = nand_flash_read_multi_page(sector, buff, count);
	
	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	
	res = nand_flash_write_multi_page(sector, buff, count);


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
	nand_flash_info_t* pNandFlashInfo = 
				(nand_flash_info_t*)&nand_flash_info_buff.param_page[80];

	// Process of the command for the RAM drive
	switch(cmd)
	{	
	  
	  case CTRL_SYNC :
		res = RES_OK;
	  break;	
	  
	  case CTRL_TRIM:
		res = RES_OK;
	  break;
	  
	  case GET_SECTOR_COUNT:
	  {
		*(int*)buff = pNandFlashInfo->block_count*pNandFlashInfo->block_size;
		res = RES_OK;
	  }
	  break;
	  
	  case GET_SECTOR_SIZE:
	  {
		*(int*)buff = pNandFlashInfo->page_size;
		res = RES_OK;
	  }
	  break;
	  
	  case GET_BLOCK_SIZE:
	  {
		*(int*)buff = pNandFlashInfo->block_size;
		res = RES_OK;
	  }
	  break;
	  
	}
  
	return res;
}

