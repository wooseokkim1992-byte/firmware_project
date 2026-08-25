#include "apMain.h"
#include "myHcSr04.h"
#include "myLcd1602.h"
#include "myAdc.h"
#include "myUart.h"
#include "myDht11.h"
#include "myI2c.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "myMpu6050.h"
#include "mySsd1306.h"
#include "myDs1302.h"
#include "tim.h"
#include "myGpio.h"


#include <stdint.h>
#include <stdio.h>
#include <string.h>


static mpu6050Data_t mpu_data={0};
extern ADC_HandleTypeDef hadc1;

  static ds1302Time_t rtc_time={0};

void apInit(void) { 
  uartInit();
  adcInit();
  dht11Init();
  lcd1602Init();
  i2cScan();
  mpu6050Init();
  //ssd1306Init();
  ds1302Init();



 
}

float internal_temp=0;
dht11Data_t dht_data={0};
bool dht_status=false;
float distance_cm=0.0f;

void apMain(void) {

  uint32_t tick_1000=0;
  uint32_t tick_250=0;
  uint32_t tick_100=0;
  uint32_t tick_50=0;
  uint32_t current_tick=0;

  // ssd1306Clear();
  // ssd1306DrawRect(0, 0, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_COLOR_WHITE);
  // ssd1306DrawString(8, 3, "STM32 MULTI-SENSOR", SSD1306_COLOR_WHITE);
  // ssd1306DrawLine(4, 13, 124,13, SSD1306_COLOR_WHITE);
  // ssd1306Update();

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start_IT(&htim4);

  while (1) {
    current_tick=HAL_GetTick();

    if(current_tick-tick_1000>=1000){
      tick_1000=current_tick;

      ds1302GetDateTime(&rtc_time);
      hcSr04Read(&distance_cm);


      //HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

      printf("sec : %d\r\n",rtc_time.sec);


    }

    if(current_tick-tick_250>=250){
      tick_250=current_tick;
      adcUpdate();
      dht_status=dht11Read(&dht_data);
      internal_temp=adcGetTemp();

      lcd1602Clear();
      lcd1602Cursor(0, 0);
      lcd1602Printf("Temp %.2f/%.2f", internal_temp,dht_data.temperature);
      lcd1602Cursor(1, 0);
      lcd1602Printf("Humi %.2f", dht_data.humidity);
    }

    if(current_tick-tick_100>=100){
      tick_100=current_tick;
      if(mpu6050Read(&mpu_data)){
        printf(">acc_x:%.3f\r\n>acc_y:%.3f\r\n>acc_z:%.3f\r\n>gyro_x:%.3f\r\n>gyro_y:%.3f\r\n>gyro_z:%.3f\r\n",
          mpu_data.accel_x,mpu_data.accel_y,mpu_data.accel_z,mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z
        );
      }

    }


    if(current_tick-tick_50>=50){
      tick_50=current_tick;
      //printf("dis : %6.1f\r\n",distance_cm);
    }

  }
}