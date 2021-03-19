/*
 * psram.c
 *
 *  Created on: 2021. 3. 17.
 *      Author: baram
 */


#include "psram.h"

#ifdef _USE_HW_PSRAM
#ifdef _USE_HW_CLI
#include "cli.h"
#endif



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


#ifdef _USE_HW_CLI
static void cliPsram(cli_args_t *args);
#endif


bool psramInit(void)
{
  bool ret = true;


  psram_tbl[0].is_init = false;
  psram_tbl[0].length = 0;


  if (psramSetup() == true)
  {
    psram_length += PSRAM_MAX_SIZE;
  }

  if (psram_length == 0)
  {
    ret = false;
  }
  else
  {
    psram_tbl[0].is_init = false;
    psram_tbl[0].id = 0;
    ret = psramGetVendorID((uint8_t *)&psram_tbl[0].id);

    if (ret == true && psram_tbl[0].id == 0x5D0D)
    {
      psram_tbl[0].is_init = true;
      psram_tbl[0].length = PSRAM_MAX_SIZE;

      psramEnterQPI();
      psramEnterMemoyMaped();
    }
    ret = psram_tbl[0].is_init;
  }

#ifdef _USE_HW_CLI
  cliAdd("psram", cliPsram);
#endif

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
  hospi1.Init.ClockPrescaler      = 3;
  hospi1.Init.SampleShifting      = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary  = 0;
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

  psramExitQPI();


  if (psramReset() != true)
  {
    return false;
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

bool psramReset(void)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_RESET_ENABLE;
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

  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_RESET;
  if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_RESET_ENABLE;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_RESET;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

#endif

  return ret;
}

bool psramEnterQPI(void)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_ENTERQPI;
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
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_ENTERQPI;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0;
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

bool psramEnterMemoyMaped(void)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_READ_QPI;
  s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 6;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_MemoryMapped(&QSPIHandle, &s_command, &s_mem_mapped_cfg) != HAL_OK)
  {
    return QSPI_ERROR;
  }

