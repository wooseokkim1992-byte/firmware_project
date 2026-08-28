#include "mpu6050.h"

#include "i2c.h"
#include "stm32f4xx_hal.h"


/*
 * ============================================================
 * MPU6050 / MPU6500 Compatible Driver
 * ============================================================
 *
 * 프로젝트에서는 MPU6050 모듈 명칭을 사용하지만,
 * WHO_AM_I 값을 확인하여 다음 두 장치를 지원한다.
 *
 * MPU6050 : WHO_AM_I = 0x68
 * MPU6500 : WHO_AM_I = 0x70
 *
 * 현재 실제 사용 중인 센서는 WHO_AM_I 값이 0x70으로
 * 확인되었으므로 MPU6500 계열로 동작한다.
 *
 * 주요 역할
 * ------------------------------------------------------------
 * 1. I2C 장치 탐색
 * 2. WHO_AM_I 확인
 * 3. 센서 Reset / Sleep 해제
 * 4. Sample Rate 설정
 * 5. DLPF 설정
 * 6. Accelerometer / Gyroscope 범위 설정
 * 7. Accel / Temp / Gyro 데이터 읽기
 * 8. Raw 데이터를 실제 단위로 변환
 *
 * 진동 RMS / Peak / 상태 판정은
 * 이 파일에서 처리하지 않는다.
 *
 * 이후 vibration.c / vibration_state.c에서 처리한다.
 * ============================================================
 */


/*
 * ============================================================
 * MPU6050 I2C Address
 * ============================================================
 *
 * AD0 핀 상태에 따라 I2C 주소가 달라진다.
 *
 * AD0 = LOW  → 7bit Address = 0x68
 * AD0 = HIGH → 7bit Address = 0x69
 *
 * STM32 HAL I2C 함수에서는
 * 7bit Address를 왼쪽으로 1bit Shift한 값을 사용한다.
 */
#define MPU6050_ADDRESS_LOW          (0x68U << 1)
#define MPU6050_ADDRESS_HIGH         (0x69U << 1)


/*
 * ============================================================
 * Register Address
 * ============================================================
 */

/*
 * Sample Rate Divider
 */
#define MPU6050_REG_SMPLRT_DIV       0x19U


/*
 * Gyroscope DLPF 설정
 *
 * MPU6050에서는 Accel / Gyro Filter 설정에 사용되고,
 * MPU6500에서는 Gyroscope Filter 설정에 사용한다.
 */
#define MPU6050_REG_CONFIG           0x1AU


/*
 * Gyroscope Full Scale 설정
 */
#define MPU6050_REG_GYRO_CONFIG      0x1BU


/*
 * Accelerometer Full Scale 설정
 */
#define MPU6050_REG_ACCEL_CONFIG     0x1CU


/*
 * MPU6500 Accelerometer DLPF 설정
 *
 * MPU6500에서는 Accelerometer DLPF가
 * 별도의 ACCEL_CONFIG2 레지스터에 존재한다.
 *
 * MPU6050에서는 사용하지 않는다.
 */
#define MPU6500_REG_ACCEL_CONFIG2    0x1DU


/*
 * 센서 데이터 시작 주소
 *
 * 0x3B부터 14byte 연속 Read
 *
 * Accel X
 * Accel Y
 * Accel Z
 * Temperature
 * Gyro X
 * Gyro Y
 * Gyro Z
 *
 * 순서로 읽을 수 있다.
 */
#define MPU6050_REG_ACCEL_XOUT_H     0x3BU


/*
 * Power Management
 */
#define MPU6050_REG_PWR_MGMT_1       0x6BU
#define MPU6050_REG_PWR_MGMT_2       0x6CU


/*
 * WHO_AM_I
 */
#define MPU6050_REG_WHO_AM_I         0x75U


/*
 * ============================================================
 * WHO_AM_I Value
 * ============================================================
 *
 * 실제 센서 종류를 확인하기 위한 Chip ID이다.
 */

