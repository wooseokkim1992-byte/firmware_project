#pragma once

#include <stdbool.h>
#include "stm32f4xx_hal.h"

void relay_init(void);
void relay_on(void);
void relay_off(void);


void relay_check_stop(
    bool danger,
    bool emergency_stop
);

/* 모터 내부 상태 판정 후 호출 */
void relay_check_stop(bool danger, bool emergency_stop);

/* USART6 DMA 수신 */
HAL_StatusTypeDef relay_receive_start(void);
void relay_receive_event(UART_HandleTypeDef *huart,
                         uint16_t position);
void relay_receive_error(UART_HandleTypeDef *huart);