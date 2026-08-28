#include "myGpio.h"
#include <stdio.h>
#include <stdbool.h>


extern bool kill_request;

void gpioInit(void){
    //
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

  if(GPIO_Pin==GPIO_PIN_13){
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
  }
  if(GPIO_Pin == GPIO_PIN_12)
    {
        kill_request = true;
    }
}

/* 타이머 주기 완료 콜백 오버라이딩 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
  if (htim->Instance == TIM2)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // 1초마다 LED 반전
  }
}

