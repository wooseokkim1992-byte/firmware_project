#pragma once

#include "main.h"
#include <stdbool.h>

#define DHT11_PIN GPIO_PIN_1
#define DHT11_PORT GPIOC

typedef struct _dht11Data_t{
    float temperature;
    float humidity;
    bool is_valid;
} dht11Data_t;

void dht11Init(void);
bool dht11Read(dht11Data_t* data);
float dht11GetTemperature(void);
float dht11GetHumidity(void);