#define MPU6050_WHO_AM_I_VALUE       0x68U
#define MPU6500_WHO_AM_I_VALUE       0x70U


/*
 * ============================================================
 * I2C Timeout
 * ============================================================
 *
 * 단위 : ms
 */
#define MPU6050_I2C_TIMEOUT_MS       100U


/*
 * ============================================================
 * Scale Factor
 * ============================================================
 */


/*
 * Accelerometer
 *
 * 현재 측정 범위 : ±2g
 *
 * 16384 LSB = 1g
 */
#define MPU6050_ACCEL_SCALE          8192.0f


/*
 * Gyroscope
 *
 * 현재 측정 범위 : ±250 deg/s
 *
 * 131 LSB = 1 deg/s
 */
#define MPU6050_GYRO_SCALE           131.0f


/*
 * ============================================================
 * 내부 상태 변수
 * ============================================================
 *
 * static 변수이므로
 * mpu6050.c 내부에서만 직접 접근할 수 있다.
 */


/*
 * 센서 초기화 성공 여부
 */
static bool is_initialized = false;


/*
 * 현재 사용 중인 I2C Address
 */
static uint16_t device_address = 0U;


/*
 * WHO_AM_I에서 읽은 Chip ID
 *
 * MPU6050 → 0x68
 * MPU6500 → 0x70
 */
static uint8_t chip_id = 0U;


/*
 * 가장 최근 Driver 상태
 */
static mpu6050_status_t current_status =
    MPU6050_STATUS_NOT_INITIALIZED;


/*
 * ============================================================
 * 내부 함수
 * ============================================================
 */


/*
 * MPU6050 Register 1byte Write
 */
static HAL_StatusTypeDef mpu6050_write_register(uint8_t reg,
                                                uint8_t data)
{
    return HAL_I2C_Mem_Write(
        &hi2c1,
        device_address,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1U,
        MPU6050_I2C_TIMEOUT_MS
    );
}


/*
 * MPU6050 Register 1byte Read
 */
static HAL_StatusTypeDef mpu6050_read_register(uint8_t reg,
                                               uint8_t *data)
{
    return HAL_I2C_Mem_Read(
        &hi2c1,
        device_address,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        1U,
        MPU6050_I2C_TIMEOUT_MS
    );
}


/*
 * 여러 Register를 연속으로 읽는다.
 *
 * 이번 프로젝트에서는
 * Accel → Temp → Gyro
 *
 * 총 14byte를 한 번에 읽는다.
 */
static HAL_StatusTypeDef mpu6050_read_registers(uint8_t start_reg,
                                                uint8_t *data,
                                                uint16_t length)
{
    return HAL_I2C_Mem_Read(
        &hi2c1,
        device_address,
        start_reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        length,
        MPU6050_I2C_TIMEOUT_MS
    );
}


/*
 * ============================================================
 * Chip ID 확인
 * ============================================================
 *
 * 이번 프로젝트에서는
 *
 * MPU6050 : 0x68
 * MPU6500 : 0x70
 *
 * 두 ID를 모두 정상 장치로 인정한다.
 */
static bool mpu6050_is_valid_chip_id(uint8_t id)
{
    return (
        id == MPU6050_WHO_AM_I_VALUE ||
        id == MPU6500_WHO_AM_I_VALUE
    );
}


/*
 * ============================================================
 * 특정 I2C Address 확인
 * ============================================================
 *
 * 1. I2C ACK 확인
 * 2. WHO_AM_I 읽기
 * 3. 지원하는 Chip ID인지 확인
 */
