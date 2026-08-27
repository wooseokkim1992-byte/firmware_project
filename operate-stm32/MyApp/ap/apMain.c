#include "apMain.h"
#include "myDs1302.h"
#include "myGpio.h"
#include "myMpu6050.h"
#include "myUart.h"
#include "oled.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "tim.h"

#include "kws_display_manager.h"
#include "kws_lcd.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void apInit(void) {
  uartInit();
  init_display();
  mpu6050Init();
  // ssd1306Init();
  ds1302Init();
  ssd1306Init();
}

float internal_temp = 0;
bool dht_status = false;
float distance_cm = 0.0f;
lcd_display_data_t lcd_dummy_data = {.state = NORMAL,

                                     .vibration_rms_mg = 125U,
                                     .vibration_peak_mg = 430U,

                                     .axis_x_rms_mg = 82U,
                                     .axis_y_rms_mg = 97U,
                                     .axis_z_rms_mg = 54U,

                                     .rpm = 1450U,
                                     .sound_raw = 1875U,

                                     .motor_running = true,
                                     .relay_on = true,
                                     .communication_ok = true,
                                     .mpu6050_ok = true,
                                     .dma_ok = true};
void apMain(void) {
  set_lcd_data(lcd_dummy_data);
  uint32_t current_tick = 0;
  uint32_t tick_250 = 0;
  uint32_t tick_500 = 0;
  while (1) {
    current_tick = HAL_GetTick();
    if (current_tick - tick_250 >= 250) {
      tick_250 = current_tick;
      if (!isSsd1306DMABusy()) {
        ssd1306Clear();
        ssd1306DrawString(4, 15, "STATE: NORMAL", SSD1306_COLOR_WHITE);
        // ssd1306Update();
        ssd1306UpdateDMA();
      }
    }
    if (current_tick - tick_500 >= 2000) {
      tick_500 = current_tick;
      update_lcd1602();
    }
  }
}
