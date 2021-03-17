/*
 * psram.c
 *
 *  Created on: 2021. 3. 17.
 *      Author: baram
 */


#include "psram.h"

#ifdef _USE_HW_PSRAM




#define PSRAM_MAX_CH              1


#define PSRAM_ADDR_OFFSET         0x90000000
#define PSRAM_MAX_SIZE            (8*1024*1024)
#define PSRAM_SECTOR_SIZE         (4*1024)
#define PSRAM_PAGE_SIZE           (1024)
#define PSRAM_MAX_SECTOR          (FLASH_MAX_SIZE / FLASH_SECTOR_SIZE)



static uint32_t psram_addr   = PSRAM_ADDR_OFFSET;
static uint32_t psram_length = 0;

typedef struct
{
  bool     is_init;
  uint32_t id;
  uint32_t length;

} psram_tbl_t;


static psram_tbl_t psram_tbl[PSRAM_MAX_CH];



static bool psramSetup(void);
static bool psramGetVendorID(uint8_t *vendorId);
static bool psramEnterQPI(void);
static bool psramExitQPI(void);
static bool psramReset(void);
static bool psramEnterMemoyMaped(void);



OSPI_HandleTypeDef hospi1;



bool psramInit(void)
{
  bool ret = true;


  psram_tbl[0].is_init = false;
  psram_tbl[0].length = 0;


  if (psramSetup() == true)
  {
    psram_length += PSRAM_MAX_SIZE;
  }

  return ret;
}

uint32_t psramGetAddr(void)
{
  return psram_addr;
}

uint32_t psramGetLength(void)
{
  return psram_length;
}

bool psramRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  return true;
}

bool psramWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  return true;
}





#define PSRAM_CMD_LUT_SEQ_IDX_ENTERQPI          0x35
#define PSRAM_CMD_LUT_SEQ_IDX_EXITQPI           0xF5
#define PSRAM_CMD_LUT_SEQ_IDX_READID            0x9F
#define PSRAM_CMD_LUT_SEQ_IDX_READ_QPI          0xEB
#define PSRAM_CMD_LUT_SEQ_IDX_WRITE_QPI         0x38
#define PSRAM_CMD_LUT_SEQ_IDX_RESET_ENABLE      0x66
#define PSRAM_CMD_LUT_SEQ_IDX_RESET             0x99


bool psramSetup(void)
{
  bool ret = true;

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};


  hospi1.Instance                 = OCTOSPI1;
  hospi1.Init.FifoThreshold       = 4;
  hospi1.Init.DualQuad            = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType          = HAL_OSPI_MEMTYPE_MICRON;
  hospi1.Init.DeviceSize          = (uint32_t)POSITION_VAL((uint32_t)PSRAM_MAX_SIZE);
  hospi1.Init.ChipSelectHighTime  = 1;
  hospi1.Init.FreeRunningClock    = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode           = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize            = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler      = 2;
  hospi1.Init.SampleShifting      = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary  = 1;
  hospi1.Init.ClkChipSelectHighTime = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    return false;
  }

  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }


  if (psramGetVendorID((uint8_t *)&psram_tbl[0].id) == true)
  {

  }

  psramExitQPI();


//  if (psramReset() != true)
  {
//    return false;
  }

  return ret;
}

bool psramExitQPI(void)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_EXITQPI;
  s_command.AddressMode       = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_NONE;
  s_command.DummyCycles       = 0;
  s_command.NbData            = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


  if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
#else
  OSPI_RegularCmdTypeDef s_command = {0};


  /* Initialize the read command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_EXITQPI;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  s_command.Address            = 0;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0;
  s_command.NbData             = 0;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
#endif

  return ret;
}

bool psramGetVendorID(uint8_t *vendorId)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_READID;
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_1_LINE;
  s_command.DummyCycles       = 0;
  s_command.NbData            = 2;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(&QSPIHandle, vendorId, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
#else
  OSPI_RegularCmdTypeDef s_command = {0};


  /* Initialize the read command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_READID;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  s_command.Address            = 0;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_1_LINE;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0;
  s_command.NbData             = 2;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Reception of the data */
  if (HAL_OSPI_Receive(&hospi1, vendorId, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
#endif

  return ret;
}









#endif
