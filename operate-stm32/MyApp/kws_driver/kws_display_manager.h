#pragma once
#include "kws_display_type.h"
#include "kws_led.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

void init_display(void);

void update_ky016_vibe(void);
void update_ky016_sound(void);

void update_lcd1602(void);

void set_lcd_data(lcd_display_data_t display_data);

void toggle_lcd_mode(void);

void update_ssd1306(void);

void update_rgc_led(void);