static bool mpu6050_check_address(uint16_t address)
{
    uint8_t who_am_i = 0U;


    /*
     * --------------------------------------------------------
     * 1. I2C 장치 응답 확인
     * --------------------------------------------------------
     */
    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            address,
            3U,
            MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return false;
    }


    /*
     * 이후 Register Read / Write에 사용할
     * Address 저장
     */
    device_address = address;


    /*
     * --------------------------------------------------------
     * 2. WHO_AM_I 읽기
     * --------------------------------------------------------
     */
    if (mpu6050_read_register(
            MPU6050_REG_WHO_AM_I,
            &who_am_i) != HAL_OK)
    {
        return false;
    }


    /*
     * Live Watch 등에서 확인할 수 있도록
     * 실제 Chip ID 저장
     */
    chip_id = who_am_i;


    /*
     * --------------------------------------------------------
     * 3. Chip ID 검사
     * --------------------------------------------------------
     *
     * MPU6050 : 0x68
     * MPU6500 : 0x70
     */
    if (!mpu6050_is_valid_chip_id(who_am_i))
    {
        return false;
    }


    return true;
}


/*
 * ============================================================
 * MPU6050 / MPU6500 검색
 * ============================================================
 *
 * AD0 상태에 따라
 *
 * 0x68
 * 0x69
 *
 * 두 I2C Address를 순서대로 확인한다.
 */
static bool mpu6050_find_device(void)
{
    /*
     * AD0 = LOW
     *
     * 가장 일반적인 Address
     */
    if (mpu6050_check_address(MPU6050_ADDRESS_LOW))
    {
        return true;
    }


    /*
     * AD0 = HIGH
     */
    if (mpu6050_check_address(MPU6050_ADDRESS_HIGH))
    {
        return true;
    }


    /*
     * 두 Address에서 장치를 찾지 못함
     */
    device_address = 0U;


    return false;
}


/*
 * ============================================================
 * 비정상 데이터 검사
 * ============================================================
 *
 * 14byte가 전부
 *
 * 0x00
 *
 * 또는
 *
 * 0xFF
 *
 * 인 경우 정상적인 센서 데이터가 아닐 가능성이 높다고
 * 판단한다.
 */
static bool mpu6050_is_invalid_data(const uint8_t *data,
                                    uint16_t length)
{
    bool all_zero = true;
    bool all_ff = true;


    for (uint16_t i = 0U; i < length; i++)
    {
        /*
         * 하나라도 0이 아니면
         * 전체 0 상태가 아니다.
         */
        if (data[i] != 0x00U)
        {
            all_zero = false;
        }


        /*
         * 하나라도 FF가 아니면
         * 전체 FF 상태가 아니다.
         */
        if (data[i] != 0xFFU)
        {
            all_ff = false;
        }
    }


    return (all_zero || all_ff);
}


/*
 * ============================================================
 * MPU6050 초기화
 * ============================================================
 */
