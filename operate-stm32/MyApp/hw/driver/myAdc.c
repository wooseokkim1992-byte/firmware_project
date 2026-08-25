#include "myAdc.h"
#include "adc.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_ll_adc.h"
#include <stdint.h>
#include <stdbool.h>


/* STM32F411 팩토리 캘리브레이션 값 주소 (3.3V 기준 공장 측정치) */
#define TS_CAL1_ADDR             ((uint16_t *)0x1FFF7A2C) /* 30°C 측정값 */
#define TS_CAL2_ADDR             ((uint16_t *)0x1FFF7A2E) /* 110°C 측정값 */
#define TS_CAL1_TEMP             30.0f
#define TS_CAL2_TEMP             110.0f

/* 데이터시트 표준 파라미터 (Fallback용) */
#define V25_MV                   760.0f  /* 25도에서의 전압: 약 0.76V (760mV) */
#define AVG_SLOPE                2.5f    /* 전압-온도 기울기: 2.5 mV/°C */
#define VREF_MV                  3300.0f /* ADC 기준 전압: 3.3V */
#define ADC_MAX_VAL              4095.0f /* 12비트 ADC 최대값 */

/* DMA 전송용 버퍼 (Half-Word / 16비트 정렬) */

uint32_t adc_multi_values[4] = {0, 0, 0,0};
static uint16_t adc_dma_buf = 0;

/* DMA 인터럽트와 메인 루프 간 공유 변수 */
static volatile uint32_t adc_raw_val = 0;
static volatile bool is_conv_done = false;

/* 계산된 결과 변수 (메인 컨텍스트에서 갱신) */
static float calculated_temp = 0.0f;
static bool temp_updated_flag = false;

static uint32_t sample_interval_ms = 500; /* 기본 샘플링 주기: 0.5초 (500ms) */
static bool is_running = false;

void adcInit(void)
{
  adc_dma_buf = 0;
  adc_raw_val = 0;
  is_conv_done = false;
  calculated_temp = 0.0f;
  temp_updated_flag = false;
  sample_interval_ms = 500;
  is_running = true;
  
  HAL_ADC_Start_DMA(&hadc1,adc_multi_values, 4);
}


ADC_ChannelConfTypeDef sConfig={0};
uint32_t Adc_Ch0(void){
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  uint32_t temp_Adc=HAL_ADC_GetValue(&hadc1);

  HAL_ADC_Stop(&hadc1);
  return temp_Adc;
}
uint32_t Adc_Ch1(void){
    sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  uint32_t temp_Adc=HAL_ADC_GetValue(&hadc1);

  HAL_ADC_Stop(&hadc1);
  return temp_Adc;
}
uint32_t Adc_Ch4(void){
    sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  uint32_t temp_Adc=HAL_ADC_GetValue(&hadc1);

  HAL_ADC_Stop(&hadc1);
  return temp_Adc;
}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
  if(hadc->Instance==ADC1){
    is_conv_done=true;
    adc_raw_val=adc_multi_values[3];
  }
}

void adcUpdate(void)
{
  if (!is_running)
    return;


  /* 1. DMA 완료 플래그가 설정된 경우 메인 컨텍스트에서 온도 계산 */
  if (is_conv_done)
  {
    is_conv_done = false;

    uint16_t ts_cal1 = *TS_CAL1_ADDR;
    uint16_t ts_cal2 = *TS_CAL2_ADDR;

    /* 팩토리 캘리브레이션 유효성 확인 */
    if (ts_cal2 > ts_cal1 && ts_cal1 > 0 && ts_cal2 < 4096)
    {
      /* ST 공식 팩토리 캘리브레이션 온도 보정 공식 */
      calculated_temp = ((TS_CAL2_TEMP - TS_CAL1_TEMP) * ((float)adc_raw_val - (float)ts_cal1)) / (float)(ts_cal2 - ts_cal1) + TS_CAL1_TEMP;
    }
    else
    {
      /* 표준 데이터시트 공식 (Fallback) */
      float vsense_mv = ((float)adc_raw_val * VREF_MV) / ADC_MAX_VAL;
      calculated_temp = ((vsense_mv - V25_MV) / AVG_SLOPE) + 25.0f;
    }

    temp_updated_flag = true;
  }

}

float adcGetTemp(void){
  return calculated_temp;
}