#include "kws_display_manager.h"
#include "kws_lcd.h"
#include "kws_led.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>

extern volatile lcd_display_data_t lcd_display_data;
volatile static display_mode_t lcd_mode = VIBE_MODE;
volatile static display_mode_t oled_mode = VIBE_MODE;

static void reset_lcd_display_mode() { lcd_mode = 0; }
static void reset_oled_display_mode() { oled_mode = 0; }

static void set_lcd_display_mode() { lcd_mode = (lcd_mode + 1) % 2; }
static void set_oled_display_mode() { oled_mode = (oled_mode + 1) % 2; }
void init_display() {
  lcd_mode = VIBE_MODE;
  if (lcd1602_init()) {
    lcd1602_cursor(0, 0);
    lcd1602_print_initial("LCD1602 READY   ");
    lcd1602_cursor(1, 0);
    lcd1602_print_initial("I2C3: 100kHz    ");
  }
  if (ssd1306Init()) {
  }
  reset_oled_display_mode();
  reset_lcd_display_mode();
}

void set_lcd_data(lcd_display_data_t display_data) {
  lcd_display_data = display_data;
  reset_lcd_display_mode();
}

void update_ssd1306() {
  if (!isSsd1306DMABusy()) {
    ssd1306Clear();
    char state_buf[8];
    char buf[SSD1306_WIDTH];
    if (oled_mode == VIBE_MODE) {
      if (lcd_display_data.vibe_state == NORMAL) {
        strcpy(state_buf, "NORMAL");
      } else if (lcd_display_data.vibe_state == WARNING) {
        strcpy(state_buf, "WARNING");
      } else {
        strcpy(state_buf, "DANGER");
      }
      if (sprintf(buf, "VIBERATION : %s", state_buf) < SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 2, buf, SSD1306_COLOR_WHITE);
      }
      if (sprintf(buf, "RMS:%.1f", lcd_display_data.vibration_rms_mg) <
          SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 12, buf, SSD1306_COLOR_WHITE);
      }
      if (sprintf(buf, "peak:%.1f", lcd_display_data.vibration_rms_mg) <
          SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 20, buf, SSD1306_COLOR_WHITE);
      }

    } else {
      if (lcd_display_data.sound_state == NORMAL) {
        strcpy(state_buf, "NORMAL");
      } else if (lcd_display_data.sound_state == WARNING) {
        strcpy(state_buf, "WARNING");
      } else {
        strcpy(state_buf, "DANGER");
      }
      if (sprintf(buf, "SOUND : %s", state_buf) < SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 2, buf, SSD1306_COLOR_WHITE);
      }
      if (sprintf(buf, "RMS:%.1f", lcd_display_data.vibration_rms_mg) <
          SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 12, buf, SSD1306_COLOR_WHITE);
      }
      if (sprintf(buf, "peak:%.1f", lcd_display_data.vibration_rms_mg) <
          SSD1306_WIDTH - 1) {
        buf[strlen(buf)] = '\0';
        ssd1306DrawString(2, 20, buf, SSD1306_COLOR_WHITE);
      }
    }
    ssd1306Update();
    if (ssd1306UpdateDMA()) {
      set_oled_display_mode();
    }
    // bt_sendHeader();
  }
}

void update_lcd1602(void) {
  char line1[LCD1602_BUF_LEN] = {0};
  char line2[LCD1602_BUF_LEN] = {0};

  const char *state_text = "UNKNOWN";
  if (lcd_mode == VIBE_MODE) {
    if (lcd_display_data.vibe_state == NORMAL) {
      state_text = "NORMAL";
    } else if (lcd_display_data.vibe_state == WARNING) {
      state_text = "WARN";
    } else if (lcd_display_data.vibe_state == DANGER) {
      state_text = "DANGER";
    } else if (lcd_display_data.vibe_state == EMERGENCY_STOP) {
      state_text = "E-STOP";
    }
    (void)snprintf(line1, sizeof(line1), "VIBERATION:%s", state_text);
    (void)snprintf(line2, sizeof(line2), "MR:%s R:%s MPU:%s",
                   lcd_display_data.motor_running ? "Y" : "N",
                   lcd_display_data.relay_on ? "Y" : "N",
                   lcd_display_data.mpu6050_ok ? "Y" : "N");

  } else {
    if (lcd_display_data.sound_state == NORMAL) {
      state_text = "NORMAL";
    } else if (lcd_display_data.sound_state == WARNING) {
      state_text = "WARN";
    } else if (lcd_display_data.sound_state == DANGER) {
      state_text = "DANGER";
    } else if (lcd_display_data.sound_state == EMERGENCY_STOP) {
      state_text = "E-STOP";
    }
    (void)snprintf(line1, sizeof(line1), "SOUND:%s", state_text);
    (void)snprintf(line2, sizeof(line2), "MR:%s R:%s MPU:%s",
                   lcd_display_data.motor_running ? "Y" : "N",
                   lcd_display_data.relay_on ? "Y" : "N",
                   lcd_display_data.mpu6050_ok ? "Y" : "N");
  }

  if (lcd1602_update_lines_dma(line1, line2)) {
    set_lcd_display_mode();
  }
}

void update_rgc_led() {
  update_ky016_sound();
  update_ky016_vibe();
}

void update_ky016_vibe() {
  update_ky016_oled_1(lcd_display_data.vibe_state, GPIOB, GPIO_PIN_5, GPIOB,
                      GPIO_PIN_4, GPIOB, GPIO_PIN_10);
}

void update_ky016_sound() {
  update_ky016_oled_1(lcd_display_data.sound_state, GPIOC, GPIO_PIN_1, GPIOB,
                      GPIO_PIN_0, GPIOA, GPIO_PIN_4);
}

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
