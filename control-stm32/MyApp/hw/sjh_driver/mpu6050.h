#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>


/*
 * ============================================================
 * MPU6050 Driver
 * ============================================================
 *
 * 담당 기능 : 중분류 1.1 MPU6050 센서 제어
 *
 * 주요 역할
 * 1. I2C 통신
 * 2. WHO_AM_I 센서 연결 확인
 * 3. Sleep 해제 및 동작 모드 설정
 * 4. Sample Rate / DLPF 설정
 * 5. 가속도 X/Y/Z 데이터 취득
 * 6. 자이로 X/Y/Z 데이터 취득
 * 7. 내부 온도 데이터 취득
 * 8. Raw 데이터를 실제 단위로 변환
 * 9. 통신 오류 상태 관리
 *
 *
 * ※ 이 모듈에서는 진동 분석을 하지 않는다.
 *
 * 중력 제거
 * 오프셋 제거
 * RMS
 * Peak
 * NORMAL / WARNING / DANGER
 *
 * 등의 처리는 이후 vibration.c / vibration_state.c에서 담당한다.
 *
 *
 * 데이터 흐름
 *
 * MPU6050
 *    ↓
 * mpu6050.c
 *    ↓
 * accel_x / accel_y / accel_z
 *    ↓
 * vibration.c
 *    ↓
 * RMS / Peak
 *    ↓
 * vibration_state.c
 *
 * ============================================================
 */


/*
 * MPU6050 내부 Sample Rate
 *
 * DLPF 사용 시 기본 출력 주파수 = 1 kHz
 *
 * SMPLRT_DIV = 7
 *
 * 1000 / (1 + 7)
 * = 125 Hz
 *
 * 즉 약 8 ms마다 새로운 센서 데이터가 생성된다.
 */
#define MPU6050_SAMPLE_RATE_HZ    125U


/*
 * MPU6050 드라이버 상태
 *
 * 단순한 true / false 외에
 * 오류 원인을 확인하기 위해 사용한다.
 */
typedef enum
{
    /* 정상 */
    MPU6050_STATUS_OK = 0,

    /* 아직 초기화되지 않음 */
    MPU6050_STATUS_NOT_INITIALIZED,

    /* 잘못된 포인터 등 인자 오류 */
    MPU6050_STATUS_INVALID_ARGUMENT,

    /* I2C 주소에서 센서를 찾지 못함 */
    MPU6050_STATUS_DEVICE_NOT_FOUND,

    /* WHO_AM_I 값 오류 */
    MPU6050_STATUS_WHO_AM_I_ERROR,

    /* 센서 설정 중 통신 오류 */
    MPU6050_STATUS_CONFIG_ERROR,

    /* 초기화 또는 데이터 읽기 중 I2C 오류 */
    MPU6050_STATUS_I2C_ERROR,

    /* 읽은 데이터가 비정상적 */
    MPU6050_STATUS_DATA_ERROR

} mpu6050_status_t;


/*
 * MPU6050 측정 데이터 구조체
 *
 * 기존 수업 코드에서 사용했던 변수명을 최대한 유지한다.
 *
 * Raw 값과 실제 단위 변환 값을 모두 저장하여
 * Live Watch와 디버깅에서 확인할 수 있도록 한다.
 */
typedef struct
{
    /*
     * --------------------------------------------------------
     * Accelerometer Raw
     * --------------------------------------------------------
     */
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;


    /*
     * --------------------------------------------------------
     * MPU6050 내부 온도 Raw
     * --------------------------------------------------------
     *
     * A플랜:
     * 읽어서 저장하지만 진동 분석에서는 사용하지 않는다.
     */
    int16_t temp_raw;


    /*
     * --------------------------------------------------------
     * Gyroscope Raw
     * --------------------------------------------------------
     */
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;


    /*
     * --------------------------------------------------------
     * Accelerometer 실제값
     * --------------------------------------------------------
     *
     * 단위 : g
     *
     * ±2g 설정에서는
     * 16384 LSB = 1g
     *
     * 이후 vibration.c에서 핵심적으로 사용할 데이터이다.
     */
    float accel_x;
    float accel_y;
    float accel_z;


    /*
     * --------------------------------------------------------
     * MPU6050 내부 온도
     * --------------------------------------------------------
     *
     * 단위 : °C
     *
     * 주변 공기 온도가 아니라
     * MPU6050 IC 내부 온도이다.
     *
     * 이번 프로젝트에서는 참고 / 디버깅용으로만 저장한다.
     */
    float temp;


    /*
     * --------------------------------------------------------
     * Gyroscope 실제값
     * --------------------------------------------------------
     *
     * 단위 : deg/s (dps)
     */
    float gyro_x;
    float gyro_y;
    float gyro_z;


    /*
     * 가장 최근 데이터 유효 여부
     *
     * true  : 정상 데이터
     * false : 데이터 읽기 실패
     */
    bool is_valid;

} mpu6050_data_t;


/*
 * MPU6050 초기화
 *
 * 성공 : true
 * 실패 : false
 */
bool mpu6050_init(void);


/*
 * MPU6050 재초기화
 *
 * 통신 오류가 반복될 경우 사용한다.
 */
bool mpu6050_reinit(void);


/*
 * MPU6050 연결 확인
 *
 * I2C 응답 + WHO_AM_I를 확인한다.
 */
bool mpu6050_is_connected(void);


/*
 * MPU6050 데이터 읽기
 *
 * Accel X/Y/Z
 * Temp
 * Gyro X/Y/Z
 *
 * 총 14byte를 한 번에 읽는다.
 */
bool mpu6050_read(mpu6050_data_t *data);


/*
 * 가장 최근 MPU6050 드라이버 상태 반환
 */
mpu6050_status_t mpu6050_get_status(void);


/*
 * WHO_AM_I에서 읽은 Chip ID 반환
 *
 * 정상 MPU6050 = 0x68
 */
uint8_t mpu6050_get_chip_id(void);


/* 가장 최근 STM32 HAL I2C ErrorCode 반환 */
uint32_t mpu6050_get_last_i2c_error(void);


/* 마지막으로 접근에 실패한 Register. 주소 탐색 실패는 0xFF */
uint8_t mpu6050_get_failed_register(void);


#endif
