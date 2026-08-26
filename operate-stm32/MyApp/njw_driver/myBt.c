#include "myBt.h"

// HC-05 AT 확인 코드
void bt_AtIsOk(void){
    uint8_t tx[] = "AT\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

// HC-05 AT set name
void bt_SetName(void){
    uint8_t tx[] = "AT+NAME=fan\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

// HC-05 AT set password
void bt_SetPassword(void){
    uint8_t tx[] = "AT+PSWD=8888\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

// HC-05 AT set master
void bt_SetRole(void){
    uint8_t tx[] = "AT+ROLE=1\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

// HC-05 AT set master
void bt_SetTarget(void){
    uint8_t tx[] = "AT+CMODE=0\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

// HC-05 SPP Init
void bt_SPPInit(void){
    uint8_t tx[] = "AT+INIT\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}

AT+INQ

// HC-05 SPP Init
void bt_SPPInit(void){
    uint8_t tx[] = "AT+INIT\r\n";
    uint8_t rx[16] = {0};

    HAL_StatusTypeDef tx_status;
    HAL_StatusTypeDef rx_status;

    tx_status = HAL_UART_Transmit(&huart1,
                                  tx,
                                  sizeof(tx) - 1,
                                  1000);

    rx_status = HAL_UART_Receive(&huart1,
                                 rx,
                                 4,
                                 2000);

    printf("TX status = %d\r\n", tx_status);
    printf("RX status = %d\r\n", rx_status);

    if (rx_status == HAL_OK)
    {
        printf("RX = %c%c%c%c\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    }

}