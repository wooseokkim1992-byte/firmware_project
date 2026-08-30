#include "myBt_Uart.h"
#include "kws_display_type.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
static data_header_t tx_ok = {
    'O',
    'K',
};
static data_header_t tx_tt = {
    'T',
    'T',
};
static uint32_t bt_request_tick = 0;

volatile bt_rx_state_t bt_rx_state = BT_RX_HEADER;
volatile HAL_StatusTypeDef bt_tx_status;
volatile HAL_StatusTypeDef bt_rx_status;
static volatile bool bt_request_active = false;
volatile uint32_t bt_uart_error = 0;
volatile uint8_t failed_to_receive_ok_cnt = 0;
uint8_t rx_header_buf[HEADER_SIZE];
volatile uint8_t rx_data_buf[LED_DATA_SIZE];

extern volatile bool kill_request;
volatile lcd_display_data_t lcd_display_data = {
    0,
};
volatile lcd_display_data_t1 test;
volatile bool is_dma_busy = false;
volatile bool is_initialized_display_data;
static bool is_ok_received = false;

void btInit(void) {
  bt_rx_state = BT_RX_HEADER;
  bt_rx_status =
      HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_data_buf, LED_DATA_SIZE);
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
    parsing_data();
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_data_buf, LED_DATA_SIZE);
    // if (bt_rx_state == BT_RX_HEADER) {
    //   bt_DataCheck(rx_header_buf);
    // } else if (bt_rx_state == BT_RX_DATA) {
    //   // memcpy(&temp, rx_data_buf, sizeof(temp));
    //   parsing_data();
    //   // test = temp;
    //   bt_rx_state = BT_RX_HEADER;
    //   bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf,
    //   HEADER_SIZE);
    //   // if (!is_initialized_display_data) {
    //   //   is_initialized_display_data = true;
    //   // }

    // } else {
    //   bt_rx_state = BT_RX_HEADER;
    //   bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf,
    //   HEADER_SIZE);
    // }
  }
}

void bt_DataCheck(uint8_t *rx_buf) {
  data_header_t *data_header;
  data_header = (data_header_t *)rx_buf;
  if (data_header->check_1 == 'O' && data_header->check_2 == 'K') {
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_tt, HEADER_SIZE) ==
        HAL_OK) {
      // Operator 쪽에서 TT 전송후 TT 수신 대기
      HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_header_buf, 2);
    } else {
      bt_rx_state = BT_RX_HEADER;
      bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
    }
  } else if (data_header->check_1 == 'T' && data_header->check_2 == 'T') {
    bt_rx_state = BT_RX_DATA;
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_data_buf, LED_DATA_SIZE);

    // bt_SendLedData();
  } else if (data_header->check_1 == 'K' && data_header->check_2 == 'L') {
    // motor_kill();
    bt_rx_state = BT_RX_HEADER;
    HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
  } else {
    bt_rx_state = BT_RX_HEADER;
    bt_rx_status = HAL_UART_Receive_DMA(&huart1, rx_header_buf, HEADER_SIZE);
  }
}

// void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
//   if (huart->Instance == USART1) {
//     if (rx_header_buf[0] == 'T' && rx_header_buf[1] == 'T') {
//       // Control 으로 부터 T T 를 받으면 그때부터 데이터 송신
//       if (HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_data_buf,
//                                LED_DATA_SIZE) == HAL_OK) {
//         bt_rx_state = BT_RX_DATA;
//       }
//     } else if (rx_header_buf[0] == 'O' && rx_header_buf[1] == 'K') {
//       if (failed_to_receive_ok_cnt < 3) {
//         bt_rx_state = BT_RX_HEADER;
//         HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_tt, HEADER_SIZE);
//         HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)&rx_header_buf, 2);
//         failed_to_receive_ok_cnt++;
//       } else {
//         failed_to_receive_ok_cnt = 0;
//         bt_rx_state = BT_RX_HEADER;
//         HAL_UART_Receive_DMA(&huart1, rx_header_buf, 2);
//       }
//     }
//   }
// }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    bt_uart_error = huart->ErrorCode;
    bt_rx_state = BT_RX_HEADER;
    bt_request_active = false;
    is_dma_busy = false;

    bt_rx_status =
        HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_data_buf, LED_DATA_SIZE);
  }
}

void bt_SendKill(void) {
  static data_header_t tx_kill = {
      'K',
      'L',
  };

  HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_kill, HEADER_SIZE);
}

void parsing_data() {
  uint8_t *handle = (uint8_t *)rx_data_buf;
  if ((char)rx_data_buf[0] == 'O' && (char)rx_data_buf[1] == 'K') {
    if (!is_initialized_display_data) {
      is_initialized_display_data = true;
    }
    handle += 2;
    lcd_display_data.vibe_state = (system_state_t)(*handle);
    handle += 4;
    lcd_display_data.sound_state = (system_state_t)(*handle);
    handle += 4;
    lcd_display_data.vibration_rms_mg = (float)(*handle);
    handle += 4;
    lcd_display_data.vibration_peak_mg = (float)(*handle);
    handle += 4;
    lcd_display_data.sound_rms = (float)(*handle);
    handle += 4;
    lcd_display_data.sound_peak = (uint16_t)(*handle);
    handle += 2;
    lcd_display_data.motor_running = (bool)(*handle);
    handle += 1;
    lcd_display_data.relay_on = (bool)(*handle);
    handle += 1;
    lcd_display_data.mpu6050_ok = (bool)(*handle);
  }
}
