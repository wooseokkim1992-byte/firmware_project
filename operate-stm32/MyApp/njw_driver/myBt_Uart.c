#include "myBt_Uart.h"
#include "gpio.h"
#include "kws_display_manager.h"
#include "kws_display_type.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include <string.h>

#define HEADER_SIZE sizeof(data_header_t)
#define LED_DATA_SIZE sizeof(lcd_display_data_t)
#define BT_REQUEST_TIMEOUT 1000

static data_header_t tx_ok = {'O', 'K', 0};
static data_header_t tx_tt = {'T', 'T', 0};
static uint32_t bt_request_tick = 0;

volatile bt_rx_state_t bt_rx_state = BT_RX_HEADER;
volatile HAL_StatusTypeDef bt_tx_status;
volatile HAL_StatusTypeDef bt_rx_status;
static volatile bool bt_request_active = false;
volatile uint32_t bt_uart_error = 0;

uint8_t rx_header_buf[HEADER_SIZE];
uint8_t rx_data_buf[LED_DATA_SIZE];

void btInit(void) {
  bt_rx_state = BT_RX_HEADER;

  bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
}

void bt_sendHeader(void) {
  if (bt_rx_state != BT_RX_HEADER) {
    return;
  }

  if (bt_request_active) {
    if (HAL_GetTick() - bt_request_tick < BT_REQUEST_TIMEOUT) {
      return;
    }

    // 1초 동안 TT가 안 왔으면 다시 시도 가능
    bt_request_active = false;
  }

  if (huart1.gState != HAL_UART_STATE_READY) {
    return;
  }

  bt_tx_status = HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_ok, HEADER_SIZE);

  if (bt_tx_status == HAL_OK) {
    bt_request_active = true;
    bt_request_tick = HAL_GetTick();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (bt_rx_state == BT_RX_HEADER) {
      bt_DataCheck(rx_header_buf);
    } else if (bt_rx_state == BT_RX_DATA) {

      lcd_display_data_t temp;

      memcpy(&temp, rx_data_buf, sizeof(temp));
      lcd_display_data = temp;
      bt_rx_state = BT_RX_HEADER;
      bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);

    } else {
      bt_rx_state = BT_RX_HEADER;
      bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
    }
  }
}

void bt_DataCheck(uint8_t *rx_buf) {
  data_header_t *data_header;
  data_header = (data_header_t *)rx_buf;
  if (data_header->check_1 == 'O' && data_header->check_2 == 'K') {
    bt_tx_status =
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_tt, HEADER_SIZE);

    if (bt_tx_status == HAL_OK) {
      bt_rx_state = BT_RX_DATA;

      bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_data_buf, LED_DATA_SIZE);
    } else {
      bt_rx_state = BT_RX_HEADER;

      bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
    }

  } else if (data_header->check_1 == 'T' && data_header->check_2 == 'T') {
    bt_request_active = false;

    bt_SendLedData();
  } else if (data_header->check_1 == 'K' && data_header->check_2 == 'L') {
    // motor_kill();
    bt_rx_state = BT_RX_HEADER;
    HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);

  } else {
    bt_rx_state = BT_RX_HEADER;

    bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
  }
}

void bt_SendLedData(void) {
  static lcd_display_data_t temp = {
      .vibe_state = NORMAL,
      .sound_state = NORMAL,
      .vibration_rms_mg = 777U,
      .vibration_peak_mg = 777U,
      .sound_peak = 8484U,
      .sound_rms = 6.7,

      .motor_running = true,
      .relay_on = true,
      .communication_ok = true,
      .mpu6050_ok = true,
  };

  bt_tx_status = HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&temp, sizeof(temp));
  bt_rx_state = BT_RX_HEADER;
  bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    bt_uart_error = huart->ErrorCode;
    bt_rx_state = BT_RX_HEADER;
    bt_request_active = false;

    bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
  }
}

void bt_SendKill(void) {
  static data_header_t tx_kill = {'K', 'L', 0};

  HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_kill, HEADER_SIZE);
}