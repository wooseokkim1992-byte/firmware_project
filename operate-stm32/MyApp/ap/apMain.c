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
volatile lcd_display_data_t lcd_display_data = {
    .vibe_state = NORMAL,
    .sound_state = NORMAL,

    .vibration_rms_mg = 35.5,
    .vibration_peak_mg = 35.5,

    .sound_rms = 82.42,
    .sound_peak = 97U,

    .motor_running = false,
    .relay_on = true,
    .communication_ok = true,
    .mpu6050_ok = true,
};
void apMain(void) {
  set_lcd_data(lcd_display_data);
  uint32_t current_tick = 0;
  uint32_t tick_1000 = 0;
  uint32_t tick_100 = 0;
  uint32_t tick_2000 = 0;

  while (1) {
    current_tick = HAL_GetTick();
    if (current_tick - tick_100 >= 100) {
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
