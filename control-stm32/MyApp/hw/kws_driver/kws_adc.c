#include "kws_adc.h"
#include "adc.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"
#include <stdint.h>

uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];
volatile bool adc_half_ready = false;
volatile bool adc_full_ready = false;
volatile uint32_t adc_overrun_count = 0U;

bool adc_init(void) {
  adc_half_ready = false;
  adc_full_ready = false;
  adc_overrun_count = 0U;

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer,
                        ADC_DMA_BUFFER_SIZE) != HAL_OK) {
    return false;
  }

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
    (void)HAL_ADC_Stop_DMA(&hadc1);
    return false;
  }

  return true;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    if (adc_half_ready) {
      adc_overrun_count++;
    }

    adc_half_ready = true;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    if (adc_full_ready) {
      adc_overrun_count++;
    }

    adc_full_ready = true;
  }
}
