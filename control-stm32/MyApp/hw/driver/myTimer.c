#include "myTimer.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"
#include <stdint.h>

void timerInit(void){
    timerPwmStart();
    timerSetDuty(50);
}

void timerPwmStart(void){
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void timerPwmStop(void){
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void timerSetDuty(uint8_t duty_percent){
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3)+1;
}

