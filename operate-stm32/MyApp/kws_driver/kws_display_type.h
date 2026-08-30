#pragma once
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum _display_mode_t {
  VIBE_MODE = 0,
  SOUND_MODE = 1,
} display_mode_t;

typedef enum _system_state_t {
  NORMAL = 0,
  WARNING,
  DANGER,
  EMERGENCY_STOP
} system_state_t;

typedef struct {
  system_state_t vibe_state;
  system_state_t sound_state;

  float vibration_rms_mg;
  float vibration_peak_mg;

  float sound_rms;
  uint16_t sound_peak;

  bool motor_running; // 모터 구동 여부
  bool relay_on;      // relay 구동 여부
  bool mpu6050_ok;    // 모터 물어보는걸
} lcd_display_data_t;

typedef struct {
  bool motor_running; // 모터 구동 여부
  bool relay_on;      // relay 구동 여부
  bool mpu6050_ok;    // 모터 물어보는걸
} lcd_display_data_t1;
