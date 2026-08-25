#pragma once

#include "i2c.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SSD1306_I2C_ADDR (0x3C << 1) // 0x78
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define SSD1306_COLOR_BLACK 0
#define SSD1306_COLOR_WHITE 1

#define SSD1306_BUFFER_SIZE ((SSD1306_WIDTH * SSD1306_HEIGHT) / 8)

bool ssd1306Init(void);
void ssd1306Clear(void);
void ssd1306Update(void);
void ssd1306DrawPixel(int16_t x, int16_t y, uint8_t color);
void ssd1306DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint8_t color);
void ssd1306DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306DrawChar(int16_t x, int16_t y, char ch, uint8_t color);
void ssd1306DrawString(int16_t x, int16_t y, const char *str, uint8_t color);
void ssd1306Test(void);
bool ssd1306UpdateDMA(void);
bool isSsd1306DMABusy(void);
