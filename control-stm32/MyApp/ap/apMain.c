#include "apMain.h"
#include "myI2c.h"

#include "hw/sjh_driver/mpu6050.h"
#include "hw/sjh_driver/vibration.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/*
 * ============================================================
 * Control STM32 - Application Main
 * ============================================================
 *
 * 현재 구현
 *
 * 1.1 MPU6050 센서 제어
 * 1.2 진동 데이터 처리
 *
 *
 * 데이터 흐름
 *
 * MPU6050
 *     ↓
 * accel_x / accel_y / accel_z
 *     ↓
 * vibration_update()
 *     ↓
 * Mean / RMS / Peak
 *     ↓
 * vibration_value
 *
 *
 * 다음 단계
 *
 * 1.3 vibration_state.c
 *
 * vibration_value를 이용하여
 *
 * NORMAL
 * WARNING
 * DANGER
 *
 * 상태를 판단한다.
 * ============================================================
 */


/*
 * MPU6050 데이터
 */
static mpu6050_data_t mpu_data = {0};


/*
 * 진동 처리 결과
 *
 * Live Watch에서
 *
 * vibration_data.vibration_rms
 * vibration_data.vibration_peak
 *
 * 등을 확인할 수 있다.
 */
static vibration_data_t vibration_data = {0};


/*
 * MPU6050 초기화 상태
 */
static bool mpu_init_status = false;


/*
 * MPU6050 Read 상태
 */
static bool mpu_read_status = false;


/*
 * MPU6050 Read 성공 횟수
 */
static uint32_t mpu_read_count = 0U;


/*
 * MPU6050 Read Error 횟수
 */
static uint32_t mpu_error_count = 0U;


/*
 * MPU6050 연속 실패 횟수
 */
static uint8_t mpu_fail_count = 0U;


/*
 * MPU6050 Driver 상태
 */
static mpu6050_status_t mpu_driver_status =
    MPU6050_STATUS_NOT_INITIALIZED;


/*
 * MPU6050 Sampling
 *
 * 125Hz
 *
 * = 약 8ms
 */
#define MPU_READ_PERIOD_MS       8U


/*
 * UART Debug 출력 주기
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
     * UART 초기화
     */
    uartInit();


    /*
     * 현재 I2C Device 확인.
     *
     * 프로젝트 초기 Debug 용도로 유지.
     */
    i2cScan();


    /*
     * MPU6050 초기화
     */
    mpu_init_status =
        mpu6050_init();


    /*
     * MPU6050 상태 확인
     */
    mpu_driver_status =
        mpu6050_get_status();


    /*
     * 진동 처리 모듈 초기화
     */
    vibration_init();


    /*
     * MPU6050 초기화 결과 출력
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


        printf(
            "[VIBRATION] Window : %u Samples\r\n",
            VIBRATION_WINDOW_SIZE
        );
    }
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
     * MPU6050 마지막 읽기 시간
     */
    uint32_t tick_mpu = 0U;


    /*
     * Debug 마지막 출력 시간
     */
    uint32_t tick_debug = 0U;


    /*
     * MPU6050 재초기화 시간
     */
    uint32_t tick_reinit = 0U;


    /*
     * 현재 System Tick
     */
    uint32_t current_tick = 0U;


    while (1)
    {
        /*
         * 현재 시간
         *
         * 단위 : ms
         */
        current_tick =
            HAL_GetTick();


        /*
         * ====================================================
         * MPU6050 Sampling
         * ====================================================
         *
         * 약 8ms마다 실행
         *
         * ≈ 125Hz
         */
        if ((current_tick - tick_mpu)
            >= MPU_READ_PERIOD_MS)
        {
            tick_mpu = current_tick;


            /*
             * MPU6050 초기화 성공 상태에서만 Read
             */
            if (mpu_init_status)
            {
                mpu_read_status =
                    mpu6050_read(&mpu_data);


                /*
                 * MPU6050 Read 성공
                 */
                if (mpu_read_status)
                {
                    mpu_read_count++;


                    mpu_fail_count = 0U;


                    /*
                     * ==========================================
                     * 1.2 진동 데이터 처리
                     * ==========================================
                     *
                     * MPU6050에서 읽은
                     *
                     * accel_x
                     * accel_y
                     * accel_z
                     *
                     * 를 vibration 모듈에 전달한다.
                     *
                     * 반환값 true는
                     * 64 Sample Window 결과가 새로 만들어졌다는 뜻.
                     */
                    vibration_update(
                        mpu_data.accel_x,
                        mpu_data.accel_y,
                        mpu_data.accel_z
                    );


                    /*
                     * Live Watch / 관제에서 사용할 수 있도록
                     * 최신 결과를 Application 변수로 복사한다.
                     */
                    vibration_get_data(
                        &vibration_data
                    );
                }

                /*
                 * MPU6050 Read 실패
                 */
                else
                {
                    mpu_error_count++;

                    mpu_fail_count++;


                    mpu_driver_status =
                        mpu6050_get_status();


                    /*
                     * 3회 연속 실패
                     */
                    if (mpu_fail_count >= 3U)
                    {
                        mpu_init_status = false;

                        mpu_fail_count = 0U;

                        tick_reinit = current_tick;
                    }
                }
            }
        }


        /*
         * ====================================================
         * MPU6050 재초기화
         * ====================================================
         */
        if (!mpu_init_status)
        {
            if ((current_tick - tick_reinit)
                >= 1000U)
            {
                tick_reinit = current_tick;


                mpu_init_status =
                    mpu6050_reinit();


                mpu_driver_status =
                    mpu6050_get_status();


                if (mpu_init_status)
                {
                    mpu_read_status = false;

                    mpu_fail_count = 0U;


                    /*
                     * 센서가 재초기화됐으므로
                     * 기존 진동 Filter 기준값도 다시 초기화한다.
                     */
                    vibration_init();
                }
            }
        }


        /*
         * ====================================================
         * Debug Print
         * ====================================================
         *
         * Sensor Sampling은 8ms이지만
         * UART Print는 1초마다 실행한다.
         */
        if ((current_tick - tick_debug)
            >= DEBUG_PRINT_PERIOD_MS)
        {
            tick_debug = current_tick;


            if (mpu_read_status)
            {
                /*
                 * MPU6050 원본 가속도
                 */
                printf(
                    ">acc_x:%.4f\r\n"
                    ">acc_y:%.4f\r\n"
                    ">acc_z:%.4f\r\n",
                    mpu_data.accel_x,
                    mpu_data.accel_y,
                    mpu_data.accel_z
                );


                /*
                 * 진동 처리 결과
                 *
                 * Teleplot에서도 바로 확인 가능.
                 */
                printf(
                    ">vibration:%.5f\r\n"
                    ">vibration_mean:%.5f\r\n"
                    ">vibration_rms:%.5f\r\n"
                    ">vibration_peak:%.5f\r\n",
                    vibration_data.vibration_magnitude,
                    vibration_data.vibration_mean,
                    vibration_data.vibration_rms,
                    vibration_data.vibration_peak
                );
            }

            else if (!mpu_init_status)
            {
                printf(
                    "[MPU6050] Error Status : %d\r\n",
                    (int)mpu_driver_status
                );
            }
        }
    }
}