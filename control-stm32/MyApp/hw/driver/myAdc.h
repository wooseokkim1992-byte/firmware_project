#pragma  once
#include "main.h"

void adcInit(void);
uint32_t Adc_Ch0(void);
uint32_t Adc_Ch1(void);
uint32_t Adc_Ch4(void);

void adcUpdate(void);
float adcGetTemp(void);