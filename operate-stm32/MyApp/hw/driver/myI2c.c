#include "myI2c.h"
#include "i2c.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>

void i2cInit(void){

}

void i2cScan(void){
    uint8_t count=0;

    for (uint8_t i=1; i<128; i++) {
        HAL_StatusTypeDef result=HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 1, 10);
        if (result==HAL_OK){
            printf("Device Found at 7-bit addr : 0x%02X (HAL 8bit: 0x%02X)\n",i, (i<<1));
            count++;
        }
    }

    if(count==0){
        printf ("Not found\n");
    }
    else{
        printf("Total Device: %d\n", count);
    }
    
}