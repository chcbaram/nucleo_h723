/*
 * bsp.h
 *
 *  Created on: Mar 14, 2021
 *      Author: baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_

#include "def.h"

#define logPrintf(...)    printf(__VA_ARGS__)


#include "stm32h7xx_hal.h"


void bspInit(void);

void delay(uint32_t ms);
uint32_t millis(void);


void Error_Handler(void);

#endif /* SRC_BSP_BSP_H_ */