bool mpu6050_init(void)
{
    /*
     * 초기 상태 Reset
     */
    is_initialized = false;

    device_address = 0U;

    chip_id = 0U;

    current_status =
        MPU6050_STATUS_NOT_INITIALIZED;


    /*
     * MPU6050 / MPU6500 전원 안정화 대기
     */
    HAL_Delay(50U);


    /*
     * ========================================================
     * 1. I2C 장치 검색 + WHO_AM_I 확인
     * ========================================================
     */
    if (!mpu6050_find_device())
    {
        /*
         * I2C 응답은 받았지만
         * 지원하지 않는 WHO_AM_I가 나온 경우
         */
        if (chip_id != 0U)
        {
            current_status =
                MPU6050_STATUS_WHO_AM_I_ERROR;
        }

        /*
         * 아예 장치를 발견하지 못한 경우
         */
        else
        {
            current_status =
                MPU6050_STATUS_DEVICE_NOT_FOUND;
        }


        return false;
    }


    /*
     * ========================================================
     * 2. Device Reset
     * ========================================================
     *
     * PWR_MGMT_1
     *
     * bit 7 = DEVICE_RESET
     */
    if (mpu6050_write_register(
            MPU6050_REG_PWR_MGMT_1,
            0x80U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * Reset 완료 대기
     */
    HAL_Delay(100U);


    /*
     * ========================================================
     * 3. Sleep 해제 + Clock Source
     * ========================================================
     *
     * PWR_MGMT_1 = 0x01
     *
     * SLEEP = 0
     *
     * CLKSEL = 1
     *
     * X축 Gyroscope PLL을 Clock Source로 사용한다.
     */
    if (mpu6050_write_register(
            MPU6050_REG_PWR_MGMT_1,
            0x01U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * 모든 Accelerometer / Gyroscope 축 활성화
     */
    if (mpu6050_write_register(
            MPU6050_REG_PWR_MGMT_2,
            0x00U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * 설정 안정화 대기
     */
    HAL_Delay(10U);


    /*
     * ========================================================
     * 4. Gyroscope DLPF
     * ========================================================
     *
     * CONFIG = 0x03
     *
     * MPU6050
     * Gyroscope 약 42 Hz
     *
     * MPU6500
     * Gyroscope 약 41 Hz 수준
     *
     * 회전체에서 발생할 수 있는
     * 불필요한 높은 주파수 노이즈를 줄인다.
     */
    if (mpu6050_write_register(
            MPU6050_REG_CONFIG,
            0x03U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * ========================================================
     * 5. Sample Rate 설정
     * ========================================================
     *
     * DLPF 사용 시 기본 출력 주파수 = 1kHz
     *
     * Sample Rate =
     *
     * 1000 / (1 + SMPLRT_DIV)
     *
     * SMPLRT_DIV = 7
     *
     * 1000 / 8
     *
     * = 125 Hz
     *
     * 따라서 약 8ms마다 새로운 센서 데이터가 생성된다.
     */
    if (mpu6050_write_register(
            MPU6050_REG_SMPLRT_DIV,
            0x07U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * ========================================================
     * 6. Gyroscope Full Scale 설정
     * ========================================================
     *
     * GYRO_CONFIG = 0x00
     *
     * FS_SEL = 0
     *
     * ±250 deg/s
     *
     * 131 LSB = 1 deg/s
     */
    if (mpu6050_write_register(
            MPU6050_REG_GYRO_CONFIG,
            0x00U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * ========================================================
     * 7. Accelerometer Full Scale 설정
     * ========================================================
     *
     * ACCEL_CONFIG = 0x00
     *
     * ±2g
     *
     * 16384 LSB = 1g
     *
     * 작은 진동 변화까지 확인하기 위해
     * 우선 가장 민감한 ±2g 범위를 사용한다.
     */
    if (mpu6050_write_register(
            MPU6050_REG_ACCEL_CONFIG,
            0x08U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_CONFIG_ERROR;


        return false;
    }


    /*
     * ========================================================
     * 8. MPU6500 Accelerometer DLPF
     * ========================================================
     *
     * MPU6500에서는 Accelerometer DLPF가
     * ACCEL_CONFIG2에 따로 존재한다.
     *
     * WHO_AM_I = 0x70인 경우에만 설정한다.
     *
     * A_DLPF_CFG = 3
     *
     * Accelerometer 대역폭 약 41Hz 수준
     *
     * MPU6050에서는 이 Register를 설정하지 않는다.
     */
    if (chip_id == MPU6500_WHO_AM_I_VALUE)
    {
        if (mpu6050_write_register(
                MPU6500_REG_ACCEL_CONFIG2,
                0x03U) != HAL_OK)
        {
            current_status =
                MPU6050_STATUS_CONFIG_ERROR;


            return false;
        }
    }


    /*
     * ========================================================
     * 초기화 성공
     * ========================================================
     */
    is_initialized = true;


    current_status =
        MPU6050_STATUS_OK;


    return true;
}


/*
 * ============================================================
 * MPU6050 재초기화
 * ============================================================
 *
 * 통신 오류 등이 발생한 경우
 * 초기화 과정을 처음부터 다시 수행한다.
 */
bool mpu6050_reinit(void)
{
    return mpu6050_init();
}


/*
 * ============================================================
 * MPU6050 연결 상태 확인
 * ============================================================
 */
bool mpu6050_is_connected(void)
{
    uint8_t who_am_i = 0U;


    /*
     * 아직 Address를 찾지 못했다면
     * 다시 장치를 검색한다.
     */
    if (device_address == 0U)
    {
        return mpu6050_find_device();
    }


    /*
     * I2C ACK 확인
     */
    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            device_address,
            3U,
            MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return false;
    }


    /*
     * WHO_AM_I 읽기
     */
    if (mpu6050_read_register(
            MPU6050_REG_WHO_AM_I,
            &who_am_i) != HAL_OK)
    {
        return false;
    }


    /*
     * MPU6050 또는 MPU6500이면
     * 정상 연결 상태
     */
    return mpu6050_is_valid_chip_id(who_am_i);
}


/*
 * ============================================================
 * MPU6050 Data Read
 * ============================================================
 */
bool mpu6050_read(mpu6050_data_t *data)
{
    /*
     * ACCEL_XOUT_H(0x3B)부터
     * 총 14byte를 한 번에 읽는다.
     *
     * buffer[0]  = ACCEL_X H
     * buffer[1]  = ACCEL_X L
     *
     * buffer[2]  = ACCEL_Y H
     * buffer[3]  = ACCEL_Y L
     *
     * buffer[4]  = ACCEL_Z H
     * buffer[5]  = ACCEL_Z L
     *
     * buffer[6]  = TEMP H
     * buffer[7]  = TEMP L
     *
     * buffer[8]  = GYRO_X H
     * buffer[9]  = GYRO_X L
     *
     * buffer[10] = GYRO_Y H
     * buffer[11] = GYRO_Y L
     *
     * buffer[12] = GYRO_Z H
     * buffer[13] = GYRO_Z L
     */
    uint8_t buffer[14] = {0U};


    /*
     * ========================================================
     * 입력 Pointer 검사
     * ========================================================
     */
    if (data == 0)
    {
        current_status =
            MPU6050_STATUS_INVALID_ARGUMENT;


        return false;
    }


    /*
     * 읽기 성공 전까지는
     * 유효하지 않은 데이터로 설정한다.
     */
    data->is_valid = false;


    /*
     * ========================================================
     * 초기화 여부 확인
     * ========================================================
     */
    if (!is_initialized)
    {
        current_status =
            MPU6050_STATUS_NOT_INITIALIZED;


        return false;
    }


    /*
     * ========================================================
     * 14byte Burst Read
     * ========================================================
     *
     * 각 축을 각각 읽는 대신
     * 한 번의 I2C 통신으로 읽는다.
     *
     * 통신 횟수를 줄이고
     * 각 축 데이터를 비슷한 시점에 취득할 수 있다.
     */
    if (mpu6050_read_registers(
            MPU6050_REG_ACCEL_XOUT_H,
            buffer,
            14U) != HAL_OK)
    {
        current_status =
            MPU6050_STATUS_I2C_ERROR;


        return false;
    }


    /*
     * ========================================================
     * 비정상 데이터 검사
     * ========================================================
     */
    if (mpu6050_is_invalid_data(
            buffer,
            14U))
    {
        current_status =
            MPU6050_STATUS_DATA_ERROR;


        return false;
    }


    /*
     * ========================================================
     * Accelerometer Raw
     * ========================================================
     *
     * MPU6050 / MPU6500은
     * 상위 Byte → 하위 Byte 순서로 데이터를 전달한다.
     *
     * 따라서 두 byte를 하나의 int16_t로 합친다.
     */
    data->accel_x_raw =
        (int16_t)(
            ((uint16_t)buffer[0] << 8)
            | buffer[1]
        );


    data->accel_y_raw =
        (int16_t)(
            ((uint16_t)buffer[2] << 8)
            | buffer[3]
        );


    data->accel_z_raw =
        (int16_t)(
            ((uint16_t)buffer[4] << 8)
            | buffer[5]
        );


    /*
     * ========================================================
     * Temperature Raw
     * ========================================================
     *
     * A플랜:
     *
     * 내부 온도는 읽어서 보관하지만
     * 진동 분석에는 사용하지 않는다.
     */
    data->temp_raw =
        (int16_t)(
            ((uint16_t)buffer[6] << 8)
            | buffer[7]
        );


    /*
     * ========================================================
     * Gyroscope Raw
     * ========================================================
     */
    data->gyro_x_raw =
        (int16_t)(
            ((uint16_t)buffer[8] << 8)
            | buffer[9]
        );


    data->gyro_y_raw =
        (int16_t)(
            ((uint16_t)buffer[10] << 8)
            | buffer[11]
        );


    data->gyro_z_raw =
        (int16_t)(
            ((uint16_t)buffer[12] << 8)
            | buffer[13]
        );


    /*
     * ========================================================
     * Accelerometer 단위 변환
     * ========================================================
     *
     * 현재 ±2g
     *
     * 16384 LSB = 1g
     *
     * 단위 : g
     *
     * 이후 vibration.c에서
     * accel_x / accel_y / accel_z를 사용한다.
     */
    data->accel_x =
        (float)data->accel_x_raw
        / MPU6050_ACCEL_SCALE;


    data->accel_y =
        (float)data->accel_y_raw
        / MPU6050_ACCEL_SCALE;


    data->accel_z =
        (float)data->accel_z_raw
        / MPU6050_ACCEL_SCALE;


    /*
     * ========================================================
     * Temperature 단위 변환
     * ========================================================
     *
     * MPU6050과 MPU6500은
     * 내부 온도 변환식이 서로 다르다.
     *
     * 온도는 이번 프로젝트에서
     * 참고 / 디버깅용으로만 사용한다.
     */

    if (chip_id == MPU6500_WHO_AM_I_VALUE)
    {
        /*
         * MPU6500
         *
         * Temp =
         * Raw / 333.87 + 21
         */
        data->temp =
            ((float)data->temp_raw / 333.87f)
            + 21.0f;
    }
    else
    {
        /*
         * MPU6050
         *
         * Temp =
         * Raw / 340 + 36.53
         */
        data->temp =
            ((float)data->temp_raw / 340.0f)
            + 36.53f;
    }


    /*
     * ========================================================
     * Gyroscope 단위 변환
     * ========================================================
     *
     * 현재 ±250 deg/s
     *
     * 131 LSB = 1 deg/s
     *
     * 단위 : deg/s
     */
    data->gyro_x =
        (float)data->gyro_x_raw
        / MPU6050_GYRO_SCALE;


    data->gyro_y =
        (float)data->gyro_y_raw
        / MPU6050_GYRO_SCALE;


    data->gyro_z =
        (float)data->gyro_z_raw
        / MPU6050_GYRO_SCALE;


    /*
     * ========================================================
     * 데이터 읽기 성공
     * ========================================================
     */
    data->is_valid = true;


    current_status =
        MPU6050_STATUS_OK;


    return true;
}


/*
 * ============================================================
 * 현재 Driver Status 반환
 * ============================================================
 */
mpu6050_status_t mpu6050_get_status(void)
{
    return current_status;
}


/*
 * ============================================================
 * WHO_AM_I Chip ID 반환
 * ============================================================
 *
 * Live Watch 예상값
 *
 * MPU6050 → 104 = 0x68
 * MPU6500 → 112 = 0x70
 *
 * 현재 센서에서는
 *
 * 112 'p'
 *
 * 로 보이는 것이 정상이다.
 */
uint8_t mpu6050_get_chip_id(void)
{
    return chip_id;
}