#include "myMpu6050.h"
#include "i2c.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>
#include <stdio.h>

static bool is_initialized=false;
static uint8_t chip_id=0;

static HAL_StatusTypeDef mpu6050_write_reg(uint8_t reg, uint8_t data){
    return HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

static HAL_StatusTypeDef mpu6050_read_reg(uint8_t reg, uint8_t *data){
    return HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

static HAL_StatusTypeDef mpu6050_read_regs(uint8_t reg, uint8_t *data, uint16_t len){
    return HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

static bool is_valid_chip_id(uint8_t id){
    return (id==0x68 || id==0x70 || id==0x71 || id==0x72 || id==0x73);
}


bool mpu6050Init(void) {
    is_initialized = false;

    HAL_Delay(50); /* 전원 인가 후 안정화 대기 */

    /* I2C 장치 응답 확인 */
    if (HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_I2C_ADDR, 3, 100) != HAL_OK) {
        printf("[MPU6050] Device not ready at I2C addr 0x%02X\r\n", MPU6050_I2C_ADDR_7BIT);
        return false;
    }

    /* WHO_AM_I 레지스터 확인 */
    uint8_t who_am_i = 0;
    if (mpu6050_read_reg(MPU6050_RA_WHO_AM_I, &who_am_i) != HAL_OK || !is_valid_chip_id(who_am_i)) {
        printf("[MPU6050] WHO_AM_I check failed! (Got: 0x%02X)\r\n", who_am_i);
        return false;
    }

    chip_id = who_am_i;

    /* 디바이스 리셋 */
    mpu6050_write_reg(MPU6050_RA_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    /* 1. 슬립 모드 해제 및 PLL (X축 자이로 레퍼런스) 클럭 설정 */
    if (mpu6050_write_reg(MPU6050_RA_PWR_MGMT_1, 0x01) != HAL_OK) {
        /* PLL 실패 시 내부 8MHz 오실레이터로 재시도 */
        if (mpu6050_write_reg(MPU6050_RA_PWR_MGMT_1, 0x00) != HAL_OK) {
            printf("[MPU6050] Wake up failed!\r\n");
            return false;
        }
    }
    HAL_Delay(10);

    /* 2. Sample Rate 분주 설정 (Sample Rate = 1kHz / (1 + 7) = 125Hz) */
    if (mpu6050_write_reg(MPU6050_RA_SMPLRT_DIV, 0x07) != HAL_OK) {
        return false;
    }

    /* 3. DLPF (디지털 로우패스 필터) 설정 (0x00: DLPF_CFG = 0, Accel 260Hz / Gyro 256Hz) */
    if (mpu6050_write_reg(MPU6050_RA_CONFIG, 0x00) != HAL_OK) {
        return false;
    }

    /* 4. 자이로스코프 Full-Scale Range 설정 (0x00: ±250 deg/s, 131 LSB/(deg/s)) */
    if (mpu6050_write_reg(MPU6050_RA_GYRO_CONFIG, 0x00) != HAL_OK) {
        return false;
    }

    /* 5. 가속도계 Full-Scale Range 설정 (0x00: ±2g, 16384 LSB/g) */
    if (mpu6050_write_reg(MPU6050_RA_ACCEL_CONFIG, 0x00) != HAL_OK) {
        return false;
    }

    is_initialized = true;
    const char *model_name = "MPU-6050";
    if (chip_id == 0x70) model_name = "MPU-6500";
    else if (chip_id == 0x71) model_name = "MPU-9250";
    else if (chip_id == 0x73) model_name = "MPU-9255";

    printf("[MPU6050] %s initialized successfully (WHO_AM_I: 0x%02X)\r\n", model_name, chip_id);
    return true;
}

bool mpu6050Read(mpu6050Data_t *data) {
    if (data == NULL) {
        return false;
    }

    if (!is_initialized) {
        data->is_valid = false;
        return false;
    }

    uint8_t buf[14] = {0};

    /* 0x3B(ACCEL_XOUT_H)부터 14바이트 연속 읽기 (Accel X,Y,Z, Temp, Gyro X,Y,Z) */
    if (mpu6050_read_regs(MPU6050_RA_ACCEL_XOUT_H, buf, 14) != HAL_OK) {
        data->is_valid = false;
        return false;
    }

    /* 16비트 Signed 정수 합성 (Big-Endian) */
    data->accel_x_raw = (int16_t)((buf[0] << 8) | buf[1]);
    data->accel_y_raw = (int16_t)((buf[2] << 8) | buf[3]);
    data->accel_z_raw = (int16_t)((buf[4] << 8) | buf[5]);
    data->temp_raw    = (int16_t)((buf[6] << 8) | buf[7]);
    data->gyro_x_raw  = (int16_t)((buf[8] << 8) | buf[9]);
    data->gyro_y_raw  = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z_raw  = (int16_t)((buf[12] << 8) | buf[13]);

    /* 물리 단위 변환 */
    data->accel_x = (float)data->accel_x_raw / 16384.0f;
    data->accel_y = (float)data->accel_y_raw / 16384.0f;
    data->accel_z = (float)data->accel_z_raw / 16384.0f;

    if (chip_id == 0x70 || chip_id == 0x71 || chip_id == 0x73) {
        /* MPU-6500 / MPU-9250 온도 변환 공식 */
        data->temp = ((float)data->temp_raw / 333.87f) + 21.0f;
    } else {
        /* MPU-6050 온도 변환 공식 */
        data->temp = ((float)data->temp_raw / 340.0f) + 36.53f;
    }

    data->gyro_x  = (float)data->gyro_x_raw / 131.0f;
    data->gyro_y  = (float)data->gyro_y_raw / 131.0f;
    data->gyro_z  = (float)data->gyro_z_raw / 131.0f;

    data->is_valid = true;
    return true;
}