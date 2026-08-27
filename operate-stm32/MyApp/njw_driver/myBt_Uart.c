#include "myBt_Uart.h"
#include "kws_display_manager.h"
#include "usart.h"
#include <string.h>

#define HEADER_SIZE sizeof(data_header_t)
#define LED_DATA_SIZE sizeof(lcd_display_data_t)

uint8_t rx_header_buf[HEADER_SIZE];
uint8_t rx_data_buf[LED_DATA_SIZE];

bt_rx_state_t bt_rx_state = BT_RX_HEADER;

void btInit(void){
    bt_rx_state = BT_RX_HEADER;

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                 rx_header_buf,
                                 HEADER_SIZE);

}

void bt_sendHeader(void)
{
    data_header_t tx = {'O','K',0};

    HAL_UART_Transmit(&huart1,
                  (uint8_t *)&tx,
                  sizeof(tx),
                  1000);
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance==USART1)
    {
        if(bt_rx_state == BT_RX_HEADER)
        {
            if(Size == HEADER_SIZE)
            {
                bt_DataCheck(rx_header_buf);
            }
        }
        else if(bt_rx_state == BT_RX_DATA)
        {
            if(Size == LED_DATA_SIZE)
            {
                lcd_display_data_t temp;
            
                memcpy(&temp, rx_data_buf, sizeof(temp));
                lcd_display_data = temp;
                bt_rx_state = BT_RX_HEADER;
                HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_header_buf, HEADER_SIZE);
            
            }
        }
    }
}

void bt_DataCheck(uint8_t *rx_buf)
{
    data_header_t *data_header;
    data_header = (data_header_t *)rx_buf;
    if(data_header->check_1=='O'&&data_header->check_2=='K')
    {
        data_header_t tx_head = {'T','T',0};
        HAL_UART_Transmit(&huart1,
          (uint8_t *)&tx_head,
          sizeof(tx_head),
          1000);
        bt_rx_state = BT_RX_DATA;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_data_buf, LED_DATA_SIZE);

    }
    if(data_header->check_1=='T'&&data_header->check_2=='T')
    {
        bt_SendLedData();
    }
}

void bt_SendLedData(void)
{
    lcd_display_data_t temp = {.state = NORMAL,

                                     .vibration_rms_mg = 777U,
                                     .vibration_peak_mg = 777U,

                                     .axis_x_rms_mg = 77U,
                                     .axis_y_rms_mg = 77U,
                                     .axis_z_rms_mg = 77U,

                                     .rpm = 77U,
                                     .sound_raw = 77U,

                                     .motor_running = true,
                                     .relay_on = true,
                                     .communication_ok = true,
                                     .mpu6050_ok = true,
                                     .dma_ok = true};

    HAL_UART_Transmit(&huart1,
                  (uint8_t *)&temp,
                  sizeof(lcd_display_data),
                  1000);
    bt_rx_state = BT_RX_HEADER;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_header_buf, HEADER_SIZE);
    
}