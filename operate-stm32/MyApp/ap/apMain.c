#include "apMain.h"
#include "kws_led.h"
#include "myDs1302.h"
#include "myGpio.h"
#include "myMpu6050.h"
#include "myUart.h"
#include "stm32f4xx_hal.h"

#include "kws_display_manager.h"
#include <stdint.h>

#include "myBt_Uart.h"

void apInit(void) {
  // bt_Reset();
  btInit();
  init_display();
  mpu6050Init();
  // ssd1306Init();
  ds1302Init();
}

float internal_temp = 0;
bool dht_status = false;
float distance_cm = 0.0f;
volatile bool kill_request = false;

void apMain(void) {
  uint32_t current_tick = 0;
  uint32_t tick_1000 = 0;
  uint32_t tick_100 = 0;
  uint32_t tick_2000 = 0;
  // bt_sendHeader();
  while (1) {
    current_tick = HAL_GetTick();

    if (current_tick - tick_100 >= 100) {
      // set_lcd_data();
      tick_100 = current_tick;

      update_rgc_led();
    }
    if (current_tick - tick_1000 >= 1000) {
      tick_1000 = current_tick;
      update_ssd1306();
    }
    if (current_tick - tick_2000 >= 2000) {
      tick_2000 = current_tick;
      update_lcd1602();
    }

    if (kill_request) {
      if (huart1.gState == HAL_UART_STATE_READY) {
        kill_request = false;
        bt_SendKill();
      }
    }
  }
}
