#pragma once
#include "main.h"
#include "kws_display_manager.h"
#include "kws_lcd.h"


typedef struct{
    char check_1;
    char check_2;
    uint8_t data_type; // 0 : NULL / 1 : lcd_display_data_t / 2 : DisplayData_t
}data_header_t;

typedef enum
{
    BT_RX_HEADER,
    BT_RX_DATA
} bt_rx_state_t;

extern volatile lcd_display_data_t lcd_display_data;
extern volatile bool kill_request;


void btInit(void);
void bt_sendHeader(void);
void bt_DataCheck(uint8_t *rx_buf);
void bt_SendLedData(void);