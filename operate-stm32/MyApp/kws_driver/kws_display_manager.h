#pragma once
#include "kws_led.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum _display_mode_t {
  VIBE_MODE = 0,
  SOUND_MODE = 1,
} display_mode_t;

typedef struct {
  system_state_t state;

  float vibration_rms_mg;
  float vibration_peak_mg;

  float sound_rms;
  uint16_t sound_peak;

  bool motor_running;
  bool relay_on;
  bool communication_ok;
  bool mpu6050_ok; // 모터 물어보는걸
} lcd_display_data_t;

void init_display(void);

void update_ky016(void);

void update_lcd1602(void);

void set_lcd_data(lcd_display_data_t display_data);

void toggle_lcd_mode(void);

void update_ssd1306(void);