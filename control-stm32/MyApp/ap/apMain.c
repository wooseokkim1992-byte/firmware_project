#include "apMain.h"
#include "myI2c.h"

#include "hw/sjh_driver/mpu6050.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/*
 * ============================================================
 * Control STM32 - Application Main
 * ============================================================
 *
 * 현재 구현 기능
 *
 * 1.1 MPU6050 센서 제어
 *
 *
 * 향후 추가 예정
 *
 * 1.2 vibration.c
 *     - 오프셋 제거
 *     - 중력 성분 제거
 *     - 필터링
 *     - RMS
 *     - Peak
 *
 * 1.3 vibration_state.c
 *     - NORMAL
 *     - WARNING
 *     - DANGER
 *
 *
 * 그 외 팀 기능
 *
 * - Sound Sensor
 * - Photo Interrupter
 * - Relay
 * - 발전 전압 측정
 * - UART 관제 데이터 전송
 *
 * ============================================================
 */


/*
 * ============================================================
 * MPU6050 관련 변수
 * ============================================================
 */


/*
 * MPU6050에서 읽은 데이터
 *
 * Live Watch에서
 *
 * mpu_data.accel_x
 * mpu_data.accel_y
 * mpu_data.accel_z
 *
 * 등을 바로 확인할 수 있다.
 */
static mpu6050_data_t mpu_data = {0};


/*
 * MPU6050 초기화 결과
 *
 * true  : 초기화 성공
 * false : 초기화 실패
 */
static bool mpu_init_status = false;


/*
 * 가장 최근 데이터 읽기 결과
 *
 * true  : Read 성공
 * false : Read 실패
 */
static bool mpu_read_status = false;


/*
 * 데이터 읽기 성공 횟수
 *
 * Live Watch에서 값이 계속 증가하면
 * 센서가 지속적으로 읽히고 있다는 의미이다.
 */
static uint32_t mpu_read_count = 0U;


/*
 * 데이터 읽기 실패 횟수
 */
static uint32_t mpu_error_count = 0U;


/*
 * 연속 Read 실패 횟수
 *
 * 연속 3회 실패 시 MPU6050 재초기화를 시도한다.
 */
static uint8_t mpu_fail_count = 0U;


/*
 * MPU6050 드라이버 상태
 */
static mpu6050_status_t mpu_driver_status =
    MPU6050_STATUS_NOT_INITIALIZED;


/*
 * ============================================================
 * MPU6050 Sampling Period
 * ============================================================
 *
 * MPU6050 내부 Sample Rate
 *
 * 125 Hz
 *
 * 1000 ms / 125
 *
 * = 8 ms
 */
#define MPU_READ_PERIOD_MS       8U


/*
 * Debug printf 출력 주기
 *
 * MPU6050 데이터는 8ms마다 읽지만
 * UART 출력까지 8ms마다 하면 너무 많은 시간이 소모된다.
 *
 * 따라서 UART 출력은 1000ms마다 수행한다.
 */
#define DEBUG_PRINT_PERIOD_MS    1000U


/*
 * ============================================================
 * Application Initialization
 * ============================================================
 */
void apInit(void)
{
    /*
     * UART Application Driver 초기화
     *
     * USART2 Peripheral 자체는
     * Core/Src/main.c에서 이미 초기화되어 있다.
     */
    uartInit();

    /* I2C에 실제 연결된 장치 확인 */
    i2cScan();

    /*
     * MPU6050 초기화
     */
    mpu_init_status =
        mpu6050_init();


    /*
     * 현재 Driver 상태 저장
     */
    mpu_driver_status =
        mpu6050_get_status();


    /*
     * 초기화 성공
     */
    if (mpu_init_status)
    {
        printf("\r\n");
        printf("============================\r\n");
        printf(" Control STM32 Start\r\n");
        printf("============================\r\n");

        printf("[MPU6050] Init Success\r\n");

        printf(
            "[MPU6050] WHO_AM_I : 0x%02X\r\n",
            mpu6050_get_chip_id()
        );

        printf(
            "[MPU6050] Sample Rate : %lu Hz\r\n",
            (unsigned long)MPU6050_SAMPLE_RATE_HZ
        );
    }

    /*
     * 초기화 실패
     */
    else
    {
        printf("\r\n");

        printf("[MPU6050] Init Failed\r\n");

        printf(
            "[MPU6050] Status : %d\r\n",
            (int)mpu_driver_status
        );
    }
}


/*
 * ============================================================
 * Application Main
 * ============================================================
 */
