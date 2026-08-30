#include "myBt_Uart.h"
#include "ksh_driver/relay.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include "gpio.h"
#include "stdbool.h"
#include "kws_display_type.h"
#include "sound_level.h"
#include "vibration.h"
#include "vibration_state.h"
#include "relay.h"

#define HEADER_SIZE ((uint16_t)sizeof(data_header_t))
#define LED_DATA_SIZE 27
#define BT_REQUEST_TIMEOUT 1000


static data_header_t tx_ok = {'O', 'K'};
static data_header_t tx_tt = {'T', 'T'};
static uint32_t bt_request_tick = 0;
static lcd_display_data_t bt_tx_data;
static uint8_t tx_buf[LED_DATA_SIZE];
static uint8_t ok_count=0;

static volatile bool bt_request_active = false;
volatile bt_rx_state_t bt_rx_state = BT_RX_HEADER;
volatile HAL_StatusTypeDef bt_tx_status;
volatile HAL_StatusTypeDef bt_rx_status;
volatile uint32_t bt_uart_error = 0;
volatile bool is_ok_send = false;


extern volatile vibration_data_t vibration_data; // rms , peak
extern volatile bool kill_request; // kill 
extern volatile bool relay_state;
// vibration target_state : 
extern volatile vibration_state_data_t vibration_state_data;
extern volatile sound_level_data_t sound_level_result;
extern volatile sound_level_state_t sound_current_state;
extern volatile bool mpu_init_status;

uint8_t rx_header_buf[2];
uint8_t rx_data_buf[LED_DATA_SIZE];

void btInit(void){
    bt_rx_state = BT_RX_HEADER;

    bt_rx_status = HAL_UART_Receive_DMA(&huart1,
                                 rx_header_buf,
                                 2);

}

bool bt_sendHeader(void)
{
    *(tx_buf)= 'O';
    *(tx_buf+1)= 'K';
    *(tx_buf+2)= vibration_state_data.current_state;
    *(tx_buf+6)=sound_current_state;
    *(tx_buf+10)= vibration_data.vibration_rms*10000;
    // float test_f = vibration_data.vibration_rms*10000;
    *(tx_buf+14)= vibration_data.vibration_peak*10000;
    // float test_f2 = vibration_data.vibration_rms*10000;

    *(tx_buf+18)=sound_level_result.rms;
    *(tx_buf+22)=sound_level_result.peak;
    *(tx_buf+24)= relay_state;
    *(tx_buf+25)=relay_state;
    *(tx_buf+26)=mpu_init_status;

    bt_tx_status =
        HAL_UART_Transmit_DMA(&huart1,
                              (uint8_t *)&tx_buf,
                              LED_DATA_SIZE);

    if(bt_tx_status == HAL_OK)
    {
        return true;
    }
    else 
    {
        return false;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance==USART1)
    {
        if(*(rx_header_buf)=='K'&&*(rx_header_buf)=='L')
        {
            relay_off();
            bt_rx_state = BT_RX_HEADER;
            HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
        }
    }
}



void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        bt_uart_error = huart->ErrorCode;
        bt_rx_state = BT_RX_HEADER;
        bt_request_active = false;

        bt_rx_status =
            HAL_UART_Receive_DMA(&huart1,
                                 rx_header_buf,
                                 HEADER_SIZE);
    }
}

