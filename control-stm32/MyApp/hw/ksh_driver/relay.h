#pragma once

#include "stm32f4xx_hal.h"

void relay_init(void);
void relay_off(void);

HAL_StatusTypeDef relay_receive_start(void);
void relay_receive_event(UART_HandleTypeDef *huart, uint16_t position);
void relay_receive_error(UART_HandleTypeDef *huart);