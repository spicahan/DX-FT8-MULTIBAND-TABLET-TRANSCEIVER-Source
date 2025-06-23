
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>

#ifdef HOST_HAL_MOCK
#include "host_mocks.h"
#else
#include "stm32746g_discovery.h"
#include "stm32f7xx_hal.h"
#endif
void _debug(const char *txt);
void tx_display_update();
// TODO refactor
void display_qso_state(const char *txt);

#define NoOp  __NOP()

extern uint32_t start_time, ft8_time;

extern int QSO_xmit;

extern int slot_state;
extern int target_slot;
extern int target_freq;
extern int decode_flag;
extern int was_txing;

extern const char* test_data_file;

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
