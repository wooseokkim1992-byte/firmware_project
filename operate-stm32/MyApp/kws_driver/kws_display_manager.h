#pragma once
#include "kws_led.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum _display_mode_t {
  SYSTEM_STATUS = 0,
  VIBE_STATUS = 1,
  EXT_SENSOR = 2
} display_mode_t;

typedef struct {
  system_state_t state;

  uint16_t vibration_rms_mg;
  uint16_t vibration_peak_mg;

  uint16_t axis_x_rms_mg;
  uint16_t axis_y_rms_mg;
  uint16_t axis_z_rms_mg;

  uint16_t rpm; // 제거
  uint16_t sound_raw;

  bool motor_running;
  bool relay_on;
  bool communication_ok;
  bool mpu6050_ok; // 모터 물어보는걸
  bool dma_ok;     // 제거
} lcd_display_data_t;

void init_display(void);

void update_ky016(void);

void update_lcd1602(void);

void set_lcd_data(lcd_display_data_t display_data);
