#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define MPU6050_I2C_ADDR_7BIT     0x68
#define MPU6050_I2C_ADDR          (MPU6050_I2C_ADDR_7BIT << 1)

/* MPU-6050 Register Map */
#define MPU6050_RA_SMPLRT_DIV     0x19
#define MPU6050_RA_CONFIG         0x1A
#define MPU6050_RA_GYRO_CONFIG    0x1B
#define MPU6050_RA_ACCEL_CONFIG   0x1C
#define MPU6050_RA_ACCEL_XOUT_H   0x3B
#define MPU6050_RA_TEMP_OUT_H     0x41
#define MPU6050_RA_GYRO_XOUT_H    0x43
#define MPU6050_RA_PWR_MGMT_1     0x6B
#define MPU6050_RA_WHO_AM_I       0x75

#define MPU6050_WHO_AM_I_VAL      0x68

typedef struct _mpu6050Data_t {
    /* Raw 16-bit signed integer values */
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t temp_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    /* Converted physical units */
    float accel_x;   /* [g] */
    float accel_y;   /* [g] */
    float accel_z;   /* [g] */
    float temp;      /* [°C] */
    float gyro_x;    /* [deg/s or dps] */
    float gyro_y;    /* [deg/s or dps] */
    float gyro_z;    /* [deg/s or dps] */

    bool is_valid;
} mpu6050Data_t;

bool mpu6050Init(void);
bool mpu6050IsConnected(void);
bool mpu6050Read(mpu6050Data_t *data);
