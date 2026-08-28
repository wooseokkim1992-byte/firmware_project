#include "relay.h"
#include "main.h"
#include "usart.h"

#define RELAY_PORT   GPIOB
#define RELAY_PIN    GPIO_PIN_13
#define RX_SIZE      64U

static uint8_t rx_buffer[RX_SIZE];
static uint16_t previous_position;
static uint8_t previous_k;

void relay_off(void)
{
    /* LOW에서 켜지는 릴레이이므로 HIGH가 OFF */
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_SET);
}

void relay_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    relay_off();

    gpio.Pin = RELAY_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(RELAY_PORT, &gpio);
}

HAL_StatusTypeDef relay_receive_start(void)
{
    previous_position = 0U;
    previous_k = 0U;

    HAL_StatusTypeDef result =
        HAL_UARTEx_ReceiveToIdle_DMA(
            &huart6, rx_buffer, RX_SIZE);

    if (result != HAL_OK) {
        relay_off();
    }

    return result;
}

static void receive_byte(uint8_t byte)
{
    /* 관제에서 보내는 {'K', 'L', 0}의 KL 감지 */
    if (previous_k && byte == 'L') {
        relay_off();
    }

    previous_k = (byte == 'K');
}

void relay_receive_event(UART_HandleTypeDef *huart,
                         uint16_t position)
{
    if (huart != &huart6) {
        return;
    }

    if (position == 0U || position > RX_SIZE) {
        relay_off();
        return;
    }

    if (position == previous_position) {
        return;
    }

    /* Circular DMA 버퍼가 처음으로 돌아온 경우 */
    if (position < previous_position) {
        while (previous_position < RX_SIZE) {
            receive_byte(rx_buffer[previous_position++]);
        }

        previous_position = 0U;
    }

    while (previous_position < position) {
        receive_byte(rx_buffer[previous_position++]);
    }
}

void relay_receive_error(UART_HandleTypeDef *huart)
{
    if (huart == &huart6) {
        previous_k = 0U;
        relay_off();
    }
}