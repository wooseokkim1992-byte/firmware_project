#pragma once
#include "main.h"
#include "stdbool.h"
#include "kws_display_type.h"

typedef struct{
    char check_1;
    char check_2;
}data_header_t;

typedef enum
{
    BT_RX_HEADER,
    BT_RX_DATA
} bt_rx_state_t;

void btInit(void);
bool bt_sendHeader(void);
void bt_DataCheck(uint8_t *rx_buf);
void bt_SendLedData(void);
void bt_SendKill(void);