void apMain(void)
{
    /*
     * 마지막 MPU6050 Read 시각
     */
    uint32_t tick_mpu = 0U;


    /*
     * 마지막 Debug 출력 시각
     */
    uint32_t tick_debug = 0U;


    /*
     * MPU6050 재초기화 시각
     */
    uint32_t tick_reinit = 0U;


    /*
     * 현재 시간
     */
    uint32_t current_tick = 0U;


    while (1)
    {
        /*
         * 현재 시스템 Tick
         *
         * 단위 : ms
         */
        current_tick =
            HAL_GetTick();


        /*
         * ====================================================
         * MPU6050 데이터 읽기
         * ====================================================
         *
         * 약 8ms마다 실행한다.
         *
         * 8ms ≈ 125Hz
         */
        if ((current_tick - tick_mpu)
            >= MPU_READ_PERIOD_MS)
        {
            tick_mpu = current_tick;


            /*
             * MPU6050이 정상 초기화된 경우에만
             * 데이터를 읽는다.
             */
            if (mpu_init_status)
            {
                mpu_read_status =
                    mpu6050_read(&mpu_data);


                /*
                 * Read 성공
                 */
                if (mpu_read_status)
                {
                    /*
                     * 성공 횟수 증가
                     */
                    mpu_read_count++;


                    /*
                     * 연속 오류 횟수 초기화
                     */
                    mpu_fail_count = 0U;
                }

                /*
                 * Read 실패
                 */
                else
                {
                    mpu_error_count++;

                    mpu_fail_count++;


                    /*
                     * 가장 최근 Driver 오류 상태 저장
                     */
                    mpu_driver_status =
                        mpu6050_get_status();


                    /*
                     * 3번 연속 Read 실패 시
                     * 센서 재초기화가 필요하다고 판단한다.
                     */
                    if (mpu_fail_count >= 3U)
                    {
                        mpu_init_status = false;

                        mpu_fail_count = 0U;

                        /*
                         * 재초기화 주기 시작점 저장
                         */
                        tick_reinit = current_tick;
                    }
                }
            }
        }


        /*
         * ====================================================
         * MPU6050 재초기화
         * ====================================================
         *
         * 초기화 실패 또는 반복 통신 오류가 발생한 경우
         *
         * 1초마다 재초기화를 시도한다.
         */
        if (!mpu_init_status)
        {
            if ((current_tick - tick_reinit)
                >= 1000U)
            {
                tick_reinit = current_tick;


                /*
                 * MPU6050 다시 초기화
                 */
                mpu_init_status =
                    mpu6050_reinit();


                /*
                 * Driver 상태 갱신
                 */
                mpu_driver_status =
                    mpu6050_get_status();


                /*
                 * 재초기화 성공 시
                 * Read 상태 초기화
                 */
                if (mpu_init_status)
                {
                    mpu_read_status = false;

                    mpu_fail_count = 0U;
                }
            }
        }


        /*
         * ====================================================
         * Debug 출력
         * ====================================================
         *
         * 센서는 8ms마다 읽지만
         * printf는 1초마다 한 번만 실행한다.
         *
         * printf는 Blocking 방식이기 때문에
         * 너무 자주 사용하면 진동 Sample 주기에 영향을 준다.
         *
         * 실제 vibration.c 개발 단계에서는
         * Debug printf를 더 줄이거나 제거하는 것이 좋다.
         */
        if ((current_tick - tick_debug)
            >= DEBUG_PRINT_PERIOD_MS)
        {
            tick_debug = current_tick;


            /*
             * 최근 센서 데이터가 정상인 경우
             */
            if (mpu_read_status)
            {
                printf(
                    ">acc_x:%.3f\r\n"
                    ">acc_y:%.3f\r\n"
                    ">acc_z:%.3f\r\n"
                    ">gyro_x:%.3f\r\n"
                    ">gyro_y:%.3f\r\n"
                    ">gyro_z:%.3f\r\n"
                    ">mpu_temp:%.2f\r\n",
                    mpu_data.accel_x,
                    mpu_data.accel_y,
                    mpu_data.accel_z,
                    mpu_data.gyro_x,
                    mpu_data.gyro_y,
                    mpu_data.gyro_z,
                    mpu_data.temp
                );
            }

            /*
             * MPU6050 초기화 실패 상태
             */
            else if (!mpu_init_status)
            {
                printf(
                    "[MPU6050] Error Status : %d\r\n",
                    (int)mpu_driver_status
                );
            }
        }


        /*
         * ====================================================
         * 향후 추가될 프로젝트 코드
         * ====================================================
         *
         * 다음 단계에서 직접 계산 코드를
         * 이 파일에 길게 작성하지 않는다.
         *
         * 각 기능을 별도 모듈로 만들고
         * 여기서는 함수만 호출한다.
         *
         * 예)
         *
         * vibration_update();
         *
         * vibration_state_update();
         *
         * rpm_update();
         *
         * sound_update();
         *
         * generator_voltage_update();
         *
         * uart_packet_update();
         */
    }
}