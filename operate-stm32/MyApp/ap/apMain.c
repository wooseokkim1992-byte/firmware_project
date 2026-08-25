#include "apMain.h"
#include "myAdc.h"
#include "myDht11.h"
#include "myDs1302.h"
#include "myGpio.h"
#include "myHcSr04.h"
#include "myLcd1602.h"
#include "myMpu6050.h"
#include "myUart.h"
#include "oled.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "tim.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern ADC_HandleTypeDef hadc1;

void draw_SSD_Frame(bool isFirst) {
  if (isFirst) {
    ssd1306Clear();
  }
  ssd1306DrawRect(0, 0, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_COLOR_WHITE);
  ssd1306DrawString(8, 3, "STM32 MULTI-SENSOR", SSD1306_COLOR_WHITE);
  ssd1306DrawLine(4, 13, 124, 13, SSD1306_COLOR_WHITE);
  if (isFirst) {
    ssd1306Update();
  }
}

void apInit(void) {
  uartInit();
  adcInit();
  dht11Init();
  lcd1602Init();
  mpu6050Init();
  // ssd1306Init();
  ds1302Init();
  ssd1306Init();
}

float internal_temp = 0;
dht11Data_t dht_data = {0};
bool dht_status = false;
float distance_cm = 0.0f;

void apMain(void) {
  draw_SSD_Frame(true);
  uint32_t current_tick = 0;
  uint32_t tick_250 = 0;
  while (1) {
    current_tick = HAL_GetTick();
    if (current_tick - tick_250 >= 250) {
      tick_250 = current_tick;
      if (!isSsd1306DMABusy()) {
        ssd1306Clear();
        draw_SSD_Frame(false);
        ssd1306DrawString(4, 15, "STATE: NORMAL", SSD1306_COLOR_WHITE);
        // ssd1306Update();
        ssd1306UpdateDMA();
      }
    }
  }
}