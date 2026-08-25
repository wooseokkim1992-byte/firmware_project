#include "myDht11.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "system_stm32f4xx.h"
#include <stdint.h>

static float temperature = 0.0f;
static float humidity = 0.0f;

static void dwtInit(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delayUs(uint32_t us) {
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000);
  while ((DWT->CYCCNT - start) < ticks)
    ;
}

static void dht11SetPinOutput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void dht11SetPinInput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

void dht11Init(void) {
  dwtInit();
  dht11SetPinOutput();
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

bool dht11Read(dht11Data_t *data) {
  uint8_t raw_bytes[5] = {0};
  uint32_t timeout = 0;

  // 1. MCU 시작 신호 : 버스 low 18ms 유지
  dht11SetPinOutput();
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
  HAL_Delay(18);

  // 2. 버스를 high 20~40us 올린후 입력모드 전환
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
  delayUs(30);
  dht11SetPinInput();

  // 타이밍 보호를 위한 임계 구역
  __disable_irq();

  // 3. 응답 대기 low 80us -> high 80us
  timeout = 10000;
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
    if ((--timeout) == 0) {
      __enable_irq();
      return false;
    }
  }

  timeout = 10000;
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
    if ((--timeout) == 0) {
      __enable_irq();
      return false;
    }
  }

  timeout = 10000;
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
    if ((--timeout) == 0) {
      __enable_irq();
      return false;
    }
  }

  // 4. 40비트(5바이트) 데이터 수신

  for (uint8_t i = 0; i < 5; i++) {
    for (int8_t j = 7; j >= 0; j--) {
      timeout = 10000;
      while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        if ((--timeout) == 0) {
          __enable_irq();
          return false;
        }
      }

      // high 지속 시간 측정 (26~28us : 0 , 70us : 1)
      uint32_t t_start = DWT->CYCCNT;
      timeout = 10000;
      while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if ((--timeout) == 0) {
          __enable_irq();
          return false;
        }
      }

      uint32_t elapsed_us =
          (DWT->CYCCNT - t_start) / (SystemCoreClock / 1000000);

      if (elapsed_us > 40) {
        raw_bytes[i] |= (1 << j);
      }
    } // for j
  } // for i

  __enable_irq();
  dht11SetPinOutput();
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);

  // 5. checksu
  uint8_t checksum = raw_bytes[0] + raw_bytes[1] + raw_bytes[2] + raw_bytes[3];
  if (checksum != raw_bytes[4]) {
    return false;
  }

  humidity = (float)raw_bytes[0] + ((float)raw_bytes[1] * 0.1f);
  temperature = (float)raw_bytes[2] + ((float)raw_bytes[3] * 0.1f);

  if (data) {
    data->humidity = humidity;
    data->temperature = temperature;
    data->is_valid = true;
  }
  return true;
}