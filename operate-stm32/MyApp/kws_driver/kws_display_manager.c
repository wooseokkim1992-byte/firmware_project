#include "kws_display_manager.h"
#include "kws_lcd.h"
#include "kws_led.h"
#include <stdio.h>

extern volatile lcd_display_data_t lcd_display_data;
static display_mode_t lcd_mode = SYSTEM_STATUS;

static void reset_lcd_display_mode() { lcd_mode = 0; }

static void set_lcd_display_mode() { lcd_mode = (lcd_mode + 1) % 3; }
void init_display() {
  lcd_mode = 0;
  if (lcd1602_init()) {
    lcd1602_cursor(0, 0);
    lcd1602_print_initial("LCD1602 READY   ");
    lcd1602_cursor(1, 0);
    lcd1602_print_initial("I2C3: 100kHz    ");
  }
  reset_lcd_display_mode();
}

void set_lcd_data(lcd_display_data_t display_data) {
  lcd_display_data = display_data;
  reset_lcd_display_mode();
}

void update_lcd1602(void) {
  char line1[LCD1602_BUF_LEN] = {0};
  char line2[LCD1602_BUF_LEN] = {0};

  if (lcd_mode == SYSTEM_STATUS) {
    const char *state_text = "UNKNOWN";

    if (lcd_display_data.state == NORMAL) {
      state_text = "NORMAL";
    } else if (lcd_display_data.state == WARNING) {
      state_text = "WARNING";
    } else if (lcd_display_data.state == DANGER) {
      state_text = "DANGER";
    } else if (lcd_display_data.state == EMERGENCY_STOP) {
      state_text = "E-STOP";
    }

    (void)snprintf(line1, sizeof(line1), "%s V:%umg", state_text,
                   (unsigned int)lcd_display_data.vibration_rms_mg);
    (void)snprintf(line2, sizeof(line2), "M:%s R:%s C:%s",
                   lcd_display_data.motor_running ? "RUN" : "STP",
                   lcd_display_data.relay_on ? "ON" : "OFF",
                   lcd_display_data.communication_ok ? "OK" : "NO");

  } else if (lcd_mode == VIBE_STATUS) {
    (void)snprintf(line1, sizeof(line1), "X:%u Y:%u",
                   (unsigned int)lcd_display_data.axis_x_rms_mg,
                   (unsigned int)lcd_display_data.axis_y_rms_mg);
    (void)snprintf(line2, sizeof(line2), "Z:%u P:%u",
                   (unsigned int)lcd_display_data.axis_z_rms_mg,
                   (unsigned int)lcd_display_data.vibration_peak_mg);
  } else if (lcd_mode == EXT_SENSOR) {
    (void)snprintf(line1, sizeof(line1), "S:%u R:%u",
                   (unsigned int)lcd_display_data.sound_raw,
                   (unsigned int)lcd_display_data.rpm);
    (void)snprintf(line2, sizeof(line2), "MPU:%s DMA:%s",
                   lcd_display_data.mpu6050_ok ? "OK" : "NO",
                   lcd_display_data.dma_ok ? "OK" : "NO");
  }

  if (lcd1602_update_lines_dma(line1, line2)) {
    set_lcd_display_mode();
  }
}

void update_ky016() { update_ky016_oled(lcd_display_data.state); }

// ***### 화면 1: 시스템 상태***

// NORMAL V:0.42g
// M:RUN R:ON C:OK

// 가장 중요한 기본 화면입니다.

// ***### 화면 2: 축별 진동***
// AX:.12 AY:.35
// AZ:.18 PK:.87

// - AX/AY/AZ: 축별 진동 RMS
// - PK: 전체 진동 Peak

// ***### 화면 3: 확장 센서***

// SND:62
// MPU:OK DMA:OK

// - SND: Sound Sensor 상대값
// - MPU: MPU6050 상태
// - DMA: DMA 수집 상태

// RPM과 Sound Sensor가 구현되지 않았다면 다음처럼 표시할 수 있습니다.
// RATE:200Hz N:200
// MPU:OK DMA:OK
