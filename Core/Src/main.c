/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "memorymap.h"
#include "quadspi.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nand_flash.h"
#include "nand_ftl.h"
#include "ff.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */
//
uint16_t bad_block;
uint8_t introduction[PAGE_SIZE] = "The GD5F2GM7 NAND Flash memory is a high-performance, ";

void nand_flash_test()
{
	// 擦除
//	if (nand_flash_erase_block(0x78))
//	{
//		return;
//	}
    // for (uint16_t blk = 0; blk < BLOCK_COUNT; blk++)
    // {
    //   if (nand_flash_bad_block_check(blk*BLOCK_SIZE) != 0xFF)
    //   {
    //     bad_block++;
    //   }
    // }


      // block_meta_data_t meta;
      // nand_flash_write_page_spare(0, (uint8_t *)&meta, sizeof(block_meta_data_t));
      // nand_flash_read_page_spare(0, (uint8_t *)&meta, sizeof(block_meta_data_t));
      // nand_flash_erase_block(0);
      // memset(&meta, 0, sizeof(block_meta_data_t));
      // nand_flash_read_page_spare(0, (uint8_t *)&meta, sizeof(block_meta_data_t));

  // memset(introduction, 'A', PAGE_SIZE);
	// introduction[2] = 'A';
	// // 写入
	// //nand_flash_write_page(0x78, PROGRAM_LOAD_x4_CMD, introduction, 2048);
	// if (nand_flash_write_multi_page(0x00, introduction, BLOCK_SIZE))
	// {
	// 	return;
	// }
	

  // // internal data move. parity attribute ?  0 <-> 1 no 0 <-> 2 yes
  // if (nand_flash_internal_block_move(0x00, 0x02) != BLOCK_SIZE)
  // {
  //   return;
  // }

  // // 读取ECC
  // uint8_t ecc_buff[BLOCK_SIZE] = {0};

  // for (uint8_t i = 0; i < BLOCK_SIZE; i++)
  // {
  //   nand_flash_read_page_ecc(i, ecc_buff);
  // }
    

	memset(introduction, 0, 2048);
	// // 回读
	// if (nand_flash_read_page_from_cache(0x78, READ_CACHE_QUAD_CMD, introduction, 2048))
	// {
	// 	return;
	// }
	
	introduction[1] = 'B';
	if (nand_flash_write_multi_page(0x0, introduction, 1))
	{
		return;
	}
  
	memset(introduction, 0, 2048);
	// 回读
	if (nand_flash_read_page_from_cache(0x0, READ_CACHE_QUAD_CMD, introduction, 2048))
	{
		return;
	}
}


FATFS fs;
FIL file;

BYTE formatWorkBuff[2048];
// BYTE buffer[2048];
char filepatah[] = "0:ts.txt";
uint32_t resbyte; 
void fatfs_test()
{
	FRESULT res;
  res = f_mount(&fs, "0:", 1);

  if (res == FR_NO_FILESYSTEM || res == FR_DISK_ERR)
  {
    
	//memset(formatWorkBuff, 0xff, 2048);
    res = f_mkfs("0:", NULL, formatWorkBuff, 2048);

    res = f_mount(NULL, "0:", 1);
    res = f_mount(&fs, "0:", 1);
  }
	res = f_open(&file, filepatah, FA_CREATE_ALWAYS | FA_WRITE |FA_READ);

  res = f_write(&file, "Hello World!", 12, &resbyte);
//  res = f_lseek(&file, 0);
//  res = f_read(&file, buffer, 30, &resbyte);

  res = f_close(&file);
  
  res = f_open(&file, filepatah, FA_READ);

}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_QUADSPI_Init();
  // nand_flash_initialize();
  // nand_flash_test();
  ftl_init();
  fatfs_test();

  // MX_USB_Device_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // ftl_garbage_collect();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3)
  {
  }

  /* HSE configuration and activation */
  LL_RCC_HSE_Enable();
  while(LL_RCC_HSE_IsReady() != 1)
  {
  }

  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_2, 8, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  /* Sysclk activation on the main PLL */
  /* Set CPU1 prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Set CPU2 prescaler*/
  LL_C2_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Set AHB SHARED prescaler*/
  LL_RCC_SetAHB4Prescaler(LL_RCC_SYSCLK_DIV_1);

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  /* Set APB2 prescaler*/
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  LL_RCC_PLLSAI1_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_2, 6, LL_RCC_PLLSAI1Q_DIV_2);
  LL_RCC_PLLSAI1_Enable();
  LL_RCC_PLLSAI1_EnableDomain_48M();

  /* Wait till PLLSAI1 is ready */
  while(LL_RCC_PLLSAI1_IsReady() != 1)
  {
  }

  LL_RCC_SetSMPSClockSource(LL_RCC_SMPS_CLKSOURCE_HSE);
  LL_RCC_SetSMPSPrescaler(LL_RCC_SMPS_DIV_0);
  LL_RCC_SetRFWKPClockSource(LL_RCC_RFWKP_CLKSOURCE_NONE);
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
