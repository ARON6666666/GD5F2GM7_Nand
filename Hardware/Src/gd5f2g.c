#include "gd5f2g.h"
#include "stm32wbxx_hal.h"

/*！
	\brief 定义GD5F2GM7的ID, 
	       C8是厂商ID，
	       DID1是设备ID
*/
static uint8_t MID = 0xC8;
#if defined GD5F2GM7UExxG
static uint8_t DID1 = 0x92;
#elif defined GD5F2GM7RExxG
static uint8_t DID1 = 0x82;
#endif


extern QSPI_HandleTypeDef hqspi;
uint8_t param_page[128] = {0};
uint8_t UID[32] = {0};

/* Private function prototypes -----------------------------------------------*/
static uint8_t nand_flash_get_status(uint8_t status_reg_addr);
static uint8_t nand_flash_clear_feature_reg(uint8_t feature);
static uint8_t nand_flash_set_feature_reg(uint8_t feature);
static uint8_t nand_flash_read_feature_reg(void);
static uint8_t nand_flash_get_protection_reg(void);
static uint8_t nand_flash_set_protection_reg(uint8_t reg_val);
static uint8_t nand_flash_get_id(uint16_t* id);
static uint8_t nand_flash_get_param_page(uint8_t*);
static uint8_t nand_flash_get_check_uid(uint8_t*);
static uint8_t nand_flash_page_read(uint32_t column_addr);




/*！
	\brief 读取GD5F2GM7的状态寄存器
	\param[in] status_reg_addr -- 状态寄存器地址
	\param[in] mask_bit -- 状态寄存器掩码
	\param[out] None
	\retval 0 
	\version 0.0.1
*/
static uint8_t nand_flash_get_status(uint8_t status_reg_addr)
{
	QSPI_CommandTypeDef s_cmd = {0};
	uint8_t status = 0;
	s_cmd.Instruction = GET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;

	s_cmd.Address = status_reg_addr;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Receive(&hqspi, &status, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);


	return status;
}

