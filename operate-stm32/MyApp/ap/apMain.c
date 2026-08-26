#include "apMain.h"
#include "myAdc.h"
#include "myDht11.h"
#include "myDs1302.h"
#include "myGpio.h"
#include "myHcSr04.h"
#include "myMpu6050.h"
#include "myUart.h"
#include "oled.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "tim.h"

#include "kws_lcd.h"
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
  if (lcd1602_init()) {
    lcd1602_cursor(0, 0);
    lcd1602_print_initial("LCD1602 READY   ");
    lcd1602_cursor(1, 0);
    lcd1602_print_initial("I2C3: 100kHz    ");
  }
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
  uint32_t tick_100 = 0;
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
    if (current_tick - tick_100 >= 500) {
      tick_100 = current_tick;
      lcd1602_clear();
      lcd1602_cursor(0, 0);
      lcd1602_print("hellow");
      lcd1602_send_dma_data();
    }
  }
}
