#include "relay.h"
#include "main.h"
#include "usart.h"

#define RELAY_PORT GPIOC
#define RELAY_PIN  GPIO_PIN_0
#define RX_SIZE    64U

static uint8_t rx_buffer[RX_SIZE];
static uint16_t previous_position = 0U;
static bool previous_k = false;


/*
 * ============================================================
 * Relay ON
 * ============================================================
 *
 * 실제 하드웨어 테스트 결과
 *
 * PC0 SET
 * → Relay 상태 변경
 * → Motor ON
 */
void relay_on(void)
{
    HAL_GPIO_WritePin(
        RELAY_PORT,
        RELAY_PIN,
        GPIO_PIN_SET
    );
}


/*
 * ============================================================
 * Relay OFF
 * ============================================================
 *
 * 실제 하드웨어 테스트 결과
 *
 * PC0 RESET
 * → Relay 상태 변경
 * → Motor OFF
 */
void relay_off(void)
{
    HAL_GPIO_WritePin(
        RELAY_PORT,
        RELAY_PIN,
        GPIO_PIN_RESET
    );
}


/*
 * ============================================================
 * Relay 초기화
 * ============================================================
 */
void relay_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /*
     * PC0을 사용하므로
     * GPIOC Clock 활성화
     */
    __HAL_RCC_GPIOC_CLK_ENABLE();


    gpio.Pin = RELAY_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(RELAY_PORT, &gpio);


    /*
     * 시스템 정상 시작 시
     * 모터 ON
     *
     * 이후 DANGER 또는 Emergency Stop 발생 시
     * relay_off()로 정지한다.
     */
    relay_on();
}


/*
 * ============================================================
 * Motor Stop 판단
 * ============================================================
 *
 * danger = true
 * 또는
 * emergency_stop = true
 *
 * 이면 Motor OFF
 */
void relay_check_stop(bool danger,
                      bool emergency_stop)
{
    if (danger || emergency_stop)
    {
        relay_off();
    }
}


/*
 * ============================================================
 * USART6 DMA 수신 시작
 * ============================================================
 */
HAL_StatusTypeDef relay_receive_start(void)
{
    previous_position = 0U;
    previous_k = false;


    HAL_StatusTypeDef result =
        HAL_UARTEx_ReceiveToIdle_DMA(
            &huart6,
            rx_buffer,
            RX_SIZE
        );


    /*
     * UART 수신 시작 자체가 실패하면
     * 안전을 위해 Motor OFF
     */
    if (result != HAL_OK)
    {
        relay_off();
    }


    return result;
}


/*
 * ============================================================
 * 수신 Byte 처리
 * ============================================================
 *
 * 관제 STM32에서
 *
 * K → L
 *
 * 순서의 Emergency Stop Header가 들어오면
 * Motor OFF
 */
static void receive_byte(uint8_t byte)
{
    if (previous_k &&
        byte == (uint8_t)'L')
    {
        relay_off();
    }


    previous_k =
        (byte == (uint8_t)'K');
}


/*
 * ============================================================
 * USART6 DMA Receive Event
 * ============================================================
 */
void relay_receive_event(
    UART_HandleTypeDef *huart,
    uint16_t position)
{
    /*
     * USART6가 아니면 무시
     */
    if (huart != &huart6)
    {
        return;
    }


    /*
     * 잘못된 위치 값
     *
     * 통신 데이터가 정상적이지 않으므로
     * 안전을 위해 Motor OFF
     */
    if (position == 0U ||
        position > RX_SIZE)
    {
        relay_off();
        return;
    }


    /*
     * 이전과 같은 위치면
     * 새 데이터가 없음
     */
    if (position == previous_position)
    {
        return;
    }


    /*
     * Circular DMA Buffer가
     * 끝까지 갔다가 처음으로 돌아온 경우
     */
    if (position < previous_position)
    {
        while (previous_position < RX_SIZE)
        {
            receive_byte(
                rx_buffer[previous_position]
            );

            previous_position++;
        }


        previous_position = 0U;
    }


    /*
     * 새롭게 들어온 Byte 처리
     */
    while (previous_position < position)
    {
        receive_byte(
            rx_buffer[previous_position]
        );

        previous_position++;
    }
}


/*
 * ============================================================
 * USART6 Error
 * ============================================================
 */
void relay_receive_error(
    UART_HandleTypeDef *huart)
{
    if (huart == &huart6)
    {
        previous_k = false;

        /*
         * 통신 오류 발생 시
         * 안전을 위해 Motor OFF
         */
        relay_off();
    }
}