/*!
	\brief 读取GD5F2GM7的feature寄存器
	\param[in] None
	\param[out] None
	\retval feature寄存器配置值
	\version 0.0.1
*/
static uint8_t nand_flash_read_feature_reg(void)
{
	uint8_t reg_val = 0;
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = GET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_FEATURES1;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	HAL_QSPI_Receive(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	return reg_val;
}

/*!
	\brief 设置GD5F2GM7 feature 寄存器
	\param[in] feature -- feature寄存器配置值
	\param[out] None
	\retval 0 -- 设置成功 1 -- 设置失败
	\version 0.0.1
*/
static uint8_t nand_flash_set_feature_reg(uint8_t feature)
{
	QSPI_CommandTypeDef s_cmd = {0};
	uint8_t reg_val = 0;
	uint8_t ret = 1;
	/*
		feature1寄存器
		bit        7         6        5		  4       3       2         1        0 
	     	| OTP_PRT | OTP_EN | RESERVED | ECC_EN | BPL | RESERVED | RESERVED | QE |
	*/
	// 获取features1寄存器值
	reg_val = nand_flash_read_feature_reg();
	reg_val |= feature;

	// 设置features1寄存器
	s_cmd.Instruction = SET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_FEATURES1;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Transmit(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	// 回读
	reg_val = 0;
	reg_val = nand_flash_read_feature_reg();
	if ((reg_val & feature) == feature)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}
	return ret;
}

/*!
	\brief 设置GD5F2GM7 feature 寄存器
	\param[in] feature -- feature寄存器配置值
	\param[out] None
	\retval 0 -- 设置成功 1 -- 设置失败
	\version 0.0.1
*/
static uint8_t nand_flash_clear_feature_reg(uint8_t feature)
{
	QSPI_CommandTypeDef s_cmd = {0};
	uint8_t reg_val = 0;
	uint8_t ret = 1;
	/*
		feature1寄存器
		bit        7         6        5		  4       3       2         1        0 
	     	| OTP_PRT | OTP_EN | RESERVED | ECC_EN | BPL | RESERVED | RESERVED | QE |
	*/
	// 获取features1寄存器值
	reg_val = nand_flash_read_feature_reg();
	reg_val &= ~feature;

	// 设置features1寄存器
	s_cmd.Instruction = SET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_FEATURES1;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Transmit(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	// 回读
	reg_val = 0;
	reg_val = nand_flash_read_feature_reg();
	if ((reg_val & feature) == 0)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}
	return ret;
}

/*!
	\brief 获取GD5F2GM7的protection寄存器
	\param[in] None
	\param[out] None
	\retval protection寄存器配置值
	\version 0.0.1
*/
static uint8_t nand_flash_get_protection_reg(void)
{
	QSPI_CommandTypeDef s_cmd = {0};
	uint8_t reg_val = 0;
	s_cmd.Instruction = GET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_PROTECTION;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Receive(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	return reg_val;
}

/*!
	\brief 设置GD5F2GM7的protection寄存器
	\param[in] reg_val -- protection寄存器配置值
	\param[out] None
	\retval 0 -- 设置成功 1 -- 设置失败
	\version 0.0.1
*/
static uint8_t nand_flash_set_protection_reg(uint8_t reg_val)
{
	QSPI_CommandTypeDef s_cmd = {0};
	uint8_t ret = 1;
	s_cmd.Instruction = SET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_PROTECTION;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Transmit(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	if (nand_flash_get_protection_reg() == reg_val)
	{
		ret = 0;
	}

	s_cmd.Instruction = GET_FEATURES_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = REG_STATUS2;
	s_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = 1;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Receive(&hqspi, &reg_val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	return ret;
}

/*!
	\brief 获取GD5F2GM7的ID
	\param[in] None
	\param[out] id -- 读取到的ID
	\retval 0 -- 读取成功 1 -- 读取失败
	\version 0.0.1
*/
static uint8_t nand_flash_get_id(uint16_t* id)
{
	uint8_t ret = 0;
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = READ_ID_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.DummyCycles = 8;
	s_cmd.NbData = 2;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	s_cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	if (HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}

	// 接收ID
	if (HAL_QSPI_Receive(&hqspi, (uint8_t *)id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	return ret;
}

static uint8_t nand_flash_get_check_uid(uint8_t* uid)
{
	uint8_t ret = 1;
	QSPI_CommandTypeDef s_cmd = {0};

	// 设置OTP_EN
	if (nand_flash_OTP_Enable())
	{
		nand_flash_OTP_Disable();
		return ret;
	}

	// 读取UID
	if (nand_flash_page_read(ADDR_UID))
	{
		return ret;
	}
	s_cmd.Instruction = READ_CACHE_x1_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = ADDR_UID;
	s_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.AlternateBytes = 0;
	s_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	s_cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_1_LINE;

	s_cmd.NbData = 32;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	HAL_QSPI_Receive_DMA(&hqspi, uid);
	HAL_Delay(1);

	nand_flash_OTP_Disable();
	// 检查UID 
	for (int i = 0; i < 16; i++)
	{
		if ((uid[i] ^ uid[i + 16]) != 0xFF)
		{
			return ret;
		}
	}
	return 0;
}

/*!
	\brief 获取GD5F2GM7的参数配置的前128字节
	\param[in] None
	\param[out] param_buff -- 读取到的参数配置
	\retval 0 -- 读取成功 1 -- 读取失败
	\version 0.0.1
*/
static uint8_t nand_flash_get_param_page(uint8_t* param_buff)
{
	uint8_t ret = 1;
	uint8_t dummy = 0;
	QSPI_CommandTypeDef s_cmd = {0};

	// 设置OTP_EN
	if (nand_flash_OTP_Enable())
	{
		nand_flash_OTP_Disable();
		return ret;
	}

	// 读取PARAM_PAGE
	if (nand_flash_page_read(ADDR_PARAM_PAGE))
	{
		return 1;
	}

	s_cmd.Instruction = READ_CACHE_x1_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = 0;
	s_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;


	s_cmd.AlternateBytes = 0;
	s_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	s_cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_1_LINE;

	s_cmd.NbData = 128;
	s_cmd.DataMode = QSPI_DATA_1_LINE;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_QSPI_Receive_DMA(&hqspi, param_buff);
	HAL_Delay(2);
	nand_flash_OTP_Disable();
	
	return 0;
}



/*!
	\brief GD5F2GM7 把数据搬到Cache Registers
	\param[in] column_addr -- 列起始地址，每个block的Page首址
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
static uint8_t nand_flash_page_read(uint32_t column_addr)
{
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = PAGE_READ_TOCACHE_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;

	s_cmd.Address = column_addr;
	s_cmd.AddressSize = QSPI_ADDRESS_24_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;

	s_cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	
	// 查询状态
	if (nand_flash_status_reg_check_bit(0x00))
	{
		return 1;
	}
	
	return 0;
}

/*!
	\brief GD5F2GM7 使能OTP Region 读写
	\param[in] None
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_OTP_Enable(void)
{
	return nand_flash_set_feature_reg(OTP_REGION_EN_CMD);
}

/*!
	\brief GD5F2GM7 关闭OTP Region 读写
	\param[in] None
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_OTP_Disable(void)
{
	nand_flash_clear_feature_reg(OTP_REGION_EN_CMD);
	return nand_flash_set_feature_reg(OTP_REGION_PRT_CMD);
}

/*!
	\brief GD5F2GM7 检查状态寄存器bit位
	\param[in] mask_bit -- 状态寄存器掩码
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_status_reg_check_bit(uint8_t mask_bit)
{
	uint8_t ret = 1;
	uint8_t retry = 255;
	uint8_t status = 0;

	if (mask_bit == IN_OP)
	{
		do 
		{
			status = nand_flash_get_status(REG_STATUS1);
			if ((status & 0x01) == 0)
			{
				ret = 0;
				break;
			}
		} while (retry--);
	}
	else if (mask_bit == WEL)
	{
		do 
		{
			status = nand_flash_get_status(REG_STATUS1);
			if ((status & mask_bit) != 0)
			{
				ret = 0;
				break;
			}
		} while (retry--);
	}
	else
	{
		do 
		{
			status = nand_flash_get_status(REG_STATUS1);
			if ((status & 0x01) == 0 && (status & mask_bit) == 0)
			{
				ret = 0;
				break;
			}
			else if ((status & mask_bit) != 0) 	// Error Occur
			{
				ret = 1;
				break;
			}
		} while (retry--);
	}
	
	return ret;
}

/*!
	\brief GD5F2GM7的软复位, 使Device进入IDLE状态
	\param[in] None
	\param[out] None
	\retval None
	\version 0.0.1
*/
void nand_flash_softreset(void)
{
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = RESET_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_Delay(1);
}

/*!
	\brief GD5F2GM7的上电复位
	\param[in] None
	\param[out] None
	\retval None
	\version 0.0.1
*/
void nand_flash_poweronreset(void)
{
	QSPI_CommandTypeDef s_cmd = {0};
	
	s_cmd.Instruction = ENABLE_POWER_ON_RESET_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	
	s_cmd.Instruction = POWER_ON_RESET_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;	
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	HAL_Delay(2);
}





/*!
	\brief 初始化GD5F2GM7
	\param[in]
	\param[out]
	\retval 0 -- 初始化成功 1 -- 初始化失败
	\version 0.0.1
*/
uint8_t nand_flash_initialize(void)
{
	uint8_t ret = 1;
	QSPI_CommandTypeDef s_cmd = {0};

	union 
	{
		uint8_t byte[2];
		uint16_t word;
	} id = {0};

	
	// 复位
	nand_flash_poweronreset();

	// 获取ID
	nand_flash_get_id(&id.word);
	if (id.byte[0] == MID && id.byte[1] == DID1)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}


	// 设置feature
	nand_flash_set_feature_reg(QE_BIT);

	// 获取UID
	nand_flash_get_check_uid(&UID[0]);

	// 获取param page
	nand_flash_get_param_page(param_page);
	
	// 设置PROTECTION
	// uint8_t temp_val = protection_reg_val | 0x80;
	nand_flash_set_protection_reg(0x00);
	return ret;
}

/*!
	\brief GD5F2GM7 写使能 (Program Load)
	\param[in] None
	\param[out] None
	\retval None
	\version 0.0.1
*/
void nand_flash_write_enable(void)
{
	uint8_t status = 0, a,b =2;
	QSPI_CommandTypeDef s_cmd = {0};
	
	while(HAL_QSPI_GetState(&hqspi) != HAL_QSPI_STATE_READY)
	{
		;
	}
	s_cmd.Instruction = WRITE_ENABLE_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	if (nand_flash_status_reg_check_bit(WEL))
	{
		return;
	}
}

/*!
	\brief GD5F2GM7 写禁止 (Program Load)
	\param[in] None
	\param[out] None
	\retval None
	\version 0.0.1
*/
void nand_flash_write_disable(void)
{
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = WRITE_DISABLE_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

/*!
	\brief GD5F2GM7 开始写入
	\param[in] addr -- 写入的地址
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_start_program(uint32_t addr)
{
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = PROGRAM_EXE_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = addr;
	s_cmd.AddressSize = QSPI_ADDRESS_24_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	// 查询状态
	if (nand_flash_status_reg_check_bit(P_FAIL))
	{
		return 1;
	}
	
	return 0;
}

/*!
	\brief GD5F2GM7 擦除一个block
	\param[in] raw_addr -- block首地址
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_erase_block(uint32_t raw_addr)
{
	QSPI_CommandTypeDef s_cmd = {0};

	nand_flash_write_enable();

	s_cmd.Instruction = BLOCK_ERASE_CMD;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_cmd.Address = raw_addr;
	s_cmd.AddressSize = QSPI_ADDRESS_24_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	// 查询状态,一般3ms擦一个block
	if (nand_flash_status_reg_check_bit(E_FAIL))
	{
		return 1;
	}

	return 0;
} 

/*!
	\brief GD5F2GM7 写一页数据
	\param[in] addr -- 写入的地址
	\param[in] prog_cmd -- 写入命令
	\param[in] len -- 写入的长度, 一般为一个页的大小
	\param[in] pbuff -- 写入的数据
	\param[out] None
	\retval 0 -- 操作成功 1 -- 操作失败
	\version 0.0.1
*/
uint8_t nand_flash_write_page(uint32_t addr, uint8_t prog_cmd, uint8_t *pbuff, uint32_t len)
{
	QSPI_CommandTypeDef s_cmd = {0};

	s_cmd.Instruction = prog_cmd;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;

	s_cmd.Address = 0;
	s_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	s_cmd.NbData = len;
	if (prog_cmd == PROGRAM_LOAD_CMD) {
		s_cmd.DataMode = QSPI_DATA_1_LINE;
	} else if (prog_cmd == PROGRAM_LOAD_x4_CMD) {
		s_cmd.DataMode = QSPI_DATA_4_LINES;
	} 
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
#ifdef USE_QSPI_DMA
	HAL_QSPI_Transmit_DMA(&hqspi, pbuff);
#elif USE_QSPI_IT
	HAL_QSPI_Transmit_IT(&hqspi, pbuff);
#else
	HAL_QSPI_Transmit(&hqspi, pbuff, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
#endif
	nand_flash_write_enable();

	if (nand_flash_start_program(addr))
	{
		nand_flash_write_disable();
		return 1;
	}
	nand_flash_write_disable();
	return 0;
}


/*!
	\brief 读取GD5F2GM7的一页数据
	\param[in] addr -- 读取的地址
	\param[in] rd_cache_cmd -- 读取命令
	\param[in] len -- 读取的长度, 一般为一个页的大小
	\param[out] pbuff -- 读取的数据
	\retval 0 -- 读取成功 1 -- 读取失败
	\version 0.0.1
*/
uint8_t nand_flash_read_page_from_cache(uint32_t addr, uint8_t rd_cache_cmd, uint8_t *pbuff, uint32_t len)
{
	QSPI_CommandTypeDef s_cmd = {0};


	// 读取数据到Cache Registers
	if (nand_flash_page_read(addr))
	{
		return 1;
	}

	s_cmd.Instruction = rd_cache_cmd;
	s_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;

	// zhe里的地址是数据在2KB Cache Registers中的偏移地址
	// 一般从零开始读取len个字节就OK
	s_cmd.Address = 0;
	s_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	s_cmd.NbData = len;
	
	s_cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	switch (rd_cache_cmd)
	{
		case READ_CACHE_x1_CMD:
			s_cmd.DummyCycles = 8;
			s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
			s_cmd.DataMode = QSPI_DATA_1_LINE;
			break;

		case READ_CACHE_x2_CMD:
			s_cmd.DummyCycles = 8;
			s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
			s_cmd.DataMode = QSPI_DATA_2_LINES;
			break;

		case READ_CACHE_x4_CMD:
			s_cmd.DummyCycles = 8;
			s_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
			s_cmd.DataMode = QSPI_DATA_4_LINES;
			break;

		case READ_CACHE_DUAL_CMD:
			s_cmd.DummyCycles = 4;
			s_cmd.AddressMode = QSPI_DATA_2_LINES;
			s_cmd.DataMode = QSPI_DATA_2_LINES;
			break;

		case READ_CACHE_QUAD_CMD:
			s_cmd.DummyCycles = 4;
			s_cmd.AddressMode = QSPI_ADDRESS_4_LINES;
			s_cmd.DataMode = QSPI_DATA_4_LINES;
			break;

		case READ_CACHE_QUADDTR_CMD:
			s_cmd.DummyCycles = 8;
			s_cmd.AddressMode = QSPI_ADDRESS_4_LINES;
			s_cmd.DataMode = QSPI_DATA_4_LINES;
			s_cmd.DdrMode = QSPI_DDR_MODE_ENABLE;
			break;
		default:
			break;
	}
	HAL_QSPI_Command(&hqspi, &s_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
#ifdef USE_QSPI_DMA
	HAL_QSPI_Receive_DMA(&hqspi, (uint8_t *)pbuff);
#elif USE_QSPI_IT
	HAL_QSPI_Receive_IT(&hqspi, (uint8_t *)pbuff);
#else
	HAL_QSPI_Receive(&hqspi, (uint8_t *)pbuff, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
#endif
	

	return 0;
}





uint8_t nand_flash_internal_data_move()
{
	return 0;
}

uint8_t nand_flash_write_page_random(uint32_t addr, uint8_t prog_cmd, uint8_t *pbuff)
{
	return 0;
}