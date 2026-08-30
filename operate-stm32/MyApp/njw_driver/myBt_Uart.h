#pragma once

#include "main.h"

typedef struct {
  char check_1;
  char check_2;
} data_header_t;

#define HEADER_SIZE 2
#define LED_DATA_SIZE 27
// #define LED_DATA_SIZE sizeof(lcd_display_data_t)
#define BT_REQUEST_TIMEOUT 1000

typedef enum { BT_RX_HEADER, BT_RX_DATA } bt_rx_state_t;

void btInit(void);
void bt_sendHeader(void);
void bt_DataCheck(uint8_t *rx_buf);
void bt_SendLedData(void);
void bt_SendKill(void);
void parsing_data(void);