#else
  OSPI_RegularCmdTypeDef      s_command = {0};
  OSPI_MemoryMappedTypeDef s_mem_mapped_cfg = {0};


  /* Initialize the read command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_READ_QPI;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_4_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 6;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the read command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Initialize the program command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_WRITE_QPI;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_ENABLE;

  /* Send the write command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
  if (HAL_OSPI_MemoryMapped(&hospi1, &s_mem_mapped_cfg) != HAL_OK)
  {
    return false;
  }

#endif
  return ret;
}

bool psramRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_READ_QPI;
  s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 6;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  s_command.Address = addr;
  s_command.NbData  = length;

  if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  if (HAL_QSPI_Receive(&QSPIHandle, p_data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_READ_QPI;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_4_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 6;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  s_command.Address            = addr;
  s_command.NbData             = length;


  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Reception of the data */
  if (HAL_OSPI_Receive(&hospi1, p_data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
#endif
  return ret;
}

bool psramWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
#if 0
  QSPI_CommandTypeDef s_command;


  /* Initialize the read flag status register command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction       = PSRAM_CMD_LUT_SEQ_IDX_WRITE_QPI;
  s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  s_command.Address = addr;
  s_command.NbData  = length;

  /* Configure the command */
  if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(&QSPIHandle, p_data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
  s_command.Instruction        = PSRAM_CMD_LUT_SEQ_IDX_WRITE_QPI;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_4_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  s_command.Address            = addr;
  s_command.NbData             = length;


  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Transmission of the data */
  if (HAL_OSPI_Transmit(&hospi1, p_data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

#endif

  return ret;
}

void HAL_OSPI_MspInit(OSPI_HandleTypeDef* ospiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(ospiHandle->Instance==OCTOSPI1)
  {
  /* USER CODE BEGIN OCTOSPI1_MspInit 0 */

  /* USER CODE END OCTOSPI1_MspInit 0 */
    /* OCTOSPI1 clock enable */
    __HAL_RCC_OSPI1_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**OCTOSPI1 GPIO Configuration
    PE2     ------> OCTOSPIM_P1_IO2
    PB2     ------> OCTOSPIM_P1_CLK
    PD11     ------> OCTOSPIM_P1_IO0
    PD12     ------> OCTOSPIM_P1_IO1
    PD13     ------> OCTOSPIM_P1_IO3
    PG6     ------> OCTOSPIM_P1_NCS
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN OCTOSPI1_MspInit 1 */

  /* USER CODE END OCTOSPI1_MspInit 1 */
  }
}

void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef* ospiHandle)
{

  if(ospiHandle->Instance==OCTOSPI1)
  {
  /* USER CODE BEGIN OCTOSPI1_MspDeInit 0 */

  /* USER CODE END OCTOSPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_OSPI1_CLK_DISABLE();

    /**OCTOSPI1 GPIO Configuration
    PE2     ------> OCTOSPIM_P1_IO2
    PB2     ------> OCTOSPIM_P1_CLK
    PD11     ------> OCTOSPIM_P1_IO0
    PD12     ------> OCTOSPIM_P1_IO1
    PD13     ------> OCTOSPIM_P1_IO3
    PG6     ------> OCTOSPIM_P1_NCS
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_6);

  /* USER CODE BEGIN OCTOSPI1_MspDeInit 1 */

  /* USER CODE END OCTOSPI1_MspDeInit 1 */
  }
}



#ifdef _USE_HW_CLI
void cliPsram(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("PSRAM Add : 0x%X\n", psram_addr);
    cliPrintf("PSRAM Len : %dKB\n", psram_length/1024);

    for (int i=0; i<PSRAM_MAX_CH; i++)
    {
      cliPrintf("PSRAM CH%d, Init: %d, 0x%X, Len : %dKB\n", i+1,
                  psram_tbl[i].is_init,
                  psram_tbl[i].id,
                  psram_tbl[i].length/1024);
    }
    ret = true;
  }
  else if (args->argc == 1 && args->isStr(0, "test") == true)
  {
    uint32_t w_data[128];
    uint32_t r_data[128];

    for (int i=0; i<128; i++)
    {
      w_data[i] = i;
      r_data[i] = 0;
    }

    if (psramWrite(0, (uint8_t *)&w_data, 128*sizeof(uint32_t)) == true)
    {
      cliPrintf("PSRAM Write OK\n");
    }
    else
    {
      cliPrintf("PSRAM Write Fail\n");
    }

    if (psramRead(0, (uint8_t *)&r_data, 128*sizeof(uint32_t)) == true)
    {
      cliPrintf("PSRAM Read OK\n");
    }
    else
    {
      cliPrintf("PSRAM Read Fail\n");
    }

    for (int i=0; i<128; i++)
    {
      cliPrintf("Data %d:%d \n", w_data[i], r_data[i]);

      if (w_data[i] != r_data[i])
      {
        cliPrintf("Fail \n");
      }
    }

    ret = true;
  }
  else if (args->argc == 2 && args->isStr(0, "test_xip") == true)
  {
    uint32_t *p_data = (uint32_t *)psram_addr;
    uint8_t number;
    uint32_t i;
    uint32_t pre_time;


    number = (uint8_t)args->getData(1);

    while(number > 0)
    {
     pre_time = millis();
     for (i=0; i<psram_length/4; i++)
     {
       p_data[i] = i;
     }
     cliPrintf( "Write : %d MB/s\n", psram_length / 1000 / (millis()-pre_time) );

     volatile uint32_t data_sum = 0;
     pre_time = millis();
     for (i=0; i<psram_length/4; i++)
     {
       data_sum += p_data[i];
     }
     cliPrintf( "Read : %d MB/s\n", psram_length / 1000 / (millis()-pre_time) );


     for (i=0; i<psram_length/4; i++)
     {
       if (p_data[i] != i)
       {
         cliPrintf( "%d : 0x%X fail\n", i, p_data[i]);
         break;
       }
     }

     if (i == psram_length/4)
     {
       cliPrintf( "Count %d\n", number);
       cliPrintf( "PSRAM %d MB OK\n\n", psram_length/1024/1024);
       for (i=0; i<psram_length/4; i++)
       {
         p_data[i] = 0x5555AAAA;
       }
     }

     number--;

     if (cliAvailable() > 0)
     {
       cliPrintf( "Stop test...\n");
       break;
     }
    }

    ret = true;
  }
  else
  {
    ret = false;
  }

  if (ret == false)
  {
    cliPrintf( "psram info \n");
    cliPrintf( "psram test \n");
  }
}
#endif

#endif
