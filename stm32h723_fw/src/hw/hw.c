/*
 * hw.c
 *
 *  Created on: Mar 14, 2021
 *      Author: baram
 */


#include "hw.h"





void hwInit(void)
{
  bspInit();

  cliInit();
  ledInit();
  buttonInit();
  uartInit();
  uartOpen(_DEF_UART1, 57600);


  logPrintf("\n\n[ Firmware Begin... ]\r\n");
  logPrintf("Booting..Clock\t\t: %d Mhz\r\n", (int)HAL_RCC_GetSysClockFreq()/1000000);
}
