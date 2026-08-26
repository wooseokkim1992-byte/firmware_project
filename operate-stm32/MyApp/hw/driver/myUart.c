#include "myUart.h"
#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  /* Polling 방식으로 1바이트 전송 (전송 완료될 때까지 대기) */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

#define RX_BUF_SIZE 128
uint8_t rx_data;
uint8_t rx_buf[RX_BUF_SIZE];


void uartInit(void){
  //HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  // bt_AtIsOk(); // bt AT check ( just once )
  // bt_SetName(); // bt Set Name ( just once )
  // bt_SetPassword(); // bt set password  ( just once )
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE);
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

  if(huart->Instance==USART2){
    if(rx_data=='a')
      printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
    else
      HAL_UART_Transmit(&huart2, rx_buf, RX_BUF_SIZE, 100);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE);
  }
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if(huart->Instance==USART2){

    HAL_UART_Transmit(&huart2, rx_buf, Size, 100);

    HAL_UART_DMAStop(&huart2);
    memset(rx_buf,0,Size);

  }
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE);

}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
  HAL_UART_Receive_DMA(&huart2, rx_buf, RX_BUF_SIZE);
}
