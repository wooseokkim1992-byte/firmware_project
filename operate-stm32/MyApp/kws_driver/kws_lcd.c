#include "kws_lcd.h"

#include "i2c.h"

#include <stdarg.h>
#include <stdio.h>

#define LCD1602_POWER_ON_DELAY_MS  50U
#define LCD1602_ENABLE_DELAY_MS     1U
#define LCD1602_LINE_COUNT          2U
#define LCD1602_COLUMN_COUNT       16U

static bool lcd_ok = false;
static bool backlight_state = true;

static HAL_StatusTypeDef lcd1602_i2c_transmit(uint8_t *data, uint16_t size)
{
  return HAL_I2C_Master_Transmit(&hi2c3, LCD1602_ADDR, data, size,
                                 LCD1602_INIT_TIME_MS);
}

/*
 * HD44780의 4-bit 모드가 아직 설정되지 않은 초기화 구간에서
 * 상위 nibble 하나와 Enable 펄스만 전송한다.
 */
static bool lcd1602_write_init_nibble(uint8_t nibble)
{
  uint8_t backlight = backlight_state ? LCD_BL : 0x00U;
  uint8_t tx_data[2] = {
      (uint8_t)((nibble & 0xF0U) | backlight | LCD_EN),
      (uint8_t)((nibble & 0xF0U) | backlight),
  };

  return lcd1602_i2c_transmit(tx_data, sizeof(tx_data)) == HAL_OK;
}

/*
 * 명령 또는 데이터 한 바이트를 상위/하위 nibble로 나누어 전송한다.
 * rs가 LCD_RS이면 데이터, 0이면 명령으로 처리된다.
 */
static bool lcd1602_write_byte(uint8_t value, uint8_t rs)
{
  uint8_t backlight = backlight_state ? LCD_BL : 0x00U;
  uint8_t high_nibble = value & 0xF0U;
  uint8_t low_nibble = (uint8_t)((value << 4U) & 0xF0U);
  uint8_t tx_data[4] = {
      (uint8_t)(high_nibble | backlight | rs | LCD_EN),
      (uint8_t)(high_nibble | backlight | rs),
      (uint8_t)(low_nibble | backlight | rs | LCD_EN),
      (uint8_t)(low_nibble | backlight | rs),
  };

  return lcd1602_i2c_transmit(tx_data, sizeof(tx_data)) == HAL_OK;
}

bool lcd1602_init(void)
{
  lcd_ok = false;
  backlight_state = true;

  /* HD44780 전원 안정화 시간 확보 */
  HAL_Delay(LCD1602_POWER_ON_DELAY_MS);

  if (HAL_I2C_IsDeviceReady(&hi2c3, LCD1602_ADDR, 2U,
                            LCD1602_INIT_TIME_MS) != HAL_OK)
  {
    return false;
  }

  /* 알 수 없는 전원 초기 상태에서 8-bit 상태를 확정한다. */
  if (!lcd1602_write_init_nibble(0x30U))
  {
    return false;
  }
  HAL_Delay(5U);

  if (!lcd1602_write_init_nibble(0x30U))
  {
    return false;
  }
  HAL_Delay(LCD1602_ENABLE_DELAY_MS);

  if (!lcd1602_write_init_nibble(0x30U))
  {
    return false;
  }
  HAL_Delay(LCD1602_ENABLE_DELAY_MS);

  /* 이후 명령부터 4-bit 인터페이스를 사용한다. */
  if (!lcd1602_write_init_nibble(0x20U))
  {
    return false;
  }
  HAL_Delay(LCD1602_ENABLE_DELAY_MS);

  lcd_ok = true;

  lcd1602_send_command(0x28U); /* 4-bit, 2-line, 5x8 font */
  lcd1602_send_command(0x08U); /* Display OFF */
  lcd1602_clear();
  lcd1602_send_command(0x06U); /* Entry mode: cursor moves right */
  lcd1602_send_command(0x0CU); /* Display ON, cursor/blink OFF */

  return lcd_ok;
}

void lcd1602_send_command(uint8_t cmd)
{
  if (!lcd_ok)
  {
    return;
  }

  if (!lcd1602_write_byte(cmd, 0x00U))
  {
    lcd_ok = false;
    return;
  }

  /* Clear와 Return Home 명령은 실행 시간이 더 길다. */
  if ((cmd == 0x01U) || (cmd == 0x02U))
  {
    HAL_Delay(2U);
  }
}

void lcd1602_send_data(uint8_t data)
{
  if (!lcd_ok)
  {
    return;
  }

  if (!lcd1602_write_byte(data, LCD_RS))
  {
    lcd_ok = false;
  }
}

void lcd1602_clear(void)
{
  lcd1602_send_command(0x01U);
}

void lcd1602_cursor(uint8_t row, uint8_t col)
{
  static const uint8_t row_address[LCD1602_LINE_COUNT] = {0x00U, 0x40U};

  if (!lcd_ok || (row >= LCD1602_LINE_COUNT) ||
      (col >= LCD1602_COLUMN_COUNT))
  {
    return;
  }

  lcd1602_send_command((uint8_t)(0x80U | (row_address[row] + col)));
}

void lcd1602_print(const char *str)
{
  if (!lcd_ok || (str == NULL))
  {
    return;
  }

  while ((*str != '\0') && lcd_ok)
  {
    lcd1602_send_data((uint8_t)*str);
    ++str;
  }
}

void lcd1602_printf(const char *fmt, ...)
{
  char text[33];
  va_list args;

  if (!lcd_ok || (fmt == NULL))
  {
    return;
  }

  va_start(args, fmt);
  (void)vsnprintf(text, sizeof(text), fmt, args);
  va_end(args);

  lcd1602_print(text);
}

void lcd1602_backlight(bool on)
{
  uint8_t output;

  if (!lcd_ok)
  {
    return;
  }

  backlight_state = on;
  output = backlight_state ? LCD_BL : 0x00U;

  if (lcd1602_i2c_transmit(&output, 1U) != HAL_OK)
  {
    lcd_ok = false;
  }
}
