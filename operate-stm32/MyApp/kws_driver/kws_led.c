#include "kws_led.h"
#include "kws_display_manager.h"
#include "kws_display_type.h"
#include "stm32f4xx_hal_gpio.h"

extern volatile lcd_display_data_t lcd_display_data;

void update_ky016_oled_1(system_state_t mode, GPIO_TypeDef *GPIOx_R, uint16_t R,
                         GPIO_TypeDef *GPIOx_G, uint16_t G,
                         GPIO_TypeDef *GPIOx_B, uint16_t B) {
  if (mode == NORMAL) {
    HAL_GPIO_WritePin(GPIOx_R, R, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx_G, G, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx_B, B, GPIO_PIN_SET);
  } else if (mode == WARNING) {
    HAL_GPIO_WritePin(GPIOx_R, R, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx_G, G, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOx_B, B, GPIO_PIN_RESET);
  } else if (mode == DANGER) {
    HAL_GPIO_WritePin(GPIOx_R, R, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOx_G, G, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx_B, B, GPIO_PIN_RESET);
  } else {
    HAL_GPIO_TogglePin(GPIOx_R, R);
    HAL_GPIO_WritePin(GPIOx_G, G, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx_B, B, GPIO_PIN_RESET);
  }
}

void update_ky016_oled(system_state_t mode) {
  if (mode == NORMAL) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
  } else if (mode == WARNING) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
  } else if (mode == DANGER) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
  }
}