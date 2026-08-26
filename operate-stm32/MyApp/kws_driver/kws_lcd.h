#pragma once
#include "main.h"

#include <stdbool.h>
#include <stdint.h>

#define LCD1602_INIT_TIME_MS 20U
#define LCD1602_ADDR (0x27U << 1)
#define LCD_RS 0x01
#define LCD_RW 0x02
#define LCD_EN 0x04
#define LCD_BL 0x08
#define LCD_D4 0x10
#define LCD_D5 0x20
#define LCD_D6 0x40
#define LCD_D7 0x80

bool lcd1602_init(void);
void lcd1602_send_command(uint8_t cmd);
void lcd1602_send_data(uint8_t data);

void lcd1602_clear(void);
void lcd1602_cursor(uint8_t row, uint8_t col);
void lcd1602_print(const char *str);
void lcd1602_printf(const char *fmt, ...);
void lcd1602_backlight(bool on);
