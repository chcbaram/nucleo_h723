/*
 * hw_def.h
 *
 *  Created on: Mar 14, 2021
 *      Author: baram
 */

#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


#include "def.h"
#include "bsp.h"


#define _USE_HW_LED
#define      HW_LED_MAX_CH          3

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1

#define _USE_HW_BUTTON
#define      HW_BUTTON_MAX_CH       1


#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    24
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    4
#define      HW_CLI_LINE_BUF_MAX    64

#define _USE_HW_PSRAM


#endif /* SRC_HW_HW_DEF_H_ */
